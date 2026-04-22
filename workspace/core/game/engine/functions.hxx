#pragma once

namespace game {
	namespace keys {
		fkey m_left_mouse;
		fkey m_right_mouse;
		fkey m_insert;
		fkey m_left_shift;
		fkey m_left_alt;
		fkey m_thumb_mouse_button;
		fkey m_thumb_mouse_button2;
		fkey m_gamepad_left_trigger;
		fkey m_w;
		fkey m_a;
		fkey m_s;
		fkey m_d;
		fkey m_spacebar;
		fkey m_mouse_scroll_up;
		fkey m_mouse_scroll_down;
		fkey m_mouse_wheel_axis;
		fkey m_f7;
		fkey m_f8;
	}

	frotator fvector::to_rotator( ) {
		return kismet::m_math_library->conv_vector_to_rotator( *this );
	}

	fmatrix get_rotation_matrix( const frotator& rotation ) {
		fmatrix matrix = {};

		const double pitch = rotation.m_pitch * std::numbers::pi / 180.0;
		const double yaw = rotation.m_yaw * std::numbers::pi / 180.0;
		const double roll = rotation.m_roll * std::numbers::pi / 180.0;

		const double sp = kismet::m_math_library->sin( pitch );
		const double cp = kismet::m_math_library->cos( pitch );
		const double sy = kismet::m_math_library->sin( yaw );
		const double cy = kismet::m_math_library->cos( yaw );
		const double sr = kismet::m_math_library->sin( roll );
		const double cr = kismet::m_math_library->cos( roll );

		matrix.m_x_plane.m_x = cp * cy;
		matrix.m_x_plane.m_y = cp * sy;
		matrix.m_x_plane.m_z = sp;

		matrix.m_y_plane.m_x = sr * sp * cy - cr * sy;
		matrix.m_y_plane.m_y = sr * sp * sy + cr * cy;
		matrix.m_y_plane.m_z = -sr * cp;

		matrix.m_z_plane.m_x = -( cr * sp * cy + sr * sy );
		matrix.m_z_plane.m_y = cy * sr - cr * sp * sy;
		matrix.m_z_plane.m_z = cr * cp;

		matrix.m_w_plane.m_w = 1.0;

		return matrix;
	}

	bool world_to_screen( fvector world_location, fvector2d* screen_position ) {
		auto matrix = get_rotation_matrix( m_camera_rotation );

		fvector x_axis = { matrix.m_x_plane.m_x, matrix.m_x_plane.m_y, matrix.m_x_plane.m_z };
		fvector y_axis = { matrix.m_y_plane.m_x, matrix.m_y_plane.m_y, matrix.m_y_plane.m_z };
		fvector z_axis = { matrix.m_z_plane.m_x, matrix.m_z_plane.m_y, matrix.m_z_plane.m_z };

		auto delta = world_location - m_camera_location;

		fvector transform = {
			delta.dot( y_axis ),
			delta.dot( z_axis ),
			delta.dot( x_axis )
		};

		if ( transform.m_z < 0.01 )
			transform.m_z = 0.01;

		auto width_center = g_interface->get_screen_width( ) / 2;
		auto height_center = g_interface->get_screen_height( ) / 2;

		double fov_multiplier = width_center / kismet::m_math_library->tan( m_field_of_view * std::numbers::pi / 360.0 );
		screen_position->m_x = width_center + transform.m_x * fov_multiplier / transform.m_z;
		screen_position->m_y = height_center - transform.m_y * fov_multiplier / transform.m_z;
		return transform.m_z > 0.01;
	}

	frotator normalize_euler_angles( frotator& result ) {
		result.m_yaw = ( kismet::m_math_library->atan2( result.m_pitch * -1.0, result.m_yaw ) * ( 180.0 / peach::k_pi ) ) * -1.0;
		result.m_yaw *= -1.0;
		result.m_pitch = kismet::m_math_library->asin( result.m_roll ) * ( 180.0 / peach::k_pi );
		result.m_roll = 0.0;
		return result;
	}

