#pragma once

#include <memory>
#include <atomic>

#include "java/BufferedImage.h"
#include "java/String.h"
#include "client/renderer/HttpTextureProcessor.h"

// newb12: HttpTexture class - manages HTTP texture downloads (HttpTexture.java)
class HttpTexture
{
public:
	int_t count = 1;
	int_t id = -1;
	bool isLoaded = false;

	HttpTexture(const jstring &url, std::unique_ptr<HttpTextureProcessor> processor);
	// Owner thread only. The worker never touches the image after publication.
	BufferedImage *image();
	
private:
	struct Download
	{
		BufferedImage image;
		std::unique_ptr<HttpTextureProcessor> processor;
		std::atomic<bool> ready{false};
	};
	std::shared_ptr<Download> download;
	static void downloadThread(const jstring &url, Download &result);
};
