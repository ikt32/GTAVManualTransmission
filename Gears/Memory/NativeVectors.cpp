/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#include "NativeVectors.h"

Vector3 GetVector3(NativeVector3 value) {
    Vector3 v{};
    v.x = value.X;
    v.y = value.Y;
    v.z = value.Z;
    return v;
}

NativeVector3 GetNativeVector3(Vector3 value) {
    return NativeVector3{value.x, value.y, value.z};
}

Vector4 GetVector4(NativeVector4 value) {
    return Vector4{value.X, value.Y, value.Z, value.W};
}

Quaternion GetQuaternion(NativeVector4 value) {
    return Quaternion{value.X, value.Y, value.Z, value.W};
}

NativeVector4 GetNativeVector4(Vector4 value) {
    return NativeVector4{value.X, value.Y, value.Z, value.W};
}

NativeVector4 GetNativeVector4(Quaternion value) {
    return NativeVector4{value.X, value.Y, value.Z, value.W};
}
