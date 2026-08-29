#pragma once

// Bit-exact equivalents of the java.lang.StrictMath functions the game
// depends on.
//
// java.lang.Math may use platform intrinsics, but java.lang.StrictMath is
// specified to produce the same bits everywhere because it is fdlibm. Any
// Alpha code path that reaches StrictMath must therefore be reproduced
// exactly here rather than forwarded to the host libm, whose results can
// differ by one unit in the last place.
namespace StrictMath
{

// Matches StrictMath.log, i.e. fdlibm __ieee754_log.
double log(double x);

}
