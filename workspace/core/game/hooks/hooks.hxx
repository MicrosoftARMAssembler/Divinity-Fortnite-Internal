#pragma once

namespace hooks {
    namespace projectile {
        extern game::fvector m_source_location;
    }

    namespace weapon {
        struct spread_backup_t {
            float base_spread = 0.0f;
            float movement_spread = 0.0f;
            float hipfire_spread = 0.0f;
            float final_multiplier = 1.0f;
            float ads_spread = 1.0f;
            float accumulator = 0.0f;
            float transition = 0.0f;
            float outgoing = 1.0f;
        };

        struct fire_state_backup_t {
            int shot_counter = 0;             // +8384
            float fire_accumulator = 0.0f;    // +10136
            uint8_t fire_flags = 0;           // +10296
            uint8_t tracer_flags = 0;         // +7920
            uintptr_t tracer_channel = 0;     // +7880
            __int64 handle_primary = 0;       // +7840
            __int64 handle_secondary = 0;     // +7848
            __int64 handle_main = 0;          // +7856
            bool valid = false;
        };

        void write_encrypted_float( uintptr_t base, int value_offset, int cookie_byte_offset, int index_offset, float value ) {
            // read current cookie
            uint8_t cookie = *reinterpret_cast< uint8_t* >( base + cookie_byte_offset );
            uint8_t index = *reinterpret_cast< uint8_t* >( base + index_offset );

            // encrypt value with current cookie
            int encrypted = *reinterpret_cast< int* >( &value ) ^ ( cookie & 0xFE ) ^ 0x4C008E95;

            // write to current ring index slot
            *reinterpret_cast< int* >( base + value_offset + 4LL * ( index & 3 ) ) = encrypted;
        }

        float read_encrypted_float( uintptr_t base, int value_offset, int cookie_byte_offset, int index_offset ) {
            uint8_t cookie = *reinterpret_cast< uint8_t* >( base + cookie_byte_offset );
            uint8_t index = *reinterpret_cast< uint8_t* >( base + index_offset );

            int raw = *reinterpret_cast< int* >( base + value_offset + 4LL * ( index & 3 ) );
            int decrypted = raw ^ ( cookie & 0xFE ) ^ 0x4C008E95;
            return *reinterpret_cast< float* >( &decrypted );
        }

        spread_backup_t backup_spread( game::a_fort_weapon* weapon ) {
            spread_backup_t backup = {};
            if ( !weapon ) return backup;

            auto base = reinterpret_cast< uintptr_t >( weapon );
            backup.base_spread = read_encrypted_float( base, 10024, 10041, 10040 );
            backup.movement_spread = read_encrypted_float( base, 10042, 10059, 10058 );
            backup.hipfire_spread = read_encrypted_float( base, 10080, 10097, 10096 );
            backup.final_multiplier = read_encrypted_float( base, 10098, 10115, 10114 );
            backup.ads_spread = read_encrypted_float( base, 10144, 10161, 10160 );
            backup.accumulator = read_encrypted_float( base, 10162, 10179, 10178 );
            backup.transition = read_encrypted_float( base, 10180, 10197, 10196 );
            backup.outgoing = read_encrypted_float( base, 10198, 10215, 10214 );
            return backup;
        }

        void restore_spread( game::a_fort_weapon* weapon, const spread_backup_t& backup ) {
            if ( !weapon ) return;
            auto base = reinterpret_cast< uintptr_t >( weapon );

            write_encrypted_float( base, 10024, 10041, 10040, backup.base_spread );
            write_encrypted_float( base, 10042, 10059, 10058, backup.movement_spread );
            write_encrypted_float( base, 10080, 10097, 10096, backup.hipfire_spread );
            write_encrypted_float( base, 10098, 10115, 10114, backup.final_multiplier );
            write_encrypted_float( base, 10144, 10161, 10160, backup.ads_spread );
            write_encrypted_float( base, 10162, 10179, 10178, backup.accumulator );
            write_encrypted_float( base, 10180, 10197, 10196, backup.transition );
            write_encrypted_float( base, 10198, 10215, 10214, backup.outgoing );
        }

