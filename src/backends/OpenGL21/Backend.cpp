// OpenGL 2.1 compatibility backend with display-list geometry residency.
//
// LegacyGL remains the only semantic state machine. This backend applies each
// resolved command through compatibility OpenGL, streams transient canonical
// vertices, and retains immutable display-list geometry in GL_STATIC_DRAW
// VBOs. NativeGL remains a separate raw-call oracle.

#include <glad/glad.h>

#include "backends/Backend.h"
#include "backends/OpenGL/Context.h"
#include "legacygl/Sink.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <vector>

namespace legacygl
{

struct GL21ResidentGeometryEntry
{
	GLuint vertexArray = 0;
	GLuint vertexBuffer = 0;
	unsigned int mode = 0;
	std::size_t vertexCount = 0;
	bool hasColor = false;
	bool hasNormal = false;
	bool hasTexCoord = false;
};

// Exact-bit comparison keeps redundant-state suppression from treating two
// distinct float encodings, including the signed zeroes and NaNs the game can
// submit, as one applied value.
static std::uint32_t gl21FloatBits(float value)
{
	std::uint32_t result = 0;
	std::memcpy(&result, &value, sizeof(result));
	return result;
}

// Which optional attributes a draw supplies. The stream vertex array only has
// to be respecified when this changes, which is what keeps an attribute-aware
// layout from costing a full pointer setup on every transient draw.
static unsigned int gl21AttributeMask(const Geometry &geometry)
{
	return (geometry.hasColor ? 1u : 0u) | (geometry.hasNormal ? 2u : 0u) |
		(geometry.hasTexCoord ? 4u : 0u);
}

class GL21LogicalNameAllocator
{
public:
	unsigned int allocate()
	{
		if (names.size() == static_cast<std::size_t>(
			std::numeric_limits<unsigned int>::max()))
		{
			return 0;
		}
		while (names.find(nextName) != names.end())
			advance();
		const unsigned int name = nextName;
		names.insert(name);
		advance();
		return name;
	}

	void reserve(unsigned int name)
	{
		if (name != 0)
			names.insert(name);
	}

	void release(unsigned int name)
	{
		if (name != 0)
			names.erase(name);
	}

	void reset()
	{
		names.clear();
		nextName = 1;
	}

private:
	void advance()
	{
		nextName = nextName == std::numeric_limits<unsigned int>::max() ?
			1 : nextName + 1;
	}

	std::set<unsigned int> names;
	unsigned int nextName = 1;
};


class OpenGL21Sink final : public Sink
{
public:
	void initialize()
	{
		if (initialized)
			return;
		if (!GLAD_GL_VERSION_2_1 || glad_glGenBuffers == nullptr ||
			glad_glBindBuffer == nullptr || glad_glBufferData == nullptr ||
			glad_glDeleteBuffers == nullptr)
		{
			throw std::runtime_error("OpenGL 2.1 backend requires VBO support");
		}
		glGenBuffers(1, &streamVertexBuffer);
		residentGeometry.reserve(8192);
		vaoSupported = (GLAD_GL_VERSION_3_0 ||
			openglbackend::hasCapability("GL_ARB_vertex_array_object")) &&
			glad_glGenVertexArrays != nullptr && glad_glBindVertexArray != nullptr &&
			glad_glDeleteVertexArrays != nullptr;
		if (vaoSupported)
		{
			glGenVertexArrays(1, &streamVertexArray);
		}
		glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, lineWidthRange);
		std::fprintf(stdout,
			"LegacyGL gl21: capability_report profile=compatibility"
			" transient_geometry=GL_STREAM_DRAW chunk_residency=GL_STATIC_DRAW"
			" vertex_arrays=%s texture_storage=driver-legacy"
			" line_width=%s legacy_dither=driver\n",
			vaoSupported ? "vao" : "compatibility-state",
			(lineWidthRange[0] <= 2.0f && lineWidthRange[1] >= 2.0f) ?
				"native" : "width1-fallback");
		initialized = true;
	}

