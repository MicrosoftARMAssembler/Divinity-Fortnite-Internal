#pragma once

namespace game {
    class u_gameplay_statics : public u_object {
    public:
        double get_real_time_seconds( u_object* world_context_object ) {
            static u_function* function = 0;
            if ( !function ) {
                function = find_object< u_function* >( oxorany( L"GameplayStatics.GetRealTimeSeconds" ) );
            }

            struct {
                u_object* world_context_object;
                double return_value;
            } params{ world_context_object };

            this->process_event( function, &params );

            return params.return_value;
        }

        template <class t>
        tarray<t> get_all_actors_of_class( u_object* world_context_object, u_object* actor_class ) {
            static u_function* function = 0;
            if ( !function ) {
                function = find_object< u_function* >( oxorany( L"GameplayStatics.GetAllActorsOfClass" ) );
            }

            struct {
                u_object* world_context_object;
                u_object* actor_class;
                tarray<t> out_actors;
            } params{ world_context_object, actor_class };

            this->process_event( function, &params );

            return params.out_actors;
        }

        double get_world_delta_seconds( u_object* world_context_object ) {
            static u_function* function = 0;
            if ( !function ) {
                function = find_object< u_function* >( oxorany( L"GameplayStatics.GetWorldDeltaSeconds" ) );
            }

            struct {
                u_object* world_context_object;
                double return_value;
            } params{ world_context_object };

            this->process_event( function, &params );

            return params.return_value;
        }

        double get_time_seconds( u_object* world_context_object ) {
            static u_function* function = 0;
            if ( !function ) {
                function = find_object< u_function* >( oxorany( L"GameplayStatics.GetTimeSeconds" ) );
            }

            struct {
                u_object* world_context_object;
                double return_value;
            } params{ world_context_object };

            this->process_event( function, &params );

            return params.return_value;
        }

        bool object_is_a( u_object* object, u_object* object_class ) {
            static u_function* function = 0;
            if ( !function ) {
                function = find_object< u_function* >( oxorany( L"GameplayStatics.ObjectIsA" ) );
            }

            struct {
                u_object* object;
                u_object* object_class;
                bool return_value;
            } params{ object, object_class };

            this->process_event( function, &params );

            return params.return_value;
        }
    };
}