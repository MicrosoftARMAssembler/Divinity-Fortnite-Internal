#pragma once
#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <windows.h>

#if !defined(__cplusplus) && defined(__STDC_VERSION__) && __STDC_VERSION__ < 199901L
#error "This library requires C99 or later."
#endif

#define VECTOR_DEFAULT_ALIGNMENT 16

// ── IAT-free CRT imports ─────────────────────────────────────────────────────

typedef void* ( __cdecl* pfn_malloc )( size_t );
typedef void* ( __cdecl* pfn_calloc )( size_t, size_t );
typedef void* ( __cdecl* pfn_realloc )( void*, size_t );
typedef void( __cdecl* pfn_free )( void* );
typedef void* ( __cdecl* pfn_memcpy )( void*, const void*, size_t );
typedef void* ( __cdecl* pfn_memmove )( void*, const void*, size_t );
typedef void* ( __cdecl* pfn_memset )( void*, int, size_t );
typedef int( __cdecl* pfn_memcmp )( const void*, const void*, size_t );
typedef void( __cdecl* pfn_qsort )( void*, size_t, size_t, int( * )( const void*, const void* ) );
typedef int( __cdecl* pfn_vsnprintf )( char*, size_t, const char*, va_list );
typedef int( __cdecl* pfn_fprintf )( void*, const char*, ... );

static struct {
    pfn_malloc    fn_malloc;
    pfn_calloc    fn_calloc;
    pfn_realloc   fn_realloc;
    pfn_free      fn_free;
    pfn_memcpy    fn_memcpy;
    pfn_memmove   fn_memmove;
    pfn_memset    fn_memset;
    pfn_memcmp    fn_memcmp;
    pfn_qsort     fn_qsort;
    pfn_vsnprintf fn_vsnprintf;
    pfn_fprintf   fn_fprintf;
    bool          initialized;
} _vec_crt = {};

