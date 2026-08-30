#include "legacygl/Context.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>

#include "legacygl/Trace.h"

namespace legacygl
{

bool TextureObject::usesMipmapFilter() const
{
	return minFilter == GL_NEAREST_MIPMAP_NEAREST || minFilter == GL_LINEAR_MIPMAP_NEAREST ||
		minFilter == GL_NEAREST_MIPMAP_LINEAR || minFilter == GL_LINEAR_MIPMAP_LINEAR;
}

bool TextureObject::complete() const
{
	if (!levels[0].defined)
		return false;
	if (!usesMipmapFilter())
		return true;

	// A mipmapped minification filter needs the whole chain down to 1x1, each
	// level exactly half the previous one.
	int width = levels[0].width;
	int height = levels[0].height;
	int level = 0;
	while (width > 1 || height > 1)
	{
		width = width > 1 ? width / 2 : 1;
		height = height > 1 ? height / 2 : 1;
		level++;
		if (level >= MAX_LEVELS)
			return false;
		if (!levels[level].defined || levels[level].width != width || levels[level].height != height)
			return false;
		if (levels[level].internalFormat != levels[0].internalFormat)
			return false;
	}
	return true;
}

void DisplayList::clear()
{
	defined = false;
	commands.clear();
	geometry.clear();
	doubles.clear();
	names.clear();
}

// ---------------------------------------------------------------------------
// A backend that renders nothing. The semantic core is unchanged, which is what
// the GPU-free tests need; names still have to come from somewhere, so this sink
// allocates them deterministically.
// ---------------------------------------------------------------------------

class NullSink : public Sink
{
public:
	void matrixMode(unsigned int) override {}
	void loadIdentity() override {}
	void pushMatrix() override {}
	void popMatrix() override {}
	void translatef(float, float, float) override {}
	void rotatef(float, float, float, float) override {}
	void scalef(float, float, float) override {}
	void scaled(double, double, double) override {}
	void ortho(double, double, double, double, double, double) override {}
	void frustum(double, double, double, double, double, double) override {}

	void enable(unsigned int) override {}
	void disable(unsigned int) override {}
	void blendFunc(unsigned int, unsigned int) override {}
	void alphaFunc(unsigned int, float) override {}
	void depthFunc(unsigned int) override {}
	void depthMask(unsigned char) override {}
	void colorMask(unsigned char, unsigned char, unsigned char, unsigned char) override {}
	void cullFace(unsigned int) override {}
	void shadeModel(unsigned int) override {}
	void logicOp(unsigned int) override {}
	void lineWidth(float) override {}
	void polygonOffset(float, float) override {}
	void viewport(int, int, int, int) override {}
	void pixelStorei(unsigned int, int) override {}

	void color4f(float, float, float, float) override {}
	void color3f(float, float, float) override {}
	void normal3f(float, float, float) override {}
	void normal3b(signed char, signed char, signed char) override {}

	void fogf(unsigned int, float) override {}
	void fogfv(unsigned int, const float *) override {}
	void fogi(unsigned int, int) override {}

	void lightfv(unsigned int, unsigned int, const float *) override {}
	void lightModelfv(unsigned int, const float *) override {}
	void colorMaterial(unsigned int, unsigned int) override {}

	void genTextures(int n, unsigned int *textures) override
	{
		for (int i = 0; i < n; i++)
			textures[i] = nextTexture++;
	}
	void deleteTextures(int, const unsigned int *) override {}
	void bindTexture(unsigned int, unsigned int) override {}
	void texParameteri(unsigned int, unsigned int, int) override {}
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override {}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int, unsigned int, const void *) override {}

	void enableClientState(unsigned int) override {}
	void disableClientState(unsigned int) override {}
	void vertexPointer(int, unsigned int, int, const void *) override {}
	void texCoordPointer(int, unsigned int, int, const void *) override {}
	void colorPointer(int, unsigned int, int, const void *) override {}
	void normalPointer(unsigned int, int, const void *) override {}
	void drawArrays(unsigned int, int, int) override {}

	void begin(unsigned int) override {}
	void end() override {}
	void vertex3f(float, float, float) override {}
	void texCoord2f(float, float) override {}

	void genBuffersARB(int n, unsigned int *buffers) override
	{
		for (int i = 0; i < n; i++)
			buffers[i] = nextBuffer++;
	}
	void bindBufferARB(unsigned int, unsigned int) override {}
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override {}

	unsigned int genLists(int range) override
	{
		if (range <= 0)
			return 0;
		unsigned int base = nextList;
		nextList += static_cast<unsigned int>(range);
		return base;
	}
	void newList(unsigned int, unsigned int) override {}
	void endList() override {}
	void callList(unsigned int) override {}
	void callLists(int, unsigned int, const void *) override {}
	void deleteLists(unsigned int, int) override {}

	void clear(unsigned int) override {}
	void clearColor(float, float, float, float) override {}
	void clearDepth(double) override {}
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override {}
	void finish() override {}

	bool wantsCanonicalGeometry() const override { return true; }

private:
	unsigned int nextTexture = 1;
	unsigned int nextBuffer = 1;
	unsigned int nextList = 1;
};

static NullSink theNullSink;

Sink *nullSink()
{
	return &theNullSink;
}

static Context theContext;

Context &context()
{
	return theContext;
}

void setSink(Sink *sink)
{
	theContext.setSink(sink);
}

// ---------------------------------------------------------------------------
// Construction and defaults
// ---------------------------------------------------------------------------

Context::Context()
	: modelViewStack(MODELVIEW_STACK_DEPTH), projectionStack(PROJECTION_STACK_DEPTH),
	  textureStack(TEXTURE_STACK_DEPTH)
{
	reset();
}

void Context::reset()
{
	matrixModeValue = GL_MODELVIEW;
	modelViewStack.reset();
	projectionStack.reset();
	textureStack.reset();

	current = Vertex();
	colorIndeterminate = false;
	normalIndeterminate = false;
	texCoordIndeterminate = false;
	indeterminateUses = 0;

	capTexture2D = false;
	capDepthTest = false;
	capAlphaTest = false;
	capBlend = false;
	capCullFace = false;
	capFog = false;
	capLighting = false;
	capColorMaterial = false;
	capRescaleNormal = false;
	capNormalize = false;
	capColorLogicOp = false;
	capPolygonOffsetFill = false;
	capScissorTest = false;
	capStencilTest = false;
	capLineSmooth = false;
	capDither = true;

	blendSrc = GL_ONE;
	blendDst = GL_ZERO;
	alphaFuncValue = GL_ALWAYS;
	alphaRefValue = 0.0f;
	depthFuncValue = GL_LESS;
	depthMaskValue = true;
	for (int i = 0; i < 4; i++)
		colorMaskValue[i] = true;
	cullFaceValue = GL_BACK;
	frontFaceValue = GL_CCW;
	shadeModelValue = GL_SMOOTH;
	logicOpValue = GL_COPY;
	lineWidthState = 1.0f;
	polygonOffsetFactorValue = 0.0f;
	polygonOffsetUnitsValue = 0.0f;
	for (int i = 0; i < 4; i++)
		viewportValue[i] = 0;
	packAlignmentValue = 4;
	unpackAlignmentValue = 4;

	fogModeValue = GL_EXP;
	fogDensityValue = 1.0f;
	fogStartValue = 0.0f;
	fogEndValue = 1.0f;
	for (int i = 0; i < 4; i++)
		fogColorValue[i] = 0.0f;
	fogDistanceModeValue = GL_EYE_PLANE_ABSOLUTE_NV;

	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		lights[i] = LightState();
		// Light zero's diffuse and specular default to white; every other
		// light defaults to black.
		if (i == 0)
		{
			for (int c = 0; c < 3; c++)
			{
				lights[i].diffuse[c] = 1.0f;
				lights[i].specular[c] = 1.0f;
			}
		}
	}
	lightModelAmbientValue[0] = 0.2f;
	lightModelAmbientValue[1] = 0.2f;
	lightModelAmbientValue[2] = 0.2f;
	lightModelAmbientValue[3] = 1.0f;
	colorMaterialFaceValue = GL_FRONT_AND_BACK;
	colorMaterialModeValue = GL_AMBIENT_AND_DIFFUSE;
	materialFront = MaterialState();
	materialBack = MaterialState();

	for (int i = 0; i < 4; i++)
		clearColorState[i] = 0.0f;
	clearDepthState = 1.0;

	errorValue = GL_NO_ERROR;

	reservedTextureNames.clear();
	textureObjects.clear();
	textureZero = TextureObject();
	textureBinding = 0;

	reservedBufferNames.clear();
	bufferObjects.clear();
	arrayBufferBinding = 0;

	displayLists.clear();
	compilingListName = 0;
	compilingListModeValue = GL_COMPILE;
	nextListName = 1;
	executionDepth = 0;

	vertexArrayState = ArrayState();
	colorArrayState = ArrayState();
	texCoordArrayState = ArrayState();
	normalArrayState = ArrayState();

	immediateActive = false;
	immediate.clear();
	lastDrawGeometry.clear();
	lastDrawPrimitives.primitives.clear();
	draws = 0;

	validationCheckCount = 0;
	validationOneUlp = 0;
	validationBeyondOneUlp = 0;
	firstValidationFailureMessage.clear();
	divergences.clear();
	indeterminateColorUses = 0;
	indeterminateNormalUses = 0;
	indeterminateTexCoordUses = 0;
	arrayColorPreserved = 0;
	arrayColorLastElement = 0;
	arrayColorOther = 0;
	callSequence = 0;
}

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

void Context::setError(GLenum error)
{
	// GL keeps the first error until it is read.
	if (errorValue == GL_NO_ERROR)
		errorValue = error;

	if (traceEnabled())
	{
		std::ostringstream out;
		out << "# error 0x" << std::hex << error << std::dec << " at call " << traceSequence(callSequence);
		traceRawLine(out.str());
	}
}

GLenum Context::getError()
{
	nextSequence();
	traceCall(callSequence, "glGetError");

	if (validate && activeSink != nullptr)
	{
		unsigned int backendError = GL_NO_ERROR;
		if (activeSink->queryError(&backendError) && backendError != GL_NO_ERROR && errorValue == GL_NO_ERROR)
		{
			std::ostringstream out;
			out << "backend reported GL error 0x" << std::hex << backendError << std::dec
				<< " that the semantic core did not, at call " << callSequence;
			validationBeyondOneUlp++;
			std::cerr << "legacygl: " << out.str() << '\n';
			if (firstValidationFailureMessage.empty())
				firstValidationFailureMessage = out.str();
		}
	}

	GLenum result = errorValue;
	errorValue = GL_NO_ERROR;
	return result;
}

// ---------------------------------------------------------------------------
// Display-list recording
// ---------------------------------------------------------------------------

void Context::record(const ListCommand &command)
{
	DisplayList &list = displayLists[compilingListName];
	list.commands.push_back(command);
}

// ---------------------------------------------------------------------------
// Matrices
// ---------------------------------------------------------------------------

MatrixStack &Context::matrixStack(GLenum mode)
{
	switch (mode)
	{
		case GL_PROJECTION:
			return projectionStack;
		case GL_TEXTURE:
			return textureStack;
		default:
			return modelViewStack;
	}
}

void Context::applyMatrixMode(GLenum mode)
{
	matrixModeValue = mode;
}

void Context::matrixMode(GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glMatrixMode", mode);

	if (mode != GL_MODELVIEW && mode != GL_PROJECTION && mode != GL_TEXTURE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->matrixMode(mode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::MatrixMode;
		command.u0 = mode;
		record(command);
		if (recordingOnly())
			return;
	}

	applyMatrixMode(mode);
}

void Context::loadIdentity()
{
	nextSequence();
	traceCall(callSequence, "glLoadIdentity");

	if (activeSink != nullptr)
		activeSink->loadIdentity();

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::LoadIdentity;
		record(command);
		if (recordingOnly())
			return;
	}

	matrixStack(matrixModeValue).loadIdentity();
}

void Context::pushMatrix()
{
	nextSequence();
	traceCall(callSequence, "glPushMatrix");

	if (activeSink != nullptr)
		activeSink->pushMatrix();

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::PushMatrix;
		record(command);
		if (recordingOnly())
			return;
	}

	if (!matrixStack(matrixModeValue).push())
		setError(GL_STACK_OVERFLOW);
}

