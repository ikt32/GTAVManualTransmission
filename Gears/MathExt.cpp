#include "MathExt.h"
#include <math.h>
#include <inc/types.h>

float lerp(float a, float b, float f) {
	return a + f * (b - a);
}

Vector3 Cross(Vector3 left, Vector3 right) {
	Vector3 result;
	result.x = left.y * right.z - left.z * right.y;
	result.y = left.z * right.x - left.x * right.z;
	result.z = left.x * right.y - left.y * right.x;
	return result;
}

Vector3 operator + (Vector3 left, Vector3 right) {
	return Vector3{
		left.x + right.x,
		0,
		left.y + right.y,
		0,
		left.z + right.z
	};
}

Vector3 operator * (Vector3 value, float scale) {
	return Vector3{
		value.x * scale,
		0,
		value.y * scale,
		0,
		value.z * scale ,
		0
	};
}

Vector3 operator * (float scale, Vector3 vec) {
	return vec * scale;
}

Vector3 GetOffsetInWorldCoords(Vector3 position, Vector3 rotation, Vector3 forward, Vector3 offset) {
	const float deg2Rad = 0.01745329251994329576923690768489;
	float num1 = cosf(rotation.y * deg2Rad);
	float x = num1 * cosf(-rotation.z  * deg2Rad);
	float y = num1 * sinf(rotation.z  * deg2Rad);
	float z = sinf(-rotation.y * deg2Rad);
	Vector3 right = { x, 0, y, 0, z, 0 };
	Vector3 up = Cross(right, forward);
	return position + (right * offset.x) + (forward * offset.y) + (up * offset.z);
}
