#pragma once
#include <dwrite_3.h>
#include <array>

// -- no-CRT type aliases -------------------------------------------------------
using fn_sprintf_s = int( __cdecl* )( char*, size_t, const char*, ... );
using fn_swprintf_s = int( __cdecl* )( wchar_t*, size_t, const wchar_t*, ... );
using fn_strlen = int( __cdecl* )( const char* );
using fn_tolower = int( __cdecl* )( int );
using fn_strstr = char* ( __cdecl* )( const char*, const char* );
using fn_GetTickCount = DWORD( WINAPI* )( );
using fn_GetLocalTime = void  ( WINAPI* )( LPSYSTEMTIME );

namespace crt {
    inline fn_strlen       strlen = nullptr;
    inline fn_tolower      tolower = nullptr;
    inline fn_strstr       strstr = nullptr;
    inline fn_GetTickCount GetTickCount = nullptr;
    inline fn_GetLocalTime GetLocalTime = nullptr;

    inline bool init( ) {
        auto msvcrt = GetModuleHandleA( "msvcrt.dll" );
        auto ucrtb = GetModuleHandleA( "ucrtbase.dll" );
        auto kernel = GetModuleHandleA( "kernel32.dll" );
        auto crtmod = ucrtb ? ucrtb : msvcrt;
        if ( !crtmod || !kernel ) return false;

        strlen = ( fn_strlen )GetProcAddress( crtmod, "strlen" );
        tolower = ( fn_tolower )GetProcAddress( crtmod, "tolower" );
        strstr = ( fn_strstr )GetProcAddress( crtmod, "strstr" );
        GetTickCount = ( fn_GetTickCount )GetProcAddress( kernel, "GetTickCount" );
        GetLocalTime = ( fn_GetLocalTime )GetProcAddress( kernel, "GetLocalTime" );

        return strlen && tolower && strstr && GetTickCount && GetLocalTime;
    }
}

// -- user profile --------------------------------------------------------------
namespace profile {
    inline char display_name[ 64 ] = "Luo Yie";
    inline char initials[ 3 ] = "LY";
    inline bool initialized = false;

    inline void set_name( const char* name ) {
        int i = 0;
        while ( name[ i ] && i < 63 ) { display_name[ i ] = name[ i ]; i++; }
        display_name[ i ] = '\0';

        int  ic = 0;
        bool next_is_initial = true;
        for ( int j = 0; name[ j ] && ic < 2; j++ ) {
            if ( name[ j ] == ' ' ) { next_is_initial = true; }
            else if ( next_is_initial ) {
                char ch = name[ j ];
                if ( ch >= 'a' && ch <= 'z' ) ch -= 32;
                initials[ ic++ ] = ch;
                next_is_initial = false;
            }
        }
        if ( ic == 1 && name[ 1 ] ) {
            char ch = name[ 1 ];
            if ( ch >= 'a' && ch <= 'z' ) ch -= 32;
            initials[ ic++ ] = ch;
        }
        initials[ ic ] = '\0';
    }

    inline void set_name( const wchar_t* name ) {
        char narrow[ 64 ]{};
        int  i = 0;
        while ( name[ i ] && i < 63 ) { narrow[ i ] = ( char )name[ i ]; i++; }
        narrow[ i ] = '\0';
        set_name( narrow );
    }
}

namespace icons {
    static std::string to_utf8( uint32_t cp ) {
        char buf[ 5 ] = {};
        buf[ 0 ] = ( char )( 0xE0 | ( cp >> 12 ) );
        buf[ 1 ] = ( char )( 0x80 | ( ( cp >> 6 ) & 0x3F ) );
        buf[ 2 ] = ( char )( 0x80 | ( cp & 0x3F ) );
        return std::string( buf );
    }

    inline const std::string aimbot = to_utf8( 0xf8cc );
    inline const std::string visuals = to_utf8( 0xf06e );
    inline const std::string exploit = to_utf8( 0xf1e2 );
    inline const std::string misc = to_utf8( 0xf085 );
}

namespace ico {
    constexpr const char* font = "icomoon";
    constexpr const char* aimbot = "\ue9ce";
}

struct combo_state_t {
    bool  open = false;
    float anim = 0.f;
    const wchar_t* items[ 16 ]{};
    int   count = 0;
};

// -- settings ------------------------------------------------------------------
namespace settings {
    namespace aimbot {
        inline bool  enabled = true;
        inline bool  bullet_tp = false;
        inline bool  multiple_damage = true;
        inline bool  silent_aim = false;
        inline bool  aim_prediction = false;

        inline bool  visible_only = true;
        inline bool  aim_at_downed = false;
        inline bool  aim_at_bots = true;
        inline float fov = 15.f;
        inline float deadzone = 2.f;
        inline float smooth_factor = 0.35f;
        inline bool  bone_head = true;
        inline bool  bone_neck = false;
        inline bool  bone_chest = false;
        inline bool  bone_pelvis = false;
        inline float crosshair_weight = 0.65f;
        inline float distance_weight = 0.35f;
        inline float damage_multiplier = 50.f;
        inline bool  damage_zero_spread = false;
        inline bool  weapon_only = false;
        inline bool  prefer_visible = true;

        inline int   target_bone_mode = 1;           // 0=Auto 1=Head 2=Neck 3=Chest
        inline combo_state_t target_bone_combo{};

        inline int   aim_key = 0;                    // 0=Hold RMB  1=Hold LMB  2=Toggle  3=Always
        inline combo_state_t aim_key_combo{};
    }
    namespace visuals {
        inline bool  enabled = true;
        inline bool  bbox_enabled = true;
        inline bool  threed_enabled = false;
        inline bool  bbox_filled = true;
        inline bool  corner_box = false;
        inline bool  directional_box = false;
        inline float bbox_thickness = 2.0f;
        inline bool  skeleton = true;
        inline bool  china_hat = false;
        inline float skeleton_thickness = 2.0f;
        inline bool  show_name = true;
        inline bool  show_rank = false;
        inline bool  show_icons = true;
        inline bool  show_kills = true;
        inline bool  show_level = true;
        inline bool  show_distance = true;
        inline bool  show_weapon = true;
        inline bool  show_platform = true;
        inline bool  look_direction = false;
        inline bool  thread_highlight = true;
        inline bool  show_reloading = true;
        inline bool  show_firing = true;
        inline bool  fov_circle = true;
        inline float fov_circle_thickness = 1.5f;
        inline bool  snaplines = false;
        inline float font_size = 14.f;
        inline bool  off_screen_arrows = true;
        inline bool  color_by_visibility = true;
        inline bool  color_teammates = true;
        inline bool  health_bar = false;
        inline float max_esp_distance = 500.f;
    }
    namespace chams {
        inline bool  enabled = false;
        inline bool  weapon = false;
        inline bool  wireframe = true;
        inline bool  ignore_depth = true;
        inline bool  two_sided = true;
        inline bool  dynamic_area_light = true;
        inline float color_r = 0.0f;
        inline float color_g = 2.0f;
        inline float color_b = 5.0f;
        bool pickups = false;
    }
    namespace tracers {
        inline bool  enabled = true;
        inline float max_age = 2.0f;
        inline float line_thickness = 2.0f;
        inline float impact_dot_radius = 4.0f;
        inline bool  deduplicate = true;
        inline float min_spawn_interval = 0.05f;
    }
    namespace trails {
        inline bool  enabled = true;
        inline float duration = 1.5f;
        inline float push_interval = 0.025f;
        inline float line_thickness = 1.5f;
    }
    namespace misc {
        inline bool  item_esp = true;
        inline float item_size = 14.f;
        inline bool  show_distance = true;
        inline float item_max_distance = 200.f;
        inline float chest_max_distance = 250.f;
        inline float ammo_max_distance = 150;
        inline float vehicle_distance = 100.f;
        inline float supplydrop_distance = 300.f;
        inline bool  show_chest = true;
        inline bool  show_ammobox = true;
        inline bool  show_ground_loot = true;
        inline bool  show_vehicles = false;
        inline bool  show_weakspot = true;
        inline bool  target_weakspot = true;
        inline bool  show_supplydrops = false;
        inline bool  particles_enabled = true;
        inline float particle_spawn_rate = 0.02f;
        inline bool  fancy_particles = true;
        inline bool  watermark = true;
        inline bool  watermark_shadow = true;
        inline bool  show_fps = true;
        inline bool  menu_shadow = true;
    }
}

// -- menu / ui state -----------------------------------------------------------
struct menu_state_t {
    bool  open = false;
    float x = 200.f, y = 130.f;
    float sidebar_w = 170.f;
    float height = 520.f;
    bool  dragging = false;
    float drag_off_x = 0.f, drag_off_y = 0.f;
    int   active_nav = 0, active_tab = 0;
};
inline menu_state_t g_menu;

struct search_state_t {
    char  buf[ 64 ]{};
    int   len = 0;
    bool  focused = false;
    float cursor_blink = 0.f;
};
inline search_state_t g_search;

namespace ui {
    constexpr const char* k_font = "Poppins";
    constexpr const char* k_font_bold = "Poppins Semibold";
    constexpr float k_font_sz = 12.f;
    constexpr float k_pad = 10.f;
    constexpr float k_item_h = 26.f;
    constexpr float k_inner_pad = 12.f;

