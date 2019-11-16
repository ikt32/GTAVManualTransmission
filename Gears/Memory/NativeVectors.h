/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#pragma once
#include "inc/types.h"

#pragma pack(push, 1)
struct NativeVector3
{
    float X;
    float Y;
    float Z;
};
#pragma pack(pop)

Vector3 GetVector3(NativeVector3 value);

NativeVector3 GetNativeVector3(Vector3 value);

struct Vector4 {
    float X;
    float Y;
    float Z;
    float W;
};

struct Quaternion {
    float X;
    float Y;
    float Z;
    float W;
};

#pragma pack(push, 1)
struct NativeVector4
{
    float X;
    float Y;
    float Z;
    float W;

    NativeVector4(float x, float y, float z, float w)
    {
        X = x;
        Y = y;
        Z = z;
        W = w;
    }
};
#pragma pack(pop)

Vector4 GetVector4(NativeVector4 value);
Quaternion GetQuaternion(NativeVector4 value);
NativeVector4 GetNativeVector4(Vector4 value);
NativeVector4 GetNativeVector4(Quaternion value);
