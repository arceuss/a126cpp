#pragma once

// Backend selection, callable from the platform layer.
//
// This header deliberately declares nothing that mentions a GL type, so the
// window/context code (which includes a GL loader) can call it without pulling
// legacygl/LegacyGL.h into the same translation unit.

namespace legacygl
{

// Installs the linked backend and applies the environment configuration. Must
// be called once the GL context is current and the loader has run. Reads:
//
//   A126_LEGACYGL_TRACE=<path>  write a deterministic frontend call trace
//   A126_LEGACYGL_VALIDATE=1    diff semantic-core queries against the backend
void installSelectedBackend();

}
