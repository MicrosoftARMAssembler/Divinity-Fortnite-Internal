#pragma once

#define NTSTATUS LONG
#include <d3dkmthk.h>
#include <d3d11.h>
#include <d3d11on12.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <stdint.h>
#include <wincodec.h>
#include <dwrite.h>
#include <cmath>
#include <cstring>
#include <winerror.h>
#include <intsafe.h>
#include <d3d10_1.h>
#include <d3d12.h>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dwrite.lib")
#undef DrawText

typedef HRESULT( WINAPI* p_d3d_compile )(
    LPCVOID, SIZE_T, LPCSTR, const D3D_SHADER_MACRO*,
    ID3DInclude*, LPCSTR, LPCSTR, UINT, UINT,
    ID3DBlob**, ID3DBlob** );

typedef HRESULT( WINAPI* p_dwrite_create_factory )(
    DWRITE_FACTORY_TYPE, REFIID, IUnknown** );

typedef HRESULT( WINAPI* p_co_create_instance )(
    REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID* );

typedef HRESULT( WINAPI* p_co_initialize_ex )( LPVOID, DWORD );

typedef HRESULT( WINAPI* p_d3d11on12_create_device )(
    IUnknown*, UINT, const D3D_FEATURE_LEVEL*, UINT,
    IUnknown* const*, UINT, UINT,
    ID3D11Device**, ID3D11DeviceContext**, D3D_FEATURE_LEVEL* );

#define D3DCOMPILE_ENABLE_STRICTNESS    (1 << 11)
#define D3DCOMPILE_OPTIMIZATION_LEVEL3  (1 << 15)

namespace peach {

    constexpr float k_pi = 3.14159265359f;
    constexpr float k_pi_2 = 1.57079632679f;
    constexpr float k_pi_3_2 = 4.71238898038f;
    constexpr float k_tau = 6.28318530718f;
    IDWriteFontCollection1* m_font_collection = nullptr;

    namespace font {
        inline IDWriteFontFace* get_face( const char* name );
    }

    static int mm_strlen( const char* s ) {
        if ( !s ) return 0;
        auto n = 0;
        while ( s[ n ] ) ++n;
        return n;
    }

    static int mm_strcmp( const char* a, const char* b ) {
        if ( !a || !b ) return ( a == b ) ? 0 : 1;
        while ( *a && *b && *a == *b ) { ++a; ++b; }
        return ( unsigned char )*a - ( unsigned char )*b;
    }

    static void mm_strncpy( char* dst, const char* src, int max_len ) {
        if ( !dst || max_len <= 0 ) return;
        auto i = 0;
        while ( i < max_len - 1 && src && src[ i ] ) { dst[ i ] = src[ i ]; ++i; }
        dst[ i ] = '\0';
    }

    static int mm_mbstowcs( wchar_t* dst, const char* src, int max_wchars ) {
        return MultiByteToWideChar( CP_UTF8, 0, src, -1, dst, max_wchars );
    }

    __forceinline static float mm_max( float a, float b ) { return a > b ? a : b; }
    __forceinline static float mm_min( float a, float b ) { return a < b ? a : b; }
    __forceinline static int   mm_imax( int a, int b ) { return a > b ? a : b; }
    __forceinline static int   mm_imin( int a, int b ) { return a < b ? a : b; }

    namespace sin_cos_lut {
        constexpr int k_max_segments = 128;
        static float s_sin[ k_max_segments + 1 ];
        static float s_cos[ k_max_segments + 1 ];
        static int   s_segments = 0;

        static void init( int segments ) {
            if ( segments == s_segments || segments > k_max_segments ) return;
            s_segments = segments;
            const auto step = k_tau / segments;
            for ( auto i = 0; i <= segments; i++ ) {
                s_sin[ i ] = sinf( i * step );
                s_cos[ i ] = cosf( i * step );
            }
        }
    }

    namespace quarter_arc_lut {
        constexpr int k_max_segs = 16;
        static float s_sin[ k_max_segs + 1 ];
        static float s_cos[ k_max_segs + 1 ];
        static int   s_segments = 0;

        static void init( int segments ) {
            if ( segments == s_segments || segments > k_max_segs ) return;
            s_segments = segments;
            const auto step = k_pi_2 / segments;
            for ( auto i = 0; i <= segments; i++ ) {
                s_sin[ i ] = sinf( i * step );
                s_cos[ i ] = cosf( i * step );
            }
        }
    }

    enum class text_align { left, center, right };

    struct color_t {
        uint8_t r, g, b, a;
        constexpr color_t( ) : r( 0 ), g( 0 ), b( 0 ), a( 255 ) { }
        constexpr color_t( uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255 ) : r( r ), g( g ), b( b ), a( a ) { }
        constexpr color_t( uint32_t rgba ) : r( ( rgba >> 16 ) & 0xFF ), g( ( rgba >> 8 ) & 0xFF ), b( rgba & 0xFF ), a( ( rgba >> 24 ) & 0xFF ) { }
        constexpr uint32_t pack( ) const { return ( b ) | ( g << 8 ) | ( r << 16 ) | ( a << 24 ); }
    };

    template<typename K, typename V, uint32_t Capacity>
    struct fixed_map_t {
        struct slot_t {
            K        key;
            V        value;
            uint32_t psl;   // probe sequence length (Robin Hood)
            bool     used;
        };

        slot_t   slots[ Capacity ] = {};
        uint32_t m_count = 0;

        // ── helpers ────────────────────────────────────────────────────────────

        static constexpr uint32_t capacity = Capacity;

        uint32_t hash( const K& key ) const {
            // FNV-1a over the raw bytes of K
            auto* p = reinterpret_cast< const uint8_t* >( &key );
            uint32_t h = 2166136261u;
            for ( auto i = 0u; i < sizeof( K ); i++ )
                h = ( h ^ p[ i ] ) * 16777619u;
            return h;
        }

        uint32_t ideal( const K& key ) const {
            return hash( key ) % Capacity;
        }

        // ── find ───────────────────────────────────────────────────────────────

        V* find( const K& key ) {
            auto pos = ideal( key );
            uint32_t psl = 0;

            for ( uint32_t i = 0; i < Capacity; i++, psl++ ) {
                auto& s = slots[ pos ];

                // Empty slot — key not present
                if ( !s.used )
                    return nullptr;

                // Robin Hood early-out: if incumbent's PSL < our probe length,
                // the key we're looking for would have displaced it, so it's absent
                if ( s.psl < psl )
                    return nullptr;

                if ( s.key == key )
                    return &s.value;

                if ( ++pos == Capacity ) pos = 0;
            }
            return nullptr;
        }

        const V* find( const K& key ) const {
            return const_cast< fixed_map_t* >( this )->find( key );
        }

        // ── insert ─────────────────────────────────────────────────────────────

        // Returns pointer to value (existing or newly inserted), or nullptr if full
        V* insert( const K& key ) {
            if ( m_count >= Capacity )
                return nullptr;

            auto pos = ideal( key );
            slot_t incoming{ key, V{}, 0u, true };

            for ( uint32_t i = 0; i < Capacity; i++ ) {
                auto& s = slots[ pos ];

                if ( !s.used ) {
                    s = incoming;
                    m_count++;
                    return &s.value;
                }

                if ( s.key == key ) {
                    return &s.value;
                }

                if ( s.psl < incoming.psl ) {
                    auto tmp = s;
                    s = incoming;
                    incoming = tmp;
                }

                incoming.psl++;
                if ( ++pos == Capacity ) pos = 0;
            }

            return nullptr;
        }

        // ── remove ─────────────────────────────────────────────────────────────

        bool remove( const K& key ) {
            auto pos = ideal( key );
            uint32_t psl = 0;

            // Phase 1: find the slot
            uint32_t target = Capacity; // sentinel — not found yet
            for ( uint32_t i = 0; i < Capacity; i++, psl++ ) {
                auto& s = slots[ pos ];
                if ( !s.used || s.psl < psl )
                    return false; // not present
                if ( s.key == key ) {
                    target = pos;
                    break;
                }
                if ( ++pos == Capacity ) pos = 0;
            }

            if ( target == Capacity )
                return false;

            // Phase 2: backward-shift deletion (fixes the bug from before)
            // We walk FORWARD from the deleted slot, pulling each neighbour back
            // one position as long as its PSL > 0 (meaning it was displaced).
            uint32_t current = target;
            for ( uint32_t i = 0; i < Capacity - 1; i++ ) {
                uint32_t next = current + 1;
                if ( next == Capacity ) next = 0;

                auto& sn = slots[ next ];

                // Stop if next slot is empty or sits at its ideal position (PSL == 0)
                if ( !sn.used || sn.psl == 0 ) {
                    slots[ current ].used = false;
                    break;
                }

                // Pull neighbour back, decrement its PSL
                slots[ current ] = sn;
                slots[ current ].psl -= 1;
                slots[ next ].used = false;
                current = next;
            }

            m_count--;
            return true;
        }

        // ── misc ───────────────────────────────────────────────────────────────

        void clear( ) {
            memset( slots, 0, sizeof( slots ) );
            m_count = 0;
        }

        uint32_t count( ) const { return m_count; }
        bool     empty( ) const { return m_count == 0; }
    };

    struct glyph_key_t {
        char     font_name[ 64 ];
        int      font_size;
        uint32_t char_code;
        int      outline_thickness;

        bool operator==( const glyph_key_t& o ) const {
            return font_size == o.font_size && char_code == o.char_code &&
                outline_thickness == o.outline_thickness &&
                mm_strcmp( font_name, o.font_name ) == 0;
        }
    };

    static uint32_t glyph_key_hash( const glyph_key_t& k ) {
        auto h = 2166136261u;
        for ( auto i = 0; k.font_name[ i ] && i < 64; i++ )
            h = ( h ^ ( uint8_t )k.font_name[ i ] ) * 16777619u;
        h = ( h ^ ( uint32_t )k.font_size ) * 16777619u;
        h = ( h ^ k.char_code ) * 16777619u;
        h = ( h ^ ( uint32_t )k.outline_thickness ) * 16777619u;
        return h;
    }

    struct glyph_info_t {
        float u0, v0, u1, v1;
        float width, height;
        float bearing_x, bearing_y;
        float advance_x;
        float line_height;
    };

    constexpr int k_glyph_map_capacity = 4096;

    struct glyph_map_t {
        struct slot_t {
            glyph_key_t  key;
            glyph_info_t value;
            bool         used = false;
        };
        slot_t slots[ k_glyph_map_capacity ];
        int    count = 0;

        void clear( ) { memset( slots, 0, sizeof( slots ) ); count = 0; }

        glyph_info_t* find( const glyph_key_t& key ) {
            auto h = glyph_key_hash( key ) % k_glyph_map_capacity;
            for ( auto i = 0; i < k_glyph_map_capacity; i++ ) {
                auto idx = ( h + i ) % k_glyph_map_capacity;
                if ( !slots[ idx ].used ) return nullptr;
                if ( slots[ idx ].key == key ) return &slots[ idx ].value;
            }
            return nullptr;
        }

        glyph_info_t* insert( const glyph_key_t& key ) {
            if ( count >= k_glyph_map_capacity * 3 / 4 ) return nullptr;
            auto h = glyph_key_hash( key ) % k_glyph_map_capacity;
            for ( auto i = 0; i < k_glyph_map_capacity; i++ ) {
                auto idx = ( h + i ) % k_glyph_map_capacity;
                if ( !slots[ idx ].used ) {
                    slots[ idx ].key = key;
                    slots[ idx ].used = true;
                    count++;
                    return &slots[ idx ].value;
                }
                if ( slots[ idx ].key == key ) return &slots[ idx ].value;
            }
            return nullptr;
        }
    };

    struct texture_handle_t {
        float u0, v0, u1, v1;
        float width, height;
        bool  valid;
    };

    constexpr int k_texture_map_capacity = 512;
    using texture_map_t = fixed_map_t<uint32_t, texture_handle_t, k_texture_map_capacity>;

    constexpr uint32_t k_max_animation_frames = 256;
    struct animated_texture_handle_t {
        uint32_t frame_ids[ k_max_animation_frames ];
        uint32_t delays[ k_max_animation_frames ];
        uint32_t frame_count;
        uint32_t loop_count;
        float    width, height;
        double   current_time;
        uint32_t current_frame;
        uint32_t loop_counter;
        bool     playing;
        bool     valid;
    };

    constexpr int k_anim_map_capacity = 32;
    using anim_map_t = fixed_map_t<uint32_t, animated_texture_handle_t, k_anim_map_capacity>;

    struct rect_packer_t {
        int width, height;
        int current_x, current_y, current_row_height;

        void init( int w, int h ) {
            width = w; height = h;
            current_x = 2; current_y = 2; current_row_height = 0;
        }

        bool pack( int w, int h, int& out_x, int& out_y ) {
            if ( current_x + w + 2 > width ) {
                current_x = 2;
                current_y += current_row_height + 4;
                current_row_height = 0;
            }
            if ( current_y + h + 2 > height ) return false;
            out_x = current_x;
            out_y = current_y;
            current_x += w + 4;
            if ( h > current_row_height ) current_row_height = h;
            return true;
        }
    };

    constexpr int    k_atlas_size = 2048;
    constexpr int    k_text_atlas_bytes = k_atlas_size * k_atlas_size * 2;
    constexpr int    k_tex_atlas_bytes = k_atlas_size * k_atlas_size * 4;
    constexpr size_t k_glyph_scratch_sz = 1024 * 1024;
    constexpr size_t k_image_scratch_sz = 8 * 1024 * 1024;
    constexpr int    k_utf8_conv_buf = 4096;

    struct unified_vertex_t {
        float    x, y;
        float    u, v;
        uint32_t color;
        uint32_t outline_color;
        uint32_t params;
    };


    class context_t {
    public:
        // ── DX11 init (existing) ──────────────────────────────────────────
        bool init( ID3D11Device* device, ID3D11DeviceContext* context, IDXGISwapChain* swap_chain, bool skip_rtv = false );

        // ── DX12 init (new) ───────────────────────────────────────────────
        // Wraps the DX12 device/queue with D3D11On12 so all existing draw
        // calls work unchanged.  Call instead of init() when the backend
        // is DX12.  queue is borrowed — caller must keep it alive.
        bool init_dx12( ID3D12Device* device12, ID3D12CommandQueue* queue, IDXGISwapChain* swapchain );

        void shutdown( );

        // Use begin_frame() for DX11, begin_frame_dx12() for DX12.
        void begin_frame( color_t clear_color = color_t( 0, 0, 0, 0 ) );
        void begin_frame_dx12( color_t clear_color = color_t( 0, 0, 0, 0 ) );

        void set_text_antialiasing( bool enabled ) { if ( m_text_aa != enabled ) { m_text_aa = enabled; m_text_aa_dirty = true; } }
        void set_shape_antialiasing( bool enabled ) { m_shape_aa = enabled; }
        void set_text_pixel_snapping( bool enabled ) { m_text_pixel_snap = enabled; }

        void draw_rect( float x, float y, float w, float h, color_t color );
        void draw_rect_outline( float x, float y, float w, float h, float thickness, color_t color );
        void draw_line( float x0, float y0, float x1, float y1, float thickness, color_t color );
        void draw_circle( float x, float y, float radius, color_t color, int segments = 32 );
        void draw_circle_outline( float x, float y, float radius, float thickness, color_t color, int segments = 32 );
        void draw_rounded_rect( float x, float y, float w, float h, float radius, color_t color );
        void draw_rounded_rect_outline( float x, float y, float w, float h, float radius, float thickness, color_t color );
        void draw_triangle( float x0, float y0, float x1, float y1, float x2, float y2, color_t color );
        void draw_triangle_outline( float x0, float y0, float x1, float y1, float x2, float y2, float thickness, color_t color );
        void draw_polygon( float cx, float cy, float radius, int sides, color_t color );
        void draw_polygon_outline( float cx, float cy, float radius, int sides, float thickness, color_t color );
        void draw_pie( float x, float y, float radius, float start_angle, float end_angle, color_t color, int segments = 32 );
        void draw_arc( float x, float y, float radius, float start_angle, float end_angle, float thickness, color_t color, int segments = 32 );
        void draw_bezier_curve( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float thickness, color_t color, int segments = 32 );
        void draw_rect_gradient( float x, float y, float w, float h, color_t top_color, color_t bottom_color );
        void draw_rect_gradient_h( float x, float y, float w, float h, color_t left_color, color_t right_color );
        void draw_rect_shadow( float x, float y, float w, float h, float radius, float offset_x, float offset_y, color_t color );
        void draw_line_dashed( float x0, float y0, float x1, float y1, float thickness, float dash_length, float gap_length, color_t color );
        inline void draw_glow( float x, float y, float w, float h, float radius = 12.f, uint8_t alpha = 40 );

        void  draw_text( const char* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color = color_t( 0, 0, 0, 0 ), float outline_thickness = 0.f, text_align align = text_align::left );
        void  draw_text_w( const wchar_t* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color = color_t( 0, 0, 0, 0 ), float outline_thickness = 0.f, text_align align = text_align::left );
        float measure_text( const char* text, const char* font_name, float font_size, float outline_thickness = 0.f );
        float measure_text_height( const char* font_name, float font_size, float outline_thickness = 0.f );
        float draw_text_return_width( const char* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color = color_t( 0, 0, 0, 0 ), float outline_thickness = 0.f );
        void  draw_text_monospace( const char* text, float x, float y, const char* font_name, float font_size, float char_width, color_t color, color_t outline_color = color_t( 0, 0, 0, 0 ), float outline_thickness = 0.f );
        void  draw_text_multi_color( const char* text, float x, float y, const char* font_name, float font_size, const color_t* colors, int color_count, color_t outline_color = color_t( 0, 0, 0, 0 ), float outline_thickness = 0.f, text_align align = text_align::left );
        float measure_text_to_column( const char* text, const char* font_name, float font_size, int column, float outline_thickness = 0.f );
        float get_codepoint_advance( const char* font_name, float font_size, uint32_t codepoint, float outline_thickness = 0.f );
        static int get_codepoint_count( const char* text );

        uint32_t register_texture( const uint8_t* rgba_data, int width, int height );
        uint32_t register_texture_from_memory( const void* data, size_t data_size );
        void     unregister_texture( uint32_t id );
        void     clear_texture_atlas( );
        void     draw_texture( uint32_t id, float x, float y, float w, float h, color_t tint = color_t( 255, 255, 255, 255 ) );
        void     draw_texture_rect( uint32_t id, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h, color_t tint = color_t( 255, 255, 255, 255 ) );
        void     draw_mask_alpha( uint32_t id, float x, float y, float w, float h, color_t color );
        void     draw_mask_luminance( uint32_t id, float x, float y, float w, float h, color_t color );
        void     draw_mask_alpha_outline( uint32_t id, float x, float y, float w, float h, color_t mask_color, color_t outline_color, float thickness = 1.f );
        bool     get_texture_size( uint32_t id, float& out_w, float& out_h ) const;

        uint32_t register_animated_texture_from_memory( const void* data, size_t data_size );
        void     draw_animated_texture( uint32_t anim_id, float x, float y, float w, float h, float delta_time, color_t tint = color_t( 255, 255, 255, 255 ) );
        void     reset_animation( uint32_t anim_id );
        void     set_animation_playing( uint32_t anim_id, bool playing );
        void     unregister_animated_texture( uint32_t anim_id );
        bool     get_animated_texture_size( uint32_t anim_id, float& out_w, float& out_h ) const;

        void push_clip_rect( float x, float y, float w, float h );
        void pop_clip_rect( );
        void clear_clip_stack( );
        int  get_clip_stack_depth( ) const { return m_clip_stack_depth; }

        void set_scissor_rect( float x, float y, float w, float h );
        void clear_scissor_rect( );

        // Use flush() for DX11, flush_dx12() for DX12.
        void flush( );
        void flush_dx12( );

        float    get_screen_width( )              const { return m_screen_width; }
        float    get_screen_height( )             const { return m_screen_height; }
        float    get_vertex_usage_percent( )      const;
        uint32_t get_last_frame_vertex_count( )   const { return m_completed_frame_vertex_count; }
        bool dx12_wrap_backbuffers( );
        void dx12_release_backbuffers( );
        void dx12_handle_resize( );
        bool in_rect(
            double radius,
            game::fvector2d screen_position
        ) const {
            return ( m_screen_width / 2.f ) >= ( ( m_screen_width / 2.f ) - radius ) && ( m_screen_width / 2.f ) <= ( ( m_screen_width / 2.f ) + radius ) &&
                screen_position.m_y >= ( screen_position.m_y - radius ) && screen_position.m_y <= ( screen_position.m_y + radius );
        }

        bool in_circle(
            double radius,
            game::fvector2d screen_position
        ) const {
            if ( in_rect( radius, screen_position ) ) {
                auto dx = ( m_screen_width / 2.f ) - screen_position.m_x; dx *= dx;
                auto dy = ( m_screen_height / 2.f ) - screen_position.m_y; dy *= dy;
                return dx + dy <= radius * radius;
            } return false;
        }

