/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#include "VehicleBone.h"
#include "VehicleExtensions.hpp"
#include <inc/natives.h>
#include <map>

using VExt = VehicleExtensions;

namespace {
    std::map<std::pair<Hash, int>, NativeMatrix4x4> matrixCache;
}

void VehicleBones::RotateAxisAbsolute(Vehicle vehicle, int index, Vector3 axis, float radians) {
    auto model = ENTITY::GET_ENTITY_MODEL(vehicle);
    auto originalMatrix = matrixCache.find(std::make_pair(model, index));
    if (originalMatrix == matrixCache.end()) {
        return;
    }

    auto address = VExt::GetAddress(vehicle);
    auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
    auto inst = reinterpret_cast<fragInstGta*>(fragInstGtaPtr);

    NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[index]);

    NativeMatrix4x4 o = originalMatrix->second;

    Vector3 scalar {0.0f};

    o = Scaling(scalar) * RotationAxis(axis, radians) * o;

    *matrix = o;
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

void VehicleBones::RegisterMatrix(Vehicle vehicle, const char* boneName) {
    auto boneIdx = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(vehicle, boneName);
    if (boneIdx != -1)
        VehicleBones::RegisterMatrix(vehicle, boneIdx);
}

void VehicleBones::RegisterMatrix(Vehicle vehicle, int boneIndex) {
    auto model = ENTITY::GET_ENTITY_MODEL(vehicle);

    if (matrixCache.find(std::make_pair(model, boneIndex)) == matrixCache.end()) {
        auto address = VExt::GetAddress(vehicle);
        auto fragInstGtaPtr = *reinterpret_cast<uint64_t*>(address + 0x30);
        auto inst = reinterpret_cast<VehicleBones::fragInstGta*>(fragInstGtaPtr);

        NativeMatrix4x4* matrix = &(inst->CacheEntry->Skeleton->ObjectMatrices[boneIndex]);

        matrixCache[std::make_pair(model, boneIndex)] = *matrix;
    }
}