void Context::popMatrix()
{
	nextSequence();
	traceCall(callSequence, "glPopMatrix");

	if (activeSink != nullptr)
		activeSink->popMatrix();

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::PopMatrix;
		record(command);
		if (recordingOnly())
			return;
	}

	if (!matrixStack(matrixModeValue).pop())
		setError(GL_STACK_UNDERFLOW);
}

void Context::applyTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
	matrixStack(matrixModeValue).multiply(translation(x, y, z));
}

void Context::translatef(GLfloat x, GLfloat y, GLfloat z)
{
	nextSequence();
	traceCall(callSequence, "glTranslatef", x, y, z);

	if (activeSink != nullptr)
		activeSink->translatef(x, y, z);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Translatef;
		command.f0 = x;
		command.f1 = y;
		command.f2 = z;
		record(command);
		if (recordingOnly())
			return;
	}

	applyTranslatef(x, y, z);
}

void Context::applyRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	matrixStack(matrixModeValue).multiply(rotation(angle, x, y, z));
}

void Context::rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
	nextSequence();
	traceCall(callSequence, "glRotatef", angle, x, y, z);

	if (activeSink != nullptr)
		activeSink->rotatef(angle, x, y, z);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Rotatef;
		command.f0 = angle;
		command.f1 = x;
		command.f2 = y;
		command.f3 = z;
		record(command);
		if (recordingOnly())
			return;
	}

	applyRotatef(angle, x, y, z);
}

void Context::applyScalef(GLfloat x, GLfloat y, GLfloat z)
{
	matrixStack(matrixModeValue).multiply(scaling(x, y, z));
}

void Context::scalef(GLfloat x, GLfloat y, GLfloat z)
{
	nextSequence();
	traceCall(callSequence, "glScalef", x, y, z);

	if (activeSink != nullptr)
		activeSink->scalef(x, y, z);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Scalef;
		command.f0 = x;
		command.f1 = y;
		command.f2 = z;
		record(command);
		if (recordingOnly())
			return;
	}

	applyScalef(x, y, z);
}

// The matrix stacks hold single-precision values because that is what
// glGetFloatv returns and what Frustum.cpp consumes; glScaled's arguments are
// narrowed here rather than carrying a second precision through the core.
void Context::scaled(GLdouble x, GLdouble y, GLdouble z)
{
	nextSequence();
	traceCall(callSequence, "glScaled", x, y, z);

	if (activeSink != nullptr)
		activeSink->scaled(x, y, z);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Scalef;
		command.f0 = static_cast<GLfloat>(x);
		command.f1 = static_cast<GLfloat>(y);
		command.f2 = static_cast<GLfloat>(z);
		record(command);
		if (recordingOnly())
			return;
	}

	applyScalef(static_cast<GLfloat>(x), static_cast<GLfloat>(y), static_cast<GLfloat>(z));
}

void Context::ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	nextSequence();
	traceCall(callSequence, "glOrtho", left, right, bottom, top, zNear, zFar);

	if (left == right || bottom == top || zNear == zFar)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->ortho(left, right, bottom, top, zNear, zFar);

	if (compilingListName != 0)
	{
		DisplayList &list = displayLists[compilingListName];
		ListCommand command;
		command.op = ListOp::Ortho;
		command.aux = static_cast<int>(list.doubles.size());
		list.doubles.push_back(left);
		list.doubles.push_back(right);
		list.doubles.push_back(bottom);
		list.doubles.push_back(top);
		list.doubles.push_back(zNear);
		list.doubles.push_back(zFar);
		record(command);
		if (recordingOnly())
			return;
	}

	matrixStack(matrixModeValue).multiply(orthographic(left, right, bottom, top, zNear, zFar));
}

void Context::frustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
	nextSequence();
	traceCall(callSequence, "glFrustum", left, right, bottom, top, zNear, zFar);

	if (zNear <= 0.0 || zFar <= 0.0 || left == right || bottom == top || zNear == zFar)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->frustum(left, right, bottom, top, zNear, zFar);

	if (compilingListName != 0)
	{
		DisplayList &list = displayLists[compilingListName];
		ListCommand command;
		command.op = ListOp::Frustum;
		command.aux = static_cast<int>(list.doubles.size());
		list.doubles.push_back(left);
		list.doubles.push_back(right);
		list.doubles.push_back(bottom);
		list.doubles.push_back(top);
		list.doubles.push_back(zNear);
		list.doubles.push_back(zFar);
		record(command);
		if (recordingOnly())
			return;
	}

	matrixStack(matrixModeValue).multiply(frustumMatrix(left, right, bottom, top, zNear, zFar));
}

// ---------------------------------------------------------------------------
// Enables
// ---------------------------------------------------------------------------

int Context::lightIndex(GLenum light) const
{
	if (light >= GL_LIGHT0 && light < GL_LIGHT0 + MAX_LIGHTS)
		return static_cast<int>(light - GL_LIGHT0);
	return -1;
}

bool *Context::enableSlot(GLenum cap)
{
	switch (cap)
	{
		case GL_TEXTURE_2D:
			return &capTexture2D;
		case GL_DEPTH_TEST:
			return &capDepthTest;
		case GL_ALPHA_TEST:
			return &capAlphaTest;
		case GL_BLEND:
			return &capBlend;
		case GL_CULL_FACE:
			return &capCullFace;
		case GL_FOG:
			return &capFog;
		case GL_LIGHTING:
			return &capLighting;
		case GL_COLOR_MATERIAL:
			return &capColorMaterial;
		case GL_RESCALE_NORMAL:
			return &capRescaleNormal;
		case GL_NORMALIZE:
			return &capNormalize;
		case GL_COLOR_LOGIC_OP:
			return &capColorLogicOp;
		case GL_POLYGON_OFFSET_FILL:
			return &capPolygonOffsetFill;
		case GL_SCISSOR_TEST:
			return &capScissorTest;
		case GL_STENCIL_TEST:
			return &capStencilTest;
		case GL_LINE_SMOOTH:
			return &capLineSmooth;
		case GL_DITHER:
			return &capDither;
		default:
			break;
	}

	const int index = lightIndex(cap);
	if (index >= 0)
		return &lights[index].enabled;
	return nullptr;
}

const bool *Context::enableSlot(GLenum cap) const
{
	return const_cast<Context *>(this)->enableSlot(cap);
}

bool Context::validateEnableCap(GLenum cap)
{
	if (enableSlot(cap) != nullptr)
		return true;
	// The supported enable set is the one current main uses. Anything else is
	// rejected rather than silently ignored, so a new renderer path shows up as
	// a GL error instead of a missing effect.
	setError(GL_INVALID_ENUM);
	return false;
}

bool Context::isEnabled(GLenum cap) const
{
	const bool *slot = enableSlot(cap);
	return slot != nullptr && *slot;
}

void Context::applyEnable(GLenum cap, bool value)
{
	bool *slot = enableSlot(cap);
	if (slot != nullptr)
		*slot = value;
}

void Context::enable(GLenum cap)
{
	nextSequence();
	traceCall(callSequence, "glEnable", cap);

	if (!validateEnableCap(cap))
		return;

	if (activeSink != nullptr)
		activeSink->enable(cap);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Enable;
		command.u0 = cap;
		record(command);
		if (recordingOnly())
			return;
	}

	applyEnable(cap, true);
}

void Context::disable(GLenum cap)
{
	nextSequence();
	traceCall(callSequence, "glDisable", cap);

	if (!validateEnableCap(cap))
		return;

	if (activeSink != nullptr)
		activeSink->disable(cap);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Disable;
		command.u0 = cap;
		record(command);
		if (recordingOnly())
			return;
	}

	applyEnable(cap, false);
}

// ---------------------------------------------------------------------------
// Fragment and raster state
// ---------------------------------------------------------------------------

static bool isBlendFactor(GLenum factor)
{
	switch (factor)
	{
		case GL_ZERO:
		case GL_ONE:
		case GL_SRC_COLOR:
		case GL_ONE_MINUS_SRC_COLOR:
		case GL_SRC_ALPHA:
		case GL_ONE_MINUS_SRC_ALPHA:
		case GL_DST_ALPHA:
		case GL_ONE_MINUS_DST_ALPHA:
		case GL_DST_COLOR:
		case GL_ONE_MINUS_DST_COLOR:
		case GL_SRC_ALPHA_SATURATE:
			return true;
		default:
			return false;
	}
}

static bool isCompareFunc(GLenum func)
{
	switch (func)
	{
		case GL_NEVER:
		case GL_LESS:
		case GL_EQUAL:
		case GL_LEQUAL:
		case GL_GREATER:
		case GL_NOTEQUAL:
		case GL_GEQUAL:
		case GL_ALWAYS:
			return true;
		default:
			return false;
	}
}

static bool isLogicOp(GLenum opcode)
{
	return opcode >= GL_CLEAR && opcode <= GL_SET;
}

void Context::blendFunc(GLenum sfactor, GLenum dfactor)
{
	nextSequence();
	traceCall(callSequence, "glBlendFunc", sfactor, dfactor);

	// GL_SRC_ALPHA_SATURATE is a source-only factor.
	if (!isBlendFactor(sfactor) || !isBlendFactor(dfactor) || dfactor == GL_SRC_ALPHA_SATURATE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->blendFunc(sfactor, dfactor);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::BlendFunc;
		command.u0 = sfactor;
		command.u1 = dfactor;
		record(command);
		if (recordingOnly())
			return;
	}

	blendSrc = sfactor;
	blendDst = dfactor;
}

void Context::alphaFunc(GLenum func, GLclampf ref)
{
	nextSequence();
	traceCall(callSequence, "glAlphaFunc", func, ref);

	if (!isCompareFunc(func))
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->alphaFunc(func, ref);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::AlphaFunc;
		command.u0 = func;
		command.f0 = ref;
		record(command);
		if (recordingOnly())
			return;
	}

	alphaFuncValue = func;
	// The reference value is clamped when it is set, not when it is used.
	alphaRefValue = ref < 0.0f ? 0.0f : (ref > 1.0f ? 1.0f : ref);
}

void Context::depthFunc(GLenum func)
{
	nextSequence();
	traceCall(callSequence, "glDepthFunc", func);

	if (!isCompareFunc(func))
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->depthFunc(func);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::DepthFunc;
		command.u0 = func;
		record(command);
		if (recordingOnly())
			return;
	}

	depthFuncValue = func;
}

void Context::depthMask(GLboolean flag)
{
	nextSequence();
	traceCall(callSequence, "glDepthMask", flag);

	if (activeSink != nullptr)
		activeSink->depthMask(flag);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::DepthMask;
		command.i0 = flag != 0 ? 1 : 0;
		record(command);
		if (recordingOnly())
			return;
	}

	depthMaskValue = flag != 0;
}

void Context::colorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
	nextSequence();
	traceCall(callSequence, "glColorMask", red, green, blue, alpha);

	if (activeSink != nullptr)
		activeSink->colorMask(red, green, blue, alpha);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::ColorMask;
		command.i0 = red != 0 ? 1 : 0;
		command.i1 = green != 0 ? 1 : 0;
		command.i2 = blue != 0 ? 1 : 0;
		command.i3 = alpha != 0 ? 1 : 0;
		record(command);
		if (recordingOnly())
			return;
	}

	colorMaskValue[0] = red != 0;
	colorMaskValue[1] = green != 0;
	colorMaskValue[2] = blue != 0;
	colorMaskValue[3] = alpha != 0;
}

void Context::cullFace(GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glCullFace", mode);

	if (mode != GL_FRONT && mode != GL_BACK && mode != GL_FRONT_AND_BACK)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->cullFace(mode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::CullFace;
		command.u0 = mode;
		record(command);
		if (recordingOnly())
			return;
	}

	cullFaceValue = mode;
}

void Context::shadeModel(GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glShadeModel", mode);

	if (mode != GL_FLAT && mode != GL_SMOOTH)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->shadeModel(mode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::ShadeModel;
		command.u0 = mode;
		record(command);
		if (recordingOnly())
			return;
	}

	shadeModelValue = mode;
}

void Context::logicOp(GLenum opcode)
{
	nextSequence();
	traceCall(callSequence, "glLogicOp", opcode);

	if (!isLogicOp(opcode))
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->logicOp(opcode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::LogicOp;
		command.u0 = opcode;
		record(command);
		if (recordingOnly())
			return;
	}

	logicOpValue = opcode;
}

