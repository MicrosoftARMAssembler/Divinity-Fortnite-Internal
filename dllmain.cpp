#include <impl/includes.h>

struct thread_context_t {
    CONTEXT m_context {};
    bool m_executed = false;
    bool m_result = false;
};

__declspec( dllexport ) bool __stdcall DllMain( HMODULE module, std::uint32_t reason, void* context ) {  
    return dll::initialize( );
}
