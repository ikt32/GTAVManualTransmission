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


struct Vector3 {
#pragma warning(push)
#pragma warning(disable : 4324)
    union {
        alignas(8) float x;
        alignas(8) float pitch;
        alignas(8) float attitude;  // NASA standard Aeroplane
    };
    union {
        alignas(8) float y;
        alignas(8) float roll;
        alignas(8) float bank;  // NASA standard Aeroplane
    };
    union {
        alignas(8) float z;
        alignas(8) float yaw;
        alignas(8) float heading;  // NASA standard Aeroplane
    };
#pragma warning(pop)

    template <typename X, typename Y, typename Z>
    constexpr Vector3(X x, Y y, Z z) : x(static_cast<float>(x)), y(static_cast<float>(y)), z(static_cast<float>(z)) {}
    constexpr Vector3() : Vector3(0, 0, 0) {}
    template <typename X>
    explicit constexpr Vector3(X xyz, typename std::enable_if<std::is_arithmetic<X>::value, X>::type* = nullptr) : x(static_cast<float>(xyz)), y(static_cast<float>(xyz)), z(static_cast<float>(xyz)) {}
    explicit constexpr Vector3(float v[3])        :  Vector3(v[0], v[1], v[2]) { }
};