	void shutdown()
	{
		if (!initialized)
			return;
		std::fprintf(stdout,
			"gl21: shutdown, resident cache hits=%llu, misses=%llu,"
			" resident bytes=%llu, entries=%zu, explicit releases=%llu\n",
			static_cast<unsigned long long>(residentCacheHits),
			static_cast<unsigned long long>(residentCacheMisses),
			static_cast<unsigned long long>(residentGeometryBytes),
			residentGeometry.size(),
			static_cast<unsigned long long>(residentGeometryReleases));
		std::fprintf(stdout,
			"gl21: state sync requests=%llu"
			" matrices=%llu/%llu lighting=%llu/%llu material_restores=%llu"
			" enables=%llu/%llu pipeline=%llu/%llu fog=%llu/%llu"
			" texture=%llu/%llu\n",
			static_cast<unsigned long long>(stateSyncRequests),
			static_cast<unsigned long long>(matrixEmits),
			static_cast<unsigned long long>(matrixSuppressions),
			static_cast<unsigned long long>(lightingEmits),
			static_cast<unsigned long long>(lightingSuppressions),
			static_cast<unsigned long long>(materialRestores),
			static_cast<unsigned long long>(enableEmits),
			static_cast<unsigned long long>(enableSuppressions),
			static_cast<unsigned long long>(pipelineEmits),
			static_cast<unsigned long long>(pipelineSuppressions),
			static_cast<unsigned long long>(fogEmits),
			static_cast<unsigned long long>(fogSuppressions),
			static_cast<unsigned long long>(textureEmits),
			static_cast<unsigned long long>(textureSuppressions));
		materialRestores = 0;
		releaseAllCanonicalGeometry();
		materialStateDirty = false;
		for (const auto &entry : textures)
			glDeleteTextures(1, &entry.second);
		textures.clear();
		textureNames.reset();
		bufferNames.reset();
		nextListName = 1;
		if (streamVertexArray != 0)
			glDeleteVertexArrays(1, &streamVertexArray);
		streamVertexArray = 0;
		if (streamVertexBuffer != 0)
			glDeleteBuffers(1, &streamVertexBuffer);
		streamVertexBuffer = 0;
		residentCacheHits = 0;
		residentCacheMisses = 0;
		residentGeometryBytes = 0;
		residentGeometryReleases = 0;
		stateSyncRequests = 0;
		matrixEmits = 0; matrixSuppressions = 0;
		lightingEmits = 0; lightingSuppressions = 0;
		enableEmits = 0; enableSuppressions = 0;
		pipelineEmits = 0; pipelineSuppressions = 0;
		fogEmits = 0; fogSuppressions = 0;
		textureEmits = 0; textureSuppressions = 0;
		matricesValid = false;
		lightingValid = false;
		enablesValid = false;
		pipelineValid = false;
		fogValid = false;
		textureValid = false;
		vaoSupported = false;
		initialized = false;
	}

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

	void genTextures(int n, unsigned int *names) override
	{
		if (n <= 0 || names == nullptr)
			return;
		for (int i = 0; i < n; i++)
			names[i] = textureNames.allocate();
	}
	void deleteTextures(int n, const unsigned int *names) override
	{
		if (n <= 0 || names == nullptr)
			return;
		for (int i = 0; i < n; i++)
		{
			const unsigned int name = names[i];
			auto found = textures.find(name);
			if (found != textures.end())
			{
				glDeleteTextures(1, &found->second);
				textureValid = false;
				textures.erase(found);
			}
			textureNames.release(name);
		}
	}
	void bindTexture(unsigned int, unsigned int texture) override
	{
		textureNames.reserve(texture);
	}
	void texParameteri(unsigned int, unsigned int, int) override {}
	void texImage2D(unsigned int, int, int, int, int, int, unsigned int,
		unsigned int, const void *) override {}
	void texSubImage2D(unsigned int, int, int, int, int, int, unsigned int,
		unsigned int, const void *) override {}

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

	void genBuffersARB(int n, unsigned int *names) override
	{
		if (n <= 0 || names == nullptr)
			return;
		for (int i = 0; i < n; i++)
			names[i] = bufferNames.allocate();
	}
	void bindBufferARB(unsigned int, unsigned int buffer) override
	{
		bufferNames.reserve(buffer);
	}
	void bufferDataARB(unsigned int, std::ptrdiff_t, const void *, unsigned int) override {}

	unsigned int genLists(int range) override
	{
		if (range <= 0 || static_cast<std::uint64_t>(range) >
			std::numeric_limits<unsigned int>::max() - nextListName)
		{
			return 0;
		}
		const unsigned int base = static_cast<unsigned int>(nextListName);
		nextListName += static_cast<std::uint64_t>(range);
		return base;
	}
	void newList(unsigned int list, unsigned int) override
	{
		if (list != 0)
			nextListName = std::max(nextListName, static_cast<std::uint64_t>(list) + 1);
	}
	void endList() override {}
	void callList(unsigned int) override {}
	void callLists(int, unsigned int, const void *) override {}
	void deleteLists(unsigned int, int) override {}

