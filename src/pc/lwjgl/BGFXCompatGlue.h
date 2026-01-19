#pragma once

#ifdef USE_BGFX

// C-linkage functions for Tesselator to call during display list compilation
extern "C" {
	bool bgfx_compiling_display_list();
	void bgfx_capture_tesselator_data(const float* vertexData, size_t vertexCount, 
		GLenum drawMode, bool hasTexture, bool hasColor, bool hasNormal);
}

#endif // USE_BGFX
