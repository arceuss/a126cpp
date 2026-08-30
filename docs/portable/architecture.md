# Portable renderer architecture

## The boundary

The game's OpenGL call stream is part of the compatibility contract. Alpha's
output depends on state ordering, alpha-tested textures, display lists, fog and
a long tail of small state transitions, so the port translates *below* the call
stream rather than rewriting it.

```text
Alpha-facing renderer code (unchanged)
        |
        | gl* calls, same order, same arguments
        v
src/pc/OpenGL.h -> legacygl/LegacyGL.h        LegacyGL frontend
        |
        v
legacygl/Context                              one semantic core
  matrices, current attributes, enables,
  errors, texture and buffer objects,
  display lists, client arrays, pixel store,
  lighting, fog, queries
        |
        +--> resolved draw/upload/clear/readback commands
        |
        +--> legacygl::Sink                    one backend per process
             |
             +-- src/backends/NativeGL         compatibility GL oracle
             +-- src/backends/OpenGL46         translated GL 4.6 Core
             +-- src/backends/Vulkan           translated Vulkan 1.1+
             +-- src/backends/D3D12            translated Direct3D 12
             +-- test-only null/recording sinks

renderbackend lifecycle
  configuration, initialize, present, shutdown,
  capability discovery, sink installation
        |
        +--> src/backends/Platform/Platform.h  window-system boundary
             |
             +-- src/backends/Platform/SDL2   current implementation
```

No renderer file changed to make this work. `src/pc/OpenGL.h` used to include
`glad/glad.h`; it now includes the frontend header, and the 49 translation units
that include it compile unmodified. Loader headers are confined below the
backend boundary because a loader defines `gl*` as macros, which would bypass
the frontend. The command line is parsed before `GLContext::instantiate()`.
That function initializes the linked platform, initializes the selected
renderer provider and installs its sink before renderer code can build a
display list. OpenGL context creation and loader verification live in
`src/backends/OpenGL`; Vulkan owns its instance, device, surface and swapchain
without creating an OpenGL context, and D3D12 obtains the platform's opaque
Win32 window handle. Vulkan obtains `vkGetInstanceProcAddr` through the opaque
platform bridge only after a Vulkan window is requested, so compiling that
provider into the executable does not make other runtime selections load the
optional Vulkan loader.

The production executable selects one compiled provider for the process:

```text
Alpha126Cpp.exe --backend nativegl
Alpha126Cpp.exe --backend gl46
Alpha126Cpp.exe --backend vulkan
Alpha126Cpp.exe --backend d3d12
```

`nativegl` is the default. CMake links the enabled providers into production;
an unknown or unavailable name is rejected before graphics initialization.
Selection remains immutable after startup because display-list vertex capture
depends on the active provider's canonical-geometry requirement. The opt-in GPU
parity build deliberately produces separate native, GL46, Vulkan and D3D12
executables, each linked to one provider, so every fixture process still has
one backend.

The window-system implementation is selected independently with the exact
CMake cache value `-DA126_PLATFORM_BACKEND=SDL2`. `SDL2` is currently both the
default and the only accepted value; other spellings fail configuration. Keeping
this as a separate selector is what lets a future console platform replace the
window, event and surface plumbing without growing platform code inside a
renderer backend or moving legacy GL semantics out of the core.

## Why the sink mirrors the call stream

`legacygl::Sink` has one method per inventoried entry point instead of a
packet-consuming interface. That is deliberate for this milestone:

- The native backend must reproduce the game's exact call stream to stay usable
  as the oracle. A backend that received canonicalized packets would already be
  a different renderer, and there would be nothing left to compare against.
- The semantic core still sees every call, so state, errors, queries, display
  lists and validation are unaffected by which backend is installed.

A translated backend implements the same interface and consumes the resolved
commands emitted after the core has applied legacy semantics.
`Sink::wantsCanonicalGeometry()` is how it opts in to decoded vertices. The
OpenGL 4.6, Vulkan and D3D12 backends opt in; the native backend leaves it off because
compatibility OpenGL walks the client arrays itself, and decoding them twice
would cost real time on the reference path.

## What the core owns

`legacygl::Context` is the single authority for:

- three matrix stacks in OpenGL conventions, postmultiplying, never converted to
  a backend clip space;
- current colour, normal and texture coordinate, including the
  `glColor3f`-resets-alpha rule;
- the server enable set, restricted to the capabilities the renderer uses;
- blend, alpha test, depth, colour mask, cull, front face, shade model, logic
  op, line width, polygon offset, viewport, pixel store;
- fog state, including the `GL_NV_fog_distance` mode as state separate from the
  fog equation;
- eight lights with eye-space positions transformed when the setter ran, the
  light model ambient, colour material tracking and front/back materials;
- texture names, objects, per-object parameters, level definitions and
  completeness;
- ARB buffer names, bindings and owned contents;
- display list names, definitions and compiled commands;
- the first pending GL error;
- every query the renderer makes.

