#pragma once

namespace module {
    peb_t* m_peb{ nullptr };
    std::uintptr_t m_module_base{ };
    std::size_t m_module_size{ };

    static bool _wstr_iequal( const wchar_t* a, const wchar_t* b ) {
        if ( !a || !b ) return false;
        while ( *a && *b ) {
            wchar_t ca = ( *a >= L'A' && *a <= L'Z' ) ? ( *a + 32 ) : *a;
            wchar_t cb = ( *b >= L'A' && *b <= L'Z' ) ? ( *b + 32 ) : *b;
            if ( ca != cb ) return false;
            ++a; ++b;
        }
        return *a == *b;
    }

    static bool _str_equal( const char* a, const char* b ) {
        if ( !a || !b ) return false;
        while ( *a && *b ) { if ( *a != *b ) return false; ++a; ++b; }
        return *a == *b;
    }

    static size_t _str_len( const char* s ) {
        size_t n = 0; if ( s ) while ( s[ n ] ) ++n; return n;
    }

    static unsigned long _hex_to_ul( const char* s, const char** endptr ) {
        unsigned long r = 0;
        while ( *s ) {
            unsigned char c = ( unsigned char )*s;
            unsigned int  d;
            if ( c >= '0' && c <= '9' ) d = c - '0';
            else if ( c >= 'a' && c <= 'f' ) d = c - 'a' + 10;
            else if ( c >= 'A' && c <= 'F' ) d = c - 'A' + 10;
            else break;
            r = r * 16 + d;
            ++s;
        }
        if ( endptr ) *endptr = s;
        return r;
    }

    struct wstr_buf {
        wchar_t data[ 512 ];
        size_t  length;

        wstr_buf( ) : length( 0 ) { data[ 0 ] = L'\0'; }
        wstr_buf( const wchar_t* src, size_t len ) {
            length = len < 511 ? len : 511;
            v_memcpy( data, src, length * sizeof( wchar_t ) );
            data[ length ] = L'\0';
        }
        const wchar_t* c_str( ) const { return data; }
        bool           empty( ) const { return length == 0; }
    };

    struct module_info {
        std::uintptr_t base_address;
        std::size_t    size;
        wstr_buf       name;
        wstr_buf       path;
    };

    template< typename type_t, typename field_t >
    inline constexpr const type_t* containing_record(
        const field_t* address,
        const field_t type_t::* field
    ) {
        const auto offset = reinterpret_cast< std::uintptr_t >(
            &( static_cast< const type_t* >( nullptr )->*field )
            );
        return reinterpret_cast< const type_t* >(
            reinterpret_cast< std::uintptr_t >( address ) - offset
            );
    }

    template< typename type_t, typename field_t >
    inline constexpr type_t* containing_record(
        field_t* address,
        const field_t type_t::* field
    ) {
        return const_cast< type_t* >(
            containing_record<type_t, field_t>(
                const_cast< const field_t* >( address ), field
            )
            );
    }

    static bool is_valid_region( void* ptr, std::size_t* out_size = nullptr ) {
        MEMORY_BASIC_INFORMATION mbi{};
        if ( !VirtualQuery( ptr, &mbi, sizeof( mbi ) ) )
            return false;

        if ( out_size )
            *out_size = mbi.RegionSize;

        return mbi.State == MEM_COMMIT
            && !( mbi.Protect & PAGE_GUARD )
            && mbi.Protect != PAGE_NOACCESS
            && ( mbi.Protect & ( PAGE_READONLY |
                PAGE_READWRITE |
                PAGE_EXECUTE_READ |
                PAGE_EXECUTE_READWRITE |
                PAGE_EXECUTE_WRITECOPY |
                PAGE_WRITECOPY ) );
    }

    static const ldr_data_table_entry_t* find_module_entry( const wchar_t* module_name ) {
        if ( !m_peb || !m_peb->m_ldr )
            return nullptr;

        const auto list_head = &m_peb->m_ldr->m_module_list_load_order;
        auto       current = list_head->m_flink;

        while ( current != list_head ) {
            const auto entry = containing_record(
                current,
                &ldr_data_table_entry_t::m_in_load_order_module_list
            );

            if ( entry
                && entry->m_base_dll_name.m_buffer
                && is_valid_region( entry->m_base_dll_name.m_buffer )
                && _wstr_iequal( entry->m_base_dll_name.m_buffer, module_name ) ) {
                return entry;
            }

            current = current->m_flink;
        }

        return nullptr;
    }

