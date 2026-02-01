#ifndef CAST_MAIN_IPC_CALLBACKS_H
#define CAST_MAIN_IPC_CALLBACKS_H

#include "Network/Packet.h"
#include "../../Network/MainSessionManager.h"
#include "../../Network/AuthSession.h"
#include "ConstantDatabase/Structures/CdbWeapon.h"

namespace Main
{
    namespace Handlers
    {
        inline void getSessionIdFor(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            Utils::Logger::log("CastSv requested SessionID, retrieving from main...", ::Utils::LogType::Info, "Handlers::getSessionIdFor");

            auto response = request;
            response.setData(nullptr, 0);
            if (request.getDataSize() != sizeof(uint32_t))
            {
                response.setExtra(1);
                session->asyncWrite(response);
                return;
            }
            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request);

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                std::uint32_t sessionId = targetSession->getId();
                response.setData(reinterpret_cast<std::uint8_t*>(&sessionId), sizeof(sessionId));
                response.setExtra(0);
            }
            else
            { 
                response.setExtra(1);
                response.setData(nullptr, 0);
            }
            session->asyncWrite(response); // "pong" the cast server
        }

        inline void getPlayerStateUpdate(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;
            response.setData(nullptr, 0);

            if (request.getDataSize() != sizeof(uint32_t))
            {
                response.setExtra(1);
                session->asyncWrite(response);
                return;
            }

            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request);
            const auto playerState = request.getOption(); 

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                targetSession->setPlayerState(static_cast<Common::Enums::PlayerState>(playerState));
                response.setExtra(0);
            }
            else
            {
                response.setExtra(1);
            }

            session->asyncWrite(response); // "pong" the cast server with success/failure status
        }

        inline void getPlayerItems(const Common::Network::UnecryptedPacket& request, std::shared_ptr<Common::Network::Session> session,
            Main::Network::SessionsManager& sessionsManager)
        {
            auto response = request;
            response.setData(nullptr, 0);

            if (request.getDataSize() != sizeof(uint32_t))  //we expect an account ID
            {
                response.setExtra(1);
                session->asyncWrite(response);
                return;
            }

            const std::uint32_t accountId = Main::Details::parseData<std::uint32_t>(request);

            if (auto targetSession = sessionsManager.getSessionByAccountId(accountId); targetSession)
            {
                auto player = targetSession->getPlayer();
                //get the currently equipped character
                const uint64_t selectedCharacter = player.getAccountInfo().latestSelectedCharacter;  //4 bit value

                if (selectedCharacter >= Common::Enums::MAX_CHARACTERS)
                {
                    response.setExtra(1);
                    session->asyncWrite(response);
                    return;
                }

                //get the items equipped
                auto items = player.getEquippedItems();
                const std::size_t startIndex = selectedCharacter * Common::Enums::MAX_ITEMTYPE;
                const std::size_t endIndex = startIndex + Common::Enums::MAX_ITEMTYPE;


                unsigned int hp = 2000;
                unsigned int speed = 0; //percent

                if (items[startIndex + Common::Enums::SET].id)
                {
                    auto equippedSet = items[startIndex + Common::Enums::SET];

                }

                struct ResponseBody {
                    unsigned int hp = 2000;
                    unsigned int speed = 0;
                    // we don't use projectile or explosion armor

                    struct {
                        unsigned int id = 0;
                        unsigned int param1 = 0;
                        unsigned int param2 = 0;
                        unsigned int param3 = 0;
                        unsigned int param4 = 0;
                    } weapons[7];
                } responseBody;

                //get the HP and run speed TODO

                //get the equipped weapons
                const unsigned int meleeId = items[startIndex + Common::Enums::MELEE].id;
                responseBody.weapons[0].id = meleeId;
                //query the CDB to extract these
                const auto& weaponTable = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeapon>::getInstance();
                const auto& melee = weaponTable.getEntry(meleeId);
                responseBody.weapons[0].param2 = melee->wi_range;

                response.setData(reinterpret_cast<uint8_t *>(&responseBody), sizeof(responseBody));
                response.setExtra(0);
            }
            else
            {
                response.setExtra(1);
            }
        }
    }
}

#endif