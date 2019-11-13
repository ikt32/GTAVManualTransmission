#pragma once
#include <inc/types.h>
#include <vector>

#define _USE_MATH_DEFINES
#include <math.h>

template <typename T> T sgn(T val) {
    return static_cast<T>((T{} < val) - (val < T{}));
}

template<typename T, typename A>
T avg(std::vector<T, A> const& vec) {
    T average{};
    for (auto elem : vec)
        average += elem;
    return average / static_cast<T>(vec.size());
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
T rad2deg(T rad) {
    return static_cast<T>(static_cast<double>(rad) * (180.0 / M_PI));
}

template <typename T, typename = typename std::enable_if<std::is_floating_point<T>::value, T>::type>
T deg2rad(T deg) {
    return static_cast<T>(static_cast<double>(deg) * M_PI / 180.0);
}

template <typename T>
T map(T x, T in_min, T in_max, T out_min, T out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

namespace Math {
    template <typename T>
    bool Near(T a, T b, T deviation) {
        return (a > b - deviation && a < b + deviation);
    }
}

float lerp(float a, float b, float f);

float Length(Vector3 vec);
float Distance(Vector3 vec1, Vector3 vec2);
float Dot(Vector3 a, Vector3 b);
Vector3 Cross(Vector3 left, Vector3 right);
Vector3 operator + (Vector3 left, Vector3 right);
Vector3 operator - (Vector3 left, Vector3 right);
Vector3 operator * (Vector3 value, float scale);
Vector3 operator * (float scale, Vector3 vec);
Vector3 Normalize(Vector3 vec);
Vector3 GetOffsetInWorldCoords(Vector3 position, Vector3 rotation, Vector3 forward, Vector3 offset);
float GetAngleBetween(float h1, float h2, float separation);
float GetAngleBetween(Vector3 a, Vector3 b);