    static std::uintptr_t get_main_module( ) {
        return m_peb
            ? reinterpret_cast< std::uintptr_t >( m_peb->m_image_base_address )
            : 0;
    }

    static std::pair< std::uintptr_t, std::size_t > get_process_module( const wchar_t* module_name ) {
        const auto entry = find_module_entry( module_name );
        if ( !entry )
            return { 0, 0 };

        return {
            reinterpret_cast< std::uintptr_t >( entry->m_dll_base ),
            entry->m_size_of_image
        };
    }

    static std::uintptr_t get_module_base( const wchar_t* module_name ) {
        const auto entry = find_module_entry( module_name );
        return entry
            ? reinterpret_cast< std::uintptr_t >( entry->m_dll_base )
            : 0;
    }

    static std::size_t get_module_size( const wchar_t* module_name ) {
        const auto entry = find_module_entry( module_name );
        return entry ? entry->m_size_of_image : 0;
    }

    static bool is_address_in_module( std::uintptr_t address, const wchar_t* module_name ) {
        const auto entry = find_module_entry( module_name );
        if ( !entry ) return false;
        const auto base = reinterpret_cast< std::uintptr_t >( entry->m_dll_base );
        return address >= base && address < base + entry->m_size_of_image;
    }

    static std::uintptr_t get_module_by_address( std::uintptr_t address ) {
        if ( !m_peb || !m_peb->m_ldr ) return 0;

        const auto list_head = &m_peb->m_ldr->m_module_list_load_order;
        auto       current = list_head->m_flink;

        while ( current != list_head ) {
            const auto entry = containing_record(
                current,
                &ldr_data_table_entry_t::m_in_load_order_module_list
            );

            if ( entry ) {
                const auto base = reinterpret_cast< std::uintptr_t >( entry->m_dll_base );
                if ( address >= base && address < base + entry->m_size_of_image )
                    return base;
            }

            current = current->m_flink;
        }

        return 0;
    }

    // returns heap vector<module_info> — caller must vector_free()
    static vector* get_module_list( ) {
        vector* modules = _vector_create_base( sizeof( module_info ), 0 );
        if ( !modules || !m_peb || !m_peb->m_ldr )
            return modules;

        const auto list_head = &m_peb->m_ldr->m_module_list_load_order;
        auto       current = list_head->m_flink;

        while ( current != list_head ) {
            const auto entry = containing_record(
                current,
                &ldr_data_table_entry_t::m_in_load_order_module_list
            );

            if ( entry && entry->m_base_dll_name.m_buffer ) {
                module_info info{};
                info.base_address = reinterpret_cast< std::uintptr_t >( entry->m_dll_base );
                info.size = entry->m_size_of_image;
                info.name = wstr_buf( entry->m_base_dll_name.m_buffer,
                    entry->m_base_dll_name.m_length / sizeof( wchar_t ) );
                info.path = wstr_buf( entry->m_full_dll_name.m_buffer,
                    entry->m_full_dll_name.m_length / sizeof( wchar_t ) );
                vector_append( modules, module_info, info );
            }

            current = current->m_flink;
        }

        return modules;
    }

    static wstr_buf get_module_file_name( const wchar_t* module_name ) {
        const auto entry = find_module_entry( module_name );
        if ( !entry || !entry->m_full_dll_name.m_buffer )
            return wstr_buf{};

        return wstr_buf( entry->m_full_dll_name.m_buffer,
            entry->m_full_dll_name.m_length / sizeof( wchar_t ) );
    }

    static bool is_valid_region_ptr( void* ptr, size_t* out_region_size = nullptr ) {
        MEMORY_BASIC_INFORMATION mbi{};
        if ( !VirtualQuery( ptr, &mbi, sizeof( mbi ) ) )
            return false;

        if ( out_region_size )
            *out_region_size = mbi.RegionSize;

        return ( !( mbi.Protect & PAGE_GUARD ) && mbi.Protect != PAGE_NOACCESS && mbi.State == MEM_COMMIT &&
            mbi.Protect & ( PAGE_READONLY | PAGE_EXECUTE_READ | PAGE_READWRITE | PAGE_EXECUTE_READWRITE ) );
    }

