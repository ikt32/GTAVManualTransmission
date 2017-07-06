#pragma once
#include <inc/types.h>

template <typename T> int sgn(T val) {
	return (T(0) < val) - (val < T(0));
}

float lerp(float a, float b, float f);

Vector3 Cross(Vector3 left, Vector3 right);

Vector3 operator + (Vector3 left, Vector3 right);
Vector3 operator * (Vector3 value, float scale);
Vector3 operator * (float scale, Vector3 vec);
Vector3 GetOffsetInWorldCoords(Vector3 position, Vector3 rotation, Vector3 forward, Vector3 offset);

