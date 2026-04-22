#pragma once
#include <dxgidebug.h>
#pragma comment( lib, "dxgi.lib" )
#pragma comment( lib, "d3d12.lib" )
#pragma comment( lib, "d3d11.lib" )

namespace dll {
	bool initialize( ) {
		log_info(
			oxorany( "Dependency injected successfully.\n\n"
				"Initialization may cause instability if issues occur - \n"
				"Report persistent issues to our server via a support ticket.\n\n"
				"Version: ++Fortnite+Release-39.50-CL-50854790-Windows\n" ),
			oxorany( "Initializing" ), MB_ICONWARNING
		);

		module::m_peb = reinterpret_cast< peb_t* >( __readgsqword( 0x60 ) );
		module::m_module_base = module::get_main_module( );
		module::m_module_size = module::get_module_size( module::m_module_base );

		game::functions::static_find_object = module::find_pattern(
			module::m_module_base, module::m_module_size,
			oxorany( "41 57 41 56 41 55 41 54 56 57 55 53 48 81 EC ? ? ? ? 44 89 CE 48 89 CF 48 8B 05 ? ? ? ? 48 31 E0 48 89 84 24 ? ? ? ? 48 89 54 24" ) 
		);
		if ( !game::functions::static_find_object ) {
			log_info(
				oxorany( "Could not locate Static Find Object.\n\n"
					"Dependency may not be update to date\n"
					"Restart and wait the in-game overlay to appear before injecting." ),
				oxorany( "Static Find Object not found" ), MB_ICONHAND
			);
			return false;
		}

		log_info( oxorany( "Located Static Find Object.\n\n" ) );

		crt::init( );
		game::kismet::m_math_library = game::u_object::find_object<game::u_kismet_math_library*>( L"Engine.Default__KismetMathLibrary" );
		game::kismet::m_string_library = game::u_object::find_object<game::u_kismet_string_library*>( L"Engine.Default__KismetStringLibrary" );
		game::kismet::m_system_library = game::u_object::find_object<game::u_kismet_system_library*>( L"Engine.Default__KismetSystemLibrary" );
		game::kismet::m_fort_library = game::u_object::find_object<game::u_fort_kismet_library*>( L"FortniteGame.Default__FortKismetLibrary" );
		game::kismet::m_gameplay_statics = game::u_object::find_object<game::u_gameplay_statics*>( L"Engine.Default__GameplayStatics" );

		game::classes::m_fort_weapon = game::u_object::find_object<game::u_class*>( L"FortniteGame.FortWeaponRanged" );
		game::classes::m_fort_pickup = game::u_object::find_object<game::u_class*>( L"FortniteGame.FortPickup" );
		game::classes::m_container = game::u_object::find_object<game::u_class*>( L"FortniteGame.BuildingContainer" );
		game::classes::m_weakspot = game::u_object::find_object<game::u_class*>( L"FortniteGame.BuildingWeakspot" );
		game::classes::m_vehicle = game::u_object::find_object<game::u_class*>( L"FortniteGame.FortAthenaVehicle" );
		game::classes::m_player_controller = game::u_object::find_object<game::u_class*>( L"Engine.Controller" );
		game::classes::m_building = game::u_object::find_object<game::u_class*>( L"FortniteGame.BuildingActor" );
		game::classes::m_fort_player_pawn_athena = game::u_object::find_object<game::u_class*>( L"FortniteGame.FortPlayerPawnAthena" );
		game::classes::m_item_definition = game::u_object::find_object<game::u_class*>( L"FortniteGame.FortItemDefinition" );
		game::classes::m_material_instance = game::u_object::find_object<game::u_class*>( L"Engine.MaterialInstance" );
		game::classes::m_skeletal_mesh_component = game::u_object::find_object<game::u_class*>( L"Engine.SkeletalMeshComponent" );
		game::classes::m_material_interface = game::u_object::find_object<game::u_class*>( L"Engine.MaterialInterface" );
		game::classes::m_material_instance_dynamic = game::u_object::find_object<game::u_class*>( L"Engine.MaterialInstanceDynamic" );

		game::material::m_rez_in_master = game::u_object::find_object<game::u_material*>( L"/Game/Effects/Fort_Effects/Effects/Characters/Generic/ResIn/RezIn_Master.RezIn_Master" );
		game::material::m_ribbon_master = game::u_object::find_object<game::u_material*>( L"/Game/Effects/Niagara/MasterMaterials/Ribbon/M_Ribbon_Master_v3.M_Ribbon_Master_v3" );

		auto create_key_binding = [ ]( const wchar_t* key_name ) -> game::fkey {
			return game::fkey{
				game::fname{
					game::kismet::m_string_library->conv_string_to_name( key_name )
				},
				0
			};
		};

		const std::pair<game::fkey*, const wchar_t*> key_mappings[ ] = {
			{&game::keys::m_left_mouse, L"LeftMouseButton"},
			{&game::keys::m_right_mouse, L"RightMouseButton"},
			{&game::keys::m_insert, L"Insert"},
			{&game::keys::m_left_shift, L"LeftShift"},
			{&game::keys::m_left_alt, L"LeftAlt"},
			{&game::keys::m_gamepad_left_trigger, L"Gamepad_LeftTrigger"},
			{&game::keys::m_w, L"W"},
			{&game::keys::m_a, L"A"},
			{&game::keys::m_s, L"S"},
			{&game::keys::m_d, L"D"},
			{&game::keys::m_spacebar, L"Spacebar"},
			{&game::keys::m_thumb_mouse_button2, L"ThumbMouseButton2"},
			{&game::keys::m_mouse_wheel_axis, L"MouseWheelAxis"},
			{&game::keys::m_f7, L"F7"},
			{&game::keys::m_f8, L"F8"}
		};

		for ( const auto& [key_ptr, key_name] : key_mappings )
			*key_ptr = create_key_binding( key_name );

		const auto [owclient_base, owclient_size] = module::get_process_module( oxorany( L"OWClient.dll" ) );
		( void )owclient_size;
		if ( !owclient_base ) {
			print_info( oxorany( "OWClient.dll not found\n" ) );
			return false;
		}

		dll::m_hook_long_jump_patch = reinterpret_cast< dll::hook_long_jump_patch_t >(
			owclient_base ? ( owclient_base + 0x9CD20 ) : 0
			);
		dll::m_hook_ctor = reinterpret_cast< dll::hook_ctor_t >(
			owclient_base ? ( owclient_base + 0x984C0 ) : 0
			);
		dll::m_hook_minhook = reinterpret_cast< dll::hook_minhook_t >(
			owclient_base ? ( owclient_base + 0x998A0 ) : 0
			);
		if ( !dll::m_hook_ctor ) {
			log_info(
				oxorany( "Could not resolve FunctionHook constructor.\n\n"
				"OWClient.dll may not be initialized yet.\n"
				"Restart and wait the in-game overlay to appear before injecting." ),
				oxorany( "FunctionHook constructor not found" ), MB_ICONHAND
			);
			return false;
		}
		if ( !dll::m_hook_minhook ) {
			log_info(
				oxorany( "Could not resolve MinHook backend.\n\n"
				"OWClient.dll may not be initialized yet.\n"
				"Restart and wait the in-game overlay to appear before injecting." ),
				oxorany( "MinHook backend not found" ), MB_ICONHAND
			);
			return false;
		}
		if ( !dll::m_hook_long_jump_patch ) {
			log_info(
				oxorany( "Could not resolve hookLongJumpPatch.\n\n"
				"OWClient.dll may not be loaded yet.\n"
				"Restart and wait the in-game overlay to appear before injecting." ),
				oxorany( "hookLongJumpPatch not found" ), MB_ICONHAND
			);
			return false;
		}

		// Thank you MCP servers
		constexpr uintptr_t k_render_ctx_global_rva = 0x43A900;
		constexpr uintptr_t k_swapchain_offset = 0x60;
		constexpr uintptr_t k_cmd_queue_global_rva = 0x439380;
		constexpr int k_max_swapchain_attempts = 200;
		constexpr DWORD k_swapchain_retry_ms = 25;

		const auto render_ctx_global_addr = owclient_base + k_render_ctx_global_rva;
		auto* render_ctx_global_ptr = reinterpret_cast< uintptr_t* >( render_ctx_global_addr );
		if ( !utility::is_valid( render_ctx_global_ptr ) ) {
			print_info( oxorany( "render context global location invalid\n" ) );
			return false;
		}

		void* swapchain = nullptr;
		for ( int attempt = 0; attempt < k_max_swapchain_attempts && !swapchain; ++attempt ) {
			const auto render_ctx = *render_ctx_global_ptr;
			if ( utility::is_valid( render_ctx ) ) {
				auto* swapchain_ptr = reinterpret_cast< void** >( render_ctx + k_swapchain_offset );
				if ( utility::is_valid( swapchain_ptr ) && utility::is_valid( *swapchain_ptr ) )
					swapchain = *swapchain_ptr;
			}

			if ( !swapchain )
				Sleep( k_swapchain_retry_ms );
		}

		if ( !swapchain ) {
			const auto render_ctx_from_value = *render_ctx_global_ptr;
			char dbg[ 256 ]{};
			sprintf(
				dbg,
				oxorany( "swapchain unresolved (global=%p value=%p addr=%p)\n" ),
				reinterpret_cast< void* >( render_ctx_global_addr ),
				reinterpret_cast< void* >( render_ctx_from_value ),
				reinterpret_cast< void* >( render_ctx_global_addr )
			);
			print_info( dbg );
			return false;
		}

		auto is_valid_cmd_queue = [ ]( ID3D12CommandQueue* queue ) -> bool {
			if ( !utility::is_valid( queue ) )
				return false;

			D3D12_COMMAND_QUEUE_DESC desc{};
			__try {
				desc = queue->GetDesc( );
			}
			__except ( 1 ) {
				return false;
			}

			return desc.Type == D3D12_COMMAND_LIST_TYPE_DIRECT;
		};

		auto* ida_queue_ptr = reinterpret_cast< ID3D12CommandQueue** >( owclient_base + k_cmd_queue_global_rva );
		auto* ida_queue = utility::is_valid( ida_queue_ptr ) ? *ida_queue_ptr : nullptr;
		if ( !is_valid_cmd_queue( ida_queue ) )
			ida_queue = nullptr;

		if ( !ida_queue ) {
			print_info( oxorany( "IDA queue not available yet\n" ) );
			return false;
		}

		if ( dll::m_cmd_queue )
			dll::m_cmd_queue->Release( );

		dll::m_cmd_queue = ida_queue;
		dll::m_cmd_queue->AddRef( );

		char qmsg[ 192 ]{};
		sprintf(
			qmsg,
			oxorany( "IDA queue acquired: queue=%p\n" ),
			dll::m_cmd_queue
		);
		print_info( qmsg );

		auto** vtable = *reinterpret_cast< void*** >( swapchain );
		auto** present_slot = vtable ? &vtable[ 8 ] : nullptr;
		auto** resizebuffers_slot = vtable ? &vtable[ 13 ] : nullptr;
		auto* current_present = present_slot ? *present_slot : nullptr;
		auto* current_resizebuffers = resizebuffers_slot ? *resizebuffers_slot : nullptr;

		char rb_slot_msg[ 224 ]{};
		sprintf_s(
			rb_slot_msg,
			oxorany( "ResizeBuffers slot info: slot=%p current=%p\n" ),
			resizebuffers_slot,
			current_resizebuffers
		);
		print_info( rb_slot_msg );

		if ( !present_slot || !current_present ) {
			print_info( oxorany( "present slot invalid\n" ) );
			return false;
		}

		if ( !resizebuffers_slot || !current_resizebuffers ) {
			print_info( oxorany( "resizebuffers slot invalid\n" ) );
			return false;
		}

		if ( dll::m_present_detour_installed ) {
			print_info( oxorany( "present detour already installed\n" ) );
			if ( !render::present_org ) {
				print_info( oxorany( "present_org missing on already-installed detour\n" ) );
				return false;
			}
		}
		else {
			const auto present_shim = reinterpret_cast< unsigned __int8* >( owclient_base + 0x25C20 );
			dll::m_present_detour = {};
			dll::m_hook_ctor( reinterpret_cast< __int64 >( &dll::m_present_detour ), oxorany( L"divinity_present" ) );

			const auto detour_result = dll::m_hook_minhook(
				reinterpret_cast< __int64 >( &dll::m_present_detour ),
				reinterpret_cast< __int64 >( present_shim ),
				reinterpret_cast< __int64 >( render::present_hk )
			);

			if ( detour_result >= 0 && dll::m_present_detour.original ) {
				render::present_org = reinterpret_cast< decltype( render::present_org ) >( dll::m_present_detour.original );
				dll::m_present_detour_installed = true;
				print_info( oxorany( "present shim hook installed through MinHook backend\n" ) );
			}
			else {
				print_info( oxorany( "present shim hook failed, falling back to VMT\n" ) );

				if ( current_present == reinterpret_cast< void* >( render::present_hk ) ) {
					print_info( oxorany( "present already hooked by this module\n" ) );
					if ( !render::present_org ) {
						print_info( oxorany( "present_org missing on already-hooked slot\n" ) );
						return false;
					}
				}
				else {
					const auto dxgi_base = module::get_module_base( L"dxgi.dll" );
					const auto current_mod = module::get_module_by_address( reinterpret_cast< uintptr_t >( current_present ) );
					if ( dxgi_base && current_mod && current_mod != dxgi_base )
						print_info( oxorany( "warning: present slot already hooked by another module\n" ) );

					static Hook::NewHook present_vmt = {};
					if ( !present_vmt.VMT( swapchain, render::present_hk, 8, &render::present_org ) ) {
						print_info( oxorany( "failed to hook swapchain present\n" ) );
						return false;
					}
				}
			}
		}

		if ( dll::m_resizebuffers_detour_installed ) {
			print_info( oxorany( "resizebuffers detour already installed\n" ) );
			if ( !render::resizebuffers_org ) {
				print_info( oxorany( "resizebuffers_org missing on already-installed detour\n" ) );
				return false;
			}
		}
		else {
			const auto resizebuffers_shim = reinterpret_cast< unsigned __int8* >( owclient_base + 0x26340 );
			dll::m_resizebuffers_detour = {};
			dll::m_hook_ctor( reinterpret_cast< __int64 >( &dll::m_resizebuffers_detour ), oxorany( L"divinity_resizebuffers" ) );

			const auto detour_result = dll::m_hook_minhook(
				reinterpret_cast< __int64 >( &dll::m_resizebuffers_detour ),
				reinterpret_cast< __int64 >( resizebuffers_shim ),
				reinterpret_cast< __int64 >( render::resizebuffers_hk )
			);

			if ( detour_result >= 0 && dll::m_resizebuffers_detour.original ) {
				render::resizebuffers_org = reinterpret_cast< decltype( render::resizebuffers_org ) >( dll::m_resizebuffers_detour.original );
				dll::m_resizebuffers_detour_installed = true;
				print_info( oxorany( "resizebuffers shim hook installed through MinHook backend\n" ) );
			}
			else {
				print_info( oxorany( "failed to hook swapchain ResizeBuffers shim\n" ) );
				return false;
			}
		}

		dll::m_hooked_swapchain = reinterpret_cast< IDXGISwapChain* >( swapchain );
		dll::m_present_slot = present_slot;
		dll::m_resizebuffers_slot = resizebuffers_slot;
		dll::m_backend = dll::backend_t::unknown;

		return true;
	}
}
























