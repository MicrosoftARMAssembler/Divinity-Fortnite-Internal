#pragma once

namespace game {
	class u_kismet_math_library : public u_object {
	public:
		fvector get_forward_vector( frotator in_rot ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.GetForwardVector" ) );
			}

			struct {
				frotator in_vec;
				fvector return_value;
			} params{ in_rot };

			this->process_event( function, &params );
			return params.return_value;
		}

		frotator conv_vector_to_rotator( fvector in_vec ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.Conv_VectorToRotator" ) );
			}

			struct {
				fvector in_vec;
				frotator return_value;
			} params{ in_vec };

			this->process_event( function, &params );
			return params.return_value;
		}

		fvector conv_vector_2d_to_vector( fvector2d in_vector_2d, float z ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.Conv_Vector2DToVector" ) );
			}

			struct {
				fvector2d in_vector_2d;
				float z;
				fvector return_value;
			} params{ in_vector_2d, z };

			this->process_event( function, &params );
			return params.return_value;
		}

		frotator find_look_at_rotation( fvector start, fvector target ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.FindLookAtRotation" ) );
			}

			struct {
				fvector start;
				fvector target;
				frotator return_value;
			} params{ start, target };

			this->process_event( function, &params );
			return params.return_value;
		}

		frotator r_interp_to( frotator current, frotator target, float delta_time, float interp_speed ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.RInterpTo" ) );
			}

			struct {
				frotator current;
				frotator target;
				float delta_time;
				float interp_speed;
				frotator return_value;
			} params{ current, target, delta_time, interp_speed };

			this->process_event( function, &params );
			return params.return_value;
		}

		double vector_distance( fvector v_1, fvector v_2 ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.Vector_Distance" ) );
			}

			struct {
				fvector v_1;
				fvector v_2;
				double return_value;
			} params{ v_1, v_2 };

			this->process_event( function, &params );
			return params.return_value;
		}

		flinear_color linear_color_lerp_using_hsv( flinear_color a, flinear_color b, float alpha ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.LinearColorLerpUsingHSV" ) );
			}

			struct {
				flinear_color a;
				flinear_color b;
				float alpha;
				flinear_color return_value;
			} params{ a, b, alpha };

			this->process_event( function, &params );
			return params.return_value;
		}

		double sin( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.sin" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double cos( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.cos" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double acos( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.acos" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double asin( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.asin" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double atan2( double m_y, double m_x ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.Atan2" ) );
			}

			struct {
				double m_y;
				double m_x;
				double return_value;
			} params{ m_y, m_x };

			this->process_event( function, &params );
			return params.return_value;
		}

		double pow( double base, double exp ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.MultiplyMultiply_FloatFloat" ) );
			}

			struct {
				double base;
				double exp;
				double return_value;
			} params{ base, exp };

			this->process_event( function, &params );
			return params.return_value;
		}

		double abs( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.abs" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double sqrt( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.sqrt" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double atan( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.atan" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}

		double tan( double a ) {
			static u_function* function = 0;
			if ( !function ) {
				function = find_object< u_function* >( oxorany( L"KismetMathLibrary.tan" ) );
			}

			struct {
				double a;
				double return_value;
			} params{ a };

			this->process_event( function, &params );
			return params.return_value;
		}
	};
}