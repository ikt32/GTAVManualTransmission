/*
 * This file is part of the CitizenFX project - http://citizen.re/
 */

#pragma once
#include <cstdint>
#include <intrin.h>

namespace rage {
    void init();
    class sysMemAllocator
    {
    public:
        virtual ~sysMemAllocator() = 0;

        virtual void m_4() = 0;
        virtual void* allocate(size_t size, size_t align, int subAllocator) = 0;
        virtual void* m_C(size_t size, size_t align, int subAllocator) = 0;
        virtual void free(void* pointer) = 0;

        // and a lot of other functions below that aren't needed right now

    public:
        static uint32_t GetAllocatorTlsOffset();

        static sysMemAllocator* UpdateAllocatorValue();
    };

    inline sysMemAllocator* GetAllocator()
    {
        sysMemAllocator* allocator = *(sysMemAllocator**)(*(uintptr_t*)(__readgsqword(88)) + sysMemAllocator::GetAllocatorTlsOffset());

        if (!allocator)
        {
            return sysMemAllocator::UpdateAllocatorValue();
        }

        return allocator;
    }

    class sysUseAllocator
    {
    public:
        void* operator new(size_t size);

        inline void* operator new[](size_t size)
        {
            return sysUseAllocator::operator new(size);
        }

        void operator delete(void* memory);

        inline void operator delete[](void* memory)
        {
            return sysUseAllocator::operator delete(memory);
        }
    };
}