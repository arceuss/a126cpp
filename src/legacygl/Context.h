#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

#include "legacygl/Geometry.h"
#include "legacygl/LegacyGL.h"
#include "legacygl/Matrix.h"
#include "legacygl/Primitive.h"
#include "legacygl/ResolvedCommands.h"
#include "legacygl/Sink.h"

// The one authoritative implementation of legacy OpenGL semantics.
//
// Every frontend entry point lands here. The context owns the state whose
// behaviour is independent of the target API - matrices, current attributes,
// enables, texture objects, buffers, display lists, pixel store, errors - and
// answers every query from that state rather than from whichever backend is
// rendering. Backends receive the call stream through a Sink and never
// reimplement any of this.
//
// Call-time versus draw-time matters and is encoded per command. glLight*
// GL_POSITION is transformed by the model-view matrix in force when the setter
// runs; a display list captures array data when it compiles but resolves
// unsupplied vertex attributes when it executes.

namespace legacygl
{

struct TextureLevel
{
	int width = 0;
	int height = 0;
	int internalFormat = 0;
	bool defined = false;
};

// Legacy texture-object defaults are not nearest/nearest: a fresh object
// minifies with NEAREST_MIPMAP_LINEAR and magnifies with LINEAR, so a
// level-zero-only object is incomplete until the application picks a
// non-mipmapped minification filter. Textures.cpp does exactly that.
class TextureObject
{
public:
	static const int MAX_LEVELS = 16;

	unsigned int minFilter = GL_NEAREST_MIPMAP_LINEAR;
	unsigned int magFilter = GL_LINEAR;
	unsigned int wrapS = GL_REPEAT;
	unsigned int wrapT = GL_REPEAT;
	float borderColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	TextureLevel levels[MAX_LEVELS];

	bool usesMipmapFilter() const;
	// Complete for the currently selected minification filter.
	bool complete() const;
};

struct BufferObject
{
	std::ptrdiff_t size = 0;
	unsigned int usage = 0;
	std::vector<unsigned char> data;
};

// Display-list commands. Only list-compilable commands appear; client-array
// enables, pointer setters, name generation, buffer management, queries,
// readback and glFinish execute immediately even while a list is compiling, so
// they have no opcode here.
enum class ListOp : unsigned char
{
	MatrixMode,
	LoadIdentity,
	PushMatrix,
	PopMatrix,
	Translatef,
	Rotatef,
	Scalef,
	Ortho,
	Frustum,
	Enable,
	Disable,
	BlendFunc,
	AlphaFunc,
	DepthFunc,
	DepthMask,
	ColorMask,
	CullFace,
	ShadeModel,
	LogicOp,
	LineWidth,
	PolygonOffset,
	Viewport,
	Color4f,
	Normal3f,
	Fogf,
	Fogfv,
	Fogi,
	Lightfv,
	LightModelfv,
	ColorMaterial,
	BindTexture,
	TexParameteri,
	TexImage2D,
	TexSubImage2D,
	Clear,
	ClearColor,
	ClearDepth,
	// Immediate mode is compiled command by command, not captured as finished
	// geometry: the vertices must read the current attributes that the list's
	// own colour and normal commands install while it executes.
	Begin,
	End,
	Vertex3f,
	TexCoord2f,
	// A client-array draw, whose vertex data was decoded and copied at compile
	// time as the specification requires.
	Geometry,
	CallList,
	CallLists
};

struct ListCommand
{
	ListOp op = ListOp::LoadIdentity;
	unsigned int u0 = 0;
	unsigned int u1 = 0;
	int i0 = 0;
	int i1 = 0;
	int i2 = 0;
	int i3 = 0;
	float f0 = 0.0f;
	float f1 = 0.0f;
	float f2 = 0.0f;
	float f3 = 0.0f;
	// Index into the owning list's doubles, geometry or names storage.
	int aux = -1;
};

struct ListTextureUpload
{
	GLenum target = GL_TEXTURE_2D;
	GLint level = 0;
	GLint internalFormat = 0;
	GLint border = 0;
	GLint xOffset = 0;
	GLint yOffset = 0;
	GLsizei width = 0;
	GLsizei height = 0;
	GLenum sourceFormat = 0;
	GLenum sourceType = 0;
	GLint unpackAlignment = 4;
	bool pixelsProvided = false;
	std::vector<unsigned char> pixelBytes;
};

struct DisplayList
{
	bool defined = false;
	std::vector<ListCommand> commands;
	// Owned payloads. A display list never keeps a caller pointer alive: array
	// draws are decoded into geometry, texture pixels and glCallLists elements
	// are copied at compile time.
	std::vector<Geometry> geometry;
	std::vector<double> doubles;
	std::vector<unsigned int> names;
	std::vector<ListTextureUpload> textureUploads;