    constexpr peach::color_t k_accent{ 75,  87,  219, 255 };
    constexpr peach::color_t k_accent2{ 94,  105, 238, 255 };
    constexpr peach::color_t k_accent3{ 179, 136, 235, 255 };
    constexpr peach::color_t k_accent4{ 179, 136, 235, 255 };
    constexpr peach::color_t k_text{ 232, 232, 232, 255 };
    constexpr peach::color_t k_text_dim{ 136, 136, 136, 180 };
    constexpr peach::color_t k_hot{ 255, 255, 255, 200 };
    constexpr peach::color_t k_border{ 46,  46,  46,  255 };

    struct cursor_t { float x = 0, y = 0; bool ldown = false, lclick = false; };
    inline cursor_t g_cursor;
    inline bool     g_prev_ldown = false;

    inline bool hovered( float x, float y, float w, float h ) {
        return g_cursor.x >= x && g_cursor.x <= x + w && g_cursor.y >= y && g_cursor.y <= y + h;
    }

    inline void poll( ) {
        // -- cursor ----------------------------------------------------------------
        POINT pt{};
        GetCursorPos( &pt );
        g_cursor.x = ( float )pt.x;
        g_cursor.y = ( float )pt.y;

        bool cur = ( GetAsyncKeyState( VK_LBUTTON ) & 0x8000 ) != 0;
        g_cursor.lclick = cur && !g_prev_ldown;
        g_cursor.ldown = cur;
        g_prev_ldown = cur;

        // -- backspace / escape ----------------------------------------------------
        if ( GetAsyncKeyState( VK_BACK ) & 1 )
            if ( g_search.len > 0 )
                g_search.buf[ --g_search.len ] = '\0';

        if ( GetAsyncKeyState( VK_ESCAPE ) & 1 ) {
            g_search.len = 0;
            g_search.buf[ 0 ] = '\0';
            g_search.focused = false;
            return;
        }

        if ( g_search.len >= 63 )
            return;

        // -- keyboard state — one syscall, static buffer ---------------------------
        static BYTE ks[ 256 ]{};
        static BYTE ks_prev[ 256 ]{};
        memcpy( ks_prev, ks, 256 );
        GetKeyboardState( ks );

        if ( ks[ VK_CONTROL ] & 0x80 )
            return;

        bool shift = ( ks[ VK_SHIFT ] & 0x80 ) != 0;

        // letters — edge detect via prev state, no extra syscalls
        for ( int vk = 'A'; vk <= 'Z'; vk++ ) {
            if ( !( ks[ vk ] & 0x80 ) ) continue;
            if ( ( ks_prev[ vk ] & 0x80 ) ) continue; // was already held last frame
            if ( g_search.len >= 63 ) break;
            char ch = shift ? ( char )vk : ( char )( vk + 32 );
            g_search.buf[ g_search.len++ ] = ch;
            g_search.buf[ g_search.len ] = '\0';
        }

        // digits
        for ( int vk = '0'; vk <= '9'; vk++ ) {
            if ( !( ks[ vk ] & 0x80 ) ) continue;
            if ( ( ks_prev[ vk ] & 0x80 ) ) continue;
            if ( g_search.len >= 63 ) break;
            g_search.buf[ g_search.len++ ] = ( char )vk;
            g_search.buf[ g_search.len ] = '\0';
        }

        // space
        if ( ( ks[ VK_SPACE ] & 0x80 ) && !( ks_prev[ VK_SPACE ] & 0x80 )
            && g_search.len < 63 ) {
            g_search.buf[ g_search.len++ ] = ' ';
            g_search.buf[ g_search.len ] = '\0';
        }
    }
}

// -- tab history ---------------------------------------------------------------
struct tab_state_t { int nav, tab; };
inline tab_state_t s_history[ 32 ]{};
inline int s_history_len = 0, s_history_pos = -1;

inline void history_push( int nav, int tab ) {
    s_history_len = s_history_pos + 1;
    if ( s_history_pos >= 0 && s_history[ s_history_pos ].nav == nav && s_history[ s_history_pos ].tab == tab ) return;
    if ( s_history_len >= 32 ) {
        for ( int i = 0; i < 31; i++ ) s_history[ i ] = s_history[ i + 1 ];
        s_history_len = 31; s_history_pos = 30;
    }
    s_history[ s_history_len ] = { nav, tab };
    s_history_pos = s_history_len++;
}
inline void history_back( ) {
    if ( s_history_pos <= 0 ) return;
    --s_history_pos;
    g_menu.active_nav = s_history[ s_history_pos ].nav;
    g_menu.active_tab = s_history[ s_history_pos ].tab;
}
inline void history_forward( ) {
    if ( s_history_pos >= s_history_len - 1 ) return;
    ++s_history_pos;
    g_menu.active_nav = s_history[ s_history_pos ].nav;
    g_menu.active_tab = s_history[ s_history_pos ].tab;
}
inline bool history_can_back( ) { return s_history_pos > 0; }
inline bool history_can_forward( ) { return s_history_pos < s_history_len - 1; }

// -- search items --------------------------------------------------------------
struct search_item_t { const char* label; const char* section; int nav; int tab; };
inline constexpr search_item_t k_search_items[ ] = {
    // Aimbot
    {"Enable Aimbot",    "Aimbot", 0, 0}, {"Silent Aim",      "Aimbot", 0, 0},
    {"Visible Only",     "Aimbot", 0, 0}, {"Aim at Downed",   "Aimbot", 0, 0},
    {"Aim at Bots",      "Aimbot", 0, 0}, {"FOV",             "Aimbot", 0, 0},
    {"Smooth Factor",    "Aimbot", 0, 0}, {"Prefer Visible",  "Aimbot", 0, 0},
    // Bone priority
    {"Head",    "Bone Priority", 0, 0}, {"Neck",   "Bone Priority", 0, 0},
    {"Chest",   "Bone Priority", 0, 0}, {"Pelvis", "Bone Priority", 0, 0},
    // Scoring
    {"Crosshair Weight", "Scoring", 0, 0}, {"Distance Weight", "Scoring", 0, 0},
    // Player ESP
    {"Bounding Box",      "Player ESP", 1, 0}, {"Filled Box",       "Player ESP", 1, 0},
    {"Skeleton",          "Player ESP", 1, 0}, {"Off-screen Arrows","Player ESP", 1, 0},
    {"Snap Lines",        "Player ESP", 1, 0}, {"Health Bar",       "Player ESP", 1, 0},
    // Labels
    {"Show Name",     "Labels", 1, 0}, {"Show Distance", "Labels", 1, 0},
    {"Show Weapon",   "Labels", 1, 0}, {"Show Platform", "Labels", 1, 0},
    {"Show Level",    "Labels", 1, 0},
    // Chams
    {"Enable Chams", "Chams", 1, 0}, {"Wireframe",      "Chams", 1, 0},
    {"Ignore Depth", "Chams", 1, 0}, {"Two Sided",      "Chams", 1, 0},
    {"Additive Blend","Chams",1, 0}, {"Dynamic Lighting","Chams",1, 0},
    // Tracers
    {"Enable Tracers",    "Tracers", 1, 1}, {"Max Age",          "Tracers", 1, 1},
    {"Impact Dot Radius", "Tracers", 1, 1}, {"Line Thickness",   "Tracers", 1, 1},
    {"Deduplicate",       "Tracers", 1, 1},
    // Trails
    {"Enable Trails",  "Trails", 1, 1}, {"Trail Duration",  "Trails", 1, 1},
    {"Push Interval",  "Trails", 1, 1}, {"Trail Thickness", "Trails", 1, 1},
    // Loot ESP
    {"Enable Loot ESP","Loot ESP",1,1}, {"Show Chests",    "Loot ESP",1,1},
    {"Show Ammoboxes", "Loot ESP",1,1}, {"Ground Loot",    "Loot ESP",1,1},
    {"Loot Distance",  "Loot ESP",1,1},
    // Visibility / colour
    {"Color by Visibility","Visibility",1,2}, {"Color Teammates","Visibility",1,2},
    // FOV circle
    {"Show FOV Circle","FOV Circle",1,2}, {"FOV Thickness","FOV Circle",1,2},
    // Distance
    {"Max ESP Distance","Distance",1,2},
    // Misc / visual fx
    {"Particles",          "Visual FX", 2, 0}, {"Fancy Particles",   "Visual FX", 2, 0},
    {"Watermark",          "Visual FX", 2, 0}, {"Watermark Shadow",  "Visual FX", 2, 0},
    {"Shot Debug",         "Visual FX", 2, 0},
    {"Particle Spawn Rate","Visual FX", 2, 0},
    // Interface
    {"Show FPS",    "Interface", 2, 1}, {"Menu Shadow", "Interface", 2, 1},
};
inline constexpr int k_search_item_count = ( int )( sizeof( k_search_items ) / sizeof( k_search_items[ 0 ] ) );
inline constexpr int k_max_results = 32;

