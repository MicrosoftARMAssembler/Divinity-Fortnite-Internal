#pragma once
#define declare_member(type, name, offset) type name() { auto* _ptr = (type*)( this + offset ); if (!utility::is_valid(_ptr)) return {}; return *_ptr; }
#define declare_member_bit(bit, name, offset) bool name( ) { auto* _ptr = (char*)( this + offset); if (!utility::is_valid(_ptr)) return false; return bool( *_ptr & (1 << bit)); }
#define apply_member(type, name, offset) void name( type val ) { auto* _ptr = (type*)( this + offset ); if (!utility::is_valid(_ptr)) return; *_ptr = val; }
#define apply_member_bit(bit, name, offset) void name(bool value) { auto* _ptr = (char*)(this + offset); if (!utility::is_valid(_ptr)) return; *_ptr |= (value << bit); }
#define invert_member_bit(bit, name, offset) void name(bool value) { auto* _ptr = (char*)(this + offset); if (!utility::is_valid(_ptr)) return; *_ptr &= ~(value << bit); }

namespace game {
	fvector m_camera_location;
	frotator m_camera_rotation;
	float m_field_of_view;

	class u_object {
	public:
		declare_member( std::uintptr_t, vtable, offsets::vtable );
		declare_member( e_object_flags, object_flags, offsets::object_flags );
		declare_member( std::uint32_t, internal_index, offsets::internal_index );
		declare_member( u_object*, class_private, offsets::class_private );
		declare_member( fname, name_private, offsets::name_private );
		declare_member( u_object*, outer_private, offsets::outer_private );

		void process_event( u_function* function, void* args );
		fstring get_object_name( );
		bool object_is_a( u_class* object_class );
		static u_object* static_find_object( u_object* klass, u_object* outer, std::wstring_view name, bool exact );

		template<typename T>
		static inline T find_object( const wchar_t* name, u_object* outer = nullptr, bool exact = false ) {
			if ( !name ) return nullptr;
			std::wstring_view wname{ name, std::wcslen( name ) };
			return reinterpret_cast< T >( static_find_object( nullptr, outer, wname, exact ) );
		}

		template<typename type>
		type* load_object( const wchar_t* command, a_player_controller* player_controller );

		template<typename T>
		tarray<T> get_actors_of_class( u_class* object_class );
	};

	class u_class : public u_object {
	public:
	};

	class u_actor_component : public u_object {
	public:
		void set_is_replicated( bool should_replicate );
	};

	class u_movement_component : public u_actor_component { };
	class u_nav_covement_component : public u_movement_component { };
	class u_pawn_movement_component : public u_nav_covement_component { };
	class u_character_movement_component : public u_pawn_movement_component { };

	class u_material_interface : public u_object { };

	class u_material : public u_material_interface {
	public:
		apply_member( e_blend_mode, blend_mode, offsets::blend_mode );
		apply_member_bit( 0, b_disable_depth_test, offsets::b_disable_depth_test );
		invert_member_bit( 0, reset_b_disable_depth_test, offsets::b_disable_depth_test );
		apply_member_bit( 7, wireframe, offsets::wireframe );
		invert_member_bit( 7, reset_wireframe, offsets::wireframe );
		apply_member_bit( 4, two_sided, offsets::two_sided );
		invert_member_bit( 4, reset_two_sided, offsets::two_sided );
		apply_member_bit( 4, b_use_emissive_for_dynamic_area_lighting, offsets::b_use_emissive_for_dynamic_area_lighting );
	};

	class u_material_instance : public u_material_interface {
	public:
		declare_member( u_material_interface*, parent, offsets::parent );
	};

	class u_material_instance_dynamic : public u_material_instance {
	public:
		void set_vector_parameter_value( fname parameter_name, flinear_color value ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object< u_function* >( oxorany( L"MaterialInstanceDynamic.SetVectorParameterValue" ) );
			struct { fname parameter_name; flinear_color value; } params{ parameter_name, value };
			this->process_event( function, &params );
		}