	void clear();
};

struct ArrayState
{
	bool enabled = false;
	int size = 0;
	unsigned int type = 0;
	int stride = 0;
	const void *pointer = nullptr;
	// Buffer bound to GL_ARRAY_BUFFER_ARB when the pointer was set; zero means
	// the pointer is client memory.
	unsigned int buffer = 0;
};

class Context
{
public:
	static const std::size_t MODELVIEW_STACK_DEPTH = 32;
	static const std::size_t PROJECTION_STACK_DEPTH = 2;
	static const std::size_t TEXTURE_STACK_DEPTH = 2;
	static const int MAX_LIGHTS = ResolvedLightingState::MAX_LIGHTS;

	Context();

	// Restores every value to its legacy default and drops all objects. Used by
	// context creation and by the state tests.
	void reset();

	void setSink(Sink *sink) { activeSink = sink; }
	Sink *sink() const { return activeSink; }

	// Validation mode diffs the semantic answers against the rendering backend.
	// Divergences are classified rather than averaged away: a one-unit-in-the-
	// last-place difference in a matrix element is a documented consequence of
	// the driver's own arithmetic, while anything larger is a defect.
	void setValidate(bool enable) { validate = enable; }
	bool validating() const { return validate; }
	long long validationFailures() const { return validationBeyondOneUlp; }
	long long validationOneUlpDivergences() const { return validationOneUlp; }
	long long validationChecks() const { return validationCheckCount; }
	const std::string &firstValidationFailure() const { return firstValidationFailureMessage; }
	// Human-readable summary of every query that ever diverged.
	std::string validationReport() const;

	// ---- frontend entry points -------------------------------------------

	void matrixMode(GLenum mode);
	void loadIdentity();
	void pushMatrix();
	void popMatrix();
	void translatef(GLfloat x, GLfloat y, GLfloat z);
	void rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void scalef(GLfloat x, GLfloat y, GLfloat z);
	void scaled(GLdouble x, GLdouble y, GLdouble z);
	void ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
	void frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);

	void enable(GLenum cap);
	void disable(GLenum cap);
	void blendFunc(GLenum sfactor, GLenum dfactor);
	void alphaFunc(GLenum func, GLclampf ref);
	void depthFunc(GLenum func);
	void depthMask(GLboolean flag);
	void colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
	void cullFace(GLenum mode);
	void shadeModel(GLenum mode);
	void logicOp(GLenum opcode);
	void lineWidth(GLfloat width);
	void polygonOffset(GLfloat factor, GLfloat units);
	void viewport(GLint x, GLint y, GLsizei width, GLsizei height);
	void pixelStorei(GLenum pname, GLint param);

	// glColor3f sets alpha to 1.0; it does not preserve the previous alpha.
	void color3f(GLfloat red, GLfloat green, GLfloat blue);
	void color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	void normal3f(GLfloat nx, GLfloat ny, GLfloat nz);
	void normal3b(GLbyte nx, GLbyte ny, GLbyte nz);

	void fogf(GLenum pname, GLfloat param);
	void fogfv(GLenum pname, const GLfloat *params);
	void fogi(GLenum pname, GLint param);

	void lightfv(GLenum light, GLenum pname, const GLfloat *params);
	void lightModelfv(GLenum pname, const GLfloat *params);
	void colorMaterial(GLenum face, GLenum mode);

