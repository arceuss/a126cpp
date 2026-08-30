#include "legacygl/Startup.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "backends/Backend.h"
#include "legacygl/Context.h"
#include "legacygl/Trace.h"

namespace legacygl
{

// Prints the validation summary when the process winds down. Without it the
// counters would only be visible to a debugger and a session's worth of
// evidence about backend divergence would be lost.
//
// std::atexit rather than a static object: registration happens at run time,
// after the context's own construction finished, so this handler is guaranteed
// to run before the context is destroyed.
static void reportValidation()
{
	if (!context().validating())
		return;
	std::cout << context().validationReport() << std::flush;
}

void installSelectedBackend()
{
	Context &ctx = context();
	const renderbackend::Configuration &backend = renderbackend::configuration();
	Sink *selectedSink = renderbackend::sink();
	if (ctx.sink() == selectedSink)
		return;
	if (ctx.sink() != nullptr)
		throw std::runtime_error("legacygl: refusing to switch the installed backend");

	const char *tracePath = std::getenv("A126_LEGACYGL_TRACE");
	if (tracePath != nullptr && tracePath[0] != '\0')
	{
		traceOpen(tracePath);
		std::cout << "legacygl: tracing frontend calls to " << tracePath << '\n';
	}

	const char *validate = std::getenv("A126_LEGACYGL_VALIDATE");
	if (validate != nullptr && validate[0] != '\0' && validate[0] != '0')
	{
		if (!backend.supportsQueryValidation)
		{
			std::cout << "legacygl: A126_LEGACYGL_VALIDATE is unavailable with the " <<
				backend.recordName << " backend\n";
		}
		else
		{
			ctx.setValidate(true);
			std::cout << "legacygl: query validation against the native backend enabled\n";
			std::atexit(&reportValidation);
		}
	}

	// The context starts at its legacy defaults; the backend that will render
	// is installed here and not before, so no call can reach GL without a
	// current context.
	ctx.setSink(selectedSink);
}

}
