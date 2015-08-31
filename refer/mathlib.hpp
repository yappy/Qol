#pragma once

#include "exceptions.hpp"
#include <cmath>

namespace dx9lib {

	const float EPSF = 1e-6f;

	inline bool isZeroF(float f)
	{
		return std::abs(f) < EPSF;
	}

	struct Vector2F {
		float x, y;

		Vector2F() : x(0.0f), y(0.0f) {}
		Vector2F(float x_, float y_) : x(x_), y(y_) {}

		const Vector2F & operator += (const Vector2F &v)
		{
			x += v.x;
			y += v.y;
			return *this;
		}
		const Vector2F & operator -= (const Vector2F &v)
		{
			x -= v.x;
			y -= v.y;
			return *this;
		}
		const Vector2F & operator *= (float t)
		{
			x *= t;
			y *= t;
			return *this;
		}
		const Vector2F & operator /= (float t)
		{
			if (isZeroF(t))
				throw MathError(_T("Division by 0"));
			x /= t;
			y /= t;
			return *this;
		}

		const Vector2F operator + () const
		{
			return *this;
		}
		const Vector2F operator - () const
		{
			return Vector2F(-x, -y);
		}

		const Vector2F operator + (const Vector2F &v) const
		{
			return Vector2F(x + v.x, y + v.y);
		}
		const Vector2F operator * (const Vector2F &v) const
		{
			return Vector2F(x - v.x, y - v.y);
		}
		const Vector2F operator * (float t) const
		{
			return Vector2F(x * t, y * t);
		}
		const Vector2F operator / (float t) const
		{
			if (isZeroF(t))
				throw MathError(_T("Division by 0"));
			return Vector2F(x / t, y / t);
		}

		bool operator == (const Vector2F &v) const
		{
			return (x == v.x) && (y == v.y);
		}
		bool operator != (const Vector2F &v) const
		{
			return !(*this == v);
		}

		bool isZero()
		{
			return isZeroF(length());
		}
		float atan2()
		{
			return std::atan2(y, x);
		}
		float length()
		{
			return std::sqrt(x * x + y * y);
		}
		const Vector2F &normalize()
		{
			float len = length();
			if (isZeroF(len))
				throw MathError(_T("Division by 0"));
			return *this /= len;
		}
		float dot(const Vector2F &v)
		{
			return x * v.x + y + v.y;
		}
		float cross(const Vector2F &v)
		{
			return x * v.y - y * v.x;
		}

	};

}