	void clear(unsigned int) override {}
	void clearColor(float, float, float, float) override {}
	void clearDepth(double) override {}
	void readPixels(int, int, int, int, unsigned int, unsigned int, void *) override {}
	void finish() override { glFinish(); }

	bool wantsCanonicalGeometry() const override { return true; }

	void releaseCanonicalGeometry(std::uint64_t residencyId) override
	{
		for (auto entry = residentGeometry.begin(); entry != residentGeometry.end();)
		{
			if (entry->first != residencyId)
			{
				++entry;
				continue;
			}
			residentGeometryBytes -= entry->second.vertexCount * sizeof(Vertex);
			residentGeometryReleases++;
			if (entry->second.vertexArray != 0)
				glDeleteVertexArrays(1, &entry->second.vertexArray);
			glDeleteBuffers(1, &entry->second.vertexBuffer);
			entry = residentGeometry.erase(entry);
		}
	}

	void resolvedDraw(const ResolvedDraw &command) override
	{
		if (command.geometry == nullptr || command.geometry->vertices.empty())
			return;
		initialize();
		applyResolvedState(command);

		const Geometry &geometry = *command.geometry;
		GLuint vertexBuffer = streamVertexBuffer;
		GLuint vertexArray = streamVertexArray;
		const std::size_t vertexCount = geometry.vertices.size();
		bool configureArrays = true;
		if (command.geometryResidencyId == 0)
		{
			glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
			glBufferData(GL_ARRAY_BUFFER,
				static_cast<GLsizeiptr>(vertexCount * sizeof(Vertex)),
				geometry.vertices.data(), GL_STREAM_DRAW);
		}
		else
		{
			auto found = residentGeometry.find(command.geometryResidencyId);
			if (found == residentGeometry.end())
			{
				residentCacheMisses++;
				GL21ResidentGeometryEntry entry;
				entry.mode = geometry.mode;
				entry.vertexCount = vertexCount;
				entry.hasColor = geometry.hasColor;
				entry.hasNormal = geometry.hasNormal;
				entry.hasTexCoord = geometry.hasTexCoord;
				glGenBuffers(1, &entry.vertexBuffer);
				glBindBuffer(GL_ARRAY_BUFFER, entry.vertexBuffer);
				glBufferData(GL_ARRAY_BUFFER,
					static_cast<GLsizeiptr>(vertexCount * sizeof(Vertex)),
					geometry.vertices.data(), GL_STATIC_DRAW);
				if (vaoSupported)
				{
					glGenVertexArrays(1, &entry.vertexArray);
					glBindVertexArray(entry.vertexArray);
					bindCanonicalArrays(entry.vertexBuffer, geometry);
					glBindVertexArray(0);
				}
				found = residentGeometry.emplace(command.geometryResidencyId, entry).first;
				residentGeometryBytes += vertexCount * sizeof(Vertex);
			}
			else
			{
				residentCacheHits++;
				if (found->second.mode != geometry.mode ||
					found->second.vertexCount != vertexCount ||
					found->second.hasColor != geometry.hasColor ||
					found->second.hasNormal != geometry.hasNormal ||
					found->second.hasTexCoord != geometry.hasTexCoord)
				{
					throw std::runtime_error(
						"OpenGL 2.1 resident geometry identity changed while cached");
				}
			}
			vertexBuffer = found->second.vertexBuffer;
			vertexArray = found->second.vertexArray;
			configureArrays = false;
		}

		applyCurrentAttributes(command);
		if (vaoSupported)
		{
			glBindVertexArray(vertexArray);
			if (configureArrays)
			{
				const unsigned int attributeMask = gl21AttributeMask(geometry);
				if (!streamLayoutValid || streamAttributeMask != attributeMask)
				{
					bindCanonicalArrays(vertexBuffer, geometry);
					streamAttributeMask = attributeMask;
					streamLayoutValid = true;
				}
			}
			glDrawArrays(geometry.mode, 0, static_cast<GLsizei>(vertexCount));
			glBindVertexArray(0);
		}
		else
		{
			bindCanonicalArrays(vertexBuffer, geometry);
			glDrawArrays(geometry.mode, 0, static_cast<GLsizei>(vertexCount));
			disableCanonicalArrays();
		}
		materialStateDirty = command.enables.colorMaterial;
	}