// -- nav tab names -------------------------------------------------------------
inline int get_nav_tabs( int nav, const char* ( &out )[ 4 ] ) {
    switch ( nav ) {
        case 0: out[ 0 ] = "General"; out[ 1 ] = "Close Aim"; out[ 2 ] = "Triggerbot"; return 3;
        case 1: out[ 0 ] = "Player";  out[ 1 ] = "World"; out[ 2 ] = "Chams"; return 3;
        case 2: out[ 0 ] = "General"; out[ 1 ] = "Interface"; return 2;
        case 3: out[ 0 ] = "General"; out[ 1 ] = "Interface"; return 2;
        default: return 0;
    }
}

// -- widget helpers ------------------------------------------------------------
inline float draw_section_label( float x, float y, const char* title ) {
    g_interface->draw_text( title, x, y, ui::k_font, 11.f, ui::k_text_dim );
    return y + 18.f;
}
inline void draw_child( float x, float y, float w, float h ) {
    g_interface->draw_rounded_rect( x, y, w, h, 10.f, { 25, 25, 25, 150 } );
    g_interface->draw_rounded_rect_outline( x, y, w, h, 10.f, 1.f, { 48, 48, 48, 150 } );
}

inline void draw_check( float x, float& y, float w, const char* lbl, bool& value ) {
    y += ui::k_item_h;
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    const float bs = 16.f, bx = x + ui::k_inner_pad, by = y + ( ui::k_item_h - bs ) * 0.5f;
    bool hot = ui::hovered( x, y, w, ui::k_item_h );
    if ( value ) {
        c.draw_rounded_rect( bx, by, bs, bs, 4.f, ui::k_accent );
        c.draw_text( icons::to_utf8( 0xf00c ).c_str( ), bx + bs * 0.5f, by + bs * 0.5f - 7.5f,
            "Font Awesome", 14.f, { 0, 0, 0, 255 }, peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center );
    }
    else {
        c.draw_rounded_rect( bx, by, bs, bs, 4.f, { 32, 32, 32, 255 } );
    }
    const float      ty = by + bs - ui::k_font_sz - 5.f;
    peach::color_t   tc = hot ? ui::k_hot : value ? ui::k_text : ui::k_text_dim;
    c.draw_text( lbl, bx + bs + 8.f, ty, ui::k_font, ui::k_font_sz, tc );
    if ( hot && cr.lclick ) value = !value;
}

inline void draw_slider( float x, float& y, float w,
    const char* lbl, float& value, float mn, float mx ) {
    y += ui::k_item_h;
    auto& c = *g_interface;
    const float tx = x + ui::k_inner_pad, tw = w - ui::k_inner_pad * 2.f;
    char  buf[ 32 ]; sprintf( buf, "%.2f", value );
    float vw = g_interface->measure_text( buf, ui::k_font, ui::k_font_sz );
    const float label_y = y + 2.f;
    c.draw_text( lbl, tx, label_y, ui::k_font, ui::k_font_sz, ui::k_text );
    c.draw_text( buf, tx + tw - vw, label_y, ui::k_font, ui::k_font_sz, ui::k_accent2 );
    const float ty2 = label_y + ui::k_font_sz + 8.f, th = 3.f;
    c.draw_rounded_rect( tx, ty2, tw, th, 1.5f, { 36, 36, 36, 255 } );
    float t = ( value - mn ) / ( mx - mn ), fw = t * tw;
    if ( fw > 0 ) c.draw_rounded_rect( tx, ty2, fw, th, 1.5f, ui::k_accent );
    bool hot = ui::hovered( tx, ty2 - 6.f, tw, th + 12.f );
    c.draw_circle( tx + fw, ty2 + th * 0.5f, hot ? 6.f : 5.f, hot ? ui::k_accent2 : ui::k_accent );
    if ( hot && ui::g_cursor.ldown && !g_menu.dragging ) {
        float nt = ( ui::g_cursor.x - tx ) / tw;
        nt = nt < 0 ? 0 : nt > 1 ? 1 : nt; value = mn + nt * ( mx - mn );
    }
    y += 6.f;
}

struct color_state_t {
    bool  open = false;
    float anim = 0.f;
    float r = 1.f, g = 0.f, b = 0.f, a = 1.f;
};

struct deferred_color_t {
    bool   active = false;
    float  bx = 0.f;
    float  by = 0.f;
    float  bw = 0.f;
    float  drop_y = 0.f;
    float  visible = 0.f;
    color_state_t* state = nullptr;
};
inline deferred_color_t g_deferred_color;

inline void draw_color_picker( float x, float y, float w, color_state_t& state ) {
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;

    constexpr float sw = 18.f, sh = 18.f;
    const float bx = x + w - ui::k_inner_pad - sw;
    const float by = y + ( ui::k_item_h - sh ) * 0.5f;

    peach::color_t preview{
        ( uint8_t )( state.r * 255 ),
        ( uint8_t )( state.g * 255 ),
        ( uint8_t )( state.b * 255 ),
        ( uint8_t )( state.a * 255 ) };

    bool hot = ui::hovered( bx, by, sw, sh );
    c.draw_rounded_rect( bx, by, sw, sh, 4.f, preview );
    c.draw_rounded_rect_outline( bx, by, sw, sh, 4.f, 1.f,
        hot ? ui::k_hot : peach::color_t{ 60, 60, 60, 200 } );

    if ( hot && cr.lclick )
        state.open = !state.open;

    const float target = state.open ? 1.f : 0.f;
    state.anim += ( target - state.anim ) * 0.25f;

    if ( state.anim > 0.01f && !g_deferred_color.active ) {
        g_deferred_color.active = true;
        g_deferred_color.bx = bx;
        g_deferred_color.by = by;
        g_deferred_color.bw = 160.f;
        g_deferred_color.drop_y = by + sh + 4.f;
        g_deferred_color.visible = 140.f * state.anim;
        g_deferred_color.state = &state;
    }
}

inline void flush_deferred_color( ) {
    if ( !g_deferred_color.active ) return;
    g_deferred_color.active = false;

    auto& d = g_deferred_color;
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    auto& st = *d.state;

    const float panel_h = 140.f;
    const float px = d.bx + d.bw > d.bx ? d.bx - d.bw + 18.f : d.bx;

    c.push_clip_rect( px, d.drop_y, d.bw, d.visible );
    c.draw_rounded_rect( px, d.drop_y, d.bw, panel_h, 6.f, { 28, 28, 28, 255 } );
    c.draw_rounded_rect_outline( px, d.drop_y, d.bw, panel_h, 6.f, 1.f,
        { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 80 } );

    // preset swatches
    struct swatch_t { float r, g, b; const char* label; };
    static constexpr swatch_t k_swatches[ ] = {
        { 1.f,   0.34f, 0.34f, "Red"    },
        { 1.f,   0.65f, 0.f,   "Orange" },
        { 1.f,   1.f,   0.2f,  "Yellow" },
        { 0.3f,  1.f,   0.4f,  "Green"  },
        { 0.37f, 0.41f, 0.86f, "Blue"   },
        { 0.7f,  0.53f, 0.92f, "Purple" },
        { 1.f,   1.f,   1.f,   "White"  },
        { 0.5f,  0.5f,  0.5f,  "Grey"   },
    };
    constexpr int k_cols = 4;
    constexpr float cell = 30.f, cell_pad = 6.f;
    const float grid_x = px + 8.f, grid_y = d.drop_y + 8.f;

    for ( int i = 0; i < 8; i++ ) {
        const float cx2 = grid_x + ( i % k_cols ) * ( cell + cell_pad );
        const float cy2 = grid_y + ( i / k_cols ) * ( cell + cell_pad );
        peach::color_t col{
            ( uint8_t )( k_swatches[ i ].r * 255 ),
            ( uint8_t )( k_swatches[ i ].g * 255 ),
            ( uint8_t )( k_swatches[ i ].b * 255 ), 255 };
        bool sw_hot = ui::hovered( cx2, cy2, cell, cell );
        c.draw_rounded_rect( cx2, cy2, cell, cell, 5.f, col );
        if ( sw_hot )
            c.draw_rounded_rect_outline( cx2, cy2, cell, cell, 5.f, 1.5f, ui::k_hot );
        if ( sw_hot && cr.lclick ) {
            st.r = k_swatches[ i ].r;
            st.g = k_swatches[ i ].g;
            st.b = k_swatches[ i ].b;
            st.open = false;
        }
    }

    // R G B A sliders
    const float sl_x = px + 8.f, sl_w = d.bw - 16.f;
    float sl_y = grid_y + ( cell + cell_pad ) * 2 + 4.f;
    auto draw_chan = [ & ] ( const char* lbl, float& val, peach::color_t bar_col ) {
        float t = val, fw = t * sl_w;
        c.draw_text( lbl, sl_x, sl_y, ui::k_font, 9.f, ui::k_text_dim );
        const float track_y = sl_y + 11.f;
        c.draw_rounded_rect( sl_x, track_y, sl_w, 3.f, 1.5f, { 36, 36, 36, 255 } );
        if ( fw > 0 ) c.draw_rounded_rect( sl_x, track_y, fw, 3.f, 1.5f, bar_col );
        bool hot2 = ui::hovered( sl_x, track_y - 5.f, sl_w, 13.f );
        c.draw_circle( sl_x + fw, track_y + 1.5f, hot2 ? 5.f : 4.f,
            hot2 ? ui::k_hot : peach::color_t{ 220, 220, 220, 255 } );
        if ( hot2 && cr.ldown && !g_menu.dragging ) {
            float nt = ( cr.x - sl_x ) / sl_w;
            val = nt < 0.f ? 0.f : nt > 1.f ? 1.f : nt;
        }
        sl_y += 20.f;
        };

    draw_chan( "R", st.r, { 220,  80,  80, 255 } );
    draw_chan( "G", st.g, { 80, 200,  80, 255 } );
    draw_chan( "B", st.b, { 80, 120, 220, 255 } );
    draw_chan( "A", st.a, { 180, 180, 180, 255 } );

    if ( cr.lclick && !ui::hovered( px, d.drop_y, d.bw, panel_h ) )
        st.open = false;

    c.pop_clip_rect( );
}

