#pragma once
namespace game {
	class u_kismet_system_library : public u_object {
	public:
		bool line_trace_single(
			u_object* world,
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
			if ( !function ) {
				function = find_object<u_function*>( oxorany( L"KismetSystemLibrary.LineTraceSingle" ) );
			}

			struct {
				u_object* world_context_object;
				fvector start;
				fvector end;
				t_enum_ss_byte<trace_type_query> trace_channel;
				bool trace_complex;
				tarray<uint64_t> actors_to_ignore;
				t_enum_ss_byte<draw_debug_trace> draw_debug_type;
				f_hit_result out_hit;
				bool ignore_self;
				flinear_color trace_color;
				flinear_color trace_hit_color;
				float draw_time;
				unsigned char padding[ 0x8 ];
				bool return_value;
			} params;

			params.world_context_object = world;
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

			return !params.return_value;
		}

		fstring get_object_name( u_object* object ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetSystemLibrary.GetObjectName" ) );
			}

			struct {
				u_object* object;
				fstring return_value;
			} params{ object };

			this->process_event( function, &params );
			return params.return_value;
		}

		void get_component_bounds( u_scene_component* component, fvector* origin, fvector* box_extent, float* sphere_radius ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetSystemLibrary.GetComponentBounds" ) );
			}

			struct {
				u_scene_component* component;
				fvector origin;
				fvector box_extent;
				float sphere_radius;
			} params{ component };

			this->process_event( function, &params );

			if ( origin != nullptr )
				*origin = params.origin;
			if ( box_extent != nullptr )
				*box_extent = params.box_extent;
			if ( sphere_radius != nullptr )
				*sphere_radius = params.sphere_radius;
		}
	};
}