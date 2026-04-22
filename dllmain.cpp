#include <impl/includes.h>

struct thread_context_t {
    CONTEXT m_context {};
    bool m_executed = false;
    bool m_result = false;
};

__declspec( dllexport ) bool __stdcall DllMain( HMODULE module, std::uint32_t reason, void* context ) {  
    MessageBoxA( 0, "Hello World", "Success", MB_OK );
    //auto result = dll::initialize( );
    //auto thread_context = reinterpret_cast< thread_context_t* >( context );
    //if ( thread_context ) {
    //    thread_context->m_executed = true;
    //    thread_context->m_result = result;

    //    auto* ntdll = GetModuleHandleA( oxorany( "ntdll.dll" ) );
    //    if ( !ntdll )
    //        return false;

    //    const auto nt_continue = reinterpret_cast< nt_continue_t >( GetProcAddress( ntdll, oxorany( "NtContinue" ) ) );
    //    nt_continue( &thread_context->m_context, true );
    //}

    // should never reach here
    return 1;
}