static bool _vec_crt_init( ) {
    if ( _vec_crt.initialized ) return true;

    auto h_crt = GetModuleHandleA( "ucrtbase.dll" );
    if ( !h_crt ) h_crt = LoadLibraryA( "ucrtbase.dll" );
    if ( !h_crt ) return false;

    auto h_msvcrt = GetModuleHandleA( "msvcrt.dll" );

#define VEC_RESOLVE(lib, name) \
        _vec_crt.fn_##name = (pfn_##name)GetProcAddress( lib, #name ); \
        if ( !_vec_crt.fn_##name ) return false;

    VEC_RESOLVE( h_crt, malloc );
    VEC_RESOLVE( h_crt, calloc );
    VEC_RESOLVE( h_crt, realloc );
    VEC_RESOLVE( h_crt, free );
    VEC_RESOLVE( h_crt, memcpy );
    VEC_RESOLVE( h_crt, memmove );
    VEC_RESOLVE( h_crt, memset );
    VEC_RESOLVE( h_crt, memcmp );
    VEC_RESOLVE( h_crt, qsort );
    VEC_RESOLVE( h_crt, vsnprintf );

    // fprintf goes to msvcrt or ucrtbase depending on system
    _vec_crt.fn_fprintf = ( pfn_fprintf )GetProcAddress( h_crt, "fprintf" );
    if ( !_vec_crt.fn_fprintf && h_msvcrt )
        _vec_crt.fn_fprintf = ( pfn_fprintf )GetProcAddress( h_msvcrt, "fprintf" );

#undef VEC_RESOLVE

    _vec_crt.initialized = true;
    return true;
}

// ── Macro shims replacing CRT calls ─────────────────────────────────────────

#define v_malloc(sz)              _vec_crt.fn_malloc(sz)
#define v_calloc(n, sz)           _vec_crt.fn_calloc(n, sz)
#define v_realloc(p, sz)          _vec_crt.fn_realloc(p, sz)
#define v_free(p)                 _vec_crt.fn_free(p)
#define v_memcpy(d, s, n)         _vec_crt.fn_memcpy(d, s, n)
#define v_memmove(d, s, n)        _vec_crt.fn_memmove(d, s, n)
#define v_memset(d, v, n)         _vec_crt.fn_memset(d, v, n)
#define v_memcmp(a, b, n)         _vec_crt.fn_memcmp(a, b, n)
#define v_qsort(b, n, s, c)       _vec_crt.fn_qsort(b, n, s, c)
#define v_vsnprintf(b, s, f, a)   _vec_crt.fn_vsnprintf(b, s, f, a)

// ── Thread-local for sort ────────────────────────────────────────────────────

__declspec( thread ) static void* _sort_context_ptr;
__declspec( thread ) static int ( *_sort_compar_ptr )( const void*, const void*, void* );

// ── SRWLOCK wrappers (already in windows.h, no CRT needed) ──────────────────

typedef struct {
    void* data;
    size_t length;
    size_t capacity;
    size_t element_size;
    struct {
        void* ( *alloc )( size_t );
        void* ( *realloc )( void*, size_t );
        void  ( *free )( void* );
    } allocator;
    SRWLOCK rwlock;
} vector;

// ── Error handling ───────────────────────────────────────────────────────────

typedef void ( *vector_error_callback )( const char* message );

static void _vec_default_error( const char* msg ) {
    ( void )msg; // silent — swap in your own callback if needed
}

static vector_error_callback _vec_error_cb = _vec_default_error;

static void vector_set_error_callback( vector_error_callback cb ) {
    _vec_error_cb = cb ? cb : _vec_default_error;
}

static void _vector_error( const char* fmt, ... ) {
    if ( !_vec_error_cb || !_vec_crt.fn_vsnprintf ) return;
    char msg[ 256 ];
    va_list args;
    va_start( args, fmt );
    v_vsnprintf( msg, sizeof( msg ), fmt, args );
    va_end( args );
    _vec_error_cb( msg );
}

// ── Safe math ───────────────────────────────────────────────────────────────

static int _safe_add( size_t a, size_t b, size_t* r ) {
    if ( b > ( size_t )-1 - a ) return -1;
    *r = a + b; return 0;
}

static int _safe_mul( size_t a, size_t b, size_t* r ) {
    if ( a && b > ( size_t )-1 / a ) return -1;
    *r = a * b; return 0;
}

// ── Allocators ──────────────────────────────────────────────────────────────

static void* _vec_alloc( size_t sz ) { return v_malloc( sz ); }
static void* _vec_realloc( void* p, size_t sz ) { return v_realloc( p, sz ); }
static void  _vec_free( void* p ) { v_free( p ); }

// ── Locking ─────────────────────────────────────────────────────────────────

static void vector_rdlock( vector* v ) { if ( v ) AcquireSRWLockShared( &v->rwlock ); }
static void vector_wrlock( vector* v ) { if ( v ) AcquireSRWLockExclusive( &v->rwlock ); }
static void vector_unlock( vector* v ) { if ( v ) ReleaseSRWLockExclusive( &v->rwlock ); }

// ── Core internals ──────────────────────────────────────────────────────────

static void* _vector_at( vector* v, size_t i ) {
    if ( !v || i >= v->length ) return NULL;
    return ( char* )v->data + i * v->element_size;
}

static vector* _vector_create_base( size_t elem_sz, size_t n ) {
    if ( !_vec_crt_init( ) ) return NULL;
    size_t alloc_sz;
    if ( _safe_mul( elem_sz, n, &alloc_sz ) == -1 ) return NULL;
    vector* v = ( vector* )v_malloc( sizeof( vector ) );
    if ( !v ) return NULL;
    v->allocator.alloc = _vec_alloc;
    v->allocator.realloc = _vec_realloc;
    v->allocator.free = _vec_free;
    v->data = alloc_sz ? v_calloc( n, elem_sz ) : NULL;
    if ( !v->data && alloc_sz ) { v_free( v ); return NULL; }
    v->length = n; v->capacity = n; v->element_size = elem_sz;
    InitializeSRWLock( &v->rwlock );
    return v;
}

static int _vector_reserve_internal( vector* v, size_t new_cap ) {
    if ( new_cap <= v->capacity ) return 0;
    size_t sz;
    if ( _safe_mul( new_cap, v->element_size, &sz ) == -1 ) return -1;
    void* nd = v->allocator.realloc( v->data, sz );
    if ( !nd && sz ) return -1;
    v->data = nd; v->capacity = new_cap; return 0;
}

static int _vector_append_internal( vector* v, size_t n, const void* vals ) {
    if ( !v ) return -1;
    if ( !n ) return 0;
    size_t total;
    if ( _safe_add( v->length, n, &total ) == -1 ) return -1;
    if ( total > v->capacity ) {
        size_t nc = v->capacity ? v->capacity + v->capacity / 2 : n;
        if ( nc < total ) nc = total;
        if ( _vector_reserve_internal( v, nc ) == -1 ) return -1;
    }
    v_memcpy( ( char* )v->data + v->length * v->element_size, vals, n * v->element_size );
    v->length = total;
    return 0;
}

static int _vector_insert_internal( vector* v, size_t idx, size_t n, const void* vals ) {
    if ( !v || idx > v->length ) return -1;
    if ( !n ) return 0;
    size_t total;
    if ( _safe_add( v->length, n, &total ) == -1 ) return -1;
    if ( total > v->capacity ) {
        size_t nc = v->capacity ? v->capacity + v->capacity / 2 : n;
        if ( nc < total ) nc = total;
        if ( _vector_reserve_internal( v, nc ) == -1 ) return -1;
    }
    if ( idx < v->length )
        v_memmove(
            ( char* )v->data + ( idx + n ) * v->element_size,
            ( char* )v->data + idx * v->element_size,
            ( v->length - idx ) * v->element_size
        );
    v_memcpy( ( char* )v->data + idx * v->element_size, vals, n * v->element_size );
    v->length = total;
    return 0;
}

static int _vector_remove_internal( vector* v, size_t idx, size_t n ) {
    if ( idx >= v->length || idx + n > v->length ) return -1;
    if ( !n ) return 0;
    size_t tail = v->length - idx - n;
    if ( tail )
        v_memmove(
            ( char* )v->data + idx * v->element_size,
            ( char* )v->data + ( idx + n ) * v->element_size,
            tail * v->element_size
        );
    v->length -= n;
    return 0;
}

static int _vector_resize_internal( vector* v, size_t nl ) {
    if ( nl > v->capacity ) {
        size_t nc = v->capacity ? v->capacity * 2 : nl;
        if ( nc < nl ) nc = nl;
        if ( _vector_reserve_internal( v, nc ) == -1 ) return -1;
    }
    if ( nl > v->length )
        v_memset(
            ( char* )v->data + v->length * v->element_size,
            0,
            ( nl - v->length ) * v->element_size
        );
    v->length = nl;
    return 0;
}

static void* _vector_pop_internal( vector* v ) {
    if ( !v || !v->length ) return NULL;
    vector_wrlock( v );
    void* out = v->allocator.alloc( v->element_size );
    if ( !out ) { vector_unlock( v ); return NULL; }
    v_memcpy( out, ( char* )v->data + ( v->length - 1 ) * v->element_size, v->element_size );
    v->length--;
    vector_unlock( v );
    return out;
}

static int _vector_swap_internal( vector* v, size_t i, size_t j ) {
    if ( i >= v->length || j >= v->length ) return -1;
    if ( i == j ) return 0;
    char   tmp[ 256 ];
    size_t esz = v->element_size;
    void* a = ( char* )v->data + i * esz;
    void* b = ( char* )v->data + j * esz;
    if ( esz <= sizeof( tmp ) ) {
        v_memcpy( tmp, a, esz );
        v_memcpy( a, b, esz );
        v_memcpy( b, tmp, esz );
    }
    return 0;
}

static int _vec_qsort_wrapper_cmp( const void* a, const void* b ) {
    return _sort_compar_ptr( a, b, _sort_context_ptr );
}

static void _vector_sort_internal( vector* v, int( *compar )( const void*, const void*, void* ) ) {
    if ( !v || v->length <= 1 ) return;
    _sort_context_ptr = v;
    _sort_compar_ptr = compar;
    v_qsort( v->data, v->length, v->element_size, _vec_qsort_wrapper_cmp );
    _sort_context_ptr = NULL;
    _sort_compar_ptr = NULL;
}

static void vector_free( vector* v ) {
    if ( !v ) return;
    vector_wrlock( v );
    if ( v->data ) v->allocator.free( v->data );
    vector_unlock( v );
    v_free( v );
}

static vector* _vector_create_with_values( size_t esz, size_t n, size_t argc, const void* vals ) {
    vector* v = _vector_create_base( esz, n );
    if ( !v ) return NULL;
    if ( argc == 0 ) { /* zeroed by calloc */ }
    else if ( argc == 1 ) { for ( size_t i = 0; i < n; i++ ) v_memcpy( ( char* )v->data + i * esz, vals, esz ); }
    else if ( argc <= n ) { v_memcpy( v->data, vals, argc * esz ); }
    else { vector_free( v ); return NULL; }
    return v;
}

static size_t _vector_find_internal(
    vector* v,
    const void* val,
    size_t         esz,
    int( *cmp )( const void*, const void*, void* )
) {
    if ( !v ) return ( size_t )-1;
    vector_rdlock( v );
    for ( size_t i = 0; i < v->length; i++ ) {
        if ( cmp( ( char* )v->data + i * esz, val, v ) == 0 ) {
            vector_unlock( v );
            return i;
        }
    }
    vector_unlock( v );
    return ( size_t )-1;
}

// ── Public API ───────────────────────────────────────────────────────────────

static int vector_clear( vector* v ) {
    if ( !v ) return -1;
    vector_wrlock( v ); v->length = 0; vector_unlock( v );
    return 0;
}

static int vector_reserve( vector* v, size_t n ) {
    if ( !v ) return -1;
    vector_wrlock( v );
    int r = _vector_reserve_internal( v, n );
    vector_unlock( v );
    return r;
}

static int vector_resize( vector* v, size_t n ) {
    if ( !v ) return -1;
    vector_wrlock( v );
    int r = _vector_resize_internal( v, n );
    vector_unlock( v );
    return r;
}

static int vector_remove( vector* v, size_t i, size_t n ) {
    if ( !v ) return -1;
    vector_wrlock( v );
    int r = _vector_remove_internal( v, i, n );
    vector_unlock( v );
    return r;
}

static int vector_swap( vector* v, size_t i, size_t j ) {
    if ( !v ) return -1;
    vector_wrlock( v );
    int r = _vector_swap_internal( v, i, j );
    vector_unlock( v );
    return r;
}

static int vector_shrink_to_fit( vector* v ) {
    if ( !v ) return -1;
    vector_wrlock( v );
    if ( v->capacity != v->length ) {
        size_t sz;
        _safe_mul( v->length, v->element_size, &sz );
        void* nd = v->length ? v->allocator.realloc( v->data, sz ) : NULL;
        if ( v->length && !nd ) { vector_unlock( v ); return -1; }
        if ( !v->length && v->data ) v->allocator.free( v->data );
        v->data = nd; v->capacity = v->length;
    }
    vector_unlock( v );
    return 0;
}

static vector* vector_copy( const vector* src ) {
    if ( !src ) return NULL;
    vector_rdlock( ( vector* )src );
    vector* dst = _vector_create_base( src->element_size, src->length );
    if ( dst ) {
        v_memcpy( dst->data, src->data, src->length * src->element_size );
        dst->length = src->length;
    }
    vector_unlock( ( vector* )src );
    return dst;
}

// ── Comparison helpers ───────────────────────────────────────────────────────

static int _vector_compare_asc( const void* a, const void* b, void* ctx ) {
    vector* v = ( vector* )ctx;
    const char* pa = ( const char* )a;
    const char* pb = ( const char* )b;
    for ( size_t i = 0; i < v->element_size; i++ ) {
        if ( pa[ i ] < pb[ i ] ) return -1;
        if ( pa[ i ] > pb[ i ] ) return  1;
    }
    return 0;
}

static int _vector_compare_desc( const void* a, const void* b, void* ctx ) {
    vector* v = ( vector* )ctx;
    const char* pa = ( const char* )a;
    const char* pb = ( const char* )b;
    for ( size_t i = 0; i < v->element_size; i++ ) {
        if ( pb[ i ] < pa[ i ] ) return -1;
        if ( pb[ i ] > pa[ i ] ) return  1;
    }
    return 0;
}

static int _vector_compare_eq( const void* a, const void* b, void* ctx ) {
    vector* v = ( vector* )ctx;
    const char* pa = ( const char* )a;
    const char* pb = ( const char* )b;
    for ( size_t i = 0; i < v->element_size; i++ ) {
        if ( pa[ i ] != pb[ i ] ) return 1;
    }
    return 0;
}

#define compare_asc  _vector_compare_asc
#define compare_desc _vector_compare_desc
#define compare_eq   _vector_compare_eq

// ── ARG_COUNT helper ─────────────────────────────────────────────────────────

#define ARG_COUNT_N(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,N,...) N
#define ARG_COUNT(...) ARG_COUNT_N(__VA_ARGS__,10,9,8,7,6,5,4,3,2,1,0)

// ── Public macros ────────────────────────────────────────────────────────────
//
//  C99  : compound literals  (const type[]){...}  are valid
//  C++  : compound literals are NOT valid — use a named array temporary instead
//

#ifdef __cplusplus

#define vector_create(type,...) \
    ( [&]() -> vector* { \
        const type _vc_tmp[] = { __VA_ARGS__ }; \
        return _vector_create_with_values( sizeof(type), ARG_COUNT(__VA_ARGS__), ARG_COUNT(__VA_ARGS__), _vc_tmp ); \
    }() )

#define vector_append(vec,type,...) do { \
    vector_wrlock(vec); \
    const type _va_tmp[] = { __VA_ARGS__ }; \
    _vector_append_internal( (vec), ARG_COUNT(__VA_ARGS__), _va_tmp ); \
    vector_unlock(vec); } while(0)

