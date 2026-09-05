#include <glad/glad.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include "backends/Backend.h"

static PFNGLCLIENTWAITSYNCPROC realWait;
static PFNGLDELETESYNCPROC realDelete;
static const GLenum *waitResults;
static int waitResultCount;
static int waitCalls;
static GLsync waitedFence;
static bool completedFence;
static bool deletedIncompleteFence;

static GLenum APIENTRY scriptedWait(GLsync fence, GLbitfield, GLuint64)
{
	waitedFence = fence;
	const GLenum result = waitResults[waitCalls < waitResultCount ? waitCalls : waitResultCount - 1];
	++waitCalls;
	completedFence = result == GL_ALREADY_SIGNALED || result == GL_CONDITION_SATISFIED;
	return result;
}

static void APIENTRY checkedDelete(GLsync fence)
{
	if (fence == waitedFence && !completedFence)
		deletedIncompleteFence = true;
	realDelete(fence);
}

// Separate translation unit: GL loader macros must never enter the semantic fixture.
void runOpenGL33SyncTests()
{
	// Empty presents establish all three region fences. Finish actual GPU work
	// before substituting statuses, so a deliberately bad test cannot race the GPU.
	for (int i = 0; i < 3; ++i)
		renderbackend::present();
	glFinish();
	realWait = glad_glClientWaitSync;
	realDelete = glad_glDeleteSync;
	struct Restore
	{
		~Restore() { glad_glClientWaitSync = realWait; glad_glDeleteSync = realDelete; }
	} restore;
	glad_glClientWaitSync = scriptedWait;
	glad_glDeleteSync = checkedDelete;
	const GLenum sequences[][3] = {
		{GL_TIMEOUT_EXPIRED, GL_TIMEOUT_EXPIRED, GL_CONDITION_SATISFIED},
		{GL_WAIT_FAILED, GL_WAIT_FAILED, GL_WAIT_FAILED},
		{GL_TIMEOUT_EXPIRED, GL_WAIT_FAILED, GL_WAIT_FAILED},
		{GL_ALREADY_SIGNALED, GL_ALREADY_SIGNALED, GL_ALREADY_SIGNALED}
	};
	for (int fixture = 0; fixture < 4; ++fixture)
	{
		waitResults = sequences[fixture];
		waitResultCount = 3;
		waitCalls = 0;
		waitedFence = nullptr;
		completedFence = false;
		deletedIncompleteFence = false;
		bool failed = false;
		try { renderbackend::present(); }
		catch (const std::runtime_error &) { failed = true; }
		if (fixture == 0 && waitCalls == 0)
		{
			std::cout << "gl33-sync: skipped, persistent mode unavailable or disabled\n";
			return;
		}
		const int expectedCalls[] = {3, 1, 2, 1};
		if (waitCalls != expectedCalls[fixture] || failed != (fixture == 1 || fixture == 2) || deletedIncompleteFence)
			throw std::runtime_error("GL33 persistent fence completion fixture failed: " + std::to_string(fixture));
	}
	std::cout << "gl33-sync: repeated timeout, poll failure, wait failure and immediate completion passed\n";
}
