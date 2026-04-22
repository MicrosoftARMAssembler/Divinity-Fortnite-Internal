#pragma once
#include <utility>

namespace game {
	class u_object;
	class u_game_viewport_client;
	class u_particle_system;
	class u_particle_system_component;
	class u_sound_base;
	class u_sound_attenuation;
	class u_sound_concurrency;
	class u_initial_active_sound_params;
	class u_blueprint_function_library;
	class u_font;
	class u_class;
	class u_engine;
	class u_field;
	class u_struct;
	class u_function;
	class u_world;
	class u_scene_component;
	class a_actor;
	class a_fort_player_state_athena;
	class u_material_interface;
	class u_material;
	class a_fort_player_state_zone;
	class u_fort_world_item;
	class u_fort_world_item_definition;
	class a_controller;
	class a_player_controller;
	class a_player_state;
	class u_kismet_math_library;
	class u_kismet_string_library;
	class u_kismet_system_library;
	class u_fort_kismet_library;
	class u_gameplay_statics;
	class u_mesh_component;
	class u_material_instance_dynamic;
	class a_fort_game_state_athena;
	struct fvector;
	struct frotator;
	struct fstring;
	struct fkey;
	template<typename T> struct tarray;

	frotator normalize_euler_angles( frotator& result );
	float arctan_reciprocal_to_degrees( double half_tangent );

	namespace functions {
		std::uint64_t raycast_multiple = 0;
		std::uint64_t static_find_object = 0;
		std::uint64_t process_event = 0;
	}

	namespace vtable {
		constexpr std::uint32_t camera_targeting_source_location = 0xB20 / 8;
		constexpr std::uint32_t weapon_targeting_source_location = 0xB50 / 8;

		// Weapon/fire pipeline
		constexpr std::uint32_t weapon_get_definition = 0x830 / 8;               // +2096
		constexpr std::uint32_t get_projectile_spawn_location = 0x838 / 8;        // +2104
		constexpr std::uint32_t fire_source_provider = 0x260 / 8;                  // +608
		constexpr std::uint32_t fire_source_fallback = 0x7A0 / 8;                  // +1952
		constexpr std::uint32_t client_fire_get_range_scalar = 0xA90 / 8;          // +2704
		constexpr std::uint32_t client_fire_is_secondary_mode = 0xA98 / 8;         // +2712
		constexpr std::uint32_t client_fire_prepare_state = 0xB38 / 8;             // +2872
		constexpr std::uint32_t client_fire_build_context = 0xB48 / 8;             // +2888
		constexpr std::uint32_t client_fire_generate_primary = 0xB50 / 8;          // +2896
		constexpr std::uint32_t client_fire_fallback = 0xCC0 / 8;                  // +3264
		constexpr std::uint32_t client_fire_generate_secondary = 0xEB8 / 8;        // +3768
		constexpr std::uint32_t client_fire_post_process = 0xEC8 / 8;              // +3784
		constexpr std::uint32_t client_fire_dispatch_batch = 0xEE0 / 8;            // +3808
		constexpr std::uint32_t client_fire_dispatch_single = 0xEE8 / 8;           // +3816

		// Existing hook point used by damage namespace
		constexpr std::uint32_t register_client_fire = 0xEB0 / 8;
	}

	namespace offsets {
		// Global
		constexpr std::uint32_t g_engine = 0x194034B8;

		// UObject (functions)
		constexpr std::uint32_t static_find_object = 0x156B73C;
		constexpr std::uint32_t process_event = 0x1C8;

		// UObject (members)
		constexpr std::uint32_t vtable = 0x0;
		constexpr std::uint32_t object_flags = 0x14;
		constexpr std::uint32_t internal_index = 0x10;
		constexpr std::uint32_t class_private = 0x20;
		constexpr std::uint32_t name_private = 0x8;
		constexpr std::uint32_t outer_private = 0x18;

		// AActor
		constexpr std::uint32_t root_component = 0x1B0;

		// APawn
		constexpr std::uint32_t player_state = 0x2D0;
		constexpr std::uint32_t controller = 0x2e0;
		constexpr std::uint32_t previous_controller = 0x2e8;

		// ACharacter
		constexpr std::uint32_t mesh = 0x330;
		constexpr std::uint32_t character_movement = 0x338;

		// AFortPawn
		constexpr std::uint32_t current_weapon = 0x990;
		constexpr std::uint32_t total_player_damage_dealt = 0x149c;
		constexpr std::uint32_t last_hit_time = 0x1498;
		constexpr std::uint32_t damage_taken_last_at_time = 0x1494;
		constexpr std::uint32_t damage_taken_done_at_time = 0x1490;
		constexpr std::uint32_t pawn_unique_id = 0x98c;
		constexpr std::uint32_t b_is_invulnerable = 0x72a;

