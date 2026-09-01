#include "client/renderer/HttpTexture.h"

#include "java/BufferedImage.h"
#include "util/HttpGet.h"
#include "util/BackgroundTask.h"
#include "java/String.h"
#include "java/File.h"
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <iostream>
#include <memory>
#include <fstream>

// newb12: HttpTexture constructor - downloads image in background thread (HttpTexture.java:14-43)
HttpTexture::HttpTexture(const jstring &url, HttpTextureProcessor *processor)
{
	// Start download in background thread
	// Not detached: see BackgroundTask.
	BackgroundTask::run([this, url, processor]() {
		downloadThread(url, processor);
	});
}

void HttpTexture::downloadThread(const jstring &url, HttpTextureProcessor *processor)
{
	try
	{
		// Convert URL to UTF-8
		std::string urlStr = String::toUTF8(url);
		
		BufferedImage image;
		bool fromCache = false;
		
		// Check if this is a skin URL and try to load from cache
		if (url.find(u"/skin/") != jstring::npos)
		{
			// Extract filename from URL (e.g., "playername.png" from "http://betacraft.uk:11705/skin/playername.png")
			size_t skinPos = urlStr.find("/skin/");
			if (skinPos != std::string::npos)
			{
				std::string filename = urlStr.substr(skinPos + 6); // Skip "/skin/"
				
				// Use working directory (.mcbetacpp/skins) instead of AppData/skins
				std::unique_ptr<File> workDir(File::openWorkingDirectory(u".mcbetacpp"));
				if (workDir != nullptr)
				{
					std::unique_ptr<File> cacheDir(File::open(*workDir, u"skins"));
				if (cacheDir != nullptr)
				{
					// Create cache directory if it doesn't exist
					if (!cacheDir->exists())
					{
						cacheDir->mkdirs();
					}
					
					// Get cache file
					std::unique_ptr<File> cacheFile(File::open(*cacheDir, String::fromUTF8(filename)));
					if (cacheFile != nullptr && cacheFile->exists() && cacheFile->isFile())
					{
						// Try to load from cache
						try
						{
							std::unique_ptr<std::istream> is(cacheFile->toStreamIn());
							if (is != nullptr)
							{
								image = BufferedImage::ImageIO_read(*is);
								if (image.getWidth() > 0 && image.getHeight() > 0)
								{
									fromCache = true;
								}
							}
						}
						catch (...)
						{
							// Failed to load from cache, will download instead
							}
						}
					}
				}
			}
		}
		
		// If not loaded from cache, download from URL
		if (!fromCache)
		{
			const HttpGet::Response res = HttpGet::fetch(urlStr);
			if (res.ok())
			{
				image = BufferedImage::ImageIO_readFromMemory(
					reinterpret_cast<const unsigned char*>(res.body.data()),
					static_cast<int_t>(res.body.size())
				);
			}
			else if (!res.error.empty())
			{
				std::cerr << "HTTP request failed for skin: " << urlStr
					<< " (" << res.error << ")" << std::endl;
			}
			else
			{
				std::cerr << "HTTP request failed for skin: " << urlStr
					<< " (status: " << res.status << ")" << std::endl;
			}
		}
		
		if (image.getWidth() > 0 && image.getHeight() > 0)
		{
			// Process image if processor is provided
			if (processor != nullptr)
			{
				loadedImage = processor->process(image);
			}
			else
			{
				loadedImage = std::move(image);
			}
			
			// Save to cache if this is a skin (save processed image)
			if (url.find(u"/skin/") != jstring::npos && loadedImage.getWidth() > 0 && loadedImage.getHeight() > 0)
			{
				try
				{
					size_t skinPos = urlStr.find("/skin/");
					if (skinPos != std::string::npos)
					{
						std::string filename = urlStr.substr(skinPos + 6);
						
						// Use working directory (.mcbetacpp/skins) instead of AppData/skins
						// This matches where other game data is stored
						std::unique_ptr<File> workDir(File::openWorkingDirectory(u".mcbetacpp"));
						if (workDir != nullptr)
						{
							std::unique_ptr<File> cacheDir(File::open(*workDir, u"skins"));
						if (cacheDir != nullptr)
						{
							if (!cacheDir->exists())
							{
									if (!cacheDir->mkdirs())
									{
										std::cerr << "Failed to create skins directory: " << String::toUTF8(cacheDir->toString()) << std::endl;
									}
							}
							
							std::unique_ptr<File> cacheFile(File::open(*cacheDir, String::fromUTF8(filename)));
							if (cacheFile != nullptr)
							{
								// Write PNG to cache file (processed image)
								std::string cachePath = String::toUTF8(cacheFile->toString());
								const unsigned char *pixels = loadedImage.getRawPixels();
								if (pixels != nullptr)
								{
										int result = stbi_write_png(cachePath.c_str(), loadedImage.getWidth(), loadedImage.getHeight(), 4, pixels, loadedImage.getWidth() * 4);
										if (result == 0)
										{
											std::cerr << "Failed to save skin to cache: " << cachePath << std::endl;
								}
							}
									else
									{
										std::cerr << "Failed to get pixel data for skin: " << filename << std::endl;
						}
					}
								else
								{
									std::cerr << "Failed to open cache file for skin: " << filename << std::endl;
								}
							}
							else
							{
								std::cerr << "Failed to open skins directory" << std::endl;
							}
						}
						else
						{
							std::cerr << "Failed to open working directory for skin cache" << std::endl;
						}
					}
				}
				catch (const std::exception &e)
				{
					std::cerr << "Exception while saving skin to cache: " << e.what() << std::endl;
				}
				catch (...)
				{
					std::cerr << "Unknown exception while saving skin to cache" << std::endl;
				}
			}
			
			// Note: isLoaded is set to true in Textures::loadHttpTexture() after uploading to OpenGL
			// Don't set it here - the download thread only sets loadedImage
		}
	}
	catch (const std::exception &e)
	{
		std::cerr << "Failed to download HTTP texture: " << e.what() << std::endl;
	}
}
