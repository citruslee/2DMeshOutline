#pragma once
#include <DirectXMath.h>

using namespace DirectX;

#pragma warning(disable:4018)

inline XMFLOAT3& operator+=(XMFLOAT3 &l, const XMFLOAT3 &r) { l.x += r.x; l.y += r.y; l.z += r.z; return l; }
inline XMFLOAT3& operator*=(XMFLOAT3 &l, float r) { l.x *= r; l.y *= r; l.z *= r; return l; }
inline XMFLOAT3 operator-(const XMFLOAT3 &l, const XMFLOAT3 &r) { return XMFLOAT3(l.x - r.x, l.y - r.y, l.z - r.z); }
inline XMFLOAT3 operator*(const XMFLOAT3 &l, float r) { return XMFLOAT3(l.x*r, l.y*r, l.z*r); }
inline XMFLOAT3 operator/(const XMFLOAT3 &l, float r) { return XMFLOAT3(l.x / r, l.y / r, l.z / r); }

inline float Dot(const XMFLOAT3 a, const XMFLOAT3 b)
{
	return a.x*b.x + a.y*b.y + a.z*b.z;
}

inline float Len(const XMFLOAT3 a)
{
	return sqrt(Dot(a, a));
}

inline XMFLOAT3 Normalize(XMFLOAT3 a)
{
	return a / Len(a);
}