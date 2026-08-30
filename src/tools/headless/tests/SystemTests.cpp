// java System clock tests.

#include <limits>

#include "java/System.h"
#include "tools/headless/TestFramework.h"

HEADLESS_TEST(java_system, capture_override_is_cleared)
{
	const long_t fixedTime = (std::numeric_limits<long_t>::min)();
	System::setCurrentTimeMillisForCapture(fixedTime);
	ctx.checkEqual(System::currentTimeMillis(), fixedTime, "capture reads the fixed wall-clock time");

	System::clearCurrentTimeMillisForCapture();
	ctx.check(System::currentTimeMillis() != fixedTime, "normal wall-clock reads resume after capture");
}