struct deferred_combo_t {
    bool   active = false;
    float  bx = 0.f;
    float  cy2 = 0.f;
    float  bw = 0.f;
    float  bh = 0.f;
    float  drop_y = 0.f;
    float  drop_h = 0.f;
    float  visible = 0.f;
    float  row_h = 24.f;
    int    item_count = 0;
    int* value_ptr = nullptr;
    combo_state_t* state = nullptr;
};
inline deferred_combo_t g_deferred_combo;

inline void draw_combo( float x, float& y, float w,
    const char* lbl, int& value,
    combo_state_t& state ) {

    const wchar_t** items = ( const wchar_t** )state.items;
    const int       item_count = state.count;

    y += ui::k_item_h + 1;

    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    const float bx = x + ui::k_inner_pad;
    const float bw = w - ui::k_inner_pad * 2.f;
    const float bh = 22.f;

    const float cy2 = y + ui::k_font_sz + 11.f;
    const float target = state.open ? 1.f : 0.f;
    state.anim += ( target - state.anim ) * 0.25f;

    bool box_hot = ui::hovered( bx, y, bw, bh + ui::k_font_sz + 11.f );
    c.draw_text( lbl, bx, y + 2.f, ui::k_font_bold, ui::k_font_sz,
        ( box_hot || state.open ) ? ui::k_text : ui::k_text_dim );

    peach::color_t box_bg = ( box_hot || state.open )
        ? peach::color_t{ 36, 36, 36, 255 }
    : peach::color_t{ 28, 28, 28, 255 };
    c.draw_rect( bx, cy2, bw, bh, box_bg );

    if ( !state.open )
        c.draw_rect_outline( bx, cy2, bw, bh, 1.f, peach::color_t{ 52, 52, 52, 255 } );
    else
        c.draw_rect_outline( bx, cy2, bw, bh, 1.f,
        { ui::k_accent.r, ui::k_accent.g, ui::k_accent.b, 180 } );

    const float ty3 = cy2 + ( bh - ui::k_font_sz ) * 0.5f - 2.f;
    const wchar_t* sel_wtext = ( value >= 0 && value < item_count ) ? items[ value ] : L"---";
    char sel_narrow[ 64 ]{};
    for ( int i = 0; sel_wtext[ i ] && i < 63; i++ ) sel_narrow[ i ] = ( char )sel_wtext[ i ];
    c.draw_text( sel_narrow, bx + 6.f, ty3, ui::k_font, ui::k_font_sz, ui::k_text );

    const float chev_cx = bx + bw - 12.f, chev_cy = cy2 + bh * 0.5f;
    const float arm = 4.f;
    const float dir = 1.f - 2.f * state.anim;
    c.draw_line( chev_cx - arm, chev_cy - arm * 0.5f * dir,
        chev_cx, chev_cy + arm * 0.5f * dir,
        1.5f, state.open ? ui::k_accent2 : ui::k_text_dim );
    c.draw_line( chev_cx, chev_cy + arm * 0.5f * dir,
        chev_cx + arm, chev_cy - arm * 0.5f * dir,
        1.5f, state.open ? ui::k_accent2 : ui::k_text_dim );

    if ( box_hot && cr.lclick )
        state.open = !state.open;

    if ( !g_deferred_combo.active && state.anim > 0.01f ) {
        const float row_h = 24.f;
        const float drop_h = row_h * item_count;
        const float visible = drop_h * state.anim;
        const float drop_y = cy2 + bh + 3.f;

        g_deferred_combo.active = true;
        g_deferred_combo.bx = bx;
        g_deferred_combo.cy2 = cy2;
        g_deferred_combo.bw = bw;
        g_deferred_combo.bh = bh;
        g_deferred_combo.drop_y = drop_y;
        g_deferred_combo.drop_h = drop_h;
        g_deferred_combo.visible = visible;
        g_deferred_combo.row_h = row_h;
        g_deferred_combo.item_count = item_count;
        g_deferred_combo.value_ptr = &value;
        g_deferred_combo.state = &state;
    }

    y += ui::k_font_sz + 9.f;
}

inline void flush_deferred_combo( ) {
    if ( !g_deferred_combo.active ) return;
    g_deferred_combo.active = false;

    auto& d = g_deferred_combo;
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;

    const wchar_t** items = ( const wchar_t** )d.state->items;

    c.push_clip_rect( d.bx, d.drop_y, d.bw, d.visible );

    c.draw_rounded_rect( d.bx, d.drop_y, d.bw, d.drop_h, 6.f, { 28, 28, 28, 255 } );
    c.draw_rounded_rect_outline( d.bx, d.drop_y, d.bw, d.drop_h, 6.f, 1.f,
        { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 80 } );

    for ( int i = 0; i < d.item_count; i++ ) {
        const float ry2 = d.drop_y + i * d.row_h;
        bool        item_hot = d.state->open && ui::hovered( d.bx, ry2, d.bw, d.row_h );
        bool        selected = ( i == *d.value_ptr );

        if ( selected )
            c.draw_rounded_rect( d.bx + 3.f, ry2 + 2.f, d.bw - 6.f, d.row_h - 4.f, 4.f,
            { ui::k_accent.r, ui::k_accent.g, ui::k_accent.b, 120 } );
        else if ( item_hot )
            c.draw_rounded_rect( d.bx + 3.f, ry2 + 2.f, d.bw - 6.f, d.row_h - 4.f, 4.f,
            { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 22 } );

        peach::color_t tc = selected ? ui::k_hot
            : item_hot ? ui::k_text
            : ui::k_text_dim;

        char item_narrow[ 64 ]{};
        for ( int j = 0; items[ i ][ j ] && j < 63; j++ )
            item_narrow[ j ] = ( char )items[ i ][ j ];
        c.draw_text( item_narrow, d.bx + 10.f,
            ry2 + ( d.row_h - ui::k_font_sz ) * 0.5f - 2.f,
            ui::k_font, ui::k_font_sz, tc );

        if ( selected ) {
            const std::string check_icon = icons::to_utf8( 0xf00c );
            c.draw_text( check_icon.c_str( ),
                d.bx + d.bw - 18.f, ry2 + d.row_h * 0.5f - 6.f,
                "Font Awesome", 11.f, ui::k_accent2,
                peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center );
        }

        if ( item_hot && cr.lclick ) {
            *d.value_ptr = i;
            d.state->open = false;
        }

        if ( i < d.item_count - 1 )
            c.draw_rect( d.bx + 6.f, ry2 + d.row_h - 1.f, d.bw - 12.f, 1.f,
            { 40, 40, 40, 160 } );
    }

    if ( cr.lclick && !ui::hovered( d.bx, d.cy2, d.bw, d.bh + 3.f + d.visible ) )
        d.state->open = false;

    c.pop_clip_rect( );
}

inline void draw_nav_item( float x, float y, float w,
    const char* label, int index, int& active ) {
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    const float row_h = 30.f, icon_y = y + row_h * 0.5f;
    bool sel = ( index == active ), hot = ui::hovered( x, y, w, row_h );
    float icon_x = x + 18.f;

    if ( sel )      c.draw_rounded_rect( x + 6.f, y + 3.f, w - 12.f, row_h - 6.f, 5.f, { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 80 } );
    else if ( hot ) c.draw_rounded_rect( x + 6.f, y + 3.f, w - 12.f, row_h - 6.f, 5.f, { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 18 } );

    peach::color_t icon_col = sel ? ui::k_accent2 : ui::k_text_dim;

    const float icon_draw_y = icon_y - 8.f;

    switch ( index ) {
        case 0: c.draw_text( icons::to_utf8( 0xf8cc ).c_str( ), icon_x, icon_draw_y, "Font Awesome", 16.f, icon_col, peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center ); break;
        case 1: c.draw_text( icons::to_utf8( 0xf06e ).c_str( ), icon_x, icon_draw_y, "Font Awesome", 16.f, icon_col, peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center ); break;
        case 2: c.draw_text( icons::to_utf8( 0xf550 ).c_str( ), icon_x, icon_draw_y, "Font Awesome", 16.f, icon_col, peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center ); break;
        case 3: c.draw_text( icons::to_utf8( 0xf54c ).c_str( ), icon_x, icon_draw_y, "Font Awesome", 16.f, icon_col, peach::color_t( 0, 0, 0, 0 ), 0.f, peach::text_align::center ); break;
    }

    c.draw_text( label, x + 36.f, icon_y - ui::k_font_sz + 3.5f, ui::k_font, 12.5f,
        sel ? ui::k_hot : ui::k_text_dim );

    if ( hot && cr.lclick && index != active ) {
        active = index; g_menu.active_tab = 0; history_push( index, 0 );
    }
}

