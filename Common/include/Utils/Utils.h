#ifndef GENERAL_UTILS_COMMON_H
#define GENERAL_UTILS_COMMON_H

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/base64.h>
#include <cryptopp/aes.h>
#include <cryptopp/gcm.h>
#include <cryptopp/filters.h>
#include <cryptopp/hex.h>
#include <cryptopp/base64.h>
#include <cryptopp/secblock.h>
#include <boost/json.hpp>
#include <curl/curl.h>
#include <random>

#include <limits>
#include <utility>
#include "../../include/ConstantDatabase/Structures/SetItemInfo.h"
#include "../Enums/GameEnums.h"
#include <vector>
#include "../Network/Session.h"
#include "SetupParser.h"

#include <chrono>
#include <string>
#include <sstream>
#include <iomanip>
#include <cstdint>
#include <format> 



#undef ENABLE_DEBUG_MESSAGES

#ifdef ENABLE_DEBUG_MESSAGES
#define SEND_DEBUG_MESSAGE(msg, session) Common::Utils::sendDebugMessage(msg, session)
#else
#define SEND_DEBUG_MESSAGE(msg, session) (void)0
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#endif



namespace Common
{
	namespace Utils
	{
		using namespace CryptoPP;
		namespace json = boost::json;

		inline std::optional<std::string> getLocalIp()
		{
			try
			{
				asio::io_context ioContext;
				asio::ip::udp::resolver resolver(ioContext);
				asio::ip::udp::socket socket(ioContext);
				socket.connect(asio::ip::udp::endpoint(asio::ip::make_address("8.8.8.8"), 53));
				return socket.local_endpoint().address().to_string();
			}
			catch (std::exception& e)
			{
				return std::nullopt;
			}
		}

		template<typename HashType>
		std::string calculateHashCryptoPP(const std::string& input)
		{
			HashType hash;
			std::string result;

			CryptoPP::byte digest[HashType::DIGESTSIZE];
			hash.Update(reinterpret_cast<const CryptoPP::byte*>(input.data()), input.size());
			hash.Final(digest);

			CryptoPP::HexEncoder encoder;
			encoder.Attach(new CryptoPP::StringSink(result));
			encoder.Put(digest, sizeof(digest));
			encoder.MessageEnd();

			std::transform(result.begin(), result.end(), result.begin(),
				[](unsigned char c) { return std::tolower(c); });

			return result;
		}

		inline std::vector<Common::Enums::ItemType> getPartTypesWhereSetItemInfoTypeNotNull(const Common::ConstantDatabase::SetItemInfo& entry,
			std::uint64_t character)
		{
			std::vector<Common::Enums::ItemType> itemTypes;
			itemTypes.reserve(10);
			if (entry.si_hair != -1) itemTypes.push_back(Common::Enums::HAIR);
			if (entry.si_face != -1) itemTypes.push_back(Common::Enums::FACE);
			if (entry.si_top != -1) itemTypes.push_back(Common::Enums::DRESS);
			if (entry.si_under != -1) itemTypes.push_back(Common::Enums::LEGS);
			if (entry.si_pants != -1) itemTypes.push_back(Common::Enums::SKIRT);
			if (entry.si_boots != -1) itemTypes.push_back(Common::Enums::BOOTS);
			if (entry.si_arms != -1) itemTypes.push_back(Common::Enums::GLOVES);
			if (entry.si_acce_A != -1) itemTypes.push_back(Common::Enums::ACC_UPPER);
			if (entry.si_acce_B != -1) itemTypes.push_back(Common::Enums::ACC_BACK); 
			if (entry.si_acce_C != -1) itemTypes.push_back(Common::Enums::ACC_WAIST); 
			return itemTypes;
		}

		inline std::uint64_t getCurrentTimestampMs()
		{
			using namespace std::chrono;
			return static_cast<std::uint64_t>(
				duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count()
				);
		}
		

		inline void setConsoleTitle(const std::wstring& title) 
		{
		#ifdef _WIN32
		    SetConsoleTitleW(title.c_str());
		#else
		    std::string utf8title(title.begin(), title.end());
		    std::cout << "\033]0;" << utf8title << "\007";
		#endif
		}

		inline std::string hexEncode(const std::vector<byte>& data)
		{
			std::string out;
			HexEncoder encoder(new StringSink(out), false);
			encoder.Put(data.data(), data.size());
			encoder.MessageEnd();
			return out;
		}

		inline std::vector<byte> hexDecode(const std::string& hex) 
		{
			std::vector<byte> out(hex.size() / 2);
			HexDecoder decoder;
			decoder.Put((byte*)hex.data(), hex.size());
			decoder.MessageEnd();
			decoder.Get(out.data(), out.size());
			return out;
		}

