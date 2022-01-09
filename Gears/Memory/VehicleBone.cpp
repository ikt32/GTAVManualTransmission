/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#include "VehicleBone.h"
#include "../Util/MathExt.h"
#include "VehicleExtensions.hpp"

using VExt = VehicleExtensions;

Vector3 GetRotation(NativeMatrix4x4 matrix) {
    float rotXangle = 0.0f;
    float rotYangle = 0.0f;
    float rotZangle = 0.0f;

    rotXangle = atan2(-matrix.M2().Z, matrix.M3().Z);
    float cosYangle = sqrt(pow(matrix.M1().X, 2) + pow(matrix.M1().Y, 2));
    rotYangle = atan2(matrix.M1().Z, cosYangle);
    float sinXangle = sin(rotXangle);
    float cosXangle = cos(rotXangle);
    rotZangle = atan2(cosXangle * matrix.M2().X + sinXangle * matrix.M3().X, cosXangle * matrix.M2().Y + sinXangle * matrix.M3().Y);

    Vector3 vals{};
    vals.x = rotXangle;
    vals.y = rotYangle;
    vals.z = rotZangle;
    return vals;
}

void VehicleBones::RotateAxisAbsolute(Vehicle vehicle, int index, Vector3 axis, float radians) {
    auto address = VExt::GetAddress(vehicle);
    auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
    auto inst = reinterpret_cast<fragInstGta*>(fragInstGtaPtr);

    NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[index]);

    auto vrot = GetRotation(*matrix);
    
    float toRotate = vrot.y + radians;
    
    Vector3 scalar{};
    scalar.x = 1.0f;
    scalar.y = 1.0f;
    scalar.z = 1.0f;
    NativeMatrix4x4 newMatrix = Scaling(scalar) * RotationAxis(axis, toRotate) * (*matrix);
    *matrix = newMatrix;
}

void VehicleBones::RotateAxis(Vehicle vehicle, int index, Vector3 axis, float radians) {
    auto address = VExt::GetAddress(vehicle);
    auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
    auto inst = reinterpret_cast<fragInstGta*>(fragInstGtaPtr);

    NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[index]);
    Vector3 scalar{};
    scalar.x = 1.0f;
    scalar.y = 1.0f;
    scalar.z = 1.0f;
    NativeMatrix4x4 newMatrix = Scaling(scalar) * RotationAxis(axis, radians) * (*matrix);
    *matrix = newMatrix;
}

void VehicleBones::Scale(Vehicle vehicle, int boneIndex, Vector3 scalar) {
    auto address = VExt::GetAddress(vehicle);
    auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
    auto inst = reinterpret_cast<fragInstGta*>(fragInstGtaPtr);

    NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[boneIndex]);
    NativeMatrix4x4 newMatrix = Scaling(scalar) * (*matrix);
    *matrix = newMatrix;
}
