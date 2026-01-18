#pragma once

#include "java/Type.h"

class Options;

class ScreenSizeCalculator
{
private:
	int_t w = 0;
	int_t h = 0;
public:
	int_t scale = 0;

public:
	ScreenSizeCalculator(int_t width, int_t height, const Options &options);
	int getWidth() const;
	int getHeight() const;
};
