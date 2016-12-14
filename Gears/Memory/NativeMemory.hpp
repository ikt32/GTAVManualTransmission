// Implementation provided by LeFix

#pragma once

#include <cstdint>


struct MemoryPool {
	uintptr_t ListAddr;
	char* BoolAdr;
	int MaxCount;
	int ItemSize;
};

class MemoryAccess {
public:
	MemoryAccess();

	uintptr_t GetAddressOfEntity(int Handle) const;
	static uintptr_t FindPattern(const char* pattern, const char* mask);

private:
	static uintptr_t GetAddressOfItemInPool(MemoryPool* PoolAddress, int Handle);
	const char* EntityPoolOpcodeMask = "xxx????xxxxxxx";
	const char* EntityPoolOpcodePattern = "\x4C\x8B\x0D\x00\x00\x00\x00\x44\x8B\xC1\x49\x8B\x41\x08";
	MemoryPool** sAddressEntityPool = nullptr;
};