		void set_scalar_parameter_value( fname parameter_name, float value ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object< u_function* >( oxorany( L"MaterialInstanceDynamic.SetScalarParameterValue" ) );
			struct { fname parameter_name; float value; } params{ parameter_name, value };
			this->process_event( function, &params );
		}
	};

	class u_scene_component : public u_actor_component {
	public:
		fvector get_forward_vector( );
		fvector get_socket_location( fname in_socket_name );
		fvector get_bone_location( const wchar_t* socket_name );
	};

	class u_primitive_component : public u_scene_component {
	public:
		u_material_instance_dynamic* create_dynamic_material_instance( int32_t element_index, u_material_interface* parent, fname optional_name ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object<u_function*>( oxorany( L"PrimitiveComponent.CreateDynamicMaterialInstance" ) );
			struct {
				int32_t element_index;
				u_material_interface* parent;
				fname optional_name;
				u_material_instance_dynamic* return_value;
			} params{ element_index, parent, optional_name };
			this->process_event( function, &params );
			return params.return_value;
		}

		u_material_interface* get_material( int32_t element_index ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object<u_function*>( oxorany( L"PrimitiveComponent.GetMaterial" ) );
			struct { int32_t element_index; u_material_interface* return_value; } params{ element_index };
			this->process_event( function, &params );
			return params.return_value;
		}

		void set_material( int32_t element_index, u_material_interface* material ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object< u_function* >( oxorany( L"PrimitiveComponent.SetMaterial" ) );
			struct { int32_t element_index; u_material_interface* material; } params{ element_index, material };
			this->process_event( function, &params );
		}
	};

	class u_mesh_component : public u_primitive_component {
	public:
		tarray<u_material_interface*> get_materials( ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object< u_function* >( oxorany( L"MeshComponent.GetMaterials" ) );
			struct { tarray<u_material_interface*> return_value; } params;
			this->process_event( function, &params );
			return params.return_value;
		}
	};

	class u_base_building_static_mesh_component : public u_mesh_component { };
	class u_skinned_mesh_component : public u_mesh_component { };

	class u_skeletal_mesh_component : public u_skinned_mesh_component {
	public:
	};

	class u_skeletal_mesh_component_budgeted : public u_skeletal_mesh_component { };

	class a_actor : public u_object {
	public:
		declare_member( u_scene_component*, root_component, offsets::root_component );

		void set_actor_enable_collision( bool b_new_actor_enable_collision );
		float get_input_axis_key_value( fkey input_axis_key );
		frotator k2_get_actor_rotation( );
		fvector k2_get_actor_location( );
		float get_distance_to( a_actor other_actor );
		void force_net_update( );
		void flush_net_dormancy( );
		void prestream_textures( float seconds, bool b_enable_streaming, int32_t cinematic_texture_groups );
		fvector get_velocity( );
	};

	class a_building_sm_actor : public a_actor {
	public:
		declare_member( u_base_building_static_mesh_component*, static_mesh_component, 0x830 );
	};

	class a_building_container : public a_building_sm_actor {
	public:
		declare_member_bit( 3, b_already_searched, 0xD52 );
	};

	class u_item_definition_base : public u_object {
	public:
		declare_member( ftext, item_name, offsets::item_name );
	};

	class u_fort_item_definition : public u_item_definition_base {
	public:
		declare_member( e_fort_item_type, item_type, offsets::item_type );
		declare_member( e_fort_rarity, rarity, offsets::rarity );
	};

	class a_fort_pickup : public a_actor {
	public:
		declare_member( f_fort_item_entry, primary_pickup_item_entry, offsets::primary_pickup_item_entry );

		u_item_definition_base* get_primary_item_definition( ) {
			static u_function* function = nullptr;
			if ( !function )
				function = find_object< u_function* >( oxorany( L"FortPickup.GetPrimaryItemDefinition" ) );

			struct { u_item_definition_base* return_value; } params;
			this->process_event( function, &params );

			return params.return_value;
		}
	};