void Context::lineWidth(GLfloat width)
{
	nextSequence();
	traceCall(callSequence, "glLineWidth", width);

	if (width <= 0.0f)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->lineWidth(width);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::LineWidth;
		command.f0 = width;
		record(command);
		if (recordingOnly())
			return;
	}

	lineWidthState = width;
}

void Context::polygonOffset(GLfloat factor, GLfloat units)
{
	nextSequence();
	traceCall(callSequence, "glPolygonOffset", factor, units);

	if (activeSink != nullptr)
		activeSink->polygonOffset(factor, units);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::PolygonOffset;
		command.f0 = factor;
		command.f1 = units;
		record(command);
		if (recordingOnly())
			return;
	}

	// The canonical GL factor/units are kept exactly as given. Deriving a
	// backend depth bias from them is a backend decision and must be
	// calibrated per depth format.
	polygonOffsetFactorValue = factor;
	polygonOffsetUnitsValue = units;
}

void Context::viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
	nextSequence();
	traceCall(callSequence, "glViewport", x, y, width, height);

	if (width < 0 || height < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->viewport(x, y, width, height);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Viewport;
		command.i0 = x;
		command.i1 = y;
		command.i2 = width;
		command.i3 = height;
		record(command);
		if (recordingOnly())
			return;
	}

	viewportValue[0] = x;
	viewportValue[1] = y;
	viewportValue[2] = width;
	viewportValue[3] = height;
}

void Context::pixelStorei(GLenum pname, GLint param)
{
	nextSequence();
	traceCall(callSequence, "glPixelStorei", pname, param);

	// Only the alignment controls are in the supported profile. Row length,
	// skip and swap controls would silently corrupt uploads if accepted
	// without an implementation.
	if (pname != GL_PACK_ALIGNMENT && pname != GL_UNPACK_ALIGNMENT)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (param != 1 && param != 2 && param != 4 && param != 8)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	// glPixelStore is never compiled into a display list; it executes
	// immediately even while a list is being built.
	if (activeSink != nullptr)
		activeSink->pixelStorei(pname, param);

	if (pname == GL_PACK_ALIGNMENT)
		packAlignmentValue = param;
	else
		unpackAlignmentValue = param;
}

// ---------------------------------------------------------------------------
// Current attributes
// ---------------------------------------------------------------------------

void Context::applyColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	current.r = red;
	current.g = green;
	current.b = blue;
	current.a = alpha;
	colorIndeterminate = false;

	// While GL_COLOR_MATERIAL is enabled an explicit current-colour change
	// also writes the selected material properties, and those values persist
	// after color material is disabled again.
	if (capColorMaterial)
	{
		const bool front = colorMaterialFaceValue == GL_FRONT || colorMaterialFaceValue == GL_FRONT_AND_BACK;
		const bool back = colorMaterialFaceValue == GL_BACK || colorMaterialFaceValue == GL_FRONT_AND_BACK;
		const float value[4] = { red, green, blue, alpha };

		MaterialState *targets[2] = { front ? &materialFront : nullptr, back ? &materialBack : nullptr };
		for (int i = 0; i < 2; i++)
		{
			MaterialState *material = targets[i];
			if (material == nullptr)
				continue;

			switch (colorMaterialModeValue)
			{
				case GL_AMBIENT:
					std::memcpy(material->ambient, value, sizeof(value));
					break;
				case GL_DIFFUSE:
					std::memcpy(material->diffuse, value, sizeof(value));
					break;
				case GL_AMBIENT_AND_DIFFUSE:
					std::memcpy(material->ambient, value, sizeof(value));
					std::memcpy(material->diffuse, value, sizeof(value));
					break;
				case GL_SPECULAR:
					std::memcpy(material->specular, value, sizeof(value));
					break;
				case GL_EMISSION:
					std::memcpy(material->emission, value, sizeof(value));
					break;
				default:
					break;
			}
		}
	}
}

// glColor3f sets alpha to 1.0. It does not keep the previous alpha, which is
// why LevelRenderer's sky colours cannot inherit a stale translucency.
void Context::color3f(GLfloat red, GLfloat green, GLfloat blue)
{
	nextSequence();
	traceCall(callSequence, "glColor3f", red, green, blue);

	if (activeSink != nullptr)
		activeSink->color3f(red, green, blue);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Color4f;
		command.f0 = red;
		command.f1 = green;
		command.f2 = blue;
		command.f3 = 1.0f;
		record(command);
		if (recordingOnly())
			return;
	}

	applyColor4f(red, green, blue, 1.0f);
}

void Context::color4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
	nextSequence();
	traceCall(callSequence, "glColor4f", red, green, blue, alpha);

	if (activeSink != nullptr)
		activeSink->color4f(red, green, blue, alpha);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Color4f;
		command.f0 = red;
		command.f1 = green;
		command.f2 = blue;
		command.f3 = alpha;
		record(command);
		if (recordingOnly())
			return;
	}

	applyColor4f(red, green, blue, alpha);
}

void Context::applyNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	current.nx = nx;
	current.ny = ny;
	current.nz = nz;
	normalIndeterminate = false;
}

void Context::normal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
	nextSequence();
	traceCall(callSequence, "glNormal3f", nx, ny, nz);

	if (activeSink != nullptr)
		activeSink->normal3f(nx, ny, nz);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Normal3f;
		command.f0 = nx;
		command.f1 = ny;
		command.f2 = nz;
		record(command);
		if (recordingOnly())
			return;
	}

	applyNormal3f(nx, ny, nz);
}

// Byte normals are normalized fixed point: (2c+1)/255 by the OpenGL 1.1 rule,
// not c/127. Tesselator packs its array normals the same way, so the immediate
// and array paths agree.
void Context::normal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
	nextSequence();
	traceCall(callSequence, "glNormal3b", nx, ny, nz);

	if (activeSink != nullptr)
		activeSink->normal3b(nx, ny, nz);

	const GLfloat x = normalizeSignedByte(nx);
	const GLfloat y = normalizeSignedByte(ny);
	const GLfloat z = normalizeSignedByte(nz);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Normal3f;
		command.f0 = x;
		command.f1 = y;
		command.f2 = z;
		record(command);
		if (recordingOnly())
			return;
	}

	applyNormal3f(x, y, z);
}

// ---------------------------------------------------------------------------
// Fog
// ---------------------------------------------------------------------------

void Context::applyFogf(GLenum pname, GLfloat param)
{
	switch (pname)
	{
		case GL_FOG_DENSITY:
			fogDensityValue = param;
			break;
		case GL_FOG_START:
			fogStartValue = param;
			break;
		case GL_FOG_END:
			fogEndValue = param;
			break;
		case GL_FOG_MODE:
			fogModeValue = static_cast<GLenum>(param);
			break;
		default:
			break;
	}
}

void Context::fogf(GLenum pname, GLfloat param)
{
	nextSequence();
	traceCall(callSequence, "glFogf", pname, param);

	if (pname == GL_FOG_MODE)
	{
		const GLenum mode = static_cast<GLenum>(param);
		if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2)
		{
			setError(GL_INVALID_ENUM);
			return;
		}
	}
	else if (pname != GL_FOG_DENSITY && pname != GL_FOG_START && pname != GL_FOG_END)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (pname == GL_FOG_DENSITY && param < 0.0f)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->fogf(pname, param);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Fogf;
		command.u0 = pname;
		command.f0 = param;
		record(command);
		if (recordingOnly())
			return;
	}

	applyFogf(pname, param);
}

void Context::fogfv(GLenum pname, const GLfloat *params)
{
	nextSequence();
	if (params == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (pname == GL_FOG_COLOR)
		traceCall(callSequence, "glFogfv", pname, params[0], params[1], params[2], params[3]);
	else
		traceCall(callSequence, "glFogfv", pname, params[0]);

	if (pname != GL_FOG_COLOR && pname != GL_FOG_DENSITY && pname != GL_FOG_START &&
		pname != GL_FOG_END && pname != GL_FOG_MODE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->fogfv(pname, params);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Fogfv;
		command.u0 = pname;
		command.f0 = params[0];
		if (pname == GL_FOG_COLOR)
		{
			command.f1 = params[1];
			command.f2 = params[2];
			command.f3 = params[3];
		}
		record(command);
		if (recordingOnly())
			return;
	}

	if (pname == GL_FOG_COLOR)
	{
		for (int i = 0; i < 4; i++)
			fogColorValue[i] = params[i];
	}
	else
	{
		applyFogf(pname, params[0]);
	}
}

void Context::applyFogi(GLenum pname, GLint param)
{
	switch (pname)
	{
		case GL_FOG_MODE:
			fogModeValue = static_cast<GLenum>(param);
			break;
		case GL_FOG_DISTANCE_MODE_NV:
			fogDistanceModeValue = static_cast<GLenum>(param);
			break;
		default:
			applyFogf(pname, static_cast<GLfloat>(param));
			break;
	}
}

void Context::fogi(GLenum pname, GLint param)
{
	nextSequence();
	traceCall(callSequence, "glFogi", pname, param);

	if (pname == GL_FOG_MODE)
	{
		const GLenum mode = static_cast<GLenum>(param);
		if (mode != GL_LINEAR && mode != GL_EXP && mode != GL_EXP2)
		{
			setError(GL_INVALID_ENUM);
			return;
		}
	}
	else if (pname == GL_FOG_DISTANCE_MODE_NV)
	{
		const GLenum mode = static_cast<GLenum>(param);
		// GL_NV_fog_distance. Radial distance is a deliberate, separate concept
		// from the standard eye-plane fog coordinate.
		if (mode != GL_EYE_RADIAL_NV && mode != GL_EYE_PLANE && mode != GL_EYE_PLANE_ABSOLUTE_NV)
		{
			setError(GL_INVALID_ENUM);
			return;
		}
	}
	else if (pname != GL_FOG_DENSITY && pname != GL_FOG_START && pname != GL_FOG_END)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->fogi(pname, param);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Fogi;
		command.u0 = pname;
		command.i0 = param;
		record(command);
		if (recordingOnly())
			return;
	}

	applyFogi(pname, param);
}

// ---------------------------------------------------------------------------
// Lighting
// ---------------------------------------------------------------------------

void Context::applyLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	const int index = lightIndex(light);
	if (index < 0)
		return;

	LightState &state = lights[index];
	switch (pname)
	{
		case GL_AMBIENT:
			for (int i = 0; i < 4; i++)
				state.ambient[i] = params[i];
			break;
		case GL_DIFFUSE:
			for (int i = 0; i < 4; i++)
				state.diffuse[i] = params[i];
			break;
		case GL_SPECULAR:
			for (int i = 0; i < 4; i++)
				state.specular[i] = params[i];
			break;
		case GL_POSITION:
		{
			// Call-time transform: the position is multiplied by the model-view
			// matrix in force right now, not by whatever is current at draw
			// time. Deferring this is one of the classic emulation bugs.
			const Mat4 &m = modelViewStack.top();
			const float x = params[0], y = params[1], z = params[2], w = params[3];
			state.positionEye[0] = m.m[0] * x + m.m[4] * y + m.m[8] * z + m.m[12] * w;
			state.positionEye[1] = m.m[1] * x + m.m[5] * y + m.m[9] * z + m.m[13] * w;
			state.positionEye[2] = m.m[2] * x + m.m[6] * y + m.m[10] * z + m.m[14] * w;
			state.positionEye[3] = m.m[3] * x + m.m[7] * y + m.m[11] * z + m.m[15] * w;
			break;
		}
		case GL_SPOT_DIRECTION:
		{
			// Transformed by the upper-left 3x3 of the model-view matrix at call
			// time. Alpha never sets a spot direction, so this path has no
			// oracle capture yet; see docs/portable/semantic-notes.md.
			const Mat4 &m = modelViewStack.top();
			const float x = params[0], y = params[1], z = params[2];
			state.spotDirectionEye[0] = m.m[0] * x + m.m[4] * y + m.m[8] * z;
			state.spotDirectionEye[1] = m.m[1] * x + m.m[5] * y + m.m[9] * z;
			state.spotDirectionEye[2] = m.m[2] * x + m.m[6] * y + m.m[10] * z;
			break;
		}
		case GL_SPOT_EXPONENT:
			state.spotExponent = params[0];
			break;
		case GL_SPOT_CUTOFF:
			state.spotCutoff = params[0];
			break;
		case GL_CONSTANT_ATTENUATION:
			state.constantAttenuation = params[0];
			break;
		case GL_LINEAR_ATTENUATION:
			state.linearAttenuation = params[0];
			break;
		case GL_QUADRATIC_ATTENUATION:
			state.quadraticAttenuation = params[0];
			break;
		default:
			break;
	}
}

