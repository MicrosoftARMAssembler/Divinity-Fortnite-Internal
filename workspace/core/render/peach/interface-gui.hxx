#pragma once

namespace peach {
    namespace text {
        enum class style : int {
            none = 0,
            outline = 1,
            shadow = 2,
        };

        inline void draw(
            const wchar_t* text,
            float            x,
            float            y,
            float            font_size,
            color_t   color,
            style            render_type = style::outline ) {
            static const color_t outline_color = color_t( 0, 0, 0, 255 );
            const color_t shadow_color = color_t( 0, 0, 0, 180 );

            if ( render_type == style::outline ) {
                const float offsets[ ][ 2 ] = {
                    { -1, -1 }, {  1, -1 }, { -1,  1 },
                    {  1,  1 }, { -1,  0 }, {  1,  0 },
                    {  0, -1 }, {  0,  1 }
                };
                for ( const auto& o : offsets )
                    g_interface->draw_text_w( text,
                        x + o[ 0 ], y + o[ 1 ],
                        oxorany( "Manrope" ), font_size,
                        outline_color, color_t( 0, 0, 0, 0 ),
                        0.f, text_align::center );
            }
            else if ( render_type == style::shadow ) {
                g_interface->draw_text_w( text,
                    x + 1.f, y + 1.f,
                    oxorany( "Manrope" ), font_size,
                    shadow_color, color_t( 0, 0, 0, 0 ),
                    0.f, text_align::center );
            }

            g_interface->draw_text_w( text,
                x, y,
                oxorany( "Manrope" ), font_size,
                color, color_t( 0, 0, 0, 0 ),
                0.f, text_align::center );
        }

        // alignment variant
        inline void draw(
            const wchar_t* text,
            float               x,
            float               y,
            float               font_size,
            color_t      color,
            text_align   align,
            style               render_type = style::shadow ) {
            static const color_t outline_color = color_t( 0, 0, 0, 255 );
            const color_t shadow_color = color_t( 0, 0, 0, 180 );

            if ( render_type == style::outline ) {
                const float offsets[ ][ 2 ] = {
                    { -1, -1 }, {  1, -1 }, { -1,  1 },
                    {  1,  1 }, { -1,  0 }, {  1,  0 },
                    {  0, -1 }, {  0,  1 }
                };
                for ( const auto& o : offsets )
                    g_interface->draw_text_w( text,
                        x + o[ 0 ], y + o[ 1 ],
                        oxorany( "Manrope" ), font_size,
                        outline_color, color_t( 0, 0, 0, 0 ),
                        0.f, align );
            }
            else if ( render_type == style::shadow ) {
                g_interface->draw_text_w( text,
                    x + 1.f, y + 1.f,
                    oxorany( "Manrope" ), font_size,
                    shadow_color, color_t( 0, 0, 0, 0 ),
                    0.f, align );
            }

            g_interface->draw_text_w( text,
                x, y,
                oxorany( "Manrope" ), font_size,
                color, color_t( 0, 0, 0, 0 ),
                0.f, align );
        }

        bool init_fonts( peach::context_t& c ) {
            if ( !peach::font::load( poppins_regular_bytes, sizeof( poppins_regular_bytes ), "Poppins" ) ) return false;
            if ( !peach::font::load( poppins_semibold_bytes, sizeof( poppins_semibold_bytes ), "Poppins Semibold" ) ) return false;
            if ( !peach::font::load( manrope_semibold_bytes, sizeof( manrope_semibold_bytes ), "Manrope" ) ) return false;
            if ( !peach::font::load_compressed_raw( af_compressed_data, af_compressed_size, "Font Awesome" ) ) return false;

            return peach::font::create_faces( c.m_dwrite_factory );
        }
    }
}