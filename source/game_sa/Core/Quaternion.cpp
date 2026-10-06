/*
    Plugin-SDK file
    Authors: GTA Community. See more here
    https://github.com/DK22Pac/plugin-sdk
    Do not delete this comment block. Respect others' work!
*/

#include "StdInc.h"

#include "Quaternion.h"

void CQuaternion::InjectHooks() {
    RH_ScopedClass(CQuaternion);
    RH_ScopedCategory("Core");

    RH_ScopedOverloadedInstall(Get, "", 0x59C080, void (CQuaternion::*)(RwMatrix*) const);
    RH_ScopedOverloadedInstall(Get, "Euler", 0x59C160, void (CQuaternion::*)(float*, float*, float*));
    RH_ScopedOverloadedInstall(Get, "AxisAngle", 0x59C230, void (CQuaternion::*)(RwV3d*, float*));
    RH_ScopedInstall(Multiply, 0x59C270);
    RH_ScopedOverloadedInstall(Slerp, "Theta", 0x59C300, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float, float, float));
    RH_ScopedOverloadedInstall(Set, "Matrix", 0x59C3E0, void (CQuaternion::*)(const RwMatrix&));
    RH_ScopedOverloadedInstall(Set, "Euler", 0x59C530, void (CQuaternion::*)(float, float, float));
    RH_ScopedOverloadedInstall(Set, "AxisAngle", 0x59C600, void (CQuaternion::*)(RwV3d*, float));
    RH_ScopedOverloadedInstall(Slerp, "Auto", 0x59C630, void (CQuaternion::*)(const CQuaternion&, const CQuaternion&, float));
    RH_ScopedInstall(Conjugate, 0x4D37D0);
    RH_ScopedInstall(Scale, 0x4CF9B0);
    RH_ScopedInstall(Copy, 0x4CF9E0);
}

// Quat to matrix
void CQuaternion::Get(RwMatrix* out) const {
    auto vecImag2 = imag + imag;
    auto x2x = vecImag2.x * imag.x;
    auto y2x = vecImag2.y * imag.x;
    auto z2x = vecImag2.z * imag.x;

    auto y2y = vecImag2.y * imag.y;
    auto z2y = vecImag2.z * imag.y;
    auto z2z = vecImag2.z * imag.z;

    auto x2r = vecImag2.x * real;
    auto y2r = vecImag2.y * real;
    auto z2r = vecImag2.z * real;

    CVector right{1.0F - (z2z + y2y), z2r + y2x, z2x - y2r}, up{y2x - z2r, 1.0F - (z2z + x2x), x2r + z2y}, at{y2r + z2x, z2y - x2r, 1.0F - (y2y + x2x)};
    RwV3dAssign(RwMatrixGetRight(out), &right);
    RwV3dAssign(RwMatrixGetUp(out), &up);
    RwV3dAssign(RwMatrixGetAt(out), &at);
}

// Quat to euler angles
// 0x59C160
void CQuaternion::Get(float* x, float* y, float* z) {
    const auto WrapAngle = [](float a) {
        return a < 0.0f ? a + 2.0f * 3.14159274f : a;
    };

    RwMatrix mat;
    Get(&mat);

    *z = WrapAngle(std::atan2(mat.right.y, mat.up.y));

    const auto s = std::sin(*z);
    const auto c = std::cos(*z);

    *x = WrapAngle(std::atan2(-mat.at.y, s * mat.right.y + c * mat.up.y));
    *y = WrapAngle(std::atan2(-(mat.right.z * c - mat.up.z * s), mat.right.x * c - mat.up.x * s));
}

// Quat to axis & angle
// 0x59C230
void CQuaternion::Get(RwV3d* axis, float* angle) {
    *angle = std::acos(w + w); // NOTE: Original code really does `acos(2 * w)` (Most likely meant to be `2 * acos(w)`)

    const auto invSin = 1.0f / std::sin(*angle);
    axis->x = invSin * x;
    axis->y = invSin * y;
    axis->z = invSin * z;
}

// Stores result of quat multiplication
// 0x59C270
void CQuaternion::Multiply(const CQuaternion& a, const CQuaternion& b) {
    // NOTE: Order of operations (and writes) is the same as in the original code (matters if `this` aliases `a` or `b`)
    x = b.z * a.y - a.z * b.y;
    y = a.z * b.x - a.x * b.z;
    z = a.x * b.y - b.x * a.y;

    x = b.w * a.x + a.w * b.x + x;
    y = a.w * b.y + b.w * a.y + y;
    z = a.z * b.w + a.w * b.z + z;

    w = a.w * b.w - (a.x * b.x + a.z * b.z + a.y * b.y);
}

