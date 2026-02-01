#ifndef CDBWEAPON_H
#define CDBWEAPON_H

#include "visit_struct/visit_struct.hpp"
#include <array>
#include "Macros.h"

namespace Common
{
    namespace ConstantDatabase
    {
PACK_PUSH(1)
        struct CdbWeapon
        {
            uint32_t wi_id = static_cast<uint32_t>(-1);
            uint32_t wi_weapon_type;
            uint32_t wi_weapon_type_zombie;
            uint32_t wi_weapon_prop;
            uint32_t wi_trigger_type;
            uint32_t wi_firing_type;
            uint32_t wi_ragdoll_type;
            uint32_t wi_notgore_ragdoll_type;
            uint32_t wi_debuff_type;
            uint32_t wi_projectile_type;
            uint32_t wi_projectile_prop;
            uint32_t wi_charging_effect_type;
            uint32_t wi_charging_time_max;
            uint32_t wi_charging_effect_damage;
            uint32_t wi_charging_effect_speed;
            uint32_t wi_charging_effect_bombrange;
            uint32_t wi_charging_effect_accuracy;
            uint32_t wi_charging_effect_multi_shot;
            uint32_t wi_charging_effect_bomb_timer;
            uint32_t wi_charging_effect_debuff;
            uint32_t wi_charging_effect_projectile;
            uint32_t wi_charging_effect_chainshot;
            uint32_t wi_auto_aim;
            uint32_t wi_aim_movetype;
            uint32_t wi_aim_move_param;
            uint32_t wi_aim_init_size;
            uint32_t wi_aim_max_size;
            uint32_t wi_aim_zoom_size_min;
            uint32_t wi_aim_zoom_size_max;
            uint32_t wi_aim_zoom2_size_min;
            uint32_t wi_aim_zoom2_size_max;
            uint32_t wi_aim_jump_size;
            uint32_t wi_aim_spread_speed;
            uint32_t wi_aim_restore_speed;
            uint32_t wi_aim_restore_speed_zoom;
            uint32_t wi_aim_moveup_type;
            uint32_t wi_aim_moveup_param;
            uint32_t wi_aim_moveup_max;
            uint32_t wi_aim_moveup_speed;
            uint32_t wi_aim_moveup_restore_speed;
            uint32_t wi_dam_head;
            uint32_t wi_dam_upper;
            uint32_t wi_dam_under;
            uint32_t wi_dam_zoom_in;
            uint32_t wi_rand_dam;
            uint32_t wi_hit_rate;
            uint32_t wi_miss_dam_rate;
            uint32_t wi_critical_rate;
            uint32_t wi_critical_damage;
            uint32_t wi_zoom_in_level;
            uint32_t wi_zoom_fov1;
            uint32_t wi_zoom_fov2;
            uint32_t wi_zoom_fire_ready_time;
            uint32_t wi_fire_bullet_count;
            uint32_t wi_chainshot_count;
            uint32_t wi_chainshot_interval;
            uint32_t wi_setup_count;
            uint32_t wi_bullet_speed;
            uint32_t wi_bullet_max_speed;
            uint32_t wi_bullet_accel;
            uint32_t wi_bullet_bounce_count;
            uint32_t wi_bullet_restitution;
            uint32_t wi_range;
            uint32_t wi_overheat_up;
            uint32_t wi_overheat_down;
            uint32_t wi_overheat_penalty_time;
            uint32_t wi_bomb_range;
            uint32_t wi_bomb_time;
            uint32_t wi_bomb_type;
            uint32_t wi_detonate_type;
            uint32_t wi_sensor_range;
            uint32_t wi_ready_fire;
            uint32_t wi_fire_run;
            uint32_t wi_fire_time;
            uint32_t wi_lockon_time;
            uint32_t wi_firing_accel_time;
            uint32_t wi_firing_hold_time;
            uint32_t wi_fire_time_right;
            uint32_t wi_reload_time;
            uint32_t wi_reload_any_type;
            uint32_t wi_knockback;
            uint32_t wi_accel_weight;
            uint32_t wi_accel_time;
            uint32_t wi_max_turning_angle;
            uint32_t wi_rate_of_fire_def;
            uint32_t wi_rate_of_fire_max;
            uint32_t wi_bullet_capacity;
            uint32_t wi_bullet_total;
            uint32_t wi_change_time;
            uint32_t wi_change_skip;
            uint32_t wi_change_delay;
            uint32_t wi_weaponself_time;
            uint32_t wi_ability_a;
            uint32_t wi_ability_b;
            uint32_t wi_ability_c;
            uint32_t wi_ability_d;
            uint32_t wi_ability_a_max;
            uint32_t wi_ability_b_max;
            uint32_t wi_ability_c_max;
            uint32_t wi_ability_d_max;
            uint32_t wi_weapon_size;

