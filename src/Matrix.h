#pragma once
#include "Vector.h"

template <typename T>
struct Matrix2 {
	Matrix2()
	{
		c[0][0] = 1; c[0][1] = 0;
		c[1][0] = 0; c[1][1] = 1;
	}
	Matrix2(const T* m)
	{
		c[0][0] = m[0]; c[0][1] = m[1];
		c[1][0] = m[2]; c[1][1] = m[3];
	}
	const T* Pointer() const
	{
		return &c[0][0];
	}

	union {
		float c[2][2];	// Column major order for OpenGL: c[column][row]
		float x[4];
	};
};

template <typename T>
struct Matrix3 {
	Matrix3()
	{
		c[0][0] = 1; c[0][1] = 0; c[0][2] = 0;
		c[1][0] = 0; c[1][1] = 1; c[1][2] = 0;
		c[2][0] = 0; c[2][1] = 0; c[2][2] = 1;
	}
	Matrix3(const T* m)
	{
		c[0][0] = m[0]; c[0][1] = m[1]; c[0][2] = m[2];
		c[1][0] = m[3]; c[1][1] = m[4]; c[1][2] = m[5];
		c[2][0] = m[6]; c[2][1] = m[7]; c[2][2] = m[8];
	}
	Matrix3 Transposed() const
	{
		Matrix3 m;
		m.c[0][0] = c[0][0]; m.c[0][1] = c[1][0]; m.c[0][2] = c[2][0];
		m.c[1][0] = c[0][1]; m.c[1][1] = c[1][1]; m.c[1][2] = c[2][1];
		m.c[2][0] = c[0][2]; m.c[2][1] = c[1][2]; m.c[2][2] = c[2][2];
		return m;
	}
	const T* Pointer() const
	{
		return &c[0][0];
	}

	union {
		float c[3][3];	// Column major order for OpenGL: c[column][row]
		float x[9];
	};
};

