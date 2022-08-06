/*
 * Original VehicleBone code in C# by AlexGuirre, from VehicleGadgetsPlus
 * https://github.com/alexguirre/VehicleGadgetsPlus
 */

#pragma once
#include "NativeMatrix.h"
#include "NativeVectors.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace VehicleBones {
#pragma pack(push, 1)
struct crBoneData
{
    NativeVector4 Rotation; // 0x00 - 0x0F (NativeVector size = 16 bytes)
    NativeVector3 Translation; // 0x010 - 0x1B
    char pad1[0x32 - 0x1C]; // 0x1C - 0x31
    uint16_t ParentIndex; // 0x32 - 0x33
    char pad0[0x38 - 0x34]; // 0x34 - 0x37
    char* NamePtr; // 0x38 - 0x3F
    char pad4[0x42 - 0x40]; // 0x40 - 0x41
    uint16_t Index; // 0x42 - 0x43
    char pad5[0x50 - 0x44]; // 0x44 - 0x49

    std::string GetName() const {
        if (NamePtr == nullptr)
            return "NULL";
        return NamePtr;
    }
};
#pragma pack(pop)
static_assert(offsetof(crBoneData, Index) == 0x42, "bad alignment");

#pragma pack(push, 1)
struct crSkeletonData
{
    char pad0[0x20]; // 0x00 - 0x1F
    crBoneData* Bones; // 0x20 - 0x27
    NativeMatrix4x4* TransformsInverted; // 0x28 - 0x2F
    NativeMatrix4x4* Transforms; // 0x30 - 0x37
    uint16_t* ParentIndices; // 0x38 - 0x3F
    char pad1[0x5E - 0x40];
    uint16_t NumBones; // 0x5E - 0x5F

    std::string GetBoneNameForIndex(uint index) const {
        if (index >= NumBones)
            return "NULL";

        return Bones[index].GetName();
    }
};
#pragma pack(pop)
static_assert(offsetof(crSkeletonData, NumBones) == 0x5E, "bad alignment");

#pragma pack(push, 1)
struct crSkeleton
{
    crSkeletonData* Data; // 0x0 - 0x7
    NativeMatrix4x4* Transform; // 0x8 - 0xF
    NativeMatrix4x4* ObjectMatrices; // 0x10 - 0x17
    NativeMatrix4x4* GlobalMatrices; // 0x18 - 0x1F
    int NumBones; //0x20 - 0x24
};
#pragma pack(pop)
static_assert(offsetof(crSkeleton, NumBones) == 0x20, "bad alignment");

#pragma pack(push, 1)
struct fragCacheEntry {
    uint8_t pad0[0x118]; // 0x0000 - 0x0117
    int* BoneIndexToTypeComponent; // 0x0118 - 0x11F
    int* TypeComponentToBoneIndex; // 0x0120 - 0x127
    uint8_t pad1[(0x148 + 0x18) - (0x128)]; // 0x128 - 0x15F
    int** BrokenAndHiddenComponentsFlags; // 0x160 - 0x167
    uint8_t pad2[(0x148 + 0x30) - (0x168)]; // 0x168 - 0x177
    crSkeleton* Skeleton;
};
#pragma pack(pop)
static_assert(offsetof(fragCacheEntry, Skeleton) == 0x178, "bad alignment");

#pragma pack(push, 1)
struct fragInstGta {
    //uint8_t pad0[0x18]; // 0x00 - 0x17
    //uint16_t LevelIndex; // 0x18, 0x19
    uint8_t pad1[0x68/* - 0x20*/]; // 0x20 - 0x67
    fragCacheEntry* CacheEntry; // 0x68
};
#pragma pack(pop)
static_assert(offsetof(fragInstGta, CacheEntry) == 0x68, "bad alignment");

#pragma pack(push, 1)
struct CVehicle {
    uint8_t pad0[0x30]; // 0x00 - 0x2E
    fragInstGta* Inst; // 0x30
};
#pragma pack(pop)
static_assert(offsetof(CVehicle, Inst) == 0x30, "bad alignment");

void RotateAxisAbsolute(Vehicle vehicle, int index, Vector3 axis, float radians);
void RotateAxis(Vehicle vehicle, int index, Vector3 axis, float radians);
void Scale(Vehicle vehicle, int boneIndex, Vector3 scalar);

void RegisterMatrix(Vehicle vehicle, const char* boneName);
void RegisterMatrix(Vehicle vehicle, int boneIndex);
}
