#include "util/GLU.h"

#define _USE_MATH_DEFINES
#include <cmath>

// newlib hides M_PI unless __BSD_VISIBLE/__XSI_VISIBLE, which -std=c++17
// switches off. Same guard as every other M_PI user in the tree.
#ifndef M_PI
#define M_PI  3.14159265358979323846
#endif

void gluPerspective(float fovy, float aspect, float zNear, float zFar)
{
    double const height = zNear * tanf(fovy * M_PI / 360.0);
    double const width = height * aspect;
    glFrustum(-width, width, -height, height, zNear, zFar);
}
