#pragma once
#include <immintrin.h>
#include "Float4.h"

// IMPORTANT! Alignment will force the use of padding when using this inside a struct
struct alignas(16) float2
{
private:
	static const inline __m128 ElementMask = _mm_castsi128_ps(_mm_set_epi32(0xffffffff, 0xffffffff, 0, 0));
public:
	union
	{
		__m128 mem;
		struct
		{
			float x, y;
			float z_, w_;
		};
	};

	float2() { _mm_xor_ps(mem, mem); }
	float2(float v) { _mm_xor_ps(mem, mem); x = y = v; }
	float2(float _x, float _y)
	{
		_mm_xor_ps(mem, mem);
		x = _x; y = _y;
	}
	float2(float4 v) { _mm_xor_ps(mem, mem); x = v.x; y = v.y; }

	inline bool operator==(const float2& rhs) { return (x == rhs.x) && (y == rhs.y); }
	inline bool operator!=(const float2& rhs) { return !(*this == rhs); }

	float2& operator+=(const float2& rhs)
	{
		mem = _mm_add_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	// friends defined inside class body are inline and are hidden from non-ADL lookup
	friend float2 operator+(float2 lhs, const float2& rhs)
	{
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}

	float2& operator-=(const float2& rhs)
	{
		mem = _mm_sub_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float2 operator-(float2 lhs, const float2& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	float2& operator*=(const float2& rhs)
	{
		mem = _mm_mul_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float2 operator*(float2 lhs, const float2& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	float2& operator/=(const float2& rhs)
	{
		mem = _mm_div_ps(mem, rhs.mem);
		mem = _mm_and_ps(mem, ElementMask);

		return *this; // return the result by reference
	}

	friend float2 operator/(float2 lhs, const float2& rhs)
	{
		lhs /= rhs;
		return lhs;
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
	float2 normalize()
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
	float dot(const float2& rhs) const
	{
		__m128 m = _mm_mul_ps(mem, rhs.mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		float r;
		_mm_store_ss(&r, p1);
		return r;
	}

	float2 clamp(const float2& a, const float2& b)
	{
		// min(max(x,a),b)
		auto m0 = _mm_max_ps(mem, a.mem);
		mem = _mm_min_ps(m0, b.mem);
		return *this;
	}

	float2 min(const float2& rhs) const
	{
		float2 ret;
		ret.mem = _mm_min_ps(rhs.mem, mem);
		return ret;
	}

	float2 max(const float2& rhs) const
	{
		float2 ret;
		ret.mem = _mm_max_ps(rhs.mem, mem);
		return ret;
	}

	// Use >> as "map". It maps a function to each component, and returns the resulting vector
	// f >> v = { f(v.x),  f(v.y), ... }
	friend float2 operator >> (float(f)(float), const float2 &i)
	{
		return { f(i.x), f(i.y) };
	}

};