#pragma once
#include <dwrite.h>
#include <cstdlib>
#include <cstring>

// ============================================================
//  memory font stream  –  serves font bytes to DirectWrite
// ============================================================
class memory_font_stream_t : public IDWriteFontFileStream {
public:
    const void* m_data;
    UINT32      m_size;

    // IDWriteFontFileStream
    HRESULT STDMETHODCALLTYPE ReadFileFragment(
        void const** fragment_start,
        UINT64        file_offset,
        UINT64        fragment_size,
        void** fragment_context ) override {
        if ( file_offset + fragment_size > m_size )
            return E_FAIL;
        *fragment_start = static_cast< const char* >( m_data ) + file_offset;
        *fragment_context = nullptr;
        return S_OK;
    }

    void STDMETHODCALLTYPE ReleaseFileFragment( void* /*fragment_context*/ ) override { }

    HRESULT STDMETHODCALLTYPE GetFileSize( UINT64* file_size ) override {
        *file_size = m_size;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE GetLastWriteTime( UINT64* last_write_time ) override {
        *last_write_time = 0;
        return S_OK;
    }

    // IUnknown  –  static lifetime, no real ref-counting needed
    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** obj ) override {
        if ( riid == __uuidof( IDWriteFontFileStream ) ||
            riid == __uuidof( IUnknown ) ) {
            *obj = this;
            return S_OK;
        }
        *obj = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef( )  override { return 1; }
    ULONG STDMETHODCALLTYPE Release( ) override { return 1; }
};

// ============================================================
//  memory font loader  –  one instance per font entry
// ============================================================
class memory_font_loader_t : public IDWriteFontFileLoader {
public:
    memory_font_stream_t m_stream; // embedded, no heap alloc

    // IDWriteFontFileLoader
    HRESULT STDMETHODCALLTYPE CreateStreamFromKey(
        void const* /*font_file_reference_key*/,
        UINT32      /*font_file_reference_key_size*/,
        IDWriteFontFileStream** font_file_stream ) override {
        *font_file_stream = &m_stream;
        return S_OK;
    }

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface( REFIID riid, void** obj ) override {
        if ( riid == __uuidof( IDWriteFontFileLoader ) ||
            riid == __uuidof( IUnknown ) ) {
            *obj = this;
            return S_OK;
        }
        *obj = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef( )  override { return 1; }
    ULONG STDMETHODCALLTYPE Release( ) override { return 1; }
};

// ============================================================
//  font entry
// ============================================================
struct font_entry_t {
    char                  name[ 64 ];
    IDWriteFontFace* face;
    memory_font_loader_t* loader;
    void* data;
    UINT32                size;
    bool                  registered; // loader registered with factory?

    void init( ) {
        name[ 0 ] = '\0';
        face = nullptr;
        loader = nullptr;
        data = nullptr;
        size = 0;
        registered = false;
    }
};

// ============================================================
//  font namespace
// ============================================================
namespace peach {
    namespace font {

        constexpr int k_max_fonts = 16;

        inline font_entry_t g_fonts[ k_max_fonts ];
        inline int          g_count = 0;
        inline bool         g_initialized = false;

        // --------------------------------------------------------
        inline void ensure_init( ) {
            if ( g_initialized ) return;
            for ( int i = 0; i < k_max_fonts; i++ )
                g_fonts[ i ].init( );
            g_initialized = true;
        }

        // --------------------------------------------------------
        //  load()  –  copies font bytes to heap, no disk I/O
        // --------------------------------------------------------
        inline bool load( const void* data, UINT32 size, const char* name ) {
            ensure_init( );
            if ( g_count >= k_max_fonts || !data || size == 0 || !name )
                return false;

            auto& e = g_fonts[ g_count ];
            e.init( );

            // heap-copy so caller can free their buffer immediately
            e.data = malloc( size );
            if ( !e.data ) return false;
            memcpy( e.data, data, size );
            e.size = size;

            // copy name
            int k = 0;
            while ( name[ k ] && k < 63 ) { e.name[ k ] = name[ k ]; k++; }
            e.name[ k ] = '\0';

            // set up loader/stream (no factory needed yet)
            e.loader = new memory_font_loader_t( );
            e.loader->m_stream.m_data = e.data;
            e.loader->m_stream.m_size = e.size;

            g_count++;
            return true;
        }

