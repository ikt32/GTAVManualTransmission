/*
    THIS FILE IS A PART OF GTA V SCRIPT HOOK SDK
                http://dev-c.com
            (C) Alexander Blade 2015
*/

#pragma once

#include <Windows.h>
#include <type_traits>


typedef DWORD Void;
typedef DWORD Any;
typedef DWORD uint;
typedef DWORD Hash;
typedef int Entity;
typedef int Player;
typedef int FireId;
typedef int Ped;
typedef int Vehicle;
typedef int Cam;
typedef int CarGenerator;
typedef int Group;
typedef int Train;
typedef int Pickup;
typedef int Object;
typedef int Weapon;
typedef int Interior;
typedef int Blip;
typedef int Texture;
typedef int TextureDict;
typedef int CoverPoint;
typedef int Camera;
typedef int TaskSequence;
typedef int ColourIndex;
typedef int Sphere;
typedef int ScrHandle;

#pragma warning(push)
#pragma warning(disable : 4324)
struct Vector2 {
    alignas(8) float x;
    alignas(8) float y;
};


struct Vector3 {
    alignas(8) float x;
    alignas(8) float y;
    alignas(8) float z;
};

struct Vector4 {
    alignas(8) float x;
    alignas(8) float y;
    alignas(8) float z;
    alignas(8) float w;
};
#pragma warning(pop)
