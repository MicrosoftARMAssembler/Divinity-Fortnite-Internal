namespace Hook {
    class NewHook {
    public:

        template <typename Type>
        bool VMT( void* Address, void* Function, int Index, Type* Original ) {

            this->VirtualTable = *( uintptr_t** )( Address );

            if ( this->LastHookedFunctionAddress && this->LastHookedFunctionIndex ) {
                if ( this->LastHookedFunctionAddress == this->VirtualTable[ this->LastHookedFunctionIndex ] ) {
                    return false;
                }
            }

            if ( reinterpret_cast< void* >( this->VirtualTable[ Index ] ) != Function ) {

                while ( this->VirtualTable[ this->VTableSize ] ) {
                    this->VTableSize++;
                }

                if ( Index < this->VTableSize ) {
                    this->AllocatedVTable = reinterpret_cast< uintptr_t* >(  calloc ( this->VTableSize * sizeof( uintptr_t ), 0x10 ) );
                    
                    for ( int i = 0; i < this->VTableSize; i++ ) {
                        this->AllocatedVTable[ i ] = this->VirtualTable[ i ];
                    }

                    *Original = reinterpret_cast< Type >( this->VirtualTable[ Index ] );

                    this->AllocatedVTable[ Index ] = reinterpret_cast< uintptr_t >( Function );

                    *( uintptr_t** )( Address ) = this->AllocatedVTable;

                    this->LastHookedFunctionAddress = reinterpret_cast< uintptr_t >( Function );
                    this->LastHookedFunctionIndex = Index;
                    return true;
                }
            }
            return false;
            //static std::mutex vtableMutex;  
            //std::lock_guard<std::mutex> lock(vtableMutex);

            //if (!Address || !Function || !Original)
            //    return false;

            //this->OriginalVTable = *(uintptr_t**)(Address);

            //if (!this->OriginalVTable)
            //    return false;

            //if (this->LastHookedFunctionAddress && this->LastHookedFunctionIndex)
            //{
            //    if (this->LastHookedFunctionAddress == this->OriginalVTable[this->LastHookedFunctionIndex])
            //    {
            //        return false;
            //    }
            //}

            //Type OriginalFunction = reinterpret_cast<Type>(this->OriginalVTable[Index]);
            //if (OriginalFunction != Function)
            //{
            //    int i = 0;
            //    for (;; i++)
            //    {
            //        void* VirtualFunction = (void*)this->OriginalVTable[i];
            //        if (!VirtualFunction)
            //            break;

            //        this->VTableSize++;
            //    }

            //    if (Index >= this->VTableSize)
            //    {
            //        return false;  
            //    }

            //    uintptr_t* AllocatedVTable = reinterpret_cast<uintptr_t*>(LI_FN(calloc).safe_cached()(this->VTableSize * sizeof(uintptr_t), 0x10));
            //    if (!AllocatedVTable)
            //    {
            //        return false;  
            //    }

            //    for (int i = 0; i < this->VTableSize; i++)
            //    {
            //        AllocatedVTable[i] = this->OriginalVTable[i];
            //    }

            //    *Original = OriginalFunction;

            //    AllocatedVTable[Index] = reinterpret_cast<uintptr_t>(Function);

            //  //  *(uintptr_t**)(Address) = AllocatedVTable.release()
            //  // 
            //    *(uintptr_t**)(Address) = AllocatedVTable;
            //    

            //    // Update hook metadata
            //    this->LastHookedFunctionAddress = reinterpret_cast<uintptr_t>(Function);
            //    this->LastHookedFunctionIndex = Index;

            //    return true;
            //}

            //return false;
        }

        template <typename Type>
        void RevertHook( Type* Original, void* Address ) {
            if ( this->VirtualTable && this->LastHookedFunctionIndex ) {
                uintptr_t* VirtualTable = *( uintptr_t** )( Address );

                VirtualTable[ this->LastHookedFunctionIndex ] = reinterpret_cast< uintptr_t >( *Original );

                *( uintptr_t** )Address = this->VirtualTable;

                //FMemory::Free(VirtualTable);
                 free( VirtualTable );

                this->LastHookedFunctionAddress = 0x0;
                this->LastHookedFunctionIndex = -1;
            }
        }

    private:
        uintptr_t* VirtualTable;
        uintptr_t* AllocatedVTable;
        int VTableSize;

        uintptr_t LastHookedFunctionAddress;
        int LastHookedFunctionIndex;
    };
}
