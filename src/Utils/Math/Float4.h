#pragma once
#include <immintrin.h>

// IMPORTANT! Alignment will force the use of padding when using this inside a struct
struct alignas(16) float4
{
	union
	{
		__m128 mem;
		struct
		{
			float x, y, z, w;
		};
	};

	float4() { mem = _mm_xor_ps(mem, mem); }
	float4(float v) { mem = _mm_set_ps1(v); }
	float4(float _x, float _y, float _z, float _w)
	{
		x = _x; y = _y; z = _z; w = _w;
	}

	inline bool operator==(const float4& rhs) { return (x == rhs.x) && (y == rhs.y) && (z == rhs.z) && (w == rhs.w); }
	inline bool operator!=(const float4& rhs) { return !(*this == rhs); }

	float4& operator+=(const float4& rhs)
	{
		mem = _mm_add_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	// friends defined inside class body are inline and are hidden from non-ADL lookup
	friend float4 operator+(float4 lhs, const float4& rhs)
	{
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}

	float4& operator-=(const float4& rhs)
	{
		mem = _mm_sub_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float4 operator-(float4 lhs, const float4& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	float4& operator*=(const float4& rhs)
	{
		mem = _mm_mul_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float4 operator*(float4 lhs, const float4& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	float4& operator/=(const float4& rhs)
	{
		mem = _mm_div_ps(mem, rhs.mem);
		return *this; // return the result by reference
	}

	friend float4 operator/(float4 lhs, const float4& rhs)
	{
		lhs /= rhs;
		return lhs;
	}

	// w component returns 0
	float4 cross(const float4& rhs)
	{
		__m128 a = _mm_shuffle_ps(mem, mem, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 b = _mm_shuffle_ps(mem, mem, _MM_SHUFFLE(3, 1, 0, 2));

		__m128 m0 = _mm_mul_ps(a, rhs.mem);
		__m128 m1 = _mm_mul_ps(b, rhs.mem);

		m0 = _mm_shuffle_ps(m0, m0, _MM_SHUFFLE(3, 0, 2, 1));
		m1 = _mm_shuffle_ps(m1, m1, _MM_SHUFFLE(3, 1, 0, 2));

		// Swap here to correct sign. Why is this needed?
		__m128 p1 = _mm_sub_ps(m1, m0);

		float4 r;
		r.mem = p1;
		return r;
	}

	float length_sqr()
	{
		__m128 m = _mm_mul_ps(mem, mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		float r;
		_mm_store_ss(&r, p1);
		return r;
	}
	float length()
	{
		__m128 m = _mm_mul_ps(mem, mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		__m128 p2 = _mm_sqrt_ps(p1);
		float r;
		_mm_store_ss(&r, p2);
		return r;
	}
	float4 normalize()
	{
		__m128 m = _mm_mul_ps(mem, mem);

		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		//__m128 p2 = _mm_rsqrt_ps(p1);
		//mem = _mm_mul_ps(mem, p2);
		__m128 p2 = _mm_sqrt_ps(p1);
		mem = _mm_div_ps(mem, p2);

		return *this;
	}
	float dot(const float4& rhs)
	{
		__m128 m = _mm_mul_ps(mem, rhs.mem);
		__m128 p0 = _mm_hadd_ps(m, m);
		__m128 p1 = _mm_hadd_ps(p0, p0);
		float r;
		_mm_store_ss(&r, p1);
		return r;
	}

	float4 clamp(const float4& a, const float4& b)
	{
		// min(max(x,a),b)
		auto m0 = _mm_max_ps(mem, a.mem);
		mem = _mm_min_ps(m0, b.mem);
		return *this;
	}

	float4 min(const float4& rhs) const
	{
		float4 ret;
		ret.mem = _mm_min_ps(rhs.mem, mem);
		return ret;
	}

	float4 max(const float4& rhs) const
	{
		float4 ret;
		ret.mem = _mm_max_ps(rhs.mem, mem);
		return ret;
	}

	// Use >> as "map". It maps a function to each component, and returns the resulting vector
	// f >> v = { f(v.x),  f(v.y), ... }
	friend float4 operator >> (float(f)(float), const float4 &i)
	{
		return { f(i.x), f(i.y), f(i.z), f(i.w) };
	}
};