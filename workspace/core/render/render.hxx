#pragma once
#include <random>

namespace render {
    static bool g_pending_resize_reinit = false;
    static volatile LONG g_resizebuffers_call_count = 0;
    static constexpr auto m_pool_size = 100;
    static constexpr auto m_trail_size = 128;

    struct tracer_info_t {
        game::fvector m_start;
        game::fvector m_end;
        double        m_impact_angle;
        double        m_creation_time;
    };

    struct player_candidate_t {
        game::a_fort_player_pawn_athena* pawn;
        game::fvector2d                  head_position;
        game::fvector2d                  root_position;
        double                           world_distance;
        bool                             is_visible;
        bool                             is_bot;
        bool                             is_downed;
    };

    static constexpr size_t m_max_tracers = 15;

    struct tracer_cache_t {
        tracer_info_t entries[ m_max_tracers ];
        int           head = 0;
        int           count = 0;
    };

    static tracer_cache_t m_tracer_cache;

    struct trace_point_t {
        game::fvector  position;
        double         timestamp;
        peach::color_t col;
        bool           alive;
    };

    struct player_trail_t {
        uintptr_t     key;
        double        last_push;
        trace_point_t points[ 128 ];
        int           head;
        int           count;
        bool          used;
    };

    static player_trail_t m_trail_pool[ 100 ];
    static bool           m_trail_pool_init = false;

    namespace particle {

        struct c_particle {
            float m_pos_x, m_pos_y;
            float m_velocity_x, m_velocity_y;
            float m_alpha, m_alpha_timer;
            int   m_lifespan, m_seed, m_flag, m_delay;

            c_particle( ) : m_pos_x( 0 ), m_pos_y( 0 ), m_velocity_x( 0 ), m_velocity_y( 0 ),
                m_alpha( 0.f ), m_alpha_timer( 0.f ), m_lifespan( 0 ), m_seed( 0 ), m_flag( 0 ), m_delay( 0 ) {
            }
        };

        static constexpr int max_particles = 6048;
        c_particle m_particles[ max_particles ]{};

        void add_particle( float origin_x, float origin_y, float size_x, float flag ) {
            auto frand = [ ] ( float lo, float hi ) {
                return lo + ( hi - lo ) * ( rand( ) / static_cast< float >( RAND_MAX ) );
                };

            c_particle p;
            p.m_pos_x = origin_x + size_x * 0.5f;
            p.m_pos_y = origin_y;
            p.m_velocity_x = frand( -0.75f, 0.75f );
            p.m_velocity_y = flag ? frand( -2.0f, -1.0f ) : frand( -0.5f, 0.25f );
            p.m_lifespan = 60 + rand( ) % 240;
            p.m_seed = rand( );
            p.m_flag = static_cast< int >( flag );
            p.m_alpha = 0.f;
            p.m_alpha_timer = 0.f;

            for ( int i = 0; i < max_particles; i++ ) {
                if ( m_particles[ i ].m_lifespan == 0 ) {
                    m_particles[ i ] = p;
                    break;
                }
            }
        }

        void render( float dt, float time_offset ) {
            if ( !settings::misc::particles_enabled )
                return;

            g_interface->set_shape_antialiasing( false );

            for ( int i = 0; i < max_particles; i++ ) {
                c_particle& p = m_particles[ i ];
                if ( !p.m_lifespan ) continue;

                if ( p.m_delay ) {
                    p.m_delay--;
                    continue;
                }

                p.m_pos_x += p.m_velocity_x;
                p.m_pos_y += p.m_velocity_y;
                p.m_velocity_y += 0.05f * ( dt * 60.0f );
                const float damping = powf( 0.97f, dt * 60.0f );
                p.m_velocity_x *= damping;
                p.m_velocity_y *= damping;

                if ( p.m_velocity_y > 0 )
                    p.m_lifespan--;

                if ( p.m_alpha_timer < 1.f )
                    p.m_alpha = p.m_alpha_timer;
                else
                    p.m_alpha = max( 0.f, 2.f - p.m_alpha_timer );
                p.m_alpha = std::clamp( p.m_alpha, 0.f, 1.f );
                p.m_alpha_timer += dt * 1.5f;

                if ( p.m_alpha <= 0.01f ) continue;

                if ( settings::misc::fancy_particles ) {
                    float outer = 6.f + ( p.m_seed % 8 ) + ( p.m_flag * 5.f );
                    float inner = outer * 0.35f;
                    float angle = time_offset * 0.8f + ( p.m_seed % 1000 ) * 0.01f + i * 0.03f;

                    float sx[ 8 ], sy[ 8 ];
                    for ( int j = 0; j < 8; j++ ) {
                        float a = angle + j * ( k_pi<float> / 4.f );
                        float r = ( j % 2 == 0 ) ? outer : inner;
                        sx[ j ] = p.m_pos_x + cosf( a ) * r;
                        sy[ j ] = p.m_pos_y + sinf( a ) * r;
                    }

                    peach::color_t base = ( i % 2 == 0 )
                        ? peach::color_t( 75, 87, 219, 255 )
                        : peach::color_t( 94, 105, 238, 255 );

                    uint8_t a = static_cast< uint8_t >( p.m_alpha * 255.f );
                    peach::color_t col( base.r, base.g, base.b, a );

                    for ( int j = 0; j < 8; j++ ) {
                        int next = ( j + 1 ) % 8;
                        g_interface->draw_triangle(
                            p.m_pos_x, p.m_pos_y,
                            sx[ j ], sy[ j ],
                            sx[ next ], sy[ next ],
                            col );
                    }
                }
                else {
                    uint8_t a = static_cast< uint8_t >( p.m_alpha * 255.f );
                    g_interface->draw_circle( p.m_pos_x, p.m_pos_y, 2.5f,
                        peach::color_t( 94, 105, 238, a ) );
                }
            }

            g_interface->set_shape_antialiasing( true );
        }
    }

    inline float get_delta_time( ) {
        static LARGE_INTEGER last = {};
        static LARGE_INTEGER freq = {};

        if ( freq.QuadPart == 0 )
            QueryPerformanceFrequency( &freq );

        LARGE_INTEGER now;
        QueryPerformanceCounter( &now );

        float dt = ( last.QuadPart == 0 ) ? 0.016f
            : static_cast< float >( now.QuadPart - last.QuadPart ) / freq.QuadPart;

        last = now;
        return dt;
    }

    __int64( __fastcall* present_org )( IDXGISwapChain*, std::uint32_t, std::uint32_t ) = nullptr;
    HRESULT( __fastcall* resizebuffers_org )( IDXGISwapChain*, UINT, UINT, UINT, DXGI_FORMAT, UINT ) = nullptr;

