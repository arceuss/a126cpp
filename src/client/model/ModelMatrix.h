#pragma once

#include <cmath>

// A column-major 4x4 float matrix with OpenGL's fixed-function composition
// rules: every operation post-multiplies, exactly as glTranslatef, glRotatef
// and glScalef do to the current matrix. Used to bake a model's transform
// chain into vertex data on the CPU so many models can share one draw.
struct ModelMatrix
{
	float m[16];

	ModelMatrix()
	{
		loadIdentity();
	}

	void loadIdentity()
	{
		for (int i = 0; i < 16; i++)
			m[i] = (i % 5 == 0) ? 1.0f : 0.0f;
	}

	// this = this * other
	void multiply(const float *other)
	{
		float result[16];
		for (int column = 0; column < 4; column++)
		{
			for (int row = 0; row < 4; row++)
			{
				result[column * 4 + row] =
					m[0 * 4 + row] * other[column * 4 + 0] +
					m[1 * 4 + row] * other[column * 4 + 1] +
					m[2 * 4 + row] * other[column * 4 + 2] +
					m[3 * 4 + row] * other[column * 4 + 3];
			}
		}
		for (int i = 0; i < 16; i++)
			m[i] = result[i];
	}

	void translate(float x, float y, float z)
	{
		float t[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, x, y, z, 1 };
		multiply(t);
	}

	void scale(float x, float y, float z)
	{
		float s[16] = { x, 0, 0, 0, 0, y, 0, 0, 0, 0, z, 0, 0, 0, 0, 1 };
		multiply(s);
	}

	// glRotatef: angle in degrees about (x, y, z), OpenGL 1.1 section 2.10.2.
	void rotate(float angle, float x, float y, float z)
	{
		const float length = std::sqrt(x * x + y * y + z * z);
		if (length == 0.0f)
			return;
		x /= length; y /= length; z /= length;
		const float radians = angle * 0.017453292519943295f;
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		const float t = 1.0f - c;
		float r[16] = {
			x * x * t + c,     y * x * t + z * s, x * z * t - y * s, 0,
			x * y * t - z * s, y * y * t + c,     y * z * t + x * s, 0,
			x * z * t + y * s, y * z * t - x * s, z * z * t + c,     0,
			0,                 0,                 0,                 1
		};
		multiply(r);
	}

	void transformPoint(float x, float y, float z, float &ox, float &oy, float &oz) const
	{
		ox = m[0] * x + m[4] * y + m[8] * z + m[12];
		oy = m[1] * x + m[5] * y + m[9] * z + m[13];
		oz = m[2] * x + m[6] * y + m[10] * z + m[14];
	}

	// Normals transform by the inverse transpose of the linear part, which is
	// what fixed-function OpenGL does with the modelview (1.1 section 2.10.3).
	// The callers compose rotations and diagonal scales, so the 3x3 is always
	// invertible; a singular one leaves the normal untouched.
	void transformNormal(float x, float y, float z, float &ox, float &oy, float &oz) const
	{
		const float a = m[0], b = m[4], c = m[8];
		const float d = m[1], e = m[5], f = m[9];
		const float g = m[2], h = m[6], i = m[10];
		const float determinant = a * (e * i - f * h) - b * (d * i - f * g) + c * (d * h - e * g);
		if (determinant == 0.0f)
		{
			ox = x; oy = y; oz = z;
			return;
		}
		const float inv = 1.0f / determinant;
		// Inverse (cofactor transpose), then transpose again for the normal:
		// the inverse transpose is the cofactor matrix over the determinant.
		const float c00 = (e * i - f * h) * inv, c01 = -(d * i - f * g) * inv, c02 = (d * h - e * g) * inv;
		const float c10 = -(b * i - c * h) * inv, c11 = (a * i - c * g) * inv, c12 = -(a * h - b * g) * inv;
		const float c20 = (b * f - c * e) * inv, c21 = -(a * f - c * d) * inv, c22 = (a * e - b * d) * inv;
		ox = c00 * x + c01 * y + c02 * z;
		oy = c10 * x + c11 * y + c12 * z;
		oz = c20 * x + c21 * y + c22 * z;
	}
};
