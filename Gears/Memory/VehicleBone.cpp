/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#include "VehicleBone.h"
#include "../Util/MathExt.h"
#include "VehicleExtensions.hpp"

using VExt = VehicleExtensions;

void VehicleBones::RotateAxis(Vehicle vehicle, int index, Vector3 axis, float degrees) {
    auto address = VExt::GetAddress(vehicle);
    auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
    auto inst = reinterpret_cast<fragInstGta*>(fragInstGtaPtr);

    NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[index]);
    Vector3 scalar{};
    scalar.x = 1.0f;
    scalar.y = 1.0f;
    scalar.z = 1.0f;
    NativeMatrix4x4 newMatrix = Scaling(scalar) * RotationAxis(axis, deg2rad(degrees)) * (*matrix);
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
