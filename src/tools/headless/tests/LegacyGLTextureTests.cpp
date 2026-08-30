// LegacyGL texture object, buffer object and readback tests.
//
// Texture names have three distinct states in legacy OpenGL - unused, reserved
// by generation, and an actual object created by the first bind - and the
// parameters belong to the object, not to the context. Textures.cpp depends on
// all of it, including the legacy GL_CLAMP wrap mode that is deliberately not
// GL_CLAMP_TO_EDGE.

#include <vector>

#include "tools/headless/TestFramework.h"
#include "tools/headless/tests/LegacyGLFixture.h"

HEADLESS_TEST(legacygl_textures, generation_reserves_names_and_binding_creates_objects)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint names[3] = { 0, 0, 0 };
	glGenTextures(3, names);
	ctx.check(names[0] != 0 && names[1] != 0 && names[2] != 0, "three names were handed out");
	ctx.check(names[0] != names[1] && names[1] != names[2], "the names are distinct");

	// A generated name is reserved but is not yet a texture object.
	ctx.check(gl.isTextureName(names[0]), "the generated name is known");
	ctx.check(!gl.isTextureObject(names[0]), "generation alone does not create an object");

	glBindTexture(GL_TEXTURE_2D, names[0]);
	ctx.check(gl.isTextureObject(names[0]), "the first bind creates the object");
	ctx.checkEqual(static_cast<long long>(gl.boundTexture()), static_cast<long long>(names[0]), "and binds it");

	// Binding an unused nonzero name also creates an object under legacy rules.
	const GLuint arbitrary = 4242;
	ctx.check(!gl.isTextureName(arbitrary), "the arbitrary name was not generated");
	glBindTexture(GL_TEXTURE_2D, arbitrary);
	ctx.check(gl.isTextureObject(arbitrary), "binding an unused name creates an object");

	// GL_TEXTURE_1D (0x0DE0) is not part of the supported profile.
	glBindTexture(0x0DE0, names[1]);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "only GL_TEXTURE_2D is in the profile");

	glGenTextures(-1, names);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative count is invalid");
}

HEADLESS_TEST(legacygl_textures, new_objects_start_at_the_legacy_defaults)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint name = 0;
	glGenTextures(1, &name);
	glBindTexture(GL_TEXTURE_2D, name);

	const legacygl::TextureObject *object = gl.texture(name);
	ctx.check(object != nullptr, "the object exists");
	if (object == nullptr)
		return;

	// A fresh legacy texture minifies with NEAREST_MIPMAP_LINEAR and magnifies
	// with LINEAR. Initialising to nearest/nearest instead would make a
	// level-zero-only texture look complete when it is not.
	ctx.checkEqual(object->minFilter, GL_NEAREST_MIPMAP_LINEAR, "default minification filter");
	ctx.checkEqual(object->magFilter, GL_LINEAR, "default magnification filter");
	ctx.checkEqual(object->wrapS, GL_REPEAT, "default wrap S");
	ctx.checkEqual(object->wrapT, GL_REPEAT, "default wrap T");
	ctx.check(object->borderColor[3] == 0.0f, "default border colour is transparent black");
	ctx.check(object->usesMipmapFilter(), "the default filter is a mipmapped one");
	ctx.check(!object->complete(), "an object with no image is incomplete");
}

HEADLESS_TEST(legacygl_textures, parameters_belong_to_the_object)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint names[2] = { 0, 0 };
	glGenTextures(2, names);

	glBindTexture(GL_TEXTURE_2D, names[0]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);

	glBindTexture(GL_TEXTURE_2D, names[1]);
	const legacygl::TextureObject *second = gl.texture(names[1]);
	ctx.check(second != nullptr && second->minFilter == GL_NEAREST_MIPMAP_LINEAR,
		"the second object kept its own defaults");

	const legacygl::TextureObject *first = gl.texture(names[0]);
	ctx.check(first != nullptr && first->minFilter == GL_NEAREST, "the first object kept its own filter");
	// Legacy GL_CLAMP is stored as GL_CLAMP. Rewriting it to GL_CLAMP_TO_EDGE
	// here would silently drop the border blend a backend has to reproduce.
	ctx.check(first != nullptr && first->wrapS == GL_CLAMP, "GL_CLAMP is not rewritten to clamp-to-edge");
	ctx.check(first != nullptr && first->wrapT == GL_REPEAT, "the other axis is untouched");

	// Object zero has independent state.
	glBindTexture(GL_TEXTURE_2D, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	ctx.checkEqual(gl.defaultTexture().minFilter, GL_LINEAR, "object zero holds its own parameters");
	ctx.check(gl.texture(names[0])->minFilter == GL_NEAREST, "changing object zero did not touch a named object");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_TEXTURE_2D);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unknown filter is rejected");
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, 0);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unimplemented parameter is rejected, not ignored");
}

