#ifndef CDB_UTILITY_H
#define CDB_UTILITY_H

#include "ConstantDatabase/CdbSingleton.h"

namespace Cast
{
    namespace CdbUtils
    {
        union WeaponStats
        {
            struct
            {
                uint16_t stat1;
                uint16_t stat2;
                uint16_t stat3;
                uint16_t stat4;
            };
            uint64_t data;
        };

        template<typename T>
        using Cdb = Common::ConstantDatabase::Cdb<T>;

        using cdbWeapons = Common::ConstantDatabase::CdbSingleton<Common::ConstantDatabase::CdbWeapon>;

        inline std::optional<uint32_t> getMeleeRange (uint32_t itemId)
        {
            if (const auto entry = cdbWeapons::getInstance().getEntry(itemId); entry)
            {
                if(entry-?
                if (entry->wi_
            }
            return std::nullopt;
        }
    }
}
#endif