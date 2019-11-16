/*
 * Original Matrix4x4 code in C# from SharpDX
 * https://github.com/sharpdx/SharpDX
 */

#pragma once
#include "NativeVectors.h"

#pragma pack(push, 1)
struct NativeMatrix4x4
{
    float M11;
    float M12;
    float M13;
    float M14;

    float M21;
    float M22;
    float M23;
    float M24;

    float M31;
    float M32;
    float M33;
    float M34;

    float M41;
    float M42;
    float M43;
    float M44;

    NativeVector4 M1() { return NativeVector4(M11, M12, M13, M14); }
    NativeVector4 M2() { return NativeVector4(M21, M22, M23, M24); }
    NativeVector4 M3() { return NativeVector4(M31, M32, M33, M34); }
    NativeVector4 M4() { return NativeVector4(M41, M42, M43, M44); }
};
#pragma pack(pop)

NativeMatrix4x4 Scaling(Vector3 scale);
NativeMatrix4x4 RotationAxis(Vector3 axis, float angle);
NativeMatrix4x4 Multiply(const NativeMatrix4x4& left, const NativeMatrix4x4& right);
NativeMatrix4x4 operator *(const NativeMatrix4x4& left, const NativeMatrix4x4& right);