inline void draw_sub_item( float x, float y, float w,
    const char* label, int index, int& active,
    peach::color_t dot_col = { 179, 136, 235, 255 } ) {
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    const float row_h = 28.f, indent = 14.f;
    const float item_x = x + indent, item_w = w - indent;
    bool sel = ( index == active ), hot = ui::hovered( item_x, y, item_w, row_h );
    const float icon_y = y + row_h * 0.5f;

    if ( hot || sel ) c.draw_rect( x + indent - 2.f, y + 3.f, 2.f, row_h - 6.f,
        { dot_col.r, dot_col.g, dot_col.b, sel ? ( uint8_t )200 : ( uint8_t )60 } );
    if ( sel )      c.draw_rounded_rect( item_x, y + 2.f, item_w - 8.f, row_h - 4.f, 6.f, { 179, 136, 235, 30 } );
    else if ( hot ) c.draw_rounded_rect( item_x, y + 2.f, item_w - 8.f, row_h - 4.f, 1.5f, { 179, 136, 235, 14 } );

    c.draw_rounded_rect( item_x + 6.f, icon_y - 5.f, 10.f, 10.f, 3.f,
        { dot_col.r, dot_col.g, dot_col.b, sel ? ( uint8_t )255 : ( uint8_t )140 } );
    c.draw_text( label, item_x + 23.f, icon_y - ui::k_font_sz + 3.5f,
        ui::k_font, 12.f, sel ? ui::k_hot : ui::k_text_dim );

    if ( hot && cr.lclick && index != active ) {
        active = index; g_menu.active_tab = 0; history_push( index, 0 );
    }
}

// -- top bar -------------------------------------------------------------------
inline void draw_top_bar( float x, float y, float w ) {
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;
    const float h = 40.f;
    c.draw_rounded_rect( x, y, w, h, 10.f, { 22, 22, 22, 255 } );
    c.draw_rounded_rect_outline( x, y, w, h, 10.f, 1.f, { 60, 60, 60, 255 } );

    const float av_cx = x + 10.f + 13.f, av_cy = y + h * 0.5f;
    c.draw_circle( av_cx, av_cy, 13.f, { 36, 36, 36, 255 } );
    c.draw_text( profile::initials, av_cx - 6.f, av_cy - ui::k_font_sz * 0.5f - 1.5f,
        ui::k_font_bold, 10.f, ui::k_hot );
    c.draw_text( profile::display_name, av_cx + 13.f + 6.f, av_cy - ui::k_font_sz * 0.5f - 1.5f,
        ui::k_font, 11.f, ui::k_text_dim );

    const char* tabs[ 4 ] = {};
    const int   tab_count = get_nav_tabs( g_menu.active_nav, tabs );
    if ( tab_count > 0 ) {
        float tab_widths[ 4 ]{}, total_w = 0.f;
        for ( int i = 0; i < tab_count; i++ ) {
            tab_widths[ i ] = g_interface->measure_text( tabs[ i ], ui::k_font_bold, 12.f ) + 20.f;
            total_w += tab_widths[ i ];
        }
        total_w += 4.f * ( tab_count - 1 );
        float tab_x = x + ( w - total_w ) * 0.5f;
        for ( int i = 0; i < tab_count; i++ ) {
            float pw = tab_widths[ i ];
            bool  sel = ( i == g_menu.active_tab ), hot = ui::hovered( tab_x, y + 7.f, pw, 26.f );
            if ( sel )      c.draw_rounded_rect( tab_x, y + 7.f, pw, 26.f, 13.f, { ui::k_accent.r, ui::k_accent.g, ui::k_accent.b, 150 } );
            else if ( hot ) c.draw_rounded_rect( tab_x, y + 7.f, pw, 26.f, 13.f, { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 30 } );
            c.draw_text( tabs[ i ], tab_x + ( pw - ( pw - 20.f ) ) * 0.5f, y + 20.f - 8.f,
                ui::k_font_bold, 12.f, sel ? ui::k_hot : ( hot ? ui::k_text : ui::k_text_dim ) );
            if ( hot && cr.lclick && i != g_menu.active_tab ) {
                g_menu.active_tab = i; history_push( g_menu.active_nav, i );
            }
            tab_x += pw + 4.f;
        }
    }

    SYSTEMTIME st{}; crt::GetLocalTime( &st );
    int h12 = st.wHour % 12; if ( !h12 ) h12 = 12;
    const char* ampm = st.wHour < 12 ? "AM" : "PM";
    char time_buf[ 16 ]; sprintf( time_buf, "%d:%02d %s", h12, st.wMinute, ampm );
    float time_w = g_interface->measure_text( time_buf, ui::k_font_bold, 12.f );
    float center_y = y + h * 0.5f, time_x = x + w - time_w - 18.f;

    bool  can_fwd = history_can_forward( );
    float fwd_x = time_x - 30.f;
    bool  fwd_hot = can_fwd && ui::hovered( fwd_x, y + 8.f, 18.f, 24.f );
    peach::color_t fwd_col = fwd_hot ? ui::k_hot : can_fwd ? ui::k_text_dim : peach::color_t{ 60,60,60,180 };
    c.draw_line( fwd_x + 4.f, center_y - 6.f, fwd_x + 10.f, center_y, 1.5f, fwd_col );
    c.draw_line( fwd_x + 10.f, center_y, fwd_x + 4.f, center_y + 6.f, 1.5f, fwd_col );
    if ( fwd_hot && cr.lclick ) history_forward( );

    bool  can_back = history_can_back( );
    float back_x = fwd_x - 18.f;
    bool  back_hot = can_back && ui::hovered( back_x, y + 8.f, 18.f, 24.f );
    peach::color_t back_col = back_hot ? ui::k_hot : can_back ? ui::k_text_dim : peach::color_t{ 60,60,60,180 };
    c.draw_line( back_x + 10.f, center_y - 6.f, back_x + 4.f, center_y, 1.5f, back_col );
    c.draw_line( back_x + 4.f, center_y, back_x + 10.f, center_y + 6.f, 1.5f, back_col );
    if ( back_hot && cr.lclick ) history_back( );

    c.draw_text( time_buf, time_x, center_y - 8.5f, ui::k_font_bold, 12.f, ui::k_text );
}

