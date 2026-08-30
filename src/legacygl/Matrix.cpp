#include "legacygl/Matrix.h"

#include <cmath>

namespace legacygl
{

Mat4 Mat4::identity()
{
	Mat4 r;
	for (int i = 0; i < 16; i++)
		r.m[i] = 0.0f;
	r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
	return r;
}

Mat4 Mat4::operator*(const Mat4 &rhs) const
{
	Mat4 r;
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			r.m[col * 4 + row] =
				m[0 * 4 + row] * rhs.m[col * 4 + 0] +
				m[1 * 4 + row] * rhs.m[col * 4 + 1] +
				m[2 * 4 + row] * rhs.m[col * 4 + 2] +
				m[3 * 4 + row] * rhs.m[col * 4 + 3];
		}
	}
	return r;
}

Mat4 translation(float x, float y, float z)
{
	Mat4 r = Mat4::identity();
	r.m[12] = x;
	r.m[13] = y;
	r.m[14] = z;
	return r;
}

Mat4 scaling(float x, float y, float z)
{
	Mat4 r = Mat4::identity();
	r.m[0] = x;
	r.m[5] = y;
	r.m[10] = z;
	return r;
}

// GL 1.1 section 2.10.2: the rotation matrix is built from the normalized axis.
// A zero-length axis leaves the matrix undefined in the specification; identity
// is used here because that is what the game's zero-angle/zero-axis calls
// (LevelRenderer.cpp:669 rotates 0 degrees) expect to be harmless.
//
// A non-finite angle also leaves the matrix alone. The renderer can produce one:
// GameRenderer's hurt tilt divides by hurtDuration, which is zero until the
// player has been damaged, so a frame rendered at a partial tick of exactly zero
// asks for glRotatef(NaN, 0, 0, 1). The reference driver's matrix is unchanged
// by that call - measured with A126_LEGACYGL_VALIDATE - while propagating the
// NaN would poison the model-view and make frustum culling reject the world.
//
// The trigonometry runs in double and only the finished elements are narrowed.
// OpenGL does not define the precision of glRotatef's sine and cosine, and
// measuring against the native driver showed single-precision intermediates
// drifting several units in the last place further away than double ones.
Mat4 rotation(float angleDegrees, float x, float y, float z)
{
	if (!std::isfinite(angleDegrees) || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(z))
		return Mat4::identity();

	const double length = std::sqrt(static_cast<double>(x) * x + static_cast<double>(y) * y +
		static_cast<double>(z) * z);
	if (length == 0.0)
		return Mat4::identity();

	const double ax = x / length;
	const double ay = y / length;
	const double az = z / length;

	const double radians = static_cast<double>(angleDegrees) * 3.14159265358979323846 / 180.0;
	const double c = std::cos(radians);
	const double s = std::sin(radians);
	const double t = 1.0 - c;

	Mat4 r = Mat4::identity();
	r.m[0] = static_cast<float>(t * ax * ax + c);
	r.m[1] = static_cast<float>(t * ax * ay + s * az);
	r.m[2] = static_cast<float>(t * ax * az - s * ay);

	r.m[4] = static_cast<float>(t * ax * ay - s * az);
	r.m[5] = static_cast<float>(t * ay * ay + c);
	r.m[6] = static_cast<float>(t * ay * az + s * ax);

	r.m[8] = static_cast<float>(t * ax * az + s * ay);
	r.m[9] = static_cast<float>(t * ay * az - s * ax);
	r.m[10] = static_cast<float>(t * az * az + c);
	return r;
}

Mat4 orthographic(double left, double right, double bottom, double top, double zNear, double zFar)
{
	Mat4 r = Mat4::identity();
	r.m[0] = static_cast<float>(2.0 / (right - left));
	r.m[5] = static_cast<float>(2.0 / (top - bottom));
	r.m[10] = static_cast<float>(-2.0 / (zFar - zNear));
	r.m[12] = static_cast<float>(-(right + left) / (right - left));
	r.m[13] = static_cast<float>(-(top + bottom) / (top - bottom));
	r.m[14] = static_cast<float>(-(zFar + zNear) / (zFar - zNear));
	return r;
}

Mat4 frustumMatrix(double left, double right, double bottom, double top, double zNear, double zFar)
{
	Mat4 r;
	for (int i = 0; i < 16; i++)
		r.m[i] = 0.0f;
	r.m[0] = static_cast<float>(2.0 * zNear / (right - left));
	r.m[5] = static_cast<float>(2.0 * zNear / (top - bottom));
	r.m[8] = static_cast<float>((right + left) / (right - left));
	r.m[9] = static_cast<float>((top + bottom) / (top - bottom));
	r.m[10] = static_cast<float>(-(zFar + zNear) / (zFar - zNear));
	r.m[11] = -1.0f;
	r.m[14] = static_cast<float>(-2.0 * zFar * zNear / (zFar - zNear));
	return r;
}

