#pragma once

namespace game {
	class u_fort_kismet_library : public u_object {
	public:
		tarray<a_controller*> get_all_fort_player_controllers( u_world*& world_context_object ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object<u_function*>( oxorany( L"FortKismetLibrary.GetAllFortPlayerControllers" ) );
			}

			struct {
				u_world*& world_context_object;
				bool include_non_spectators;
				bool include_spectators;
				tarray<a_controller*> return_value;
			} params{ world_context_object, true, true };

			this->process_event( function, &params );

			return params.return_value;
		}

		a_controller* get_controller_from_actor( a_actor* actor ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object<u_function*>( oxorany( L"FortKismetLibrary.GetControllerFromActor" ) );
			}

			struct {
				a_actor* actor;
				a_controller* return_value;
			} params{ actor };

			this->process_event( function, &params );

			return params.return_value;
		}

		fstring get_human_readable_name( a_actor* actor_a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"FortKismetLibrary.GetHumanReadableName" ) );
			}

			struct {
				a_actor* actor_a;
				fstring return_value;
			} params{ actor_a };

			this->process_event( function, &params );

			return params.return_value;
		}

		bool check_line_of_sight_to_actor_with_channel( fvector source_pos, a_actor* target, e_collision_channel target_filter_channel, a_actor* source ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"FortKismetLibrary.CheckLineOfSightToActorWithChannel" ) );
			}

			struct {
				fvector source_pos;
				a_actor* target;
				e_collision_channel target_filter_channel;
				a_actor* source;
				bool return_value;
			} params{ source_pos,target, target_filter_channel, source };

			this->process_event( function, &params );

			return params.return_value;
		}

		bool on_same_team( a_actor* actor_a, a_actor* actor_b ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"FortKismetLibrary.OnSameTeam" ) );
			}

			struct {
				a_actor* actor_a;
				a_actor* actor_b;
				bool return_value;
			} params{ actor_a, actor_b };

			this->process_event( function, &params );

			return params.return_value;
		}
	};
}