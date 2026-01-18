#include "client/gui/ScreenSizeCalculator.h"
#include "client/Options.h"

ScreenSizeCalculator::ScreenSizeCalculator(int_t width, int_t height, const Options &options)
{
	w = width;
	h = height;
	scale = 1;

	int_t guiScale = options.guiScale;
	if (guiScale == 0)
	{
		// Auto: find maximum scale that keeps dimensions >= 320x240
		guiScale = 1000;
	}
	
	// Find scale: while scaleFactor < guiScale AND dimensions stay >= 320x240
	while (scale < guiScale && w / (scale + 1) >= 320 && h / (scale + 1) >= 240)
		scale++;

	w /= scale;
	h /= scale;
}

int ScreenSizeCalculator::getWidth() const
{
	return w;
}

int ScreenSizeCalculator::getHeight() const
{
	return h;
}
