#pragma once

struct int2
{
	int x, y;

	int2() { x = y = 0; }
	int2(int v) { x = y = v; }
	int2(int _x, int _y)
	{
		x = _x; y = _y;
	}

	inline bool operator==(const int2& rhs) { return (x == rhs.x) && (y == rhs.y); }
	inline bool operator!=(const int2& rhs) { return !(*this == rhs); }

	int2& operator+=(const int2& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return *this; // return the result by reference
	}

	// friends defined inside class body are inline and are hidden from non-ADL lookup
	friend int2 operator+(int2 lhs, const int2& rhs)         // passing lhs by value helps optimize chained a+b+c, otherwise, both parameters may be const references
	{
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}

	int2& operator-=(const int2& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return *this; // return the result by reference
	}

	friend int2 operator-(int2 lhs, const int2& rhs)
	{
		lhs -= rhs;
		return lhs;
	}

	int2& operator*=(const int2& rhs)
	{
		x *= rhs.x;
		y *= rhs.y;
		return *this;
	}

	friend int2 operator*(int2 lhs, const int2& rhs)
	{
		lhs *= rhs;
		return lhs;
	}

	int2& operator/=(const int2& rhs)
	{
		x /= rhs.x;
		y /= rhs.y;
		return *this;
	}

	friend int2 operator/(int2 lhs, const int2& rhs)
	{
		lhs /= rhs;
		return lhs;
	}

	// Use >> as "map". It maps a function to each component, and returns the resulting vector
	// f >> v = { f(v.x),  f(v.y), ... }
	friend int2 operator >> (int(f)(int), const int2 &i)
	{
		return{ f(i.x), f(i.y) };
	}
};