	void resolvedClear(const ResolvedClear &command) override
	{
		glColorMask(command.colorWrite[0], command.colorWrite[1],
			command.colorWrite[2], command.colorWrite[3]);
		glDepthMask(command.depthWrite);
		setEnabled(GL_SCISSOR_TEST, command.scissorTest);
		setEnabled(GL_DITHER, command.dither);
		glClearColor(command.color[0], command.color[1], command.color[2], command.color[3]);
		glClearDepth(command.depth);
		glClear(command.mask);
		enablesValid = false;
		pipelineValid = false;
	}

	void resolvedTextureUpload(const ResolvedTextureUpload &command) override
	{
		glBindTexture(GL_TEXTURE_2D, physicalTexture(command.texture));
		glPixelStorei(GL_UNPACK_ALIGNMENT, command.unpackAlignment);
		if (command.subImage)
		{
			glTexSubImage2D(GL_TEXTURE_2D, command.level, command.x, command.y,
				command.width, command.height, command.sourceFormat, command.sourceType,
				command.pixels);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, command.level, command.internalFormat,
				command.width, command.height, 0, command.sourceFormat, command.sourceType,
				command.pixels);
		}
		textureValid = false;
	}

	void resolvedReadback(const ResolvedReadback &command) override
	{
		glPixelStorei(GL_PACK_ALIGNMENT, command.packAlignment);
		glReadPixels(command.x, command.y, command.width, command.height,
			command.format, command.type, command.pixels);
	}

