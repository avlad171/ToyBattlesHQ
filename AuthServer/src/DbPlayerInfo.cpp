
#include "../include/DbPlayerInfo.h"
#include "../include/Structures/AuthAccountInfo.h"
#include "../include/AuthEnums.h"
#include "../include/Network/Packet.h"
#include "../include/AuthServer.h"
#include "../include/AuthUtils.h"

#include <chrono>
#include <format>
#include <string>
#include <utility>
#include "Utils/Logger.h"
#include "Utils/Utils.h"
#include <Enums/PlayerEnums.h>
#include <cstdint>

std::string currentUtcTime()
{
	//auto now = std::chrono::utc_clock::now(); // on macos error: no member named 'utc_clock' in namespace 'std::chrono'
	auto now = std::chrono::system_clock::now();
	return std::format("{:%Y-%m-%d %H:%M:%S}", now);
}

namespace Auth
{
	namespace Persistence
	{
		PersistentDatabase::PersistentDatabase()
		{
			const auto& dbSetup = Common::Utils::SetupParser::getInstance().getDatabaseSetup();
			int retryCount = 0;
			const int maxRetries = 5;

			while (retryCount < maxRetries)
			{
				try
				{
					con = sql::mariadb::get_driver_instance()->connect("tcp://" + dbSetup.ip + ":" + std::to_string(dbSetup.port),
						dbSetup.username, dbSetup.password);
					con->setSchema(dbSetup.databaseName);

					::Utils::Logger::log("Connected to database at " + dbSetup.ip + ":" + std::to_string(dbSetup.port) + ", using database " + dbSetup.databaseName,
						::Utils::LogType::Info);
					return;  
				}
				catch (sql::SQLException& e)
				{
					++retryCount;
					::Utils::Logger::log("Error connecting to database: " + std::string(e.what()) + ", attempt " + std::to_string(retryCount),
						::Utils::LogType::Error);

					if (retryCount < maxRetries)
					{
						std::this_thread::sleep_for(std::chrono::seconds(2 * retryCount));  
					}
					else
					{
						::Utils::Logger::log("Max reconnection attempts reached, giving up.",
							::Utils::LogType::Error);
						throw;  
					}
				}
			}
		}