Mat4 normalMatrix(const Mat4 &modelView)
{
	const float a00 = modelView.m[0], a01 = modelView.m[4], a02 = modelView.m[8];
	const float a10 = modelView.m[1], a11 = modelView.m[5], a12 = modelView.m[9];
	const float a20 = modelView.m[2], a21 = modelView.m[6], a22 = modelView.m[10];

	const float c00 = a11 * a22 - a12 * a21;
	const float c01 = a12 * a20 - a10 * a22;
	const float c02 = a10 * a21 - a11 * a20;

	const float determinant = a00 * c00 + a01 * c01 + a02 * c02;
	if (determinant == 0.0f)
		return Mat4::identity();

	const float c10 = a02 * a21 - a01 * a22;
	const float c11 = a00 * a22 - a02 * a20;
	const float c12 = a01 * a20 - a00 * a21;
	const float c20 = a01 * a12 - a02 * a11;
	const float c21 = a02 * a10 - a00 * a12;
	const float c22 = a00 * a11 - a01 * a10;

	const float inverseDeterminant = 1.0f / determinant;

	// inverse(A) = adjugate / det, where adjugate = transpose(cofactor), so
	// transpose(inverse(A))[row][col] = cofactor[row][col] / det. The cofactor
	// named cRC therefore belongs at column C, row R, which is slot C * 4 + R:
	// storing them in cofactor order would write the transpose, and a rotation
	// (whose inverse transpose is itself) would come back reversed.
	Mat4 r = Mat4::identity();
	r.m[0] = c00 * inverseDeterminant;
	r.m[4] = c01 * inverseDeterminant;
	r.m[8] = c02 * inverseDeterminant;
	r.m[1] = c10 * inverseDeterminant;
	r.m[5] = c11 * inverseDeterminant;
	r.m[9] = c12 * inverseDeterminant;
	r.m[2] = c20 * inverseDeterminant;
	r.m[6] = c21 * inverseDeterminant;
	r.m[10] = c22 * inverseDeterminant;
	return r;
}

// GL_EXT_rescale_normal / GL 1.2 section 2.10.3: the factor is
// 1 / sqrt(mi31^2 + mi32^2 + mi33^2), taken from the inverse model-view's third
// row. Under the uniform-scale assumption that is the reciprocal of the scale.
float rescaleNormalFactorFromNormalMatrix(const Mat4 &normal)
{
	// The inverse's third row is the inverse transpose's third column, which in
	// column-major layout is slots 8, 9 and 10.
	const float x = normal.m[8];
	const float y = normal.m[9];
	const float z = normal.m[10];
	const float lengthSquared = x * x + y * y + z * z;
	if (lengthSquared == 0.0f)
		return 1.0f;
	return 1.0f / std::sqrt(lengthSquared);
}

float rescaleNormalFactor(const Mat4 &modelView)
{
	return rescaleNormalFactorFromNormalMatrix(normalMatrix(modelView));
}

bool isUniformScale(const Mat4 &modelView, float tolerance)
{
	const float columnLengths[3] = {
		std::sqrt(modelView.m[0] * modelView.m[0] + modelView.m[1] * modelView.m[1] + modelView.m[2] * modelView.m[2]),
		std::sqrt(modelView.m[4] * modelView.m[4] + modelView.m[5] * modelView.m[5] + modelView.m[6] * modelView.m[6]),
		std::sqrt(modelView.m[8] * modelView.m[8] + modelView.m[9] * modelView.m[9] + modelView.m[10] * modelView.m[10])
	};

	const float largest = std::fmax(columnLengths[0], std::fmax(columnLengths[1], columnLengths[2]));
	const float smallest = std::fmin(columnLengths[0], std::fmin(columnLengths[1], columnLengths[2]));
	if (largest == 0.0f)
		return true;
	return (largest - smallest) <= tolerance * largest;
}

MatrixStack::MatrixStack(std::size_t maxDepth) : currentMatrix(Mat4::identity()), limit(maxDepth)
{
	saved.reserve(maxDepth);
}

bool MatrixStack::push()
{
	if (saved.size() + 1 >= limit)
		return false;
	saved.push_back(currentMatrix);
	return true;
}

bool MatrixStack::pop()
{
	if (saved.empty())
		return false;
	currentMatrix = saved.back();
	saved.pop_back();
	return true;
}

void MatrixStack::reset()
{
	currentMatrix = Mat4::identity();
	saved.clear();
}

}