        void zero_spread( game::a_fort_weapon* weapon ) {
            if ( !weapon ) return;
            auto base = reinterpret_cast< uintptr_t >( weapon );

            // write 1.0f to accuracy multiplier - highest accuracy
            // write 0.0f to all spread accumulators - no spread buildup
            write_encrypted_float( base, 10024, 10041, 10040, 0.0f ); // base spread
            write_encrypted_float( base, 10042, 10059, 10058, 0.0f ); // movement spread
            write_encrypted_float( base, 10080, 10097, 10096, 0.0f ); // hipfire spread
            write_encrypted_float( base, 10098, 10115, 10114, 1.0f ); // final multiplier - 1.0 = full accuracy
            write_encrypted_float( base, 10144, 10161, 10160, 1.0f ); // ads spread - 1.0 = full accuracy
            write_encrypted_float( base, 10162, 10179, 10178, 0.0f ); // accumulator
            write_encrypted_float( base, 10180, 10197, 10196, 0.0f ); // transition
            write_encrypted_float( base, 10198, 10215, 10214, 1.0f ); // outgoing - 1.0 = perfect accuracy
        }

        fire_state_backup_t backup_fire_state( game::a_fort_weapon* weapon ) {
            fire_state_backup_t backup = {};
            if ( !weapon ) return backup;

            auto base = reinterpret_cast< uintptr_t >( weapon );
            backup.shot_counter = *reinterpret_cast< int* >( base + 0x20C0 );
            backup.fire_accumulator = *reinterpret_cast< float* >( base + 0x2798 );
            backup.fire_flags = *reinterpret_cast< uint8_t* >( base + 0x2838 );
            backup.tracer_flags = *reinterpret_cast< uint8_t* >( base + 0x1EF0 );
            backup.tracer_channel = *reinterpret_cast< uintptr_t* >( base + 0x1EC8 );
            backup.handle_primary = *reinterpret_cast< __int64* >( base + 0x1EA0 );
            backup.handle_secondary = *reinterpret_cast< __int64* >( base + 0x1EA8 );
            backup.handle_main = *reinterpret_cast< __int64* >( base + 0x1EB0 );
            backup.valid = true;
            return backup;
        }

        void restore_fire_state( game::a_fort_weapon* weapon, const fire_state_backup_t& backup ) {
            if ( !weapon || !backup.valid ) return;

            auto base = reinterpret_cast< uintptr_t >( weapon );
            *reinterpret_cast< int* >( base + 0x20C0 ) = backup.shot_counter;
            *reinterpret_cast< float* >( base + 0x2798 ) = backup.fire_accumulator;
            *reinterpret_cast< uint8_t* >( base + 0x2838 ) = backup.fire_flags;
            *reinterpret_cast< uint8_t* >( base + 0x1EF0 ) = backup.tracer_flags;
            *reinterpret_cast< uintptr_t* >( base + 0x1EC8 ) = backup.tracer_channel;
            *reinterpret_cast< __int64* >( base + 0x1EA0 ) = backup.handle_primary;
            *reinterpret_cast< __int64* >( base + 0x1EA8 ) = backup.handle_secondary;
            *reinterpret_cast< __int64* >( base + 0x1EB0 ) = backup.handle_main;
        }

        void force_ads_accuracy( game::a_fort_weapon* weapon ) {
            if ( !weapon ) return;
            *reinterpret_cast< float* >(
                reinterpret_cast< uintptr_t >( weapon ) + 0x22D0  // +8912
                ) = 1.0f;
        }

        void force_zero_spread( game::a_fort_weapon* weapon ) {
            if ( !weapon ) return;
            // +9144 is the accumulated delta time / shot timer
            // zeroing this prevents spread buildup between shots
            *reinterpret_cast< float* >(
                reinterpret_cast< uintptr_t >( weapon ) + 0x23B8  // +9144
                ) = 0.0f;

            // +8372 is fire interval countdown
            // keeping this at 0 means weapon thinks its always ready to fire
            *reinterpret_cast< float* >(
                reinterpret_cast< uintptr_t >( weapon ) + 0x20B4  // +8372
                ) = 0.0f;
        }

        void force_spread_early_out( game::a_fort_weapon* weapon ) {
            if ( !weapon ) return;
            // clear bit 6 of +4088 to force early out path
            // which writes 0.0f to spread multiplier +10098
            *reinterpret_cast< uint8_t* >(
                reinterpret_cast< uintptr_t >( weapon ) + 0xFF8
                ) &= ~0x40;
        }