	void genTextures(GLsizei n, GLuint *textures);
	void deleteTextures(GLsizei n, const GLuint *textures);
	void bindTexture(GLenum target, GLuint texture);
	void texParameteri(GLenum target, GLenum pname, GLint param);
	void texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
		GLint border, GLenum format, GLenum type, const GLvoid *pixels);
	void texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
		GLenum format, GLenum type, const GLvoid *pixels);

	void enableClientState(GLenum array);
	void disableClientState(GLenum array);
	void vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
	void normalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
	void drawArrays(GLenum mode, GLint first, GLsizei count);

	void begin(GLenum mode);
	void end();
	void vertex3f(GLfloat x, GLfloat y, GLfloat z);
	void texCoord2f(GLfloat s, GLfloat t);

	void genBuffersARB(GLsizei n, GLuint *buffers);
	void bindBufferARB(GLenum target, GLuint buffer);
	void bufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);

	GLuint genLists(GLsizei range);
	void newList(GLuint list, GLenum mode);
	void endList();
	void callList(GLuint list);
	void callLists(GLsizei n, GLenum type, const GLvoid *lists);
	void deleteLists(GLuint list, GLsizei range);

	void clear(GLbitfield mask);
	void clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
	void clearDepth(GLclampd depth);
	void readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
	void finish();

	void getFloatv(GLenum pname, GLfloat *params);
	GLenum getError();

	// ---- observable state, read by tests and backends --------------------

	MatrixStack &matrixStack(GLenum mode);
	const MatrixStack &modelView() const { return modelViewStack; }
	const MatrixStack &projection() const { return projectionStack; }
	const MatrixStack &textureMatrix() const { return textureStack; }
	GLenum currentMatrixMode() const { return matrixModeValue; }

	const Vertex &currentAttributes() const { return current; }
	bool currentColorIndeterminate() const { return colorIndeterminate; }
	bool currentNormalIndeterminate() const { return normalIndeterminate; }
	bool currentTexCoordIndeterminate() const { return texCoordIndeterminate; }
	long long indeterminateUseCount() const { return indeterminateUses; }

	bool isEnabled(GLenum cap) const;

	GLenum pendingError() const { return errorValue; }

	// Texture namespace
	bool isTextureName(GLuint name) const;
	bool isTextureObject(GLuint name) const;
	const TextureObject *texture(GLuint name) const;
	GLuint boundTexture() const { return textureBinding; }
	// Object zero's state is independent of every named object.
	const TextureObject &defaultTexture() const { return textureZero; }

	// Buffer namespace
	bool isBufferName(GLuint name) const;
	const BufferObject *buffer(GLuint name) const;
	GLuint boundArrayBuffer() const { return arrayBufferBinding; }

	// Display lists
	const DisplayList *displayList(GLuint name) const;
	bool compilingList() const { return compilingListName != 0; }
	GLuint compilingListNameValue() const { return compilingListName; }
	GLenum compilingListMode() const { return compilingListModeValue; }
	int listDepth() const { return executionDepth; }

	const ArrayState &vertexArray() const { return vertexArrayState; }
	const ArrayState &colorArray() const { return colorArrayState; }
	const ArrayState &texCoordArray() const { return texCoordArrayState; }
	const ArrayState &normalArray() const { return normalArrayState; }

	// Fixed-function state, exposed for backends and tests.
	GLenum blendSource() const { return blendSrc; }
	GLenum blendDestination() const { return blendDst; }
	GLenum alphaTestFunc() const { return alphaFuncValue; }
	GLfloat alphaTestRef() const { return alphaRefValue; }
	GLenum depthTestFunc() const { return depthFuncValue; }
	bool depthWriteEnabled() const { return depthMaskValue; }
	const bool *colorWriteMask() const { return colorMaskValue; }
	GLenum cullFaceMode() const { return cullFaceValue; }
	GLenum frontFaceMode() const { return frontFaceValue; }
	GLenum shadeModelMode() const { return shadeModelValue; }
	GLenum logicOpcode() const { return logicOpValue; }
	GLfloat lineWidthValue() const { return lineWidthState; }
	GLfloat polygonOffsetFactor() const { return polygonOffsetFactorValue; }
	GLfloat polygonOffsetUnits() const { return polygonOffsetUnitsValue; }
	const GLint *viewportBox() const { return viewportValue; }
	GLint packAlignment() const { return packAlignmentValue; }
	GLint unpackAlignment() const { return unpackAlignmentValue; }

	GLenum fogMode() const { return fogModeValue; }
	GLfloat fogDensity() const { return fogDensityValue; }
	GLfloat fogStart() const { return fogStartValue; }
	GLfloat fogEnd() const { return fogEndValue; }
	const GLfloat *fogColor() const { return fogColorValue; }
	GLenum fogDistanceMode() const { return fogDistanceModeValue; }

	const LightState &light(int index) const { return lights[index]; }
	const GLfloat *lightModelAmbient() const { return lightModelAmbientValue; }
	GLenum colorMaterialFace() const { return colorMaterialFaceValue; }
	GLenum colorMaterialMode() const { return colorMaterialModeValue; }
	const MaterialState &frontMaterial() const { return materialFront; }
	const MaterialState &backMaterial() const { return materialBack; }

	const GLfloat *clearColorValue() const { return clearColorState; }
	GLclampd clearDepthValue() const { return clearDepthState; }

	// The geometry produced by the most recent draw, immediate-mode block or
	// executed display-list draw. Backends and tests read it instead of
	// re-decoding client memory.
	const Geometry &lastGeometry() const { return lastDrawGeometry; }
	const PrimitiveBatch &lastPrimitives() const { return lastDrawPrimitives; }
	long long drawCount() const { return draws; }

	// Fog coordinate helper shared by every backend: standard fixed-function
	// fog uses |eye z|; radial distance is only correct when the NV distance
	// mode selected it.
	bool fogUsesRadialDistance() const { return fogDistanceModeValue == GL_EYE_RADIAL_NV; }

	// Sequence number of the next frontend call, used by the trace and by
	// validation messages.
	long long sequence() const { return callSequence; }
	long long nextSequence() { return ++callSequence; }

	// Validation probe results for the current attributes after an array draw.
	// The specification leaves those values undefined, so what the reference
	// driver actually does is measured instead of assumed.
	long long postArrayColorPreserved() const { return arrayColorPreserved; }
	long long postArrayColorLastElement() const { return arrayColorLastElement; }
	long long postArrayColorOther() const { return arrayColorOther; }

