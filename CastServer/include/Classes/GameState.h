//
// Created by avlad171 on 22/01/2026.
//

#ifndef MICROVOLTS_EMULATOR_V2_GAMESTATE_H
#define MICROVOLTS_EMULATOR_V2_GAMESTATE_H

#include <vector>
#include <inttypes.h>

namespace Cast
{
    namespace Classes
    {
        constexpr size_t CIRCULAR_BUFFER_SIZE = 32;

        struct PlayerPos
        {
            float x;
            float y;
            float z;

            PlayerPos() : x(0.0f), y(0.0f), z(0.0f) {}
            float distanceTo(PlayerPos &other) const;
        };

        struct PlayerState
        {
            uint64_t tick;
            uint64_t hp;
            //add more as needed

            PlayerPos pos;
            PlayerState () : tick(0), hp(0) {}
        };

        class GameState
        {
            //we need a circular buffer to store player positions
            PlayerState playerStates [16][CIRCULAR_BUFFER_SIZE];  //32 ticks should be enough
            uint64_t tick;
            GameState(uint64_t tick);

        public:
            int movePlayer(uint64_t tick, unsigned int playerSlot, PlayerPos newPosition);
            int attackMeleeNonhost(uint64_t tick, unsigned int attackerSlot, unsigned int targetSlot);

        };
    }
}
#endif //MICROVOLTS_EMULATOR_V2_GAMESTATE_H