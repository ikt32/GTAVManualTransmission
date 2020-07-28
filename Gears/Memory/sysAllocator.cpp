/*
 * This file is part of the CitizenFX project - http://citizen.re/
 */

#include "sysAllocator.h"

#include <cassert>

#include "NativeMemory.hpp"
#include "../Util/Logger.hpp"

namespace rage {

    static sysMemAllocator* g_gtaTlsEntry;
    
    void* sysUseAllocator::operator new(size_t size)
    {
        return GetAllocator()->allocate(size, 16, 0);
    }

    void sysUseAllocator::operator delete(void* memory)
    {
        GetAllocator()->free(memory);
    }

    static uint32_t g_tlsOffset;

    uint32_t sysMemAllocator::GetAllocatorTlsOffset()
    {
        return g_tlsOffset;
    }

    sysMemAllocator* sysMemAllocator::UpdateAllocatorValue()
    {
        assert(g_gtaTlsEntry);

        *(sysMemAllocator**)(*(uintptr_t*)(__readgsqword(88)) + sysMemAllocator::GetAllocatorTlsOffset()) = g_gtaTlsEntry;
        *(sysMemAllocator**)(*(uintptr_t*)(__readgsqword(88)) + sysMemAllocator::GetAllocatorTlsOffset() - 8) = g_gtaTlsEntry;

        return g_gtaTlsEntry;
    }

    void init() {
        auto addr = mem::FindPattern("B9 ? ? ? ? 48 8B 0C 01 45 33 C9 49 8B D2 48");
        if (addr == 0) return;
        rage::g_tlsOffset = *(uint32_t*)(addr + 1);
        logger.Write(DEBUG, "g_tlsOffset = 0x%X", g_tlsOffset);
        
        g_gtaTlsEntry = *(sysMemAllocator**)(*(uintptr_t*)(__readgsqword(88)) + sysMemAllocator::GetAllocatorTlsOffset());
        logger.Write(DEBUG, "g_gtaTlsEntry = 0x%X", g_gtaTlsEntry);
    }
}

