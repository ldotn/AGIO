#pragma once
#include <immintrin.h>
#include "Float4.h"

// IMPORTANT! Alignment will force the use of padding when using this inside a struct
struct alignas(16) float3
{
private:
	static const inline __m128 ElementMask = _mm_castsi128_ps(_mm_set_epi32(0xffffffff, 0xffffffff, 0xffffffff, 0));
public:
	union
	{
		__m128 mem;
		struct
		{
			float x, y, z;
			float w_;
		};
	};

	float3() { _mm_xor_ps(mem, mem); }
	float3(float v) { _mm_xor_ps(mem, mem); x = y = z = v; }
	float3(float _x, float _y, float _z)
	{
		_mm_xor_ps(mem, mem);
		x = _x; y = _y; z = _z;
	}
	float3(float4 v) { x = v.x; y = v.y; z = v.z; }

	inline bool operator==(const float3& rhs) { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z); }
	inline bool operator!=(const float3& rhs) { return !(*this == rhs); }

	float3& operator+=(const float3& rhs)
	{
		mem = _mm_add_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	// friends defined inside class body are inline and are hidden from non-ADL lookup
	friend float3 operator+(float3 lhs, const float3& rhs)
	{
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}

	float3& operator-=(const float3& rhs)
	{
		mem = _mm_sub_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float3 operator-(float3 lhs, const float3& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	float3& operator*=(const float3& rhs)
	{
		mem = _mm_mul_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float3 operator*(float3 lhs, const float3& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	float3& operator/=(const float3& rhs)
	{
		mem = _mm_div_ps(mem, rhs.mem);
		mem = _mm_and_ps(mem, ElementMask);

		return *this; // return the result by reference
	}

	friend float3 operator/(float3 lhs, const float3& rhs)
	{
		lhs /= rhs;
		return lhs;
	}

	// w component returns 0
	float3 cross(const float3& rhs) const
	{
		__m128 a = _mm_shuffle_ps(mem, mem, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 b = _mm_shuffle_ps(mem, mem, _MM_SHUFFLE(3, 1, 0, 2));

		__m128 m0 = _mm_mul_ps(a, rhs.mem);
		__m128 m1 = _mm_mul_ps(b, rhs.mem);

		m0 = _mm_shuffle_ps(m0, m0, _MM_SHUFFLE(3, 0, 2, 1));
		m1 = _mm_shuffle_ps(m1, m1, _MM_SHUFFLE(3, 1, 0, 2));

		// Swap here to correct sign. Why is this needed?
		__m128 p1 = _mm_sub_ps(m1, m0);

		float3 r;
		p1 = _mm_and_ps(p1, ElementMask);

		r.mem = p1;
		return r;
	}

	float length_sqr() const
	{
		__m128 m = _mm_mul_ps(mem, mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		float r;
		_mm_store_ss(&r, p1);
		return r;
	}
	float length() const
	{
		__m128 m = _mm_mul_ps(mem, mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		__m128 p2 = _mm_sqrt_ps(p1);
		float r;
		_mm_store_ss(&r, p2);
		return r;
	}
	float3 normalize()
	{
		__m128 m = _mm_mul_ps(mem, mem);

		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		//__m128 p2 = _mm_rsqrt_ps(p1); // approximation
		__m128 p2 = _mm_sqrt_ps(p1);

		//mem = _mm_mul_ps(mem, p2);
		mem = _mm_div_ps(mem, p2);
		mem = _mm_and_ps(mem, ElementMask);

		return *this;
	}
	float dot(const float3& rhs) const
	{
		__m128 m = _mm_mul_ps(mem, rhs.mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		float r;
		_mm_store_ss(&r, p1);
		return r;
	}

	float3 clamp(const float3& a, const float3& b)
	{
		// min(max(x,a),b)
		auto m0 = _mm_max_ps(mem, a.mem);
		mem = _mm_min_ps(m0, b.mem);
		return *this;
	}

	float3 min(const float3& rhs) const
	{
		float3 ret;
		ret.mem = _mm_min_ps(rhs.mem, mem);
		return ret;
	}

	float3 max(const float3& rhs) const
	{
		float3 ret;
		ret.mem = _mm_max_ps(rhs.mem, mem);
		return ret;
	}

	// Use >> as "map". It maps a function to each component, and returns the resulting vector
	// f >> v = { f(v.x),  f(v.y), ... }
	friend float3 operator >> (float(f)(float), const float3 &i)
	{
		return { f(i.x), f(i.y), f(i.z) };
	}
};