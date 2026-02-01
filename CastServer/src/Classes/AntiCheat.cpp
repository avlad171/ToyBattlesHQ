#include "Classes/AntiCheat.h"
#include <cmath>

namespace Cast
{
    namespace Classes
    {
        float PlayerPos::distanceTo(PlayerPos &other) const
        {
            return sqrt(static_cast<double>(x - other.x) * (x - other.x) +
                (y - other.y) * (y - other.y) +
                (z - other.z) * (z - other.z)
                );
        }

        AntiCheat::AntiCheat(uint64_t tick)
        {
            this->tick = tick;
        }

        int AntiCheat::movePlayer(uint64_t tick, unsigned int playerSlot, PlayerPos newPosition)
        {
            if (playerSlot > 25)
            {
                //16 room + 10 observer
                return -1;
            }

            else if (playerSlot > 15)
            {
                //observer, can move anyhow
                return 1;
            }

            else
            {
                auto &state = this->gameStates[playerSlot][tick % CIRCULAR_BUFFER_SIZE];
                const unsigned int ticksPassed = tick - state.tick;

                const float distance = state.pos.distanceTo(newPosition);
                printf("[AC] The player %u has moved %f meters in %u ticks\n", playerSlot, distance, ticksPassed);


                state.pos = newPosition;
                return 1;
            }
        }

        //int AntiCheat::attackMeleeNonhost(uint64_t tick, unsigned int attackerSlot, unsigned int targetSlot)
        //{
        //    float meleeRange

        //}
    }
}