	float arctan_reciprocal_to_degrees( double half_tangent ) {
		auto result = kismet::m_math_library->atan( 1.0 / half_tangent ) * 2.f;
		result *= ( 180.f / peach::k_pi );
		return result;
	}

	void u_object::process_event( u_function* function, void* args ) {
		if ( !this || !function )
			return;

		if ( auto vtable = *( void*** )this ) {
			using process_event_t = void( __fastcall* )( u_object*, u_function*, void* );
			auto fn = reinterpret_cast< process_event_t >( vtable[ offsets::process_event ] );
			fn( this, function, args );
		}
	}

	u_object* u_object::static_find_object( u_object* klass, u_object* outer, std::wstring_view name, bool exact ) {
		using static_find_object_t = u_object * ( __cdecl* )( u_object*, u_object*, void*, bool );
		auto fn = reinterpret_cast< static_find_object_t >( functions::static_find_object );
		return fn( klass, outer, &name, exact );
	}

	fstring u_object::get_object_name( ) {
		return kismet::m_system_library->get_object_name( this );
	}

	bool u_object::object_is_a( u_class* object_class ) {
		return kismet::m_gameplay_statics->object_is_a( this, object_class );
	}

	template <class t>
	tarray<t> u_object::get_actors_of_class( u_class* object_class ) {
		return kismet::m_gameplay_statics->get_all_actors_of_class<t>( this, object_class );
	}