#define vector_push vector_append

#define vector_insert(vec,type,idx,...) do { \
    vector_wrlock(vec); \
    const type _vi_tmp[] = { __VA_ARGS__ }; \
    _vector_insert_internal( (vec), (idx), ARG_COUNT(__VA_ARGS__), _vi_tmp ); \
    vector_unlock(vec); } while(0)

#define vector_prepend(vec,type,...) do { \
    vector_wrlock(vec); \
    const type _vp_tmp[] = { __VA_ARGS__ }; \
    _vector_insert_internal( (vec), 0, ARG_COUNT(__VA_ARGS__), _vp_tmp ); \
    vector_unlock(vec); } while(0)

#define vector_set(type,vec,idx,val) do { \
    vector_wrlock(vec); \
    { type _vs_tmp = (val); type* _p = (type*)_vector_at((vec),(idx)); if(_p)*_p=_vs_tmp; } \
    vector_unlock(vec); } while(0)

// lambda used only for vector_find so the temp has a scope without a statement-expression
#define vector_find(type,vec,val,cmp) \
    ( [&]() -> size_t { const type _vf_tmp = (val); \
      return _vector_find_internal( (vec), (const void*)&_vf_tmp, sizeof(type), (cmp) ); }() )

#else  /* ── C99 path ── */

