// Common Math symbols declarations.

#include <stdint.h>
#include <math.h>

// Core template for Vectors of 2 scalar elements.
// Defines operator overloads.
template<typename ScalarType>
struct Vector2
{
	ScalarType x, y;

	Vector2() = default;
	Vector2(ScalarType inX, ScalarType inY): x(inX), y(inY) {}

	// Conversion constructors and operators.

	template<typename ScalarTypeFROM>
	Vector2(ScalarTypeFROM fromX, ScalarTypeFROM fromY) : x((ScalarType)(fromX)), y((ScalarType)(fromY)) {}

	template<typename ScalarTypeFROM>
	Vector2(const Vector2<ScalarTypeFROM>& from) : x((ScalarType)(from.x)), y((ScalarType)(from.y)) {}

	template<typename ScalarTypeFROM>
	inline void operator =(const Vector2<ScalarTypeFROM>& from)
	{
		x = (ScalarType)(from.x);
		y = (ScalarType)(from.y);
	}
};

// Construction

template<typename ScalarType>
constexpr Vector2<ScalarType> MakeVec2(ScalarType X, ScalarType Y)
{
	return { X, Y };
}

// Operator overloads

template<typename ScalarType, typename ScalarType2>
constexpr Vector2<ScalarType> operator+(const Vector2<ScalarType>& A, const Vector2<ScalarType2>& B)
{
	return { A.x + B.x, A.y + B.y };
}

template<typename ScalarType, typename ScalarType2>
constexpr Vector2<ScalarType> operator-(const Vector2<ScalarType>& A, const Vector2<ScalarType2>& B)
{
	return { A.x - B.x, A.y - B.y };
}

template<typename ScalarType, typename ScalarType2>
constexpr Vector2<ScalarType> operator*(const Vector2<ScalarType>& A, const ScalarType2& B)
{
	return { A.x * B, A.y * B };
}

template<typename ScalarType, typename ScalarType2>
constexpr Vector2<ScalarType> operator/(const Vector2<ScalarType>& A, const ScalarType2& B)
{
	return { A.x / B, A.y / B };
}

// Utility Functions
// Note: These are not really written for performance, just convenience. Any heavy operation with many vectors getting manipulated should
// feature their own performant solution such as using Simd or whatever else is possible in their own context.

template<typename ScalarType>
constexpr float VecLength(const Vector2<ScalarType>& Vec)
{
	return sqrtf(Vec.x * Vec.x + Vec.y * Vec.y);
}

template<typename ScalarType>
constexpr Vector2<ScalarType> VecNormalized(const Vector2<ScalarType>& Vec)
{
	return Vec / VecLength(Vec);
}

// Floating types
typedef Vector2<float> Vector2f;
typedef Vector2<double> Vector2d;

// Integer types
typedef Vector2<int16_t> Vector2s;
typedef Vector2<int32_t> Vector2i;
typedef Vector2<int64_t> Vector2l;