		// AFortPlayerPawnAthena
		constexpr std::uint32_t current_vehicle = 0x2cf0;
		constexpr std::uint32_t skeletal_meshes = 0x6250;
		constexpr std::uint32_t last_fired_location = 0x5b50;
		constexpr std::uint32_t last_fired_direction = 0x5b68;
		constexpr std::uint32_t player_aim_rotation = 0x69C8;

		// APlayerController
		constexpr std::uint32_t acknowledged_pawn = 0x358;
		constexpr std::uint32_t player_camera_manager = 0x368;
		constexpr std::uint32_t client_quick_bars = 0x1AD8;

		// USceneViewState
		constexpr std::uint32_t projection = 0x940;
		constexpr std::uint32_t field_of_view = 0x740;

		// UPlayer
		constexpr std::uint32_t player_controller = 0x30;

		// ULocalPlayer
		constexpr std::uint32_t viewport_client = 0x78;
		constexpr std::uint32_t view_state = 0xD0;

		// UGameInstance
		constexpr std::uint32_t localplayers = 0x38;

		// AGameStateBase
		constexpr std::uint32_t server_world_time = 0x2e8;
		constexpr std::uint32_t player_array = 0x2c8;

		// UGameViewportClient
		constexpr std::uint32_t world = 0x78;
		constexpr std::uint32_t b_is_play_in_editor_viewport = 0xA0;
		constexpr std::uint32_t b_disable_world_rendering = 0xB0;

		// UEngine
		constexpr std::uint32_t game_viewport = 0xae0;

		// ULevel
		constexpr std::uint32_t actors = 0xa8;

		// UWorld
		constexpr std::uint32_t camera_location_ptr = 0x180;
		constexpr std::uint32_t camera_rotation_ptr = 0x190;
		constexpr std::uint32_t game_state = 0x1d8;
		constexpr std::uint32_t game_instance = 0x250;
		constexpr std::uint32_t levels = 0x1F0;

		// APlayerState
		constexpr std::uint32_t pawn_private = 0x328;
		constexpr std::uint32_t player_id = 0x2b4;

		// AFortPlayerStateAthena
		constexpr std::uint32_t season_level_ui_display = 0x11dc;
		constexpr std::uint32_t kill_score = 0x11d8;
		constexpr std::uint32_t team_score_placement = 0x11b4;
		constexpr std::uint32_t total_player_score = 0x1264;

		// AFortPickup
		constexpr std::uint32_t primary_pickup_item_entry = 0x3a8;

		// AFortWeapon
		constexpr std::uint32_t weapon_data = 0x678;
		constexpr std::uint32_t all_weapon_meshes = 0x1080;

		// UItemDefinitionBase
		constexpr std::uint32_t item_name = 0x38;

		// UFortItemDefinition
		constexpr std::uint32_t item_type = 0xA8;
		constexpr std::uint32_t rarity = 0xAA;

		// UMaterialInstance
		constexpr std::uint32_t parent = 0x130;

		// USkeletalMeshComponent
		constexpr std::uint32_t get_bounds = 0x100;

		// AFortQuickBars
		constexpr std::uint32_t primary_quick_bar = 0x2B8;
		constexpr std::uint32_t secondary_quick_bar = 0x350;
		constexpr std::uint32_t creative_quick_bar = 0x3F0;

		// UMaterial
		constexpr std::uint32_t blend_mode = 0x149;
		constexpr std::uint32_t b_disable_depth_test = 0x1D0;
		constexpr std::uint32_t wireframe = 0x1E8;
		constexpr std::uint32_t two_sided = 0x198;
		constexpr std::uint32_t b_use_emissive_for_dynamic_area_lighting = 0x1D0;
	}

	namespace kismet {
		u_kismet_math_library* m_math_library;
		u_kismet_string_library* m_string_library;
		u_kismet_system_library* m_system_library;
		u_fort_kismet_library* m_fort_library;
		u_gameplay_statics* m_gameplay_statics;
	}

	namespace classes {
		u_class* m_fort_weapon;
		u_class* m_fort_pickup;
		u_class* m_container;
		u_class* m_weakspot;
		u_class* m_vehicle;
		u_class* m_material_instance;
		u_class* m_skeletal_mesh_component;
		u_class* m_material_instance_dynamic;
		u_class* m_material_interface;
		u_class* m_building;
		u_class* m_item_definition;
		u_class* m_player_controller;
		u_class* m_fort_player_pawn_athena;
	}

	namespace material {
		u_material* m_rez_in_master;
		u_material* m_ribbon_master;
	}

	struct bone_pair_t {
		const wchar_t* first;
		const wchar_t* second;
	};
}