		bool PersistentDatabase::updateLastLoggedNow(std::uint32_t aid, const std::string& ipHash, const std::string& ipSalt)
		{
			try
			{
				bool columnExists = false;
				std::string checkColumnQuery = "SHOW COLUMNS FROM Users LIKE 'LastIpSalt'";
				std::unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement(checkColumnQuery));
				std::unique_ptr<sql::ResultSet> checkRes(checkStmt->executeQuery());

				if (checkRes->next()) columnExists = true;

				if (!columnExists) 
				{
					std::string alterQueryStr = "ALTER TABLE Users ADD COLUMN LastIpSalt VARCHAR(128) NULL";
					std::unique_ptr<sql::PreparedStatement> alterStmt(con->prepareStatement(alterQueryStr));
					alterStmt->executeUpdate();
				}

				std::string updateQueryStr = "UPDATE Users SET LastLogged = ?, LastIP = ?, LastIpSalt = ? WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> updateStmt(con->prepareStatement(updateQueryStr));

				updateStmt->setString(1, currentUtcTime());
				updateStmt->setString(2, ipHash);
				updateStmt->setString(3, ipSalt);
				updateStmt->setUInt(4, aid);

				int rowsAffected = updateStmt->executeUpdate();
				return rowsAffected > 0;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),::Utils::LogType::Error, "PersistentDatabase::updateLastLoggedNow");
				return false;
			}
		}

		bool PersistentDatabase::logGameEvent(const std::string& logType, const std::string& message, const std::string& severity)
		{
			try
			{
				std::string createTableQuery = R"(CREATE TABLE IF NOT EXISTS GameLogs (ID INT AUTO_INCREMENT PRIMARY KEY, LogType VARCHAR(255) NOT NULL,
					Message TEXT NOT NULL, Severity VARCHAR(20) NOT NULL, CreatedAt TIMESTAMP DEFAULT CURRENT_TIMESTAMP))";

				std::unique_ptr<sql::Statement> stmtCreate(con->createStatement());
				stmtCreate->execute(createTableQuery);

				std::string insertQuery = "INSERT INTO GameLogs (LogType, Message, Severity) VALUES (?, ?, ?)";
				std::unique_ptr<sql::PreparedStatement> stmtInsert(con->prepareStatement(insertQuery));

				stmtInsert->setString(1, logType);
				stmtInsert->setString(2, message);
				stmtInsert->setString(3, severity);

				return stmtInsert->executeUpdate() > 0;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()),::Utils::LogType::Error,"PersistentDatabase::logGameEvent");
				return false;
			}
		}

		void PersistentDatabase::addHash(std::uint32_t accountID, std::uint32_t key)
		{
			try
			{
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement("UPDATE Users SET AccountKey = ? WHERE AccountID = ?"));
				stmt->setUInt(1, key);
				stmt->setUInt(2, accountID);
				stmt->executeUpdate();
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::addHash");
			}
		}

		std::expected<Auth::Structures::BasicAccountInfo, Auth::Enums::Login> PersistentDatabase::getCompletePlayerInfo(const std::string& username)
		{
			Auth::Structures::BasicAccountInfo playerInfoStructure{};

			const std::string queryStr = "SELECT Users.*, Clans.Clanname, Clans.ClanFrontIcon, Clans.ClanBackIcon "
				"FROM Users LEFT JOIN Clans ON Users.ClanID = Clans.ClanId WHERE Username = ?";

			try
			{
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(queryStr));
				stmt->setString(1, username);

				std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
				if (res->next())
				{
					playerInfoStructure.grade = static_cast<std::uint32_t>(res->getInt("Grade"));
					playerInfoStructure.encryptedEmail = res->getString("Email").c_str();
					playerInfoStructure.secret = res->getString("Secret").c_str();
					playerInfoStructure.hashedPassword = res->getString("Password").c_str();
					playerInfoStructure.suspendedUntil = res->getString("SuspendedUntil").c_str();
					playerInfoStructure.ainfoClient.accountId = static_cast<std::uint32_t>(res->getInt("AccountID"));
					std::strncpy(playerInfoStructure.ainfoClient.playerName, res->getString("Nickname").c_str(), sizeof(playerInfoStructure.ainfoClient.playerName) - 1);
					playerInfoStructure.ainfoClient.playerName[sizeof(playerInfoStructure.ainfoClient.playerName) - 1] = '\0';
					std::strncpy(playerInfoStructure.ainfoClient.clanName, res->getString("Clanname").c_str(), sizeof(playerInfoStructure.ainfoClient.clanName) - 1);
					playerInfoStructure.ainfoClient.clanName[sizeof(playerInfoStructure.ainfoClient.clanName) - 1] = '\0';
					playerInfoStructure.grade = static_cast<std::uint32_t>(res->getInt("Grade"));
					playerInfoStructure.ainfoClient.level = static_cast<std::uint32_t>(res->getInt("Level")) + 1;
					playerInfoStructure.ainfoClient.exp = static_cast<std::uint32_t>(res->getInt("Experience"));
					playerInfoStructure.ainfoClient.kills = static_cast<std::uint32_t>(res->getInt("Kills"));
					playerInfoStructure.ainfoClient.deaths = static_cast<std::uint32_t>(res->getInt("Deaths"));
					playerInfoStructure.ainfoClient.assists = static_cast<std::uint32_t>(res->getInt("Assists"));
					playerInfoStructure.ainfoClient.wins = static_cast<std::uint32_t>(res->getInt("Wins"));
					playerInfoStructure.ainfoClient.losses = static_cast<std::uint32_t>(res->getInt("Loses"));
					playerInfoStructure.ainfoClient.draws = static_cast<std::uint32_t>(res->getInt("Draws"));
					playerInfoStructure.ainfoClient.clanIconFrontID = static_cast<std::uint16_t>(res->getInt("ClanFrontIcon"));
					playerInfoStructure.ainfoClient.clanIconBackID = static_cast<std::uint16_t>(res->getInt("ClanBackIcon"));

					return playerInfoStructure;
				}
				else
				{
					return std::unexpected(Auth::Enums::Login::INCORRECT);
				}
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception: " + std::string(e.what()), ::Utils::LogType::Error, "PersistentDatabase::getPlayerInfo");
				return std::unexpected(Auth::Enums::Login::DATA_ERROR);
			}
		}

		bool PersistentDatabase::removeGradeAndSuspend(std::uint32_t accountId, bool isGraded)
		{
			using namespace std::chrono;
			using namespace std::literals;

			try
			{
				std::string checkGradeQuery = "SELECT Grade FROM Users WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> checkStmt(con->prepareStatement(checkGradeQuery));
				checkStmt->setUInt(1, accountId);

				sql::ResultSet* res(checkStmt->executeQuery());
				if (!res->next())
				{
					return false;
				}

				std::string updateQuery = "UPDATE Users SET SuspendedUntil = ?, SuspensionReason = ?, Grade = ? WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> updateStmt(con->prepareStatement(updateQuery));

				//zoned_time zt{ "UTC", local_seconds{duration_cast<seconds>(system_clock::now().time_since_epoch()) + seconds(9999 * 24 * 60 * 60)} };
				const std::string bannedUntil = currentUtcTime(); //std::format("{:%Y-%m-%d %H:%M:%S}", zt.get_sys_time());

				updateStmt->setString(1, bannedUntil);
				updateStmt->setString(2, isGraded ? "GRADED_TOO_MANY_FAILED_LOGIN_ATTEMPTS" : "UNGRADED_TOO_MANY_FAILED_LOGIN_ATTEMPTS");
				updateStmt->setUInt(3, 1);
				updateStmt->setUInt(4, accountId);

				if (updateStmt->executeUpdate() == 0)
				{
					::Utils::Logger::log("Error executing query: " + updateQuery, ::Utils::LogType::Warning, "PersistentDatabase::removeGradeAndSuspend");
					return false;
				}
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log(std::string("MariaDB exception: ") + e.what(), ::Utils::LogType::Error, "PersistentDatabase::removeGradeAndSuspend");
				return false;
			}
			return true;
		}

		bool PersistentDatabase::getGradedHwid(std::uint32_t accountId, std::string& outHash, std::string& outSalt) const
		{
			try
			{
				std::string query = "SELECT HWIDGraded, HWIDGradedSalt FROM Users WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(query));
				stmt->setUInt(1, accountId);

				std::unique_ptr<sql::ResultSet> res(stmt->executeQuery());
				if (res->next())
				{
					outHash = res->getString("HWIDGraded").c_str();
					outSalt = res->getString("HWIDGradedSalt").c_str();
					return true;
				}

				return false;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception in getGradedHwid: " + std::string(e.what()), ::Utils::LogType::Error);
				return false;
			}
		}

		bool PersistentDatabase::setGradedHwid(std::uint32_t accountId, const std::string& hash, const std::string& salt)
		{
			try
			{
				std::string query =
					"UPDATE Users SET HWIDGraded = IF(HWIDGraded IS NULL OR HWIDGraded = '', ?, HWIDGraded), "
					"HWIDGradedSalt = IF(HWIDGradedSalt IS NULL OR HWIDGradedSalt = '', ?, HWIDGradedSalt) "
					"WHERE AccountID = ?";

				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(query));
				stmt->setString(1, hash);
				stmt->setString(2, salt);
				stmt->setUInt(3, accountId);

				return stmt->executeUpdate() > 0;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception in setGradedHwid: " + std::string(e.what()), ::Utils::LogType::Error);
				return false;
			}
		}

		bool PersistentDatabase::updateCurrentHwid(std::uint32_t accountId, const std::string& hash, const std::string& salt)
		{
			try
			{
				std::string query = "UPDATE Users SET HWID = ?, HWIDSalt = ? WHERE AccountID = ?";
				std::unique_ptr<sql::PreparedStatement> stmt(con->prepareStatement(query));

				stmt->setString(1, hash);
				stmt->setString(2, salt);
				stmt->setUInt(3, accountId);

				return stmt->executeUpdate() > 0;
			}
			catch (const sql::SQLException& e)
			{
				::Utils::Logger::log("MariaDB exception in updateCurrentHwid: " + std::string(e.what()), ::Utils::LogType::Error);
				return false;
			}
		}
	};
}