#define vector_create(type,...) \
    _vector_create_with_values( sizeof(type), ARG_COUNT(__VA_ARGS__), ARG_COUNT(__VA_ARGS__), (const type[]){__VA_ARGS__} )

#define vector_append(vec,type,...) do { \
    vector_wrlock(vec); \
    _vector_append_internal( (vec), ARG_COUNT(__VA_ARGS__), (const type[]){__VA_ARGS__} ); \
    vector_unlock(vec); } while(0)

#define vector_push vector_append

#define vector_insert(vec,type,idx,...) do { \
    vector_wrlock(vec); \
    _vector_insert_internal( (vec), (idx), ARG_COUNT(__VA_ARGS__), (const type[]){__VA_ARGS__} ); \
    vector_unlock(vec); } while(0)

#define vector_prepend(vec,type,...) do { \
    vector_wrlock(vec); \
    _vector_insert_internal( (vec), 0, ARG_COUNT(__VA_ARGS__), (const type[]){__VA_ARGS__} ); \
    vector_unlock(vec); } while(0)

#define vector_set(type,vec,idx,val) do { \
    vector_wrlock(vec); \
    type* _p = (type*)_vector_at((vec),(idx)); if(_p)*_p=(val); \
    vector_unlock(vec); } while(0)

#define vector_find(type,vec,val,cmp) \
    _vector_find_internal( (vec), (const void*)&(type){(val)}, sizeof(type), (cmp) )

