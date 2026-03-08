#ifndef COMMON_SETUP_PARSER_H
#define COMMON_SETUP_PARSER_H

#include <mutex>
#include <vector>
#include <iostream>
#include <cstdlib> 
#include "IniParser/inicpp.h"
#include "Logger.h"
#include <cryptopp/secblock.h>

#include <cryptopp/hex.h>
#include <cryptopp/filters.h>
#include <cryptopp/osrng.h> 
#include <cryptopp/aes.h>    

namespace Common
{
    namespace Utils
    {
        using namespace CryptoPP;

        struct AuthSetup
        {
            std::string ip;
            std::uint32_t port;
            std::uint32_t gradedPort;
            bool enhancedSecurity;
            std::string vpnIp;
        };

        struct MainSetup
        {
            std::string ip;
            std::string localIp;
            std::uint32_t port;
            std::uint32_t ipcPort;
            std::uint32_t serverNumber;
            bool isPublic;
        };

        struct CastSetup
        {
            std::string ip;
            std::string localIp;
            std::uint32_t port;
            std::uint32_t ipcPort;
            std::uint32_t serverNumber;
            bool IPC_enableDeadBroadcast;
        };

        struct DatabaseSetup
        {
            std::string ip;
            std::uint32_t port;
            std::string databaseName;
            std::string username;
            std::string password;
        };

        struct WebsiteSetup
        {
            std::string ip;
            std::uint32_t port;
            std::string jwtToken;
            std::vector<std::string> allowedOrigins;
        };

        struct ClientSetup
        {
            std::uint32_t version1 : 8 = 0;
            std::uint32_t version2 : 8 = 0;
            std::uint32_t version3 : 8 = 0;
        };

        struct GeneralSetup
        {
            CryptoPP::SecByteBlock emailSecret;
            CryptoPP::SecByteBlock  twoFaSecret;
            std::string smtpServer;
            std::string email;
            std::string emailUsername;
            std::string emailToken;
            std::vector<std::string> securityNotificationEmails;  
        };

        class SetupParser
        {
        private:
            ini::IniFile m_iniFile;
            std::once_flag m_flag;
            std::string m_localIp;
            MainSetup m_selfMainInfo;
            CastSetup m_selfCastInfo;
            std::vector<MainSetup> m_mainInfos;
            std::vector<CastSetup> m_castInfos;
            DatabaseSetup m_dbSetup;
            AuthSetup m_authSetup;
            WebsiteSetup m_websiteSetup;
            ClientSetup m_clientSetup;
            GeneralSetup m_generalSetup;

            SetupParser();
            SetupParser(const SetupParser&) = delete;
            SetupParser& operator=(const SetupParser&) = delete;

            bool checkMainCastSession();
            bool checkAuthSection();
            bool checkDatabaseConfig();
            bool checkWebsiteConfig();
            bool checkClientConfig();
            bool sanityCheck();
            bool checkGeneralConfig();

            std::optional<MainSetup> getSelfMainServerInfoImpl();
            std::optional<CastSetup> getSelfCastServerInfoImpl();
            std::optional<std::vector<MainSetup>> getMainServersInfoImpl();
            std::optional<std::vector<CastSetup>> getCastServersInfoImpl();
            AuthSetup getAuthSetupImpl();
            ClientSetup getClientSetupImpl();
            DatabaseSetup getDatabaseSetupImpl();
            WebsiteSetup getWebsiteSetupImpl();
            GeneralSetup getGeneralSetupImpl();
            void handleError(const std::string& message);

            template <typename T>
            void check_assign(std::optional<T> opt, T& member, const std::string& errorMessage)
            {
                if (!opt)
                {
                    handleError(errorMessage);
                    return;
                }
                member = *opt;
            }

            std::string trim(const std::string& str)
            {
                std::size_t start = 0;
                while (start < str.size() && std::isspace(static_cast<unsigned char>(str[start])))
                    ++start;

                size_t end = str.size();
                while (end > start && std::isspace(static_cast<unsigned char>(str[end - 1])))
                    --end;

                return str.substr(start, end - start);
            }

            std::optional<std::string> decodeHexKey(const std::string& hex)
            {
                std::string decoded;

                CryptoPP::StringSource ss(hex,true,new CryptoPP::HexDecoder(new CryptoPP::StringSink(decoded)));

                if (decoded.size() != 32)
                    return std::nullopt;

                return decoded;
            }

        public:
            static SetupParser& getInstance()
            {
                static SetupParser instance;
                return instance;
            }

            const MainSetup& getSelfMainServerInfo() const { return m_selfMainInfo; }
            const CastSetup& getSelfCastServerInfo() const { return m_selfCastInfo; }
            const std::vector<MainSetup>& getMainServersInfo() const { return m_mainInfos; }
            const std::vector<CastSetup>& getCastServersInfo() const { return m_castInfos; }
            const DatabaseSetup& getDatabaseSetup() const { return m_dbSetup; }
            const AuthSetup& getAuthSetup() const { return m_authSetup; }
            const WebsiteSetup& getWebsiteSetup() const { return m_websiteSetup; }
            const ClientSetup& getClientSetup() const { return m_clientSetup; }
            const GeneralSetup& getGeneralSetup() const { return m_generalSetup; }


            bool updateCapsuleEventInfo(std::uint32_t newCapsuleEventEndDate, std::uint32_t newCapsuleRtPrice, std::uint32_t newCapsuleMpPrice);
        };
    }
}

#endif
