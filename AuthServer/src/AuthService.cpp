#include "../include/AuthService.h"
#include <string>
#include "Utils.h"
#include "Utils/SetupParser.h"
#include "AuthUtils.h"
#include <libcppotp/auth.h>
#include <openssl/rand.h>
#include <cryptopp/osrng.h>
#include <cryptopp/hex.h>
#include <libbcrypt/include/bcrypt/BCrypt.hpp>

namespace Auth
{
	using namespace Common::Utils;

	Auth::Enums::Login AuthService::authorizeGraded(const Auth::Structures::BasicAccountInfo& ainfo, const std::optional<std::string>& token,
		const std::string& plainPw, const std::string& plainIp, const std::string& plainHwid)
	{
		if (ainfo.secret.empty())
		{
			m_persistentDatabase.logGameEvent("AuthGradedLogin",
				"Failed login: missing mandatory secret for graded account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
			return Auth::Enums::Login::INCORRECT;
		}

		auto& authSetup = Common::Utils::SetupParser::getInstance().getAuthSetup();

		if (authSetup.enhancedSecurity)
		{
			asio::ip::network_v4 vpnNet = asio::ip::make_network_v4(authSetup.gradedAccessSubnet);
			std::error_code ec;
			auto clientIp = asio::ip::make_address_v4(plainIp, ec);

			if (plainIp.empty() || ec)
			{
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Failed login: No PlainIp found for graded account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
				return Auth::Enums::Login::INCORRECT;
			}

			if (!isIpInSubnet(vpnNet, clientIp))
			{
				if (!Common::Utils::sendEmailAlert("[MEDIUM Alert] TB - Graded Login Wrong VPN IP", "High Level Alert: Graded Account(ID: " 
						+ std::to_string(ainfo.ainfoClient.accountId) + ") accessed with the wrong VPN IP, access was blocked"))
				{
					m_persistentDatabase.logGameEvent("EmailGraded",
						"Failed to send admin notification after wrong IP subnet at login for graded accountID: " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
				}

				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Failed login: IP " + plainIp + " not in allowed subnet for graded account " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
				return Auth::Enums::Login::INCORRECT;
			}

			if (plainHwid.empty())
			{
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Failed login: missing HWID for graded account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
				return Auth::Enums::Login::INCORRECT;
			}

			std::string dbGradedHash, dbGradedSalt;
			if (!m_persistentDatabase.getGradedHwid(ainfo.ainfoClient.accountId, dbGradedHash, dbGradedSalt))
			{
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Failed login: unable to retrieve graded HWID from DB for graded account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
				return Auth::Enums::Login::INCORRECT;
			}

			if (dbGradedHash.empty() || dbGradedSalt.empty())
			{
				// Register first-time HWID
				dbGradedSalt = generateRandomSalt();
				dbGradedHash = Common::Utils::hashSha256(plainHwid, dbGradedSalt);
				if (!m_persistentDatabase.setGradedHwid(ainfo.ainfoClient.accountId, dbGradedHash, dbGradedSalt))
				{
					m_persistentDatabase.logGameEvent("AuthGradedLogin",
						"Failed login: could not set first-time graded HWID for account " + std::to_string(ainfo.ainfoClient.accountId), "LOW");
					return Auth::Enums::Login::INCORRECT;
				}
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Registered first-time graded HWID for account " + std::to_string(ainfo.ainfoClient.accountId), "LOW");
			}
			else
			{
				if (Common::Utils::hashSha256(plainHwid, dbGradedSalt) != dbGradedHash)
				{
					if (!Common::Utils::sendEmailAlert("[HIGH Alert] TB - Graded Login Wrong HWID", "High Level Alert: Graded Account(ID: " 
							+ std::to_string(ainfo.ainfoClient.accountId) + ") accessed with the wrong HWID, access was blocked"))
					{				
						m_persistentDatabase.logGameEvent("EmailGraded",
							"Failed to send admin notification after wrong HWID at login for graded accountID: " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
					}
					m_persistentDatabase.logGameEvent("AuthGradedLogin",
						"Failed login: HWID mismatch for graded account " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
					return Auth::Enums::Login::INCORRECT;
				}
			}

			// Update current HWID
			std::string currentSalt = generateRandomSalt();
			std::string currentHwidHash = Common::Utils::hashSha256(plainHwid, currentSalt);
			if (!m_persistentDatabase.updateCurrentHwid(ainfo.ainfoClient.accountId, currentHwidHash, currentSalt))
			{
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Failed login: could not update current HWID for graded account " + std::to_string(ainfo.ainfoClient.accountId), "LOW");
				return Auth::Enums::Login::INCORRECT;
			}
		}

		const bool passwordOk = BCrypt::validatePassword(plainPw, ainfo.hashedPassword);
		const bool tokenOk = verifyToken(ainfo.secret, token);

		auto& counters = m_badLoginAttempts[ainfo.ainfoClient.accountId];
		if (!passwordOk)
		{
			++counters.totalWrongPasswords;
			m_persistentDatabase.logGameEvent("AuthGradedLogin",
				"Failed login: incorrect password for account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
		}
		if (!tokenOk)
		{
			++counters.totalWrong2fas;
			m_persistentDatabase.logGameEvent("AuthGradedLogin",
				"Failed login: invalid 2FA token for account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
		}

		constexpr std::uint32_t MAX_FAILED_ATTEMPTS = 3;
		if (counters.totalWrongPasswords >= MAX_FAILED_ATTEMPTS || counters.totalWrong2fas >= MAX_FAILED_ATTEMPTS)
		{
			if (!tryLockAccount(ainfo.ainfoClient.accountId, true))
			{
				m_persistentDatabase.logGameEvent("AuthGradedLogin",
					"Account lock attempt failed for account " + std::to_string(ainfo.ainfoClient.accountId), "CRITICAL");

				if (!Common::Utils::sendEmailAlert("[CRITICAL Alert] TB - Graded Account LOCK FAIL!", "Critical Level Alert: Graded Account(ID: "
						+ std::to_string(ainfo.ainfoClient.accountId) + ") coult NOT BE LOCKED after too many wrong login attempts, lock the account manually!"))
				{
					m_persistentDatabase.logGameEvent("EmailGraded",
						"Failed to send admin notification after not being able to lock graded account after too many failed login attempts, accountID: "
						+ std::to_string(ainfo.ainfoClient.accountId), "HIGH");
				}

				return Auth::Enums::Login::INCORRECT;
			}

			if (!Common::Utils::sendEmailAlert("[CRITICAL Alert] TB - Graded Account LOCKED!", "Critical Level Alert: Graded Account(ID: "
					+ std::to_string(ainfo.ainfoClient.accountId) + ") was locked due to too many wrong login attempts, see logs in the database"))
			{
				m_persistentDatabase.logGameEvent("EmailGraded",
					"Failed to send admin notification after successfully locking graded account after too many failed login attempts, accountID: "
					+ std::to_string(ainfo.ainfoClient.accountId), "HIGH");
			}

			m_persistentDatabase.logGameEvent("AuthGradedLogin",
				"Account locked due to repeated failed login attempts: " + std::to_string(ainfo.ainfoClient.accountId), "CRITICAL");

			m_badLoginAttempts.erase(ainfo.ainfoClient.accountId);
			return Auth::Enums::Login::INCORRECT;
		}

		if (!passwordOk || !tokenOk)
		{
			return Auth::Enums::Login::INCORRECT;
		}

		m_badLoginAttempts.erase(ainfo.ainfoClient.accountId);

		const std::string currentTime = std::format("{:%Y-%m-%d %X}", std::chrono::system_clock::now());
		if (ainfo.suspendedUntil > currentTime)
		{
			m_persistentDatabase.logGameEvent("AuthGradedLogin",
				"Login attempt on suspended account " + std::to_string(ainfo.ainfoClient.accountId), "MEDIUM");
			return Auth::Enums::Login::SUSPENDED;
		}

		return Auth::Enums::Login::SUCCESS;
	}


	Auth::Enums::Login AuthService::authorizeUngraded(const Auth::Structures::BasicAccountInfo& ainfo,const std::optional<std::string>& token,const std::string& plainPw)
	{
		const bool enhancedSecurity = Common::Utils::SetupParser::getInstance().getAuthSetup().enhancedSecurity;
		const bool passwordOk = BCrypt::validatePassword(plainPw, ainfo.hashedPassword);

		bool tokenOk = true;
		if (ainfo.secret.empty())
		{
			if (enhancedSecurity)
			{
				m_persistentDatabase.logGameEvent("AuthUngradedLogin",
					"Failed login: missing mandatory secret for ungraded account " + std::to_string(ainfo.ainfoClient.accountId),"MEDIUM");
				return Auth::Enums::Login::INCORRECT;
			}
		}
		if (!ainfo.secret.empty())
		{
			tokenOk = verifyToken(ainfo.secret, token);
		}

		auto& counters = m_badLoginAttempts[ainfo.ainfoClient.accountId];
		if (!passwordOk)
		{
			++counters.totalWrongPasswords;
			m_persistentDatabase.logGameEvent("AuthUngradedLogin",
				"Failed login: incorrect password for ungraded account " + std::to_string(ainfo.ainfoClient.accountId),"LOW");
		}
		if (!tokenOk)
		{
			++counters.totalWrong2fas;
			m_persistentDatabase.logGameEvent("AuthUngradedLogin",
				"Failed login: invalid 2FA token for ungraded account " + std::to_string(ainfo.ainfoClient.accountId),"LOW");
		}

		constexpr std::uint32_t MAX_2FA_ATTEMPTS = 5;
		
		if (counters.totalWrong2fas >= MAX_2FA_ATTEMPTS && enhancedSecurity)
		{
			if (auto decrypted = Common::Utils::decryptEmail(ainfo.encryptedEmail, Common::Utils::SetupParser::getInstance().getGeneralSetup().emailSecret))
			{
				Common::Utils::sendEmail(decrypted.value(), "[Security] Your ToyBattles account was banned", "This is an automatic notification. " 
					"For security reasons, your ToyBattles account was suspended due to 5 wrong login attempts in the game (5 wrong 2FA tokens used");
			}
			else
			{
				m_persistentDatabase.logGameEvent("EmailNotification",
					"Failed to send email after the account was locked due to too many 2FA wrong attempts for accountID: " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
			}
			if (!tryLockAccount(ainfo.ainfoClient.accountId, false))
			{
				m_persistentDatabase.logGameEvent("AuthUngradedLogin",
					"Account lock attempt failed for ungraded account " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");
				return Auth::Enums::Login::INCORRECT;
			}

			m_persistentDatabase.logGameEvent("AuthUngradedLogin",
				"Account locked due to repeated failed 2FA attempts: " + std::to_string(ainfo.ainfoClient.accountId), "HIGH");

			m_badLoginAttempts.erase(ainfo.ainfoClient.accountId);
			return Auth::Enums::Login::INCORRECT;
		}

		if (!passwordOk || !tokenOk)
		{
			return Auth::Enums::Login::INCORRECT;
		}

		m_badLoginAttempts.erase(ainfo.ainfoClient.accountId);

		const std::string currentTime = std::format("{:%Y-%m-%d %X}", std::chrono::system_clock::now());
		if (ainfo.suspendedUntil > currentTime)
		{
			m_persistentDatabase.logGameEvent("AuthUngradedLogin",
				"Login attempt on suspended ungraded account " + std::to_string(ainfo.ainfoClient.accountId), "LOW");
			return Auth::Enums::Login::SUSPENDED;
		}

		return Auth::Enums::Login::SUCCESS;
	}

	std::string AuthService::generateRandomSalt(std::size_t length) const
	{
		CryptoPP::AutoSeededRandomPool rng;
		std::string saltBytes(length, 0);
		rng.GenerateBlock(reinterpret_cast<CryptoPP::byte*>(&saltBytes[0]), saltBytes.size());

		std::string hex;
		CryptoPP::HexEncoder encoder;
		encoder.Attach(new CryptoPP::StringSink(hex));
		encoder.Put(reinterpret_cast<const CryptoPP::byte*>(saltBytes.data()), saltBytes.size());
		encoder.MessageEnd();

		return hex; 
	}

	bool AuthService::isIpInSubnet(const asio::ip::network_v4& network, const asio::ip::address_v4& ip) const
	{
		asio::ip::network_v4 test_network(ip, network.prefix_length());
		return network.canonical().address() == test_network.canonical().address();
	}

	bool AuthService::verifyToken(const std::string& encryptedSecret, const std::optional<std::string>& token)
	{
		if (encryptedSecret.empty() || !token.has_value()) return false;

		auto optSecret = Common::Utils::decrypt2FASecret(encryptedSecret, Common::Utils::SetupParser::getInstance().getGeneralSetup().twoFaSecret);
		if (!optSecret) return false;

		const int t_interval = 30;
		std::time_t now = std::time(nullptr);

		for (int i = -1; i <= 1; ++i)
		{
			auto expectedToken = auth::generateToken(optSecret.value(), now + i * t_interval, t_interval);
			std::ostringstream oss;
			oss << std::setw(6) << std::setfill('0') << expectedToken;

			if (oss.str() == token.value()) return true;
		}

		return false;
	}

	bool AuthService::tryLockAccount(std::uint32_t accountId, bool isGraded) 
	{
		constexpr int MAX_RETRIES = 3;
		bool locked = false;
		for (int i = 0; i < MAX_RETRIES; ++i)
		{
			locked = m_persistentDatabase.removeGradeAndSuspend(accountId, isGraded);
			if (locked) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

		return locked;
	}

	std::uint32_t AuthService::generateAccountKey() const
	{
		std::uint32_t value = 0;
		if (RAND_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(value)) != 1) {
			// entropy source not available => no cryptographically secure random bytes generated
			return 0; // account key 0 is default, main server won't accept it
		}
		return value;
	}

	Auth::Structures::Parsed2faLogin AuthService::parseUsernameAnd2FA(const std::string& rawUsername) const
	{
		Auth::Structures::Parsed2faLogin res{};
		const auto pos = rawUsername.find('+');

		if (pos == std::string::npos)
		{
			res.username = rawUsername;
			return res;
		}

		res.username = rawUsername.substr(0, pos);
		res.token = rawUsername.substr(pos + 1);

		if (res.token->size() != 6 || !std::all_of(res.token->begin(), res.token->end(), ::isdigit))
		{
			res.token->clear(); 
		}

		return res;
	}


	std::expected<Auth::Structures::BasicAccountInfo, Auth::Enums::Login>
		AuthService::login(const std::string& username, const std::string& password, const std::string& plainIp, const std::string& plainHwid)
	{
		const auto parsed = parseUsernameAnd2FA(username);
		const auto result = m_persistentDatabase.getCompletePlayerInfo(parsed.username);
		if (!result) return std::unexpected(result.error());

		Auth::Structures::BasicAccountInfo userInfo = result.value();
		Auth::Enums::Login authResult = (userInfo.grade >= 3)
			? authorizeGraded(userInfo, parsed.token, password, plainIp, plainHwid)
			: authorizeUngraded(userInfo, parsed.token, password);

		if (authResult != Auth::Enums::SUCCESS)
		{
			m_persistentDatabase.addHash(userInfo.ainfoClient.accountId, 0); // AccountKey=0 signals failure to MainServer
			return std::unexpected(authResult);
		}

		userInfo.ainfoClient.hashKey = generateAccountKey();
		m_persistentDatabase.addHash(userInfo.ainfoClient.accountId, userInfo.ainfoClient.hashKey);

		std::string ipSalt = generateRandomSalt();
		if (!m_persistentDatabase.updateLastLoggedNow(userInfo.ainfoClient.accountId, Common::Utils::hashSha256(plainIp, ipSalt), ipSalt))
		{
			::Utils::Logger::log("Failed login: could not update LastLogged for account " + std::to_string(userInfo.ainfoClient.accountId),
				::Utils::LogType::Error, "AuthSession::onPacket");
			return std::unexpected(Auth::Enums::Login::INCORRECT);
		}

		return userInfo;
	}
}