            constexpr std::uint32_t getId() const noexcept { return wi_id; }
            constexpr bool isValid() const noexcept { return wi_id != static_cast<std::uint32_t>(-1); }
        };
PACK_POP()
    }
}

//VISITABLE_STRUCT(Common::ConstantDatabase::CdbWeapon, wi_id, wi_weapon_type, wi_dam_head, wi_dam_upper, wi_dam_under, wi_range);
VISITABLE_STRUCT(Common::ConstantDatabase::CdbWeapon, wi_id, wi_weapon_type, wi_weapon_type_zombie, wi_weapon_prop, wi_trigger_type,
    wi_firing_type, wi_ragdoll_type, wi_notgore_ragdoll_type, wi_debuff_type, wi_projectile_type, wi_projectile_prop, wi_charging_effect_type,
    wi_charging_time_max, wi_charging_effect_damage, wi_charging_effect_speed, wi_charging_effect_bombrange, wi_charging_effect_accuracy,
    wi_charging_effect_multi_shot, wi_charging_effect_bomb_timer, wi_charging_effect_debuff, wi_charging_effect_projectile,
    wi_charging_effect_chainshot, wi_auto_aim, wi_aim_movetype, wi_aim_move_param, wi_aim_init_size, wi_aim_max_size,
    wi_aim_zoom_size_min, wi_aim_zoom_size_max, wi_aim_zoom2_size_min, wi_aim_zoom2_size_max, wi_aim_jump_size, wi_aim_spread_speed,
    wi_aim_restore_speed, wi_aim_restore_speed_zoom, wi_aim_moveup_type, wi_aim_moveup_param, wi_aim_moveup_max, wi_aim_moveup_speed,
    wi_aim_moveup_restore_speed, wi_dam_head, wi_dam_upper, wi_dam_under, wi_dam_zoom_in, wi_rand_dam, wi_hit_rate, wi_miss_dam_rate,
    wi_critical_rate, wi_critical_damage, wi_zoom_in_level, wi_zoom_fov1, wi_zoom_fov2, wi_zoom_fire_ready_time, wi_fire_bullet_count,
    wi_chainshot_count, wi_chainshot_interval, wi_setup_count, wi_bullet_speed, wi_bullet_max_speed, wi_bullet_accel, wi_bullet_bounce_count,
    wi_bullet_restitution, wi_range, wi_overheat_up, wi_overheat_down, wi_overheat_penalty_time, wi_bomb_range, wi_bomb_time, wi_bomb_type/*,
    wi_detonate_type, wi_sensor_range, wi_ready_fire, wi_fire_run, wi_fire_time, wi_lockon_time, wi_firing_accel_time, wi_firing_hold_time,
    wi_fire_time_right, wi_reload_time, wi_reload_any_type, wi_knockback, wi_accel_weight, wi_accel_time, wi_max_turning_angle, wi_rate_of_fire_def,
    wi_rate_of_fire_max, wi_bullet_capacity, wi_bullet_total, wi_change_time, wi_change_skip, wi_change_delay, wi_weaponself_time,
    wi_ability_a, wi_ability_b, wi_ability_c, wi_ability_d, wi_ability_a_max, wi_ability_b_max, wi_ability_c_max, wi_ability_d_max,
    wi_weapon_size*/
    );


#endif //CDBWEAPON_H
