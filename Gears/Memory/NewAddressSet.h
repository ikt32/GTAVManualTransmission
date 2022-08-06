/*
 * Zolika1351 - V SDK
 */

#pragma once

#pragma comment(lib, "version.lib")
#include "NativeMemory.hpp"

namespace AddressSetter
{
	auto timeRelatedAddr = mem::FindPattern("F3 0F ? ? ? ? ? ? 0F 94 C0 F6 81 4A 09 00 00 10");
	auto clockRelatedAddr = mem::FindPattern("48 83 EC 28 89 0D ? ? ? ? 89 15");

	float& GetTimeStep()
	{
		// 0x2D752E4
		static auto p = timeRelatedAddr;

		static auto origlocation = p; // F71F9E

		uint64_t ptr = p + 4;

		ptr = *(uint32_t*)ptr; // 1E0333E

		ptr += 8;

		return *(float*)(origlocation + ptr);
	}

	uint32_t& GetFrameCounter()
	{
		// 0x2D752E0
		static auto p = timeRelatedAddr;

		static auto origlocation = p; // F71F9E

		uint64_t ptr = p + 4;

		ptr = *(uint32_t*)ptr; // 1E0333E

		ptr += 8;

		return *(uint32_t*)(origlocation + ptr - 4);
	}
	//mov cs:dest, register
	uint64_t ReadAddrFromMovFromReg(uint64_t location)
	{
		// value stored: 16D9776
		// at: 637030
		// points to: 1D107AC
		// offset: 1D107A6
		// +6

		// value stored: 16D9774
		// at: 637036
		// points to: 1D107B0
		// offset: 1D107AA
		// +6

		uint64_t origaddr = location;

		int32_t offset32 = *(int32_t*)(origaddr + 2);

		int64_t offset = offset32;

		uint64_t ret = origaddr + offset + 6;

		return ret;
	}

	uint64_t ReadAddrFromRelative(uint64_t location)
	{
		uint64_t origaddr = location;

		auto offset32 = *(int32_t*)(origaddr);

		int64_t offset = offset32;

		uint64_t ret = origaddr + offset + 4;

		return ret;
	}

	uint32_t& GetClockDays()
	{
		static auto p = clockRelatedAddr; // 63702C
		return *(uint32_t*)(ReadAddrFromMovFromReg(p + 4));
	}

	uint32_t& GetClockMonths()
	{
		static auto p = clockRelatedAddr; // 63702C
		return *(uint32_t*)(ReadAddrFromMovFromReg(p + 4));
	}

	template<typename T> T& GetRefFromMovFromReg(uint64_t addr, uint64_t offset = 0)
	{
		if (addr > 0xEFFFFFFFFFFFFFFF) return *reinterpret_cast<T*>(nullptr);
		if (addr < 0x10000000) return *reinterpret_cast<T*>(nullptr);
		return *reinterpret_cast<T*>(ReadAddrFromMovFromReg(addr) + offset);
	}

	template<typename T> T& GetRefFromRelative(uint64_t addr, uint64_t offset = 0)
	{
		if (addr > 0xEFFFFFFFFFFFFFFF) return *reinterpret_cast<T*>(nullptr);
		if (addr < 0x10000000) return *reinterpret_cast<T*>(nullptr);
		return *reinterpret_cast<T*>(ReadAddrFromRelative(addr) + offset);
	}

	template<typename T> T& GetRef(const char* pattern)
	{
		uint64_t addr = mem::FindPattern(pattern);

		return *reinterpret_cast<T*>(addr);
	}
}