        bool in_screen( game::fvector2d screen_position ) const {
            if ( screen_position.m_x < 5.0 ||
                screen_position.m_x > get_screen_width( ) - ( 5.0 * 2 ) &&
                screen_position.m_y < 5.0 ||
                screen_position.m_y > get_screen_height( ) - ( 5.0 * 2 ) )
                return false;
            return true;
        }

    public:
        IDWriteFontFace* m_helvetica_face = nullptr;
        static constexpr int k_max_vertices = 6 * 1024 * 1024 / sizeof( unified_vertex_t );
        static constexpr int k_max_backbuffers = 8;

        static unified_vertex_t s_staging_buffer[ k_max_vertices ];

        struct frame_context_t {
            ID3D11Buffer* vertex_buffer;
            ID3D11Query* fence;
            uint32_t      write_offset;
            bool          gpu_busy;
        };

        static constexpr int k_max_clip_rects = 16;
        struct clip_rect_t { float x, y, w, h; };

        // ── DX11 core ─────────────────────────────────────────────────────
        ID3D11Device* m_device = nullptr;
        ID3D11DeviceContext* m_context = nullptr;
        IDXGISwapChain* m_swap_chain = nullptr;
        ID3D11RenderTargetView* m_rtv = nullptr;

        frame_context_t  m_frames[ 3 ] = {};
        uint32_t         m_current_frame = 0;
        uint32_t         m_completed_frame_vertex_count = 0;

        ID3D11Texture2D* m_atlas_texture = nullptr;
        ID3D11ShaderResourceView* m_atlas_srv = nullptr;
        ID3D11Texture2D* m_tex_atlas_texture = nullptr;
        ID3D11ShaderResourceView* m_tex_atlas_srv = nullptr;

        ID3D11VertexShader* m_unified_vs = nullptr;
        ID3D11PixelShader* m_unified_ps = nullptr;
        ID3D11InputLayout* m_unified_layout = nullptr;
        ID3D11Buffer* m_screen_size_cb = nullptr;
        ID3D11Buffer* m_clip_rects_cb = nullptr;
        ID3D11BlendState* m_blend_state = nullptr;
        ID3D11RasterizerState* m_rasterizer_state = nullptr;
        ID3D11RasterizerState* m_rasterizer_state_scissor = nullptr;
        ID3D11DepthStencilState* m_depth_stencil_state = nullptr;
        ID3D11SamplerState* m_sampler_state = nullptr;
        ID3D11SamplerState* m_sampler_state_aa = nullptr;

        // ── DX12 / D3D11On12 interop ──────────────────────────────────────
        // Populated only when init_dx12() is used.  All draw calls still go
        // through the DX11 path above — D3D11On12 handles the translation.
        bool                     m_using_dx12 = false;
        ID3D11On12Device* m_11on12 = nullptr;
        IDXGISwapChain3* m_swapchain3 = nullptr;  // owned AddRef
        ID3D12CommandQueue* m_cmd_queue = nullptr;  // borrowed — caller owns
        p_d3d11on12_create_device m_pfn_d3d11on12 = nullptr;

        // One DX12 back-buffer resource + wrapped DX11 view per swapchain buffer
        ID3D12Resource* m_dx12_bbs[ k_max_backbuffers ] = {};
        ID3D11Resource* m_wrapped_bbs[ k_max_backbuffers ] = {};
        ID3D11RenderTargetView* m_wrapped_rtvs[ k_max_backbuffers ] = {};
        int                      m_bb_count = 0;
        UINT                     m_current_bb = 0;  // cached index for flush_dx12
        bool                     m_dx12_resource_acquired = false;
        UINT                     m_dx12_acquired_bb = ~0u;
        uint32_t                 m_dx12_flush_counter = 0;
        bool                     m_dx12_frame_ready = false;

        // ── Dynamic imports ───────────────────────────────────────────────
        p_d3d_compile            m_pfn_d3d_compile = nullptr;
        p_dwrite_create_factory  m_pfn_dwrite_create_factory = nullptr;
        p_co_create_instance     m_pfn_co_create_instance = nullptr;
        p_co_initialize_ex       m_pfn_co_initialize_ex = nullptr;

        IDWriteFactory* m_dwrite_factory = nullptr;
        IWICImagingFactory* m_wic_factory = nullptr;

        rect_packer_t m_atlas_packer;
        bool          m_atlas_needs_upload = false;
        rect_packer_t m_tex_atlas_packer;
        bool          m_tex_atlas_needs_upload = false;

        glyph_map_t m_glyphs;

        struct fast_cache_t {
            char          font_name[ 64 ];
            int           font_size;
            int           outline;
            glyph_info_t* table[ 128 ];

            void invalidate( ) {
                font_name[ 0 ] = '\0';
                font_size = 0;
                outline = 0;
                memset( table, 0, sizeof( table ) );
            }
        };
        fast_cache_t m_fast_cache;

        texture_map_t m_textures;
        uint32_t      m_next_tex_id = 1;
        anim_map_t    m_anim_textures;
        uint32_t      m_next_anim_id = 1;

        clip_rect_t m_clip_rects[ k_max_clip_rects ];
        int         m_clip_index_stack[ k_max_clip_rects ];
        int         m_clip_stack_depth = 0;
        int         m_clip_next_index = 1;
        bool        m_clip_rects_dirty = true;

        float m_screen_width = 1920.f;
        float m_screen_height = 1080.f;
        bool  m_cb_dirty = true;
        bool  m_text_aa = false;
        bool  m_text_aa_dirty = true;
        bool  m_shape_aa = true;
        bool  m_text_pixel_snap = true;
        bool  m_state_initialized = false;
        bool  m_scissor_enabled = false;
        D3D11_RECT m_scissor_rect = {};

        static uint8_t          s_atlas_data[ k_text_atlas_bytes ];
        static uint8_t          s_tex_atlas_data[ k_tex_atlas_bytes ];
        static uint8_t          s_glyph_scratch[ 4 ][ k_glyph_scratch_sz ];
        static uint8_t          s_image_scratch[ k_image_scratch_sz ];
        static char             s_utf8_conv[ k_utf8_conv_buf ];
        static float            s_srgb_to_linear[ 256 ];
        static float            s_linear_to_srgb[ 256 ];
        static bool             s_luts_init;

        bool resolve_dynamic_imports( );
        bool create_render_target( );
        bool create_shaders( );
        bool create_pipeline_state( );
        bool create_buffers( );
        bool create_atlases( );

        ID3D11VertexShader* m_blur_vs = nullptr;
        ID3D11VertexShader* m_blur_blit_vs = nullptr;
        ID3D11PixelShader* m_blur_blit_ps = nullptr;
        ID3D11Buffer* m_blur_rect_cb = nullptr;

        ID3D11Texture2D* m_captured_backbuffer = nullptr;
        ID3D11ShaderResourceView* m_captured_srv = nullptr;
        ID3D11Texture2D* m_blur_temp[ 2 ] = {};
        ID3D11RenderTargetView* m_blur_rtv[ 2 ] = {};
        ID3D11ShaderResourceView* m_blur_srv[ 2 ] = {};
        ID3D11PixelShader* m_blur_h_ps = nullptr;
        ID3D11PixelShader* m_blur_v_ps = nullptr;
        bool                     m_blur_resources_created = false;

        glyph_info_t* get_or_create_glyph( const char* font_name, float font_size, uint32_t char_code, float outline_thickness );
        void          rasterize_glyph( const glyph_key_t& key,
            int& out_w, int& out_h,
            uint8_t* out_data, size_t out_data_size,
            ABC& out_abc, TEXTMETRICA& out_tm,
            int& out_bounds_left, int& out_bounds_top,
            float& out_advance_width );

        __forceinline uint32_t active_clip_idx( ) const {
            return ( m_clip_stack_depth > 0 ) ? ( uint32_t )m_clip_index_stack[ m_clip_stack_depth - 1 ] : 0u;
        }
        __forceinline uint32_t pack_params( uint32_t prim_type ) const {
            return prim_type | ( active_clip_idx( ) << 8 );
        }
    };

    /*static*/ uint8_t          context_t::s_atlas_data[ k_text_atlas_bytes ];
    /*static*/ uint8_t          context_t::s_tex_atlas_data[ k_tex_atlas_bytes ];
    /*static*/ unified_vertex_t context_t::s_staging_buffer[ context_t::k_max_vertices ];
    /*static*/ uint8_t          context_t::s_glyph_scratch[ 4 ][ k_glyph_scratch_sz ];
    /*static*/ uint8_t          context_t::s_image_scratch[ k_image_scratch_sz ];
    /*static*/ char             context_t::s_utf8_conv[ k_utf8_conv_buf ];
    /*static*/ float            context_t::s_srgb_to_linear[ 256 ];
    /*static*/ float            context_t::s_linear_to_srgb[ 256 ];
    /*static*/ bool             context_t::s_luts_init = false;

    // ═══════════════════════════════════════════════════════════════════════
    // Shaders
    // ═══════════════════════════════════════════════════════════════════════

    static const char* g_unified_vs = R"(
cbuffer ScreenSize : register(b0) {
    float2 rcpScreenSize;
    float2 padding;
};
struct VS_INPUT {
    float2 pos        : POSITION;
    float2 uv         : TEXCOORD0;
    float4 color      : COLOR0;
    float4 outlineColor : COLOR1;
    uint   params     : TEXCOORD1;
};
struct VS_OUTPUT {
    float4 pos          : SV_POSITION;
    float2 uv           : TEXCOORD0;
    float2 screenPos    : TEXCOORD2;
    float4 color        : COLOR0;
    float4 outlineColor : COLOR1;
    nointerpolation uint packedParams : TEXCOORD1;
};
VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT o;
    float2 ndc = input.pos * rcpScreenSize * 2.0 - 1.0;
    ndc.y = -ndc.y;
    o.pos          = float4(ndc, 0.0, 1.0);
    o.screenPos    = input.pos;
    o.uv           = input.uv;
    o.color        = input.color;
    o.outlineColor = input.outlineColor;
    o.packedParams = input.params;
    return o;
}
)";

    static const char* g_unified_ps = R"(
