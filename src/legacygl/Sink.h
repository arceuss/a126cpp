#pragma once

#include <cstddef>
#include <cstdint>

#include "legacygl/ResolvedCommands.h"

// Backend sink for the LegacyGL frontend.
//
// The sink mirrors the inventoried frontend surface one call at a time. That is
// deliberate: the native compatibility-GL sink must reproduce the game's call
// stream exactly so it stays usable as the behavioural oracle, and the semantic
// core must never be bypassed on the way there.
//
// The raw call mirror uses plain scalar types, and the resolved command structs
// are loader-neutral. A GL loader header redefines gl* names as macros and
// typedefs GLenum itself, so the loader-facing implementation cannot include
// legacygl/LegacyGL.h. Plain types are the exact same underlying representation
// (GLenum/GLuint/GLbitfield are unsigned int, GLsizei/GLint are int, GLboolean is
// unsigned char) without dragging either header into the other's translation
// unit.
//
// Sinks that defer work (any packet-consuming GPU backend) must copy pixel and
// vertex payloads before returning; the frontend guarantees the pointers are
// valid only for the duration of the call.

namespace legacygl
{

class Sink
{
public:
	virtual ~Sink() = default;

	// Matrix stack
	virtual void matrixMode(unsigned int mode) = 0;
	virtual void loadIdentity() = 0;
	virtual void pushMatrix() = 0;
	virtual void popMatrix() = 0;
	virtual void translatef(float x, float y, float z) = 0;
	virtual void rotatef(float angle, float x, float y, float z) = 0;
	virtual void scalef(float x, float y, float z) = 0;
	virtual void scaled(double x, double y, double z) = 0;
	virtual void ortho(double left, double right, double bottom, double top, double zNear, double zFar) = 0;
	virtual void frustum(double left, double right, double bottom, double top, double zNear, double zFar) = 0;

	// Server state
	virtual void enable(unsigned int cap) = 0;
	virtual void disable(unsigned int cap) = 0;
	virtual void blendFunc(unsigned int sfactor, unsigned int dfactor) = 0;
	virtual void alphaFunc(unsigned int func, float ref) = 0;
	virtual void depthFunc(unsigned int func) = 0;
	virtual void depthMask(unsigned char flag) = 0;
	virtual void colorMask(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) = 0;
	virtual void cullFace(unsigned int mode) = 0;
	virtual void shadeModel(unsigned int mode) = 0;
	virtual void logicOp(unsigned int opcode) = 0;
	virtual void lineWidth(float width) = 0;
	virtual void polygonOffset(float factor, float units) = 0;
	virtual void viewport(int x, int y, int width, int height) = 0;
	virtual void pixelStorei(unsigned int pname, int param) = 0;

	// Current attributes
	virtual void color4f(float red, float green, float blue, float alpha) = 0;
	virtual void color3f(float red, float green, float blue) = 0;
	virtual void normal3f(float nx, float ny, float nz) = 0;
	virtual void normal3b(signed char nx, signed char ny, signed char nz) = 0;

	// Fog
	virtual void fogf(unsigned int pname, float param) = 0;
	virtual void fogfv(unsigned int pname, const float *params) = 0;
	virtual void fogi(unsigned int pname, int param) = 0;

	// Lighting
	virtual void lightfv(unsigned int light, unsigned int pname, const float *params) = 0;
	virtual void lightModelfv(unsigned int pname, const float *params) = 0;
	virtual void colorMaterial(unsigned int face, unsigned int mode) = 0;

	// Textures
	virtual void genTextures(int n, unsigned int *textures) = 0;
	virtual void deleteTextures(int n, const unsigned int *textures) = 0;
	virtual void bindTexture(unsigned int target, unsigned int texture) = 0;
	virtual void texParameteri(unsigned int target, unsigned int pname, int param) = 0;
	virtual void texImage2D(unsigned int target, int level, int internalformat, int width, int height,
		int border, unsigned int format, unsigned int type, const void *pixels) = 0;
	virtual void texSubImage2D(unsigned int target, int level, int xoffset, int yoffset, int width, int height,
		unsigned int format, unsigned int type, const void *pixels) = 0;

	// Client arrays
	virtual void enableClientState(unsigned int array) = 0;
	virtual void disableClientState(unsigned int array) = 0;
	virtual void vertexPointer(int size, unsigned int type, int stride, const void *pointer) = 0;
	virtual void texCoordPointer(int size, unsigned int type, int stride, const void *pointer) = 0;
	virtual void colorPointer(int size, unsigned int type, int stride, const void *pointer) = 0;
	virtual void normalPointer(unsigned int type, int stride, const void *pointer) = 0;
	virtual void drawArrays(unsigned int mode, int first, int count) = 0;

	// Immediate mode
	virtual void begin(unsigned int mode) = 0;
	virtual void end() = 0;
	virtual void vertex3f(float x, float y, float z) = 0;
	virtual void texCoord2f(float s, float t) = 0;

	// GL_ARB_vertex_buffer_object
	virtual void genBuffersARB(int n, unsigned int *buffers) = 0;
	virtual void bindBufferARB(unsigned int target, unsigned int buffer) = 0;
	virtual void bufferDataARB(unsigned int target, std::ptrdiff_t size, const void *data, unsigned int usage) = 0;

	// Display lists
	virtual unsigned int genLists(int range) = 0;
	virtual void newList(unsigned int list, unsigned int mode) = 0;
	virtual void endList() = 0;
	virtual void callList(unsigned int list) = 0;
	virtual void callLists(int n, unsigned int type, const void *lists) = 0;
	virtual void deleteLists(unsigned int list, int range) = 0;

	// Framebuffer
	virtual void clear(unsigned int mask) = 0;
	virtual void clearColor(float red, float green, float blue, float alpha) = 0;
	virtual void clearDepth(double depth) = 0;
	virtual void readPixels(int x, int y, int width, int height, unsigned int format, unsigned int type,
		void *pixels) = 0;
	virtual void finish() = 0;

	// A sink that consumes resolved geometry (any packet-based GPU backend, and
	// the state tests) opts in here. The native sink does not: OpenGL already
	// walks the client arrays itself, so decoding them again would be pure
	// overhead on the reference path.
	virtual bool wantsCanonicalGeometry() const { return false; }

	// Loader-neutral work emitted after the semantic core has validated each
	// call and resolved all legacy state needed by a translated backend.
	virtual void releaseCanonicalGeometry(std::uint64_t residencyId) { (void)residencyId; }
	virtual void resolvedDraw(const ResolvedDraw &command) { (void)command; }
	virtual void resolvedClear(const ResolvedClear &command) { (void)command; }
	virtual void resolvedTextureUpload(const ResolvedTextureUpload &command) { (void)command; }
	virtual void resolvedReadback(const ResolvedReadback &command) { (void)command; }

	// Oracle comparison hooks. The semantic core answers every query itself;
	// these exist so the validation build can diff core state against the
	// backend that is actually rendering. Sinks without a queryable device
	// return false and the comparison is skipped.
	virtual bool queryFloatv(unsigned int pname, float *params) { (void)pname; (void)params; return false; }
	virtual bool queryError(unsigned int *error) { (void)error; return false; }
};

}
