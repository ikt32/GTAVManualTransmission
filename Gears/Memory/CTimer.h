/*
 * Zolika1351 - V SDK
 */

#pragma once

#include "NativeMemory.hpp"
#include "NewAddressSet.h"

class CTimer
{
public:
	static inline bool& m_CodePause = AddressSetter::GetRefFromRelative<bool>(mem::FindPattern("0A 05 ? ? ? ? 75 7A 48 8B 35") + 2);
	static inline bool& m_UserPause = AddressSetter::GetRefFromRelative<bool>(mem::FindPattern("0A 05 ? ? ? ? 75 7A 48 8B 35") - 4);
	static inline float& ms_fTimeStep = AddressSetter::GetTimeStep();
	static inline uint32_t& m_FrameCounter = AddressSetter::GetFrameCounter();
};