        void confirm_shot_handle( game::a_fort_weapon* weapon ) {
            if ( !weapon ) return;
            auto base = reinterpret_cast< uintptr_t >( weapon );

            // get main shot handle
            auto handle = *reinterpret_cast< __int64* >( base + 0x1EB0 ); // +7856
            if ( !handle ) return;

            // sub_7FF621BC81A6 equivalent - walk the shot array
            // to find the record matching our handle
            auto weapon_config = *reinterpret_cast< uintptr_t* >( base + 0x68 );
            if ( !weapon_config ) return;

            auto shot_array = *reinterpret_cast< uintptr_t* >( weapon_config + 0x590 ); // v89 = *v33
            if ( !shot_array ) return;

            auto count = *reinterpret_cast< int* >( shot_array + 16 );
            auto entries = *reinterpret_cast< uintptr_t* >( shot_array + 8 );
            if ( !entries ) return;

            for ( int i = 0; i < count; i++ ) {
                auto entry = entries + 96 * i;
                if ( *reinterpret_cast< __int64* >( entry + 80 ) == handle ) {
                    // found our shot record
                    auto state = reinterpret_cast< uint8_t* >( entry + 1 );
                    if ( *state != 4 ) // dont touch invalid
                        *state = 2;    // force confirmed
                    break;
                }
            }
        }
    }
    namespace listener_blocking {
        // From RebindTargetListenersAndEventBridges:
        // v36 = (*(__int64(__fastcall **)(_QWORD))(**(_QWORD **)(a1 + 1312) + 680LL))(*(_QWORD *)(a1 + 1312));
        // +680 / 8 = vtable index 85
        using resolve_listener_bridge_fn = __int64( __fastcall* )( void* );
        resolve_listener_bridge_fn resolve_listener_bridge_org = nullptr;

        bool block_fall_damage_listener = true;

        __int64 __fastcall resolve_listener_bridge_hook( void* owner ) {
            log_info( oxorany( "[divinity] listner hooked\n" ) );
            if ( block_fall_damage_listener )
                return 0;
            return resolve_listener_bridge_org( owner );
        }

        bool install_on_owner( void* owner_with_vtable ) {
            if ( !owner_with_vtable )
                return false;

            static Hook::NewHook hook = {};
            if ( resolve_listener_bridge_org )
                return true;

            return hook.VMT(
                owner_with_vtable,
                resolve_listener_bridge_hook,
                680 / 8,
                &resolve_listener_bridge_org
            );
        }

        bool install_from_manager( void* manager_a1 ) {
            if ( !manager_a1 )
                return false;

            auto owner = *reinterpret_cast< void** >( reinterpret_cast< uintptr_t >( manager_a1 ) + 0x520 ); // +1312
            return install_on_owner( owner );
        }
    }

    namespace damage {
        using fire_fn = char( __fastcall* )( __int64, __int64, __int64 );
        fire_fn register_client_fire_org = nullptr;
        bool register_pellets = true;

        char __fastcall register_client_fire( __int64 a1, __int64 a2, __int64 a3 ) {
            //if ( settings::aimbot::multiple_damage && register_pellets ) {

            log_info( "hooked\n" );


                auto vtable = *reinterpret_cast< uintptr_t** >( a1 );
                auto get_def = reinterpret_cast< __int64( * )( __int64 ) >( vtable[ game::vtable::weapon_get_definition ] );
                auto def = get_def( a1 );
                if ( !def )
                    return register_client_fire_org( a1, a2, a3 );

                auto pellet_count_ptr = reinterpret_cast< int* >( def + 680 );
                auto flags_ptr = reinterpret_cast< uint8_t* >( a1 + 10296 );
                int original_count = *pellet_count_ptr;
                const uint8_t original_flags = *flags_ptr;

                *pellet_count_ptr = max( 1, static_cast< int >( settings::aimbot::damage_multiplier ) );
                *flags_ptr = static_cast< uint8_t >( original_flags | 0x5 );

                auto result = register_client_fire_org( a1, a2, a3 );

                *pellet_count_ptr = original_count;
                *flags_ptr = original_flags;
                return result;
            //}

            return register_client_fire_org( a1, a2, a3 );
        }

