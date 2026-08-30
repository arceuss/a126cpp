#pragma once

#include <cstddef>
#include <vector>

// OpenGL matrix conventions.
//
// Matrices are 4x4, column-major, indexed m[column * 4 + row] - the same layout
// glGetFloatv(GL_MODELVIEW_MATRIX, ...) fills, which Frustum.cpp reads directly.
// Every transform call postmultiplies: M' = M * T. Nothing in this file ever
// converts to a backend clip-space convention; that correction belongs to the
// backend at draw time.

namespace legacygl
{

struct Mat4
{
	float m[16];

	static Mat4 identity();

	// this * rhs, OpenGL operand order.
	Mat4 operator*(const Mat4 &rhs) const;
};

Mat4 translation(float x, float y, float z);
Mat4 rotation(float angleDegrees, float x, float y, float z);
Mat4 scaling(float x, float y, float z);
Mat4 orthographic(double left, double right, double bottom, double top, double zNear, double zFar);
Mat4 frustumMatrix(double left, double right, double bottom, double top, double zNear, double zFar);

// Inverse transpose of the upper-left 3x3, written into a 4x4 whose fourth row
// and column are those of the identity. Normals require this, not the model-view
// matrix itself: a non-uniform scale breaks the naive transform. A singular 3x3
// yields the identity so a degenerate model-view cannot produce NaN normals.
Mat4 normalMatrix(const Mat4 &modelView);

// GL_RESCALE_NORMAL's scalar. The inverse-transpose model-view rows are assumed
// uniformly scaled; the factor restores unit length for that case only. Callers
// that need to know whether the assumption holds ask isUniformScale().
float rescaleNormalFactorFromNormalMatrix(const Mat4 &normal);
float rescaleNormalFactor(const Mat4 &modelView);
bool isUniformScale(const Mat4 &modelView, float tolerance);

class MatrixStack
{
public:
	explicit MatrixStack(std::size_t maxDepth);

	const Mat4 &top() const { return currentMatrix; }
	Mat4 &top() { return currentMatrix; }

	std::size_t depth() const { return saved.size() + 1; }
	std::size_t capacity() const { return limit; }

	void loadIdentity() { currentMatrix = Mat4::identity(); }
	void multiply(const Mat4 &rhs) { currentMatrix = currentMatrix * rhs; }

	// Both return false when the operation would over/underflow, in which case
	// the stack is left untouched and the caller raises the GL error.
	bool push();
	bool pop();

	void reset();

private:
	Mat4 currentMatrix;
	std::vector<Mat4> saved;
	std::size_t limit;
};

}