	void u_actor_component::set_is_replicated( bool should_replicate ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"ActorComponent.SetIsReplicated" ) );

		struct { bool should_replicate; } params{ should_replicate };
		this->process_event( function, &params );
	}

	fvector u_scene_component::get_forward_vector( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"SceneComponent.GetForwardVector" ) );

		struct { fvector return_value; } params{};
		this->process_event( function, &params );
		return params.return_value;
	}

	fvector u_scene_component::get_socket_location( fname in_socket_name ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"SceneComponent.GetSocketLocation" ) );

		struct { fname in_socket_name; fvector return_value; } params{ in_socket_name };
		this->process_event( function, &params );
		return params.return_value;
	}

	fvector u_scene_component::get_bone_location( const wchar_t* socket_name ) {
		if ( !utility::is_valid( this ) )      return fvector{};
		if ( !game::kismet::m_string_library ) return fvector{};

		struct cache_entry_t {
			wchar_t  key[ 64 ];
			fname    name;
			bool     used = false;
		};

		static constexpr uint32_t k_cache_size = 128; // must be power of 2
		static cache_entry_t      s_cache[ k_cache_size ]{};

		// FNV-1a
		auto fnv1a = [ ] ( const wchar_t* s ) -> uint32_t {
			uint32_t h = 2166136261u;
			while ( *s ) {
				h ^= ( uint8_t )( *s & 0xFF );        h *= 16777619u;
				h ^= ( uint8_t )( *s >> 8 & 0xFF );    h *= 16777619u;
				++s;
			}
			return h;
			};

		const uint32_t hash = fnv1a( socket_name );
		const uint32_t mask = k_cache_size - 1;
		uint32_t       idx = hash & mask;

		// linear probe — find hit or first free slot
		cache_entry_t* free_slot = nullptr;
		for ( uint32_t i = 0; i < k_cache_size; ++i, idx = ( idx + 1 ) & mask ) {
			auto& e = s_cache[ idx ];

			if ( !e.used ) {
				if ( !free_slot ) free_slot = &e;
				break; // open slot = key definitely not in table
			}

			if ( wcscmp( e.key, socket_name ) == 0 )
				return get_socket_location( e.name ); // cache hit
		}

		// resolve
		fname fn = kismet::m_string_library->conv_string_to_name( socket_name );
		if ( fn.m_index == 0 )
			return fvector{};

		// insert
		if ( free_slot ) {
			wcsncpy( free_slot->key, socket_name, 63 );
			free_slot->key[ 63 ] = L'\0';
			free_slot->name = fn;
			free_slot->used = true;
		}

		return get_socket_location( fn );
	}

	double fvector::distance_to( const fvector& other ) {
		return kismet::m_math_library->vector_distance( other, *this ) * 0.01;
	}

	double fvector2d::distance_to( const fvector2d& other ) {
		return kismet::m_math_library->vector_distance(
			game::fvector( other.m_x, other.m_y, 0.0 ),
			game::fvector( this->m_x, this->m_y, 0.0 )
		) * 0.01;
	}

	fvector frotator::get_forward_vector( ) {
		return kismet::m_math_library->get_forward_vector( *this );
	}

	void a_actor::set_actor_enable_collision( bool b_new_actor_enable_collision ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.SetActorEnableCollision" ) );

		struct { bool b_new_actor_enable_collision; } params{ b_new_actor_enable_collision };
		this->process_event( function, &params );
	}

	float a_actor::get_input_axis_key_value( fkey input_axis_key ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.GetInputAxisKeyValue" ) );

		struct { fkey input_axis_key; float return_value; } params{ input_axis_key };
		this->process_event( function, &params );
		return params.return_value;
	}

	frotator a_actor::k2_get_actor_rotation( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.K2_GetActorRotation" ) );

		struct { frotator return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	fvector a_actor::k2_get_actor_location( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.K2_GetActorLocation" ) );

		struct { fvector return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	void a_actor::force_net_update( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.ForceNetUpdate" ) );

		struct { } params;
		this->process_event( function, &params );
	}

	void a_actor::flush_net_dormancy( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.FlushNetDormancy" ) );

		struct { } params;
		this->process_event( function, &params );
	}

	void a_actor::prestream_textures( float seconds, bool b_enable_streaming, int32_t cinematic_texture_groups ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.PrestreamTextures" ) );

		struct { float seconds; bool b_enable_streaming; int32_t cinematic_texture_groups; } params;
		this->process_event( function, &params );
	}

	fvector a_actor::get_velocity( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"Actor.GetVelocity" ) );

		struct { fvector return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_fort_weapon::is_reloading( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.IsReloading" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_fort_weapon::is_firing( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.IsFiring" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_fort_weapon::can_fire( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.CanFire" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	fvector a_fort_weapon::get_muzzle_location( int pattern_index ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.GetMuzzleLocation" ) );

		struct { int pattern_index; fvector return_value; } params{ pattern_index };
		this->process_event( function, &params );
		return params.return_value;
	}

	int32_t a_fort_weapon::get_magazine_ammo_count( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.GetMagazineAmmoCount" ) );

		struct { int32_t return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	int32_t a_fort_weapon::get_bullets_per_clip( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortWeapon.GetBulletsPerClip" ) );

		struct { int32_t return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	void a_fort_pawn::pawn_start_fire( char fire_mode_num ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortPawn.PawnStartFire" ) );

		struct { char fire_mode_num; } params{ fire_mode_num };
		this->process_event( function, &params );
	}

	void a_fort_pawn::pawn_stop_fire( char fire_mode_num ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortPawn.PawnStopFire" ) );

		struct { char fire_mode_num; } params{ fire_mode_num };
		this->process_event( function, &params );
	}

	bool a_fort_pawn::is_dead( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortPawn.IsDead" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_fort_pawn::is_dbno( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortPawn.IsDBNO" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_fort_player_pawn::is_in_vehicle( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"FortPlayerPawn.IsInVehicle" ) );

		struct { bool return_value; } params;
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_player_state::is_a_bot( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"PlayerState.IsABot" ) );

		struct { bool return_value; } params{};
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_player_controller::deproject_screen_position_to_world( fvector2d& screen_position, fvector* world_position, fvector* world_direction ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object<u_function*>( oxorany( L"PlayerController.DeprojectScreenPositionToWorld" ) );

		struct {
			float   screen_x;
			float   screen_y;
			fvector world_position;
			fvector world_direction;
			bool    return_value;
		} params{ screen_position.m_x, screen_position.m_y };

		this->process_event( function, &params );

		if ( params.return_value ) {
			*world_position = params.world_position;
			*world_direction = params.world_direction;
		}

		return params.return_value;
	}

	fvector2d a_player_controller::get_mouse_position( ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"PlayerController.GetMousePosition" ) );

		struct { float location_x; float location_y; bool return_value; } params;
		this->process_event( function, &params );
		return fvector2d( double( params.location_x ), double( params.location_y ) );
	}

	bool a_player_controller::is_input_key_down( fkey key ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"PlayerController.IsInputKeyDown" ) );

		struct { fkey key; bool return_value; } params{ key };
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_player_controller::was_input_key_just_pressed( fkey key ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"PlayerController.WasInputKeyJustPressed" ) );

		struct { fkey key; bool return_value; } params{ key };
		this->process_event( function, &params );
		return params.return_value;
	}

	bool a_player_controller::was_input_key_just_released( fkey key ) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object< u_function* >( oxorany( L"PlayerController.WasInputKeyJustReleased" ) );

		struct { fkey key; bool return_value; } params{ key };
		this->process_event( function, &params );
		return params.return_value;
	}

	bool u_scene_view_state::update_camera( ) {
		if ( !this || !utility::is_valid( this ) )
			return false;

		auto projection = this->projection( );
		if ( std::isnan( projection.m[ 3 ][ 0 ] ) || std::isinf( projection.m[ 3 ][ 0 ] ) )
			return false;

		frotator camera_rotation;
		camera_rotation.m_pitch = utility::to_deg( std::asin( projection.z_plane.m_w ) );
		camera_rotation.m_yaw = utility::to_deg( std::atan2( projection.y_plane.m_w, projection.x_plane.m_w ) );
		camera_rotation.m_roll = 0.0;
		m_camera_rotation = camera_rotation;

		fvector camera_location;
		camera_location.m_x = projection.m[ 3 ][ 0 ];
		camera_location.m_y = projection.m[ 3 ][ 1 ];
		camera_location.m_z = projection.m[ 3 ][ 2 ];
		m_camera_location = camera_location;

		auto fov = atanf( 1 / this->field_of_view( ) ) * 2;
		m_field_of_view = utility::to_deg( fov );
		return true;
	}

	bool u_game_viewport_client::line_trace( fvector start, fvector end, f_hit_result* out_hit ) {
		return line_trace_single(
			this->world( ),
			start,
			end,
			trace_type_query::trace_type_query_1,
			false,
			tarray<uint64_t>( ),
			draw_debug_trace::for_duration,
			false,
			flinear_color( ),
			flinear_color( ),
			0.f,
			out_hit
		);
	}

	bool u_game_viewport_client::line_trace_single(
		u_world* world_context_object,
		const fvector& start,
		const fvector& end,
		t_enum_ss_byte<trace_type_query> trace_channel,
		bool trace_complex,
		const tarray<uint64_t>& actors_to_ignore,
		t_enum_ss_byte<draw_debug_trace> draw_debug_type,
		bool ignore_self,
		const flinear_color& trace_color,
		const flinear_color& trace_hit_color,
		float draw_time,
		f_hit_result* out_hit
	) {
		static u_function* function = nullptr;
		if ( !function )
			function = find_object<u_function*>( oxorany( L"KismetSystemLibrary.LineTraceSingle" ) );

		struct {
			u_world* world_context_object;
			fvector                           start;
			fvector                           end;
			t_enum_ss_byte<trace_type_query>  trace_channel;
			bool                              trace_complex;
			tarray<uint64_t>                  actors_to_ignore;
			t_enum_ss_byte<draw_debug_trace>  draw_debug_type;
			f_hit_result                      out_hit;
			bool                              ignore_self;
			flinear_color                     trace_color;
			flinear_color                     trace_hit_color;
			float                             draw_time;
			unsigned char                     padding[ 0x8 ];
			bool                              return_value;
		} params;

		params.world_context_object = world_context_object;
		params.start = start;
		params.end = end;
		params.trace_channel = trace_channel;
		params.trace_complex = trace_complex;
		params.actors_to_ignore = actors_to_ignore;
		params.draw_debug_type = draw_debug_type;
		params.ignore_self = ignore_self;
		params.trace_color = trace_color;
		params.trace_hit_color = trace_hit_color;
		params.draw_time = draw_time;

		this->process_event( function, &params );

		if ( out_hit != nullptr )
			*out_hit = params.out_hit;

		return params.return_value;
	}
}