Texture2D   megaAtlas    : register(t0);
Texture2D   textureAtlas : register(t1);
SamplerState atlasSampler : register(s0);
cbuffer ClipRects : register(b1) {
    float4 clipRects[16];
};
struct PS_INPUT {
    float4 pos          : SV_POSITION;
    float2 uv           : TEXCOORD0;
    float2 screenPos    : TEXCOORD2;
    float4 color        : COLOR0;
    float4 outlineColor : COLOR1;
    nointerpolation uint packedParams : TEXCOORD1;
};
float4 main(PS_INPUT input) : SV_TARGET {
    uint primitiveType = input.packedParams & 0xFF;
    uint clipIndex     = (input.packedParams >> 8) & 0xF;
    if (clipIndex > 0) {
        float4 cr = clipRects[clipIndex];
        float2 minB = cr.xy, maxB = cr.xy + cr.zw;
        if (any(input.screenPos < minB) || any(input.screenPos > maxB)) discard;
    }
if (primitiveType == 0) {
    float dist = input.uv.x, aaEnabled = input.uv.y;
    if (aaEnabled < 0.5) return float4(input.color.rgb * input.color.a, input.color.a);
    float2 grad = float2(ddx(dist), ddy(dist));
    float gradLen = length(grad);
    if (dist < 0.3 && gradLen < 0.01) return float4(input.color.rgb * input.color.a, input.color.a);
    float pixelDist = (1.0 - dist) / max(gradLen, 0.001);
    float alpha = smoothstep(0.0, 1.0, saturate(pixelDist));
    float fa = input.color.a * alpha;
    return float4(input.color.rgb * fa, fa);
}
    if (primitiveType == 2) {
        float4 tex = textureAtlas.Sample(atlasSampler, input.uv);
        float4 r = tex * input.color;
        return float4(r.rgb * r.a, r.a);
    }
    if (primitiveType == 3) {
        float4 tex = textureAtlas.Sample(atlasSampler, input.uv);
        float fa = input.color.a * tex.a;
        return float4(input.color.rgb * fa, fa);
    }
    if (primitiveType == 4) {
        float4 tex = textureAtlas.Sample(atlasSampler, input.uv);
        float luma = dot(tex.rgb, float3(0.299, 0.587, 0.114));
        float fa = input.color.a * luma;
        return float4(input.color.rgb * fa, fa);
    }
    if (primitiveType == 5) {
        uint thicknessBits = (input.packedParams >> 12) & 0x7;
        float thickness = 0.5 + thicknessBits * 0.5;
        float2 offset = fwidth(input.uv) * thickness;
        float centerAlpha = textureAtlas.Sample(atlasSampler, input.uv).a;
        float2 negOffset = -offset;
        float maxN = 0.0;
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(negOffset.x, 0)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(offset.x, 0)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(0, negOffset.y)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(0, offset.y)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + negOffset).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(offset.x, negOffset.y)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + float2(negOffset.x, offset.y)).a);
        maxN = max(maxN, textureAtlas.Sample(atlasSampler, input.uv + offset).a);
        float outlineStrength = saturate(maxN - centerAlpha);
        float3 outColor = lerp(input.outlineColor.rgb, input.color.rgb, centerAlpha);
        float outAlpha = max(centerAlpha * input.color.a, outlineStrength * input.outlineColor.a);
        return float4(outColor * outAlpha, outAlpha);
    }
    float2 alphas = megaAtlas.Sample(atlasSampler, input.uv).rg;
    clip((alphas.r + alphas.g) - 0.003);
    float screenScale = length(fwidth(input.uv));
    float scaleFactor = saturate(screenScale * 10.0);
    float k = lerp(10.0, 22.0, scaleFactor);
    float a = alphas.g;
    float sharpened = 1.0 / (1.0 + exp(-k * (a - 0.5)));
    float blendAmount = lerp(0.3, 0.1, scaleFactor);
    float edgeBlend = smoothstep(0.0, 0.15, a) * smoothstep(1.0, 0.85, a);
    float textAlpha = lerp(sharpened, a, edgeBlend * blendAmount);
    float outlineStrength = lerp(1.0, 0.75, saturate(screenScale * 8.0));
    float outlineAlpha = alphas.r * outlineStrength;
    float3 textPremul    = input.color.rgb * input.color.a * textAlpha;
    float3 outlinePremul = input.outlineColor.rgb * input.outlineColor.a * outlineAlpha * (1.0 - textAlpha);
    float3 finalRGB  = textPremul + outlinePremul;
    float  finalAlpha = textAlpha * input.color.a + outlineAlpha * input.outlineColor.a * (1.0 - textAlpha);
    return float4(finalRGB, finalAlpha);
}
)";

    // ═══════════════════════════════════════════════════════════════════════
    // resolve_dynamic_imports
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::resolve_dynamic_imports( ) {
        auto h_d3dc = LoadLibraryA( "d3dcompiler_47.dll" );
        if ( !h_d3dc ) return false;
        m_pfn_d3d_compile = ( p_d3d_compile )GetProcAddress( h_d3dc, "D3DCompile" );
        if ( !m_pfn_d3d_compile ) return false;

        auto h_dw = LoadLibraryA( "dwrite.dll" );
        if ( !h_dw ) return false;
        m_pfn_dwrite_create_factory = ( p_dwrite_create_factory )GetProcAddress( h_dw, "DWriteCreateFactory" );
        if ( !m_pfn_dwrite_create_factory ) return false;

        auto h_ole = LoadLibraryA( "ole32.dll" );
        if ( !h_ole ) return false;
        m_pfn_co_initialize_ex = ( p_co_initialize_ex )GetProcAddress( h_ole, "CoInitializeEx" );
        m_pfn_co_create_instance = ( p_co_create_instance )GetProcAddress( h_ole, "CoCreateInstance" );
        if ( !m_pfn_co_initialize_ex || !m_pfn_co_create_instance ) return false;

        // D3D11On12CreateDevice lives in d3d11.dll (always available on Win10+).
        auto h_d3d11 = GetModuleHandleA( "d3d11.dll" );
        if ( !h_d3d11 ) h_d3d11 = LoadLibraryA( "d3d11.dll" );
        if ( h_d3d11 ) {
            m_pfn_d3d11on12 = ( p_d3d11on12_create_device )GetProcAddress(
                h_d3d11, "D3D11On12CreateDevice" );
        }
        // Not fatal if missing — only used in the DX12 path.

        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // init  (DX11 path — unchanged)
    // ═══════════════════════════════════════════════════════════════════════
    inline bool context_t::init( ID3D11Device* device, ID3D11DeviceContext* ctx, IDXGISwapChain* sc, bool skip_rtv ) {
        m_device = device;
        m_context = ctx;
        m_swap_chain = sc;

        m_device->AddRef( );
        m_context->AddRef( );
        m_swap_chain->AddRef( );

        memset( s_atlas_data, 0, k_text_atlas_bytes );
        memset( s_tex_atlas_data, 0, k_tex_atlas_bytes );
        m_atlas_packer.init( k_atlas_size, k_atlas_size );
        m_tex_atlas_packer.init( k_atlas_size, k_atlas_size );
        m_glyphs.clear( );
        m_fast_cache.invalidate( );
        m_textures.clear( );
        m_anim_textures.clear( );
        m_next_tex_id = 1;
        m_next_anim_id = 1;
        m_clip_stack_depth = 0;
        m_clip_next_index = 1;
        memset( m_clip_rects, 0, sizeof( m_clip_rects ) );
        memset( m_clip_index_stack, 0, sizeof( m_clip_index_stack ) );

        if ( !resolve_dynamic_imports( ) ) {
            log_info( oxorany( "init: resolve_dynamic_imports failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        if ( FAILED( m_pfn_dwrite_create_factory(
            DWRITE_FACTORY_TYPE_SHARED, __uuidof( IDWriteFactory ),
            reinterpret_cast< IUnknown** >( &m_dwrite_factory ) ) ) ) {
            log_info( oxorany( "init: DWriteCreateFactory failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        m_pfn_co_initialize_ex( nullptr, COINIT_MULTITHREADED );
        m_pfn_co_create_instance( CLSID_WICImagingFactory, nullptr,
            CLSCTX_INPROC_SERVER, IID_PPV_ARGS( &m_wic_factory ) );

        if ( !skip_rtv ) {
            if ( !create_render_target( ) ) {
                log_info( oxorany( "init: create_render_target failed" ), oxorany( "Init Step" ), MB_ICONHAND );
                return false;
            }
        }
        else {
            DXGI_SWAP_CHAIN_DESC desc = {};
            if ( SUCCEEDED( sc->GetDesc( &desc ) ) ) {
                m_screen_width = ( float )desc.BufferDesc.Width;
                m_screen_height = ( float )desc.BufferDesc.Height;
            }
        }

        if ( !create_shaders( ) ) {
            log_info( oxorany( "init: create_shaders failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        if ( !create_pipeline_state( ) ) {
            log_info( oxorany( "init: create_pipeline_state failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        if ( !create_buffers( ) ) {
            log_info( oxorany( "init: create_buffers failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        if ( !create_atlases( ) ) {
            log_info( oxorany( "init: create_atlases failed" ), oxorany( "Init Step" ), MB_ICONHAND );
            return false;
        }

        m_cb_dirty = true;
        return true;
    }

    inline bool context_t::init_dx12( ID3D12Device* device12,
        ID3D12CommandQueue* queue,
        IDXGISwapChain* swapchain ) {

        if ( !device12 || !queue || !swapchain ) {
            log_info(
                oxorany( "init_dx12 called with null parameter.\n\n"
                "device12, queue, and swapchain must all be non-null." ),
                oxorany( "DX12 Init: Null parameter" ), MB_ICONHAND );
            return false;
        }

        D3D12_COMMAND_QUEUE_DESC qdesc{};
        __try {
            qdesc = queue->GetDesc();
        }
        __except ( 1 ) {
            log_info(
                oxorany( "Failed to query command queue descriptor.\n\n"
                "The queue pointer is likely invalid." ),
                oxorany( "DX12 Init: Invalid command queue" ), MB_ICONHAND );
            return false;
        }

        if ( qdesc.Type != D3D12_COMMAND_LIST_TYPE_DIRECT ) {
            log_info(
                oxorany( "Command queue is not DIRECT.\n\n"
                "D3D11On12 requires the graphics (DIRECT) queue." ),
                oxorany( "DX12 Init: Wrong queue type" ), MB_ICONHAND );
            return false;
        }

        ID3D12Device* queue_device = nullptr;
        HRESULT qhr = queue->GetDevice( IID_PPV_ARGS( &queue_device ) );
        if ( FAILED( qhr ) || !queue_device ) {
            char qbuf[ 128 ];
            sprintf_s( qbuf, oxorany( "Failed to get queue device.\n\nHRESULT: 0x%08X" ), qhr );
            log_info( qbuf, oxorany( "DX12 Init: Queue device lookup failed" ), MB_ICONHAND );
            return false;
        }

        if ( queue_device != device12 ) {
            queue_device->Release();
            log_info(
                oxorany( "Command queue device does not match swapchain device.\n\n"
                "Using a queue from a different device can deadlock/hang." ),
                oxorany( "DX12 Init: Queue/device mismatch" ), MB_ICONHAND );
            return false;
        }
        queue_device->Release();

        if ( !m_pfn_d3d11on12 ) {
            auto h = GetModuleHandleA( "d3d11.dll" );
            if ( !h ) h = LoadLibraryA( "d3d11.dll" );
            if ( !h ) {
                log_info(
                    oxorany( "Failed to load d3d11.dll.\n\n"
                    "This is required for D3D11On12 interop." ),
                    oxorany( "DX12 Init: d3d11.dll not found" ), MB_ICONHAND );
                return false;
            }
            m_pfn_d3d11on12 = ( p_d3d11on12_create_device )GetProcAddress( h, "D3D11On12CreateDevice" );
            if ( !m_pfn_d3d11on12 ) {
                log_info(
                    oxorany( "Failed to resolve D3D11On12CreateDevice from d3d11.dll.\n\n"
                    "Your Windows version may not support D3D11On12 interop." ),
                    oxorany( "DX12 Init: D3D11On12CreateDevice not found" ), MB_ICONHAND );
                return false;
            }
        }

        ID3D11Device* dev11 = nullptr;
        ID3D11DeviceContext* ctx11 = nullptr;
        IUnknown* queues[ ] = { static_cast< IUnknown* >( queue ) };

        HRESULT hr = m_pfn_d3d11on12(
            device12,
            D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr, 0,
            queues, 1,
            0,
            &dev11, &ctx11, nullptr );

        if ( FAILED( hr ) ) {
            char buf[ 128 ];
            sprintf_s( buf, oxorany( "D3D11On12CreateDevice failed.\n\nHRESULT: 0x%08X" ), hr );
            log_info( buf, oxorany( "DX12 Init: 11On12 device creation failed" ), MB_ICONHAND );
            return false;
        }

        hr = dev11->QueryInterface( IID_PPV_ARGS( &m_11on12 ) );
        if ( FAILED( hr ) ) {
            char buf[ 128 ];
            sprintf_s( buf, oxorany( "QueryInterface for ID3D11On12Device failed.\n\nHRESULT: 0x%08X" ), hr );
            log_info( buf, oxorany( "DX12 Init: ID3D11On12Device QI failed" ), MB_ICONHAND );
            dev11->Release( ); ctx11->Release( );
            return false;
        }

        hr = swapchain->QueryInterface( IID_PPV_ARGS( &m_swapchain3 ) );
        if ( FAILED( hr ) ) {
            char buf[ 128 ];
            sprintf_s( buf, oxorany( "QueryInterface for IDXGISwapChain3 failed.\n\nHRESULT: 0x%08X" ), hr );
            log_info( buf, oxorany( "DX12 Init: IDXGISwapChain3 QI failed" ), MB_ICONHAND );
            m_11on12->Release( ); m_11on12 = nullptr;
            dev11->Release( ); ctx11->Release( );
            return false;
        }

        m_cmd_queue = queue;
        m_using_dx12 = true;

        // ── 4. Run the normal DX11 init ───────────────────────────────────
        if ( !init( dev11, ctx11, swapchain, true ) ) {
            log_info(
                oxorany( "Underlying DX11 init() failed during DX12 path.\n\n"
                "Shader compilation, atlas creation, or pipeline state setup failed.\n"
                "Check that d3dcompiler_47.dll and dwrite.dll are available." ),
                oxorany( "DX12 Init: DX11 base init failed" ), MB_ICONHAND );
            dev11->Release( ); ctx11->Release( );
            m_11on12->Release( );     m_11on12 = nullptr;
            m_swapchain3->Release( ); m_swapchain3 = nullptr;
            m_cmd_queue = nullptr;
            m_using_dx12 = false;
            return false;
        }

        dev11->Release( );
        ctx11->Release( );

        if ( m_rtv ) { m_rtv->Release( ); m_rtv = nullptr; }

        if ( !dx12_wrap_backbuffers( ) ) {
            log_info(
                oxorany( "dx12_wrap_backbuffers() failed.\n\n"
                "Could not wrap one or more swapchain back-buffers.\n"
                "Ensure the swapchain has been fully initialized by Discord before injecting." ),
                oxorany( "DX12 Init: Back-buffer wrap failed" ), MB_ICONHAND );
            dx12_release_backbuffers( );
            shutdown( );
            m_11on12->Release( );     m_11on12 = nullptr;
            m_swapchain3->Release( ); m_swapchain3 = nullptr;
            m_cmd_queue = nullptr;
            m_using_dx12 = false;
            return false;
        }

        if ( m_wrapped_rtvs[ 0 ] ) {
            m_rtv = m_wrapped_rtvs[ 0 ];
            m_rtv->AddRef( );
        }

        return true;
    }

    // ── dx12_wrap_backbuffers ─────────────────────────────────────────────
    // Enumerates swapchain buffers, gets each as ID3D12Resource, creates a
    // D3D11On12 wrapped resource for it, then creates a DX11 RTV on top.
    inline bool context_t::dx12_wrap_backbuffers( ) {
        DXGI_SWAP_CHAIN_DESC sc_desc = {};
        HRESULT hr = m_swapchain3->GetDesc( &sc_desc );
        if ( FAILED( hr ) ) {
            char buf[ 128 ];
            sprintf_s( buf, oxorany( "dx12_wrap_backbuffers: GetDesc failed.\n\nHRESULT: 0x%08X" ), hr );
            log_info( buf, oxorany( "DX12 Wrap: GetDesc failed" ), MB_ICONHAND );
            return false;
        }

        m_bb_count = ( int )sc_desc.BufferCount;
        if ( m_bb_count > k_max_backbuffers ) {
            char buf[ 160 ];
            sprintf_s( buf,
                oxorany( "dx12_wrap_backbuffers: swapchain buffer count (%d) exceeds supported max (%d)." ),
                m_bb_count, k_max_backbuffers );
            log_info( buf, oxorany( "DX12 Wrap: Unsupported buffer count" ), MB_ICONHAND );
            return false;
        }

        D3D11_RESOURCE_FLAGS rf = { D3D11_BIND_RENDER_TARGET };

        for ( int i = 0; i < m_bb_count; ++i ) {
            hr = m_swapchain3->GetBuffer( i, IID_PPV_ARGS( &m_dx12_bbs[ i ] ) );
            if ( FAILED( hr ) ) {
                char buf[ 128 ];
                sprintf_s( buf, oxorany( "dx12_wrap_backbuffers: GetBuffer(%d) failed.\n\nHRESULT: 0x%08X" ), i, hr );
                log_info( buf, oxorany( "DX12 Wrap: GetBuffer failed" ), MB_ICONHAND );
                return false;
            }

            hr = m_11on12->CreateWrappedResource(
                m_dx12_bbs[ i ], &rf,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PRESENT,
                IID_PPV_ARGS( &m_wrapped_bbs[ i ] ) );
            if ( FAILED( hr ) ) {
                char buf[ 128 ];
                sprintf_s( buf, oxorany( "dx12_wrap_backbuffers: CreateWrappedResource(%d) failed.\n\nHRESULT: 0x%08X" ), i, hr );
                log_info( buf, oxorany( "DX12 Wrap: CreateWrappedResource failed" ), MB_ICONHAND );
                return false;
            }

            ID3D11Texture2D* tex = nullptr;
            hr = m_wrapped_bbs[ i ]->QueryInterface( IID_PPV_ARGS( &tex ) );
            if ( FAILED( hr ) ) {
                char buf[ 128 ];
                sprintf_s( buf, oxorany( "dx12_wrap_backbuffers: QI for ID3D11Texture2D(%d) failed.\n\nHRESULT: 0x%08X" ), i, hr );
                log_info( buf, oxorany( "DX12 Wrap: Texture2D QI failed" ), MB_ICONHAND );
                return false;
            }

            hr = m_device->CreateRenderTargetView( tex, nullptr, &m_wrapped_rtvs[ i ] );
            tex->Release( );
            if ( FAILED( hr ) ) {
                char buf[ 128 ];
                sprintf_s( buf, oxorany( "dx12_wrap_backbuffers: CreateRenderTargetView(%d) failed.\n\nHRESULT: 0x%08X" ), i, hr );
                log_info( buf, oxorany( "DX12 Wrap: CreateRTV failed" ), MB_ICONHAND );
                return false;
            }
        }

        m_screen_width = ( float )sc_desc.BufferDesc.Width;
        m_screen_height = ( float )sc_desc.BufferDesc.Height;
        m_cb_dirty = true;
        return true;
    }
    // ── dx12_release_backbuffers ──────────────────────────────────────────
    inline void context_t::dx12_release_backbuffers( ) {
        for ( int i = 0; i < k_max_backbuffers; ++i ) {
            if ( m_wrapped_rtvs[ i ] ) { m_wrapped_rtvs[ i ]->Release( ); m_wrapped_rtvs[ i ] = nullptr; }
            if ( m_wrapped_bbs[ i ] ) { m_wrapped_bbs[ i ]->Release( ); m_wrapped_bbs[ i ] = nullptr; }
            if ( m_dx12_bbs[ i ] ) { m_dx12_bbs[ i ]->Release( ); m_dx12_bbs[ i ] = nullptr; }
        }
        m_bb_count = 0;
    }

    // ── dx12_handle_resize ────────────────────────────────────────────────
    // Called from begin_frame_dx12 when swapchain dimensions have changed.
    inline void context_t::dx12_handle_resize( ) {
        if ( !m_device || !m_context || !m_11on12 ) return;

        // Validate the device hasn't been lost
        if ( m_device->GetDeviceRemovedReason( ) != S_OK ) {
            shutdown( );
            return;
        }

        // Flush before touching resources.
        m_context->Flush( );

        // Ensure no wrapped resource remains acquired across resize.
        if ( m_dx12_resource_acquired &&
            ( int )m_dx12_acquired_bb < m_bb_count &&
            m_wrapped_bbs[ m_dx12_acquired_bb ] ) {
            ID3D11Resource* res = m_wrapped_bbs[ m_dx12_acquired_bb ];
            m_11on12->ReleaseWrappedResources( &res, 1 );
            m_dx12_resource_acquired = false;
            m_dx12_acquired_bb = ~0u;
        }

        // Release the override RTV we hold in m_rtv (points into wrapped_rtvs).
        if ( m_rtv ) { m_rtv->Release( ); m_rtv = nullptr; }

        dx12_release_backbuffers( );

        // Swapchain has already been resized by the game — re-wrap.
        if ( !dx12_wrap_backbuffers( ) ) {
            m_bb_count = 0;
            m_dx12_frame_ready = false;
            return;
        }

        // Restore m_rtv to buffer 0 as a safe fallback.
        if ( m_wrapped_rtvs[ 0 ] ) {
            m_rtv = m_wrapped_rtvs[ 0 ];
            m_rtv->AddRef( );
        }

        m_state_initialized = false;   // force viewport / pipeline re-bind
    }

    // ═══════════════════════════════════════════════════════════════════════
    // begin_frame  (DX11 — unchanged)
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::begin_frame( color_t clear_color ) {
        DXGI_SWAP_CHAIN_DESC desc;
        m_swap_chain->GetDesc( &desc );
        if ( m_screen_width != ( float )desc.BufferDesc.Width ||
            m_screen_height != ( float )desc.BufferDesc.Height )
            create_render_target( );

        m_clip_stack_depth = 0;
        m_clip_next_index = 1;

        if ( clear_color.a > 0 ) {
            float c[ 4 ] = {
                clear_color.r / 255.f, clear_color.g / 255.f,
                clear_color.b / 255.f, clear_color.a / 255.f
            };
            m_context->ClearRenderTargetView( m_rtv, c );
        }

        m_context->OMSetRenderTargets( 1, &m_rtv, nullptr );
    }

    // ═══════════════════════════════════════════════════════════════════════
    // begin_frame_dx12  (new — must be used instead of begin_frame on DX12)
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::begin_frame_dx12( color_t clear_color ) {
        m_dx12_frame_ready = false;
        if ( !m_using_dx12 || !m_swapchain3 || !m_11on12 ) {
            // Fallback: called on wrong path — just do the DX11 version.
            begin_frame( clear_color );
            return;
        }

        // ── Check resize ──────────────────────────────────────────────────
        DXGI_SWAP_CHAIN_DESC sc_desc = {};
        m_swapchain3->GetDesc( &sc_desc );
        if ( m_screen_width != ( float )sc_desc.BufferDesc.Width ||
            m_screen_height != ( float )sc_desc.BufferDesc.Height ) {
            dx12_handle_resize( );
            if ( m_bb_count <= 0 )
                return;
        }

        // ── Get the current back-buffer index ─────────────────────────────
        m_current_bb = m_swapchain3->GetCurrentBackBufferIndex( );
        if ( ( int )m_current_bb >= m_bb_count ) return;

        ID3D12Resource* live_bb = nullptr;
        HRESULT bb_hr = m_swapchain3->GetBuffer( m_current_bb, IID_PPV_ARGS( &live_bb ) );
        if ( FAILED( bb_hr ) || !live_bb ) {
            dx12_handle_resize( );
            return;
        }
        bool bb_changed = ( live_bb != m_dx12_bbs[ m_current_bb ] );
        live_bb->Release( );
        if ( bb_changed ) {
            dx12_handle_resize( );
            if ( m_bb_count <= 0 )
                return;

            m_current_bb = m_swapchain3->GetCurrentBackBufferIndex( );
            if ( ( int )m_current_bb >= m_bb_count )
                return;
        }


        // Recover from a missed flush path by force-releasing previous acquire.
        if ( m_dx12_resource_acquired &&
            ( int )m_dx12_acquired_bb < m_bb_count &&
            m_wrapped_bbs[ m_dx12_acquired_bb ] ) {
            ID3D11Resource* stale = m_wrapped_bbs[ m_dx12_acquired_bb ];
            m_11on12->ReleaseWrappedResources( &stale, 1 );
            m_dx12_resource_acquired = false;
            m_dx12_acquired_bb = ~0u;
        }

        // ── Transition: PRESENT → RENDER_TARGET ───────────────────────────
        // AcquireWrappedResources issues the resource barrier internally.
        ID3D11Resource* res = m_wrapped_bbs[ m_current_bb ];
        m_11on12->AcquireWrappedResources( &res, 1 );
        m_dx12_resource_acquired = true;
        m_dx12_acquired_bb = m_current_bb;

        // ── Redirect the active RTV ───────────────────────────────────────
        if ( m_rtv ) { m_rtv->Release( ); m_rtv = nullptr; }
        m_rtv = m_wrapped_rtvs[ m_current_bb ];
        m_rtv->AddRef( );

        // ── Reset clip state ──────────────────────────────────────────────
        m_clip_stack_depth = 0;
        m_clip_next_index = 1;

        if ( clear_color.a > 0 ) {
            float c[ 4 ] = {
                clear_color.r / 255.f, clear_color.g / 255.f,
                clear_color.b / 255.f, clear_color.a / 255.f
            };
            m_context->ClearRenderTargetView( m_rtv, c );
        }

        m_context->OMSetRenderTargets( 1, &m_rtv, nullptr );
        m_dx12_frame_ready = true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // shutdown
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::shutdown( ) {
        // Release the per-frame vertex buffers and fences.
        for ( auto i = 0; i < 3; i++ ) {
            if ( m_frames[ i ].vertex_buffer ) { m_frames[ i ].vertex_buffer->Release( ); m_frames[ i ].vertex_buffer = nullptr; }
            if ( m_frames[ i ].fence ) { m_frames[ i ].fence->Release( );          m_frames[ i ].fence = nullptr; }
        }

        auto safe_release = [ ] ( auto*& p ) { if ( p ) { p->Release( ); p = nullptr; } };

        // If we're in DX12 mode, clean up the wrapped resources first so the
        // RTV we release below doesn't double-free.
        if ( m_using_dx12 ) {
            if ( m_dx12_resource_acquired &&
                ( int )m_dx12_acquired_bb < m_bb_count &&
                m_wrapped_bbs[ m_dx12_acquired_bb ] ) {
                ID3D11Resource* res = m_wrapped_bbs[ m_dx12_acquired_bb ];
                m_11on12->ReleaseWrappedResources( &res, 1 );
                m_dx12_resource_acquired = false;
                m_dx12_acquired_bb = ~0u;
            }
            dx12_release_backbuffers( );
            safe_release( m_11on12 );
            safe_release( m_swapchain3 );
            m_cmd_queue = nullptr;
            m_using_dx12 = false;
        }

        safe_release( m_rtv );
        safe_release( m_atlas_texture );      safe_release( m_atlas_srv );
        safe_release( m_tex_atlas_texture );  safe_release( m_tex_atlas_srv );
        safe_release( m_unified_vs );  safe_release( m_unified_ps );  safe_release( m_unified_layout );
        safe_release( m_screen_size_cb );     safe_release( m_clip_rects_cb );
        safe_release( m_blend_state );        safe_release( m_rasterizer_state );
        safe_release( m_rasterizer_state_scissor );
        safe_release( m_depth_stencil_state );
        safe_release( m_sampler_state );      safe_release( m_sampler_state_aa );
        safe_release( m_dwrite_factory );     safe_release( m_wic_factory );

        // Release the device/context last (init() AddRef'd them).
        safe_release( m_context );
        safe_release( m_swap_chain );
        safe_release( m_device );
    }

    // ═══════════════════════════════════════════════════════════════════════
    // create_render_target
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::create_render_target( ) {
        if ( m_rtv ) { m_rtv->Release( ); m_rtv = nullptr; }

        ID3D11Texture2D* bb = nullptr;
        if ( FAILED( m_swap_chain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( void** )&bb ) ) )
            return false;

        D3D11_TEXTURE2D_DESC desc;
        bb->GetDesc( &desc );
        m_screen_width = ( float )desc.Width;
        m_screen_height = ( float )desc.Height;

        auto hr = m_device->CreateRenderTargetView( bb, nullptr, &m_rtv );
        bb->Release( );
        if ( FAILED( hr ) ) return false;

        m_cb_dirty = true;
        m_state_initialized = false;
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // create_shaders
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::create_shaders( ) {
        const auto flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3;
        ID3DBlob* vs_blob = nullptr;
        ID3DBlob* ps_blob = nullptr;
        ID3DBlob* err_blob = nullptr;

        auto hr = m_pfn_d3d_compile( g_unified_vs, mm_strlen( g_unified_vs ),
            nullptr, nullptr, nullptr, "main", "vs_5_0", flags, 0, &vs_blob, &err_blob );
        if ( FAILED( hr ) ) { if ( err_blob ) err_blob->Release( ); return false; }
        if ( err_blob ) { err_blob->Release( ); err_blob = nullptr; }

        hr = m_pfn_d3d_compile( g_unified_ps, mm_strlen( g_unified_ps ),
            nullptr, nullptr, nullptr, "main", "ps_5_0", flags, 0, &ps_blob, &err_blob );
        if ( FAILED( hr ) ) {
            if ( err_blob ) err_blob->Release( );
            vs_blob->Release( ); return false;
        }
        if ( err_blob ) { err_blob->Release( ); err_blob = nullptr; }

        hr = m_device->CreateVertexShader( vs_blob->GetBufferPointer( ), vs_blob->GetBufferSize( ), nullptr, &m_unified_vs );
        if ( FAILED( hr ) ) { vs_blob->Release( ); ps_blob->Release( ); return false; }

        hr = m_device->CreatePixelShader( ps_blob->GetBufferPointer( ), ps_blob->GetBufferSize( ), nullptr, &m_unified_ps );
        if ( FAILED( hr ) ) { vs_blob->Release( ); ps_blob->Release( ); return false; }

        D3D11_INPUT_ELEMENT_DESC layout[ ] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 8,  D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    0, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR",    1, DXGI_FORMAT_B8G8R8A8_UNORM,  0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 1, DXGI_FORMAT_R32_UINT,        0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
        };
        hr = m_device->CreateInputLayout( layout, 5,
            vs_blob->GetBufferPointer( ), vs_blob->GetBufferSize( ), &m_unified_layout );
        vs_blob->Release( ); ps_blob->Release( );
        return SUCCEEDED( hr );
    }

    // ═══════════════════════════════════════════════════════════════════════
    // create_pipeline_state
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::create_pipeline_state( ) {
        D3D11_BLEND_DESC blend_desc = {};
        blend_desc.RenderTarget[ 0 ].BlendEnable = TRUE;
        blend_desc.RenderTarget[ 0 ].SrcBlend = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[ 0 ].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
        blend_desc.RenderTarget[ 0 ].BlendOp = D3D11_BLEND_OP_ADD;
        blend_desc.RenderTarget[ 0 ].SrcBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[ 0 ].DestBlendAlpha = D3D11_BLEND_ONE;
        blend_desc.RenderTarget[ 0 ].BlendOpAlpha = D3D11_BLEND_OP_MAX;
        blend_desc.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        if ( FAILED( m_device->CreateBlendState( &blend_desc, &m_blend_state ) ) ) return false;

        D3D11_RASTERIZER_DESC rast_desc = {};
        rast_desc.FillMode = D3D11_FILL_SOLID;
        rast_desc.CullMode = D3D11_CULL_NONE;
        rast_desc.ScissorEnable = FALSE;
        if ( FAILED( m_device->CreateRasterizerState( &rast_desc, &m_rasterizer_state ) ) ) return false;
        rast_desc.ScissorEnable = TRUE;
        if ( FAILED( m_device->CreateRasterizerState( &rast_desc, &m_rasterizer_state_scissor ) ) ) return false;

        D3D11_DEPTH_STENCIL_DESC ds_desc = {};
        ds_desc.DepthEnable = FALSE;
        ds_desc.StencilEnable = FALSE;
        if ( FAILED( m_device->CreateDepthStencilState( &ds_desc, &m_depth_stencil_state ) ) ) return false;

        D3D11_SAMPLER_DESC samp_desc = {};
        samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
        samp_desc.AddressU = samp_desc.AddressV = samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
        if ( FAILED( m_device->CreateSamplerState( &samp_desc, &m_sampler_state ) ) ) return false;
        samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        if ( FAILED( m_device->CreateSamplerState( &samp_desc, &m_sampler_state_aa ) ) ) return false;

        D3D11_BUFFER_DESC cb_desc = {};
        cb_desc.ByteWidth = 16;
        cb_desc.Usage = D3D11_USAGE_DYNAMIC;
        cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if ( FAILED( m_device->CreateBuffer( &cb_desc, nullptr, &m_screen_size_cb ) ) ) return false;

        cb_desc.ByteWidth = k_max_clip_rects * 16;
        if ( FAILED( m_device->CreateBuffer( &cb_desc, nullptr, &m_clip_rects_cb ) ) ) return false;

        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // create_buffers
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::create_buffers( ) {
        for ( auto i = 0; i < 3; i++ ) {
            D3D11_BUFFER_DESC desc = {};
            desc.ByteWidth = k_max_vertices * sizeof( unified_vertex_t );
            desc.Usage = D3D11_USAGE_DYNAMIC;
            desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            if ( FAILED( m_device->CreateBuffer( &desc, nullptr, &m_frames[ i ].vertex_buffer ) ) ) return false;

            m_frames[ i ].write_offset = 0;
            m_frames[ i ].gpu_busy = false;

            D3D11_QUERY_DESC qd = {}; qd.Query = D3D11_QUERY_EVENT;
            if ( FAILED( m_device->CreateQuery( &qd, &m_frames[ i ].fence ) ) ) return false;
        }
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // create_atlases
    // ═══════════════════════════════════════════════════════════════════════

    inline bool context_t::create_atlases( ) {
        {
            D3D11_TEXTURE2D_DESC td = {};
            td.Width = td.Height = k_atlas_size; td.MipLevels = td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R8G8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DYNAMIC;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SUBRESOURCE_DATA init = { s_atlas_data, k_atlas_size * 2, 0 };
            if ( FAILED( m_device->CreateTexture2D( &td, &init, &m_atlas_texture ) ) ) return false;
            if ( FAILED( m_device->CreateShaderResourceView( m_atlas_texture, nullptr, &m_atlas_srv ) ) ) return false;
        }
        {
            D3D11_TEXTURE2D_DESC td = {};
            td.Width = td.Height = k_atlas_size; td.MipLevels = td.ArraySize = 1;
            td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            td.SampleDesc.Count = 1;
            td.Usage = D3D11_USAGE_DYNAMIC;
            td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            D3D11_SUBRESOURCE_DATA init = { s_tex_atlas_data, k_atlas_size * 4, 0 };
            if ( FAILED( m_device->CreateTexture2D( &td, &init, &m_tex_atlas_texture ) ) ) return false;
            if ( FAILED( m_device->CreateShaderResourceView( m_tex_atlas_texture, nullptr, &m_tex_atlas_srv ) ) ) return false;
        }
        return true;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // flush  (DX11 — unchanged)
    // ═══════════════════════════════════════════════════════════════════════

    __declspec( noinline ) inline void context_t::flush( ) {
        auto& frame = m_frames[ m_current_frame ];
        if ( !frame.write_offset ) return;

        {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if ( FAILED( m_context->Map( frame.vertex_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) )
                return;
            memcpy( mapped.pData, s_staging_buffer, frame.write_offset * sizeof( unified_vertex_t ) );
            m_context->Unmap( frame.vertex_buffer, 0 );
        }

        if ( m_atlas_needs_upload ) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if ( SUCCEEDED( m_context->Map( m_atlas_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) {
                memcpy( mapped.pData, s_atlas_data, k_text_atlas_bytes );
                m_context->Unmap( m_atlas_texture, 0 );
                m_atlas_needs_upload = false;
            }
        }
        if ( m_tex_atlas_needs_upload ) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if ( SUCCEEDED( m_context->Map( m_tex_atlas_texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) {
                memcpy( mapped.pData, s_tex_atlas_data, k_tex_atlas_bytes );
                m_context->Unmap( m_tex_atlas_texture, 0 );
                m_tex_atlas_needs_upload = false;
            }
        }
        if ( m_cb_dirty ) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if ( SUCCEEDED( m_context->Map( m_screen_size_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) {
                ( ( float* )mapped.pData )[ 0 ] = 1.f / m_screen_width;
                ( ( float* )mapped.pData )[ 1 ] = 1.f / m_screen_height;
                m_context->Unmap( m_screen_size_cb, 0 );
                m_cb_dirty = false;
            }
        }
        if ( m_clip_rects_dirty && m_clip_next_index > 1 ) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            if ( SUCCEEDED( m_context->Map( m_clip_rects_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped ) ) ) {
                auto* dst = ( float* )mapped.pData;
                for ( auto i = 0; i < m_clip_next_index; i++ ) {
                    dst[ i * 4 + 0 ] = m_clip_rects[ i ].x; dst[ i * 4 + 1 ] = m_clip_rects[ i ].y;
                    dst[ i * 4 + 2 ] = m_clip_rects[ i ].w; dst[ i * 4 + 3 ] = m_clip_rects[ i ].h;
                }
                m_context->Unmap( m_clip_rects_cb, 0 );
                m_clip_rects_dirty = false;
            }
        }

        m_context->OMSetBlendState( m_blend_state, nullptr, 0xFFFFFFFF );
        m_context->OMSetDepthStencilState( m_depth_stencil_state, 0 );
        m_context->IASetInputLayout( m_unified_layout );
        m_context->VSSetShader( m_unified_vs, nullptr, 0 );
        m_context->PSSetShader( m_unified_ps, nullptr, 0 );
        m_context->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

        ID3D11ShaderResourceView* srvs[ 2 ] = { m_atlas_srv, m_tex_atlas_srv };
        m_context->PSSetShaderResources( 0, 2, srvs );
        m_context->VSSetConstantBuffers( 0, 1, &m_screen_size_cb );
        m_context->PSSetConstantBuffers( 1, 1, &m_clip_rects_cb );

        auto* samp = m_text_aa ? m_sampler_state_aa : m_sampler_state;
        m_context->PSSetSamplers( 0, 1, &samp );
        m_text_aa_dirty = false;

        m_context->RSSetState( m_scissor_enabled ? m_rasterizer_state_scissor : m_rasterizer_state );
        if ( m_scissor_enabled ) m_context->RSSetScissorRects( 1, &m_scissor_rect );

        D3D11_VIEWPORT vp = { 0, 0, m_screen_width, m_screen_height, 0, 1 };
        m_context->RSSetViewports( 1, &vp );

        auto stride = ( UINT )sizeof( unified_vertex_t ), offset = ( UINT )0;
        m_context->IASetVertexBuffers( 0, 1, &frame.vertex_buffer, &stride, &offset );
        m_context->Draw( frame.write_offset, 0 );
        m_completed_frame_vertex_count = frame.write_offset;

        m_context->End( frame.fence );
        frame.gpu_busy = true;
        frame.write_offset = 0;

        m_current_frame = ( m_current_frame + 1 ) % 3;
        m_frames[ m_current_frame ].write_offset = 0;
        m_frames[ m_current_frame ].gpu_busy = false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // flush_dx12  (new — wraps flush() with 11on12 acquire/release cycle)
    // ═══════════════════════════════════════════════════════════════════════

    __declspec( noinline ) inline void context_t::flush_dx12( ) {
        if ( !m_using_dx12 || !m_11on12 ) {
            flush( );
            return;
        }

        if ( !m_dx12_frame_ready )
            return;

        // Cache BEFORE flush() advances m_current_frame
        UINT bb_to_release = m_current_bb;

        flush( );

        // Use cached index, not m_current_bb which is still correct here
        // but be defensive in case begin_frame_dx12 wasn't called
        if ( m_dx12_resource_acquired &&
            ( int )bb_to_release < m_bb_count &&
            m_wrapped_bbs[ bb_to_release ] ) {
            ID3D11Resource* res = m_wrapped_bbs[ bb_to_release ];
            m_11on12->ReleaseWrappedResources( &res, 1 );
            m_dx12_resource_acquired = false;
            m_dx12_acquired_bb = ~0u;
        }

        m_context->Flush( );
        m_dx12_frame_ready = false;
    }

    inline float context_t::get_vertex_usage_percent( ) const {
        return ( float )m_completed_frame_vertex_count / ( float )k_max_vertices * 100.f;
    }


    // ═══════════════════════════════════════════════════════════════════════
    // UTF-8 / emoji helpers
    // ═══════════════════════════════════════════════════════════════════════

    static uint32_t decode_utf8( const char*& text ) {
        const auto* p = ( const uint8_t* )text;
        auto cp = 0u;
        if ( ( *p & 0x80 ) == 0 ) { cp = *p;                                                                                                               text += 1; }
        else if ( ( *p & 0xE0 ) == 0xC0 ) { cp = ( ( *p & 0x1F ) << 6 ) | ( p[ 1 ] & 0x3F );                                                                        text += 2; }
        else if ( ( *p & 0xF0 ) == 0xE0 ) { cp = ( ( *p & 0x0F ) << 12 ) | ( ( p[ 1 ] & 0x3F ) << 6 ) | ( p[ 2 ] & 0x3F );                                            text += 3; }
        else if ( ( *p & 0xF8 ) == 0xF0 ) { cp = ( ( *p & 0x07 ) << 18 ) | ( ( p[ 1 ] & 0x3F ) << 12 ) | ( ( p[ 2 ] & 0x3F ) << 6 ) | ( p[ 3 ] & 0x3F );               text += 4; }
        else { cp = '?'; text += 1; }
        return cp;
    }

    static bool is_emoji_codepoint( uint32_t cp ) {
        if ( cp >= 0x1F600 && cp <= 0x1F64F ) return true;
        if ( cp >= 0x1F300 && cp <= 0x1F5FF ) return true;
        if ( cp >= 0x1F680 && cp <= 0x1F6FF ) return true;
        if ( cp >= 0x1F900 && cp <= 0x1F9FF ) return true;
        if ( cp >= 0x2600 && cp <= 0x26FF ) return true;
        if ( cp >= 0x2700 && cp <= 0x27BF ) return true;
        return false;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // rasterize_glyph
    // ═══════════════════════════════════════════════════════════════════════


    inline void context_t::rasterize_glyph(
        const glyph_key_t& key,
        int& out_w, int& out_h,
        uint8_t* out_data, size_t out_data_size,
        ABC& out_abc, TEXTMETRICA& out_tm,
        int& out_bounds_left, int& out_bounds_top,
        float& out_advance_width ) {

        out_bounds_left = out_bounds_top = 0;
        out_advance_width = 0.f;
        out_w = out_h = 0;

        if ( !m_dwrite_factory ) return;

        const auto actual_outline = key.outline_thickness * 0.25f;
        const auto actual_size = key.font_size * 0.25f;

        wchar_t font_name_w[ 128 ] = {};
        mm_mbstowcs( font_name_w, key.font_name, 128 );

        // ── Text format (needed for rendering params only) ────────────────────
        IDWriteTextFormat* text_format = nullptr;
        if ( FAILED( m_dwrite_factory->CreateTextFormat(
            font_name_w, nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
            actual_size, L"", &text_format ) ) ) return;

        // ── Font face resolution ──────────────────────────────────────────────
        IDWriteFontFace* font_face = nullptr;
        IDWriteFont* font_local = nullptr;
        IDWriteFontFamily* family_local = nullptr;
        IDWriteFontCollection* coll_local = nullptr;

        auto* registered_face = peach::font::get_face( key.font_name );
        if ( registered_face ) {
            font_face = registered_face;
            font_face->AddRef( );
        }
        else {
            IDWriteFontCollection* fc = nullptr;
            if ( FAILED( m_dwrite_factory->GetSystemFontCollection( &fc, true ) ) ) {
                text_format->Release( );
                return;
            }

            UINT32 font_index = 0;
            BOOL   exists = FALSE;
            fc->FindFamilyName( font_name_w, &font_index, &exists );
            if ( !exists ) font_index = 0;

            IDWriteFontFamily* ff = nullptr;
            if ( FAILED( fc->GetFontFamily( font_index, &ff ) ) ) {
                fc->Release( );
                text_format->Release( );
                return;
            }

            IDWriteFont* fn = nullptr;
            if ( FAILED( ff->GetFirstMatchingFont(
                DWRITE_FONT_WEIGHT_NORMAL,
                DWRITE_FONT_STRETCH_NORMAL,
                DWRITE_FONT_STYLE_NORMAL, &fn ) ) ) {
                ff->Release( );
                fc->Release( );
                text_format->Release( );
                return;
            }

            if ( FAILED( fn->CreateFontFace( &font_face ) ) ) {
                fn->Release( );
                ff->Release( );
                fc->Release( );
                text_format->Release( );
                return;
            }

            font_local = fn;
            family_local = ff;
            coll_local = fc;
        }

        // ── Unified cleanup lambda ────────────────────────────────────────────
        auto cleanup = [ & ] ( ) {
            if ( font_face ) { font_face->Release( );    font_face = nullptr; }
            if ( font_local ) { font_local->Release( );   font_local = nullptr; }
            if ( family_local ) { family_local->Release( ); family_local = nullptr; }
            if ( coll_local ) { coll_local->Release( );   coll_local = nullptr; }
            if ( text_format ) { text_format->Release( );  text_format = nullptr; }
            };

        // ── Glyph index lookup ────────────────────────────────────────────────
        UINT16 glyph_index = 0;
        UINT32 cp32 = key.char_code;
        if ( FAILED( font_face->GetGlyphIndicesW( &cp32, 1, &glyph_index ) ) || glyph_index == 0 ) {
            cleanup( );
            return;
        }

        // ── Metrics ───────────────────────────────────────────────────────────
        DWRITE_FONT_METRICS font_metrics = {};
        font_face->GetMetrics( &font_metrics );

        DWRITE_GLYPH_METRICS glyph_metrics = {};
        font_face->GetGdiCompatibleGlyphMetrics(
            actual_size, 1.f, nullptr, TRUE,
            &glyph_index, 1, &glyph_metrics, FALSE );

        const auto d2p = actual_size / ( float )font_metrics.designUnitsPerEm;
        const auto adv_w = glyph_metrics.advanceWidth * d2p;

        auto black_box_w = ( int )( ( glyph_metrics.advanceWidth
            - glyph_metrics.leftSideBearing
            - glyph_metrics.rightSideBearing ) * d2p + 0.5f );
        if ( black_box_w < 0 ) black_box_w = 0;

        out_abc.abcA = ( int )( glyph_metrics.leftSideBearing * d2p );
        out_abc.abcB = ( UINT )black_box_w;
        out_abc.abcC = ( int )( glyph_metrics.rightSideBearing * d2p );
        out_tm.tmHeight = ( LONG )( ( font_metrics.ascent + font_metrics.descent ) * d2p + 0.5f );
        out_tm.tmAscent = ( LONG )( font_metrics.ascent * d2p + 0.5f );
        out_tm.tmDescent = ( LONG )( font_metrics.descent * d2p + 0.5f );
        out_tm.tmExternalLeading = ( LONG )( font_metrics.lineGap * d2p + 0.5f );
        out_tm.tmAveCharWidth = ( LONG )( adv_w + 0.5f );
        out_advance_width = adv_w;

        // ── Glyph run ─────────────────────────────────────────────────────────
        DWRITE_GLYPH_RUN glyph_run = {};
        glyph_run.fontFace = font_face;
        glyph_run.fontEmSize = actual_size;
        glyph_run.glyphCount = 1;
        glyph_run.glyphIndices = &glyph_index;

        // ── Rendering params ──────────────────────────────────────────────────
        IDWriteRenderingParams* rp = nullptr;
        bool is_icon_font = ( peach::font::get_face( key.font_name ) != nullptr );
        DWRITE_RENDERING_MODE rendering_mode = is_icon_font
            ? DWRITE_RENDERING_MODE_NATURAL
            : DWRITE_RENDERING_MODE_NATURAL_SYMMETRIC;

        m_dwrite_factory->CreateCustomRenderingParams(
            1.8f, 0.5f, 1.0f,
            DWRITE_PIXEL_GEOMETRY_RGB,
            rendering_mode, &rp );

        // ── Glyph run analysis ────────────────────────────────────────────────
        IDWriteGlyphRunAnalysis* analysis = nullptr;
        if ( FAILED( m_dwrite_factory->CreateGlyphRunAnalysis(
            &glyph_run, 1.f, nullptr,
            rendering_mode,
            DWRITE_MEASURING_MODE_NATURAL,
            0.f, 0.f, &analysis ) ) ) {
            if ( rp ) rp->Release( );
            cleanup( );
            return;
        }

        // ── Bounds ────────────────────────────────────────────────────────────
        RECT bounds = {};
        analysis->GetAlphaTextureBounds( DWRITE_TEXTURE_CLEARTYPE_3x1, &bounds );
        int g_w = bounds.right - bounds.left;
        int g_h = bounds.bottom - bounds.top;

        if ( g_w <= 0 || g_h <= 0 ) {
            analysis->GetAlphaTextureBounds( DWRITE_TEXTURE_ALIASED_1x1, &bounds );
            g_w = bounds.right - bounds.left;
            g_h = bounds.bottom - bounds.top;
        }

        out_bounds_left = bounds.left;
        out_bounds_top = bounds.top;

        if ( g_w <= 0 || g_h <= 0 ) {
            analysis->Release( );
            if ( rp ) rp->Release( );
            cleanup( );
            out_w = out_h = 1;
            if ( out_data_size >= 2 ) { out_data[ 0 ] = 0; out_data[ 1 ] = 0; }
            return;
        }

        const auto outline_pad = ( int )( actual_outline + 0.5f );
        const auto render_w = g_w + outline_pad * 2;
        const auto render_h = g_h + outline_pad * 2;
        const auto rpx = ( size_t )render_w * render_h;
        const auto ct_size = ( size_t )g_w * g_h * 3;

        if ( rpx > k_glyph_scratch_sz || ct_size > k_glyph_scratch_sz ) {
            analysis->Release( );
            if ( rp ) rp->Release( );
            cleanup( );
            return;
        }

        // ── Scratch buffers — no aliasing with out_data (scratch[3]) ─────────
        // scratch[0] = ct_data   (raw cleartype pixels from DWrite)
        // scratch[1] = text_data (single-channel glyph alpha, padded)
        // scratch[2] = outline_data
        // scratch[2] + k_glyph_scratch_sz/2 = temp_data (horizontal blur pass)
        // out_data   = scratch[3] from caller — written only at the very end
        auto* ct_data = s_glyph_scratch[ 0 ];
        auto* text_data = s_glyph_scratch[ 1 ];
        auto* outline_data = s_glyph_scratch[ 2 ];
        auto* temp_data = s_glyph_scratch[ 2 ] + ( k_glyph_scratch_sz / 2 );

        // ── Rasterize ─────────────────────────────────────────────────────────
        bool is_ct = true;
        HRESULT hr = analysis->CreateAlphaTexture(
            DWRITE_TEXTURE_CLEARTYPE_3x1, &bounds,
            ct_data, ( UINT32 )ct_size );

        if ( FAILED( hr ) || g_w <= 0 || g_h <= 0 ) {
            is_ct = false;

            RECT aliased_bounds = {};
            analysis->GetAlphaTextureBounds( DWRITE_TEXTURE_ALIASED_1x1, &aliased_bounds );
            g_w = aliased_bounds.right - aliased_bounds.left;
            g_h = aliased_bounds.bottom - aliased_bounds.top;

            if ( g_w <= 0 || g_h <= 0 ) {
                analysis->Release( );
                if ( rp ) rp->Release( );
                cleanup( );
                out_w = out_h = 1;
                if ( out_data_size >= 2 ) { out_data[ 0 ] = 0; out_data[ 1 ] = 0; }
                return;
            }

            bounds = aliased_bounds;
            const auto new_rpx = ( size_t )( g_w + outline_pad * 2 ) * ( g_h + outline_pad * 2 );
            const auto aliased_sz = ( size_t )g_w * g_h;

            if ( new_rpx > k_glyph_scratch_sz || aliased_sz > k_glyph_scratch_sz ) {
                analysis->Release( );
                if ( rp ) rp->Release( );
                cleanup( );
                return;
            }

            analysis->CreateAlphaTexture(
                DWRITE_TEXTURE_ALIASED_1x1, &bounds,
                ct_data, ( UINT32 )aliased_sz );
        }

        analysis->Release( );
        if ( rp ) rp->Release( );

        // Recalculate render dimensions in case aliased path changed g_w/g_h
        const auto rw = g_w + outline_pad * 2;
        const auto rh = g_h + outline_pad * 2;
        const auto rp2 = ( size_t )rw * rh;

        // ── Stem darkening for small sizes ────────────────────────────────────
        auto stem_darken = 0.f;
        if ( actual_size < 24.f ) {
            stem_darken = 0.3f * ( 1.f - ( actual_size - 8.f ) / 16.f );
            if ( stem_darken < 0.f ) stem_darken = 0.f;
        }

        // ── Fill text_data ────────────────────────────────────────────────────
        memset( text_data, 0, rp2 );
        for ( auto y = 0; y < g_h; y++ ) {
            for ( auto x = 0; x < g_w; x++ ) {
                auto dst = ( y + outline_pad ) * rw + ( x + outline_pad );
                float alpha;
                if ( is_ct ) {
                    auto src = ( y * g_w + x ) * 3;
                    alpha = ( ct_data[ src ] + ct_data[ src + 1 ] + ct_data[ src + 2 ] ) / ( 3.f * 255.f );
                }
                else {
                    alpha = ct_data[ y * g_w + x ] / 255.f;
                }
                if ( stem_darken > 0.f && alpha > 0.f ) {
                    alpha += stem_darken * ( 1.f - alpha );
                    if ( alpha > 1.f ) alpha = 1.f;
                }
                text_data[ dst ] = ( uint8_t )( alpha * 255.f + 0.5f );
            }
        }

        // ── Outline dilation ──────────────────────────────────────────────────
        memset( outline_data, 0, rp2 );
        if ( actual_outline > 0.f ) {
            memset( temp_data, 0, rp2 );

            // ADD THIS: cap outline_pad before the loops
            const auto safe_outline_pad = mm_imin( outline_pad, 32 );

            // Horizontal pass
            for ( auto y = 0; y < rh; y++ ) {
                for ( auto x = 0; x < rw; x++ ) {
                    auto mv = ( uint8_t )0;
                    auto xs = mm_imax( 0, x - safe_outline_pad );   // was: outline_pad
                    auto xe = mm_imin( rw - 1, x + safe_outline_pad ); // was: outline_pad
                    for ( auto sx = xs; sx <= xe; sx++ ) {
                        auto v = text_data[ y * rw + sx ];
                        if ( v > mv ) mv = v;
                    }
                    temp_data[ y * rw + x ] = mv;
                }
            }

            // Vertical pass
            for ( auto y = 0; y < rh; y++ ) {
                auto ys = mm_imax( 0, y - safe_outline_pad );       // was: outline_pad
                auto ye = mm_imin( rh - 1, y + safe_outline_pad ); // was: outline_pad
                for ( auto x = 0; x < rw; x++ ) {
                    auto mv = ( uint8_t )0;
                    for ( auto sy = ys; sy <= ye; sy++ ) {
                        auto v = temp_data[ sy * rw + x ];
                        if ( v > mv ) mv = v;
                    }
                    outline_data[ y * rw + x ] = mv;
                }
            }
        }

        // ── Output size check ─────────────────────────────────────────────────
        if ( ( size_t )rw * rh * 2 > out_data_size ) {
            cleanup( );
            out_w = out_h = 0;
            return;
        }

        // ── sRGB LUT init ─────────────────────────────────────────────────────
        if ( !s_luts_init ) {
            const auto gamma = 2.2f, ig = 1.f / gamma;
            for ( auto i = 0; i < 256; i++ ) {
                auto v = i / 255.f;
                s_srgb_to_linear[ i ] = powf( v, gamma );
                s_linear_to_srgb[ i ] = powf( v, ig );
            }
            s_luts_init = true;
        }

        // ── Write interleaved outline/text into out_data (scratch[3]) ─────────
        out_w = rw;
        out_h = rh;
        for ( auto y = 0; y < out_h; y++ ) {
            for ( auto x = 0; x < out_w; x++ ) {
                auto src = y * rw + x;
                auto dst = ( y * out_w + x ) * 2;
                auto t_l = s_srgb_to_linear[ text_data[ src ] ];
                auto o_l = s_srgb_to_linear[ outline_data[ src ] ];
                out_data[ dst + 0 ] = ( uint8_t )( s_linear_to_srgb[ ( int )( o_l * 255.f + 0.5f ) ] * 255.f + 0.5f );
                out_data[ dst + 1 ] = ( uint8_t )( s_linear_to_srgb[ ( int )( t_l * 255.f + 0.5f ) ] * 255.f + 0.5f );
            }
        }

        cleanup( );
    }

    // ═══════════════════════════════════════════════════════════════════════
    // get_or_create_glyph
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::draw_glow( float x, float y, float w, float h, float radius, uint8_t alpha ) {
        draw_rect_shadow(
            x, y, w, h,
            radius,
            0.f, 0.f,
            { 18, 18, 18, alpha }  // matches content panel color
        );
    }

    inline glyph_info_t* context_t::get_or_create_glyph( const char* font_name, float font_size, uint32_t char_code, float outline_thickness ) {
        //const auto is_emoji = is_emoji_codepoint( char_code );
        //const auto* eff_font = is_emoji ? "Segoe UI Emoji" : font_name;

        auto q_size = ( int )( font_size * 4.f + 0.5f );
        auto q_outline = ( int )( outline_thickness * 4.f + 0.5f );

        if ( char_code < 128 ) {
            auto& fc = m_fast_cache;
            if ( fc.font_size == q_size && fc.outline == q_outline && mm_strcmp( fc.font_name, font_name ) == 0 ) {
                auto* cached = fc.table[ char_code ];
                if ( cached ) return cached;
            }
            else {
                mm_strncpy( fc.font_name, font_name, 64 );
                fc.font_size = q_size; fc.outline = q_outline;
                memset( fc.table, 0, sizeof( fc.table ) );
            }
        }

        glyph_key_t key = {};
        // replace mm_strncpy with manual copy
        for ( int i = 0; i < 63 && font_name[ i ]; i++ )
            key.font_name[ i ] = font_name[ i ];
        key.font_name[ 63 ] = '\0';
        key.font_size = q_size;
        key.char_code = char_code;
        key.outline_thickness = q_outline;

        auto* existing = m_glyphs.find( key );
        if ( existing ) {
            if ( char_code < 128 ) m_fast_cache.table[ char_code ] = existing;
            return existing;
        }

        int g_w, g_h, bounds_l, bounds_t;
        float adv_w;
        ABC abc; TEXTMETRICA tm;
        rasterize_glyph( key, g_w, g_h, s_glyph_scratch[ 3 ], k_glyph_scratch_sz,
            abc, tm, bounds_l, bounds_t, adv_w );

        if ( g_w == 0 || g_h == 0 ) return nullptr;

        int ax, ay;
        if ( !m_atlas_packer.pack( g_w, g_h, ax, ay ) ) return nullptr;

        const auto safe_h = mm_imin( g_h, k_atlas_size - ay );
        const auto safe_w = mm_imin( g_w, k_atlas_size - ax );
        for ( auto y = 0; y < safe_h; y++ ) {
            for ( auto x = 0; x < safe_w; x++ ) {
                auto ai = ( ( ay + y ) * k_atlas_size + ( ax + x ) ) * 2;
                auto gi = ( y * g_w + x ) * 2;
                if ( gi + 1 >= ( int )k_glyph_scratch_sz ) break;  // scratch overread guard
                s_atlas_data[ ai + 0 ] = s_glyph_scratch[ 3 ][ gi + 0 ];
                s_atlas_data[ ai + 1 ] = s_glyph_scratch[ 3 ][ gi + 1 ];
            }
        }
        m_atlas_needs_upload = true;

        auto* slot = m_glyphs.insert( key );
        if ( !slot ) return nullptr;

        const auto aop = key.outline_thickness * 0.25f;
        slot->u0 = ( float )ax / k_atlas_size;
        slot->v0 = ( float )ay / k_atlas_size;
        slot->u1 = ( float )( ax + g_w ) / k_atlas_size;
        slot->v1 = ( float )( ay + g_h ) / k_atlas_size;
        slot->width = ( float )g_w;
        slot->height = ( float )g_h;
        slot->bearing_x = ( float )bounds_l - aop;
        slot->bearing_y = -( float )tm.tmAscent - ( float )bounds_t + aop;
        slot->advance_x = adv_w;
        slot->line_height = ( float )tm.tmHeight;

        if ( char_code < 128 ) m_fast_cache.table[ char_code ] = slot;
        return slot;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Draw primitives — all unchanged from original
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::draw_rect( float x, float y, float w, float h, color_t color ) {
        auto& f = m_frames[ m_current_frame ];
        if ( f.write_offset + 6 > k_max_vertices ) return;
        auto* v = s_staging_buffer + f.write_offset;
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto x1 = x + w, y1 = y + h;
        v[ 0 ] = { x,y,0,0,c,0,p }; v[ 1 ] = { x1,y,0,0,c,0,p }; v[ 2 ] = { x1,y1,0,0,c,0,p };
        v[ 3 ] = { x,y,0,0,c,0,p }; v[ 4 ] = { x1,y1,0,0,c,0,p }; v[ 5 ] = { x,y1,0,0,c,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_rect_outline( float x, float y, float w, float h, float t, color_t color ) {
        draw_rect( x, y, w, t, color );
        draw_rect( x, y + h - t, w, t, color );
        draw_rect( x, y, t, h, color );
        draw_rect( x + w - t, y, t, h, color );
    }

    inline void context_t::draw_line( float x0, float y0, float x1, float y1, float thickness, color_t color ) {
        const auto dx = x1 - x0, dy = y1 - y0, lsq = dx * dx + dy * dy;
        if ( lsq < 1e-6f ) return;
        const auto rl = 1.f / sqrtf( lsq ), nx = -dy * rl, ny = dx * rl;
        auto& f = m_frames[ m_current_frame ];
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + 18 > k_max_vertices ) return;
            const auto aab = thickness > 1.5f ? 1.f : 0.75f, hc = thickness * 0.5f, ho = hc + aab;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { x0 + nx * hc,y0 + ny * hc,0.f,aa_flag,c,0,p }; v[ 1 ] = { x0 + nx * ho,y0 + ny * ho,1.f,aa_flag,c,0,p }; v[ 2 ] = { x1 + nx * ho,y1 + ny * ho,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x0 + nx * hc,y0 + ny * hc,0.f,aa_flag,c,0,p }; v[ 4 ] = { x1 + nx * ho,y1 + ny * ho,1.f,aa_flag,c,0,p }; v[ 5 ] = { x1 + nx * hc,y1 + ny * hc,0.f,aa_flag,c,0,p };
            v[ 6 ] = { x0 - nx * hc,y0 - ny * hc,0.f,aa_flag,c,0,p }; v[ 7 ] = { x0 + nx * hc,y0 + ny * hc,0.f,aa_flag,c,0,p }; v[ 8 ] = { x1 + nx * hc,y1 + ny * hc,0.f,aa_flag,c,0,p };
            v[ 9 ] = { x0 - nx * hc,y0 - ny * hc,0.f,aa_flag,c,0,p }; v[ 10 ] = { x1 + nx * hc,y1 + ny * hc,0.f,aa_flag,c,0,p }; v[ 11 ] = { x1 - nx * hc,y1 - ny * hc,0.f,aa_flag,c,0,p };
            v[ 12 ] = { x1 - nx * hc,y1 - ny * hc,0.f,aa_flag,c,0,p }; v[ 13 ] = { x1 - nx * ho,y1 - ny * ho,1.f,aa_flag,c,0,p }; v[ 14 ] = { x0 - nx * ho,y0 - ny * ho,1.f,aa_flag,c,0,p };
            v[ 15 ] = { x1 - nx * hc,y1 - ny * hc,0.f,aa_flag,c,0,p }; v[ 16 ] = { x0 - nx * ho,y0 - ny * ho,1.f,aa_flag,c,0,p }; v[ 17 ] = { x0 - nx * hc,y0 - ny * hc,0.f,aa_flag,c,0,p };
            f.write_offset += 18;
        }
        else {
            if ( f.write_offset + 6 > k_max_vertices ) return;
            const auto ht = thickness * 0.5f;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { x0 + nx * ht,y0 + ny * ht,1.f,0.f,c,0,p }; v[ 1 ] = { x1 + nx * ht,y1 + ny * ht,1.f,0.f,c,0,p }; v[ 2 ] = { x1 - nx * ht,y1 - ny * ht,1.f,0.f,c,0,p };
            v[ 3 ] = { x0 + nx * ht,y0 + ny * ht,1.f,0.f,c,0,p }; v[ 4 ] = { x1 - nx * ht,y1 - ny * ht,1.f,0.f,c,0,p }; v[ 5 ] = { x0 - nx * ht,y0 - ny * ht,1.f,0.f,c,0,p };
            f.write_offset += 6;
        }
    }

    inline void context_t::draw_circle( float x, float y, float radius, color_t color, int segments ) {
        if ( segments == 32 ) { auto circ = k_tau * radius; segments = mm_imax( 24, mm_imin( 96, ( int )( circ / 2.f ) ) ); }
        auto& f = m_frames[ m_current_frame ];
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        sin_cos_lut::init( segments );
        if ( m_shape_aa ) {
            const auto aab = 1.f, ir = radius - aab;
            auto need = ir <= 0 ? segments * 3 : segments * 9;
            if ( f.write_offset + need > k_max_vertices ) return;
            auto* v = s_staging_buffer + f.write_offset;
            auto c0 = 1.f, s0 = 0.f;
            for ( auto i = 0; i < segments; i++ ) {
                auto c1 = sin_cos_lut::s_cos[ i + 1 ], s1 = sin_cos_lut::s_sin[ i + 1 ];
                if ( ir <= 0 ) { v[ 0 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 1 ] = { x + c0 * radius,y + s0 * radius,1.f,aa_flag,c,0,p }; v[ 2 ] = { x + c1 * radius,y + s1 * radius,1.f,aa_flag,c,0,p }; v += 3; }
                else {
                    v[ 0 ] = { x,y,0.f,0.f,c,0,p }; v[ 1 ] = { x + c0 * ir,y + s0 * ir,0.f,0.f,c,0,p }; v[ 2 ] = { x + c1 * ir,y + s1 * ir,0.f,0.f,c,0,p };
                    v[ 3 ] = { x + c0 * ir,y + s0 * ir,0.85f,aa_flag,c,0,p }; v[ 4 ] = { x + c0 * radius,y + s0 * radius,1.f,aa_flag,c,0,p }; v[ 5 ] = { x + c1 * ir,y + s1 * ir,0.85f,aa_flag,c,0,p };
                    v[ 6 ] = { x + c1 * ir,y + s1 * ir,0.85f,aa_flag,c,0,p }; v[ 7 ] = { x + c0 * radius,y + s0 * radius,1.f,aa_flag,c,0,p }; v[ 8 ] = { x + c1 * radius,y + s1 * radius,1.f,aa_flag,c,0,p };
                    v += 9;
                }
                c0 = c1; s0 = s1;
            }
            f.write_offset += need;
        }
        else {
            if ( f.write_offset + segments * 3 > k_max_vertices ) return;
            auto* v = s_staging_buffer + f.write_offset;
            auto c0 = 1.f, s0 = 0.f;
            for ( auto i = 0; i < segments; i++ ) {
                auto c1 = sin_cos_lut::s_cos[ i + 1 ], s1 = sin_cos_lut::s_sin[ i + 1 ];
                v[ 0 ] = { x,y,0.f,0.f,c,0,p }; v[ 1 ] = { x + c0 * radius,y + s0 * radius,0.f,0.f,c,0,p }; v[ 2 ] = { x + c1 * radius,y + s1 * radius,0.f,0.f,c,0,p }; v += 3;
                c0 = c1; s0 = s1;
            }
            f.write_offset += segments * 3;
        }
    }

    inline void context_t::draw_circle_outline( float x, float y, float radius, float thickness, color_t color, int segments ) {
        if ( segments == 32 ) { auto circ = k_tau * radius; segments = mm_imax( 24, mm_imin( 96, ( int )( circ / 2.f ) ) ); }
        auto& f = m_frames[ m_current_frame ];
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        sin_cos_lut::init( segments );
        const auto aab = m_shape_aa ? 1.f : 0.f;
        const auto ir = radius - thickness * 0.5f, orr = radius + thickness * 0.5f, ira = ir - aab, ora = orr + aab;
        auto need = m_shape_aa ? segments * 18 : segments * 6;
        if ( f.write_offset + need > k_max_vertices ) return;
        auto* v = s_staging_buffer + f.write_offset;
        auto c0 = 1.f, s0 = 0.f;
        for ( auto i = 0; i < segments; i++ ) {
            auto c1 = sin_cos_lut::s_cos[ i + 1 ], s1 = sin_cos_lut::s_sin[ i + 1 ];
            if ( m_shape_aa ) {
                auto x0ia = x + c0 * ira, y0ia = y + s0 * ira, x1ia = x + c1 * ira, y1ia = y + s1 * ira;
                auto x0i = x + c0 * ir, y0i = y + s0 * ir, x1i = x + c1 * ir, y1i = y + s1 * ir;
                auto x0o = x + c0 * orr, y0o = y + s0 * orr, x1o = x + c1 * orr, y1o = y + s1 * orr;
                auto x0oa = x + c0 * ora, y0oa = y + s0 * ora, x1oa = x + c1 * ora, y1oa = y + s1 * ora;
                v[ 0 ] = { x0ia,y0ia,1.f,aa_flag,c,0,p }; v[ 1 ] = { x0i,y0i,0.f,aa_flag,c,0,p }; v[ 2 ] = { x1i,y1i,0.f,aa_flag,c,0,p };
                v[ 3 ] = { x0ia,y0ia,1.f,aa_flag,c,0,p }; v[ 4 ] = { x1i,y1i,0.f,aa_flag,c,0,p }; v[ 5 ] = { x1ia,y1ia,1.f,aa_flag,c,0,p };
                v[ 6 ] = { x0i,y0i,0.f,aa_flag,c,0,p };   v[ 7 ] = { x0o,y0o,0.f,aa_flag,c,0,p }; v[ 8 ] = { x1o,y1o,0.f,aa_flag,c,0,p };
                v[ 9 ] = { x0i,y0i,0.f,aa_flag,c,0,p };   v[ 10 ] = { x1o,y1o,0.f,aa_flag,c,0,p }; v[ 11 ] = { x1i,y1i,0.f,aa_flag,c,0,p };
                v[ 12 ] = { x0o,y0o,0.f,aa_flag,c,0,p };  v[ 13 ] = { x0oa,y0oa,1.f,aa_flag,c,0,p }; v[ 14 ] = { x1oa,y1oa,1.f,aa_flag,c,0,p };
                v[ 15 ] = { x0o,y0o,0.f,aa_flag,c,0,p };  v[ 16 ] = { x1oa,y1oa,1.f,aa_flag,c,0,p }; v[ 17 ] = { x1o,y1o,0.f,aa_flag,c,0,p };
                v += 18;
            }
            else {
                auto x0i = x + c0 * ir, y0i = y + s0 * ir, x1i = x + c1 * ir, y1i = y + s1 * ir;
                auto x0o = x + c0 * orr, y0o = y + s0 * orr, x1o = x + c1 * orr, y1o = y + s1 * orr;
                v[ 0 ] = { x0i,y0i,1.f,0.f,c,0,p }; v[ 1 ] = { x0o,y0o,1.f,0.f,c,0,p }; v[ 2 ] = { x1o,y1o,1.f,0.f,c,0,p };
                v[ 3 ] = { x0i,y0i,1.f,0.f,c,0,p }; v[ 4 ] = { x1o,y1o,1.f,0.f,c,0,p }; v[ 5 ] = { x1i,y1i,1.f,0.f,c,0,p };
                v += 6;
            }
            c0 = c1; s0 = s1;
        }
        f.write_offset += need;
    }

    // rounded_rect, rounded_rect_outline, triangle, triangle_outline,
    // polygon, polygon_outline, pie, arc, bezier, gradients, shadow,
    // dashed_line — all identical to original, kept verbatim below.

    inline void context_t::draw_rounded_rect( float x, float y, float w, float h, float radius, color_t color ) {
        const auto max_r = ( ( w < h ) ? w : h ) * 0.5f;
        if ( radius > max_r ) radius = max_r;
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        auto& f = m_frames[ m_current_frame ];
        auto emit6 = [ & ] ( float ax, float ay, float bx, float by, float cx, float cy, float dx, float dy ) {
            if ( f.write_offset + 6 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { ax,ay,0,aa_flag,c,0,p }; v[ 1 ] = { bx,by,0,aa_flag,c,0,p }; v[ 2 ] = { cx,cy,0,aa_flag,c,0,p };
            v[ 3 ] = { ax,ay,0,aa_flag,c,0,p }; v[ 4 ] = { cx,cy,0,aa_flag,c,0,p }; v[ 5 ] = { dx,dy,0,aa_flag,c,0,p };
            f.write_offset += 6;
            };
        emit6( x + radius, y + radius, x + w - radius, y + radius, x + w - radius, y + h - radius, x + radius, y + h - radius );
        emit6( x + radius, y, x + w - radius, y, x + w - radius, y + radius, x + radius, y + radius );
        emit6( x + radius, y + h - radius, x + w - radius, y + h - radius, x + w - radius, y + h, x + radius, y + h );
        emit6( x, y + radius, x + radius, y + radius, x + radius, y + h - radius, x, y + h - radius );
        emit6( x + w - radius, y + radius, x + w, y + radius, x + w, y + h - radius, x + w - radius, y + h - radius );
        const auto cs = 8; quarter_arc_lut::init( cs );
        auto corner = [ & ] ( float cx2, float cy2, int quadrant ) {
            // quadrant 0=TL, 1=TR, 2=BR, 3=BL
            for ( auto i = 0; i < cs; i++ ) {
                if ( f.write_offset + 3 > k_max_vertices ) return;
                float c0, s0, c1, s1;
                switch ( quadrant ) {
                    case 0: // top-left: 180..270
                    c0 = -quarter_arc_lut::s_cos[ i ];
                    s0 = -quarter_arc_lut::s_sin[ i ];
                    c1 = -quarter_arc_lut::s_cos[ i + 1 ];
                    s1 = -quarter_arc_lut::s_sin[ i + 1 ];
                    break;
                    case 1: // top-right: 270..360
                    c0 = quarter_arc_lut::s_sin[ i ];
                    s0 = -quarter_arc_lut::s_cos[ i ];
                    c1 = quarter_arc_lut::s_sin[ i + 1 ];
                    s1 = -quarter_arc_lut::s_cos[ i + 1 ];
                    break;
                    case 2: // bottom-right: 0..90
                    c0 = quarter_arc_lut::s_cos[ i ];
                    s0 = quarter_arc_lut::s_sin[ i ];
                    c1 = quarter_arc_lut::s_cos[ i + 1 ];
                    s1 = quarter_arc_lut::s_sin[ i + 1 ];
                    break;
                    default: // bottom-left: 90..180
                    c0 = -quarter_arc_lut::s_sin[ i ];
                    s0 = quarter_arc_lut::s_cos[ i ];
                    c1 = -quarter_arc_lut::s_sin[ i + 1 ];
                    s1 = quarter_arc_lut::s_cos[ i + 1 ];
                    break;
                }
                auto* vv = s_staging_buffer + f.write_offset;
                vv[ 0 ] = { cx2,           cy2,                          0.f, aa_flag, c, 0, p };
                vv[ 1 ] = { cx2 + c0 * radius, cy2 + s0 * radius, 1.f, aa_flag, c, 0, p };
                vv[ 2 ] = { cx2 + c1 * radius, cy2 + s1 * radius, 1.f, aa_flag, c, 0, p };
                f.write_offset += 3;
            }
            };

        corner( x + radius, y + radius, 0 );  // top-left
        corner( x + w - radius, y + radius, 1 );  // top-right
        corner( x + w - radius, y + h - radius, 2 );  // bottom-right
        corner( x + radius, y + h - radius, 3 );  // bottom-left
    }

    inline void context_t::draw_rounded_rect_outline( float x, float y, float w, float h, float radius, float thickness, color_t color ) {
        const auto max_r = ( ( w < h ) ? w : h ) * 0.5f;
        if ( radius > max_r )radius = max_r;
        if ( radius < 0.5f ) { draw_rect_outline( x, y, w, h, thickness, color ); return; }
        const auto arc_segs = 8;
        const auto half_thick = thickness * 0.5f, arc_cr = radius - half_thick;
        float path_x[ 40 ], path_y[ 40 ]; auto n = 0;
        quarter_arc_lut::init( arc_segs );
        auto add_arc = [ & ] ( float cx, float cy, bool neg_c, bool neg_s ) {
            for ( auto i = 0; i <= arc_segs; i++ ) {
                auto cv = quarter_arc_lut::s_cos[ i ], sv = quarter_arc_lut::s_sin[ i ];
                float rx, ry;
                if ( neg_c && neg_s ) { rx = -cv; ry = -sv; }
                else if ( neg_c ) { rx = -sv; ry = cv; }
                else if ( neg_s ) { rx = sv; ry = -cv; }
                else { rx = cv; ry = sv; }
                path_x[ n ] = cx + rx * arc_cr; path_y[ n ] = cy + ry * arc_cr; n++;
            }
            };
        add_arc( x + radius, y + radius, true, true );
        add_arc( x + w - radius, y + radius, false, true );
        add_arc( x + w - radius, y + h - radius, false, false );
        add_arc( x + radius, y + h - radius, true, false );
        float norm_x[ 40 ], norm_y[ 40 ], miter_x[ 40 ], miter_y[ 40 ], miter_l[ 40 ];
        for ( auto i = 0; i < n; i++ ) {
            auto i1 = ( i + 1 ) % n;
            const auto dx = path_x[ i1 ] - path_x[ i ], dy = path_y[ i1 ] - path_y[ i ], len = sqrtf( dx * dx + dy * dy );
            if ( len < 0.001f ) { norm_x[ i ] = 0; norm_y[ i ] = 1; continue; }
            auto rl = 1.f / len; norm_x[ i ] = -dy * rl; norm_y[ i ] = dx * rl;
        }
        for ( auto i = 0; i < n; i++ ) {
            auto prev = ( i + n - 1 ) % n;
            const auto anx = ( norm_x[ prev ] + norm_x[ i ] ) * 0.5f, any = ( norm_y[ prev ] + norm_y[ i ] ) * 0.5f;
            auto al = sqrtf( anx * anx + any * any );
            if ( al < 0.001f ) { miter_x[ i ] = norm_x[ i ]; miter_y[ i ] = norm_y[ i ]; miter_l[ i ] = 1.f; }
            else {
                auto rl = 1.f / al; miter_x[ i ] = anx * rl; miter_y[ i ] = any * rl;
                auto dot = miter_x[ i ] * norm_x[ i ] + miter_y[ i ] * norm_y[ i ];
                miter_l[ i ] = ( dot > 0.1f ) ? 1.f / dot : 10.f;
                if ( miter_l[ i ] > 3.f )miter_l[ i ] = 3.f;
            }
        }
        auto& f = m_frames[ m_current_frame ];
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + n * 18 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; const auto aab = 1.f;
            for ( auto i = 0; i < n; i++ ) {
                auto i1 = ( i + 1 ) % n;
                const auto m0x = miter_x[ i ] * miter_l[ i ], m0y = miter_y[ i ] * miter_l[ i ];
                auto m1x = miter_x[ i1 ] * miter_l[ i1 ], m1y = miter_y[ i1 ] * miter_l[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ], ih = half_thick, oh = half_thick + aab;
                v[ 0 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 1 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 2 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 3 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 4 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p }; v[ 5 ] = { px1 - m1x * oh,py1 - m1y * oh,1.f,aa_flag,c,0,p };
                v[ 6 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 7 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 8 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v[ 9 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 10 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p }; v[ 11 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 12 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 13 ] = { px0 + m0x * oh,py0 + m0y * oh,1.f,aa_flag,c,0,p }; v[ 14 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p };
                v[ 15 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 16 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p }; v[ 17 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v += 18;
            }
            f.write_offset += n * 18;
        }
        else {
            if ( f.write_offset + n * 6 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            for ( auto i = 0; i < n; i++ ) {
                auto i1 = ( i + 1 ) % n;
                const auto m0x = miter_x[ i ] * miter_l[ i ], m0y = miter_y[ i ] * miter_l[ i ];
                auto m1x = miter_x[ i1 ] * miter_l[ i1 ], m1y = miter_y[ i1 ] * miter_l[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ];
                v[ 0 ] = { px0 - m0x * half_thick,py0 - m0y * half_thick,0.f,0.f,c,0,p }; v[ 1 ] = { px0 + m0x * half_thick,py0 + m0y * half_thick,0.f,0.f,c,0,p }; v[ 2 ] = { px1 + m1x * half_thick,py1 + m1y * half_thick,0.f,0.f,c,0,p };
                v[ 3 ] = { px0 - m0x * half_thick,py0 - m0y * half_thick,0.f,0.f,c,0,p }; v[ 4 ] = { px1 + m1x * half_thick,py1 + m1y * half_thick,0.f,0.f,c,0,p }; v[ 5 ] = { px1 - m1x * half_thick,py1 - m1y * half_thick,0.f,0.f,c,0,p };
                v += 6;
            }
            f.write_offset += n * 6;
        }
    }

    inline void context_t::draw_triangle( float x0, float y0, float x1, float y1, float x2, float y2, color_t color ) {
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + 21 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; const auto aab = 1.f;
            auto edge_norm = [ ] ( float ax, float ay, float bx, float by, float cx, float cy, float& nx, float& ny ) {
                auto dx = bx - ax, dy = by - ay, len = sqrtf( dx * dx + dy * dy );
                if ( len < 1e-4f ) { nx = 0; ny = 0; return; }
                auto rl = 1.f / len; nx = -dy * rl; ny = dx * rl;
                if ( nx * ( cx - ax ) + ny * ( cy - ay ) > 0 ) { nx = -nx; ny = -ny; }
                };
            v[ 0 ] = { x0,y0,0.f,aa_flag,c,0,p }; v[ 1 ] = { x1,y1,0.f,aa_flag,c,0,p }; v[ 2 ] = { x2,y2,0.f,aa_flag,c,0,p }; v += 3;
            float n01x, n01y, n12x, n12y, n20x, n20y;
            edge_norm( x0, y0, x1, y1, x2, y2, n01x, n01y ); edge_norm( x1, y1, x2, y2, x0, y0, n12x, n12y ); edge_norm( x2, y2, x0, y0, x1, y1, n20x, n20y );
            auto x0e = x0 + n01x * aab, y0e = y0 + n01y * aab, x1e = x1 + n01x * aab, y1e = y1 + n01y * aab;
            v[ 0 ] = { x0,y0,0.f,aa_flag,c,0,p }; v[ 1 ] = { x1,y1,0.f,aa_flag,c,0,p }; v[ 2 ] = { x1e,y1e,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x0,y0,0.f,aa_flag,c,0,p }; v[ 4 ] = { x1e,y1e,1.f,aa_flag,c,0,p }; v[ 5 ] = { x0e,y0e,1.f,aa_flag,c,0,p }; v += 6;
            auto x1f = x1 + n12x * aab, y1f = y1 + n12y * aab, x2e = x2 + n12x * aab, y2e = y2 + n12y * aab;
            v[ 0 ] = { x1,y1,0.f,aa_flag,c,0,p }; v[ 1 ] = { x2,y2,0.f,aa_flag,c,0,p }; v[ 2 ] = { x2e,y2e,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x1,y1,0.f,aa_flag,c,0,p }; v[ 4 ] = { x2e,y2e,1.f,aa_flag,c,0,p }; v[ 5 ] = { x1f,y1f,1.f,aa_flag,c,0,p }; v += 6;
            auto x2f = x2 + n20x * aab, y2f = y2 + n20y * aab, x0f = x0 + n20x * aab, y0f = y0 + n20y * aab;
            v[ 0 ] = { x2,y2,0.f,aa_flag,c,0,p }; v[ 1 ] = { x0,y0,0.f,aa_flag,c,0,p }; v[ 2 ] = { x0f,y0f,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x2,y2,0.f,aa_flag,c,0,p }; v[ 4 ] = { x0f,y0f,1.f,aa_flag,c,0,p }; v[ 5 ] = { x2f,y2f,1.f,aa_flag,c,0,p };
            f.write_offset += 21;
        }
        else {
            if ( f.write_offset + 3 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { x0,y0,0.f,0.f,c,0,p }; v[ 1 ] = { x1,y1,0.f,0.f,c,0,p }; v[ 2 ] = { x2,y2,0.f,0.f,c,0,p };
            f.write_offset += 3;
        }
    }

    inline void context_t::draw_triangle_outline( float x0, float y0, float x1, float y1, float x2, float y2, float thickness, color_t color ) {
        float path_x[ 3 ] = { x0,x1,x2 }, path_y[ 3 ] = { y0,y1,y2 }, ht = thickness * 0.5f;
        float norm_x[ 3 ], norm_y[ 3 ], mx[ 3 ], my[ 3 ], ml[ 3 ];
        for ( auto i = 0; i < 3; i++ ) {
            auto i1 = ( i + 1 ) % 3;
            const auto dx = path_x[ i1 ] - path_x[ i ], dy = path_y[ i1 ] - path_y[ i ], l = sqrtf( dx * dx + dy * dy );
            if ( l < 0.001f ) { norm_x[ i ] = 0; norm_y[ i ] = 1; continue; }
            auto rl = 1.f / l; norm_x[ i ] = -dy * rl; norm_y[ i ] = dx * rl;
        }
        for ( auto i = 0; i < 3; i++ ) {
            auto prev = ( i + 2 ) % 3;
            const auto anx = ( norm_x[ prev ] + norm_x[ i ] ) * 0.5f, any = ( norm_y[ prev ] + norm_y[ i ] ) * 0.5f;
            auto al = sqrtf( anx * anx + any * any );
            if ( al < 0.001f ) { mx[ i ] = norm_x[ i ]; my[ i ] = norm_y[ i ]; ml[ i ] = 1.f; }
            else {
                auto rl = 1.f / al; mx[ i ] = anx * rl; my[ i ] = any * rl;
                auto dot = mx[ i ] * norm_x[ i ] + my[ i ] * norm_y[ i ];
                ml[ i ] = ( dot > 0.1f ) ? 1.f / dot : 10.f; if ( ml[ i ] > 4.f )ml[ i ] = 4.f;
            }
        }
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + 54 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; const auto aab = 1.f;
            for ( auto i = 0; i < 3; i++ ) {
                auto i1 = ( i + 1 ) % 3;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ], ih = ht, oh = ht + aab;
                v[ 0 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 1 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 2 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 3 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 4 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p }; v[ 5 ] = { px1 - m1x * oh,py1 - m1y * oh,1.f,aa_flag,c,0,p };
                v[ 6 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 7 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 8 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v[ 9 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 10 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p }; v[ 11 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 12 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 13 ] = { px0 + m0x * oh,py0 + m0y * oh,1.f,aa_flag,c,0,p }; v[ 14 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p };
                v[ 15 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 16 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p }; v[ 17 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v += 18;
            }
            f.write_offset += 54;
        }
        else {
            if ( f.write_offset + 18 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            for ( auto i = 0; i < 3; i++ ) {
                auto i1 = ( i + 1 ) % 3;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ];
                v[ 0 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 1 ] = { px0 + m0x * ht,py0 + m0y * ht,0.f,0.f,c,0,p }; v[ 2 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p };
                v[ 3 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 4 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p }; v[ 5 ] = { px1 - m1x * ht,py1 - m1y * ht,0.f,0.f,c,0,p };
                v += 6;
            }
            f.write_offset += 18;
        }
    }

    inline void context_t::draw_polygon( float cx, float cy, float radius, int sides, color_t color ) {
        if ( sides < 3 )return; auto& f = m_frames[ m_current_frame ];
        if ( f.write_offset + sides * 3 > k_max_vertices )return;
        const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        sin_cos_lut::init( sides ); auto* v = s_staging_buffer + f.write_offset;
        auto c0 = sin_cos_lut::s_cos[ 0 ], s0 = sin_cos_lut::s_sin[ 0 ];
        for ( auto i = 0; i < sides; i++ ) {
            auto c1 = sin_cos_lut::s_cos[ i + 1 ], s1 = sin_cos_lut::s_sin[ i + 1 ];
            v[ 0 ] = { cx,cy,0.f,aa_flag,c,0,p }; v[ 1 ] = { cx + c0 * radius,cy + s0 * radius,1.f,aa_flag,c,0,p }; v[ 2 ] = { cx + c1 * radius,cy + s1 * radius,1.f,aa_flag,c,0,p }; v += 3;
            c0 = c1; s0 = s1;
        }
        f.write_offset += sides * 3;
    }

    inline void context_t::draw_polygon_outline( float cx, float cy, float radius, int sides, float thickness, color_t color ) {
        if ( sides < 3 || sides>64 )return;
        float path_x[ 64 ], path_y[ 64 ]; sin_cos_lut::init( sides );
        for ( auto i = 0; i < sides; i++ ) { path_x[ i ] = cx + sin_cos_lut::s_cos[ i ] * radius; path_y[ i ] = cy + sin_cos_lut::s_sin[ i ] * radius; }
        auto ht = thickness * 0.5f; float norm_x[ 64 ], norm_y[ 64 ], mx[ 64 ], my[ 64 ], ml[ 64 ];
        for ( auto i = 0; i < sides; i++ ) {
            auto i1 = ( i + 1 ) % sides;
            const auto dx = path_x[ i1 ] - path_x[ i ], dy = path_y[ i1 ] - path_y[ i ], l = sqrtf( dx * dx + dy * dy );
            if ( l < 0.001f ) { norm_x[ i ] = 0; norm_y[ i ] = 1; continue; }
            auto rl = 1.f / l; norm_x[ i ] = -dy * rl; norm_y[ i ] = dx * rl;
        }
        for ( auto i = 0; i < sides; i++ ) {
            auto prev = ( i + sides - 1 ) % sides;
            const auto anx = ( norm_x[ prev ] + norm_x[ i ] ) * 0.5f, any = ( norm_y[ prev ] + norm_y[ i ] ) * 0.5f, al = sqrtf( anx * anx + any * any );
            if ( al < 0.001f ) { mx[ i ] = norm_x[ i ]; my[ i ] = norm_y[ i ]; ml[ i ] = 1.f; }
            else { auto rl = 1.f / al; mx[ i ] = anx * rl; my[ i ] = any * rl; auto dot = mx[ i ] * norm_x[ i ] + my[ i ] * norm_y[ i ]; ml[ i ] = ( dot > 0.1f ) ? 1.f / dot : 10.f; if ( ml[ i ] > 4.f )ml[ i ] = 4.f; }
        }
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + sides * 18 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; const auto aab = 1.f;
            for ( auto i = 0; i < sides; i++ ) {
                auto i1 = ( i + 1 ) % sides;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ], ih = ht, oh = ht + aab;
                v[ 0 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 1 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 2 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 3 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 4 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p }; v[ 5 ] = { px1 - m1x * oh,py1 - m1y * oh,1.f,aa_flag,c,0,p };
                v[ 6 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 7 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 8 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v[ 9 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 10 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p }; v[ 11 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 12 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 13 ] = { px0 + m0x * oh,py0 + m0y * oh,1.f,aa_flag,c,0,p }; v[ 14 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p };
                v[ 15 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 16 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p }; v[ 17 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v += 18;
            }
            f.write_offset += sides * 18;
        }
        else {
            if ( f.write_offset + sides * 6 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            for ( auto i = 0; i < sides; i++ ) {
                auto i1 = ( i + 1 ) % sides;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ];
                v[ 0 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 1 ] = { px0 + m0x * ht,py0 + m0y * ht,0.f,0.f,c,0,p }; v[ 2 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p };
                v[ 3 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 4 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p }; v[ 5 ] = { px1 - m1x * ht,py1 - m1y * ht,0.f,0.f,c,0,p };
                v += 6;
            }
            f.write_offset += sides * 6;
        }
    }

    inline void context_t::draw_pie( float x, float y, float radius, float start_angle, float end_angle, color_t color, int segments ) {
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        const auto a_step = ( end_angle - start_angle ) / segments;
        if ( m_shape_aa ) {
            if ( f.write_offset + segments * 3 + 12 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; auto c0 = cosf( start_angle ), s0 = sinf( start_angle ), sc = c0, ss = s0;
            for ( auto i = 0; i < segments; i++ ) {
                auto a1 = start_angle + ( i + 1 ) * a_step, c1 = cosf( a1 ), s1 = sinf( a1 );
                v[ 0 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 1 ] = { x + c0 * radius,y + s0 * radius,1.f,aa_flag,c,0,p }; v[ 2 ] = { x + c1 * radius,y + s1 * radius,1.f,aa_flag,c,0,p }; v += 3;
                c0 = c1; s0 = s1;
            }
            auto n1x = ss, n1y = -sc;
            v[ 0 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 1 ] = { x + sc * radius,y + ss * radius,0.f,aa_flag,c,0,p }; v[ 2 ] = { x + sc * radius + n1x,y + ss * radius + n1y,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 4 ] = { x + sc * radius + n1x,y + ss * radius + n1y,1.f,aa_flag,c,0,p }; v[ 5 ] = { x + n1x,y + n1y,1.f,aa_flag,c,0,p }; v += 6;
            auto n2x = -s0, n2y = c0;
            v[ 0 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 1 ] = { x + c0 * radius,y + s0 * radius,0.f,aa_flag,c,0,p }; v[ 2 ] = { x + c0 * radius + n2x,y + s0 * radius + n2y,1.f,aa_flag,c,0,p };
            v[ 3 ] = { x,y,0.f,aa_flag,c,0,p }; v[ 4 ] = { x + c0 * radius + n2x,y + s0 * radius + n2y,1.f,aa_flag,c,0,p }; v[ 5 ] = { x + n2x,y + n2y,1.f,aa_flag,c,0,p };
            f.write_offset += segments * 3 + 12;
        }
        else {
            if ( f.write_offset + segments * 3 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; auto c0 = cosf( start_angle ), s0 = sinf( start_angle );
            for ( auto i = 0; i < segments; i++ ) {
                auto a1 = start_angle + ( i + 1 ) * a_step, c1 = cosf( a1 ), s1 = sinf( a1 );
                v[ 0 ] = { x,y,0.f,0.f,c,0,p }; v[ 1 ] = { x + c0 * radius,y + s0 * radius,0.f,0.f,c,0,p }; v[ 2 ] = { x + c1 * radius,y + s1 * radius,0.f,0.f,c,0,p }; v += 3;
                c0 = c1; s0 = s1;
            }
            f.write_offset += segments * 3;
        }
    }

    inline void context_t::draw_arc( float x, float y, float radius, float start_angle, float end_angle, float thickness, color_t color, int segments ) {
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        const auto a_step = ( end_angle - start_angle ) / segments, ir = radius - thickness * 0.5f, orr = radius + thickness * 0.5f;
        if ( m_shape_aa ) {
            if ( f.write_offset + segments * 18 > k_max_vertices )return;
            const auto aab = 1.5f; auto* v = s_staging_buffer + f.write_offset; auto c0 = cosf( start_angle ), s0 = sinf( start_angle );
            for ( auto i = 0; i < segments; i++ ) {
                auto a1 = start_angle + ( i + 1 ) * a_step, c1 = cosf( a1 ), s1 = sinf( a1 );
                auto x0ia = x + c0 * ( ir - aab ), y0ia = y + s0 * ( ir - aab ), x1ia = x + c1 * ( ir - aab ), y1ia = y + s1 * ( ir - aab );
                auto x0i = x + c0 * ir, y0i = y + s0 * ir, x1i = x + c1 * ir, y1i = y + s1 * ir;
                auto x0o = x + c0 * orr, y0o = y + s0 * orr, x1o = x + c1 * orr, y1o = y + s1 * orr;
                auto x0oa = x + c0 * ( orr + aab ), y0oa = y + s0 * ( orr + aab ), x1oa = x + c1 * ( orr + aab ), y1oa = y + s1 * ( orr + aab );
                v[ 0 ] = { x0ia,y0ia,1.f,aa_flag,c,0,p }; v[ 1 ] = { x0i,y0i,0.85f,aa_flag,c,0,p }; v[ 2 ] = { x1i,y1i,0.85f,aa_flag,c,0,p };
                v[ 3 ] = { x0ia,y0ia,1.f,aa_flag,c,0,p }; v[ 4 ] = { x1i,y1i,0.85f,aa_flag,c,0,p }; v[ 5 ] = { x1ia,y1ia,1.f,aa_flag,c,0,p };
                v[ 6 ] = { x0i,y0i,0.85f,aa_flag,c,0,p }; v[ 7 ] = { x0o,y0o,0.85f,aa_flag,c,0,p }; v[ 8 ] = { x1o,y1o,0.85f,aa_flag,c,0,p };
                v[ 9 ] = { x0i,y0i,0.85f,aa_flag,c,0,p }; v[ 10 ] = { x1o,y1o,0.85f,aa_flag,c,0,p }; v[ 11 ] = { x1i,y1i,0.85f,aa_flag,c,0,p };
                v[ 12 ] = { x0o,y0o,0.85f,aa_flag,c,0,p }; v[ 13 ] = { x0oa,y0oa,1.f,aa_flag,c,0,p }; v[ 14 ] = { x1oa,y1oa,1.f,aa_flag,c,0,p };
                v[ 15 ] = { x0o,y0o,0.85f,aa_flag,c,0,p }; v[ 16 ] = { x1oa,y1oa,1.f,aa_flag,c,0,p }; v[ 17 ] = { x1o,y1o,0.85f,aa_flag,c,0,p };
                v += 18; c0 = c1; s0 = s1;
            }
            f.write_offset += segments * 18;
        }
        else {
            if ( f.write_offset + segments * 6 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; auto c0 = cosf( start_angle ), s0 = sinf( start_angle );
            for ( auto i = 0; i < segments; i++ ) {
                auto a1 = start_angle + ( i + 1 ) * a_step, c1 = cosf( a1 ), s1 = sinf( a1 );
                auto x0i = x + c0 * ir, y0i = y + s0 * ir, x1i = x + c1 * ir, y1i = y + s1 * ir;
                auto x0o = x + c0 * orr, y0o = y + s0 * orr, x1o = x + c1 * orr, y1o = y + s1 * orr;
                v[ 0 ] = { x0i,y0i,1.f,0.f,c,0,p }; v[ 1 ] = { x0o,y0o,1.f,0.f,c,0,p }; v[ 2 ] = { x1o,y1o,1.f,0.f,c,0,p };
                v[ 3 ] = { x0i,y0i,1.f,0.f,c,0,p }; v[ 4 ] = { x1o,y1o,1.f,0.f,c,0,p }; v[ 5 ] = { x1i,y1i,1.f,0.f,c,0,p };
                v += 6; c0 = c1; s0 = s1;
            }
            f.write_offset += segments * 6;
        }
    }

    inline void context_t::draw_bezier_curve( float x0, float y0, float x1, float y1, float x2, float y2, float x3, float y3, float thickness, color_t color, int segments ) {
        if ( segments > 63 )segments = 63;
        float path_x[ 64 ], path_y[ 64 ]; auto rs = 1.f / segments;
        for ( auto i = 0; i <= segments; i++ ) {
            auto t = i * rs, it = 1.f - t;
            path_x[ i ] = it * it * it * x0 + 3 * it * it * t * x1 + 3 * it * t * t * x2 + t * t * t * x3;
            path_y[ i ] = it * it * it * y0 + 3 * it * it * t * y1 + 3 * it * t * t * y2 + t * t * t * y3;
        }
        auto n = segments + 1;
        const auto ht = thickness * 0.5f; float norm_x[ 64 ], norm_y[ 64 ], mx[ 64 ], my[ 64 ], ml[ 64 ];
        for ( auto i = 0; i < segments; i++ ) {
            auto dx = path_x[ i + 1 ] - path_x[ i ], dy = path_y[ i + 1 ] - path_y[ i ], l = sqrtf( dx * dx + dy * dy );
            if ( l < 0.001f ) { norm_x[ i ] = 0; norm_y[ i ] = 1; continue; }
            auto rl = 1.f / l; norm_x[ i ] = -dy * rl; norm_y[ i ] = dx * rl;
        }
        mx[ 0 ] = norm_x[ 0 ]; my[ 0 ] = norm_y[ 0 ]; ml[ 0 ] = 1.f;
        mx[ n - 1 ] = norm_x[ segments - 1 ]; my[ n - 1 ] = norm_y[ segments - 1 ]; ml[ n - 1 ] = 1.f;
        for ( auto i = 1; i < n - 1; i++ ) {
            auto anx = ( norm_x[ i - 1 ] + norm_x[ i ] ) * 0.5f, any = ( norm_y[ i - 1 ] + norm_y[ i ] ) * 0.5f, al = sqrtf( anx * anx + any * any );
            if ( al < 0.001f ) { mx[ i ] = norm_x[ i ]; my[ i ] = norm_y[ i ]; ml[ i ] = 1.f; }
            else { auto rl = 1.f / al; mx[ i ] = anx * rl; my[ i ] = any * rl; auto dot = mx[ i ] * norm_x[ i ] + my[ i ] * norm_y[ i ]; ml[ i ] = ( dot > 0.1f ) ? 1.f / dot : 10.f; if ( ml[ i ] > 4.f )ml[ i ] = 4.f; }
        }
        auto& f = m_frames[ m_current_frame ]; const auto c = color.pack( ), p = pack_params( 0 );
        const auto aa_flag = m_shape_aa ? 1.f : 0.f;
        if ( m_shape_aa ) {
            if ( f.write_offset + segments * 18 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset; const auto aab = 1.f;
            for ( auto i = 0; i < segments; i++ ) {
                auto i1 = i + 1;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ], ih = ht, oh = ht + aab;
                v[ 0 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 1 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 2 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 3 ] = { px0 - m0x * oh,py0 - m0y * oh,1.f,aa_flag,c,0,p }; v[ 4 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p }; v[ 5 ] = { px1 - m1x * oh,py1 - m1y * oh,1.f,aa_flag,c,0,p };
                v[ 6 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 7 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 8 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v[ 9 ] = { px0 - m0x * ih,py0 - m0y * ih,0.f,aa_flag,c,0,p }; v[ 10 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p }; v[ 11 ] = { px1 - m1x * ih,py1 - m1y * ih,0.f,aa_flag,c,0,p };
                v[ 12 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 13 ] = { px0 + m0x * oh,py0 + m0y * oh,1.f,aa_flag,c,0,p }; v[ 14 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p };
                v[ 15 ] = { px0 + m0x * ih,py0 + m0y * ih,0.f,aa_flag,c,0,p }; v[ 16 ] = { px1 + m1x * oh,py1 + m1y * oh,1.f,aa_flag,c,0,p }; v[ 17 ] = { px1 + m1x * ih,py1 + m1y * ih,0.f,aa_flag,c,0,p };
                v += 18;
            }
            f.write_offset += segments * 18;
        }
        else {
            if ( f.write_offset + segments * 6 > k_max_vertices )return;
            auto* v = s_staging_buffer + f.write_offset;
            for ( auto i = 0; i < segments; i++ ) {
                auto i1 = i + 1;
                const auto m0x = mx[ i ] * ml[ i ], m0y = my[ i ] * ml[ i ], m1x = mx[ i1 ] * ml[ i1 ], m1y = my[ i1 ] * ml[ i1 ];
                auto px0 = path_x[ i ], py0 = path_y[ i ], px1 = path_x[ i1 ], py1 = path_y[ i1 ];
                v[ 0 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 1 ] = { px0 + m0x * ht,py0 + m0y * ht,0.f,0.f,c,0,p }; v[ 2 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p };
                v[ 3 ] = { px0 - m0x * ht,py0 - m0y * ht,0.f,0.f,c,0,p }; v[ 4 ] = { px1 + m1x * ht,py1 + m1y * ht,0.f,0.f,c,0,p }; v[ 5 ] = { px1 - m1x * ht,py1 - m1y * ht,0.f,0.f,c,0,p };
                v += 6;
            }
            f.write_offset += segments * 6;
        }
    }

    inline void context_t::draw_rect_gradient( float x, float y, float w, float h, color_t tc, color_t bc ) {
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto* v = s_staging_buffer + f.write_offset; auto ct = tc.pack( ), cb = bc.pack( ), p = pack_params( 0 );
        v[ 0 ] = { x,y,0,0,ct,0,p }; v[ 1 ] = { x + w,y,0,0,ct,0,p }; v[ 2 ] = { x + w,y + h,0,0,cb,0,p };
        v[ 3 ] = { x,y,0,0,ct,0,p }; v[ 4 ] = { x + w,y + h,0,0,cb,0,p }; v[ 5 ] = { x,y + h,0,0,cb,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_rect_gradient_h( float x, float y, float w, float h, color_t lc, color_t rc2 ) {
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto* v = s_staging_buffer + f.write_offset; auto cl = lc.pack( ), cr = rc2.pack( ), p = pack_params( 0 );
        v[ 0 ] = { x,y,0,0,cl,0,p }; v[ 1 ] = { x + w,y,0,0,cr,0,p }; v[ 2 ] = { x + w,y + h,0,0,cr,0,p };
        v[ 3 ] = { x,y,0,0,cl,0,p }; v[ 4 ] = { x + w,y + h,0,0,cr,0,p }; v[ 5 ] = { x,y + h,0,0,cl,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_rect_shadow( float x, float y, float w, float h, float radius, float ox, float oy, color_t color ) {
        constexpr auto layers = 6;
        const auto base = ( float )color.a;
        const auto rpl = radius / layers;
        for ( auto i = layers; i > 0; i-- ) {
            auto t = i / ( float )layers, off = ( layers - i ) * rpl;
            color_t lc( color.r, color.g, color.b, ( uint8_t )( t * t * base ) );
            draw_rounded_rect( x + ox - off, y + oy - off, w + off * 2, h + off * 2, 12.f, lc );
        }
    }

    inline void context_t::draw_line_dashed( float x0, float y0, float x1, float y1, float thickness, float dash_length, float gap_length, color_t color ) {
        auto dx = x1 - x0, dy = y1 - y0, tl = sqrtf( dx * dx + dy * dy );
        if ( tl < 0.001f ) return;
        if ( dash_length < 0.5f ) dash_length = 0.5f;
        if ( gap_length < 0.f ) gap_length = 0.f;
        auto cyc = dash_length + gap_length;
        if ( cyc < 0.5f ) cyc = 0.5f;
        auto rl = 1.f / tl, dir_x = dx * rl, dir_y = dy * rl, pos = 0.f;
        const auto k_max_dashes = 4096;
        auto iters = 0;
        while ( pos < tl && iters++ < k_max_dashes ) {
            auto de = pos + dash_length;
            if ( de > tl ) de = tl;
            draw_line( x0 + dir_x * pos, y0 + dir_y * pos, x0 + dir_x * de, y0 + dir_y * de, thickness, color );
            pos += cyc;
        }
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Text draw / measure
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::draw_text( const char* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color, float outline_thickness, text_align align ) {
        if ( !text || !*text )return;
        auto start_x = x;
        if ( align == text_align::center )start_x = x - measure_text( text, font_name, font_size, outline_thickness ) * 0.5f;
        else if ( align == text_align::right )start_x = x - measure_text( text, font_name, font_size, outline_thickness );
        auto& f = m_frames[ m_current_frame ]; auto cx = start_x;
        const auto c = color.pack( ), oc = outline_color.pack( ), p = pack_params( 1 );
        const auto by = m_text_pixel_snap ? floorf( y + 0.5f ) : y;
        while ( *text ) {
            auto cp = decode_utf8( text );

            auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness );
            if ( !g )continue; if ( f.write_offset + 6 > k_max_vertices )break;
            auto gx = m_text_pixel_snap ? floorf( cx + g->bearing_x + 0.5f ) : ( cx + g->bearing_x );
            auto gy = by - g->bearing_y; auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 1 ] = { gx + g->width,gy,g->u1,g->v0,c,oc,p }; v[ 2 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p };
            v[ 3 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 4 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p }; v[ 5 ] = { gx,gy + g->height,g->u0,g->v1,c,oc,p };
            f.write_offset += 6; cx += g->advance_x;
        }
    }

    inline void context_t::draw_text_w( const wchar_t* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color, float outline_thickness, text_align align ) {
        if ( !text || !*text )return;
        WideCharToMultiByte( CP_UTF8, 0, text, -1, s_utf8_conv, k_utf8_conv_buf, nullptr, nullptr );
        draw_text( s_utf8_conv, x, y, font_name, font_size, color, outline_color, outline_thickness, align );
    }

    inline float context_t::draw_text_return_width( const char* text, float x, float y, const char* font_name, float font_size, color_t color, color_t outline_color, float outline_thickness ) {
        if ( !text || !*text )return 0.f;
        auto& f = m_frames[ m_current_frame ]; auto cx = x;
        const auto c = color.pack( ), oc = outline_color.pack( ), p = pack_params( 1 );
        const auto by = m_text_pixel_snap ? floorf( y + 0.5f ) : y;
        while ( *text ) {
            auto cp = decode_utf8( text ); auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness );
            if ( !g )continue; if ( f.write_offset + 6 > k_max_vertices )break;
            auto gx = m_text_pixel_snap ? floorf( cx + g->bearing_x + 0.5f ) : ( cx + g->bearing_x );
            auto gy = by - g->bearing_y; auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 1 ] = { gx + g->width,gy,g->u1,g->v0,c,oc,p }; v[ 2 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p };
            v[ 3 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 4 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p }; v[ 5 ] = { gx,gy + g->height,g->u0,g->v1,c,oc,p };
            f.write_offset += 6; cx += g->advance_x;
        }
        return cx - x;
    }

    inline void context_t::draw_text_monospace( const char* text, float x, float y, const char* font_name, float font_size, float char_width, color_t color, color_t outline_color, float outline_thickness ) {
        if ( !text || !*text )return;
        auto& f = m_frames[ m_current_frame ]; auto cx = x;
        const auto c = color.pack( ), oc = outline_color.pack( ), p = pack_params( 1 );
        const auto by = m_text_pixel_snap ? floorf( y + 0.5f ) : y;
        while ( *text ) {
            auto cp = decode_utf8( text ); auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness );
            if ( !g ) { cx += char_width; continue; } if ( f.write_offset + 6 > k_max_vertices )break;
            auto off = ( g->width > char_width ) ? ( char_width - g->width ) * 0.5f : 0.f;
            auto raw = cx + g->bearing_x + off, gx = m_text_pixel_snap ? floorf( raw + 0.5f ) : raw, gy = by - g->bearing_y;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 1 ] = { gx + g->width,gy,g->u1,g->v0,c,oc,p }; v[ 2 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p };
            v[ 3 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 4 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p }; v[ 5 ] = { gx,gy + g->height,g->u0,g->v1,c,oc,p };
            f.write_offset += 6; cx += char_width;
        }
    }

    inline void context_t::draw_text_multi_color( const char* text, float x, float y, const char* font_name, float font_size, const color_t* colors, int color_count, color_t outline_color, float outline_thickness, text_align align ) {
        if ( !text || !*text || color_count == 0 )return;
        auto start_x = x;
        if ( align == text_align::center )start_x = x - measure_text( text, font_name, font_size, outline_thickness ) * 0.5f;
        else if ( align == text_align::right )start_x = x - measure_text( text, font_name, font_size, outline_thickness );
        auto& f = m_frames[ m_current_frame ]; auto cx = start_x;
        const auto oc = outline_color.pack( ), p = pack_params( 1 );
        const auto by = m_text_pixel_snap ? floorf( y + 0.5f ) : y; auto ci = 0;
        while ( *text ) {
            auto cp = decode_utf8( text ); auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness );
            if ( !g ) { ci++; continue; } if ( f.write_offset + 6 > k_max_vertices )break;
            auto c = colors[ ci % color_count ].pack( );
            auto gx = m_text_pixel_snap ? floorf( cx + g->bearing_x + 0.5f ) : ( cx + g->bearing_x ), gy = by - g->bearing_y;
            auto* v = s_staging_buffer + f.write_offset;
            v[ 0 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 1 ] = { gx + g->width,gy,g->u1,g->v0,c,oc,p }; v[ 2 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p };
            v[ 3 ] = { gx,gy,g->u0,g->v0,c,oc,p }; v[ 4 ] = { gx + g->width,gy + g->height,g->u1,g->v1,c,oc,p }; v[ 5 ] = { gx,gy + g->height,g->u0,g->v1,c,oc,p };
            f.write_offset += 6; cx += g->advance_x; ci++;
        }
    }

    inline float context_t::measure_text( const char* text, const char* font_name, float font_size, float outline_thickness ) {
        if ( !text || !*text )return 0.f; auto w = 0.f;
        while ( *text ) { auto cp = decode_utf8( text ); auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness ); if ( g )w += g->advance_x; }
        return w;
    }

    inline float context_t::measure_text_height( const char* font_name, float font_size, float outline_thickness ) {
        auto* g = get_or_create_glyph( font_name, font_size, 'M', outline_thickness );
        return g ? g->line_height + outline_thickness * 2.f : ( float )font_size + outline_thickness * 2.f;
    }

    inline float context_t::measure_text_to_column( const char* text, const char* font_name, float font_size, int column, float outline_thickness ) {
        if ( !text || !*text || column <= 0 )return 0.f; auto w = 0.f;
        auto col = 0;
        while ( *text && col < column ) { auto cp = decode_utf8( text ); auto* g = get_or_create_glyph( font_name, font_size, cp, outline_thickness ); if ( g )w += g->advance_x; col++; }
        return w;
    }

    inline int context_t::get_codepoint_count( const char* text ) {
        if ( !text )return 0; auto n = 0;
        while ( *text ) { auto b = ( unsigned char )*text; if ( b < 0x80 )text += 1; else if ( ( b & 0xE0 ) == 0xC0 )text += 2; else if ( ( b & 0xF0 ) == 0xE0 )text += 3; else if ( ( b & 0xF8 ) == 0xF0 )text += 4; else text += 1; n++; }
        return n;
    }

    inline float context_t::get_codepoint_advance( const char* font_name, float font_size, uint32_t codepoint, float outline_thickness ) {
        auto* g = get_or_create_glyph( font_name, font_size, codepoint, outline_thickness ); return g ? g->advance_x : 0.f;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Texture registration / drawing
    // ═══════════════════════════════════════════════════════════════════════

    inline uint32_t context_t::register_texture( const uint8_t* rgba_data, int width, int height ) {
        if ( !rgba_data || width <= 0 || height <= 0 )return 0;
        int px, py; if ( !m_tex_atlas_packer.pack( width, height, px, py ) )return 0;
        for ( auto y = 0; y < height; y++ ) { auto src = y * width * 4, dst = ( ( py + y ) * k_atlas_size + px ) * 4; memcpy( s_tex_atlas_data + dst, rgba_data + src, width * 4 ); }
        texture_handle_t h;
        h.u0 = ( float )px / k_atlas_size; h.v0 = ( float )py / k_atlas_size;
        h.u1 = ( float )( px + width ) / k_atlas_size; h.v1 = ( float )( py + height ) / k_atlas_size;
        h.width = ( float )width; h.height = ( float )height; h.valid = true;
        auto id = m_next_tex_id++; auto* slot = m_textures.insert( id ); if ( !slot )return 0;
        *slot = h; m_tex_atlas_needs_upload = true; return id;
    }

    inline uint32_t context_t::register_texture_from_memory( const void* data, size_t data_size ) {
        if ( !data || !data_size || !m_wic_factory )return 0;
        IWICStream* stream = nullptr; if ( FAILED( m_wic_factory->CreateStream( &stream ) ) )return 0;
        if ( FAILED( stream->InitializeFromMemory( ( BYTE* )data, ( DWORD )data_size ) ) ) { stream->Release( ); return 0; }
        IWICBitmapDecoder* decoder = nullptr;
        if ( FAILED( m_wic_factory->CreateDecoderFromStream( stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder ) ) ) { stream->Release( ); return 0; }
        stream->Release( );
        IWICBitmapFrameDecode* frame = nullptr; if ( FAILED( decoder->GetFrame( 0, &frame ) ) ) { decoder->Release( ); return 0; }
        IWICFormatConverter* conv = nullptr; if ( FAILED( m_wic_factory->CreateFormatConverter( &conv ) ) ) { frame->Release( ); decoder->Release( ); return 0; }
        if ( FAILED( conv->Initialize( frame, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) ) { conv->Release( ); frame->Release( ); decoder->Release( ); return 0; }
        UINT w = 0, h = 0; conv->GetSize( &w, &h );
        auto sz = ( size_t )w * h * 4; if ( sz > k_image_scratch_sz ) { conv->Release( ); frame->Release( ); decoder->Release( ); return 0; }
        conv->CopyPixels( nullptr, w * 4, ( UINT )sz, s_image_scratch );
        conv->Release( ); frame->Release( ); decoder->Release( );
        return register_texture( s_image_scratch, ( int )w, ( int )h );
    }

    inline void context_t::unregister_texture( uint32_t id ) { auto* h = m_textures.find( id ); if ( h )h->valid = false; }

    inline void context_t::clear_texture_atlas( ) {
        m_tex_atlas_packer.init( k_atlas_size, k_atlas_size );
        m_textures.clear( ); m_next_tex_id = 1;
        m_anim_textures.clear( ); m_next_anim_id = 1;
        memset( s_tex_atlas_data, 0, k_tex_atlas_bytes ); m_tex_atlas_needs_upload = true;
    }

    inline bool context_t::get_texture_size( uint32_t id, float& ow, float& oh )const {
        auto* h = const_cast< texture_map_t& >( m_textures ).find( id );
        if ( !h || !h->valid )return false; ow = h->width; oh = h->height; return true;
    }

    inline void context_t::draw_texture( uint32_t id, float x, float y, float w, float h, color_t tint ) {
        auto* tex = m_textures.find( id ); if ( !tex || !tex->valid )return;
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto* v = s_staging_buffer + f.write_offset; auto c = tint.pack( ), p = pack_params( 2 );
        auto x1 = x + w, y1 = y + h;
        v[ 0 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 1 ] = { x1,y,tex->u1,tex->v0,c,0,p }; v[ 2 ] = { x1,y1,tex->u1,tex->v1,c,0,p };
        v[ 3 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 4 ] = { x1,y1,tex->u1,tex->v1,c,0,p }; v[ 5 ] = { x,y1,tex->u0,tex->v1,c,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_texture_rect( uint32_t id, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h, color_t tint ) {
        auto* tex = m_textures.find( id ); if ( !tex || !tex->valid )return;
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto tw = tex->u1 - tex->u0, th = tex->v1 - tex->v0;
        auto su0 = tex->u0 + ( src_x / tex->width ) * tw, sv0 = tex->v0 + ( src_y / tex->height ) * th;
        auto su1 = tex->u0 + ( ( src_x + src_w ) / tex->width ) * tw, sv1 = tex->v0 + ( ( src_y + src_h ) / tex->height ) * th;
        auto* v = s_staging_buffer + f.write_offset; auto c = tint.pack( ), p = pack_params( 2 );
        auto x1 = x + w, y1 = y + h;
        v[ 0 ] = { x,y,su0,sv0,c,0,p }; v[ 1 ] = { x1,y,su1,sv0,c,0,p }; v[ 2 ] = { x1,y1,su1,sv1,c,0,p };
        v[ 3 ] = { x,y,su0,sv0,c,0,p }; v[ 4 ] = { x1,y1,su1,sv1,c,0,p }; v[ 5 ] = { x,y1,su0,sv1,c,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_mask_alpha( uint32_t id, float x, float y, float w, float h, color_t color ) {
        auto* tex = m_textures.find( id ); if ( !tex || !tex->valid )return;
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto c = color.pack( ), p = pack_params( 3 );
        auto x1 = x + w, y1 = y + h; auto* v = s_staging_buffer + f.write_offset;
        v[ 0 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 1 ] = { x1,y,tex->u1,tex->v0,c,0,p }; v[ 2 ] = { x1,y1,tex->u1,tex->v1,c,0,p };
        v[ 3 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 4 ] = { x1,y1,tex->u1,tex->v1,c,0,p }; v[ 5 ] = { x,y1,tex->u0,tex->v1,c,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_mask_luminance( uint32_t id, float x, float y, float w, float h, color_t color ) {
        auto* tex = m_textures.find( id ); if ( !tex || !tex->valid )return;
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto c = color.pack( ), p = pack_params( 4 );
        auto x1 = x + w, y1 = y + h; auto* v = s_staging_buffer + f.write_offset;
        v[ 0 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 1 ] = { x1,y,tex->u1,tex->v0,c,0,p }; v[ 2 ] = { x1,y1,tex->u1,tex->v1,c,0,p };
        v[ 3 ] = { x,y,tex->u0,tex->v0,c,0,p }; v[ 4 ] = { x1,y1,tex->u1,tex->v1,c,0,p }; v[ 5 ] = { x,y1,tex->u0,tex->v1,c,0,p };
        f.write_offset += 6;
    }

    inline void context_t::draw_mask_alpha_outline( uint32_t id, float x, float y, float w, float h, color_t mask_color, color_t outline_color, float thickness ) {
        auto* tex = m_textures.find( id ); if ( !tex || !tex->valid )return;
        auto& f = m_frames[ m_current_frame ]; if ( f.write_offset + 6 > k_max_vertices )return;
        auto ct = thickness < 0.5f ? 0.5f : ( thickness > 4.f ? 4.f : thickness );
        auto tb = ( uint32_t )( ( ct - 0.5f ) * 2.f + 0.5f ); if ( tb > 7 )tb = 7;
        auto mc = mask_color.pack( ), oc = outline_color.pack( ), p = pack_params( 5 ) | ( tb << 12 );
        auto x1 = x + w, y1 = y + h;
        auto* v = s_staging_buffer + f.write_offset;
        v[ 0 ] = { x,y,tex->u0,tex->v0,mc,oc,p }; v[ 1 ] = { x1,y,tex->u1,tex->v0,mc,oc,p }; v[ 2 ] = { x1,y1,tex->u1,tex->v1,mc,oc,p };
        v[ 3 ] = { x,y,tex->u0,tex->v0,mc,oc,p }; v[ 4 ] = { x1,y1,tex->u1,tex->v1,mc,oc,p }; v[ 5 ] = { x,y1,tex->u0,tex->v1,mc,oc,p };
        f.write_offset += 6;
    }

    // ═══════════════════════════════════════════════════════════════════════
    // Animated textures
    // ═══════════════════════════════════════════════════════════════════════

    inline uint32_t context_t::register_animated_texture_from_memory( const void* data, size_t data_size ) {
        if ( !data || !data_size || !m_wic_factory )return 0;
        IWICStream* stream = nullptr; if ( FAILED( m_wic_factory->CreateStream( &stream ) ) )return 0;
        if ( FAILED( stream->InitializeFromMemory( ( BYTE* )data, ( DWORD )data_size ) ) ) { stream->Release( ); return 0; }
        IWICBitmapDecoder* decoder = nullptr;
        if ( FAILED( m_wic_factory->CreateDecoderFromStream( stream, nullptr, WICDecodeMetadataCacheOnDemand, &decoder ) ) ) { stream->Release( ); return 0; }
        stream->Release( );
        UINT frame_count = 0; decoder->GetFrameCount( &frame_count ); if ( !frame_count ) { decoder->Release( ); return 0; }
        UINT canvas_w = 0, canvas_h = 0;
        IWICMetadataQueryReader* global_meta = nullptr;
        if ( SUCCEEDED( decoder->GetMetadataQueryReader( &global_meta ) ) ) {
            PROPVARIANT pv; PropVariantInit( &pv );
            if ( SUCCEEDED( global_meta->GetMetadataByName( L"/logscrdesc/Width", &pv ) ) && pv.vt == VT_UI2 )canvas_w = pv.uiVal; PropVariantClear( &pv ); PropVariantInit( &pv );
            if ( SUCCEEDED( global_meta->GetMetadataByName( L"/logscrdesc/Height", &pv ) ) && pv.vt == VT_UI2 )canvas_h = pv.uiVal; PropVariantClear( &pv ); global_meta->Release( );
        }
        if ( !canvas_w || !canvas_h ) { IWICBitmapFrameDecode* ff = nullptr; if ( SUCCEEDED( decoder->GetFrame( 0, &ff ) ) ) { ff->GetSize( &canvas_w, &canvas_h ); ff->Release( ); } }
        if ( !canvas_w || !canvas_h ) { decoder->Release( ); return 0; }
        if ( ( size_t )canvas_w * canvas_h * 4 > k_image_scratch_sz / 2 ) { decoder->Release( ); return 0; }
        auto* canvas = s_image_scratch, * prev_canvas = s_image_scratch + k_image_scratch_sz / 2;
        auto canvas_sz = ( size_t )canvas_w * canvas_h * 4;
        memset( canvas, 0, canvas_sz ); memset( prev_canvas, 0, canvas_sz );
        animated_texture_handle_t anim = {};
        anim.loop_count = 0; anim.playing = true; anim.valid = true;
        anim.width = ( float )canvas_w; anim.height = ( float )canvas_h;
        auto max_f = frame_count < k_max_animation_frames ? frame_count : k_max_animation_frames;
        for ( UINT i = 0; i < max_f; i++ ) {
            IWICBitmapFrameDecode* frm = nullptr; if ( FAILED( decoder->GetFrame( i, &frm ) ) )continue;
            auto delay = 100u, disposal = 0u; UINT f_l = 0, f_t = 0, f_w = 0, f_h = 0; frm->GetSize( &f_w, &f_h );
            IWICMetadataQueryReader* mr = nullptr;
            if ( SUCCEEDED( frm->GetMetadataQueryReader( &mr ) ) ) {
                PROPVARIANT pv;
                PropVariantInit( &pv ); if ( SUCCEEDED( mr->GetMetadataByName( L"/grctlext/Delay", &pv ) ) && pv.vt == VT_UI2 ) { delay = pv.uiVal * 10; if ( !delay )delay = 100; }PropVariantClear( &pv );
                PropVariantInit( &pv ); if ( SUCCEEDED( mr->GetMetadataByName( L"/grctlext/Disposal", &pv ) ) && pv.vt == VT_UI1 ) { disposal = pv.bVal; }PropVariantClear( &pv );
                PropVariantInit( &pv ); if ( SUCCEEDED( mr->GetMetadataByName( L"/imgdesc/Left", &pv ) ) && pv.vt == VT_UI2 ) { f_l = pv.uiVal; }PropVariantClear( &pv );
                PropVariantInit( &pv ); if ( SUCCEEDED( mr->GetMetadataByName( L"/imgdesc/Top", &pv ) ) && pv.vt == VT_UI2 ) { f_t = pv.uiVal; }PropVariantClear( &pv );
                mr->Release( );
            }
            IWICFormatConverter* conv = nullptr; if ( FAILED( m_wic_factory->CreateFormatConverter( &conv ) ) ) { frm->Release( ); continue; }
            if ( FAILED( conv->Initialize( frm, GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0, WICBitmapPaletteTypeCustom ) ) ) { conv->Release( ); frm->Release( ); continue; }
            auto f_pix_sz = ( size_t )f_w * f_h * 4;
            if ( canvas_sz + f_pix_sz > k_image_scratch_sz ) { conv->Release( ); frm->Release( ); continue; }
            auto* f_pixels = s_image_scratch + canvas_sz;
            conv->CopyPixels( nullptr, f_w * 4, ( UINT )f_pix_sz, f_pixels );
            conv->Release( ); frm->Release( );
            if ( disposal == 3 )memcpy( prev_canvas, canvas, canvas_sz );
            for ( UINT fy = 0; fy < f_h; fy++ ) {
                auto cy2 = f_t + fy; if ( cy2 >= canvas_h )break;
                for ( UINT fx = 0; fx < f_w; fx++ ) {
                    auto cx2 = f_l + fx; if ( cx2 >= canvas_w )break;
                    auto si = ( ( size_t )fy * f_w + fx ) * 4, di = ( ( size_t )cy2 * canvas_w + cx2 ) * 4;
                    if ( f_pixels[ si + 3 ] > 0 ) { canvas[ di ] = f_pixels[ si ]; canvas[ di + 1 ] = f_pixels[ si + 1 ]; canvas[ di + 2 ] = f_pixels[ si + 2 ]; canvas[ di + 3 ] = f_pixels[ si + 3 ]; }
                }
            }
            auto fid = register_texture( canvas, ( int )canvas_w, ( int )canvas_h ); if ( !fid )continue;
            anim.frame_ids[ anim.frame_count ] = fid; anim.delays[ anim.frame_count ] = delay; anim.frame_count++;
            if ( disposal == 2 ) { for ( UINT fy = 0; fy < f_h; fy++ ) { auto cy2 = f_t + fy; if ( cy2 >= canvas_h )break; for ( UINT fx = 0; fx < f_w; fx++ ) { auto cx2 = f_l + fx; if ( cx2 >= canvas_w )break; auto di = ( ( size_t )cy2 * canvas_w + cx2 ) * 4; canvas[ di ] = canvas[ di + 1 ] = canvas[ di + 2 ] = canvas[ di + 3 ] = 0; } } }
            else if ( disposal == 3 ) { memcpy( canvas, prev_canvas, canvas_sz ); }
        }
        decoder->Release( ); if ( !anim.frame_count )return 0;
        auto anim_id = m_next_anim_id++; auto* slot = m_anim_textures.insert( anim_id ); if ( !slot )return 0;
        *slot = anim; return anim_id;
    }

    inline void context_t::draw_animated_texture( uint32_t anim_id, float x, float y, float w, float h, float delta_time, color_t tint ) {
        auto* anim = m_anim_textures.find( anim_id );
        if ( !anim || !anim->valid ) return;
        if ( anim->playing && delta_time > 0.f ) {
            if ( delta_time > 0.5f ) delta_time = 0.5f;      // clamp runaway delta (max half second)
            anim->current_time += ( double )delta_time * 1000.0;
            auto fe = 0.0;
            for ( auto i = 0u; i <= anim->current_frame && i < anim->frame_count; i++ )
                fe += anim->delays[ i ];
            const auto k_max_advance = 256;                 // cap to one full cycle
            auto iters = 0;
            while ( anim->current_time >= fe && anim->current_frame < anim->frame_count && iters++ < k_max_advance ) {
                anim->current_frame++;
                if ( anim->current_frame >= anim->frame_count ) {
                    if ( anim->loop_count == 0 || anim->loop_counter < anim->loop_count - 1 ) {
                        anim->current_frame = 0;
                        anim->current_time = 0.0;
                        anim->loop_counter++;
                        fe = anim->delays[ 0 ];
                    }
                    else {
                        anim->current_frame = anim->frame_count - 1;
                        anim->playing = false;
                        break;
                    }
                }
                else { fe += anim->delays[ anim->current_frame ]; }
            }
        }
        if ( anim->current_frame < anim->frame_count )
            draw_texture( anim->frame_ids[ anim->current_frame ], x, y, w, h, tint );
    }

    inline void context_t::reset_animation( uint32_t anim_id ) { auto* a = m_anim_textures.find( anim_id ); if ( !a )return; a->current_time = 0.0; a->current_frame = 0; a->loop_counter = 0; a->playing = true; }
    inline void context_t::set_animation_playing( uint32_t anim_id, bool playing ) { auto* a = m_anim_textures.find( anim_id ); if ( a )a->playing = playing; }
    inline void context_t::unregister_animated_texture( uint32_t anim_id ) { auto* a = m_anim_textures.find( anim_id ); if ( !a || !a->valid )return; for ( auto i = 0u; i < a->frame_count; i++ )unregister_texture( a->frame_ids[ i ] ); a->valid = false; a->frame_count = 0; }
    inline bool context_t::get_animated_texture_size( uint32_t anim_id, float& ow, float& oh )const { auto* a = const_cast< anim_map_t& >( m_anim_textures ).find( anim_id ); if ( !a || !a->valid )return false; ow = a->width; oh = a->height; return true; }

    // ═══════════════════════════════════════════════════════════════════════
    // Clip / scissor
    // ═══════════════════════════════════════════════════════════════════════

    inline void context_t::push_clip_rect( float x, float y, float w, float h ) {
        if ( m_clip_next_index >= k_max_clip_rects || m_clip_stack_depth >= k_max_clip_rects )return;
        if ( m_clip_stack_depth > 0 ) {
            auto pi = m_clip_index_stack[ m_clip_stack_depth - 1 ]; const auto& par = m_clip_rects[ pi ];
            auto nl = x > par.x ? x : par.x, nt = y > par.y ? y : par.y;
            auto nr = ( x + w < par.x + par.w ) ? x + w : par.x + par.w, nb = ( y + h < par.y + par.h ) ? y + h : par.y + par.h;
            x = nl; y = nt; w = nr - nl; if ( w < 0 )w = 0; h = nb - nt; if ( h < 0 )h = 0;
        }
        auto ni = m_clip_next_index++; m_clip_rects[ ni ] = { x,y,w,h };
        m_clip_index_stack[ m_clip_stack_depth++ ] = ni; m_clip_rects_dirty = true;
    }

    inline void context_t::pop_clip_rect( ) { if ( m_clip_stack_depth > 0 )m_clip_stack_depth--; }
    inline void context_t::clear_clip_stack( ) { m_clip_stack_depth = 0; m_clip_next_index = 1; }
    inline void context_t::set_scissor_rect( float x, float y, float w, float h ) { m_scissor_enabled = true; m_scissor_rect = { ( LONG )x,( LONG )y,( LONG )( x + w ),( LONG )( y + h ) }; }
    inline void context_t::clear_scissor_rect( ) { m_scissor_enabled = false; }

} // namespace peach

// ─────────────────────────────────────────────────────────────────────────────
// Global instance
// ─────────────────────────────────────────────────────────────────────────────
inline std::byte g_draw_storage[ sizeof( peach::context_t ) ]{};
inline peach::context_t* g_interface = reinterpret_cast< peach::context_t* >( g_draw_storage );