static int lightParamCount(GLenum pname)
{
	switch (pname)
	{
		case GL_AMBIENT:
		case GL_DIFFUSE:
		case GL_SPECULAR:
		case GL_POSITION:
			return 4;
		case GL_SPOT_DIRECTION:
			return 3;
		case GL_SPOT_EXPONENT:
		case GL_SPOT_CUTOFF:
		case GL_CONSTANT_ATTENUATION:
		case GL_LINEAR_ATTENUATION:
		case GL_QUADRATIC_ATTENUATION:
			return 1;
		default:
			return 0;
	}
}

void Context::lightfv(GLenum light, GLenum pname, const GLfloat *params)
{
	nextSequence();

	const int count = lightParamCount(pname);
	if (params == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (count == 4)
		traceCall(callSequence, "glLightfv", light, pname, params[0], params[1], params[2], params[3]);
	else if (count == 3)
		traceCall(callSequence, "glLightfv", light, pname, params[0], params[1], params[2]);
	else
		traceCall(callSequence, "glLightfv", light, pname, params[0]);

	if (lightIndex(light) < 0 || count == 0)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->lightfv(light, pname, params);

	if (compilingListName != 0)
	{
		DisplayList &list = displayLists[compilingListName];
		ListCommand command;
		command.op = ListOp::Lightfv;
		command.u0 = light;
		command.u1 = pname;
		command.aux = static_cast<int>(list.doubles.size());
		for (int i = 0; i < count; i++)
			list.doubles.push_back(params[i]);
		record(command);
		if (recordingOnly())
			return;
	}

	applyLightfv(light, pname, params);
}

void Context::lightModelfv(GLenum pname, const GLfloat *params)
{
	nextSequence();
	if (params == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	traceCall(callSequence, "glLightModelfv", pname, params[0]);

	if (pname != GL_LIGHT_MODEL_AMBIENT && pname != GL_LIGHT_MODEL_TWO_SIDE &&
		pname != GL_LIGHT_MODEL_LOCAL_VIEWER)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->lightModelfv(pname, params);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::LightModelfv;
		command.u0 = pname;
		command.f0 = params[0];
		if (pname == GL_LIGHT_MODEL_AMBIENT)
		{
			command.f1 = params[1];
			command.f2 = params[2];
			command.f3 = params[3];
		}
		record(command);
		if (recordingOnly())
			return;
	}

	if (pname == GL_LIGHT_MODEL_AMBIENT)
	{
		for (int i = 0; i < 4; i++)
			lightModelAmbientValue[i] = params[i];
	}
}

void Context::colorMaterial(GLenum face, GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glColorMaterial", face, mode);

	if (face != GL_FRONT && face != GL_BACK && face != GL_FRONT_AND_BACK)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (mode != GL_AMBIENT && mode != GL_DIFFUSE && mode != GL_AMBIENT_AND_DIFFUSE &&
		mode != GL_SPECULAR && mode != GL_EMISSION)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->colorMaterial(face, mode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::ColorMaterial;
		command.u0 = face;
		command.u1 = mode;
		record(command);
		if (recordingOnly())
			return;
	}

	colorMaterialFaceValue = face;
	colorMaterialModeValue = mode;
}

// ---------------------------------------------------------------------------
// Textures
// ---------------------------------------------------------------------------

bool Context::isTextureName(GLuint name) const
{
	if (name == 0)
		return false;
	if (textureObjects.find(name) != textureObjects.end())
		return true;
	for (GLuint reserved : reservedTextureNames)
	{
		if (reserved == name)
			return true;
	}
	return false;
}

bool Context::isTextureObject(GLuint name) const
{
	return name != 0 && textureObjects.find(name) != textureObjects.end();
}

const TextureObject *Context::texture(GLuint name) const
{
	if (name == 0)
		return &textureZero;
	auto it = textureObjects.find(name);
	return it == textureObjects.end() ? nullptr : &it->second;
}

void Context::genTextures(GLsizei n, GLuint *textures)
{
	nextSequence();
	traceCall(callSequence, "glGenTextures", n);

	if (n < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (n == 0 || textures == nullptr)
		return;

	// Names come from the backend so that the ids the game hands back to GL are
	// the backend's own. Generation reserves names; an object only exists once
	// the name is bound.
	if (activeSink != nullptr)
	{
		activeSink->genTextures(n, textures);
	}
	else
	{
		for (GLsizei i = 0; i < n; i++)
			textures[i] = 0;
		return;
	}

	for (GLsizei i = 0; i < n; i++)
	{
		if (textures[i] != 0 && !isTextureName(textures[i]))
			reservedTextureNames.push_back(textures[i]);
	}
}

void Context::deleteTextures(GLsizei n, const GLuint *textures)
{
	nextSequence();
	traceCall(callSequence, "glDeleteTextures", n);

	if (n < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (n == 0 || textures == nullptr)
		return;

	if (activeSink != nullptr)
		activeSink->deleteTextures(n, textures);

	for (GLsizei i = 0; i < n; i++)
	{
		const GLuint name = textures[i];
		// Deleting zero or a name that is not a texture is silently ignored.
		if (name == 0)
			continue;

		textureObjects.erase(name);
		for (std::size_t r = 0; r < reservedTextureNames.size(); r++)
		{
			if (reservedTextureNames[r] == name)
			{
				reservedTextureNames.erase(reservedTextureNames.begin() + static_cast<std::ptrdiff_t>(r));
				break;
			}
		}

		// Deleting the bound texture reverts the binding to object zero.
		if (textureBinding == name)
			textureBinding = 0;
	}
}

void Context::applyBindTexture(GLuint texture)
{
	textureBinding = texture;
	if (texture == 0)
		return;

	// Binding an unused nonzero name creates the object with legacy defaults.
	if (textureObjects.find(texture) == textureObjects.end())
	{
		textureObjects.emplace(texture, TextureObject());
		for (std::size_t r = 0; r < reservedTextureNames.size(); r++)
		{
			if (reservedTextureNames[r] == texture)
			{
				reservedTextureNames.erase(reservedTextureNames.begin() + static_cast<std::ptrdiff_t>(r));
				break;
			}
		}
	}
}

void Context::bindTexture(GLenum target, GLuint texture)
{
	nextSequence();
	traceCall(callSequence, "glBindTexture", target, texture);

	if (target != GL_TEXTURE_2D)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->bindTexture(target, texture);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::BindTexture;
		command.u0 = target;
		command.u1 = texture;
		record(command);
		if (recordingOnly())
			return;
	}

	applyBindTexture(texture);
}

void Context::applyTexParameteri(GLenum pname, GLint param)
{
	TextureObject *object = textureBinding == 0 ? &textureZero : nullptr;
	if (object == nullptr)
	{
		auto it = textureObjects.find(textureBinding);
		if (it == textureObjects.end())
		{
			applyBindTexture(textureBinding);
			it = textureObjects.find(textureBinding);
			if (it == textureObjects.end())
				return;
		}
		object = &it->second;
	}

	switch (pname)
	{
		case GL_TEXTURE_MIN_FILTER:
			object->minFilter = static_cast<GLenum>(param);
			break;
		case GL_TEXTURE_MAG_FILTER:
			object->magFilter = static_cast<GLenum>(param);
			break;
		case GL_TEXTURE_WRAP_S:
			object->wrapS = static_cast<GLenum>(param);
			break;
		case GL_TEXTURE_WRAP_T:
			object->wrapT = static_cast<GLenum>(param);
			break;
		default:
			break;
	}
}

void Context::texParameteri(GLenum target, GLenum pname, GLint param)
{
	nextSequence();
	traceCall(callSequence, "glTexParameteri", target, pname, param);

	if (target != GL_TEXTURE_2D)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	const GLenum value = static_cast<GLenum>(param);
	switch (pname)
	{
		case GL_TEXTURE_MIN_FILTER:
			if (value != GL_NEAREST && value != GL_LINEAR && value != GL_NEAREST_MIPMAP_NEAREST &&
				value != GL_LINEAR_MIPMAP_NEAREST && value != GL_NEAREST_MIPMAP_LINEAR &&
				value != GL_LINEAR_MIPMAP_LINEAR)
			{
				setError(GL_INVALID_ENUM);
				return;
			}
			break;
		case GL_TEXTURE_MAG_FILTER:
			if (value != GL_NEAREST && value != GL_LINEAR)
			{
				setError(GL_INVALID_ENUM);
				return;
			}
			break;
		case GL_TEXTURE_WRAP_S:
		case GL_TEXTURE_WRAP_T:
			// Legacy GL_CLAMP is kept as GL_CLAMP. It is not GL_CLAMP_TO_EDGE:
			// linear filtering blends against the border colour at the edge.
			if (value != GL_CLAMP && value != GL_REPEAT && value != GL_CLAMP_TO_EDGE)
			{
				setError(GL_INVALID_ENUM);
				return;
			}
			break;
		default:
			setError(GL_INVALID_ENUM);
			return;
	}

	if (activeSink != nullptr)
		activeSink->texParameteri(target, pname, param);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::TexParameteri;
		command.u0 = target;
		command.u1 = pname;
		command.i0 = param;
		record(command);
		if (recordingOnly())
			return;
	}

	applyTexParameteri(pname, param);
}

static int pixelComponents(GLenum format)
{
	switch (format)
	{
		case GL_RGBA:
		case GL_BGRA_EXT:
			return 4;
		case GL_RGB:
		case GL_BGR_EXT:
			return 3;
		case GL_LUMINANCE_ALPHA:
			return 2;
		case GL_ALPHA:
		case GL_LUMINANCE:
			return 1;
		default:
			return 0;
	}
}

void Context::texImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height,
	GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
	nextSequence();
	if (traceEnabled())
	{
		const int components = pixelComponents(format);
		const std::size_t rowBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(components);
		const std::size_t stride = (rowBytes + static_cast<std::size_t>(unpackAlignmentValue) - 1) /
			static_cast<std::size_t>(unpackAlignmentValue) * static_cast<std::size_t>(unpackAlignmentValue);
		traceCall(callSequence, "glTexImage2D", target, level, internalformat, width, height, border, format, type,
			traceHash(pixels, pixels == nullptr ? 0 : stride * static_cast<std::size_t>(height)));
	}

	if (target != GL_TEXTURE_2D)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (type != GL_UNSIGNED_BYTE || pixelComponents(format) == 0)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (level < 0 || level >= TextureObject::MAX_LEVELS || border != 0 || width < 0 || height < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
	{
		activeSink->texImage2D(target, level, internalformat, width, height, border, format, type, pixels);
	}

	// Pixel commands are list-compilable, but no path in current main defines a
	// texture image inside a display list. Recording one would require copying
	// the image at compile time; until a call site needs it, refuse instead of
	// capturing a pointer that may dangle.
	if (compilingListName != 0)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	TextureObject *object = textureBinding == 0 ? &textureZero : nullptr;
	if (object == nullptr)
	{
		applyBindTexture(textureBinding);
		auto it = textureObjects.find(textureBinding);
		if (it == textureObjects.end())
			return;
		object = &it->second;
	}

	TextureLevel &target_level = object->levels[level];
	target_level.width = width;
	target_level.height = height;
	target_level.internalFormat = internalformat;
	target_level.defined = true;

	if (activeSink != nullptr)
	{
		ResolvedTextureUpload command;
		command.sequence = callSequence;
		command.texture = textureBinding;
		command.level = level;
		command.width = width;
		command.height = height;
		command.internalFormat = internalformat;
		command.sourceFormat = format;
		command.sourceType = type;
		command.unpackAlignment = unpackAlignmentValue;
		command.pixels = pixels;
		activeSink->resolvedTextureUpload(command);
	}
}

void Context::texSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
	GLenum format, GLenum type, const GLvoid *pixels)
{
	nextSequence();
	if (traceEnabled())
	{
		const int components = pixelComponents(format);
		const std::size_t rowBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(components);
		const std::size_t stride = (rowBytes + static_cast<std::size_t>(unpackAlignmentValue) - 1) /
			static_cast<std::size_t>(unpackAlignmentValue) * static_cast<std::size_t>(unpackAlignmentValue);
		traceCall(callSequence, "glTexSubImage2D", target, level, xoffset, yoffset, width, height, format, type,
			traceHash(pixels, pixels == nullptr ? 0 : stride * static_cast<std::size_t>(height)));
	}

	if (target != GL_TEXTURE_2D)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (type != GL_UNSIGNED_BYTE || pixelComponents(format) == 0)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (level < 0 || level >= TextureObject::MAX_LEVELS || width < 0 || height < 0 || xoffset < 0 || yoffset < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	const TextureObject *object = texture(textureBinding);
	if (object == nullptr || !object->levels[level].defined)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}
	if (xoffset + width > object->levels[level].width || yoffset + height > object->levels[level].height)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->texSubImage2D(target, level, xoffset, yoffset, width, height, format, type, pixels);

	if (compilingListName != 0)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
	{
		ResolvedTextureUpload command;
		command.sequence = callSequence;
		command.texture = textureBinding;
		command.subImage = true;
		command.level = level;
		command.x = xoffset;
		command.y = yoffset;
		command.width = width;
		command.height = height;
		command.internalFormat = object->levels[level].internalFormat;
		command.sourceFormat = format;
		command.sourceType = type;
		command.unpackAlignment = unpackAlignmentValue;
		command.pixels = pixels;
		activeSink->resolvedTextureUpload(command);
	}
}

