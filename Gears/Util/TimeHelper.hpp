#pragma once

long long milliseconds_now();
//long long milliseconds_now() { // static
//	LARGE_INTEGER s_frequency; // static
//	BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency); // static
//	if (s_use_qpc) {
//		LARGE_INTEGER now;
//		QueryPerformanceCounter(&now);
//		return (1000LL * now.QuadPart) / s_frequency.QuadPart;
//	}
//	return GetTickCount64();
//}
