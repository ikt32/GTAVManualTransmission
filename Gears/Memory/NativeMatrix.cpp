/*
 * Original Matrix4x4 code in C# from SharpDX
 * https://github.com/sharpdx/SharpDX
 */

#include "NativeMatrix.h"
#include <cmath>

NativeMatrix4x4 Scaling(Vector3 scale) {
    // identity
    NativeMatrix4x4 result{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };

    result.M11 = scale.x;
    result.M22 = scale.y;
    result.M33 = scale.z;

    return result;
}

NativeMatrix4x4 RotationAxis(Vector3 axis, float angle) {
    NativeMatrix4x4 result{
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;

    float cos_ = cos(angle);
    float sin_ = sin(angle);
    float xx = x * x;
    float yy = y * y;
    float zz = z * z;
    float xy = x * y;
    float xz = x * z;
    float yz = y * z;

    result.M11 = xx + (cos_ * (1.0f - xx));
    result.M12 = (xy - (cos_ * xy)) + (sin_ * z);
    result.M13 = (xz - (cos_ * xz)) - (sin_ * y);
    result.M21 = (xy - (cos_ * xy)) - (sin_ * z);
    result.M22 = yy + (cos_ * (1.0f - yy));
    result.M23 = (yz - (cos_ * yz)) + (sin_ * x);
    result.M31 = (xz - (cos_ * xz)) + (sin_ * y);
    result.M32 = (yz - (cos_ * yz)) - (sin_ * x);
    result.M33 = zz + (cos_ * (1.0f - zz));
    return result;
}

NativeMatrix4x4 Multiply(const NativeMatrix4x4& left, const NativeMatrix4x4& right) {
    NativeMatrix4x4 temp;
    temp.M11 = (left.M11 * right.M11) + (left.M12 * right.M21) + (left.M13 * right.M31) + (left.M14 * right.M41);
    temp.M12 = (left.M11 * right.M12) + (left.M12 * right.M22) + (left.M13 * right.M32) + (left.M14 * right.M42);
    temp.M13 = (left.M11 * right.M13) + (left.M12 * right.M23) + (left.M13 * right.M33) + (left.M14 * right.M43);
    temp.M14 = (left.M11 * right.M14) + (left.M12 * right.M24) + (left.M13 * right.M34) + (left.M14 * right.M44);
    temp.M21 = (left.M21 * right.M11) + (left.M22 * right.M21) + (left.M23 * right.M31) + (left.M24 * right.M41);
    temp.M22 = (left.M21 * right.M12) + (left.M22 * right.M22) + (left.M23 * right.M32) + (left.M24 * right.M42);
    temp.M23 = (left.M21 * right.M13) + (left.M22 * right.M23) + (left.M23 * right.M33) + (left.M24 * right.M43);
    temp.M24 = (left.M21 * right.M14) + (left.M22 * right.M24) + (left.M23 * right.M34) + (left.M24 * right.M44);
    temp.M31 = (left.M31 * right.M11) + (left.M32 * right.M21) + (left.M33 * right.M31) + (left.M34 * right.M41);
    temp.M32 = (left.M31 * right.M12) + (left.M32 * right.M22) + (left.M33 * right.M32) + (left.M34 * right.M42);
    temp.M33 = (left.M31 * right.M13) + (left.M32 * right.M23) + (left.M33 * right.M33) + (left.M34 * right.M43);
    temp.M34 = (left.M31 * right.M14) + (left.M32 * right.M24) + (left.M33 * right.M34) + (left.M34 * right.M44);
    temp.M41 = (left.M41 * right.M11) + (left.M42 * right.M21) + (left.M43 * right.M31) + (left.M44 * right.M41);
    temp.M42 = (left.M41 * right.M12) + (left.M42 * right.M22) + (left.M43 * right.M32) + (left.M44 * right.M42);
    temp.M43 = (left.M41 * right.M13) + (left.M42 * right.M23) + (left.M43 * right.M33) + (left.M44 * right.M43);
    temp.M44 = (left.M41 * right.M14) + (left.M42 * right.M24) + (left.M43 * right.M34) + (left.M44 * right.M44);
    return temp;
}

NativeMatrix4x4 operator*(const NativeMatrix4x4& left, const NativeMatrix4x4& right) {
    return Multiply(left, right);
}