// ---------------------------------------------------------------------------
// Client arrays and buffers
// ---------------------------------------------------------------------------

bool Context::isBufferName(GLuint name) const
{
	if (name == 0)
		return false;
	if (bufferObjects.find(name) != bufferObjects.end())
		return true;
	for (GLuint reserved : reservedBufferNames)
	{
		if (reserved == name)
			return true;
	}
	return false;
}

const BufferObject *Context::buffer(GLuint name) const
{
	auto it = bufferObjects.find(name);
	return it == bufferObjects.end() ? nullptr : &it->second;
}

void Context::genBuffersARB(GLsizei n, GLuint *buffers)
{
	nextSequence();
	traceCall(callSequence, "glGenBuffersARB", n);

	if (n < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (n == 0 || buffers == nullptr)
		return;

	if (activeSink == nullptr)
	{
		for (GLsizei i = 0; i < n; i++)
			buffers[i] = 0;
		return;
	}

	activeSink->genBuffersARB(n, buffers);
	for (GLsizei i = 0; i < n; i++)
	{
		if (buffers[i] != 0 && !isBufferName(buffers[i]))
			reservedBufferNames.push_back(buffers[i]);
	}
}

void Context::bindBufferARB(GLenum target, GLuint buffer)
{
	nextSequence();
	traceCall(callSequence, "glBindBufferARB", target, buffer);

	if (target != GL_ARRAY_BUFFER_ARB)
	{
		setError(GL_INVALID_ENUM);
		return;
	}

	if (activeSink != nullptr)
		activeSink->bindBufferARB(target, buffer);

	arrayBufferBinding = buffer;
	if (buffer != 0 && bufferObjects.find(buffer) == bufferObjects.end())
		bufferObjects.emplace(buffer, BufferObject());
}

void Context::bufferDataARB(GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage)
{
	nextSequence();
	traceCall(callSequence, "glBufferDataARB", target, static_cast<long long>(size), usage);

	if (target != GL_ARRAY_BUFFER_ARB)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (size < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (arrayBufferBinding == 0)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->bufferDataARB(target, size, data, usage);

	BufferObject &object = bufferObjects[arrayBufferBinding];
	object.size = size;
	object.usage = usage;
	// Buffer contents are owned immediately. A null pointer allocates
	// uninitialised storage without reading the caller's memory.
	object.data.assign(static_cast<std::size_t>(size), 0);
	if (data != nullptr && size > 0)
		std::memcpy(object.data.data(), data, static_cast<std::size_t>(size));
}

void Context::enableClientState(GLenum array)
{
	nextSequence();
	traceCall(callSequence, "glEnableClientState", array);

	ArrayState *state = nullptr;
	switch (array)
	{
		case GL_VERTEX_ARRAY:
			state = &vertexArrayState;
			break;
		case GL_COLOR_ARRAY:
			state = &colorArrayState;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			state = &texCoordArrayState;
			break;
		case GL_NORMAL_ARRAY:
			state = &normalArrayState;
			break;
		default:
			setError(GL_INVALID_ENUM);
			return;
	}

	// Client state is not compiled into display lists; it takes effect
	// immediately even while a list is being built.
	if (activeSink != nullptr)
		activeSink->enableClientState(array);

	state->enabled = true;
}

void Context::disableClientState(GLenum array)
{
	nextSequence();
	traceCall(callSequence, "glDisableClientState", array);

	ArrayState *state = nullptr;
	switch (array)
	{
		case GL_VERTEX_ARRAY:
			state = &vertexArrayState;
			break;
		case GL_COLOR_ARRAY:
			state = &colorArrayState;
			break;
		case GL_TEXTURE_COORD_ARRAY:
			state = &texCoordArrayState;
			break;
		case GL_NORMAL_ARRAY:
			state = &normalArrayState;
			break;
		default:
			setError(GL_INVALID_ENUM);
			return;
	}

	if (activeSink != nullptr)
		activeSink->disableClientState(array);

	state->enabled = false;
}

static bool isArrayType(GLenum type)
{
	switch (type)
	{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
		case GL_DOUBLE:
			return true;
		default:
			return false;
	}
}

static int arrayTypeSize(GLenum type)
{
	switch (type)
	{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
			return 1;
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
			return 2;
		case GL_INT:
		case GL_UNSIGNED_INT:
		case GL_FLOAT:
			return 4;
		case GL_DOUBLE:
			return 8;
		default:
			return 0;
	}
}

void Context::vertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	nextSequence();
	traceCall(callSequence, "glVertexPointer", size, type, stride, pointer);

	if (size < 2 || size > 4)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (stride < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->vertexPointer(size, type, stride, pointer);

	vertexArrayState.size = size;
	vertexArrayState.type = type;
	vertexArrayState.stride = stride;
	vertexArrayState.pointer = pointer;
	vertexArrayState.buffer = arrayBufferBinding;
}

void Context::texCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	nextSequence();
	traceCall(callSequence, "glTexCoordPointer", size, type, stride, pointer);

	if (size < 1 || size > 4)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (stride < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->texCoordPointer(size, type, stride, pointer);

	texCoordArrayState.size = size;
	texCoordArrayState.type = type;
	texCoordArrayState.stride = stride;
	texCoordArrayState.pointer = pointer;
	texCoordArrayState.buffer = arrayBufferBinding;
}

void Context::colorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
	nextSequence();
	traceCall(callSequence, "glColorPointer", size, type, stride, pointer);

	if (size != 3 && size != 4)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (!isArrayType(type))
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (stride < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->colorPointer(size, type, stride, pointer);

	colorArrayState.size = size;
	colorArrayState.type = type;
	colorArrayState.stride = stride;
	colorArrayState.pointer = pointer;
	colorArrayState.buffer = arrayBufferBinding;
}

void Context::normalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
	nextSequence();
	traceCall(callSequence, "glNormalPointer", type, stride, pointer);

	if (type != GL_BYTE && type != GL_SHORT && type != GL_INT && type != GL_FLOAT && type != GL_DOUBLE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (stride < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->normalPointer(type, stride, pointer);

	normalArrayState.size = 3;
	normalArrayState.type = type;
	normalArrayState.stride = stride;
	normalArrayState.pointer = pointer;
	normalArrayState.buffer = arrayBufferBinding;
}

// Reads one component of an array element and converts it with the OpenGL 1.1
// rules. Unnormalized targets (position, texture coordinate) take the raw
// value; colours and normals are normalized fixed point.
static float readComponent(const unsigned char *base, GLenum type, int index, bool normalized)
{
	switch (type)
	{
		case GL_BYTE:
		{
			signed char value;
			std::memcpy(&value, base + index, 1);
			return normalized ? normalizeSignedByte(value) : static_cast<float>(value);
		}
		case GL_UNSIGNED_BYTE:
		{
			unsigned char value = base[index];
			return normalized ? normalizeUnsignedByte(value) : static_cast<float>(value);
		}
		case GL_SHORT:
		{
			short value;
			std::memcpy(&value, base + index * 2, 2);
			return normalized ? static_cast<float>(2 * static_cast<int>(value) + 1) / 65535.0f
							  : static_cast<float>(value);
		}
		case GL_UNSIGNED_SHORT:
		{
			unsigned short value;
			std::memcpy(&value, base + index * 2, 2);
			return normalized ? static_cast<float>(value) / 65535.0f : static_cast<float>(value);
		}
		case GL_INT:
		{
			int value;
			std::memcpy(&value, base + index * 4, 4);
			return normalized ? static_cast<float>(static_cast<double>(2.0 * value + 1.0) / 4294967295.0)
							  : static_cast<float>(value);
		}
		case GL_UNSIGNED_INT:
		{
			unsigned int value;
			std::memcpy(&value, base + index * 4, 4);
			return normalized ? static_cast<float>(static_cast<double>(value) / 4294967295.0)
							  : static_cast<float>(value);
		}
		case GL_FLOAT:
		{
			float value;
			std::memcpy(&value, base + index * 4, 4);
			return value;
		}
		case GL_DOUBLE:
		{
			double value;
			std::memcpy(&value, base + index * 8, 8);
			return static_cast<float>(value);
		}
		default:
			return 0.0f;
	}
}

// Validates the enabled arrays and, when captureVertices is set, converts the
// referenced elements into canonical vertices. Validation runs either way so
// the GL error behaviour does not depend on which backend is installed.
bool Context::decodeArrays(GLenum mode, GLint first, GLsizei count, Geometry &out, bool captureVertices)
{
	out.clear();
	out.mode = mode;
	out.vertexCount = count;

	if (!vertexArrayState.enabled)
	{
		out.vertexCount = 0;
		return true;
	}

	struct Source
	{
		const ArrayState *state;
		const unsigned char *base;
		int stride;
		int components;
	};

	const ArrayState *states[4] = { &vertexArrayState, &texCoordArrayState, &colorArrayState, &normalArrayState };
	Source sources[4];

	for (int i = 0; i < 4; i++)
	{
		const ArrayState *state = states[i];
		sources[i].state = state;
		sources[i].base = nullptr;
		sources[i].components = state->size;
		if (!state->enabled)
			continue;

		const int elementSize = arrayTypeSize(state->type);
		if (elementSize == 0)
		{
			setError(GL_INVALID_ENUM);
			return false;
		}

		// Stride zero means tightly packed.
		sources[i].stride = state->stride != 0 ? state->stride : elementSize * state->size;

		if (state->buffer != 0)
		{
			const BufferObject *object = buffer(state->buffer);
			if (object == nullptr)
			{
				setError(GL_INVALID_OPERATION);
				return false;
			}
			const std::ptrdiff_t offset = reinterpret_cast<std::ptrdiff_t>(state->pointer);
			const std::ptrdiff_t last = offset +
				static_cast<std::ptrdiff_t>(first + count - 1) * sources[i].stride +
				static_cast<std::ptrdiff_t>(elementSize) * state->size;
			if (offset < 0 || last > object->size)
			{
				setError(GL_INVALID_OPERATION);
				return false;
			}
			sources[i].base = object->data.data() + offset;
		}
		else
		{
			if (state->pointer == nullptr)
			{
				setError(GL_INVALID_OPERATION);
				return false;
			}
			sources[i].base = static_cast<const unsigned char *>(state->pointer);
		}
	}

	out.hasTexCoord = texCoordArrayState.enabled;
	out.hasColor = colorArrayState.enabled;
	out.hasNormal = normalArrayState.enabled;

	if (!captureVertices)
		return true;

	out.vertices.resize(static_cast<std::size_t>(count));

	for (GLsizei v = 0; v < count; v++)
	{
		Vertex &vertex = out.vertices[static_cast<std::size_t>(v)];
		const int element = first + v;

		{
			const Source &source = sources[0];
			const unsigned char *p = source.base + static_cast<std::ptrdiff_t>(element) * source.stride;
			vertex.x = readComponent(p, vertexArrayState.type, 0, false);
			vertex.y = readComponent(p, vertexArrayState.type, 1, false);
			vertex.z = vertexArrayState.size >= 3 ? readComponent(p, vertexArrayState.type, 2, false) : 0.0f;
		}

		if (texCoordArrayState.enabled)
		{
			const Source &source = sources[1];
			const unsigned char *p = source.base + static_cast<std::ptrdiff_t>(element) * source.stride;
			vertex.s = readComponent(p, texCoordArrayState.type, 0, false);
			vertex.t = texCoordArrayState.size >= 2 ? readComponent(p, texCoordArrayState.type, 1, false) : 0.0f;
		}

		if (colorArrayState.enabled)
		{
			const Source &source = sources[2];
			const unsigned char *p = source.base + static_cast<std::ptrdiff_t>(element) * source.stride;
			const bool normalized = colorArrayState.type != GL_FLOAT && colorArrayState.type != GL_DOUBLE;
			vertex.r = readComponent(p, colorArrayState.type, 0, normalized);
			vertex.g = readComponent(p, colorArrayState.type, 1, normalized);
			vertex.b = readComponent(p, colorArrayState.type, 2, normalized);
			vertex.a = colorArrayState.size == 4 ? readComponent(p, colorArrayState.type, 3, normalized) : 1.0f;
		}

		if (normalArrayState.enabled)
		{
			const Source &source = sources[3];
			const unsigned char *p = source.base + static_cast<std::ptrdiff_t>(element) * source.stride;
			const bool normalized = normalArrayState.type != GL_FLOAT && normalArrayState.type != GL_DOUBLE;
			vertex.nx = readComponent(p, normalArrayState.type, 0, normalized);
			vertex.ny = readComponent(p, normalArrayState.type, 1, normalized);
			vertex.nz = readComponent(p, normalArrayState.type, 2, normalized);
		}
	}

	return true;
}

void Context::noteIndeterminateUse(const char *what)
{
	indeterminateUses++;
	// The three attributes are counted separately: the report has to say which
	// current value the renderer is leaning on, not just that it happens.
	if (what[0] == 'c')
		indeterminateColorUses++;
	else if (what[0] == 'n')
		indeterminateNormalUses++;
	else
		indeterminateTexCoordUses++;

	if (traceEnabled())
	{
		std::ostringstream out;
		out << "# indeterminate current " << what << " used at call " << traceSequence(callSequence);
		traceRawLine(out.str());
	}
}

void Context::executeGeometry(const Geometry &geometry)
{
	// An attribute the draw did not supply is read from the current state at
	// execution time, which is what lets a captured glyph list be recoloured by
	// a surrounding glColor4f. If that current value is indeterminate after an
	// array draw, the reliance is counted rather than papered over.
	if (!geometry.hasColor && colorIndeterminate)
		noteIndeterminateUse("color");
	if (!geometry.hasNormal && normalIndeterminate)
		noteIndeterminateUse("normal");
	if (!geometry.hasTexCoord && texCoordIndeterminate)
		noteIndeterminateUse("texture coordinate");

	draws++;

	// Only a backend that consumes resolved vertices pays for building them.
	// The native backend already drew this geometry itself, and a chunk display
	// list holds thousands of vertices that would otherwise be copied on every
	// frame it is called.
	if (activeSink == nullptr || !activeSink->wantsCanonicalGeometry())
		return;

	lastDrawGeometry = geometry;
	if (!geometry.hasColor)
	{
		for (Vertex &vertex : lastDrawGeometry.vertices)
		{
			vertex.r = current.r;
			vertex.g = current.g;
			vertex.b = current.b;
			vertex.a = current.a;
		}
	}
	if (!geometry.hasNormal)
	{
		for (Vertex &vertex : lastDrawGeometry.vertices)
		{
			vertex.nx = current.nx;
			vertex.ny = current.ny;
			vertex.nz = current.nz;
		}
	}
	if (!geometry.hasTexCoord)
	{
		for (Vertex &vertex : lastDrawGeometry.vertices)
		{
			vertex.s = current.s;
			vertex.t = current.t;
		}
	}

	canonicalizePrimitives(geometry.mode, geometry.vertexCount, lastDrawPrimitives);

	ResolvedDraw command;
	command.sequence = callSequence;
	command.geometry = &lastDrawGeometry;
	command.primitives = &lastDrawPrimitives;
	command.modelView = modelViewStack.top();
	command.projection = projectionStack.top();
	command.textureMatrix = textureStack.top();
	command.normal = normalMatrix(command.modelView);
	command.normalRescaleFactor = rescaleNormalFactor(command.modelView);

	command.enables.texture2D = capTexture2D;
	command.enables.depthTest = capDepthTest;
	command.enables.alphaTest = capAlphaTest;
	command.enables.blend = capBlend;
	command.enables.cullFace = capCullFace;
	command.enables.fog = capFog;
	command.enables.lighting = capLighting;
	command.enables.colorMaterial = capColorMaterial;
	command.enables.rescaleNormal = capRescaleNormal;
	command.enables.normalize = capNormalize;
	command.enables.colorLogicOp = capColorLogicOp;
	command.enables.polygonOffsetFill = capPolygonOffsetFill;
	command.enables.scissorTest = capScissorTest;
	command.enables.stencilTest = capStencilTest;
	command.enables.lineSmooth = capLineSmooth;
	command.enables.dither = capDither;

	command.pipeline.blendSource = blendSrc;
	command.pipeline.blendDestination = blendDst;
	command.pipeline.alphaFunction = alphaFuncValue;
	command.pipeline.alphaReference = alphaRefValue;
	command.pipeline.depthFunction = depthFuncValue;
	command.pipeline.depthWrite = depthMaskValue;
	command.pipeline.cullFaceMode = cullFaceValue;
	command.pipeline.frontFaceMode = frontFaceValue;
	command.pipeline.shadeModel = shadeModelValue;
	command.pipeline.logicOpcode = logicOpValue;
	command.pipeline.lineWidth = lineWidthState;
	command.pipeline.polygonOffsetFactor = polygonOffsetFactorValue;
	command.pipeline.polygonOffsetUnits = polygonOffsetUnitsValue;
	for (int i = 0; i < 4; i++)
	{
		command.pipeline.colorWrite[i] = colorMaskValue[i];
		command.pipeline.viewport[i] = viewportValue[i];
	}

	command.fog.mode = fogModeValue;
	command.fog.density = fogDensityValue;
	command.fog.start = fogStartValue;
	command.fog.end = fogEndValue;
	command.fog.distanceMode = fogDistanceModeValue;
	for (int i = 0; i < 4; i++)
		command.fog.color[i] = fogColorValue[i];

	for (int i = 0; i < MAX_LIGHTS; i++)
		command.lighting.lights[i] = lights[i];
	for (int i = 0; i < 4; i++)
		command.lighting.modelAmbient[i] = lightModelAmbientValue[i];
	command.lighting.colorMaterialFace = colorMaterialFaceValue;
	command.lighting.colorMaterialMode = colorMaterialModeValue;
	command.lighting.frontMaterial = materialFront;
	command.lighting.backMaterial = materialBack;

	command.texture.name = textureBinding;
	const TextureObject *object = texture(textureBinding);
	if (object != nullptr)
	{
		command.texture.minFilter = object->minFilter;
		command.texture.magFilter = object->magFilter;
		command.texture.wrapS = object->wrapS;
		command.texture.wrapT = object->wrapT;
		for (int i = 0; i < 4; i++)
			command.texture.borderColor[i] = object->borderColor[i];
		command.texture.level0Width = object->levels[0].width;
		command.texture.level0Height = object->levels[0].height;
		command.texture.level0InternalFormat = object->levels[0].internalFormat;
		command.texture.level0Defined = object->levels[0].defined;
		command.texture.complete = object->complete();
	}

	activeSink->resolvedDraw(command);
}

// OpenGL leaves the current colour undefined after an array draw whose colour
// array was enabled, and current main relies on whatever the driver leaves
// behind hundreds of thousands of times per session. Rather than guessing which
// convention to reproduce, validation mode asks the reference driver directly
// and counts the answers. See docs/portable/semantic-notes.md.
void Context::probePostArrayColor(const Vertex &before, const Geometry &drawn)
{
	if (!validate || activeSink == nullptr)
		return;

	GLfloat backend[4];
	if (!activeSink->queryFloatv(GL_CURRENT_COLOR, backend))
		return;

	if (backend[0] == before.r && backend[1] == before.g && backend[2] == before.b && backend[3] == before.a)
	{
		arrayColorPreserved++;
		return;
	}

	if (!drawn.vertices.empty())
	{
		const Vertex &last = drawn.vertices.back();
		if (backend[0] == last.r && backend[1] == last.g && backend[2] == last.b && backend[3] == last.a)
		{
			arrayColorLastElement++;
			return;
		}
	}

	arrayColorOther++;
}

void Context::drawArrays(GLenum mode, GLint first, GLsizei count)
{
	nextSequence();
	traceCall(callSequence, "glDrawArrays", mode, first, count);

	if (!isSupportedPrimitiveMode(mode))
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (first < 0 || count < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (immediateActive)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	// Validation mode needs the decoded vertices so the probe below can compare
	// the driver's post-draw current colour against the last array element.
	const bool wantGeometry = validate || (activeSink != nullptr && activeSink->wantsCanonicalGeometry());
	const Vertex colorBeforeDraw = current;
	const Geometry *drawnGeometry = nullptr;

	// The backend draws first, from the application's own arrays, exactly as the
	// game submitted them. Everything below is semantic bookkeeping.
	if (activeSink != nullptr)
		activeSink->drawArrays(mode, first, count);

	if (compilingListName != 0)
	{
		// A compiled array draw captures the vertex data now. Mutating or
		// freeing the source afterwards must not change what the list draws.
		DisplayList &list = displayLists[compilingListName];
		Geometry captured;
		if (!decodeArrays(mode, first, count, captured, wantGeometry))
			return;

		ListCommand command;
		command.op = ListOp::Geometry;
		command.aux = static_cast<int>(list.geometry.size());
		list.geometry.push_back(std::move(captured));
		record(command);

		if (recordingOnly())
			return;

		drawnGeometry = &list.geometry[static_cast<std::size_t>(command.aux)];
		executeGeometry(*drawnGeometry);
	}
	else
	{
		if (!decodeArrays(mode, first, count, decodeScratch, wantGeometry))
			return;
		if (!vertexArrayState.enabled)
			return;
		drawnGeometry = &decodeScratch;
		executeGeometry(decodeScratch);
	}

	// After an array draw, OpenGL leaves the current attributes whose arrays
	// were enabled undefined. Measured against the native driver (247849 draws,
	// see docs/portable/semantic-notes.md) it preserves the pre-draw values, and
	// so does this core: nothing is invented. The flags exist so validation can
	// count how often the renderer leans on that behaviour.
	if (colorArrayState.enabled)
	{
		probePostArrayColor(colorBeforeDraw, *drawnGeometry);
		colorIndeterminate = true;
	}
	if (normalArrayState.enabled)
		normalIndeterminate = true;
	if (texCoordArrayState.enabled)
		texCoordIndeterminate = true;
}

// ---------------------------------------------------------------------------
// Immediate mode
//
// Inside a display list these commands are compiled individually rather than
// captured as finished geometry. A vertex reads the current colour and normal,
// and those may be installed by the list's own compiled commands, which under
// GL_COMPILE have not run yet. Assembling at execution time is what makes the
// cached sign lists pick up their compiled glColor3f.
// ---------------------------------------------------------------------------

void Context::begin(GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glBegin", mode);

	if (!isSupportedPrimitiveMode(mode))
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (immediateActive)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->begin(mode);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Begin;
		command.u0 = mode;
		record(command);
		if (recordingOnly())
			return;
	}

	applyBegin(mode);
}

void Context::applyBegin(GLenum mode)
{
	immediateActive = true;
	immediate.clear();
	immediate.mode = mode;
	// Every immediate vertex carries a full snapshot of the current attributes,
	// so the draw always supplies all of them.
	immediate.hasColor = true;
	immediate.hasTexCoord = true;
	immediate.hasNormal = true;
}

void Context::applyEnd()
{
	if (!immediateActive)
		return;
	immediateActive = false;
	if (immediate.vertices.empty())
		return;

	immediate.vertexCount = static_cast<int>(immediate.vertices.size());
	executeGeometry(immediate);
}

void Context::applyVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	if (!immediateActive)
		return;

	if (colorIndeterminate)
		noteIndeterminateUse("color");
	if (normalIndeterminate)
		noteIndeterminateUse("normal");
	if (texCoordIndeterminate)
		noteIndeterminateUse("texture coordinate");

	Vertex vertex = current;
	vertex.x = x;
	vertex.y = y;
	vertex.z = z;
	immediate.vertices.push_back(vertex);
}

void Context::end()
{
	nextSequence();
	traceCall(callSequence, "glEnd");

	if (compilingListName == 0 && !immediateActive)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->end();

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::End;
		record(command);
		if (recordingOnly())
			return;
	}

	applyEnd();
}

void Context::vertex3f(GLfloat x, GLfloat y, GLfloat z)
{
	nextSequence();
	traceCall(callSequence, "glVertex3f", x, y, z);

	if (compilingListName == 0 && !immediateActive)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->vertex3f(x, y, z);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Vertex3f;
		command.f0 = x;
		command.f1 = y;
		command.f2 = z;
		record(command);
		if (recordingOnly())
			return;
	}

	applyVertex3f(x, y, z);
}