// -- content panels ------------------------------------------------------------
inline void draw_content( float cx, float cy, float cw, float ch,
    float rx, float ry, float rw, float rh ) {
    auto& c = *g_interface;
    auto& m = g_menu;
    auto& s = g_search;

    c.draw_rounded_rect( cx, cy, cw, ch, 12.f, { 18, 18, 18, 255 } );
    c.draw_rounded_rect_outline( cx, cy, cw, ch, 12.f, 1.f, { 38, 38, 38, 255 } );
    c.draw_rounded_rect( rx, ry, rw, rh, 12.f, { 18, 18, 18, 255 } );
    c.draw_rounded_rect_outline( rx, ry, rw, rh, 12.f, 1.f, { 38, 38, 38, 255 } );

    c.push_clip_rect( cx + 2.f, cy + 2.f, cw - 4.f, ch - 4.f );
    {
        constexpr float k_child_top_pad = -18.f;
        const float pad = 14.f, gap = 10.f;
        const float col_w = ( cw - pad * 2.f - gap ) * 0.5f;
        const float lx = cx + pad, rx2 = lx + col_w + gap;
        const float kc = ui::k_item_h, ks = ui::k_item_h + 12.f, ko = 14.f;

        switch ( m.active_nav ) {

            case 0: {
                switch ( m.active_tab ) {
                    case 0: {
                        constexpr float lbl_h = 18.f, r_gap = 10.f;
                        const float top = cy + 14.f;
                        const float full_h = ch - ( top - cy ) - 10.f;

                        float ly = top;
                        ly = draw_section_label( lx, ly, "General" );
                        draw_child( lx, ly, col_w, full_h - lbl_h );
                        float iy = ly + k_child_top_pad;
                        draw_check( lx, iy, col_w, "Memory Aim", settings::aimbot::enabled );
                        draw_check( lx, iy, col_w, "Silent Aim", settings::aimbot::silent_aim );
                        draw_check( lx, iy, col_w, "Bullet TP", settings::aimbot::bullet_tp );
                        draw_check( lx, iy, col_w, "Multiple Damage", settings::aimbot::multiple_damage );
                        draw_check( lx, iy, col_w, "Weapon Only", settings::aimbot::weapon_only );
                        draw_check( lx, iy, col_w, "Aim Prediction", settings::aimbot::aim_prediction );
                        draw_check( lx, iy, col_w, "Aim at Bots", settings::aimbot::aim_at_bots );
                        draw_check( lx, iy, col_w, "Aim at Downed", settings::aimbot::aim_at_downed );
                        draw_slider( lx, iy, col_w, "FOV", settings::aimbot::fov, 1.f, 120.f );
                        draw_slider( lx, iy, col_w, "Speed Factor", settings::aimbot::smooth_factor, 0.01f, 1.f );
                        draw_slider( lx, iy, col_w, "Deadzone Factor", settings::aimbot::deadzone, 0.01f, 6.f );
                        draw_slider( lx, iy, col_w, "Damage Multiplier", settings::aimbot::damage_multiplier, 1.f, 70.f );

                        const float pool = full_h - lbl_h * 2.f - r_gap;
                        const float lbl_ch = pool * 0.4f;
                        const float trl_h = pool - lbl_ch;

                        float ry2 = top;
                        ry2 = draw_section_label( rx2, ry2, "Priority" );
                        draw_child( rx2, ry2, col_w, lbl_ch );
                        iy = ry2 + k_child_top_pad;
                        draw_check( rx2, iy, col_w, "Visible Only", settings::aimbot::visible_only );
                        draw_check( rx2, iy, col_w, "Prefer Visible", settings::aimbot::prefer_visible );
                        draw_slider( rx2, iy, col_w, "Crosshair Priority", settings::aimbot::crosshair_weight, 0.f, 1.f );
                        draw_slider( rx2, iy, col_w, "Distance Priority", settings::aimbot::distance_weight, 0.f, 1.f );

                        ry2 += lbl_ch + r_gap;
                        ry2 = draw_section_label( rx2, ry2, "Selection" );
                        draw_child( rx2, ry2, col_w, trl_h );
                        iy = ry2 + k_child_top_pad;

                        static const wchar_t* k_bone_modes[ ] = { L"Auto", L"Head", L"Neck", L"Chest" };
                        static const wchar_t* k_aim_keys[ ] = { L"Hold RMB", L"Hold LMB", L"Toggle", L"Always On" };

                        if ( !settings::aimbot::target_bone_combo.count ) {
                            settings::aimbot::target_bone_combo.items[ 0 ] = L"Best";
                            settings::aimbot::target_bone_combo.items[ 1 ] = L"Head";
                            settings::aimbot::target_bone_combo.items[ 2 ] = L"Neck";
                            settings::aimbot::target_bone_combo.items[ 3 ] = L"Chest";
                            settings::aimbot::target_bone_combo.count = 4;
                        }

                        if ( !settings::aimbot::aim_key_combo.count ) {
                            settings::aimbot::aim_key_combo.items[ 0 ] = L"Right MB";
                            settings::aimbot::aim_key_combo.items[ 1 ] = L"Left MB";
                            settings::aimbot::aim_key_combo.items[ 2 ] = L"Left Ctrl";
                            settings::aimbot::aim_key_combo.items[ 3 ] = L"Left Shift";
                            settings::aimbot::aim_key_combo.items[ 4 ] = L"When Shooting";
                            settings::aimbot::aim_key_combo.count = 5;
                        }

                        draw_combo( rx2, iy, col_w, "Aim Bone",
                            settings::aimbot::target_bone_mode,
                            settings::aimbot::target_bone_combo );

                        draw_combo( rx2, iy, col_w, "Aim Keybind",
                            settings::aimbot::aim_key,
                            settings::aimbot::aim_key_combo );
                    } break;

                }
            } break;

            case 1: {
                    switch ( m.active_tab ) {
                        case 0: {
                                constexpr float lbl_h = 18.f, r_gap = 10.f;
                                const float top = cy + 14.f;
                                const float full_h = ch - ( top - cy ) - 10.f;

                                // left — one child fills everything
                                float ly = top;
                                ly = draw_section_label( lx, ly, "ESP" );
                                draw_child( lx, ly, col_w, full_h - lbl_h );
                                float iy = ly + k_child_top_pad;
                                draw_check( lx, iy, col_w, "Enable", settings::visuals::enabled );
                                draw_check( lx, iy, col_w, "Normal Box", settings::visuals::bbox_enabled );
                                draw_check( lx, iy, col_w, "Directional Box", settings::visuals::directional_box );
                                draw_check( lx, iy, col_w, "3D Box", settings::visuals::threed_enabled );
                                draw_check( lx, iy, col_w, "Corner Box", settings::visuals::corner_box );
                                draw_check( lx, iy, col_w, "Filled Box", settings::visuals::bbox_filled );
                                draw_check( lx, iy, col_w, "Skeleton", settings::visuals::skeleton );
                                draw_check( lx, iy, col_w, "China Hat", settings::visuals::china_hat );
                                draw_check( lx, iy, col_w, "Threat Highlight", settings::visuals::thread_highlight );
                                draw_check( lx, iy, col_w, "Off-screen Arrows", settings::visuals::off_screen_arrows );
                                draw_check( lx, iy, col_w, "Look Direction", settings::visuals::look_direction );
                                draw_check( lx, iy, col_w, "Snap Lines", settings::visuals::snaplines );
                                draw_slider( lx, iy, col_w, "Font Size", settings::visuals::font_size, 9.f, 18.f );
                                draw_slider( lx, iy, col_w, "Box Thickness", settings::visuals::bbox_thickness, 0.5f, 4.f );
                                draw_slider( lx, iy, col_w, "Skel Thickness", settings::visuals::skeleton_thickness, 0.5f, 4.f );

                                // right — Labels + Trails split the pool
                                const float pool = full_h - lbl_h * 2.f - r_gap;
                                const float lbl_ch = pool * 0.62f;
                                const float trl_h = pool - lbl_ch;

                                float ry2 = top;
                                ry2 = draw_section_label( rx2, ry2, "Labels" );
                                draw_child( rx2, ry2, col_w, lbl_ch );
                                iy = ry2 + k_child_top_pad;
                                draw_check( rx2, iy, col_w, "Name", settings::visuals::show_name );
                                draw_check( rx2, iy, col_w, "Rank", settings::visuals::show_rank );
                                draw_check( rx2, iy, col_w, "Distance", settings::visuals::show_distance );
                                draw_check( rx2, iy, col_w, "Platform", settings::visuals::show_platform );
                                draw_check( rx2, iy, col_w, "Reloading", settings::visuals::show_reloading );
                                draw_check( rx2, iy, col_w, "Firing", settings::visuals::show_firing );
                                draw_check( rx2, iy, col_w, "Weapon", settings::visuals::show_weapon );
                                draw_check( rx2, iy, col_w, "Level", settings::visuals::show_level );
                                draw_check( rx2, iy, col_w, "Kills", settings::visuals::show_kills );

                                ry2 += lbl_ch + r_gap;
                                ry2 = draw_section_label( rx2, ry2, "Trails" );
                                draw_child( rx2, ry2, col_w, trl_h );
                                iy = ry2 + k_child_top_pad;
                                draw_check( rx2, iy, col_w, "Enable Trails", settings::trails::enabled );
                                draw_slider( rx2, iy, col_w, "Duration (s)", settings::trails::duration, 0.5f, 5.f );
                                draw_slider( rx2, iy, col_w, "Push Interval", settings::trails::push_interval, 0.005f, 0.1f );
                                draw_slider( rx2, iy, col_w, "Line Thickness", settings::trails::line_thickness, 1.f, 4.f );
                            } break;

                        case 1: {
                                constexpr float lbl_h = 18.f;
                                const float top = cy + 14.f;
                                const float full_h = ch - ( top - cy ) - 10.f;

                                float ly = top;
                                ly = draw_section_label( lx, ly, "Environment" );
                                draw_child( lx, ly, col_w, full_h - lbl_h );
                                float iy = ly + k_child_top_pad;
                                draw_check( lx, iy, col_w, "Enable ESP", settings::misc::item_esp );
                                draw_check( lx, iy, col_w, "Distance", settings::misc::show_distance );
                                draw_check( lx, iy, col_w, "Chests", settings::misc::show_chest );
                                draw_check( lx, iy, col_w, "Ammoboxes", settings::misc::show_ammobox );
                                draw_check( lx, iy, col_w, "Vehicles", settings::misc::show_vehicles );
                                draw_check( lx, iy, col_w, "Supplydrops", settings::misc::show_supplydrops );
                                draw_check( lx, iy, col_w, "Weakspots", settings::misc::show_weakspot );
                                draw_check( lx, iy, col_w, "Target Weakspots", settings::misc::target_weakspot );
                                draw_slider( lx, iy, col_w, "Font Size", settings::misc::item_size, 8.f, 17.f );
                                draw_slider( lx, iy, col_w, "Pickup Distance", settings::misc::item_max_distance, 50.f, 500.f );
                                draw_slider( lx, iy, col_w, "Chest Distance", settings::misc::chest_max_distance, 50.f, 500.f );
                                draw_slider( lx, iy, col_w, "Ammobox Distance", settings::misc::ammo_max_distance, 50.f, 500.f );
                                draw_slider( lx, iy, col_w, "Vehicle Distance", settings::misc::vehicle_distance, 50.f, 500.f );
                                draw_slider( lx, iy, col_w, "Supplydrop Distance", settings::misc::supplydrop_distance, 50.f, 500.f );

                                float ry2 = top;
                                ry2 = draw_section_label( rx2, ry2, "Tracers" );
                                draw_child( rx2, ry2, col_w, full_h - lbl_h );
                                float iy2 = ry2 + k_child_top_pad;
                                draw_check( rx2, iy2, col_w, "Enable Tracers", settings::tracers::enabled );
                                draw_check( rx2, iy2, col_w, "Deduplicate", settings::tracers::deduplicate );
                                draw_slider( rx2, iy2, col_w, "Max Age (s)", settings::tracers::max_age, 0.5f, 5.f );
                                draw_slider( rx2, iy2, col_w, "Dot Radius", settings::tracers::impact_dot_radius, 1.f, 10.f );
                                draw_slider( rx2, iy2, col_w, "Thickness", settings::tracers::line_thickness, 1.f, 4.f );
                            } break;

                        case 2: {
                                constexpr float lbl_h = 18.f;
                                const float top = cy + 14.f;
                                const float full_h = ch - ( top - cy ) - 10.f;

                                float ly = top;
                                ly = draw_section_label( lx, ly, "Player" );
                                draw_child( lx, ly, col_w, full_h - lbl_h );
                                float iy = ly + k_child_top_pad;
                                draw_check( lx, iy, col_w, "Chams", settings::chams::enabled );
                                draw_check( lx, iy, col_w, "Weapons", settings::chams::weapon );
                                draw_check( lx, iy, col_w, "Wireframe", settings::chams::wireframe );
                                draw_check( lx, iy, col_w, "Behind Walls", settings::chams::ignore_depth );
                                draw_check( lx, iy, col_w, "Two Sided", settings::chams::two_sided );
                                draw_check( lx, iy, col_w, "Dynamic Lighting", settings::chams::dynamic_area_light );
                                draw_slider( lx, iy, col_w, "Red", settings::chams::color_r, 0.f, 255.f );
                                draw_slider( lx, iy, col_w, "Green", settings::chams::color_g, 0.f, 255.f );
                                draw_slider( lx, iy, col_w, "Blue", settings::chams::color_b, 0.f, 255.f );

                                float ry2 = top;
                                ry2 = draw_section_label( rx2, ry2, "Pickup" );
                                draw_child( rx2, ry2, col_w, full_h - lbl_h );
                                float iy2 = ry2 + k_child_top_pad;
                                draw_check( rx2, iy, col_w, "Chams", settings::chams::pickups );

                            } break;
                    }
                } break;

            case 2: {
                    switch ( m.active_tab ) {
                        case 0: {
                                constexpr float lbl_h = 18.f;
                                const float top = cy + 14.f;
                                const float full_h = ch - ( top - cy ) - 10.f;

                                float ly = top;
                                ly = draw_section_label( lx, ly, "Effects" );
                                draw_child( lx, ly, col_w, full_h - lbl_h );
                                float iy = ly + k_child_top_pad;
                                draw_check( lx, iy, col_w, "Particles", settings::misc::particles_enabled );
                                draw_check( lx, iy, col_w, "Fancy Particles", settings::misc::fancy_particles );
                                draw_check( lx, iy, col_w, "Watermark", settings::misc::watermark );
                                draw_check( lx, iy, col_w, "Watermark Shadow", settings::misc::watermark_shadow );
                                draw_slider( lx, iy, col_w, "Spawn Rate", settings::misc::particle_spawn_rate, 0.01f, 0.2f );

                                float ry2 = top;
                                ry2 = draw_section_label( rx2, ry2, "Font" );
                                draw_child( rx2, ry2, col_w, full_h - lbl_h );
                                float iy2 = ry2 + k_child_top_pad;

                                draw_check( rx2, ry2, col_w, "Particles", settings::misc::particles_enabled );
                            } break;

                        case 1: {
                                constexpr float lbl_h = 18.f;
                                const float top = cy + 14.f;
                                const float full_h = ch - ( top - cy ) - 10.f;

                                float ly = top;
                                ly = draw_section_label( lx, ly, "Interface" );
                                draw_child( lx, ly, col_w, full_h - lbl_h );
                                float iy = ly + k_child_top_pad;
                                draw_check( lx, iy, col_w, "Show FPS", settings::misc::show_fps );
                                draw_check( lx, iy, col_w, "Menu Shadow", settings::misc::menu_shadow );
                            } break;
                    }
                } break;

            default: break;
        }
    }
    c.pop_clip_rect( );

    // -- right panel: search ---------------------------------------------------
    c.push_clip_rect( rx + 2.f, ry + 2.f, rw - 4.f, rh - 4.f );
    {
        constexpr float bar_h = 34.f, bar_r = 8.f, pad = 10.f;
        const float bx = rx + pad, by = ry + pad, bw = rw - pad * 2.f;

        peach::color_t bar_bg = s.focused
            ? peach::color_t{ 24, 22, 30, 255 }
        : peach::color_t{ 22, 22, 22, 255 };
        c.draw_rounded_rect( bx, by, bw, bar_h, bar_r, bar_bg );
        if ( s.focused )
            c.draw_rounded_rect_outline( bx, by, bw, bar_h, bar_r, 1.f,
                { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 140 } );
        else
            c.draw_rounded_rect_outline( bx, by, bw, bar_h, bar_r, 1.f, { 44, 44, 44, 255 } );

        const float ic_x = bx + 13.f, ic_y = by + bar_h * 0.5f, ic_r = 5.f;
        peach::color_t ic_col = s.focused ? ui::k_accent2 : ui::k_text_dim;
        c.draw_circle( ic_x, ic_y - 1.f, ic_r, { ic_col.r, ic_col.g, ic_col.b, 25 } );
        c.draw_circle( ic_x, ic_y - 1.f, ic_r * 0.5f, ic_col );
        c.draw_line( ic_x + ic_r * 0.65f, ic_y + ic_r * 0.65f - 1.f,
            ic_x + ic_r * 1.55f, ic_y + ic_r * 1.55f - 1.f, 1.5f, ic_col );

        const float tx = bx + 30.f, ty = by + ( bar_h - ui::k_font_sz ) * 0.5f;
        if ( s.len == 0 && !s.focused ) {
            c.draw_text( "Search", tx, ty - 2, ui::k_font, ui::k_font_sz, { 52, 52, 52, 255 } );
        }
        else {
            c.draw_text( s.buf, tx, ty - 3, ui::k_font, ui::k_font_sz, ui::k_text );
            s.cursor_blink += 0.025f;
            if ( s.cursor_blink > 1.2f ) s.cursor_blink = 0.f;
            if ( s.cursor_blink < 0.6f ) {
                float cw2 = g_interface->measure_text( s.buf, ui::k_font, ui::k_font_sz );
                c.draw_rect( tx + cw2 + 1.f, ty + 1.f, 1.5f, ui::k_font_sz - 2.f, ui::k_accent2 );
            }
        }

        if ( s.len > 0 ) {
            const float cx2 = bx + bw - 12.f, cy2 = by + bar_h * 0.5f;
            bool  x_hot = ui::hovered( cx2 - 8.f, by + 4.f, 16.f, bar_h - 8.f );
            peach::color_t xc = x_hot ? ui::k_hot : ui::k_text_dim;
            c.draw_line( cx2 - 4.f, cy2 - 4.f, cx2 + 4.f, cy2 + 4.f, 1.5f, xc );
            c.draw_line( cx2 + 4.f, cy2 - 4.f, cx2 - 4.f, cy2 + 4.f, 1.5f, xc );
            if ( x_hot && ui::g_cursor.lclick ) { s.len = 0; s.buf[ 0 ] = '\0'; }
        }
        if ( ui::hovered( bx, by, bw, bar_h ) && ui::g_cursor.lclick )
            s.focused = true;
        else if ( ui::g_cursor.lclick && !ui::hovered( rx, ry, rw, rh ) )
            s.focused = false;

        const float res_y = by + bar_h + pad;
        if ( s.len == 0 ) {
            const char* hint = "Type to search";
            float hw = g_interface->measure_text( hint, ui::k_font, 11.f );
            c.draw_text( hint, rx + ( rw - hw ) * 0.5f, ry + rh * 0.5f - 5.5f,
                ui::k_font, 11.f, { 44, 44, 44, 255 } );
        }
        else {
            int matches[ k_max_results ]; int match_count = 0;
            char q[ 64 ]{};
            for ( int i = 0; i < s.len && i < 63; i++ )
                q[ i ] = ( char )tolower( ( unsigned char )s.buf[ i ] );

            for ( int i = 0; i < k_search_item_count && match_count < k_max_results; i++ ) {
                char lbl[ 64 ]{};
                for ( int j = 0; k_search_items[ i ].label[ j ] && j < 63; j++ )
                    lbl[ j ] = ( char )tolower( ( unsigned char )k_search_items[ i ].label[ j ] );
                if ( strstr( lbl, q ) ) matches[ match_count++ ] = i;
            }

            if ( match_count == 0 ) {
                const char* msg = "No results";
                float mw = g_interface->measure_text( msg, ui::k_font, 11.f );
                c.draw_text( msg, rx + ( rw - mw ) * 0.5f, res_y + ( rh - res_y ) * 0.5f - 5.5f,
                    ui::k_font, 11.f, { 55, 55, 55, 255 } );
            }
            else {
                char hdr[ 32 ]; sprintf( hdr, "%d result%s", match_count, match_count == 1 ? "" : "s" );
                c.draw_text( hdr, bx, res_y, ui::k_font, 10.f, { 70, 70, 70, 255 } );
                constexpr float row_h = 34.f;
                float row_y = res_y + 16.f;
                for ( int i = 0; i < match_count; i++ ) {
                    if ( row_y + row_h > ry + rh - pad ) break;
                    const auto& item = k_search_items[ matches[ i ] ];
                    const float rw2 = rw - pad * 2.f;
                    bool row_hot = ui::hovered( bx, row_y, rw2, row_h );
                    if ( row_hot )
                        c.draw_rounded_rect( bx, row_y, rw2, row_h, 5.f,
                            { ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 18 } );
                    else if ( i % 2 == 0 )
                        c.draw_rounded_rect( bx, row_y, rw2, row_h, 5.f, { 255, 255, 255, 3 } );

                    c.draw_circle( bx + 8.f, row_y + row_h * 0.5f, 3.f,
                        row_hot ? ui::k_accent2 : peach::color_t{ ui::k_accent2.r, ui::k_accent2.g, ui::k_accent2.b, 120 } );
                    c.draw_text( item.label, bx + 18.f, row_y + ( row_h - ui::k_font_sz ) * 0.5f - 2.f,
                        ui::k_font, ui::k_font_sz, row_hot ? ui::k_hot : ui::k_text );

                    if ( i < match_count - 1 )
                        c.draw_rect( bx + 10.f, row_y + row_h - 1.f, rw2 - 12.f, 1.f, { 34, 34, 34, 180 } );

                    if ( row_hot && ui::g_cursor.lclick ) {
                        g_menu.active_nav = item.nav;
                        g_menu.active_tab = item.tab;
                        history_push( item.nav, item.tab );
                        s.len = 0; s.buf[ 0 ] = '\0'; s.focused = false;
                    }
                    row_y += row_h;
                }
            }
        }
    }

    c.pop_clip_rect( );

    flush_deferred_combo( );
    flush_deferred_color( );
}