        bool install( game::a_fort_weapon* weapon ) {
            if ( !weapon )
                return false;

            static game::a_fort_weapon* old_weapon = nullptr;
            static Hook::NewHook hook = {};

            //if ( weapon == old_weapon && register_client_fire_org )
            //    return true;

            if ( !hook.VMT(
                weapon,
                register_client_fire,
                game::vtable::register_client_fire,
                &register_client_fire_org
                ) ) return false;

            old_weapon = weapon;
            return true;
        }
    }

    namespace projectile {
        game::fvector m_source_location = {};
 
        __m128d* ( *get_camera_targeting_source_location_org )( __int64, __m128d* );
        __m128d* get_camera_targeting_source_location( __int64 a1, __m128d* a2 ) {
            auto result = get_camera_targeting_source_location_org( a1, a2 );

            if ( settings::aimbot::bullet_tp && m_source_location ) {
                a2[ 0 ].m128d_f64[ 0 ] = m_source_location.m_x;
                a2[ 0 ].m128d_f64[ 1 ] = m_source_location.m_y;
                a2[ 1 ].m128d_f64[ 0 ] = m_source_location.m_z;
            }

            return a2;
        }

        bool install( game::a_fort_weapon* weapon ) {
            if ( !weapon )
                return false;

            static game::a_fort_weapon* old_weapon = nullptr;
            static Hook::NewHook hook = {};

            if ( weapon == old_weapon && get_camera_targeting_source_location_org )
                return true;

            if ( !hook.VMT(
                weapon,
                get_camera_targeting_source_location,
                game::vtable::camera_targeting_source_location,
                &get_camera_targeting_source_location_org
                ) ) return false;

            old_weapon = weapon;
            return true;
        }
    }

    namespace raycast {
        inline game::fvector m_source_location{};

        using fn_line_trace = void* ( __fastcall* )(
            void* world_context,
            void* out_hit,
            void* start,
            void* end,
            int   channel,
            void* params,
            void* response_params,
            void* collision_query
            );

        inline fn_line_trace line_trace_org = nullptr;

        void* __fastcall line_trace_hk(
            void* world_context, void* out_hit, void* start, void* end,
            int channel, void* params, void* response_params, void* collision_query
        ) {
            static int call_count = 0;
            if ( ++call_count <= 3 ) {
                char buf[ 128 ]{};
                std::snprintf( buf, sizeof( buf ),
                    "[raycast] hk called #%d org=%p out_hit=%p\n",
                    call_count,
                    reinterpret_cast< void* >( line_trace_org ),
                    out_hit );
                log_info( buf );
            }

            if ( !line_trace_org ) {
                log_info( "[raycast] line_trace_org is null in hk!\n" );
                return nullptr;
            }

            auto result = line_trace_org(
                world_context, out_hit, start, end,
                channel, params, response_params, collision_query
            );

            if ( settings::aimbot::silent_aim && m_source_location && out_hit ) {
                auto* hit = reinterpret_cast< game::f_hit_result* >( out_hit );
                hit->m_impact_point = m_source_location;
                hit->m_trace_end = m_source_location;
            }

            return result;
        }

        inline bool install( ) {
            char buf[ 256 ]{};

            if ( !game::functions::raycast_multiple ) {
                log_info( "[raycast] raycast_multiple is null, skipping\n" );
                return false;
            }

            if ( line_trace_org ) {
                log_info( "[raycast] already installed\n" );
                return true;
            }

            std::snprintf( buf, sizeof( buf ), "[raycast] target = %p\n",
                reinterpret_cast< void* >( game::functions::raycast_multiple ) );
            log_info( buf );

            dll::m_line_trace_detour = {};
            dll::m_hook_ctor(
                reinterpret_cast< __int64 >( &dll::m_line_trace_detour ),
                oxorany( L"divinity_line_trace" )
            );

            const auto result = dll::m_hook_minhook(
                reinterpret_cast< __int64 >( &dll::m_line_trace_detour ),
                game::functions::raycast_multiple,
                reinterpret_cast< __int64 >( line_trace_hk )
            );

            std::snprintf( buf, sizeof( buf ), "[raycast] minhook result = %lld, original = %p\n",
                static_cast< long long >( result ),
                dll::m_line_trace_detour.original );
            log_info( buf );

            if ( result >= 0 && dll::m_line_trace_detour.original ) {
                line_trace_org = reinterpret_cast< fn_line_trace >(
                    dll::m_line_trace_detour.original );

                std::snprintf( buf, sizeof( buf ), "[raycast] line_trace_org = %p\n",
                    reinterpret_cast< void* >( line_trace_org ) );
                log_info( buf );
                return true;
            }

            log_info( "[raycast] install failed\n" );
            return false;
        }
    }