#endif /* __cplusplus */

// ── Read-only accessors (no lock needed for these — caller locks if needed) ──

#define vector_at(type,vec,idx)      ( (type*)_vector_at((vec),(idx)) )
#define vector_at_ptr(type,vec,idx)  ( (type*)_vector_at((vec),(idx)) )
#define vector_pop(type,vec)         ( (type*)_vector_pop_internal(vec) )
#define vector_length(vec)           ( (vec) ? (vec)->length   : 0 )
#define vector_capacity(vec)         ( (vec) ? (vec)->capacity : 0 )
#define vector_is_empty(vec)         ( (vec) ? (vec)->length == 0 : true )

#define vector_sort(vec,type,compar) do { \
    vector_wrlock(vec); \
    _vector_sort_internal( (vec), (compar) ); \
    vector_unlock(vec); } while(0)

// vector_foreach / vector_foreach_end — must be used as a matched pair:
//   vector_foreach(int, my_vec, ptr) { ... } vector_foreach_end(my_vec);
#define vector_foreach(type,vec,ptr) \
    if(vec){ vector_rdlock(vec); \
    for( type* ptr = (type*)(vec)->data; \
         ptr < (type*)((char*)(vec)->data + (vec)->length * (vec)->element_size); \
         ++ptr )

#define vector_foreach_end(vec) vector_unlock(vec); }

#endif /* __VECTOR_H__ */