    HRESULT __fastcall resizebuffers_hk( IDXGISwapChain* swapchain, UINT buffer_count, UINT width, UINT height, DXGI_FORMAT new_format, UINT swapchain_flags ) {
        const auto call_id = InterlockedIncrement( &g_resizebuffers_call_count );
        const auto thread_id = GetCurrentThreadId( );

        g_pending_resize_reinit = true;
        if ( resizebuffers_org ) {
            const auto hr = resizebuffers_org( swapchain, buffer_count, width, height, new_format, swapchain_flags );
            return hr;
        }

        print_info( oxorany( "ResizeBuffers: original function pointer is null\n" ) );

        return E_FAIL;
    }
    __int64 __fastcall present_hk( IDXGISwapChain* swapchain, std::uint32_t sync_interval, std::uint32_t flags ) {
        static bool initialized = false;
        static bool logged_dx11_backend = false;
        static bool logged_dx12_backend = false;
        static bool attempted_dx12_init = false;
        static bool logged_dx12_missing_queue = false;
        static bool logged_dx12_init_failed = false;

        if ( g_pending_resize_reinit ) {
            char pending_msg[ 256 ]{};
            sprintf_s(
                pending_msg,
                oxorany( "present_hk: applying pending reinit (thread=%lu sc=%p backend=%d)\n" ),
                GetCurrentThreadId( ),
                swapchain,
                static_cast< int >( dll::m_backend )
            );
            print_info( pending_msg );

            g_interface->shutdown( );

            if ( dll::m_device ) { dll::m_device->Release( ); dll::m_device = nullptr; }
            if ( dll::m_context ) { dll::m_context->Release( ); dll::m_context = nullptr; }
            if ( dll::m_device12 ) { dll::m_device12->Release( ); dll::m_device12 = nullptr; }

            initialized = false;
            attempted_dx12_init = false;
            logged_dx12_missing_queue = false;
            logged_dx12_init_failed = false;
            dll::m_backend = dll::backend_t::unknown;
            dll::m_hooked_swapchain = nullptr;
            g_pending_resize_reinit = false;
        }

        if ( initialized ) {
            bool need_reinit = false;

            if ( swapchain != dll::m_hooked_swapchain )
                need_reinit = true;

            if ( dll::m_backend == dll::backend_t::dx12 ) {
                ID3D12Device* current_sc_dev12 = nullptr;
                if ( SUCCEEDED( swapchain->GetDevice( __uuidof( ID3D12Device ), reinterpret_cast< void** >( &current_sc_dev12 ) ) ) ) {
                    if ( current_sc_dev12 != dll::m_device12 )
                        need_reinit = true;
                    if ( current_sc_dev12 )
                        current_sc_dev12->Release( );
                }
            }
            else if ( dll::m_backend == dll::backend_t::dx11 ) {
                ID3D11Device* current_sc_dev11 = nullptr;
                if ( SUCCEEDED( swapchain->GetDevice( __uuidof( ID3D11Device ), reinterpret_cast< void** >( &current_sc_dev11 ) ) ) ) {
                    if ( current_sc_dev11 != dll::m_device )
                        need_reinit = true;
                    if ( current_sc_dev11 )
                        current_sc_dev11->Release( );
                }
            }

            if ( need_reinit ) {
                print_info( oxorany( "present_hk swapchain/device changed, reinitializing renderer\n" ) );

                g_interface->shutdown( );

                if ( dll::m_device ) { dll::m_device->Release( ); dll::m_device = nullptr; }
                if ( dll::m_context ) { dll::m_context->Release( ); dll::m_context = nullptr; }
                if ( dll::m_device12 ) { dll::m_device12->Release( ); dll::m_device12 = nullptr; }

                initialized = false;
                attempted_dx12_init = false;
                logged_dx12_missing_queue = false;
                logged_dx12_init_failed = false;
                dll::m_backend = dll::backend_t::unknown;
                dll::m_hooked_swapchain = swapchain;
            }
        }

        if ( !initialized ) {
            if ( SUCCEEDED( swapchain->GetDevice( __uuidof( ID3D11Device ), reinterpret_cast< void** >( &dll::m_device ) ) ) ) {
                dll::m_backend = dll::backend_t::dx11;
                if ( !logged_dx11_backend ) {
                    print_info( oxorany( "present_hk backend=dx11\n" ) );
                    logged_dx11_backend = true;
                }
                dll::m_device->GetImmediateContext( &dll::m_context );
                if ( g_interface->init( dll::m_device, dll::m_context, swapchain ) ) {
                    initialized = true;
                    log_info(
                        oxorany( "DirectX 11 renderer initialized successfully.\n"
                        "Note: DX11 has known instabilities\n\n"
                        "If you experience any issues follow the instructions below:\n"
                        "Report them in our discord via a support ticket." ),
                        oxorany( "Initialized: DirectX 11" ), MB_ICONINFORMATION );
                }
                else {
                    if ( dll::m_device ) { dll::m_device->Release( ); dll::m_device = nullptr; }
                    if ( dll::m_context ) { dll::m_context->Release( ); dll::m_context = nullptr; }
                    dll::m_backend = dll::backend_t::unknown;
                }
            }
            else {
                ID3D12Device* swapchain_device12 = nullptr;
                if ( SUCCEEDED( swapchain->GetDevice( __uuidof( ID3D12Device ), reinterpret_cast< void** >( &swapchain_device12 ) ) ) ) {
                    dll::m_backend = dll::backend_t::dx12;
                    if ( !logged_dx12_backend ) {
                        print_info( oxorany( "present_hk backend=dx12\n" ) );
                        logged_dx12_backend = true;
                    }

                    if ( attempted_dx12_init ) {
                        if ( swapchain_device12 ) swapchain_device12->Release( );
                        return present_org( swapchain, sync_interval, flags );
                    }
                    attempted_dx12_init = true;

                    if ( !dll::m_cmd_queue ) {
                        if ( !logged_dx12_missing_queue ) {
                            print_info( oxorany( "present_hk dx12 queue missing; skipping init\n" ) );
                            logged_dx12_missing_queue = true;
                        }
                        dll::m_backend = dll::backend_t::unknown;
                        if ( swapchain_device12 ) swapchain_device12->Release( );
                        dll::m_device12 = nullptr;
                        return present_org( swapchain, sync_interval, flags );
                    }

                    dll::m_device12 = swapchain_device12;
                    if ( g_interface->init_dx12( dll::m_device12, dll::m_cmd_queue, swapchain ) ) {
                        initialized = true;
                        log_info(
                            oxorany( "DirectX 12 renderer initialized successfully.\n"
                            "You should have no issues as this is the recommended render.\n\n"
                            "If you experience any issues follow the instructions below:\n"
                            "Report them in our discord via a support ticket." ),
                            oxorany( "Initialized: DirectX 12" ), MB_ICONINFORMATION );
                    }

                    if ( !initialized ) {
                        if ( !logged_dx12_init_failed ) {
                            print_info( oxorany( "present_hk dx12 init failed (single-attempt)\n" ) );
                            logged_dx12_init_failed = true;
                        }
                        dll::m_backend = dll::backend_t::unknown;
                        if ( swapchain_device12 ) swapchain_device12->Release( );
                        dll::m_device12 = nullptr;
                    }
                }
            }

            if ( !initialized )
                return present_org( swapchain, sync_interval, flags );

            auto& c = *g_interface;
            if ( !peach::text::init_fonts( c ) ) {
                log_info( "could not load fonts.\n" );
                return present_org( swapchain, sync_interval, flags );
            }

            c.set_text_antialiasing( true );
            c.set_shape_antialiasing( true );
            c.set_text_pixel_snapping( true );
        }

        const auto use_dx12 = ( dll::m_backend == dll::backend_t::dx12 );
        if ( use_dx12 )
            g_interface->begin_frame_dx12( peach::color_t( 0, 0, 0, 0 ) );
        else
            g_interface->begin_frame( peach::color_t( 0, 0, 0, 0 ) );

        static float time_offset = 0.f;
        float dt = get_delta_time( );
        time_offset += dt;

        auto cleanup = [ &, use_dx12 ] ( ) -> __int64 {
            if ( GetAsyncKeyState( VK_INSERT ) & 1 ) {
                g_menu.open = !g_menu.open;
                if ( !g_menu.open ) {
                    g_search.focused = false;
                    g_search.len = 0;
                    g_search.buf[ 0 ] = '\0';
                }
            }

            if ( g_menu.open ) {
                ui::poll( );

                const auto vp_w = g_interface->get_screen_width( );
                const auto vp_h = g_interface->get_screen_height( );

                g_interface->draw_rect(
                    0.f, 0.f, vp_w, vp_h,
                    peach::color_t( 0, 0, 0, 153 )
                );

                g_interface->draw_rect_gradient(
                    0.f, vp_h - 180.f,
                    vp_w, 180.f,
                    peach::color_t( 20, 40, 100, 0 ),
                    peach::color_t( 65, 130, 180, 120 )
                );

                static float m_aura_intensity = 0.f;
                if ( g_menu.open )
                    m_aura_intensity = min( 1.f, m_aura_intensity + dt * 2.f );
                else
                    m_aura_intensity = max( 0.f, m_aura_intensity - dt * 2.f );

                g_interface->draw_rect_gradient(
                    0.f, vp_h - 120.f * m_aura_intensity,
                    vp_w, 120.f * m_aura_intensity,
                    peach::color_t( 0, 0, 0, 0 ),
                    peach::color_t( 20, 130, 255, static_cast< uint8_t >( 80 * m_aura_intensity ) )
                );

                if ( settings::misc::watermark ) {
                    static float bounce_x = 100.f;
                    static float bounce_y = 100.f;
                    static float vel_x = 2.5f;
                    static float vel_y = 2.0f;

                    const auto   wm_text = oxorany( L"divinity" );
                    const auto   size = 20.f;
                    const auto   screen_w = g_interface->get_screen_width( );
                    const auto   screen_h = g_interface->get_screen_height( );
                    const float  text_w = 90.f;
                    const float  text_h = size;

                    bounce_x += vel_x;
                    bounce_y += vel_y;

                    if ( bounce_x + text_w >= screen_w || bounce_x <= 0.f ) {
                        vel_x = -vel_x;
                        bounce_x = std::clamp( bounce_x, 0.f, screen_w - text_w );
                    }
                    if ( bounce_y + text_h >= screen_h || bounce_y <= 0.f ) {
                        vel_y = -vel_y;
                        bounce_y = std::clamp( bounce_y, 0.f, screen_h - text_h );
                    }

                    peach::text::draw( wm_text, bounce_x, bounce_y, size,
                        peach::color_t( 94, 105, 238, 255 ),
                        peach::text_align::left,
                        settings::misc::watermark_shadow ? peach::text::style::shadow : peach::text::style::none );
                }

                if ( settings::misc::particles_enabled ) {
                    static float spawn_timer = 0.f;
                    spawn_timer += dt;
                    if ( spawn_timer >= settings::misc::particle_spawn_rate ) {
                        spawn_timer = 0.f;
                        float rx = static_cast< float >( rand( ) % static_cast< int >( g_interface->get_screen_width( ) ) );
                        float ry = static_cast< float >( rand( ) % static_cast< int >( g_interface->get_screen_height( ) ) );
                        particle::add_particle( rx, ry, 0.f, ( rand( ) % 2 ) ? 1.f : 0.f );
                    }
                }

                particle::render( dt, time_offset );
            }

            render_menu( );

            if ( use_dx12 )
                g_interface->flush_dx12( );
            else
                g_interface->flush( );
            return present_org( swapchain, sync_interval, flags );
            };

        double target_distance = DBL_MAX;
        double best_target_score = DBL_MAX;
        game::a_fort_player_pawn_athena* target_pawn = nullptr;

        const auto g_engine = game::u_engine::get_engine( );
        if ( !g_engine ) return cleanup( );

        const auto game_viewport = g_engine->game_viewport( );
        if ( !game_viewport ) return cleanup( );

        const auto world = game_viewport->world( );
        if ( !world ) return cleanup( );

        const auto game_state = world->game_state( );
        if ( !game_state ) return cleanup( );

        const auto game_instance = world->game_instance( );
        if ( !game_instance ) return cleanup( );

        const auto local_player = game_instance->get_localplayer( );
        if ( !local_player ) return cleanup( );

        const auto view_state = local_player->get_view_state( );
        if ( !view_state ) return cleanup( );
        if ( !view_state->update_camera( ) ) return cleanup( );

        const auto center_width = g_interface->get_screen_width( ) / 2.f;
        const auto center_height = g_interface->get_screen_height( ) / 2.f;
        const auto fov_radius = ( settings::aimbot::fov * center_width / game::m_field_of_view ) / 2.0;

        const auto player_controller = local_player->player_controller( );
        if ( !player_controller ) return cleanup( );

        const auto acknowledged_pawn = player_controller->acknowledged_pawn( );
        auto player_array = game_state->player_array( );

        if ( acknowledged_pawn ) {

        }

        static constexpr int m_max_candidates = 100;
        player_candidate_t   candidates[ m_max_candidates ];
        int                  candidate_count = 0;

        const auto now_time = game::kismet::m_gameplay_statics->get_real_time_seconds( world );
        if ( settings::trails::enabled ) {
            for ( auto i = 0; i < m_pool_size; ++i ) {
                auto& t = m_trail_pool[ i ];
                if ( !t.used ) continue;

                int live = 0;
                for ( auto j = 0; j < m_trail_size; ++j ) {
                    if ( !t.points[ j ].alive ) continue;
                    ( now_time - t.points[ j ].timestamp ) > settings::trails::duration
                        ? t.points[ j ].alive = false
                        : ++live;
                }
                if ( !live && ( now_time - t.last_push ) > settings::trails::duration )
                    t.used = false;
            }
        }

            auto levels = world->levels( );
            for ( auto level_idx = 0; level_idx < levels.size( ); level_idx++ ) {
                const auto level = levels[ level_idx ];
                if ( !level ) continue;

                auto actors = level->actors( );
                for ( auto actor_idx = 0; actor_idx < actors.size( ); actor_idx++ ) {
                    const auto actor = actors[ actor_idx ];
                    if ( !actor ) continue;

                    if ( settings::misc::item_esp ) {
                        if ( settings::misc::show_ground_loot ) {
                            auto pickup = reinterpret_cast< game::a_fort_pickup* >( actor );
                            if ( pickup->object_is_a( game::classes::m_fort_pickup ) ) {
                                game::fvector2d actor_position;
                                auto actor_location = pickup->k2_get_actor_location( );
                                if ( !game::world_to_screen( actor_location, &actor_position ) )  continue;
                                if ( !g_interface->in_screen( actor_position ) )  continue;

                                auto distance = actor_location.distance_to( game::m_camera_location );
                                if ( distance > settings::misc::item_max_distance )  continue;

                                auto item_entry = pickup->primary_pickup_item_entry( );
                                auto item_definition = item_entry.item_definition;
                                if ( !item_definition )  continue;

                                auto rarity = item_definition->rarity( );
                                const auto get_rarity_col = [ rarity ] ( ) {
                                    switch ( rarity ) {
                                        case game::e_fort_rarity::common:       return peach::color_t( 185, 185, 185, 255 );
                                        case game::e_fort_rarity::uncommon:     return peach::color_t( 96, 170, 58, 255 );
                                        case game::e_fort_rarity::rare:         return peach::color_t( 75, 128, 255, 255 );
                                        case game::e_fort_rarity::epic:         return peach::color_t( 209, 110, 244, 255 );
                                        case game::e_fort_rarity::legendary:    return peach::color_t( 255, 189, 62, 255 );
                                        case game::e_fort_rarity::mythic:       return peach::color_t( 255, 215, 0, 255 );
                                        case game::e_fort_rarity::transcendent: return peach::color_t( 0, 205, 250, 255 );
                                        default:                                return peach::color_t( 185, 185, 185, 255 );
                                    }
                                    };

                                auto display_name = item_definition->item_name( ).get( );
                                if ( !display_name )  continue;

                                auto rarity_col = get_rarity_col( );
                                auto text_height = g_interface->measure_text_height( oxorany( "Manrope" ), settings::misc::item_size );

                                peach::text::draw( display_name.c_str( ),
                                    actor_position.m_x, actor_position.m_y - text_height,
                                    settings::misc::item_size, rarity_col );

                                wchar_t distance_text[ 32 ];
                                swprintf_s( distance_text, L"[%.1fm]", distance );

                                peach::text::draw( distance_text,
                                    actor_position.m_x, actor_position.m_y,
                                    settings::misc::item_size, rarity_col );
                            }
                        }

                        if ( settings::misc::show_chest || settings::misc::show_ammobox ) {
                            auto container = reinterpret_cast< game::a_building_container* >( actor );
                            if ( container->object_is_a( game::classes::m_container ) ) {
                                if ( container->b_already_searched( ) ) continue;

                                game::fvector2d actor_position;
                                auto actor_location = container->k2_get_actor_location( );
                                if ( !game::world_to_screen( actor_location, &actor_position ) ) continue;
                                if ( !g_interface->in_screen( actor_position ) ) continue;

                                peach::color_t   rarity_col;
                                const wchar_t* display_name = nullptr;
                                auto object_name = container->get_object_name( );

                                bool is_chest = true;
                                if ( settings::misc::show_chest &&
                                    game::kismet::m_string_library->contains( object_name, oxorany( L"Tiered_Chest" ), true, false ) ) {
                                    display_name = oxorany( L"Chest" );
                                    rarity_col = peach::color_t( 255, 162, 0 );
                                }
                                else if ( settings::misc::show_ammobox &&
                                    game::kismet::m_string_library->contains( object_name, oxorany( L"Tiered_Ammo" ), true, false ) ) {
                                    display_name = oxorany( L"Ammobox" );
                                    rarity_col = peach::color_t( 65, 83, 59 );
                                    is_chest = false;
                                }

                                auto max_distance = is_chest ? settings::misc::chest_max_distance : settings::misc::ammo_max_distance;
                                auto distance = actor_location.distance_to( game::m_camera_location );
                                if ( distance > max_distance ) continue;

                                if ( display_name ) {
                                    auto text_height = g_interface->measure_text_height( oxorany( "Manrope" ), settings::misc::item_size );

                                    peach::text::draw( display_name,
                                        actor_position.m_x, actor_position.m_y - text_height,
                                        settings::misc::item_size, rarity_col );

                                    wchar_t distance_text[ 32 ];
                                    swprintf_s( distance_text, L"[%.1fm]", distance );

                                    peach::text::draw( distance_text,
                                        actor_position.m_x, actor_position.m_y,
                                        settings::misc::item_size, rarity_col );
                                }
                            }
                        }
                    }

                    if ( settings::visuals::enabled ) {
                        auto player = reinterpret_cast< game::a_fort_player_pawn_athena* >( actor );
                        if ( player->object_is_a( game::classes::m_fort_player_pawn_athena ) ) {
                            if ( !utility::is_valid( player ) || player->is_dead( ) ) continue;
                            auto current_weapon = player->current_weapon( );
                            auto is_downed = player->is_dbno( );
                            auto is_teammate = game::kismet::m_fort_library->on_same_team(
                                player, acknowledged_pawn );

                            auto player_state = player->player_state( );
                            if ( !player_state ) continue;
                            if ( !utility::is_valid( player_state ) )  continue;
                            auto is_bot = player_state->is_a_bot( );

                            auto main_col = peach::color_t( 94, 105, 238, 255 );
                            auto second_col = peach::color_t( 113, 137, 231, 255 );
                            auto third_col = peach::color_t( 100, 144, 232, 255 );
                            auto fourth_col = peach::color_t( 100, 144, 232, 255 );

                            auto cham_color = game::flinear_color{
                                settings::chams::color_r,
                                settings::chams::color_g,
                                settings::chams::color_b,
                                1.0f };

                            auto mesh = player->mesh( );
                            if ( !mesh ) continue;

                            static bool m_profile_set = false;
                            if ( !m_profile_set && local_player ) {
                                auto name = game::kismet::m_fort_library->get_human_readable_name( player );
                                if ( name && name.c_str( ) ) {
                                    profile::set_name( name.c_str( ) );
                                    m_profile_set = true;
                                }
                            }

                            if ( utility::is_valid( acknowledged_pawn ) ) {
                                if ( player == acknowledged_pawn ) {
                                    auto local_weapon = acknowledged_pawn->current_weapon( );

                                    //if ( settings::tracers::enabled &&
                                    //    utility::is_valid( local_weapon ) &&
                                    //    local_weapon->object_is_a( game::classes::m_fort_weapon ) &&
                                    //    local_weapon->is_firing( ) ) {

                                    //    static double last_tracer_time = 0.0;
                                    //    if ( ( now_time - last_tracer_time ) >= settings::tracers::min_spawn_interval ) {
                                    //        auto impact_direction = acknowledged_pawn->last_fired_direction( );
                                    //        if ( !impact_direction ) goto skip_tracer;
                                    //        impact_direction.normalize( );

                                    //        auto impact_location = acknowledged_pawn->last_fired_location( );
                                    //        if ( !impact_location ) goto skip_tracer;

                                    //        auto distance = static_cast< double >( local_weapon->current_projected_impact_distance( ) );
                                    //        auto end_location = impact_location + ( impact_direction * distance );

                                    //        auto direction = game::m_camera_rotation.get_forward_vector( );
                                    //        auto impact_angle = game::kismet::m_math_library->acos( direction.dot( impact_direction ) );
                                    //        auto angle_degrees = impact_angle * ( 180.0 / k_pi< double > );

                                    //        bool is_tracer_found = false;
                                    //        if ( settings::tracers::deduplicate ) {
                                    //            for ( auto n = 0; n < m_tracer_cache.count; ++n ) {
                                    //                int slot = ( ( m_tracer_cache.head - m_tracer_cache.count + n + ( int )m_max_tracers ) % ( int )m_max_tracers );
                                    //                auto& tracer = m_tracer_cache.entries[ slot ];
                                    //                if ( tracer.m_start.distance_to( impact_location ) < 0.50 &&
                                    //                    tracer.m_end.distance_to( end_location ) < 0.50 ) {
                                    //                    is_tracer_found = true;
                                    //                    break;
                                    //                }
                                    //            }
                                    //        }

                                    //        if ( !is_tracer_found ) {
                                    //            m_tracer_cache.entries[ m_tracer_cache.head ] = {
                                    //                impact_location, end_location, angle_degrees, now_time };

                                    //            m_tracer_cache.head = ( m_tracer_cache.head + 1 ) % ( int )m_max_tracers;
                                    //            if ( m_tracer_cache.count < ( int )m_max_tracers )
                                    //                ++m_tracer_cache.count;

                                    //            last_tracer_time = now_time;
                                    //        }
                                    //    }
                                    //}
                                skip_tracer:;
                                    continue;
                                }
                            }

                            if ( settings::chams::enabled ) {
                                static game::u_material* cham_material = nullptr;
                                if ( !cham_material )
                                    cham_material = game::u_object::find_object<game::u_material*>(
                                    L"/Game/Effects/Fort_Effects/Effects/Characters/Generic/ResIn/RezIn_Master.RezIn_Master" );

                                if ( cham_material && utility::is_valid( cham_material ) ) {
                                    cham_material->blend_mode( game::e_blend_mode::additive );
                                    cham_material->wireframe( settings::chams::wireframe );
                                    cham_material->b_disable_depth_test( settings::chams::ignore_depth );
                                    cham_material->two_sided( settings::chams::two_sided );
                                    cham_material->b_use_emissive_for_dynamic_area_lighting( settings::chams::dynamic_area_light );

                                    const auto apply_cham_params = [ & ] ( game::u_material_instance_dynamic* instance ) {
                                        if ( !utility::is_valid( instance ) ) return;

                                        const auto apply_param = [ & ] ( const wchar_t* name ) {
                                            instance->set_vector_parameter_value(
                                                game::kismet::m_string_library->conv_string_to_name( name ),
                                                cham_color );
                                            };

                                        apply_param( L"WireFrameFadeOffColor" );
                                        apply_param( L"WireFrameParameterHighlight" );
                                        apply_param( L"DissolveGradientDirectionalVector" );
                                        apply_param( L"DissolveGradientBaseLocation" );
                                        apply_param( L"DissolveSphericalGradientHandLocationAndRadius" );
                                        apply_param( L"AverageHandPosition" );
                                        apply_param( L"EmissiveMultiplier" );
                                        apply_param( L"colcol1" );
                                        apply_param( L"colcol2" );
                                        apply_param( L"offset" );
                                        apply_param( L"Top Color" );
                                        apply_param( L"Bottom Color" );
                                        };

                                    const auto apply_chams_to_mesh = [ & ] ( auto* skeletal_mesh ) {
                                        if ( !utility::is_valid( skeletal_mesh ) ) return;
                                        if ( !skeletal_mesh->object_is_a( game::classes::m_skeletal_mesh_component ) ) return;
                                        if ( !skeletal_mesh->outer_private( ) ) return;

                                        auto materials = skeletal_mesh->get_materials( );
                                        if ( !materials.size( ) || materials.size( ) >= 10 ) return;

                                        for ( auto slot = 0; slot < ( int )materials.size( ); slot++ ) {
                                            auto material = materials[ slot ];
                                            if ( !utility::is_valid( material ) ) continue;
                                            if ( !material->object_is_a( game::classes::m_material_interface ) ) continue;

                                            auto as_mi = reinterpret_cast< game::u_material_instance* >( material );
                                            if ( !utility::is_valid( as_mi ) ) continue;
                                            if ( as_mi->parent( ) == cham_material ) continue;
                                            if ( !material->outer_private( ) ) continue;

                                            auto instance = skeletal_mesh->create_dynamic_material_instance(
                                                slot, cham_material, game::fname( ) );
                                            if ( !utility::is_valid( instance ) ) continue;
                                            if ( !instance->object_is_a( game::classes::m_material_instance_dynamic ) ) continue;

                                            auto as_instance = reinterpret_cast< game::u_material_instance* >( instance );
                                            if ( !as_instance->parent( ) || as_instance->parent( ) != cham_material ) continue;
                                            if ( !instance->outer_private( ) ) continue;

                                            apply_cham_params( instance );
                                            skeletal_mesh->set_material( slot, instance );
                                        }
                                        };

                                    auto skeletal_meshes = player->skeletal_meshes( );
                                    for ( auto i = 0; i < skeletal_meshes.size( ); i++ )
                                        apply_chams_to_mesh( skeletal_meshes[ i ] );

                                    if ( settings::chams::weapon ) {
                                        if ( utility::is_valid( current_weapon ) ) {
                                            auto weapon_meshes = current_weapon->all_weapon_meshes( );
                                            for ( auto i = 0; i < weapon_meshes.size( ); i++ )
                                                apply_chams_to_mesh( weapon_meshes[ i ] );
                                        }
                                    }
                                }
                            }

                            game::fvector2d head_position;
                            auto head_location = mesh->get_bone_location( oxorany( L"head" ) );
                            if ( !game::world_to_screen( head_location, &head_position ) ) continue;

                            game::fvector2d root_position;
                            auto root_location = mesh->get_bone_location( oxorany( L"root" ) );
                            if ( !game::world_to_screen( root_location, &root_position ) ) continue;

                            auto world_dist = root_location.distance_to( game::m_camera_location );
                            if ( world_dist > settings::visuals::max_esp_distance ) continue;

                            auto is_visible = game::kismet::m_fort_library->check_line_of_sight_to_actor_with_channel(
                                game::m_camera_location,
                                player,
                                game::e_collision_channel::visibility,
                                acknowledged_pawn );

                            if ( settings::visuals::color_by_visibility && !is_visible ) {
                                main_col = peach::color_t( 249, 169, 249, 255 );
                                second_col = peach::color_t( 208, 147, 230, 255 );
                                fourth_col = peach::color_t( 197, 121, 224, 255 );
                            }

                            if ( settings::visuals::color_teammates && is_teammate ) {
                                main_col = peach::color_t( 248, 132, 63, 255 );
                                second_col = peach::color_t( 242, 166, 90, 255 );
                                fourth_col = peach::color_t( 255, 146, 72, 255 );
                            }

                            if ( settings::visuals::off_screen_arrows && acknowledged_pawn ) {
                                auto ack_weapon = acknowledged_pawn->current_weapon( );
                                if ( ack_weapon &&
                                    ( settings::aimbot::weapon_only ? ack_weapon->object_is_a( game::classes::m_fort_weapon ) : true )
                                    ) {
                                    if ( !g_interface->in_circle( fov_radius, head_position ) ) {
                                        const auto current_world = game_viewport->world( );
                                        if ( !current_world ) return cleanup( );

                                        auto delta_seconds = game::kismet::m_gameplay_statics->get_world_delta_seconds( current_world );

                                        auto angle = game::kismet::m_math_library->atan2(
                                            head_position.m_y - center_height,
                                            head_position.m_x - center_width );

                                        game::fvector2d arrow_position{
                                            center_width + ( fov_radius + 12.0 ) * game::kismet::m_math_library->cos( angle ),
                                            center_height + ( fov_radius + 12.0 ) * game::kismet::m_math_library->sin( angle )
                                        };

                                        auto rotation_angle = angle - delta_seconds;

                                        game::fvector2d arrow_vertex[ 3 ]{
                                            game::fvector2d(
                                                arrow_position.m_x + game::kismet::m_math_library->cos( rotation_angle ) * 15.0,
                                                arrow_position.m_y + game::kismet::m_math_library->sin( rotation_angle ) * 15.0 ),
                                            game::fvector2d(
                                                arrow_position.m_x + game::kismet::m_math_library->cos( rotation_angle - 1.5 ) * 12.0,
                                                arrow_position.m_y + game::kismet::m_math_library->sin( rotation_angle - 1.5 ) * 12.0 ),
                                            game::fvector2d(
                                                arrow_position.m_x + game::kismet::m_math_library->cos( rotation_angle + 1.5 ) * 12.0,
                                                arrow_position.m_y + game::kismet::m_math_library->sin( rotation_angle + 1.5 ) * 12.0 ),
                                        };

                                        for ( float t = 0.f; t <= 1.0f; t += 0.1f ) {
                                            game::fvector2d left{
                                                arrow_vertex[ 0 ].m_x + ( arrow_vertex[ 1 ].m_x - arrow_vertex[ 0 ].m_x ) * t,
                                                arrow_vertex[ 0 ].m_y + ( arrow_vertex[ 1 ].m_y - arrow_vertex[ 0 ].m_y ) * t };
                                            game::fvector2d right{
                                                arrow_vertex[ 0 ].m_x + ( arrow_vertex[ 2 ].m_x - arrow_vertex[ 0 ].m_x ) * t,
                                                arrow_vertex[ 0 ].m_y + ( arrow_vertex[ 2 ].m_y - arrow_vertex[ 0 ].m_y ) * t };
                                            g_interface->draw_line( left.m_x, left.m_y, right.m_x, right.m_y, 1.5f, main_col );
                                        }
                                    }
                                }
                            }

                            if ( !g_interface->in_screen( head_position ) ) continue;

                            if ( !is_teammate ) {
                                bool skip = false;
                                if ( is_downed && !settings::aimbot::aim_at_downed ) skip = true;
                                if ( is_bot && !settings::aimbot::aim_at_bots ) skip = true;
                                if ( settings::aimbot::visible_only && !is_visible ) skip = true;

                                if ( !skip && g_interface->in_circle( fov_radius, head_position ) ) {
                                    if ( candidate_count < m_max_candidates )
                                        candidates[ candidate_count++ ] = {
                                            player,
                                            head_position,
                                            root_position,
                                            world_dist,
                                            is_visible, is_bot, is_downed
                                    };
                                }
                            }

                            auto origin = game::fvector( );
                            auto extent = game::fvector( );
                            auto sphere_radius = 0.f;
                            game::kismet::m_system_library->get_component_bounds( mesh, &origin, &extent, &sphere_radius );

                            game::fvector2d min_pos( DBL_MAX, DBL_MAX );
                            game::fvector2d max_pos( -DBL_MAX, -DBL_MAX );

                            for ( int i = 0; i < 8; i++ ) {
                                auto corner = game::fvector(
                                    ( i & 1 ) ? extent.m_x : -extent.m_x,
                                    ( i & 2 ) ? extent.m_y : -extent.m_y,
                                    ( i & 4 ) ? extent.m_z : -extent.m_z );

                                game::fvector2d origin_2d;
                                if ( !game::world_to_screen( origin + corner, &origin_2d ) ) continue;

                                min_pos.m_x = min( min_pos.m_x, origin_2d.m_x );
                                min_pos.m_y = min( min_pos.m_y, origin_2d.m_y );
                                max_pos.m_x = max( max_pos.m_x, origin_2d.m_x );
                                max_pos.m_y = max( max_pos.m_y, origin_2d.m_y );
                            }

                            auto x = min_pos.m_x;
                            auto y = min_pos.m_y;
                            auto w = max_pos.m_x - min_pos.m_x;
                            auto h = max_pos.m_y - min_pos.m_y;

                            const auto top_offset = 0.5f;
                            const auto bottom_offset = 3.0f;
                            const auto label_offset = 15.f;

                            const auto line_height = g_interface->measure_text_height( oxorany( "Manrope" ), settings::visuals::font_size ) + 1.f;
                            const auto icon_height = g_interface->measure_text_height( oxorany( "Font Awesome" ), settings::visuals::font_size * 1.8 );

                            auto bottom_y = y + h + bottom_offset;
                            auto label_y = y + label_offset;
                            auto top_y = y - top_offset;

                            const auto draw_top_label = [ & ] ( const wchar_t* lbl_text, peach::color_t col ) {
                                top_y -= line_height + top_offset;
                                peach::text::draw( lbl_text, x + w * 0.5f, top_y, settings::visuals::font_size, col );
                                };

                            const auto draw_icon = [ & ] ( const char* icon, peach::color_t col, peach::color_t outline_col ) {
                                const auto icon_width = g_interface->measure_text( icon, oxorany( "Font Awesome" ), settings::visuals::font_size * 1.8 );
                                g_interface->draw_text(
                                    icon,
                                    x - icon_width - label_offset,
                                    label_y,
                                    "Font Awesome",
                                    settings::visuals::font_size * 1.8,
                                    col,
                                    outline_col,
                                    1.5f
                                );

                                const float offsets[ ][ 2 ] = {
                                    { -1, -1 }, {  1, -1 }, { -1,  1 },
                                    {  1,  1 }, { -1,  0 }, {  1,  0 },
                                    {  0, -1 }, {  0,  1 }
                                };
                                for ( const auto& o : offsets )
                                    g_interface->draw_text( icon,
                                    x + o[ 0 ], y + o[ 1 ],
                                    oxorany( "Font Awesome" ), settings::visuals::font_size * 1.8,
                                    outline_col, peach::color_t( 0, 0, 0, 0 ),
                                    0.f );

                                label_y += icon_height + label_offset;
                                };

                            const auto draw_label = [ & ] ( const wchar_t* lbl_text, peach::color_t col ) {
                                peach::text::draw( lbl_text, x + w * 0.5f, bottom_y, settings::visuals::font_size, col );
                                bottom_y += line_height - bottom_offset;
                                };

                            if ( settings::trails::enabled ) {
                                const auto pawn_key = reinterpret_cast< std::uint64_t >( player );

                                player_trail_t* trail = nullptr;
                                player_trail_t* free_slot = nullptr;

                                for ( auto i = 0; i < m_pool_size; ++i ) {
                                    if ( m_trail_pool[ i ].used && m_trail_pool[ i ].key == pawn_key ) {
                                        trail = &m_trail_pool[ i ];
                                        break;
                                    }
                                    if ( !m_trail_pool[ i ].used && !free_slot )
                                        free_slot = &m_trail_pool[ i ];
                                }

                                if ( !trail && free_slot ) {
                                    free_slot->key = pawn_key;
                                    free_slot->used = true;
                                    free_slot->head = 0;
                                    free_slot->count = 0;
                                    trail = free_slot;
                                }

                                if ( trail && ( now_time - trail->last_push ) >= settings::trails::push_interval ) {
                                    auto slot = trail->head;
                                    trail->points[ slot ] = { root_location, now_time, main_col, true };
                                    trail->head = ( trail->head + 1 ) & ( m_trail_size - 1 );
                                    if ( trail->count < m_trail_size ) trail->count++;
                                    trail->last_push = now_time;
                                }

                                if ( trail && trail->count >= 2 ) {
                                    game::fvector2d prev_screen;
                                    bool            has_prev = false;

                                    for ( auto n = 0; n < trail->count; ++n ) {
                                        auto  pt_idx = ( trail->head - trail->count + n + m_trail_size ) & ( m_trail_size - 1 );
                                        auto& pt = trail->points[ pt_idx ];
                                        if ( !pt.alive ) { has_prev = false; continue; }

                                        game::fvector2d curr_screen;
                                        if ( !game::world_to_screen( pt.position, &curr_screen ) ) {
                                            has_prev = false;
                                            continue;
                                        }

                                        if ( has_prev &&
                                            ( g_interface->in_screen( prev_screen ) ||
                                            g_interface->in_screen( curr_screen )
                                            ) ) {
                                            auto age_ratio = min( ( now_time - pt.timestamp ) / settings::trails::duration, 1.0 );
                                            auto alpha = static_cast< uint8_t >( ( 1.0 - age_ratio ) * 255.0 );
                                            if ( alpha > 0 )
                                                g_interface->draw_line(
                                                prev_screen.m_x, prev_screen.m_y,
                                                curr_screen.m_x, curr_screen.m_y,
                                                settings::trails::line_thickness,
                                                peach::color_t( pt.col.r, pt.col.g, pt.col.b, alpha ) );
                                        }

                                        prev_screen = curr_screen;
                                        has_prev = true;
                                    }
                                }
                            }

                            if ( settings::visuals::look_direction ) {
                                auto root_component = player->root_component( );
                                if ( root_component ) {
                                    auto direction = root_component->get_forward_vector( );
                                    auto view_location = head_location + ( direction * 160.0 );

                                    game::fvector2d view_position;
                                    if ( game::world_to_screen( view_location, &view_position ) )
                                        g_interface->draw_line(
                                        head_position.m_x,
                                        head_position.m_y,
                                        view_position.m_x,
                                        view_position.m_y,
                                        1.5f,
                                        main_col );
                                }
                            }

                            if ( settings::visuals::china_hat ) {
                                auto tip_3d = game::fvector( head_location.m_x, head_location.m_y, head_location.m_z + 20.f );

                                game::fvector2d tip_screen;
                                bool tip_valid = game::world_to_screen( tip_3d, &tip_screen );

                                const int segments = 24;
                                const float angle_step = 2.0f * k_pi<float> / segments;

                                game::fvector2d base_points[ 64 ]; // cap segments at 64
                                bool base_valid[ 64 ] = {};

                                for ( int i = 0; i < segments; i++ ) {
                                    const float angle = angle_step * i;
                                    game::fvector base_3d = {
                                        head_location.m_x + cosf( angle ) * 25.f,
                                        head_location.m_y + sinf( angle ) * 25.f,
                                        head_location.m_z - 12.f
                                    };
                                    base_valid[ i ] = game::world_to_screen( base_3d, &base_points[ i ] );
                                }

                                // draw base ring
                                for ( int i = 0; i < segments; i++ ) {
                                    int next = ( i + 1 ) % segments;
                                    if ( base_valid[ i ] && base_valid[ next ] )
                                        g_interface->draw_line(
                                        base_points[ i ].m_x, base_points[ i ].m_y,
                                        base_points[ next ].m_x, base_points[ next ].m_y,
                                        1.5f, main_col );
                                }

                                // draw tip lines
                                if ( tip_valid ) {
                                    for ( int i = 0; i < segments; i++ ) {
                                        if ( base_valid[ i ] )
                                            g_interface->draw_line(
                                            tip_screen.m_x, tip_screen.m_y,
                                            base_points[ i ].m_x, base_points[ i ].m_y,
                                            1.5f, main_col );
                                    }
                                }
                            }

                            if ( current_weapon &&
                                current_weapon->outer_private( ) &&
                                current_weapon->object_is_a( game::classes::m_fort_weapon
                                ) ) {
                                if ( settings::visuals::show_reloading ) {
                                    if ( current_weapon->is_reloading( ) ) {
                                        const auto gun_icon = icons::to_utf8( 0xe54e );
                                        draw_icon(
                                            gun_icon.c_str( ),
                                            peach::color_t( 75, 112, 245, 255 ),
                                            peach::color_t( 3, 52, 110, 255 )
                                        );
                                    }
                                }

                                if ( settings::visuals::show_firing ) {
                                    if ( current_weapon->is_firing( ) ) {
                                        const auto fire_icon = icons::to_utf8( 0xf06d );
                                        draw_icon(
                                            fire_icon.c_str( ),
                                            peach::color_t( 255, 132, 29, 255 ),
                                            peach::color_t( 220, 0, 0, 255 )
                                        );
                                    }
                                }
                            }

                            if ( settings::visuals::bbox_filled ) {
                                g_interface->draw_rect_gradient( x, y, w, h,
                                    peach::color_t( second_col.r, second_col.g, second_col.b, 50 ),
                                    peach::color_t( second_col.r, second_col.g, second_col.b, 50 ) );
                            }

                            if ( settings::visuals::bbox_enabled ) {
                                g_interface->draw_rect_outline( min_pos.m_x, min_pos.m_y, w, h,
                                    settings::visuals::bbox_thickness, main_col );
                            }
                            else if ( settings::visuals::corner_box ) {
                                auto corner_len_x = min( w, h ) * 0.15f;
                                auto corner_len_y = min( w, h ) * 0.30f;
                                auto thickness = settings::visuals::bbox_thickness;

                                // top - left
                                g_interface->draw_line( x, y, x + corner_len_x, y, thickness, main_col );
                                g_interface->draw_line( x, y, x, y + corner_len_y, thickness, main_col );

                                // top - right
                                g_interface->draw_line( x + w, y, x + w - corner_len_x, y, thickness, main_col );
                                g_interface->draw_line( x + w, y, x + w, y + corner_len_y, thickness, main_col );

                                // bottom - left
                                g_interface->draw_line( x, y + h, x + corner_len_x, y + h, thickness, main_col );
                                g_interface->draw_line( x, y + h, x, y + h - corner_len_y, thickness, main_col );

                                // bottom - right
                                g_interface->draw_line( x + w, y + h, x + w - corner_len_x, y + h, thickness, main_col );
                                g_interface->draw_line( x + w, y + h, x + w, y + h - corner_len_y, thickness, main_col );
                            }
                            else if ( settings::visuals::threed_enabled ) {
                                const auto draw_3d_box = [ & ] ( const game::fvector& origin, const game::fvector& extent, peach::color_t col, float thickness ) {
                                    game::fvector corners[ 8 ] = {
                                        { origin.m_x - extent.m_x, origin.m_y - extent.m_y, origin.m_z - extent.m_z },
                                        { origin.m_x + extent.m_x, origin.m_y - extent.m_y, origin.m_z - extent.m_z },
                                        { origin.m_x - extent.m_x, origin.m_y + extent.m_y, origin.m_z - extent.m_z },
                                        { origin.m_x + extent.m_x, origin.m_y + extent.m_y, origin.m_z - extent.m_z },
                                        { origin.m_x - extent.m_x, origin.m_y - extent.m_y, origin.m_z + extent.m_z },
                                        { origin.m_x + extent.m_x, origin.m_y - extent.m_y, origin.m_z + extent.m_z },
                                        { origin.m_x - extent.m_x, origin.m_y + extent.m_y, origin.m_z + extent.m_z },
                                        { origin.m_x + extent.m_x, origin.m_y + extent.m_y, origin.m_z + extent.m_z },
                                    };

                                    game::fvector2d sc[ 8 ];
                                    for ( int i = 0; i < 8; i++ ) {
                                        if ( !game::world_to_screen( corners[ i ], &sc[ i ] ) )
                                            return;
                                    }

                                    auto edge = [ & ] ( int a, int b ) {
                                        g_interface->draw_line( sc[ a ].m_x, sc[ a ].m_y, sc[ b ].m_x, sc[ b ].m_y, thickness, col );
                                        };

                                    // bottom face
                                    edge( 0, 1 ); edge( 1, 3 ); edge( 3, 2 ); edge( 2, 0 );
                                    // top face
                                    edge( 4, 5 ); edge( 5, 7 ); edge( 7, 6 ); edge( 6, 4 );
                                    // vertical edges
                                    edge( 0, 4 ); edge( 1, 5 ); edge( 2, 6 ); edge( 3, 7 );
                                    };

                                draw_3d_box( origin, extent, main_col, settings::visuals::bbox_thickness );
                            }
                            else if ( settings::visuals::directional_box ) {
                                auto thickness = settings::visuals::bbox_thickness;

                                auto rotation = player->k2_get_actor_rotation( );
                                auto half_width = max( extent.m_x, extent.m_y );

                                auto yaw_rad = rotation.m_yaw * ( 3.14159265f / 180.f );
                                auto cos_yaw = cosf( yaw_rad );
                                auto sin_yaw = sinf( yaw_rad );

                                auto lx = -half_width * sin_yaw, ly = half_width * cos_yaw;
                                auto rx = half_width * sin_yaw, ry = -half_width * cos_yaw;

                                auto bottom_z = root_location.m_z;
                                auto top_z = origin.m_z + extent.m_z;

                                game::fvector corners_3d[ 4 ] = {
                                    { origin.m_x + lx, origin.m_y + ly, bottom_z },
                                    { origin.m_x + rx, origin.m_y + ry, bottom_z },
                                    { origin.m_x + rx, origin.m_y + ry, top_z    },
                                    { origin.m_x + lx, origin.m_y + ly, top_z    },
                                };

                                game::fvector2d sc[ 4 ];
                                bool valid = true;
                                for ( int i = 0; i < 4; i++ ) {
                                    if ( !game::world_to_screen( corners_3d[ i ], &sc[ i ] ) ) {
                                        valid = false;
                                        break;
                                    }
                                }

                                if ( valid ) {
                                    auto draw_edge = [ & ] ( int a, int b ) {
                                        g_interface->draw_line(
                                            ( float )sc[ a ].m_x, ( float )sc[ a ].m_y,
                                            ( float )sc[ b ].m_x, ( float )sc[ b ].m_y,
                                            thickness, main_col
                                        );
                                        };

                                    draw_edge( 0, 1 ); // bottom
                                    draw_edge( 2, 3 ); // top
                                    draw_edge( 0, 3 ); // left
                                    draw_edge( 1, 2 ); // right
                                }
                            }


                            if ( settings::visuals::snaplines ) {
                                g_interface->draw_line(
                                    center_width,
                                    static_cast< float >( g_interface->get_screen_height( ) ),
                                    root_position.m_x, root_position.m_y,
                                    1.f, peach::color_t( main_col.r, main_col.g, main_col.b, 120 ) );
                            }

                            if ( settings::visuals::show_platform ) {
                                auto platform_str = player_state->get_platform( );
                                auto* platform = platform_str ? platform_str.c_str( ) : nullptr;
                                if ( platform ) {
                                    const auto get_platform_color = [ & ] ( ) {
                                        if ( _wcs_contains( platform, ( L"WIN" ) ) )                                    return peach::color_t( 125, 165, 170, 255 );
                                        if ( _wcs_contains( platform, ( L"XBL" ) ) || _wcs_contains( platform, ( L"XSX" ) ) ) return peach::color_t( 119, 187, 68, 255 );
                                        if ( _wcs_contains( platform, ( L"PSN" ) ) || _wcs_contains( platform, ( L"PS5" ) ) ) return peach::color_t( 46, 109, 180, 255 );
                                        if ( _wcs_contains( platform, ( L"SWT" ) ) )                                    return peach::color_t( 255, 69, 84, 255 );
                                        if ( _wcs_contains( platform, ( L"AND" ) ) )                                    return peach::color_t( 115, 135, 83, 255 );
                                        if ( _wcs_contains( platform, ( L"IOS" ) ) )                                    return peach::color_t( 94, 105, 238, 255 );
                                        return peach::color_t( 125, 165, 170, 255 );
                                        };

                                    const auto get_platform_display = [ & ] ( ) -> const wchar_t* {
                                        if ( _wcs_contains( platform, ( L"WIN" ) ) )                                    return ( L"Windows" );
                                        if ( _wcs_contains( platform, ( L"XBL" ) ) || _wcs_contains( platform, ( L"XSX" ) ) ) return ( L"Xbox" );
                                        if ( _wcs_contains( platform, ( L"PSN" ) ) || _wcs_contains( platform, ( L"PS5" ) ) ) return ( L"PlayStation" );
                                        if ( _wcs_contains( platform, ( L"SWT" ) ) )                                    return ( L"Nintendo" );
                                        if ( _wcs_contains( platform, ( L"AND" ) ) || _wcs_contains( platform, ( L"IOS" ) ) ) return ( L"Mobile" );
                                        return platform;
                                        };

                                    draw_top_label( get_platform_display( ), get_platform_color( ) );
                                }
                            }

                            if ( settings::visuals::show_name ) {
                                const wchar_t* display_name = nullptr;
                                auto readable = game::kismet::m_fort_library->get_human_readable_name( player );
                                if ( settings::visuals::show_level ) {
                                    readable = game::kismet::m_string_library->build_string_int(
                                        readable, L" [", player_state->season_level_ui_display( ), L"]" );
                                }
                                display_name = readable ? readable.c_str( ) : nullptr;
                                if ( display_name )
                                    draw_top_label( display_name, fourth_col );
                            }

                            if ( settings::visuals::show_weapon ) {
                                if ( current_weapon && current_weapon->outer_private( ) &&
                                    current_weapon->object_is_a( game::classes::m_fort_weapon ) ) {
                                    auto weapon_data = current_weapon->weapon_data( );
                                    if ( weapon_data ) {
                                        auto w_display_name = weapon_data->item_name( ).get( );
                                        if ( w_display_name ) {
                                            auto w_rarity = weapon_data->rarity( );
                                            const auto get_rarity_col = [ w_rarity ] ( ) {
                                                switch ( w_rarity ) {
                                                    case game::e_fort_rarity::common:       return peach::color_t( 185, 185, 185, 255 );
                                                    case game::e_fort_rarity::uncommon:     return peach::color_t( 96, 170, 58, 255 );
                                                    case game::e_fort_rarity::rare:         return peach::color_t( 75, 128, 255, 255 );
                                                    case game::e_fort_rarity::epic:         return peach::color_t( 209, 110, 244, 255 );
                                                    case game::e_fort_rarity::legendary:    return peach::color_t( 255, 189, 62, 255 );
                                                    case game::e_fort_rarity::mythic:       return peach::color_t( 255, 215, 0, 255 );
                                                    case game::e_fort_rarity::transcendent: return peach::color_t( 255, 100, 100, 255 );
                                                    default:                                return peach::color_t( 185, 185, 185, 255 );
                                                }
                                                };

                                            auto ammo = current_weapon->ammo_count( );
                                            auto max_ammo = current_weapon->get_bullets_per_clip( );

                                            if ( ammo && max_ammo ) {
                                                w_display_name = game::kismet::m_string_library->build_string_int(
                                                    w_display_name, L" [", ammo, L"/" );
                                                w_display_name = game::kismet::m_string_library->build_string_int(
                                                    w_display_name, L"", max_ammo, L"]" );
                                            }

                                            draw_label( w_display_name.c_str( ), get_rarity_col( ) );
                                        }
                                    }
                                }
                            }

                            if ( settings::visuals::show_distance ) {
                                wchar_t distance_text[ 32 ];
                                swprintf_s( distance_text, L"[%.1fm]", world_dist );
                                draw_label( distance_text, fourth_col );
                            }

                            if ( settings::visuals::skeleton ) {
                                const auto draw_bone = [ & ] ( const wchar_t* a, const wchar_t* b ) {
                                    game::fvector2d pos_a, pos_b;
                                    if ( !game::world_to_screen( mesh->get_bone_location( a ), &pos_a ) ) return;
                                    if ( !game::world_to_screen( mesh->get_bone_location( b ), &pos_b ) ) return;
                                    g_interface->draw_line( pos_a.m_x, pos_a.m_y,
                                        pos_b.m_x, pos_b.m_y,
                                        settings::visuals::skeleton_thickness, fourth_col );
                                    };

                                draw_bone( L"neck_01", L"pelvis" );
                                draw_bone( L"neck_01", L"upperarm_l" );
                                draw_bone( L"upperarm_l", L"lowerarm_l" );
                                draw_bone( L"lowerarm_l", L"hand_l" );
                                draw_bone( L"neck_01", L"upperarm_r" );
                                draw_bone( L"upperarm_r", L"lowerarm_r" );
                                draw_bone( L"lowerarm_r", L"hand_r" );
                                draw_bone( L"pelvis", L"thigh_l" );
                                draw_bone( L"thigh_l", L"calf_l" );
                                draw_bone( L"calf_l", L"ik_foot_l" );
                                draw_bone( L"pelvis", L"thigh_r" );
                                draw_bone( L"thigh_r", L"calf_r" );
                                draw_bone( L"calf_r", L"ik_foot_r" );
                            }
                        }
                    }
                }
            }

        if ( settings::tracers::enabled ) {
            while ( m_tracer_cache.count > 0 ) {
                int tail = ( ( m_tracer_cache.head - m_tracer_cache.count + ( int )m_max_tracers ) % ( int )m_max_tracers );
                auto& oldest = m_tracer_cache.entries[ tail ];
                if ( ( now_time - oldest.m_creation_time ) > settings::tracers::max_age )
                    --m_tracer_cache.count;
                else
                    break;
            }

            for ( auto n = 0; n < m_tracer_cache.count; ++n ) {
                int  slot = ( ( m_tracer_cache.head - m_tracer_cache.count + n + ( int )m_max_tracers ) % ( int )m_max_tracers );
                auto& tracer = m_tracer_cache.entries[ slot ];

                auto age = now_time - tracer.m_creation_time;
                if ( age > settings::tracers::max_age ) continue;

                auto alpha = static_cast< uint8_t >( ( 1.0 - ( age / settings::tracers::max_age ) ) * 255.0 );
                if ( alpha == 0 ) continue;

                game::fvector2d start_screen, end_screen;
                if ( !game::world_to_screen( tracer.m_start, &start_screen ) ) continue;
                if ( !game::world_to_screen( tracer.m_end, &end_screen ) ) continue;

                peach::color_t line_col( 160, 136, 215, alpha );

                g_interface->draw_line(
                    start_screen.m_x, start_screen.m_y,
                    end_screen.m_x, end_screen.m_y,
                    settings::tracers::line_thickness, line_col );

                g_interface->draw_circle(
                    end_screen.m_x, end_screen.m_y,
                    settings::tracers::impact_dot_radius, line_col );
            }
        }

        hooks::damage::register_pellets = false;
        hooks::projectile::m_source_location = {};
        hooks::targeting::m_source_location = {};
        hooks::camera::m_source_location = {};

        if ( acknowledged_pawn ) {
            auto current_weapon = acknowledged_pawn->current_weapon( );
            if ( current_weapon &&
                ( settings::aimbot::weapon_only ? current_weapon->object_is_a( game::classes::m_fort_weapon ) : true ) ) {
                if ( settings::visuals::fov_circle ) {
                    g_interface->draw_circle_outline(
                        center_width, center_height, fov_radius,
                        settings::visuals::fov_circle_thickness,
                        peach::color_t( 94, 105, 238, 255 ) );
                }

                if ( candidate_count ) {
                    double total_dist = 0.0;
                    for ( int ci = 0; ci < candidate_count; ci++ )
                        total_dist += candidates[ ci ].world_distance;

                    double avg_dist = total_dist / static_cast< double >( candidate_count );
                    double dist_high = avg_dist * 1.75;

                    for ( int ci = 0; ci < candidate_count; ci++ ) {
                        auto c = candidates[ ci ];
                        if ( !c.pawn ) continue;
                        if ( c.world_distance > dist_high ) continue;

                        double crosshair_score = c.head_position.distance_to(
                            game::fvector2d( center_width, center_height ) );
                        double dist_factor = c.world_distance / dist_high;
                        double combined_score =
                            ( crosshair_score * settings::aimbot::crosshair_weight ) +
                            ( dist_factor * 400.0 * settings::aimbot::distance_weight );

                        if ( settings::aimbot::prefer_visible && c.is_visible )
                            combined_score *= 0.15;

                        if ( combined_score < best_target_score ) {
                            best_target_score = combined_score;
                            target_distance = c.world_distance;
                            target_pawn = c.pawn;
                        }
                    }

                    if ( target_pawn ) {
                        auto target_mesh = target_pawn->mesh( );
                        if ( target_mesh ) {
                            const wchar_t* aim_bone = oxorany( L"head" );
                            switch ( settings::aimbot::target_bone_mode ) {
                                case 0: { } break;
                                case 1: { aim_bone = oxorany( L"head" ); } break;
                                case 2: { aim_bone = oxorany( L"neck_01" ); } break;
                                case 3: { aim_bone = oxorany( L"spine_03" ); } break;
                            }

                            auto aim_location = target_mesh->get_bone_location( aim_bone );
                            if ( aim_location ) {
                                game::fvector2d aim_position;
                                if ( game::world_to_screen( aim_location, &aim_position ) ) {
                                    if ( g_interface->in_screen( aim_position ) ) {
                                        auto draw_muzzle_line = [ current_weapon, aim_position,
                                            target_distance, best_target_score ] ( ) {
                                            game::fvector2d muzzle_position;
                                            auto muzzle_location = current_weapon->get_muzzle_location( 0 );
                                            if ( game::world_to_screen( muzzle_location, &muzzle_position ) ) {
                                                g_interface->draw_line(
                                                    muzzle_position.m_x, muzzle_position.m_y,
                                                    aim_position.m_x, aim_position.m_y,
                                                    2.f, peach::color_t( 120, 160, 230, 255 ) );
                                                if ( settings::visuals::thread_highlight ) {
                                                    //g_interface->draw_circle(
                                                    //    aim_position.m_x, aim_position.m_y,
                                                    //    4.f, peach::color_t( 120, 160, 230, 255 ) );

                                                    const float min_size = 8.f, max_size = 28.f;
                                                    const float min_dist = 10.f, max_dist = 300.f;
                                                    float t = 1.f - static_cast< float >(
                                                        ( best_target_score > 0 ? target_distance : 50.0 ) - min_dist ) / ( max_dist - min_dist );
                                                    t = t < 0.f ? 0.f : t > 1.f ? 1.f : t;
                                                    const float icon_size = min_size + ( max_size - min_size ) * t;

                                                    const auto crosshair_icon = icons::to_utf8( 0xf05b );
                                                    const float icon_w = g_interface->measure_text( crosshair_icon.c_str( ), "Font Awesome", icon_size );

                                                    g_interface->draw_text(
                                                        crosshair_icon.c_str( ),
                                                        aim_position.m_x - icon_w * 0.5f,
                                                        aim_position.m_y - icon_size * 0.5f,
                                                        "Font Awesome", icon_size,
                                                        peach::color_t( 120, 160, 230, 220 ),
                                                        peach::color_t( 0, 0, 0, 0 ),
                                                        0.f
                                                    );
                                                }
                                            }
                                            };

                                        auto aim_key = VK_RBUTTON;
                                        switch ( settings::aimbot::aim_key ) {
                                            case 0: { aim_key = VK_RBUTTON; } break;
                                            case 1: { aim_key = VK_LBUTTON; } break;
                                            case 2: { aim_key = VK_LCONTROL; } break;
                                            case 3: { aim_key = VK_LSHIFT; } break;
                                        }

                                        if ( settings::aimbot::enabled && GetAsyncKeyState( aim_key ) & 0x8000 ) {
                                            draw_muzzle_line( );

                                            auto dx = aim_position.m_x - center_width;
                                            auto dy = aim_position.m_y - center_height;
                                            auto screen_dist = sqrt( dx * dx + dy * dy );

                                            if ( screen_dist > settings::aimbot::deadzone ) {
                                                auto aim_rotation = game::kismet::m_math_library->find_look_at_rotation(
                                                    game::m_camera_location, aim_location );
                                                aim_rotation -= game::m_camera_rotation;
                                                aim_rotation.normalize( );

                                                aim_rotation.m_pitch *= settings::aimbot::smooth_factor;
                                                aim_rotation.m_yaw *= settings::aimbot::smooth_factor;

                                                if ( aim_rotation )
                                                    player_controller->rotation_input( aim_rotation );
                                            }
                                        }

                                        if ( current_weapon && current_weapon->outer_private( ) &&
                                            current_weapon->object_is_a( game::classes::m_fort_weapon ) ) {

                                            auto weapon_data = current_weapon->weapon_data( );
                                            if ( weapon_data ) {
                                                draw_muzzle_line( );

                                                if ( settings::aimbot::multiple_damage ) {
                                                    if ( aim_location && GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) {
                                                        hooks::damage::register_pellets = true;
                                                        hooks::damage::install( current_weapon );
                                                    }
                                                }

                                                if ( settings::aimbot::bullet_tp ) {
                                                    if ( aim_location && GetAsyncKeyState( VK_LCONTROL ) & 0x8000 ) {
                                                        hooks::projectile::m_source_location = aim_location;
                                                        hooks::projectile::install( current_weapon );
                                                    }
                                                }

                                                if ( settings::aimbot::silent_aim ) {
                                                    if ( aim_location ) {
                                                        hooks::camera::m_source_location = aim_location;
                                                        hooks::camera::install( current_weapon );
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        return cleanup( );
    }
}