    namespace targeting {
        static game::a_fort_weapon* m_current_weapon;
        game::fvector m_source_location;

        // Weapon.UseWeaponTargetingSourceOffsetModifiers
        __m128d* ( *get_camera_targeting_source_location_org )( __int64, __m128d* );
        __m128d* get_camera_targeting_source_location( __int64 a1, __m128d* a2 ) {
            auto result = get_camera_targeting_source_location_org( a1, a2 );

            a2[ 0 ].m128d_f64[ 0 ] = m_source_location.m_x;
            a2[ 0 ].m128d_f64[ 1 ] = m_source_location.m_y;
            a2[ 1 ].m128d_f64[ 0 ] = m_source_location.m_z;

            return a2;
        }

        bool install( game::a_fort_weapon* weapon ) {
            if ( !weapon )
                return false;

            static game::a_fort_weapon* old_weapon = 0;
            static Hook::NewHook hook;

            if ( weapon == old_weapon && get_camera_targeting_source_location_org )
                return true;

            if ( !hook.VMT(
                weapon,
                get_camera_targeting_source_location,
                game::vtable::camera_targeting_source_location,
                &get_camera_targeting_source_location_org
                ) ) return false;

            m_current_weapon = weapon;
            return true;
        }
    }

    namespace camera {
        static game::a_fort_weapon* m_current_weapon;
        game::fvector m_source_location;

        using hk_fn = void* ( __fastcall* )(
            void*,
            void*,
            double*,
            double*,
            void*,
            int,
            void*
            );

        // Fort.Rollback.FixFirstPersonFiringFromCameraLogic
        hk_fn get_weapon_targeting_source_location_org = nullptr;
        void* __fastcall get_weapon_targeting_source_location(
            void* ability,
            void* hit_result,
            double* origin,
            double* end,
            void* a5,
            int a6,
            void* a7
        ) {

            if ( settings::aimbot::silent_aim && m_source_location && end != nullptr ) {
                auto target = m_source_location;
                game::fvector org = { origin[ 0 ], origin[ 1 ], origin[ 2 ] };
                auto delta = target - org;

                auto len = sqrt( delta.m_x * delta.m_x + delta.m_y * delta.m_y + delta.m_z * delta.m_z );
                if ( len > 0.000001 ) {
                    game::fvector orig_end = { end[ 0 ], end[ 1 ], end[ 2 ] };
                    game::fvector orig_delta = orig_end - org;
                    auto range = sqrt( orig_delta.m_x * orig_delta.m_x + orig_delta.m_y * orig_delta.m_y + orig_delta.m_z * orig_delta.m_z );

                    double inv = 1.0 / len;
                    end[ 0 ] = org.m_x + delta.m_x * inv * range;
                    end[ 1 ] = org.m_y + delta.m_y * inv * range;
                    end[ 2 ] = org.m_z + delta.m_z * inv * range;
                }
            }

            return get_weapon_targeting_source_location_org(
                ability,
                hit_result,
                origin,
                end,
                a5,
                a6,
                a7
            );
        }

        bool install( game::a_fort_weapon* weapon ) {
            if ( !weapon )
                return false;

            static game::a_fort_weapon* old_weapon = 0;
            static Hook::NewHook hook;

            if ( weapon == old_weapon && get_weapon_targeting_source_location_org )
                return true;

            if ( !hook.VMT( 
                weapon,
                get_weapon_targeting_source_location,
                game::vtable::weapon_targeting_source_location,
                &get_weapon_targeting_source_location_org 
            ) ) return false;

            m_current_weapon = weapon;
            return true;
        }
    }
}




















