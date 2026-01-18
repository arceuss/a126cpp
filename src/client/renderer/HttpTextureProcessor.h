#pragma once

#include "java/BufferedImage.h"

// newb12: HttpTextureProcessor interface - processes downloaded images (HttpTextureProcessor.java)
class HttpTextureProcessor
{
public:
	virtual ~HttpTextureProcessor() {}
	virtual BufferedImage process(BufferedImage &in) = 0;
};