// Spherical linear interpolation
// 0x59C300
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float halftheta, float sintheta_inv, float t) {
    if (halftheta == 0.0f) {
        *this = to;
        return;
    }

    float fromWeight, toWeight;
    if (halftheta > 3.14159274f / 2.0f) {
        halftheta  = 3.14159274f - halftheta;
        fromWeight = std::sin((1.0f - t) * halftheta) * sintheta_inv;
        toWeight   = -(std::sin(halftheta * t) * sintheta_inv);
    } else {
        fromWeight = std::sin((1.0f - t) * halftheta) * sintheta_inv;
        toWeight   = std::sin(halftheta * t) * sintheta_inv;
    }

    x = fromWeight * from.x + toWeight * to.x;
    y = toWeight * to.y + fromWeight * from.y;
    z = toWeight * to.z + fromWeight * from.z;
    w = toWeight * to.w + fromWeight * from.w;
}

// Quat from matrix
// 0x59C3E0
void CQuaternion::Set(const RwMatrix& m) {
    if (const auto trace = m.up.y + m.right.x + m.at.z; trace >= 0.0f) {
        const auto s    = std::sqrt(trace + 1.0f);
        const auto invS = 0.5f / s;
        w = 0.5f * s;
        x = (m.up.z - m.at.y) * invS;
        y = (m.at.x - m.right.z) * invS;
        z = (m.right.y - m.up.x) * invS;
    } else if (const auto tx = m.right.x - m.up.y - m.at.z; tx >= 0.0f) {
        const auto s    = std::sqrt(tx + 1.0f);
        const auto invS = 0.5f / s;
        x = 0.5f * s;
        y = (m.up.x + m.right.y) * invS;
        z = (m.at.x + m.right.z) * invS;
        w = (m.up.z - m.at.y) * invS;
    } else if (const auto ty = m.up.y - m.right.x - m.at.z; ty >= 0.0f) {
        const auto s    = std::sqrt(ty + 1.0f);
        const auto invS = 0.5f / s;
        y = 0.5f * s;
        w = (m.at.x - m.right.z) * invS;
        x = (m.up.x - m.right.y) * invS; // NOTE: Original code really does subtract here (Should most likely be `+`)
        z = (m.at.y + m.up.z) * invS;
    } else {
        const auto s    = std::sqrt(m.at.z - (m.up.y + m.right.x) + 1.0f);
        const auto invS = 0.5f / s;
        z = 0.5f * s;
        w = (m.right.y - m.up.x) * invS;
        x = (m.at.x + m.right.z) * invS;
        y = (m.at.y + m.up.z) * invS;
    }
}

// Quat from euler angles
// 0x59C530
void CQuaternion::Set(float x, float y, float z) {
    const auto cx = std::cos(x * 0.5f), sx = std::sin(x * 0.5f);
    const auto cy = std::cos(y * 0.5f), sy = std::sin(y * 0.5f);
    const auto cz = std::cos(z * 0.5f), sz = std::sin(z * 0.5f);

    const auto cycx = cy * cx;
    const auto sysx = sy * sx;
    const auto sxcy = sx * cy;
    const auto cxsy = cx * sy;

    this->w = sysx * sz + cycx * cz;
    this->x = cycx * sz - sysx * cz;
    this->y = sz * cxsy + sxcy * cz;
    this->z = cxsy * cz - sxcy * sz;
}

// Quat from axis & angle
// 0x59C600
void CQuaternion::Set(RwV3d* axis, float angle) {
    const auto halfAngle = angle * 0.5f;
    const auto s = std::sin(halfAngle);
    x = s * axis->x;
    y = s * axis->y;
    z = s * axis->z;
    w = std::cos(halfAngle);
}

// Spherical linear interpolation
// 0x59C630
void CQuaternion::Slerp(const CQuaternion& from, const CQuaternion& to, float t) {
    // Inlined code of the (cdecl) function at 0x4D00E0
    const auto theta       = std::acos(std::min(DotProduct(from, to), 1.0f));
    const auto invSinTheta = theta == 0.0f
        ? 0.0f
        : 1.0f / std::sin(theta);
    Slerp(from, to, theta, invSinTheta, t);
}

// Conjugate of a quat
// 0x4D37D0
void CQuaternion::Conjugate() {
    x = -x;
    y = -y;
    z = -z;
}

// Squared length of a quat
float CQuaternion::GetLengthSquared() const {
    // Originally NOP.
    return sq(x) + sq(y) + sq(z) + sq(w);
}

// Multiplies quat by a floating point value
// 0x4CF9B0
void CQuaternion::Scale(float multiplier) {
    x *= multiplier;
    y *= multiplier;
    z *= multiplier;
    w *= multiplier;
}

// Copies value from other quat
// 0x4CF9E0
void CQuaternion::Copy(const CQuaternion& from) {
    x = from.x;
    y = from.y;
    z = from.z;
    w = from.w;
}

// Gets a dot product for quats
float CQuaternion::Dot(const CQuaternion& rhs) {
    return this->w * rhs.w + this->z * rhs.z + this->y * rhs.y + this->x * rhs.x;
}

// Normalises a quat
void CQuaternion::Normalise() {
    const auto sqMag = GetLengthSquared();
    if (sqMag == 0.f) {
        w = 1.0;
    } else {
        *this = *this / std::sqrt(sqMag);
    }
}
