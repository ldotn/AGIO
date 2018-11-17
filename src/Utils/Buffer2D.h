#pragma once
#include <algorithm>

namespace agio
{
	// Row major container for dynamic 2D arrays
	template<typename T>
	struct Buffer2D
	{
		struct int2
		{
			int x, y;
		};

		int2 offset;
		int2 res;
		T* data;

		Buffer2D() { data = nullptr; }
		Buffer2D(int2 _res)
		{
			data = nullptr;
			resize(_res);
			offset.x = 0;
			offset.y = 0;
		}

		~Buffer2D()
		{
			if (data) delete[] data;
		}

		Buffer2D(const Buffer2D& rhs)
		{
			data = nullptr;
			resize(rhs.res);
			for (int i = 0; i < res.x*res.y; i++)
				data[i] = rhs.data[i];
			offset = rhs.offset;
		}

		Buffer2D(int2 _res, T val, int) // dummy parameter at the end to resolve ambiguities
		{
			data = nullptr;
			resize(_res);

			offset.x = 0;
			offset.y = 0;

			foreach([&val](int, int2, T& v) { v = val; });
		}

		Buffer2D(Buffer2D&& rhs) : Buffer2D() // initialize via default constructor, C++11 only
		{
			swap(*this, rhs);
		}

		Buffer2D& operator=(Buffer2D rhs)
		{
			swap(*this, rhs);
			return *this;
		}

		friend void swap(Buffer2D& lhs, Buffer2D& rhs)
		{
			using std::swap; // allow use of std::swap...
			swap(lhs.res, rhs.res);
			swap(lhs.data, rhs.data);
			swap(lhs.offset, rhs.offset);
		}

		void resize(int2 newRes)
		{
			if (data) delete[] data;

			res = newRes;
			data = new T[res.x*res.y];
		}

		T& operator[](int2 p)
		{
			p.x = clamp<int>(p.x - offset.x, 0, res.x - 1);
			p.y = clamp<int>(p.y - offset.y, 0, res.y - 1);

			return data[p.x + res.x*p.y];
		}
		const T& operator[](int2 p) const
		{
			p.x = clamp<int>(p.x - offset.x, 0, res.x - 1);
			p.y = clamp<int>(p.y - offset.y, 0, res.y - 1);

			return data[p.x + res.x*p.y];
		}

		void foreach(function<void(int, int2, T&)> f)
		{
			int idx = 0;

			for (int y = 0; y < res.y; y++)
			{
				for (int x = 0; x < res.x; x++)
				{
					f(idx, { x,y }, data[idx]);
					idx++;
				}
			}
		}

		void foreach(function<void(int, int2, T)> f) const
		{
			int idx = 0;

			for (int y = 0; y < res.y; y++)
			{
				for (int x = 0; x < res.x; x++)
				{
					f(idx, { x,y }, data[idx]);
					idx++;
				}
			}
		}
	};
}