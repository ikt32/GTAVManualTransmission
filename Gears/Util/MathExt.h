#pragma once
#include <inc/types.h>
#include <vector>

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

template<typename T, typename A>
T avg(std::vector<T, A> const& vec) {
	T average{};
	for (auto elem : vec)
		average += elem;
	return average / sizeof(vec);
}

float lerp(float a, float b, float f);

Vector3 Cross(Vector3 left, Vector3 right);

Vector3 operator + (Vector3 left, Vector3 right);
Vector3 operator * (Vector3 value, float scale);
Vector3 operator * (float scale, Vector3 vec);
Vector3 GetOffsetInWorldCoords(Vector3 position, Vector3 rotation, Vector3 forward, Vector3 offset);

