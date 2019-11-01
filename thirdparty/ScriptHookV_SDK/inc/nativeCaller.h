/*
	THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
				http://dev-c.com			
			(C) Alexander Blade 2015
*/

#pragma once

#include "main.h"
#include <utility>

template <typename T>
static inline void nativePush(T val)
{
	UINT64 val64 = 0;
	if (sizeof(T) > sizeof(UINT64))
	{
		throw "error, value size > 64 bit";
	}
	*reinterpret_cast<T *>(&val64) = val; // &val + sizeof(dw) - sizeof(val)
	nativePush64(val64);
}

template <typename R>
static inline R invoke(UINT64 hash)
{
	nativeInit(hash);
	return *reinterpret_cast<R *>(nativeCall());
}

template <typename R, class ... Args>
static inline R invoke(UINT64 hash, Args&& ... args)
{
    nativeInit(hash);

    (nativePush(std::forward<Args>(args)), ...);

    return *reinterpret_cast<R*>(nativeCall());
}