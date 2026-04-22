#pragma once

namespace dll {
    enum class backend_t { unknown, dx11, dx12 };
    static backend_t m_backend = backend_t::unknown;

    using hook_ctor_t = __int64( __fastcall* )( __int64, const wchar_t* );
    using hook_long_jump_patch_t = __int64( __fastcall* )( __int64, unsigned __int8*, __int64, __int64 );
    using hook_minhook_t = __int64( __fastcall* )( __int64, __int64, __int64 );
    static hook_ctor_t m_hook_ctor = nullptr;
    static hook_long_jump_patch_t m_hook_long_jump_patch = nullptr;
    static hook_minhook_t m_hook_minhook = nullptr;

    struct function_hook_t {
        void* vftable = nullptr;
        std::uint64_t state = 0;
        void* target = nullptr;
        void* original = nullptr;
        void* replacement = nullptr;
        std::uint32_t allocation_size = 0;
        std::uint32_t reserved_2c = 0;
        void* patch_memory = nullptr;
        void* backup_memory = nullptr;
        std::uint8_t reserved_40[ 0x08 ]{};
        wchar_t hook_name_inline[ 8 ]{};
        std::uint64_t hook_name_length = 0;
        std::uint64_t hook_name_capacity = 0;
        std::uint32_t extra_state = 0;
        std::uint32_t reserved_6c = 0;
        void* hook_set_data = nullptr;
        std::uint8_t reserved_78[ 0x28 ]{};
        void* reserved_a0 = nullptr;
    };
    static_assert( sizeof( function_hook_t ) == 0xA8 );

    static function_hook_t m_present_detour{};
    static bool m_present_detour_installed = false;
    static function_hook_t m_resizebuffers_detour{};
    static bool m_resizebuffers_detour_installed = false;
    static function_hook_t m_line_trace_detour{};

    static ID3D11Device* m_device = nullptr;
    static ID3D11DeviceContext* m_context = nullptr;

    static ID3D12Device* m_device12 = nullptr;
    static ID3D12CommandQueue* m_cmd_queue = nullptr;
    static IDXGISwapChain* m_hooked_swapchain = nullptr;
    static void** m_present_slot = nullptr;
    static void** m_resizebuffers_slot = nullptr;


    struct d3d1x_context_t {
        uint32_t m_api_flag;
        uint32_t m_pad;  
        void* m_swapchain;
        uint64_t m_last_timestamp;
        uint32_t m_retry_count;
    };

    struct dx12_context_t {
        void* m_vtable;
        void* m_device;
        void* m_swapchain3;
        void* m_queue;
        void* m_capture_allocator;
        void* m_allocator;
        void* m_capture_cmd;
        void* m_list; 
        std::uint8_t   pad0[ 0x50 ]; 
        void* m_readback_buffer; 
        std::uint8_t   pad1[ 0x38 ]; 
        void* m_fence;     
        std::uint8_t   pad2[ 0x60 ];
        std::uint64_t  m_fence_value;
        HANDLE         m_fence_event;
        std::uint64_t  m_fence_last; 
        std::uint8_t   pad3[ 0x68 ];
        std::uint8_t   m_initialized;
        std::uint8_t   m_capture_ready;
        std::uint8_t   pad4[ 2 ];
        std::uint32_t  m_rtv_descriptor_size;
        void* m_rtv_heap;        
        std::uint8_t   pad5[ 0x60 ];
        void** m_render_targets_beg;
        void** m_render_targets_end;
        void** m_render_targets_cap;
        std::uint8_t   pad6[ 0x50 ];
        LARGE_INTEGER  m_capture_timestamp;
    };
}
