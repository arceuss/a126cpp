#include "java/StrictMath.h"

#include <cstdint>
#include <cstring>

// log() below is a transcription of fdlibm 5.3 e_log.c (__ieee754_log), which
// is what java.lang.StrictMath.log evaluates. The original carries this
// notice, preserved as its licence requires:
//
//   ====================================================
//   Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
//
//   Developed at SunSoft, a Sun Microsystems, Inc. business.
//   Permission to use, copy, modify, and distribute this
//   software is freely granted, provided that this notice
//   is preserved.
//   ====================================================
//
// The word accessors of the original (__HI/__LO) are expressed with memcpy so
// that the code does not depend on type punning through a union.

namespace StrictMath
{

static const double ln2_hi = 6.93147180369123816490e-01; /* 3fe62e42 fee00000 */
static const double ln2_lo = 1.90821492927058770002e-10; /* 3dea39ef 35793c76 */
static const double two54  = 1.80143985094819840000e+16; /* 43500000 00000000 */
static const double Lg1 = 6.666666666666735130e-01; /* 3FE55555 55555593 */
static const double Lg2 = 3.999999999940941908e-01; /* 3FD99999 9997FA04 */
static const double Lg3 = 2.857142874366239149e-01; /* 3FD24924 94229359 */
static const double Lg4 = 2.222219843214978396e-01; /* 3FCC71C5 1D8E78AF */
static const double Lg5 = 1.818357216161805012e-01; /* 3FC74664 96CB03DE */
static const double Lg6 = 1.531383769920937332e-01; /* 3FC39A09 D078C69F */
static const double Lg7 = 1.479819860511658591e-01; /* 3FC2F112 DF3E5244 */

// Deliberately not const: the original relies on a runtime division by zero to
// produce the infinities and NaNs the specification requires.
static double zero = 0.0;

static uint64_t bitsOf(double value)
{
	uint64_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	return bits;
}

static double doubleOf(uint64_t bits)
{
	double value = 0.0;
	std::memcpy(&value, &bits, sizeof(value));
	return value;
}

static int32_t highWord(double value)
{
	return static_cast<int32_t>(static_cast<uint32_t>(bitsOf(value) >> 32));
}

static uint32_t lowWord(double value)
{
	return static_cast<uint32_t>(bitsOf(value) & 0xFFFFFFFFULL);
}

static double withHighWord(double value, int32_t high)
{
	const uint64_t bits = (bitsOf(value) & 0xFFFFFFFFULL)
		| (static_cast<uint64_t>(static_cast<uint32_t>(high)) << 32);
	return doubleOf(bits);
}

double log(double x)
{
	double hfsq, f, s, z, R, w, t1, t2, dk;
	int32_t k, hx, i, j;
	uint32_t lx;

	hx = highWord(x);
	lx = lowWord(x);

	k = 0;
	if (hx < 0x00100000) /* x < 2**-1022 */
	{
		if (((hx & 0x7fffffff) | static_cast<int32_t>(lx)) == 0)
			return -two54 / zero; /* log(+-0)=-inf */
		if (hx < 0)
			return (x - x) / zero; /* log(-#) = NaN */
		k -= 54;
		x *= two54; /* subnormal number, scale up x */
		hx = highWord(x);
	}
	if (hx >= 0x7ff00000)
		return x + x;
	k += (hx >> 20) - 1023;
	hx &= 0x000fffff;
	i = (hx + 0x95f64) & 0x100000;
	x = withHighWord(x, hx | (i ^ 0x3ff00000)); /* normalize x or x/2 */
	k += (i >> 20);
	f = x - 1.0;
	if ((0x000fffff & (2 + hx)) < 3) /* |f| < 2**-20 */
	{
		if (f == zero)
		{
			if (k == 0)
				return zero;
			dk = static_cast<double>(k);
			return dk * ln2_hi + dk * ln2_lo;
		}
		R = f * f * (0.5 - 0.33333333333333333 * f);
		if (k == 0)
			return f - R;
		dk = static_cast<double>(k);
		return dk * ln2_hi - ((R - dk * ln2_lo) - f);
	}
	s = f / (2.0 + f);
	dk = static_cast<double>(k);
	z = s * s;
	i = hx - 0x6147a;
	w = z * z;
	j = 0x6b851 - hx;
	t1 = w * (Lg2 + w * (Lg4 + w * Lg6));
	t2 = z * (Lg1 + w * (Lg3 + w * (Lg5 + w * Lg7)));
	i |= j;
	R = t2 + t1;
	if (i > 0)
	{
		hfsq = 0.5 * f * f;
		if (k == 0)
			return f - (hfsq - s * (hfsq + R));
		return dk * ln2_hi - ((hfsq - (s * (hfsq + R) + dk * ln2_lo)) - f);
	}

	if (k == 0)
		return f - s * (f - R);
	return dk * ln2_hi - ((s * (f - R) - dk * ln2_lo) - f);
}

}