		inline std::optional<std::tuple<std::vector<byte>, std::vector<byte>, std::vector<byte>>> encryptAESGCM(const SecByteBlock& key, const std::string& plaintext)
		{
			try
			{
				AutoSeededRandomPool prng;
				std::vector<byte> iv(12);
				prng.GenerateBlock(iv.data(), iv.size());

				GCM<AES>::Encryption encrypt;
				encrypt.SetKeyWithIV(key, key.size(), iv.data(), iv.size());

				std::string cipherWithTag;
				AuthenticatedEncryptionFilter ef(encrypt,new StringSink(cipherWithTag),false,16                  );

				ef.ChannelPut("AAD", nullptr, 0);
				ef.ChannelPut("", reinterpret_cast<const byte*>(plaintext.data()), plaintext.size());
				ef.ChannelMessageEnd("");

				const size_t tagSize = 16;
				std::vector<byte> ct(cipherWithTag.begin(), cipherWithTag.end() - tagSize);
				std::vector<byte> tag(cipherWithTag.end() - tagSize, cipherWithTag.end());

				return std::make_tuple(iv, tag, ct);
			}
			catch (...)
			{
				return std::nullopt;
			}
		}

		inline std::optional<std::string> decryptAESGCM(const SecByteBlock& key, const std::vector<byte>& iv, const std::vector<byte>& tag, const std::vector<byte>& ct) 
		{
			GCM<AES>::Decryption decrypt;
			decrypt.SetKeyWithIV(key, key.size(), iv.data(), iv.size());

			std::string recovered;
			try 
			{
				AuthenticatedDecryptionFilter df(decrypt, new StringSink(recovered), AuthenticatedDecryptionFilter::THROW_EXCEPTION, tag.size());
				df.ChannelPut("AAD", nullptr, 0); 
				df.ChannelPut("", ct.data(), ct.size());
				df.ChannelPut("", tag.data(), tag.size());
				df.ChannelMessageEnd("");
			}
			catch (const Exception& e) 
			{
				return std::nullopt;
			}
			return recovered;
		}

		inline std::optional<std::string> decryptEmail(const std::string& stored,const SecByteBlock& key)
		{
			const std::string PREFIX = "email:gcm:v1:";

			if (stored.rfind(PREFIX, 0) != 0)
				return stored;

			std::string jsonStr;
			StringSource ss(stored.substr(PREFIX.size()),true,new Base64Decoder(new StringSink(jsonStr)));

			json::value jv = json::parse(jsonStr);
			json::object obj = jv.as_object();

			std::vector<byte> iv = hexDecode(obj["iv"].as_string().c_str());
			std::vector<byte> tag = hexDecode(obj["tag"].as_string().c_str());
			std::vector<byte> ct = hexDecode(obj["ct"].as_string().c_str());

			try
			{
				return decryptAESGCM(key, iv, tag, ct);
			}
			catch (...)
			{
				return std::nullopt;
			}
		}

		inline std::optional<std::string> encryptEmail(const std::string& email, const SecByteBlock& key)
		{
			const std::string PREFIX = "email:gcm:v1:";

			if (email.rfind(PREFIX, 0) == 0)
				return std::nullopt;

			auto result = encryptAESGCM(key, email);
			if (!result.has_value())
				return std::nullopt;

			auto& [iv, tag, ct] = result.value();

			json::object obj;
			obj["iv"] = hexEncode(iv);
			obj["tag"] = hexEncode(tag);
			obj["ct"] = hexEncode(ct);

			std::string jsonStr = json::serialize(obj);

			std::string encoded;
			StringSource ss(jsonStr,true,new Base64Encoder(new StringSink(encoded), false) );

			return PREFIX + encoded;
		}

		inline std::optional<std::string> decrypt2FASecret(const std::string& stored, const SecByteBlock& key)
		{
			const std::string PREFIX = "2fa:gcm:v1:";

			if (stored.rfind(PREFIX, 0) != 0)
				return stored;

			std::string jsonStr;
			StringSource ss(stored.substr(PREFIX.size()), true, new Base64Decoder(new StringSink(jsonStr)));

			json::value jv = json::parse(jsonStr);
			json::object obj = jv.as_object();

			std::vector<byte> iv = hexDecode(obj["iv"].as_string().c_str());
			std::vector<byte> tag = hexDecode(obj["tag"].as_string().c_str());
			std::vector<byte> ct = hexDecode(obj["ct"].as_string().c_str());

			return decryptAESGCM(key, iv, tag, ct); 
		}

		inline std::optional<std::string> encrypt2FASecret(const std::string& secret, const SecByteBlock& key)
		{
			const std::string PREFIX = "2fa:gcm:v1:";

			if (secret.rfind(PREFIX, 0) == 0)
				return secret;

			auto result = encryptAESGCM(key, secret);
			if (!result.has_value())
				return std::nullopt;

			auto& [iv, tag, ct] = result.value();

			json::object obj;
			obj["iv"] = hexEncode(iv);
			obj["tag"] = hexEncode(tag);
			obj["ct"] = hexEncode(ct);

			std::string jsonStr = json::serialize(obj);

			std::string encoded;
			StringSource ss(jsonStr,true,new Base64Encoder(new StringSink(encoded), false));

			return PREFIX + encoded;
		}