private:
	static void setEnabled(unsigned int capability, bool enabled)
	{
		if (enabled)
			glEnable(capability);
		else
			glDisable(capability);
	}

	static void applyMaterial(unsigned int face, const MaterialState &material)
	{
		glMaterialfv(face, GL_AMBIENT, material.ambient);
		glMaterialfv(face, GL_DIFFUSE, material.diffuse);
		glMaterialfv(face, GL_SPECULAR, material.specular);
		glMaterialfv(face, GL_EMISSION, material.emission);
		glMaterialf(face, GL_SHININESS, material.shininess);
	}

	static bool sameFloat(float left, float right)
	{
		return gl21FloatBits(left) == gl21FloatBits(right);
	}

	static bool sameFloats(const float *left, const float *right, int count)
	{
		for (int i = 0; i < count; i++)
		{
			if (!sameFloat(left[i], right[i]))
				return false;
		}
		return true;
	}

	static bool sameMatrix(const Mat4 &left, const Mat4 &right)
	{
		return sameFloats(left.m, right.m, 16);
	}

	static bool sameMaterial(const MaterialState &left, const MaterialState &right)
	{
		return sameFloats(left.ambient, right.ambient, 4) &&
			sameFloats(left.diffuse, right.diffuse, 4) &&
			sameFloats(left.specular, right.specular, 4) &&
			sameFloats(left.emission, right.emission, 4) &&
			sameFloat(left.shininess, right.shininess);
	}

	static bool sameLight(const LightState &left, const LightState &right)
	{
		return left.enabled == right.enabled &&
			sameFloats(left.ambient, right.ambient, 4) &&
			sameFloats(left.diffuse, right.diffuse, 4) &&
			sameFloats(left.specular, right.specular, 4) &&
			sameFloats(left.positionEye, right.positionEye, 4) &&
			sameFloats(left.spotDirectionEye, right.spotDirectionEye, 3) &&
			sameFloat(left.spotExponent, right.spotExponent) &&
			sameFloat(left.spotCutoff, right.spotCutoff) &&
			sameFloat(left.constantAttenuation, right.constantAttenuation) &&
			sameFloat(left.linearAttenuation, right.linearAttenuation) &&
			sameFloat(left.quadraticAttenuation, right.quadraticAttenuation);
	}

	static bool sameLighting(const ResolvedLightingState &left,
		const ResolvedLightingState &right)
	{
		if (!sameFloats(left.modelAmbient, right.modelAmbient, 4) ||
			left.colorMaterialFace != right.colorMaterialFace ||
			left.colorMaterialMode != right.colorMaterialMode ||
			!sameMaterial(left.frontMaterial, right.frontMaterial) ||
			!sameMaterial(left.backMaterial, right.backMaterial))
		{
			return false;
		}
		for (int i = 0; i < ResolvedLightingState::MAX_LIGHTS; i++)
		{
			if (!sameLight(left.lights[i], right.lights[i]))
				return false;
		}
		return true;
	}

	static bool sameEnables(const ResolvedEnableState &left,
		const ResolvedEnableState &right)
	{
		return left.texture2D == right.texture2D &&
			left.depthTest == right.depthTest &&
			left.alphaTest == right.alphaTest &&
			left.blend == right.blend &&
			left.cullFace == right.cullFace &&
			left.fog == right.fog &&
			left.lighting == right.lighting &&
			left.colorMaterial == right.colorMaterial &&
			left.rescaleNormal == right.rescaleNormal &&
			left.normalize == right.normalize &&
			left.colorLogicOp == right.colorLogicOp &&
			left.polygonOffsetFill == right.polygonOffsetFill &&
			left.scissorTest == right.scissorTest &&
			left.stencilTest == right.stencilTest &&
			left.lineSmooth == right.lineSmooth &&
			left.dither == right.dither;
	}

	static bool samePipeline(const ResolvedPipelineState &left,
		const ResolvedPipelineState &right)
	{
		if (left.blendSource != right.blendSource ||
			left.blendDestination != right.blendDestination ||
			left.alphaFunction != right.alphaFunction ||
			!sameFloat(left.alphaReference, right.alphaReference) ||
			left.depthFunction != right.depthFunction ||
			left.depthWrite != right.depthWrite ||
			left.cullFaceMode != right.cullFaceMode ||
			left.frontFaceMode != right.frontFaceMode ||
			left.shadeModel != right.shadeModel ||
			left.logicOpcode != right.logicOpcode ||
			!sameFloat(left.lineWidth, right.lineWidth) ||
			!sameFloat(left.polygonOffsetFactor, right.polygonOffsetFactor) ||
			!sameFloat(left.polygonOffsetUnits, right.polygonOffsetUnits))
		{
			return false;
		}
		for (int i = 0; i < 4; i++)
		{
			if (left.colorWrite[i] != right.colorWrite[i] ||
				left.viewport[i] != right.viewport[i])
			{
				return false;
			}
		}
		return true;
	}

	static bool sameFog(const ResolvedFogState &left, const ResolvedFogState &right)
	{
		return left.mode == right.mode &&
			sameFloat(left.density, right.density) &&
			sameFloat(left.start, right.start) &&
			sameFloat(left.end, right.end) &&
			sameFloats(left.color, right.color, 4) &&
			left.distanceMode == right.distanceMode;
	}

	static bool sameTexture(const ResolvedTextureState &left,
		const ResolvedTextureState &right)
	{
		return left.name == right.name &&
			left.minFilter == right.minFilter &&
			left.magFilter == right.magFilter &&
			left.wrapS == right.wrapS &&
			left.wrapT == right.wrapT;
	}

	void applyMaterials(const ResolvedDraw &command)
	{
		glDisable(GL_COLOR_MATERIAL);
		applyMaterial(GL_FRONT, command.lighting.frontMaterial);
		applyMaterial(GL_BACK, command.lighting.backMaterial);
		glColorMaterial(command.lighting.colorMaterialFace,
			command.lighting.colorMaterialMode);
		setEnabled(GL_COLOR_MATERIAL, command.enables.colorMaterial);
	}

	void applyLighting(const ResolvedDraw &command)
	{
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT, command.lighting.modelAmbient);
		applyMaterials(command);
		for (int i = 0; i < ResolvedLightingState::MAX_LIGHTS; i++)
		{
			const LightState &light = command.lighting.lights[i];
			const unsigned int name = GL_LIGHT0 + static_cast<unsigned int>(i);
			glLightfv(name, GL_AMBIENT, light.ambient);
			glLightfv(name, GL_DIFFUSE, light.diffuse);
			glLightfv(name, GL_SPECULAR, light.specular);
			glLightfv(name, GL_POSITION, light.positionEye);
			glLightfv(name, GL_SPOT_DIRECTION, light.spotDirectionEye);
			glLightf(name, GL_SPOT_EXPONENT, light.spotExponent);
			glLightf(name, GL_SPOT_CUTOFF, light.spotCutoff);
			glLightf(name, GL_CONSTANT_ATTENUATION, light.constantAttenuation);
			glLightf(name, GL_LINEAR_ATTENUATION, light.linearAttenuation);
			glLightf(name, GL_QUADRATIC_ATTENUATION, light.quadraticAttenuation);
			setEnabled(name, light.enabled);
		}
	}

	static void applyMatrices(const ResolvedDraw &command, bool modelViewDirty,
		bool projectionDirty, bool textureDirty)
	{
		if (projectionDirty)
		{
			glMatrixMode(GL_PROJECTION);
			glLoadMatrixf(command.projection.m);
		}
		if (textureDirty)
		{
			glMatrixMode(GL_TEXTURE);
			glLoadMatrixf(command.textureMatrix.m);
		}
		if (modelViewDirty)
		{
			glMatrixMode(GL_MODELVIEW);
			glLoadMatrixf(command.modelView.m);
		}
	}

	static void applyEnables(const ResolvedEnableState &enables)
	{
		setEnabled(GL_TEXTURE_2D, enables.texture2D);
		setEnabled(GL_DEPTH_TEST, enables.depthTest);
		setEnabled(GL_ALPHA_TEST, enables.alphaTest);
		setEnabled(GL_BLEND, enables.blend);
		setEnabled(GL_CULL_FACE, enables.cullFace);
		setEnabled(GL_FOG, enables.fog);
		setEnabled(GL_LIGHTING, enables.lighting);
		setEnabled(GL_COLOR_MATERIAL, enables.colorMaterial);
		setEnabled(GL_RESCALE_NORMAL, enables.rescaleNormal);
		setEnabled(GL_NORMALIZE, enables.normalize);
		setEnabled(GL_COLOR_LOGIC_OP, enables.colorLogicOp);
		setEnabled(GL_POLYGON_OFFSET_FILL, enables.polygonOffsetFill);
		setEnabled(GL_SCISSOR_TEST, enables.scissorTest);
		setEnabled(GL_STENCIL_TEST, enables.stencilTest);
		setEnabled(GL_LINE_SMOOTH, enables.lineSmooth);
		setEnabled(GL_DITHER, enables.dither);
	}

	void applyPipeline(const ResolvedPipelineState &pipeline)
	{
		glBlendFunc(pipeline.blendSource, pipeline.blendDestination);
		glAlphaFunc(pipeline.alphaFunction, pipeline.alphaReference);
		glDepthFunc(pipeline.depthFunction);
		glDepthMask(pipeline.depthWrite);
		glColorMask(pipeline.colorWrite[0], pipeline.colorWrite[1],
			pipeline.colorWrite[2], pipeline.colorWrite[3]);
		glCullFace(pipeline.cullFaceMode);
		glFrontFace(pipeline.frontFaceMode);
		glShadeModel(pipeline.shadeModel);
		glLogicOp(pipeline.logicOpcode);
		const float requestedWidth = pipeline.lineWidth;
		glLineWidth(requestedWidth >= lineWidthRange[0] &&
			requestedWidth <= lineWidthRange[1] ? requestedWidth : 1.0f);
		glPolygonOffset(pipeline.polygonOffsetFactor, pipeline.polygonOffsetUnits);
		glViewport(pipeline.viewport[0], pipeline.viewport[1],
			pipeline.viewport[2], pipeline.viewport[3]);
	}

	static void applyFog(const ResolvedFogState &fog)
	{
		glFogi(GL_FOG_MODE, static_cast<GLint>(fog.mode));
		glFogf(GL_FOG_DENSITY, fog.density);
		glFogf(GL_FOG_START, fog.start);
		glFogf(GL_FOG_END, fog.end);
		glFogfv(GL_FOG_COLOR, fog.color);
		if (openglbackend::hasCapability("GL_NV_fog_distance"))
			glFogi(GL_FOG_DISTANCE_MODE_NV, static_cast<GLint>(fog.distanceMode));
	}

	void applyTexture(const ResolvedTextureState &texture)
	{
		glBindTexture(GL_TEXTURE_2D, physicalTexture(texture.name));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
			static_cast<GLint>(texture.minFilter));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
			static_cast<GLint>(texture.magFilter));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
			static_cast<GLint>(texture.wrapS));
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
			static_cast<GLint>(texture.wrapT));
	}

	void applyResolvedState(const ResolvedDraw &command)
	{
		stateSyncRequests++;
		// Lights and materials cannot affect a draw with GL_LIGHTING disabled,
		// and Alpha draws terrain unlit while leaving GL_COLOR_MATERIAL on,
		// which otherwise forced a full material restore after every one of
		// those draws. Nothing is uploaded for an unlit draw, and the applied
		// state is left describing what GL still holds so a later lit draw can
		// still suppress its own upload.
		const bool lightingStale = !lightingValid ||
			!sameLighting(appliedLighting, command.lighting);
		const bool lightingApplied = command.enables.lighting && lightingStale;
		if (!command.enables.lighting)
		{
			lightingSuppressions++;
		}
		else if (lightingStale)
		{
			applyLighting(command);
			appliedLighting = command.lighting;
			lightingValid = true;
			materialStateDirty = false;
			lightingEmits++;
		}
		else
		{
			lightingSuppressions++;
			if (materialStateDirty)
			{
				applyMaterials(command);
				materialStateDirty = false;
				materialRestores++;
			}
		}

		const bool normalModeDirty = !enablesValid ||
			appliedEnables.rescaleNormal != command.enables.rescaleNormal ||
			appliedEnables.normalize != command.enables.normalize;
		const bool modelViewDirty = lightingApplied || normalModeDirty || !matricesValid ||
			!sameMatrix(appliedModelView, command.modelView);
		const bool projectionDirty = !matricesValid ||
			!sameMatrix(appliedProjection, command.projection);
		const bool textureMatrixDirty = !matricesValid ||
			!sameMatrix(appliedTextureMatrix, command.textureMatrix);
		if (modelViewDirty || projectionDirty || textureMatrixDirty)
		{
			applyMatrices(command, modelViewDirty, projectionDirty, textureMatrixDirty);
			if (modelViewDirty)
				appliedModelView = command.modelView;
			if (projectionDirty)
				appliedProjection = command.projection;
			if (textureMatrixDirty)
				appliedTextureMatrix = command.textureMatrix;
			matricesValid = true;
			matrixEmits++;
		}
		else
		{
			matrixSuppressions++;
		}

		if (!enablesValid || !sameEnables(appliedEnables, command.enables))
		{
			applyEnables(command.enables);
			appliedEnables = command.enables;
			enablesValid = true;
			enableEmits++;
		}
		else
		{
			enableSuppressions++;
		}

		if (!pipelineValid || !samePipeline(appliedPipeline, command.pipeline))
		{
			applyPipeline(command.pipeline);
			appliedPipeline = command.pipeline;
			pipelineValid = true;
			pipelineEmits++;
		}
		else
		{
			pipelineSuppressions++;
		}

		if (!fogValid || !sameFog(appliedFog, command.fog))
		{
			applyFog(command.fog);
			appliedFog = command.fog;
			fogValid = true;
			fogEmits++;
		}
		else
		{
			fogSuppressions++;
		}

		if (!textureValid || !sameTexture(appliedTexture, command.texture))
		{
			applyTexture(command.texture);
			appliedTexture = command.texture;
			textureValid = true;
			textureEmits++;
		}
		else
		{
			textureSuppressions++;
		}
	}

	static void applyCurrentAttributes(const ResolvedDraw &command)
	{
		const Geometry &geometry = *command.geometry;
		if (!geometry.hasColor)
		{
			if (command.enables.colorMaterial)
				glDisable(GL_COLOR_MATERIAL);
			glColor4f(command.currentAttributes.r, command.currentAttributes.g,
				command.currentAttributes.b, command.currentAttributes.a);
			if (command.enables.colorMaterial)
				glEnable(GL_COLOR_MATERIAL);
		}
		if (!geometry.hasNormal)
		{
			glNormal3f(command.currentAttributes.nx, command.currentAttributes.ny,
				command.currentAttributes.nz);
		}
		if (!geometry.hasTexCoord)
			glTexCoord2f(command.currentAttributes.s, command.currentAttributes.t);
	}

	static void bindCanonicalArrays(GLuint buffer, const Geometry &geometry)
	{
		glBindBuffer(GL_ARRAY_BUFFER, buffer);
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex),
			reinterpret_cast<const void *>(offsetof(Vertex, x)));
		glEnableClientState(GL_VERTEX_ARRAY);

		if (geometry.hasTexCoord)
		{
			glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex),
				reinterpret_cast<const void *>(offsetof(Vertex, s)));
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		}
		else
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		if (geometry.hasColor)
		{
			glColorPointer(4, GL_FLOAT, sizeof(Vertex),
				reinterpret_cast<const void *>(offsetof(Vertex, r)));
			glEnableClientState(GL_COLOR_ARRAY);
		}
		else
			glDisableClientState(GL_COLOR_ARRAY);

		if (geometry.hasNormal)
		{
			glNormalPointer(GL_FLOAT, sizeof(Vertex),
				reinterpret_cast<const void *>(offsetof(Vertex, nx)));
			glEnableClientState(GL_NORMAL_ARRAY);
		}
		else
			glDisableClientState(GL_NORMAL_ARRAY);
	}

	static void disableCanonicalArrays()
	{
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	GLuint physicalTexture(unsigned int name)
	{
		if (name == 0)
			return 0;
		textureNames.reserve(name);
		auto found = textures.find(name);
		if (found != textures.end())
			return found->second;
		GLuint texture = 0;
		glGenTextures(1, &texture);
		return textures.emplace(name, texture).first->second;
	}
	void releaseAllCanonicalGeometry()
	{
		for (auto &entry : residentGeometry)
		{
			if (entry.second.vertexArray != 0)
				glDeleteVertexArrays(1, &entry.second.vertexArray);
			glDeleteBuffers(1, &entry.second.vertexBuffer);
		}
		residentGeometry.clear();
	}

	std::unordered_map<std::uint64_t, GL21ResidentGeometryEntry> residentGeometry;
	std::map<unsigned int, GLuint> textures;
	GL21LogicalNameAllocator textureNames;
	GL21LogicalNameAllocator bufferNames;
	GLuint streamVertexBuffer = 0;
	GLuint streamVertexArray = 0;
	unsigned int streamAttributeMask = 0;
	bool streamLayoutValid = false;
	std::uint64_t nextListName = 1;
	std::uint64_t residentCacheHits = 0;
	std::uint64_t residentCacheMisses = 0;
	std::uint64_t residentGeometryBytes = 0;
	std::uint64_t residentGeometryReleases = 0;
	Mat4 appliedModelView = {};
	Mat4 appliedProjection = {};
	Mat4 appliedTextureMatrix = {};
	ResolvedLightingState appliedLighting;
	ResolvedEnableState appliedEnables;
	ResolvedPipelineState appliedPipeline;
	ResolvedFogState appliedFog;
	ResolvedTextureState appliedTexture;
	std::uint64_t stateSyncRequests = 0;
	std::uint64_t matrixEmits = 0;
	std::uint64_t matrixSuppressions = 0;
	std::uint64_t lightingEmits = 0;
	std::uint64_t materialRestores = 0;
	std::uint64_t lightingSuppressions = 0;
	std::uint64_t enableEmits = 0;
	std::uint64_t enableSuppressions = 0;
	std::uint64_t pipelineEmits = 0;
	std::uint64_t pipelineSuppressions = 0;
	std::uint64_t fogEmits = 0;
	std::uint64_t fogSuppressions = 0;
	std::uint64_t textureEmits = 0;
	std::uint64_t textureSuppressions = 0;
	float lineWidthRange[2] = { 1.0f, 1.0f };
	bool matricesValid = false;
	bool lightingValid = false;
	bool enablesValid = false;
	bool pipelineValid = false;
	bool fogValid = false;
	bool textureValid = false;
	bool materialStateDirty = false;
	bool vaoSupported = false;
	bool initialized = false;
};

static OpenGL21Sink theOpenGL21Sink;

}

namespace renderbackend
{

static const Configuration &openGL21Configuration()
{
	static const Configuration config = {
		"gl21",
		2, 1, OpenGLProfile::Compatibility,
		2, 1, OpenGLProfile::None,
		false,
		false
	};
	return config;
}

static void openGL21Initialize()
{
	openglbackend::initialize(openGL21Configuration());
	legacygl::theOpenGL21Sink.initialize();
}

static void openGL21Present()
{
	openglbackend::present();
}

static void openGL21Shutdown()
{
	legacygl::theOpenGL21Sink.shutdown();
	openglbackend::shutdown();
}

static bool openGL21HasCapability(const char *capability)
{
	return openglbackend::hasCapability(capability);
}

static legacygl::Sink *openGL21Sink()
{
	return &legacygl::theOpenGL21Sink;
}

const Backend &openGL21Backend()
{
	static const Backend backend = {
		"gl21",
		openGL21Configuration,
		openGL21Initialize,
		openGL21Present,
		openGL21Shutdown,
		openGL21HasCapability,
		openGL21Sink
	};
	return backend;
}

}