    template <typename T = uintptr_t>
    static T rva_to_va( uintptr_t address, int size ) { return address ? ( T )( address + *( int* )( address + size - 4 ) + size ) : T( ); }

    struct pattern_byte {
        uint8_t value;
        bool wildcard;
    };

    inline bool is_memory_readable( const void* ptr, size_t size ) {
        MEMORY_BASIC_INFORMATION mbi;
        if ( VirtualQuery( ptr, &mbi, sizeof( mbi ) ) == 0 ) {
            return false;
        }

        if ( mbi.State != MEM_COMMIT ) {
            return false;
        }

        if ( mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_GUARD ) {
            return false;
        }

        return true;
    }

    inline bool safe_memcpy( void* dest, const void* src, size_t size ) {
        MEMORY_BASIC_INFORMATION mbi;
        if ( VirtualQuery( src, &mbi, sizeof( mbi ) ) == 0 ) {
            return false;
        }

        if ( mbi.State != MEM_COMMIT ) {
            return false;
        }

        if ( mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_GUARD || mbi.Protect == 0 ) {
            return false;
        }

        if ( !( mbi.Protect & ( PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE ) ) ) {
            return false;
        }

        __try {
            memcpy( dest, src, size );
            return true;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            return false;
        }
    }

    struct parsed_pattern {
        pattern_byte bytes[ 256 ];
        size_t count;
    };

    inline parsed_pattern parse_pattern( const char* pattern ) {
        parsed_pattern result = {};
        const char* p = pattern;
        while ( *p && result.count < 256 ) {
            if ( *p == ' ' ) { p++; continue; }
            if ( *p == '?' ) {
                result.bytes[ result.count++ ] = { 0, true };
                p++;
                if ( *p == '?' ) p++;
            }
            else {
                char byte_str[ 3 ] = { p[ 0 ], p[ 1 ], '\0' };
                result.bytes[ result.count++ ] = { static_cast< uint8_t >( strtoul( byte_str, nullptr, 16 ) ), false };
                p += 2;
            }
        }
        return result;
    }

    inline bool safe_test_read( const void* addr ) {
        __try {
            volatile uint8_t test = *reinterpret_cast< const volatile uint8_t* >( addr );
            ( void )test;
            return true;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER ) {
            return false;
        }
    }

    inline bool is_page_likely_encrypted( const uint8_t* buffer, size_t size ) {
        if ( size < 64 ) return false;

        uint32_t zero_count = 0;
        uint32_t high_entropy_count = 0;

        for ( size_t i = 0; i < 64; i++ ) {
            if ( buffer[ i ] == 0 ) zero_count++;
            if ( buffer[ i ] > 0x7F ) high_entropy_count++;
        }

        if ( zero_count == 64 ) return true;
        if ( high_entropy_count > 55 ) return true;

        return false;
    }

    uintptr_t find_pattern( uintptr_t start, size_t size, const char* pattern ) {
        parsed_pattern pat = parse_pattern( pattern );
        if ( pat.count == 0 ) {
            return 0;
        }

        size_t pat_len = pat.count;
        const uint8_t* scan_start = reinterpret_cast< const uint8_t* >( start );
        const uint8_t* scan_end = scan_start + size - pat_len;
        const size_t   page_size = 0x1000;
        uint8_t        temp_buffer[ 0x1000 ];

        if ( scan_end <= scan_start ) {
            return 0;
        }

        for ( const uint8_t* current = scan_start; current < scan_end;) {
            MEMORY_BASIC_INFORMATION mbi;
            if ( VirtualQuery( current, &mbi, sizeof( mbi ) ) == 0 ) {
                current += page_size;
                continue;
            }
            if ( mbi.State != MEM_COMMIT ) {
                current = reinterpret_cast< const uint8_t* >( mbi.BaseAddress ) + mbi.RegionSize;
                continue;
            }
            if ( mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_GUARD || mbi.Protect == 0 ) {
                current = reinterpret_cast< const uint8_t* >( mbi.BaseAddress ) + mbi.RegionSize;
                continue;
            }
            if ( !( mbi.Protect & ( PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE ) ) ) {
                current = reinterpret_cast< const uint8_t* >( mbi.BaseAddress ) + mbi.RegionSize;
                continue;
            }
            if ( !safe_test_read( current ) ) {
                current = reinterpret_cast< const uint8_t* >( mbi.BaseAddress ) + mbi.RegionSize;
                continue;
            }
            bool copy_success = safe_memcpy( temp_buffer, current, page_size );
            if ( !copy_success || is_page_likely_encrypted( temp_buffer, page_size ) ) {
                current += page_size;
                continue;
            }
            const uint8_t* page_end = ( current + page_size > scan_end ) ? scan_end : current + page_size;
            size_t search_len = page_end - current;
            if ( search_len < pat_len ) {
                current += page_size;
                continue;
            }
            for ( size_t i = 0; i < search_len - pat_len; ++i ) {
                bool found = true;
                for ( size_t j = 0; j < pat_len; ++j ) {
                    const pattern_byte& pb = pat.bytes[ j ];
                    if ( !pb.wildcard && temp_buffer[ i + j ] != pb.value ) {
                        found = false;
                        break;
                    }
                }
                if ( found )
                    return reinterpret_cast< uintptr_t >( current + i );
            }
            current += page_size;
        }
        return 0;
    }