        inline bool load_compressed_raw( const unsigned char* compressed_data, size_t compressed_size, const char* name ) {
            if ( !compressed_data || compressed_size == 0 || !name )
                return false;

            unsigned int raw_sz = stb_decompress_length( ( unsigned char* )compressed_data );
            if ( raw_sz == 0 ) return false;

            unsigned char* ttf_buf = ( unsigned char* )malloc( raw_sz );
            if ( !ttf_buf ) return false;

            unsigned int result = stb_decompress( ttf_buf, ( unsigned char* )compressed_data, ( unsigned int )compressed_size );
            if ( result == 0 ) { free( ttf_buf ); return false; }

            bool ok = load( ttf_buf, raw_sz, name );
            free( ttf_buf );
            return ok;
        }

        inline bool create_faces( IDWriteFactory* factory ) {
            if ( !factory ) 
                return false;

            IDWriteFactory5* factory5 = nullptr;
            if ( FAILED( factory->QueryInterface( __uuidof( IDWriteFactory5 ),
                reinterpret_cast< void** >( &factory5 ) ) ) ) {
                return false;
            }

            IDWriteInMemoryFontFileLoader* mem_loader = nullptr;
            factory5->CreateInMemoryFontFileLoader( &mem_loader );
            factory5->RegisterFontFileLoader( mem_loader );

            for ( int fi = 0; fi < g_count; fi++ ) {
                auto& e = g_fonts[ fi ];
                if ( e.face ) continue;

                IDWriteFontFile* font_file = nullptr;
                HRESULT hr = mem_loader->CreateInMemoryFontFileReference(
                    factory5,
                    e.data,
                    e.size,
                    nullptr,
                    &font_file
                );

                if ( FAILED( hr ) || !font_file )
                    continue;

                BOOL supported = FALSE;
                DWRITE_FONT_FILE_TYPE file_type = DWRITE_FONT_FILE_TYPE_UNKNOWN;
                DWRITE_FONT_FACE_TYPE face_type = DWRITE_FONT_FACE_TYPE_UNKNOWN;
                UINT32 face_count = 0;
                font_file->Analyze( &supported, &file_type, &face_type, &face_count );

                if ( supported && face_count > 0 ) {
                    DWRITE_FONT_FACE_TYPE actual_type = face_type;
                    if ( actual_type == DWRITE_FONT_FACE_TYPE_UNKNOWN ||
                        ( e.data && ( ( unsigned char* )e.data )[ 0 ] == 0x4F ) ) // 'O' = OTTO
                        actual_type = DWRITE_FONT_FACE_TYPE_CFF;

                    hr = factory->CreateFontFace(
                        actual_type,
                        1, &font_file,
                        0,
                        DWRITE_FONT_SIMULATIONS_NONE,
                        &e.face
                    );
                }

                font_file->Release( );
            }

            factory->UnregisterFontFileLoader( mem_loader );
            mem_loader->Release( );
            factory5->Release( );
            return true;
        }

        inline IDWriteFontFace* get_face( const char* name ) {
            if ( !name ) return nullptr;

            for ( int i = 0; i < g_count; i++ ) {
                const char* a = g_fonts[ i ].name;
                const char* b = name;
                bool match = true;
                int j = 0;
                while ( a[ j ] || b[ j ] ) {
                    if ( a[ j ] != b[ j ] ) { match = false; break; }
                    j++;
                }

                if ( match ) {
                    return g_fonts[ i ].face;
                }
            }

            return nullptr;
        }


        inline void shutdown( IDWriteFactory* factory ) {
            for ( int i = 0; i < g_count; i++ ) {
                auto& e = g_fonts[ i ];

                if ( e.face ) {
                    e.face->Release( );
                    e.face = nullptr;
                }
                if ( e.loader ) {
                    if ( e.registered && factory )
                        factory->UnregisterFontFileLoader( e.loader );
                    delete e.loader;
                    e.loader = nullptr;
                }
                if ( e.data ) {
                    free( e.data );
                    e.data = nullptr;
                }
                e.registered = false;
            }
            g_count = 0;
            g_initialized = false;
        }

    } // namespace font
} // namespace peach