// -- main render entry ---------------------------------------------------------
inline void render_menu( ) {
    if ( !g_menu.open ) return;

    static bool s_seeded = false;
    if ( !s_seeded ) { history_push( g_menu.active_nav, g_menu.active_tab ); s_seeded = true; }

    auto& m = g_menu;
    auto& c = *g_interface;
    auto& cr = ui::g_cursor;

    const float gap = 7.f, bar_h = 40.f;
    const float right_w = 188.f, content_w = 580.f;
    const float total_w = m.sidebar_w + gap + content_w + gap + right_w;
    const float sx = m.x, sy = m.y, sh = m.height, sw = m.sidebar_w;

    if ( ui::hovered( sx, sy, total_w, bar_h ) && cr.lclick ) {
        m.dragging = true;
        m.drag_off_x = cr.x - sx;
        m.drag_off_y = cr.y - sy;
    }
    if ( !cr.ldown ) m.dragging = false;
    if ( m.dragging ) { m.x = cr.x - m.drag_off_x; m.y = cr.y - m.drag_off_y; }

    const float bar_x = sx, bar_y = sy, bar_w = sw + gap + content_w + gap + right_w;
    const float content_x = sx + sw + gap, content_y = sy + bar_h + gap;
    const float content_h = sh - bar_h - gap;
    const float right_x = content_x + content_w + gap, right_y = content_y, right_h = content_h;

    //c.draw_glow( sx, sy, sw + gap + content_w + gap + right_w, sh, 18, 60 );

    // -- sidebar ---------------------------------------------------------------
    const float sidebar_y = sy + bar_h + gap;
    const float sidebar_h = sh - bar_h - gap;
    c.draw_rounded_rect( sx, sidebar_y, sw, sidebar_h, 12.f, { 16, 16, 16, 255 } );
    c.draw_rect_gradient_h( sx + 12.f, sidebar_y + 1.f, sw - 24.f, 1.f,
        { 255, 255, 255, 0 }, { 255, 255, 255, 18 } );
    c.draw_rounded_rect_outline( sx, sidebar_y, sw, sidebar_h, 12.f, 1.f, { 38, 38, 38, 255 } );
    c.push_clip_rect( sx + 2.f, sidebar_y + 2.f, sw - 4.f, sidebar_h - 4.f );

    const float brand_x = sx + 14.f;
    const float brand_y = sidebar_y + 12.f;
    const float brand_w = g_interface->measure_text( "divinity", ui::k_font_bold, 14.f );
    c.draw_text( "divinity", brand_x, brand_y, ui::k_font_bold, 14.f, ui::k_hot );
    c.draw_text( ".wtf", brand_x + brand_w + 1.f, brand_y, ui::k_font_bold, 14.f, ui::k_accent2 );
    c.draw_text( "Legendary Loot", sx + 14.f, sidebar_y + 30.f, ui::k_font, 11.f, ui::k_text_dim );

    const float btn_x = sx + sw - 28.f, btn_y = sidebar_y + 16.f;
    bool btn_hot = ui::hovered( btn_x - 2.f, btn_y - 4.f, 20.f, 16.f );
    peach::color_t dot_c = btn_hot ? ui::k_hot : ui::k_text_dim;
    for ( int i = 0; i < 3; i++ ) c.draw_circle( btn_x + i * 5.f, btn_y + 4.f, 1.5f, dot_c );

    c.draw_rect( sx + 10.f, sidebar_y + 52.f, sw - 20.f, 1.f, { 38, 38, 38, 255 } );

    float ny = sidebar_y + 58.f;
    draw_nav_item( sx, ny, sw, "Aimbot", 0, m.active_nav ); ny += 30.f;
    draw_nav_item( sx, ny, sw, "Visuals", 1, m.active_nav ); ny += 30.f;
    draw_nav_item( sx, ny, sw, "Miscellaneous", 2, m.active_nav ); ny += 30.f;
    draw_nav_item( sx, ny, sw, "Exploits", 3, m.active_nav ); ny += 30.f;

    ny += 12.f;
    c.draw_text( "Configuration", sx + 14.f, ny, ui::k_font_bold, 12.5f, ui::k_text );
    const float chev_x = sx + sw - 22.f, chev_y = ny + 4.f;
    c.draw_line( chev_x, chev_y, chev_x + 5.f, chev_y + 5.f, 1.5f, ui::k_text_dim );
    c.draw_line( chev_x + 5.f, chev_y + 5.f, chev_x + 10.f, chev_y, 1.5f, ui::k_text_dim );
    ny += 22.f;

    const float cfg_bar_x = sx + 14.f - 2.f;
    const float cfg_bar_top = ny + 4.f;
    const float cfg_bar_bot = ny + 30.f * 3 + 28.f - 4.f;
    c.draw_rect( cfg_bar_x, cfg_bar_top, 2.f, cfg_bar_bot - cfg_bar_top, { 50, 50, 50, 60 } );
    draw_sub_item( sx, ny, sw, "Rage", 5, m.active_nav, { 179, 136, 235, 255 } ); ny += 30.f;
    draw_sub_item( sx, ny, sw, "Legitimate", 6, m.active_nav, { 194, 160, 239, 255 } ); ny += 30.f;
    draw_sub_item( sx, ny, sw, "Tournament", 7, m.active_nav, { 206, 179, 242, 255 } );

    ny += 29.f;
    const float add_indent = 14.f;
    bool add_hot = ui::hovered( sx + add_indent, ny, sw - add_indent, 28.f );
    if ( add_hot ) c.draw_rounded_rect( sx + add_indent, ny + 3.f, sw - add_indent - 8.f, 22.f, 5.f, { 46, 46, 46, 50 } );
    const float plus_x = sx + add_indent + 11.f, plus_y = ny + 14.f;
    c.draw_line( plus_x - 5.f, plus_y, plus_x + 5.f, plus_y, 1.5f, ui::k_text_dim );
    c.draw_line( plus_x, plus_y - 5.f, plus_x, plus_y + 5.f, 1.5f, ui::k_text_dim );
    c.draw_text( "Add Config", plus_x + 12.f, ny + ( 28.f - ui::k_font_sz ) - 11.f,
        ui::k_font, 12.f, add_hot ? ui::k_text : ui::k_text_dim );

    // FPS counter
    if ( settings::misc::show_fps ) {
        static DWORD s_last_tick = 0;
        static int   s_fc = 0, s_fps = 0;
        DWORD now_t = crt::GetTickCount( ); s_fc++;
        if ( now_t - s_last_tick >= 1000 ) { s_fps = s_fc; s_fc = 0; s_last_tick = now_t; }
        char fps_buf[ 16 ]; sprintf( fps_buf, "%d FPS", s_fps );
        c.draw_rounded_rect( sx + 10.f, sidebar_y + sidebar_h - 28.f, sw - 20.f, 18.f, 4.f, { 26, 26, 26, 255 } );
        peach::color_t fps_col = s_fps >= 60
            ? peach::color_t{ 100, 220, 120, 255 }
            : s_fps >= 30
            ? peach::color_t{ 220, 180, 60, 255 }
        : peach::color_t{ 220, 80,  80, 255 };
        c.draw_circle( sx + 20.f, sidebar_y + sidebar_h - 19.f, 3.f, fps_col );
        c.draw_text( fps_buf, sx + 29.f, sidebar_y + sidebar_h - 28.f + ( 18.f - 11.f ) * 0.5f - 3.0f,
            ui::k_font, 11.f, ui::k_text_dim );
    }

    c.pop_clip_rect( );

    draw_top_bar( bar_x, bar_y, bar_w );
    draw_content( content_x, content_y, content_w, content_h, right_x, right_y, right_w, right_h );
}