    inline size_t get_module_size( uintptr_t base ) {
        if ( !base ) return 0;

        IMAGE_DOS_HEADER* dos_header = reinterpret_cast< IMAGE_DOS_HEADER* >( base );
        if ( dos_header->e_magic != IMAGE_DOS_SIGNATURE ) {
            return 0;
        }

        IMAGE_NT_HEADERS* nt_headers = reinterpret_cast< IMAGE_NT_HEADERS* >( base + dos_header->e_lfanew );
        if ( nt_headers->Signature != IMAGE_NT_SIGNATURE ) {
            return 0;
        }

        return nt_headers->OptionalHeader.SizeOfImage;
    }

    static std::uintptr_t get_export_address( std::uintptr_t base, const char* export_name ) {
        const auto dos = reinterpret_cast< dos_header_t* >( base );
        if ( !dos->is_valid( ) ) return 0;

        const auto nth = reinterpret_cast< nt_headers_t* >( base + dos->m_lfanew );
        if ( !nth->is_valid( ) ) return 0;

        const auto& dir = nth->m_export_table;
        if ( !dir.m_virtual_address ) return 0;

        const auto  exp = reinterpret_cast< export_directory_t* >( base + dir.m_virtual_address );
        const auto* names = reinterpret_cast< std::uint32_t* >( base + exp->m_address_of_names );
        const auto* ords = reinterpret_cast< std::uint16_t* >( base + exp->m_address_of_names_ordinals );
        const auto* fns = reinterpret_cast< std::uint32_t* >( base + exp->m_address_of_functions );

        for ( std::uint32_t i = 0; i < exp->m_number_of_names; ++i ) {
            const auto name = reinterpret_cast< const char* >( base + names[ i ] );
            if ( _str_equal( name, export_name ) ) {
                const auto ord = ords[ i ];
                if ( ord >= exp->m_number_of_functions ) return 0;
                return base + fns[ ord ];
            }
        }

        return 0;
    }

    static std::uintptr_t get_export( const wchar_t* module_name, const char* export_name ) {
        const auto base = get_module_base( module_name );
        return base ? get_export_address( base, export_name ) : 0;
    }

    template <typename T = void>
    T* find_section( HMODULE module, const char* name ) {
        if ( !module || !name )
            return nullptr;

        auto* dos = reinterpret_cast< dos_header_t* >( module );
        if ( !dos->is_valid( ) )
            return nullptr;

        auto* nt = reinterpret_cast< nt_headers_t* >(
            reinterpret_cast< std::uint8_t* >( module ) + dos->m_lfanew );
        if ( !nt->is_valid( ) )
            return nullptr;

        auto* section = reinterpret_cast< section_header_t* >(
            reinterpret_cast< std::uintptr_t >( nt ) + nt->m_size_of_optional_header + 0x18 );

        for ( std::uint16_t i = 0; i < nt->m_number_of_sections; ++i, ++section ) {
            if ( strncmp( reinterpret_cast< const char* >( section->m_name ), name, 8 ) == 0 ) {
                return reinterpret_cast< T* >(
                    reinterpret_cast< std::uint8_t* >( module ) + section->m_virtual_address );
            }
        }

        return nullptr;
    }

}