private:
	// State mutation without sink forwarding. Direct calls and display-list
	// execution share these so a compiled command behaves exactly like the
	// original call.
	void applyMatrixMode(GLenum mode);
	void applyTranslatef(GLfloat x, GLfloat y, GLfloat z);
	void applyRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
	void applyScalef(GLfloat x, GLfloat y, GLfloat z);
	void applyEnable(GLenum cap, bool value);
	void applyColor4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a);
	void applyNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
	void applyBegin(GLenum mode);
	void applyEnd();
	void applyVertex3f(GLfloat x, GLfloat y, GLfloat z);
	void applyFogf(GLenum pname, GLfloat param);
	void applyFogi(GLenum pname, GLint param);
	void applyLightfv(GLenum light, GLenum pname, const GLfloat *params);
	void probePostArrayColor(const Vertex &before, const Geometry &drawn);
	void applyBindTexture(GLuint texture);
	void applyTexParameteri(GLenum pname, GLint param);
	void executeTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
		GLint border, GLenum format, GLenum type, GLint unpackAlignment, const GLvoid *pixels, bool forwardRaw);
	void executeTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width,
		GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const GLvoid *pixels, bool forwardRaw);
	void executeGeometry(const Geometry &geometry);
	void releaseDisplayListGeometry(const DisplayList &list);
	std::uint64_t allocateGeometryResidencyId();
	void emitResolvedClear(GLbitfield mask);

	void setError(GLenum error);
	bool validateEnableCap(GLenum cap);
	bool *enableSlot(GLenum cap);
	const bool *enableSlot(GLenum cap) const;
	int lightIndex(GLenum light) const;

	void record(const ListCommand &command);
	bool recordingOnly() const { return compilingListName != 0 && compilingListModeValue == GL_COMPILE; }
	void executeList(GLuint name);
	void noteIndeterminateUse(const char *what);
	void checkQueryAgainstSink(GLenum pname, const GLfloat *values, int count);

	// Decodes count vertices starting at first from the enabled client arrays
	// into out. Returns false and raises the GL error when the arrays are
	// unusable.
	bool decodeArrays(GLenum mode, GLint first, GLsizei count, Geometry &out, bool captureVertices);

	Sink *activeSink = nullptr;
	bool validate = false;
	long long validationCheckCount = 0;
	long long validationOneUlp = 0;
	long long validationBeyondOneUlp = 0;
	std::string firstValidationFailureMessage;

	// One entry per queried name that has ever diverged.
	struct QueryDivergence
	{
		GLenum pname = 0;
		int component = 0;
		long long oneUlp = 0;
		long long beyondOneUlp = 0;
		long long worstUlps = 0;
		float worstCore = 0.0f;
		float worstBackend = 0.0f;
	};
	std::vector<QueryDivergence> divergences;
	long long callSequence = 0;

	GLenum matrixModeValue = GL_MODELVIEW;
	MatrixStack modelViewStack;
	MatrixStack projectionStack;
	MatrixStack textureStack;

	Vertex current;
	bool colorIndeterminate = false;
	bool normalIndeterminate = false;
	bool texCoordIndeterminate = false;
	long long indeterminateUses = 0;
	long long indeterminateColorUses = 0;
	long long indeterminateNormalUses = 0;
	long long indeterminateTexCoordUses = 0;
	long long arrayColorPreserved = 0;
	long long arrayColorLastElement = 0;
	long long arrayColorOther = 0;

	bool capTexture2D = false;
	bool capDepthTest = false;
	bool capAlphaTest = false;
	bool capBlend = false;
	bool capCullFace = false;
	bool capFog = false;
	bool capLighting = false;
	bool capColorMaterial = false;
	bool capRescaleNormal = false;
	bool capNormalize = false;
	bool capColorLogicOp = false;
	bool capPolygonOffsetFill = false;
	bool capScissorTest = false;
	bool capStencilTest = false;
	bool capLineSmooth = false;
	bool capDither = true;

	GLenum blendSrc = GL_ONE;
	GLenum blendDst = GL_ZERO;
	GLenum alphaFuncValue = GL_ALWAYS;
	GLfloat alphaRefValue = 0.0f;
	GLenum depthFuncValue = GL_LESS;
	bool depthMaskValue = true;
	bool colorMaskValue[4] = { true, true, true, true };
	GLenum cullFaceValue = GL_BACK;
	GLenum frontFaceValue = GL_CCW;
	GLenum shadeModelValue = GL_SMOOTH;
	GLenum logicOpValue = GL_COPY;
	GLfloat lineWidthState = 1.0f;
	GLfloat polygonOffsetFactorValue = 0.0f;
	GLfloat polygonOffsetUnitsValue = 0.0f;
	GLint viewportValue[4] = { 0, 0, 0, 0 };
	GLint packAlignmentValue = 4;
	GLint unpackAlignmentValue = 4;

	GLenum fogModeValue = GL_EXP;
	GLfloat fogDensityValue = 1.0f;
	GLfloat fogStartValue = 0.0f;
	GLfloat fogEndValue = 1.0f;
	GLfloat fogColorValue[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLenum fogDistanceModeValue = GL_EYE_PLANE_ABSOLUTE_NV;

	LightState lights[MAX_LIGHTS];
	GLfloat lightModelAmbientValue[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
	GLenum colorMaterialFaceValue = GL_FRONT_AND_BACK;
	GLenum colorMaterialModeValue = GL_AMBIENT_AND_DIFFUSE;
	MaterialState materialFront;
	MaterialState materialBack;

	GLfloat clearColorState[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	GLclampd clearDepthState = 1.0;

	GLenum errorValue = GL_NO_ERROR;

	// Texture names that exist but have never been bound are reserved, not yet
	// objects. Binding a reserved or entirely unused nonzero name creates one.
	std::vector<GLuint> reservedTextureNames;
	std::unordered_map<GLuint, TextureObject> textureObjects;
	TextureObject textureZero;
	GLuint textureBinding = 0;

	std::vector<GLuint> reservedBufferNames;
	std::unordered_map<GLuint, BufferObject> bufferObjects;
	GLuint arrayBufferBinding = 0;

	std::unordered_map<GLuint, DisplayList> displayLists;
	DisplayList compilingDefinition;
	GLuint compilingListName = 0;
	GLenum compilingListModeValue = GL_COMPILE;
	GLuint nextListName = 1;
	std::uint64_t nextGeometryResidencyId = 1;
	int executionDepth = 0;

	ArrayState vertexArrayState;
	ArrayState colorArrayState;
	ArrayState texCoordArrayState;
	ArrayState normalArrayState;

	bool immediateActive = false;
	Geometry immediate;

	Geometry lastDrawGeometry;
	PrimitiveBatch lastDrawPrimitives;
	Geometry decodeScratch;
	long long draws = 0;
};

// The process-wide frontend context.
Context &context();

// Installs a backend. Passing nullptr leaves the semantic core running with no
// rendering, which is how the GPU-free state tests execute.
void setSink(Sink *sink);

// A sink that renders nothing. State, lists, errors and queries still behave
// exactly as they do with a real backend.
Sink *nullSink();

}