	class a_player_camera_manager : public a_actor { };

	class u_fort_world_item_definition : public u_fort_item_definition { };
	class u_fort_weapon_item_definition : public u_fort_world_item_definition { };
	class a_fort_ranged_weapon : public a_actor { };

	class a_fort_weapon : public a_fort_ranged_weapon {
	public:
		declare_member( u_fort_weapon_item_definition*, weapon_data, offsets::weapon_data );
		declare_member( tarray<u_mesh_component*>, all_weapon_meshes, offsets::all_weapon_meshes );
		declare_member( float, current_projected_impact_distance, 0x0FB8 );
		declare_member( int32_t, ammo_count, 0x10A4 );

		bool is_reloading( );
		bool is_firing( );
		bool can_fire( );
		fvector get_muzzle_location( int pattern_index );
		int32_t get_magazine_ammo_count( );
		int32_t get_bullets_per_clip( );
	};

	class a_pawn : public a_actor {
	public:
		declare_member( a_player_state*, player_state, offsets::player_state );
		declare_member( a_player_controller*, controller, offsets::controller );
		declare_member( a_player_controller*, previous_controller, offsets::previous_controller );
		declare_member( u_character_movement_component*, character_movement, offsets::character_movement );
	};

	class a_character : public a_pawn {
	public:
		declare_member( u_skeletal_mesh_component*, mesh, offsets::mesh );
	};

	class afgf_character : public a_character { };

	class a_fort_pawn : public afgf_character {
	public:
		declare_member( a_fort_weapon*, current_weapon, offsets::current_weapon );
		declare_member( float, total_player_damage_dealt, offsets::total_player_damage_dealt );
		declare_member( int32_t, pawn_unique_id, offsets::pawn_unique_id );
		declare_member_bit( 2, b_is_invulnerable, offsets::b_is_invulnerable );

		void pawn_start_fire( char fire_mode_num );
		void pawn_stop_fire( char fire_mode_num );
		bool is_dead( );
		bool is_dbno( );
	};

	class a_fort_player_pawn : public a_fort_pawn {
	public:
		bool is_in_vehicle( );
	};

	class a_fort_player_pawn_athena : public a_fort_player_pawn {
	public:
		declare_member( a_actor*, current_vehicle, offsets::current_vehicle );
		declare_member( tarray<u_skeletal_mesh_component*>, skeletal_meshes, offsets::skeletal_meshes );
		declare_member( fvector, last_fired_location, offsets::last_fired_location );
		declare_member( fvector, last_fired_direction, offsets::last_fired_direction );
		apply_member( frotator, player_aim_rotation, offsets::player_aim_rotation );
	};

	class u_fort_player_state_component_habanero : public u_object { };
	class a_fort_player_state : public u_object { };
	class a_fort_player_state_zone : public a_fort_player_state { };

	class a_fort_player_state_athena : public a_fort_player_state_zone {
	public:
		declare_member( int, season_level_ui_display, offsets::season_level_ui_display );
		declare_member( int, kill_score, offsets::kill_score );
		declare_member( int, team_score_placement, offsets::team_score_placement );
		declare_member( int, total_player_score, offsets::total_player_score );
	};

	class a_player_state : public a_fort_player_state_athena {
	public:
		declare_member( a_fort_player_pawn_athena*, pawn_private, offsets::pawn_private );
		declare_member( int32_t, player_id, offsets::player_id );

		bool is_a_bot( );