void Context::texCoord2f(GLfloat s, GLfloat t)
{
	nextSequence();
	traceCall(callSequence, "glTexCoord2f", s, t);

	if (activeSink != nullptr)
		activeSink->texCoord2f(s, t);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::TexCoord2f;
		command.f0 = s;
		command.f1 = t;
		record(command);
		if (recordingOnly())
			return;
	}

	current.s = s;
	current.t = t;
	texCoordIndeterminate = false;
}

// ---------------------------------------------------------------------------
// Display lists
// ---------------------------------------------------------------------------

const DisplayList *Context::displayList(GLuint name) const
{
	auto it = displayLists.find(name);
	return it == displayLists.end() ? nullptr : &it->second;
}

GLuint Context::genLists(GLsizei range)
{
	nextSequence();
	traceCall(callSequence, "glGenLists", range);

	if (range < 0)
	{
		setError(GL_INVALID_VALUE);
		return 0;
	}
	if (range == 0)
		return 0;

	GLuint base = 0;
	if (activeSink != nullptr)
		base = activeSink->genLists(range);
	else
		return 0;

	if (base == 0)
	{
		setError(GL_OUT_OF_MEMORY);
		return 0;
	}

	// Generation reserves a contiguous block of names but does not define the
	// lists, and an undefined list draws nothing. No entry is created here:
	// LevelRenderer asks for 786432 names at once, so materialising a
	// definition per name would cost tens of megabytes for lists that may never
	// be compiled.
	if (base + static_cast<GLuint>(range) > nextListName)
		nextListName = base + static_cast<GLuint>(range);
	return base;
}