Queries are answered from this state. `client/renderer/culling/Frustum.cpp`
reads `GL_PROJECTION_MATRIX` and `GL_MODELVIEW_MATRIX` every frame, so those
matrices have to stay in GL conventions no matter which backend renders. A
backend applies its clip-space correction when it lowers a draw, once.

## Call-time versus execution-time

The distinction is encoded per command rather than handled by a generic
"flush current state at draw time" rule:

| command | when it resolves |
|---|---|
| `glLight*(..., GL_POSITION, ...)` | transformed by the model-view matrix in force at the call |
| `glDrawArrays` inside a display list | vertex data decoded and copied at compile time |
| immediate mode inside a display list | compiled command by command; vertices read the current attributes the list installs while it executes |
| attributes a draw does not supply | read from the current state when the draw executes |
| `glPixelStorei`, client array enables, pointer setters, name generation, buffer commands, queries, readback, `glFinish` | execute immediately, never compiled into a list |

The display-list rules are what make Alpha's font and sign paths work. A glyph
list carries positions and texture coordinates but no colours, so a surrounding
`glColor4f` recolours it; a cached world sign carries its own `glColor3f`, and
its immediate-mode vertices must read that colour when the list runs, not
whatever happened to be current while it was compiled.

## Object naming

Names come from the backend, not from a counter in the core. The game hands
texture ids, list ids and buffer ids straight back to the facade, so the native
backend uses the compatibility driver's namespaces and the translated backends
allocate equivalent logical namespaces independently of private Core GL or
Vulkan handles. The core registers what it is told. The null backend allocates
deterministic names so the tests can assert on them.

`glGenLists` does not create list definitions. `LevelRenderer` asks for 786432
names in one call; materialising a definition per name would cost tens of
megabytes for lists that may never be compiled.

## Trace and validation

Two opt-in facilities, both off by default:

- `A126_LEGACYGL_TRACE=<path>` writes frontend calls in order with their
  arguments, the display-list compile context, upload payload hashes and any GL
  error. Interactive runs trace the complete stream. Deterministic `--capture`
  runs arm tracing before initialization but record and renumber only the final
  production render, excluding setup, warm-up and readback. This is gate A of
  the parity plan: a backend change must not alter the call stream.
- `A126_LEGACYGL_VALIDATE=1` diffs every query answer against a backend that
  exposes oracle hooks (currently NativeGL), classifies divergences by units in
  the last place, counts how often the renderer leans on a formally
  indeterminate current attribute, and prints a summary at exit. Translated
  backends still answer game queries only from the core and expose no
  compatibility-driver query hooks.

Findings from those runs are in `semantic-notes.md` and `parity-testing.md`.

## Backend and platform ownership

`src/legacygl` is deliberately backend-agnostic: it owns legacy semantics and
resolved commands. Production GPU implementations live under `src/backends`.
`Backend.h` exposes backend configuration, initialization, presentation,
shutdown, capability discovery and the installed sink. `Platform.h` separately
owns platform initialization, window lifetime and state, event pumping,
drawable sizing, cursor placement, the opaque Vulkan WSI bridge and, on
Windows, an opaque native-window handle for D3D12. Vulkan and Win32 types stay
out of the generic platform header.

The current implementation is SDL2, selected independently from the renderer.
The shared OpenGL context helper, Vulkan backend and D3D12 backend consume that
platform contract; SDL's `SDL_SysWMinfo` is confined to the SDL2 implementation
that supplies the opaque HWND. `pc/lwjgl/Display.cpp` only delegates window
operations and presentation. Normal startup is platform initialization followed
by renderer initialization. Process shutdown reverses ownership: renderer
shutdown, window destruction, then platform shutdown. The game executable and
all four GPU fixtures use the same lifecycle.

This is the portable ownership seam, not a claim that a console implementation
already exists. A future platform backend still has to provide the target's
window/display, input and graphics-surface hooks. Today the SDL2 implementation
still translates events directly into the existing LWJGL-shaped keyboard/mouse
queues, and the shared OpenGL context helper reaches the `SDL_Window` through an
SDL2-private header; a non-SDL port must replace those private couplings and
provide the native surface/window bridge required by its explicit API. Those
additions remain below LegacyGL; matrix stacks, lights, display lists, texture
defaults and queries do not move into platform, Vulkan or Direct3D code.

## Threading

One thread owns the context. Chunk meshing happens on the main thread in this
port, and nothing in `legacygl` takes a lock; introducing worker threads later
means giving them CPU-side mesh buffers, not frontend access.

## Optimisation rules

Allowed below the boundary: caching, interning, batching, eliminating redundant
*backend* state changes, converted display-list meshes.

A resident display-list mesh is not identified by list geometry alone when the
captured vertices omit colour, normal or texture coordinates. Its variant key
must include the execution-time current values for every omitted attribute; the
`list.execution-current-color-variants` fixture guards this rule.

Not allowed: removing, reordering or coalescing frontend calls; answering a
query from backend state; applying a state change earlier than legacy semantics
require; changing primitive decomposition; treating `GL_CLAMP` as
`GL_CLAMP_TO_EDGE`.