template <typename T>
struct Matrix4 {
	Matrix4()
	{
		c[0][0] = 1; c[0][1] = 0; c[0][2] = 0; c[0][3] = 0;
		c[1][0] = 0; c[1][1] = 1; c[1][2] = 0; c[1][3] = 0;
		c[2][0] = 0; c[2][1] = 0; c[2][2] = 1; c[2][3] = 0;
		c[3][0] = 0; c[3][1] = 0; c[3][2] = 0; c[3][3] = 1;
	}
	Matrix4(const Matrix3<T>& m)
	{
		c[0][0] = m.c[0][0]; c[0][1] = m.c[0][1]; c[0][2] = m.c[0][2]; c[0][3] = 0;
		c[1][0] = m.c[1][0]; c[1][1] = m.c[1][1]; c[1][2] = m.c[1][2]; c[1][3] = 0;
		c[2][0] = m.c[2][0]; c[2][1] = m.c[2][1]; c[2][2] = m.c[2][2]; c[2][3] = 0;
		c[3][0] = 0; c[3][1] = 0; c[3][2] = 0; c[3][3] = 1;
	}
	Matrix4(const T* m)
	{
		Assign(m);
	}
	void Assign(const T* m)
	{
		c[0][0] = m[0];  c[0][1] = m[1];  c[0][2] = m[2];  c[0][3] = m[3];
		c[1][0] = m[4];  c[1][1] = m[5];  c[1][2] = m[6];  c[1][3] = m[7];
		c[2][0] = m[8];  c[2][1] = m[9];  c[2][2] = m[10]; c[2][3] = m[11];
		c[3][0] = m[12]; c[3][1] = m[13]; c[3][2] = m[14]; c[3][3] = m[15];
	}
	Matrix4 operator * (const Matrix4& b) const
	{
		Matrix4 m;
		m.c[0][0] = c[0][0] * b.c[0][0] + c[0][1] * b.c[1][0] + c[0][2] * b.c[2][0] + c[0][3] * b.c[3][0];
		m.c[0][1] = c[0][0] * b.c[0][1] + c[0][1] * b.c[1][1] + c[0][2] * b.c[2][1] + c[0][3] * b.c[3][1];
		m.c[0][2] = c[0][0] * b.c[0][2] + c[0][1] * b.c[1][2] + c[0][2] * b.c[2][2] + c[0][3] * b.c[3][2];
		m.c[0][3] = c[0][0] * b.c[0][3] + c[0][1] * b.c[1][3] + c[0][2] * b.c[2][3] + c[0][3] * b.c[3][3];
		m.c[1][0] = c[1][0] * b.c[0][0] + c[1][1] * b.c[1][0] + c[1][2] * b.c[2][0] + c[1][3] * b.c[3][0];
		m.c[1][1] = c[1][0] * b.c[0][1] + c[1][1] * b.c[1][1] + c[1][2] * b.c[2][1] + c[1][3] * b.c[3][1];
		m.c[1][2] = c[1][0] * b.c[0][2] + c[1][1] * b.c[1][2] + c[1][2] * b.c[2][2] + c[1][3] * b.c[3][2];
		m.c[1][3] = c[1][0] * b.c[0][3] + c[1][1] * b.c[1][3] + c[1][2] * b.c[2][3] + c[1][3] * b.c[3][3];
		m.c[2][0] = c[2][0] * b.c[0][0] + c[2][1] * b.c[1][0] + c[2][2] * b.c[2][0] + c[2][3] * b.c[3][0];
		m.c[2][1] = c[2][0] * b.c[0][1] + c[2][1] * b.c[1][1] + c[2][2] * b.c[2][1] + c[2][3] * b.c[3][1];
		m.c[2][2] = c[2][0] * b.c[0][2] + c[2][1] * b.c[1][2] + c[2][2] * b.c[2][2] + c[2][3] * b.c[3][2];
		m.c[2][3] = c[2][0] * b.c[0][3] + c[2][1] * b.c[1][3] + c[2][2] * b.c[2][3] + c[2][3] * b.c[3][3];
		m.c[3][0] = c[3][0] * b.c[0][0] + c[3][1] * b.c[1][0] + c[3][2] * b.c[2][0] + c[3][3] * b.c[3][0];
		m.c[3][1] = c[3][0] * b.c[0][1] + c[3][1] * b.c[1][1] + c[3][2] * b.c[2][1] + c[3][3] * b.c[3][1];
		m.c[3][2] = c[3][0] * b.c[0][2] + c[3][1] * b.c[1][2] + c[3][2] * b.c[2][2] + c[3][3] * b.c[3][2];
		m.c[3][3] = c[3][0] * b.c[0][3] + c[3][1] * b.c[1][3] + c[3][2] * b.c[2][3] + c[3][3] * b.c[3][3];
		return m;
	}
	Matrix4& operator *= (const Matrix4& b)
	{
		Matrix4 m = *this * b;
		return (*this = m);
	}
	Vector4<T> operator * (const Vector4<T>& b) const
	{
		Vector4<T> v;
		v.x = b.x * c[0][0] + b.y * c[1][0] + b.z * c[2][0] + b.w * c[3][0];
		v.y = b.x * c[0][1] + b.y * c[1][1] + b.z * c[2][1] + b.w * c[3][1];
		v.z = b.x * c[0][2] + b.y * c[1][2] + b.z * c[2][2] + b.w * c[3][2];
		v.w = b.x * c[0][3] + b.y * c[1][3] + b.z * c[2][3] + b.w * c[3][3];
		return v;
	}
	Matrix4 Transposed() const
	{
		Matrix4 m;
		m.c[0][0] = c[0][0]; m.c[0][1] = c[1][0]; m.c[0][2] = c[2][0]; m.c[0][3] = c[3][0];
		m.c[1][0] = c[0][1]; m.c[1][1] = c[1][1]; m.c[1][2] = c[2][1]; m.c[1][3] = c[3][1];
		m.c[2][0] = c[0][2]; m.c[2][1] = c[1][2]; m.c[2][2] = c[2][2]; m.c[2][3] = c[3][2];
		m.c[3][0] = c[0][3]; m.c[3][1] = c[1][3]; m.c[3][2] = c[2][3]; m.c[3][3] = c[3][3];
		return m;
	}
	Matrix3<T> ToMat3() const
	{
		Matrix3<T> m;
		m.c[0][0] = c[0][0]; m.c[1][0] = c[1][0]; m.c[2][0] = c[2][0];
		m.c[0][1] = c[0][1]; m.c[1][1] = c[1][1]; m.c[2][1] = c[2][1];
		m.c[0][2] = c[0][2]; m.c[1][2] = c[1][2]; m.c[2][2] = c[2][2];
		return m;
	}
	const T* Pointer() const
	{
		return &c[0][0];
	}
	static Matrix4<T> Identity()
	{
		return Matrix4();
	}
	static Matrix4<T> Translate(T x, T y, T z)
	{
		Matrix4 m;
		m.c[0][0] = 1; m.c[0][1] = 0; m.c[0][2] = 0; m.c[0][3] = 0;
		m.c[1][0] = 0; m.c[1][1] = 1; m.c[1][2] = 0; m.c[1][3] = 0;
		m.c[2][0] = 0; m.c[2][1] = 0; m.c[2][2] = 1; m.c[2][3] = 0;
		m.c[3][0] = x; m.c[3][1] = y; m.c[3][2] = z; m.c[3][3] = 1;
		return m;
	}
	static Matrix4<T> Scale(T s)
	{
		Matrix4 m;
		m.c[0][0] = s; m.c[0][1] = 0; m.c[0][2] = 0; m.c[0][3] = 0;
		m.c[1][0] = 0; m.c[1][1] = s; m.c[1][2] = 0; m.c[1][3] = 0;
		m.c[2][0] = 0; m.c[2][1] = 0; m.c[2][2] = s; m.c[2][3] = 0;
		m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0; m.c[3][3] = 1;
		return m;
	}
	static Matrix4<T> RotateX(T degrees)
	{
		T radians = degrees * 3.14159f / 180.0f;
		T s = sin(radians);
		T c = cos(radians);

		Matrix4 m;
		m.c[0][0] = 1; m.c[0][1] = 0; m.c[0][2] = 0; m.c[0][3] = 0;
		m.c[1][0] = 0; m.c[1][1] = c; m.c[1][2] = s; m.c[1][3] = 0;
		m.c[2][0] = 0; m.c[2][1] =-s; m.c[2][2] = c; m.c[2][3] = 0;
		m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0; m.c[3][3] = 1;
		return m;
	}
	static Matrix4<T> RotateY(T degrees)
	{
		T radians = degrees * 3.14159f / 180.0f;
		T s = sin(radians);
		T c = cos(radians);

		Matrix4 m;
		m.c[0][0] = c; m.c[0][1] = 0; m.c[0][2] =-s; m.c[0][3] = 0;
		m.c[1][0] = 0; m.c[1][1] = 1; m.c[1][2] = 0; m.c[1][3] = 0;
		m.c[2][0] = s; m.c[2][1] = 0; m.c[2][2] = c; m.c[2][3] = 0;
		m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0; m.c[3][3] = 1;
		return m;
	}
	static Matrix4<T> RotateZ(T degrees)
	{
		T radians = degrees * 3.14159f / 180.0f;
		T s = sin(radians);
		T c = cos(radians);

		Matrix4 m;
		m.c[0][0] = c; m.c[0][1] = s; m.c[0][2] = 0; m.c[0][3] = 0;
		m.c[1][0] =-s; m.c[1][1] = c; m.c[1][2] = 0; m.c[1][3] = 0;
		m.c[2][0] = 0; m.c[2][1] = 0; m.c[2][2] = 1; m.c[2][3] = 0;
		m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = 0; m.c[3][3] = 1;
		return m;
	}
	static Matrix4<T> Frustum(T left, T right, T bottom, T top, T near, T far)
	{
		T a = 2 * near / (right - left);
		T b = 2 * near / (top - bottom);
		T c = (right + left) / (right - left);
		T d = (top + bottom) / (top - bottom);
		T e = - (far + near) / (far - near);
		T f = -2 * far * near / (far - near);
		Matrix4 m;
		m.c[0][0] = a; m.c[0][1] = 0; m.c[0][2] = 0; m.c[0][3] = 0;
		m.c[1][0] = 0; m.c[1][1] = b; m.c[1][2] = 0; m.c[1][3] = 0;
		m.c[2][0] = c; m.c[2][1] = d; m.c[2][2] = e; m.c[2][3] = -1;
		m.c[3][0] = 0; m.c[3][1] = 0; m.c[3][2] = f; m.c[3][3] = 1;
		return m;
	}

	union {
		float c[4][4];	// Column major order for OpenGL: c[column][row]
		float x[16];
	};
};

typedef Matrix2<float> mat2;
typedef Matrix3<float> mat3;
typedef Matrix4<float> mat4;