		inline size_t payload_source(void* ptr, size_t size, size_t nmemb, void* userp) 
		{
			std::string* data = reinterpret_cast<std::string*>(userp);
			size_t max = size * nmemb;
			if (data->empty())
				return 0;
			size_t copy_size = (data->size() < max) ? data->size() : max;
			memcpy(ptr, data->c_str(), copy_size);
			data->erase(0, copy_size);
			return copy_size;
		}

	
		inline bool sendEmails(const std::vector<std::string>& recipients, const std::string& subject, const std::string& body)
		{
			if (recipients.empty()) 
			{
				::Utils::Logger::log("[sendEmails] sendEmail called with empty recipients list", ::Utils::LogType::Warning, "sendEmail");
				return false;
			}

			const auto& generalSetup = Common::Utils::SetupParser::getInstance().getGeneralSetup();

			CURL* curl = curl_easy_init();
			if (!curl) 
			{
				::Utils::Logger::log("[sendEmails] Failed to initialize CURL",::Utils::LogType::Error, "sendEmail");
				return false;
			}

			struct curl_slist* recipientList = nullptr;
			std::string toHeader;
			for (size_t i = 0; i < recipients.size(); ++i) 
			{
				recipientList = curl_slist_append(recipientList, recipients[i].c_str());

				if (i > 0) toHeader += ", ";
				toHeader += recipients[i];
			}

			std::string data =
				"To: " + toHeader + "\r\n" +
				"From: " + generalSetup.email + "\r\n" +
				"Subject: " + subject + "\r\n" +
				"MIME-Version: 1.0\r\n"
				"Content-Type: text/html; charset=UTF-8\r\n"
				"\r\n" + body + "\r\n";

			curl_easy_setopt(curl, CURLOPT_USERNAME, generalSetup.emailUsername.c_str());
			curl_easy_setopt(curl, CURLOPT_PASSWORD, generalSetup.emailToken.c_str());
			curl_easy_setopt(curl, CURLOPT_URL, generalSetup.smtpServer.c_str());
			curl_easy_setopt(curl, CURLOPT_MAIL_FROM, generalSetup.email.c_str());
			curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipientList);
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
			curl_easy_setopt(curl, CURLOPT_READDATA, &data);
			curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
			//curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
			curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);

			CURLcode res = curl_easy_perform(curl);

			if (res != CURLE_OK) 
			{
				::Utils::Logger::log("[sendEmails] curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)), ::Utils::LogType::Error, "sendEmail");
			}

			curl_slist_free_all(recipientList);
			curl_easy_cleanup(curl);

			return res == CURLE_OK;
		}


		inline bool sendEmail(const std::string& to, const std::string& subject, const std::string& body)
		{
			return sendEmails(std::vector<std::string>{to}, subject, body);
		}

		inline bool sendEmailAlert(const std::string& title, const std::string message)
		{
			return sendEmails(Common::Utils::SetupParser::getInstance().getGeneralSetup().securityNotificationEmails, title, message);
		}

		inline std::string hashSha256(const std::string& hwid, const std::string& salt)
		{
			std::string concatenated = hwid + salt;
			std::string digest;

			CryptoPP::SHA256 hash;
			CryptoPP::StringSource ss(concatenated, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest), false)));

			return digest;
		}

		inline uint64_t datetimeToEpoch(const std::string& datetimeStr) noexcept
		{
			using namespace std::chrono;
			if (datetimeStr.empty()) return 0;

			std::istringstream ss(datetimeStr);
			std::tm tm{};
			ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

			if (ss.fail()) return 0;

			return static_cast<uint64_t>(std::mktime(&tm));
		}

		inline std::string epochToDatetime(uint64_t secondsSinceEpoch) noexcept
		{
			using namespace std::chrono;
			sys_seconds tp{ seconds{static_cast<int64_t>(secondsSinceEpoch)} };
			std::ostringstream ss;
			ss << format("{:%Y-%m-%d %H:%M:%S}", tp);
			return ss.str();
		}

		inline std::string generateRandomPassword(std::size_t len = 12)
		{
			const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

			std::string result;
			result.reserve(len);
			for (size_t i = 0; i < len; ++i)
				result += chars[dist(gen)];
			return result;
		}

		inline std::string generate2FASecret(std::size_t len = 16)
		{
			const std::string chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<size_t> dist(0, chars.size() - 1);

			std::string result;
			result.reserve(len);
			for (size_t i = 0; i < len; ++i)
				result += chars[dist(gen)];
			return result;
		}
	}
}

#endif