HEADLESS_TEST(legacygl_textures, deleting_a_bound_object_rebinds_zero)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint name = 0;
	glGenTextures(1, &name);
	glBindTexture(GL_TEXTURE_2D, name);
	glDeleteTextures(1, &name);

	ctx.checkEqual(static_cast<long long>(gl.boundTexture()), 0, "deleting the bound object rebinds zero");
	ctx.check(!gl.isTextureObject(name), "the object is gone");

	// Deleting zero or an unknown name is silently ignored.
	const GLuint zero = 0;
	const GLuint unknown = 999999;
	glDeleteTextures(1, &zero);
	glDeleteTextures(1, &unknown);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "deleting zero or an unknown name is ignored");

	glDeleteTextures(-1, &name);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative delete count is invalid");
}

HEADLESS_TEST(legacygl_textures, image_definition_tracks_levels_and_completeness)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint name = 0;
	glGenTextures(1, &name);
	glBindTexture(GL_TEXTURE_2D, name);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	std::vector<unsigned char> pixels(4 * 4 * 4, 0x7F);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "a level zero RGBA image is accepted");

	const legacygl::TextureObject *object = gl.texture(name);
	ctx.check(object != nullptr && object->levels[0].defined, "level zero is defined");
	ctx.check(object != nullptr && object->levels[0].width == 4 && object->levels[0].height == 4,
		"the dimensions were recorded");
	// A non-mipmapped filter makes a level-zero-only texture complete, which is
	// exactly what Textures.cpp selects.
	ctx.check(object != nullptr && object->complete(), "level zero plus a nearest filter is complete");

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	ctx.check(!gl.texture(name)->complete(), "selecting a mipmapped filter makes it incomplete again");

	glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	glTexImage2D(GL_TEXTURE_2D, 2, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.check(gl.texture(name)->complete(), "supplying the whole chain completes it");

	// A null image is legal and defines the level without reading memory.
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "a null image is accepted");
	ctx.check(gl.texture(name)->levels[0].width == 8, "the redefinition took effect");
	ctx.check(!gl.texture(name)->complete(), "and it invalidated the mip chain");

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a texture border is outside the profile");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_RGBA, GL_FLOAT, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported pixel type is rejected");
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0, GL_LIGHT0, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported source format is rejected");
	glTexImage2D(GL_TEXTURE_2D, 99, GL_RGBA, 4, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "an out-of-range level is invalid");
}

HEADLESS_TEST(legacygl_textures, subimage_is_clipped_against_the_defined_level)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint name = 0;
	glGenTextures(1, &name);
	glBindTexture(GL_TEXTURE_2D, name);

	std::vector<unsigned char> pixels(16 * 16 * 4, 0);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "a subimage needs a defined level first");

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	// This is the animated-texture path: a 16x16 tile written into the terrain
	// atlas.
	glTexSubImage2D(GL_TEXTURE_2D, 0, 32, 48, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "an in-range tile update is accepted");

	glTexSubImage2D(GL_TEXTURE_2D, 0, 248, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "an update that runs off the edge is invalid");
	glTexSubImage2D(GL_TEXTURE_2D, 0, -1, 0, 16, 16, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative offset is invalid");
}