		fstring get_platform( ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"FortPlayerState.GetPlatform" ) );
			}

			struct {
				fstring return_value;
			} params{ };

			this->process_event( function, &params );

			return params.return_value;
		}
	};

	struct f_quick_bar {
		int32_t current_focused_slot;
		int32_t previous_focused_slot;
		int32_t secondary_focused_slot;
		bool has_been_focused_before;
		tarray<f_quick_bar_slot> slots;
		f_quick_bar_data data_definition;
		t_set<u_fort_world_item_definition*> equipped_item_definitions;
		tarray<int32_t> shared_visible_slot_indices_when_using_gamepad;
		tarray<int32_t> hidden_slot_indices;
	};

	class a_fort_quick_bars : public a_actor {
	public:
		declare_member( f_quick_bar, primary_quick_bar, offsets::primary_quick_bar );
		declare_member( f_quick_bar, secondary_quick_bar, offsets::secondary_quick_bar );
		declare_member( f_quick_bar, creative_quick_bar, offsets::creative_quick_bar );
	};

	class a_controller : public a_actor {
	public:
		declare_member( a_fort_player_pawn*, my_fort_pawn, 0xB78 );
		declare_member( a_fort_pawn*, cached_fort_pawn, 0xB80 );
	};

	class a_player_controller : public a_controller {
	public:
		declare_member( a_fort_player_pawn_athena*, acknowledged_pawn, offsets::acknowledged_pawn );
		declare_member( a_player_camera_manager*, player_camera_manager, offsets::player_camera_manager );
		declare_member( a_fort_quick_bars*, client_quick_bars, offsets::client_quick_bars );
		apply_member( frotator, rotation_input, 0x528 + sizeof( std::uint64_t ) );
		apply_member( frotator, player_aim_offset, 0x2BC8 );

		bool deproject_screen_position_to_world( fvector2d& screen_position, fvector* world_position, fvector* world_direction );
		fvector2d get_mouse_position( );
		bool is_input_key_down( fkey key );
		bool was_input_key_just_pressed( fkey key );
		bool was_input_key_just_released( fkey key );
	};

	class u_scene_view_state : public u_object {
	public:
		declare_member( f_matrix, projection, offsets::projection );
		declare_member( double, field_of_view, offsets::field_of_view );
		bool update_camera( );
	};

	class u_player : public u_object {
	public:
		declare_member( a_player_controller*, player_controller, offsets::player_controller );
	};

	class u_localplayer : public u_player {
	public:
		declare_member( u_game_viewport_client*, viewport_client, offsets::viewport_client );
		declare_member( tarray<u_scene_view_state*>, view_state, offsets::view_state );

		u_scene_view_state* get_view_state( ) {
			return view_state( )[ 1 ];
		}
	};

	class u_game_instance : public u_object {
	public:
		declare_member( tarray<u_localplayer*>, localplayers, offsets::localplayers );

		u_localplayer* get_localplayer( ) {
			return localplayers( )[ 0 ];
		}
	};

	class a_game_state_base : public u_object {
	public:
		declare_member( float, server_world_time, offsets::server_world_time );
		declare_member( tarray<a_player_state*>, player_array, offsets::player_array );

		bool is_in_lobby( ) {
			return server_world_time( ) ? false : true;
		}
	};

	class u_level : public u_object {
	public:
		declare_member( tarray<a_actor*>, actors, offsets::actors );
	};

	class u_world : public u_object {
	public:
		declare_member( a_game_state_base*, game_state, offsets::game_state );
		declare_member( u_game_instance*, game_instance, offsets::game_instance );
		declare_member( tarray<u_level*>, levels, offsets::levels );
	};

	class u_game_viewport_client : public u_object {
	public:
		declare_member( u_world*, world, offsets::world );
		apply_member( std::uint32_t, b_is_play_in_editor_viewport, offsets::b_is_play_in_editor_viewport );
		apply_member( std::uint32_t, b_disable_world_rendering, offsets::b_disable_world_rendering );

		bool line_trace( fvector start, fvector end, f_hit_result* out_hit );
		bool line_trace_single( u_world* world_context_object,
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
			f_hit_result* out_hit );
	};

	class u_engine : public u_object {
	public:
		declare_member( u_game_viewport_client*, game_viewport, offsets::game_viewport );

		static u_engine* get_engine( ) {
			return *reinterpret_cast< u_engine** >( module::m_module_base + offsets::g_engine );
		}
	};

}