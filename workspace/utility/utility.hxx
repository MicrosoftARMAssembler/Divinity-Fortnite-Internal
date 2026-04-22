#pragma once
#include <cstdio>

inline void divinity_log_write( const char* information ) {
    if ( !information )
        return;

    char prefixed[ 2048 ]{};
    std::snprintf( prefixed, sizeof( prefixed ), "[divinity] %s", information );
    OutputDebugStringA( prefixed );
}

#define log_info( information, ... ) divinity_log_write( information )
#define print_info( information ) divinity_log_write( information )
template < typename _value_t >
    requires std::is_arithmetic_v< _value_t >
using enough_float_t = std::conditional_t< sizeof( _value_t ) <= sizeof( float ), float, double >;

template < typename _ret_t >
    requires std::is_floating_point_v< _ret_t >
constexpr auto k_pi = static_cast< _ret_t >( std::numbers::pi );

template < typename _ret_t >
    requires std::is_floating_point_v< _ret_t >
constexpr auto k_pi2 = static_cast< _ret_t >( k_pi< double > *2.0 );

template < typename _ret_t >
    requires std::is_floating_point_v< _ret_t >
constexpr auto k_rad_pi = static_cast< _ret_t >( 180.0 / k_pi< double > );

template < typename _ret_t >
    requires std::is_floating_point_v< _ret_t >
constexpr auto k_deg_pi = static_cast< _ret_t >( k_pi< double > / 180.0 );

static bool _wcs_contains( const wchar_t* haystack, const wchar_t* needle ) {
    if ( !haystack || reinterpret_cast< std::uintptr_t >( haystack ) < 0x1000 )
        return false;
    if ( !needle || !needle[ 0 ] )
        return true;

    for ( ; *haystack; ++haystack ) {
        if ( *haystack != *needle ) continue;
        const wchar_t* h = haystack;
        const wchar_t* n = needle;
        while ( *h && *n && *h == *n ) { ++h; ++n; }
        if ( !*n ) return true;
    }
    return false;
}

namespace utility {
    template< typename fn_t >
    static bool swap_vtable( void** slot, fn_t hook, fn_t& original ) {
        if ( !slot || !*slot )
            return false;

        DWORD old{ };
        VirtualProtect( slot, sizeof( void* ), PAGE_EXECUTE_READWRITE, &old );
        original = reinterpret_cast< fn_t >( *slot );
        *slot = reinterpret_cast< void* >( hook );
        VirtualProtect( slot, sizeof( void* ), old, &old );
        return true;
    }

    template< std::size_t index, typename fn_t >
    static bool swap_vtable( void* object, fn_t hook, fn_t& original ) {
        if ( !object )
            return false;

        auto vtable = *reinterpret_cast< void*** >( object );
        if ( !vtable )
            return false;

        return swap_vtable( &vtable[ index ], hook, original );
    }

    template < typename _value_t >
        requires std::is_arithmetic_v< _value_t >
    constexpr auto to_deg( const _value_t rad ) {
        using ret_t = enough_float_t< _value_t >;

        return static_cast< ret_t >( rad * k_rad_pi< ret_t > );
    }

    bool is_valid( uintptr_t pointer ) {
        if ( pointer < 0x10000 )
            return false;

        // Kernel space
        if ( pointer > 0x7FFFFFFFFFFF )
            return false;

        // Sub-4GB: plain integers misread as pointers (0x3c449ba6 pattern)
        if ( pointer <= 0xFFFFFFFF )
            return false;

        // Near-4GB boundary: packed struct fields misread as pointers
        // (0x100000001 pattern — two 32-bit ints concatenated)
        // Real heap allocations on Win64 with a large UE process will
        // never be in the 1GB–16GB range in practice.
        if ( pointer < 0x0000010000000000ULL )
            return false;

        // Non-canonical upper bits
        if ( pointer & 0xFFFF000000000000ULL )
            return false;

        __try {
            auto val = *reinterpret_cast< std::uint8_t* >( pointer );
            _mm_mfence( );
            ( void )val;
            _mm_mfence( );
            return true;
        }
        __except ( 1 ) {
            return false;
        }
    }

    bool is_valid( const void* pointer ) {
        return is_valid( reinterpret_cast< uintptr_t >( pointer ) );
    }
}