void Context::newList(GLuint list, GLenum mode)
{
	nextSequence();
	traceCall(callSequence, "glNewList", list, mode);

	if (list == 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (mode != GL_COMPILE && mode != GL_COMPILE_AND_EXECUTE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (compilingListName != 0)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->newList(list, mode);

	DisplayList &definition = displayLists[list];
	definition.clear();

	compilingListName = list;
	compilingListModeValue = mode;
	traceListContext(list, mode);
}

void Context::endList()
{
	nextSequence();
	traceCall(callSequence, "glEndList");

	if (compilingListName == 0)
	{
		setError(GL_INVALID_OPERATION);
		return;
	}

	if (activeSink != nullptr)
		activeSink->endList();

	displayLists[compilingListName].defined = true;
	compilingListName = 0;
	traceListContext(0, 0);
}

void Context::executeList(GLuint name)
{
	auto it = displayLists.find(name);
	if (it == displayLists.end() || !it->second.defined)
		return;

	// The nesting limit is the OpenGL minimum. Exceeding it is reported rather
	// than allowed to recurse without bound.
	if (executionDepth >= 64)
	{
		setError(GL_STACK_OVERFLOW);
		return;
	}

	executionDepth++;
	// A reference is safe: glNewList and glDeleteLists are not list-compilable,
	// so nothing a list executes can redefine or delete the definition being
	// walked. Copying it would duplicate a chunk's captured vertices on every
	// call.
	const DisplayList &list = it->second;
	for (const ListCommand &command : list.commands)
	{
		switch (command.op)
		{
			case ListOp::MatrixMode:
				applyMatrixMode(command.u0);
				break;
			case ListOp::LoadIdentity:
				matrixStack(matrixModeValue).loadIdentity();
				break;
			case ListOp::PushMatrix:
				if (!matrixStack(matrixModeValue).push())
					setError(GL_STACK_OVERFLOW);
				break;
			case ListOp::PopMatrix:
				if (!matrixStack(matrixModeValue).pop())
					setError(GL_STACK_UNDERFLOW);
				break;
			case ListOp::Translatef:
				applyTranslatef(command.f0, command.f1, command.f2);
				break;
			case ListOp::Rotatef:
				applyRotatef(command.f0, command.f1, command.f2, command.f3);
				break;
			case ListOp::Scalef:
				applyScalef(command.f0, command.f1, command.f2);
				break;
			case ListOp::Ortho:
			{
				const double *d = &list.doubles[static_cast<std::size_t>(command.aux)];
				matrixStack(matrixModeValue).multiply(orthographic(d[0], d[1], d[2], d[3], d[4], d[5]));
				break;
			}
			case ListOp::Frustum:
			{
				const double *d = &list.doubles[static_cast<std::size_t>(command.aux)];
				matrixStack(matrixModeValue).multiply(frustumMatrix(d[0], d[1], d[2], d[3], d[4], d[5]));
				break;
			}
			case ListOp::Enable:
				applyEnable(command.u0, true);
				break;
			case ListOp::Disable:
				applyEnable(command.u0, false);
				break;
			case ListOp::BlendFunc:
				blendSrc = command.u0;
				blendDst = command.u1;
				break;
			case ListOp::AlphaFunc:
				alphaFuncValue = command.u0;
				alphaRefValue = command.f0 < 0.0f ? 0.0f : (command.f0 > 1.0f ? 1.0f : command.f0);
				break;
			case ListOp::DepthFunc:
				depthFuncValue = command.u0;
				break;
			case ListOp::DepthMask:
				depthMaskValue = command.i0 != 0;
				break;
			case ListOp::ColorMask:
				colorMaskValue[0] = command.i0 != 0;
				colorMaskValue[1] = command.i1 != 0;
				colorMaskValue[2] = command.i2 != 0;
				colorMaskValue[3] = command.i3 != 0;
				break;
			case ListOp::CullFace:
				cullFaceValue = command.u0;
				break;
			case ListOp::ShadeModel:
				shadeModelValue = command.u0;
				break;
			case ListOp::LogicOp:
				logicOpValue = command.u0;
				break;
			case ListOp::LineWidth:
				lineWidthState = command.f0;
				break;
			case ListOp::PolygonOffset:
				polygonOffsetFactorValue = command.f0;
				polygonOffsetUnitsValue = command.f1;
				break;
			case ListOp::Viewport:
				viewportValue[0] = command.i0;
				viewportValue[1] = command.i1;
				viewportValue[2] = command.i2;
				viewportValue[3] = command.i3;
				break;
			case ListOp::Color4f:
				applyColor4f(command.f0, command.f1, command.f2, command.f3);
				break;
			case ListOp::Normal3f:
				applyNormal3f(command.f0, command.f1, command.f2);
				break;
			case ListOp::Fogf:
				applyFogf(command.u0, command.f0);
				break;
			case ListOp::Fogfv:
				if (command.u0 == GL_FOG_COLOR)
				{
					fogColorValue[0] = command.f0;
					fogColorValue[1] = command.f1;
					fogColorValue[2] = command.f2;
					fogColorValue[3] = command.f3;
				}
				else
				{
					applyFogf(command.u0, command.f0);
				}
				break;
			case ListOp::Fogi:
				applyFogi(command.u0, command.i0);
				break;
			case ListOp::Lightfv:
			{
				const double *d = &list.doubles[static_cast<std::size_t>(command.aux)];
				float params[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
				const int count = lightParamCount(command.u1);
				for (int i = 0; i < count; i++)
					params[i] = static_cast<float>(d[i]);
				// The light position is transformed by the model-view matrix
				// current during list execution, not during compilation.
				applyLightfv(command.u0, command.u1, params);
				break;
			}
			case ListOp::LightModelfv:
				if (command.u0 == GL_LIGHT_MODEL_AMBIENT)
				{
					lightModelAmbientValue[0] = command.f0;
					lightModelAmbientValue[1] = command.f1;
					lightModelAmbientValue[2] = command.f2;
					lightModelAmbientValue[3] = command.f3;
				}
				break;
			case ListOp::ColorMaterial:
				colorMaterialFaceValue = command.u0;
				colorMaterialModeValue = command.u1;
				break;
			case ListOp::BindTexture:
				applyBindTexture(command.u1);
				break;
			case ListOp::TexParameteri:
				applyTexParameteri(command.u1, command.i0);
				break;
			case ListOp::Clear:
				emitResolvedClear(command.u0);
				break;
			case ListOp::ClearColor:
				clearColorState[0] = command.f0;
				clearColorState[1] = command.f1;
				clearColorState[2] = command.f2;
				clearColorState[3] = command.f3;
				break;
			case ListOp::ClearDepth:
				clearDepthState = command.f0;
				break;
			case ListOp::Begin:
				applyBegin(command.u0);
				break;
			case ListOp::End:
				applyEnd();
				break;
			case ListOp::Vertex3f:
				applyVertex3f(command.f0, command.f1, command.f2);
				break;
			case ListOp::TexCoord2f:
				current.s = command.f0;
				current.t = command.f1;
				texCoordIndeterminate = false;
				break;
			case ListOp::Geometry:
				executeGeometry(list.geometry[static_cast<std::size_t>(command.aux)]);
				break;
			case ListOp::CallList:
				executeList(command.u0);
				break;
			case ListOp::CallLists:
			{
				const std::size_t start = static_cast<std::size_t>(command.aux);
				for (int i = 0; i < command.i0; i++)
					executeList(list.names[start + static_cast<std::size_t>(i)]);
				break;
			}
		}
	}
	executionDepth--;
}

void Context::callList(GLuint list)
{
	nextSequence();
	traceCall(callSequence, "glCallList", list);

	if (activeSink != nullptr)
		activeSink->callList(list);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::CallList;
		command.u0 = list;
		record(command);
		if (recordingOnly())
			return;
	}

	// The backend executed its own copy of the list; the core replays its
	// recorded commands so its state tracks the same result.
	traceExecutionContext(list);
	executeList(list);
}

void Context::callLists(GLsizei n, GLenum type, const GLvoid *lists)
{
	nextSequence();
	traceCall(callSequence, "glCallLists", n, type);

	if (n < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	switch (type)
	{
		case GL_BYTE:
		case GL_UNSIGNED_BYTE:
		case GL_SHORT:
		case GL_UNSIGNED_SHORT:
		case GL_INT:
		case GL_UNSIGNED_INT:
			break;
		default:
			// The 2_BYTES/3_BYTES/4_BYTES encodings are outside the supported
			// profile; current main only uses GL_UNSIGNED_INT.
			setError(GL_INVALID_ENUM);
			return;
	}
	if (n > 0 && lists == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->callLists(n, type, lists);

	// Decode the element array into names now. It is caller memory, so it is
	// never retained.
	const unsigned char *bytes = static_cast<const unsigned char *>(lists);
	std::vector<GLuint> names(static_cast<std::size_t>(n));
	for (GLsizei i = 0; i < n; i++)
	{
		switch (type)
		{
			case GL_BYTE:
			{
				signed char value;
				std::memcpy(&value, bytes + i, 1);
				names[static_cast<std::size_t>(i)] = static_cast<GLuint>(value);
				break;
			}
			case GL_UNSIGNED_BYTE:
				names[static_cast<std::size_t>(i)] = bytes[i];
				break;
			case GL_SHORT:
			{
				short value;
				std::memcpy(&value, bytes + i * 2, 2);
				names[static_cast<std::size_t>(i)] = static_cast<GLuint>(value);
				break;
			}
			case GL_UNSIGNED_SHORT:
			{
				unsigned short value;
				std::memcpy(&value, bytes + i * 2, 2);
				names[static_cast<std::size_t>(i)] = value;
				break;
			}
			case GL_INT:
			{
				int value;
				std::memcpy(&value, bytes + i * 4, 4);
				names[static_cast<std::size_t>(i)] = static_cast<GLuint>(value);
				break;
			}
			default:
			{
				unsigned int value;
				std::memcpy(&value, bytes + i * 4, 4);
				names[static_cast<std::size_t>(i)] = value;
				break;
			}
		}
	}

	if (compilingListName != 0)
	{
		DisplayList &list = displayLists[compilingListName];
		ListCommand command;
		command.op = ListOp::CallLists;
		command.i0 = n;
		command.aux = static_cast<int>(list.names.size());
		for (GLsizei i = 0; i < n; i++)
			list.names.push_back(names[static_cast<std::size_t>(i)]);
		record(command);
		if (recordingOnly())
			return;
	}

	for (GLsizei i = 0; i < n; i++)
		executeList(names[static_cast<std::size_t>(i)]);
}

void Context::deleteLists(GLuint list, GLsizei range)
{
	nextSequence();
	traceCall(callSequence, "glDeleteLists", list, range);

	if (range < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (range == 0)
		return;

	if (activeSink != nullptr)
		activeSink->deleteLists(list, range);

	for (GLsizei i = 0; i < range; i++)
		displayLists.erase(list + static_cast<GLuint>(i));
}

// ---------------------------------------------------------------------------
// Framebuffer
// ---------------------------------------------------------------------------

void Context::emitResolvedClear(GLbitfield mask)
{
	if (activeSink == nullptr)
		return;

	ResolvedClear command;
	command.sequence = callSequence;
	command.mask = mask;
	command.depth = clearDepthState;
	command.depthWrite = depthMaskValue;
	command.scissorTest = capScissorTest;
	command.dither = capDither;
	for (int i = 0; i < 4; i++)
	{
		command.color[i] = clearColorState[i];
		command.colorWrite[i] = colorMaskValue[i];
	}
	activeSink->resolvedClear(command);
}

void Context::clear(GLbitfield mask)
{
	nextSequence();
	traceCall(callSequence, "glClear", mask);

	const GLbitfield known = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
	if ((mask & ~known) != 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	if (activeSink != nullptr)
		activeSink->clear(mask);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::Clear;
		command.u0 = mask;
		record(command);
		if (recordingOnly())
			return;
	}

	emitResolvedClear(mask);
}

void Context::clearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
	nextSequence();
	traceCall(callSequence, "glClearColor", red, green, blue, alpha);

	if (activeSink != nullptr)
		activeSink->clearColor(red, green, blue, alpha);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::ClearColor;
		command.f0 = red;
		command.f1 = green;
		command.f2 = blue;
		command.f3 = alpha;
		record(command);
		if (recordingOnly())
			return;
	}

	clearColorState[0] = red;
	clearColorState[1] = green;
	clearColorState[2] = blue;
	clearColorState[3] = alpha;
}

void Context::clearDepth(GLclampd depth)
{
	nextSequence();
	traceCall(callSequence, "glClearDepth", depth);

	if (activeSink != nullptr)
		activeSink->clearDepth(depth);

	if (compilingListName != 0)
	{
		ListCommand command;
		command.op = ListOp::ClearDepth;
		command.f0 = static_cast<float>(depth);
		record(command);
		if (recordingOnly())
			return;
	}

	clearDepthState = depth;
}

void Context::readPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
	nextSequence();
	traceCall(callSequence, "glReadPixels", x, y, width, height, format, type);

	if (pixelComponents(format) == 0 || type != GL_UNSIGNED_BYTE)
	{
		setError(GL_INVALID_ENUM);
		return;
	}
	if (width < 0 || height < 0)
	{
		setError(GL_INVALID_VALUE);
		return;
	}
	if (pixels == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	// Readback is never compiled into a display list; it executes immediately.
	if (activeSink != nullptr)
	{
		activeSink->readPixels(x, y, width, height, format, type, pixels);

		ResolvedReadback command;
		command.sequence = callSequence;
		command.x = x;
		command.y = y;
		command.width = width;
		command.height = height;
		command.format = format;
		command.type = type;
		command.packAlignment = packAlignmentValue;
		command.pixels = pixels;
		activeSink->resolvedReadback(command);
	}
}

void Context::finish()
{
	nextSequence();
	traceCall(callSequence, "glFinish");

	if (activeSink != nullptr)
		activeSink->finish();
}

// ---------------------------------------------------------------------------
// Queries
// ---------------------------------------------------------------------------

// Distance between two floats measured in representable steps. Exact equality
// is one comparison; everything else needs to say how far apart the values are,
// because a one-step difference in a driver-computed matrix element is not the
// same finding as a wrong matrix.
static long long floatUlpDistance(float a, float b)
{
	std::int32_t ai = 0;
	std::int32_t bi = 0;
	std::memcpy(&ai, &a, sizeof(ai));
	std::memcpy(&bi, &b, sizeof(bi));

	// Map the sign-magnitude encoding onto a monotonic integer ordering.
	if (ai < 0)
		ai = static_cast<std::int32_t>(0x80000000u) - ai;
	if (bi < 0)
		bi = static_cast<std::int32_t>(0x80000000u) - bi;

	const long long difference = static_cast<long long>(ai) - static_cast<long long>(bi);
	return difference < 0 ? -difference : difference;
}

void Context::checkQueryAgainstSink(GLenum pname, const GLfloat *values, int count)
{
	if (!validate || activeSink == nullptr)
		return;

	GLfloat backend[16];
	if (!activeSink->queryFloatv(pname, backend))
		return;

	validationCheckCount++;

	for (int i = 0; i < count; i++)
	{
		if (backend[i] == values[i])
			continue;

		const long long ulps = floatUlpDistance(values[i], backend[i]);
		if (ulps <= 1)
			validationOneUlp++;
		else
			validationBeyondOneUlp++;

		QueryDivergence *entry = nullptr;
		for (QueryDivergence &candidate : divergences)
		{
			if (candidate.pname == pname && candidate.component == i)
			{
				entry = &candidate;
				break;
			}
		}
		if (entry == nullptr)
		{
			QueryDivergence fresh;
			fresh.pname = pname;
			fresh.component = i;
			divergences.push_back(fresh);
			entry = &divergences.back();
		}

		if (ulps <= 1)
			entry->oneUlp++;
		else
			entry->beyondOneUlp++;
		if (ulps > entry->worstUlps)
		{
			entry->worstUlps = ulps;
			entry->worstCore = values[i];
			entry->worstBackend = backend[i];
		}

		if (ulps > 1 && firstValidationFailureMessage.empty())
		{
			// Raw bit patterns, not decimal: the interesting divergences print
			// identically in decimal.
			unsigned int coreBits = 0;
			unsigned int backendBits = 0;
			std::memcpy(&coreBits, &values[i], sizeof(coreBits));
			std::memcpy(&backendBits, &backend[i], sizeof(backendBits));

			std::ostringstream out;
			out << "glGetFloatv(0x" << std::hex << pname << ") component " << std::dec << i
				<< " differs by " << ulps << " ulp: core=" << values[i] << " (0x" << std::hex << coreBits
				<< ") backend=" << std::dec << backend[i] << " (0x" << std::hex << backendBits << std::dec
				<< ") at call " << callSequence;
			firstValidationFailureMessage = out.str();
			// Reported as it happens. A silent counter would let a parity break
			// sit unnoticed for a whole session.
			std::cerr << "legacygl: state divergence: " << firstValidationFailureMessage << '\n';
		}
	}
}

std::string Context::validationReport() const
{
	std::ostringstream out;
	out << "legacygl validation: " << validationCheckCount << " queries compared against the backend, "
		<< validationOneUlp << " one-ulp and " << validationBeyondOneUlp << " larger component divergences";
	out << '\n';

	for (const QueryDivergence &entry : divergences)
	{
		out << "  query 0x" << std::hex << entry.pname << std::dec << " component " << entry.component
			<< ": " << entry.oneUlp << " one-ulp, " << entry.beyondOneUlp << " larger, worst "
			<< entry.worstUlps << " ulp (core " << entry.worstCore << " vs backend " << entry.worstBackend
			<< ")\n";
	}

	if (indeterminateUses != 0)
	{
		out << "  indeterminate current attribute used " << indeterminateUses << " times: "
			<< indeterminateColorUses << " colour, " << indeterminateNormalUses << " normal, "
			<< indeterminateTexCoordUses << " texture coordinate\n";
	}

	const long long probes = arrayColorPreserved + arrayColorLastElement + arrayColorOther;
	if (probes != 0)
	{
		out << "  post-array current colour in the backend: " << arrayColorPreserved
			<< " preserved the pre-draw value, " << arrayColorLastElement
			<< " took the last array element, " << arrayColorOther << " were neither\n";
	}
	return out.str();
}

void Context::getFloatv(GLenum pname, GLfloat *params)
{
	nextSequence();
	traceCall(callSequence, "glGetFloatv", pname);

	if (params == nullptr)
	{
		setError(GL_INVALID_VALUE);
		return;
	}

	int count = 0;
	switch (pname)
	{
		case GL_MODELVIEW_MATRIX:
			std::memcpy(params, modelViewStack.top().m, sizeof(float) * 16);
			count = 16;
			break;
		case GL_PROJECTION_MATRIX:
			std::memcpy(params, projectionStack.top().m, sizeof(float) * 16);
			count = 16;
			break;
		case GL_TEXTURE_MATRIX:
			std::memcpy(params, textureStack.top().m, sizeof(float) * 16);
			count = 16;
			break;
		case GL_CURRENT_COLOR:
			if (colorIndeterminate)
				noteIndeterminateUse("color");
			params[0] = current.r;
			params[1] = current.g;
			params[2] = current.b;
			params[3] = current.a;
			count = 4;
			break;
		case GL_CURRENT_NORMAL:
			if (normalIndeterminate)
				noteIndeterminateUse("normal");
			params[0] = current.nx;
			params[1] = current.ny;
			params[2] = current.nz;
			count = 3;
			break;
		case GL_CURRENT_TEXTURE_COORDS:
			if (texCoordIndeterminate)
				noteIndeterminateUse("texture coordinate");
			params[0] = current.s;
			params[1] = current.t;
			params[2] = 0.0f;
			params[3] = 1.0f;
			count = 4;
			break;
		case GL_FOG_COLOR:
			std::memcpy(params, fogColorValue, sizeof(float) * 4);
			count = 4;
			break;
		case GL_FOG_DENSITY:
			params[0] = fogDensityValue;
			count = 1;
			break;
		case GL_FOG_START:
			params[0] = fogStartValue;
			count = 1;
			break;
		case GL_FOG_END:
			params[0] = fogEndValue;
			count = 1;
			break;
		case GL_FOG_MODE:
			params[0] = static_cast<float>(fogModeValue);
			count = 1;
			break;
		case GL_LIGHT_MODEL_AMBIENT:
			std::memcpy(params, lightModelAmbientValue, sizeof(float) * 4);
			count = 4;
			break;
		case GL_COLOR_CLEAR_VALUE:
			std::memcpy(params, clearColorState, sizeof(float) * 4);
			count = 4;
			break;
		case GL_DEPTH_CLEAR_VALUE:
			params[0] = static_cast<float>(clearDepthState);
			count = 1;
			break;
		case GL_LINE_WIDTH:
			params[0] = lineWidthState;
			count = 1;
			break;
		case GL_POLYGON_OFFSET_FACTOR:
			params[0] = polygonOffsetFactorValue;
			count = 1;
			break;
		case GL_POLYGON_OFFSET_UNITS:
			params[0] = polygonOffsetUnitsValue;
			count = 1;
			break;
		case GL_ALPHA_TEST_REF:
			params[0] = alphaRefValue;
			count = 1;
			break;
		case GL_VIEWPORT:
			for (int i = 0; i < 4; i++)
				params[i] = static_cast<float>(viewportValue[i]);
			count = 4;
			break;
		default:
			setError(GL_INVALID_ENUM);
			return;
	}

	checkQueryAgainstSink(pname, params, count);
}

}