HEADLESS_TEST(legacygl_textures, buffer_objects_own_their_data_immediately)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint buffer = 0;
	glGenBuffersARB(1, &buffer);
	ctx.check(buffer != 0, "a buffer name was handed out");
	ctx.check(gl.isBufferName(buffer), "the name is known");

	std::vector<unsigned char> data(64, 0xAB);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(data.size()), data.data(),
		GL_STREAM_DRAW_ARB);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "uploading without a binding is invalid");

	glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(data.size()), data.data(),
		GL_STREAM_DRAW_ARB);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "the upload is accepted once bound");

	// The buffer copied the bytes, so changing the source afterwards cannot
	// change what a later draw reads.
	data[0] = 0x00;
	const legacygl::BufferObject *object = gl.buffer(buffer);
	ctx.check(object != nullptr, "the buffer object exists");
	if (object != nullptr)
	{
		ctx.checkEqual(static_cast<long long>(object->size), 64, "the size was recorded");
		ctx.checkEqual(object->usage, GL_STREAM_DRAW_ARB, "the usage hint was recorded");
		ctx.check(object->data.size() == 64 && object->data[0] == 0xAB, "the contents were copied at call time");
	}

	// A null pointer allocates storage without reading caller memory.
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, 32, nullptr, GL_STREAM_DRAW_ARB);
	ctx.check(gl.buffer(buffer)->data.size() == 32, "a null upload resizes the storage");

	glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB, buffer);
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "only the array buffer target is in the profile");
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, -1, nullptr, GL_STREAM_DRAW_ARB);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative size is invalid");
}

HEADLESS_TEST(legacygl_textures, array_draw_can_read_from_a_buffer_object)
{
	legacygl::Context &gl = legacyglTest::begin();

	GLuint buffer = 0;
	glGenBuffersARB(1, &buffer);
	glBindBufferARB(GL_ARRAY_BUFFER_ARB, buffer);

	const float vertices[6] = { 0.0f, 0.0f, 0.0f, 1.0f, 2.0f, 3.0f };
	glBufferDataARB(GL_ARRAY_BUFFER_ARB, static_cast<GLsizeiptrARB>(sizeof(vertices)), vertices,
		GL_STREAM_DRAW_ARB);

	// With a buffer bound the pointer is a byte offset into that buffer, not a
	// client address.
	glVertexPointer(3, GL_FLOAT, 0, reinterpret_cast<const GLvoid *>(0));
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawArrays(GL_LINES, 0, 2);

	const legacygl::Geometry &drawn = gl.lastGeometry();
	ctx.checkEqual(static_cast<long long>(drawn.vertices.size()), 2, "two vertices came from the buffer");
	if (drawn.vertices.size() == 2)
		ctx.checkEqualBits(drawn.vertices[1].z, 3.0f, "the buffer contents were decoded");

	// Reading past the end of the buffer is an error rather than a wild read.
	glDrawArrays(GL_LINES, 0, 8);
	ctx.checkEqual(gl.getError(), GL_INVALID_OPERATION, "an out-of-range buffer read is refused");
}

HEADLESS_TEST(legacygl_textures, readback_validates_its_format_and_destination)
{
	legacygl::Context &gl = legacyglTest::begin();

	std::vector<unsigned char> destination(64 * 64 * 3, 0);

	// The screenshot path: pack alignment one and a BGR unsigned byte readback.
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, 64, 64, GL_BGR_EXT, GL_UNSIGNED_BYTE, destination.data());
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "the screenshot readback is accepted");

	glReadPixels(0, 0, 64, 64, GL_BGR_EXT, GL_UNSIGNED_BYTE, nullptr);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a null destination is invalid");
	glReadPixels(0, 0, -1, 64, GL_BGR_EXT, GL_UNSIGNED_BYTE, destination.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "a negative rectangle is invalid");
	glReadPixels(0, 0, 64, 64, GL_BGR_EXT, GL_FLOAT, destination.data());
	ctx.checkEqual(gl.getError(), GL_INVALID_ENUM, "an unsupported readback type is rejected");
}

HEADLESS_TEST(legacygl_textures, clear_rejects_unknown_buffer_bits)
{
	legacygl::Context &gl = legacyglTest::begin();

	glClearColor(0.25f, 0.5f, 0.75f, 1.0f);
	glClearDepth(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	ctx.checkEqual(gl.pendingError(), GL_NO_ERROR, "a combined colour and depth clear is accepted");
	ctx.checkEqualBits(gl.clearColorValue()[2], 0.75f, "the clear colour is tracked");

	glClear(0x00000002);
	ctx.checkEqual(gl.getError(), GL_INVALID_VALUE, "an unknown clear bit is invalid");
}
