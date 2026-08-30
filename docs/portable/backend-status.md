# Backend status

## Selection and source layout

Production implementations live below `src/backends`; the shared LegacyGL
facade and semantic core remain below `src/legacygl`.

```text
src/backends/Backend.h
src/backends/Platform/Platform.h
src/backends/Platform/SDL2/Platform.{h,cpp}
src/backends/NativeGL/Backend.cpp
src/backends/OpenGL/Context.{h,cpp}
src/backends/OpenGL46/Backend.cpp
src/backends/OpenGL46/Shaders.{h,cpp}
src/backends/Vulkan/Backend.cpp
src/backends/Vulkan/Shaders.{h,cpp}
src/backends/Vulkan/shaders/*.{vert,frag}
```

Choose the production backend at configure time with the exact CMake cache
value `A126_RENDER_BACKEND=NativeGL|OpenGL46|Vulkan`. `NativeGL` is the default
and any other value is rejected. One backend implementation is linked into each
production executable and one sink is installed per process; there is no
`A126_LEGACYGL_BACKEND` run-time selector.

Choose the platform independently with `A126_PLATFORM_BACKEND=SDL2`. `SDL2` is
currently the default and only accepted exact value; any other value is a
configure error. Renderer selection does not imply platform selection.

`A126_ENABLE_GL_GPU_TESTS=ON` is a test-only exception to building one selected
target: it creates separate native, GL46 and Vulkan parity executables. Each
process still links exactly one backend.

## Portable platform and renderer lifecycle - working

`src/backends/Backend.h` defines configuration, initialize, present, shutdown,
capability and sink operations. `src/backends/Platform/Platform.h` separately
defines platform lifetime, window state, events, drawable size, cursor
placement and an opaque Vulkan instance-extension/surface bridge. Vulkan types
do not leak into the generic header.

`GLContext::instantiate()` initializes the selected platform, initializes the
linked renderer and installs its sink. Exit order is renderer shutdown, window
destruction, then platform shutdown. The production executable and all GPU
fixtures use this same path. SDL initialization retains the production video,
events, timer and audio subsystems and is idempotent.

The current SDL2 implementation proves the selector and ownership seam, but it
is not a completed console port. SDL event translation still feeds the existing
LWJGL-shaped keyboard/mouse queues directly, and the shared OpenGL context
helper obtains an `SDL_Window` through the SDL2-private platform header. A
non-SDL platform must replace those private input/OpenGL couplings while
preserving the generic lifecycle and LegacyGL boundary.

## Native compatibility OpenGL - working, and the oracle

`src/backends/NativeGL/Backend.cpp` forwards every frontend call to the loaded
compatibility entry point in the order the game issued it. It is the reference
every translated backend is compared against.

Verified by running the game: title screen, world load, in-world rendering,
pause menu and quit, with no GL errors reported by
`Minecraft::checkGlError`. See `parity-testing.md`.

It also answers the validation hooks - `queryFloatv` and `queryError` - so the
semantic core's answers can be diffed against the driver's. Those hooks never
answer a query the game makes.

## Null and recording backends - working

The null backend renders nothing; the semantic core is unchanged. It lets 65
LegacyGL tests run without a GPU and hands out deterministic object names so the
tests can assert on them. Those tests are part of the currently passing
259-case headless suite.

The recording backend lives in the dispatch tests and asserts that every
inventoried entry point reaches the backend exactly once, in order. It exists
because a semantic core can be perfectly correct while the backend receives
nothing and the screen stays empty.

## OpenGL 4.6 Core - working

`src/backends/OpenGL46` is the first translated backend. It requests and
requires a real 4.6 Core context; a compatibility fallback is rejected. The
verified startup log was NVIDIA 610.88, OpenGL 4.6, `profile=core`.

Implemented:

- VAO/VBO/UBO draw lowering from canonical vertices, with topology conversion
  and explicit provoking attributes;
- a GLSL 460 fixed-function subset covering texturing, two-light lighting with
  colour material, distinct unchanged/rescale/normalize normal behaviour,
  eye-Z and NV radial fog, all eight alpha comparisons, texture matrices and
  flat/smooth primary colour;
- reflected shader-block layout validation, including both light records;
- logical texture and buffer namespaces independent of private Core GL handles,
  plus sparse display-list name allocation in the shared core;
- per-object texture filtering/wrap state, incomplete-texture behaviour and a
  one-texel-gutter representation of legacy `GL_CLAMP`;
- colour/depth-mask- and scissor-enable-aware clears;
- pack-aligned RGB/RGBA/BGR/BGRA/luminance/alpha readback in lower-left row
  order;
- polygon offset and classified line-width fallback;
- no fixed-function entry point. The inventory recursively scans every GL46
  `.cpp` and `.h` and rejects compatibility calls and ARB aliases.

Verification on the current machine:

- all 259 headless cases pass;
- the combined GPU-enabled CTest run passes 7/7 and all 82 native/Core fixture
  case payloads match exactly;
- native and GL46 final-frame traces are byte-identical (1,977 lines,
  SHA-256 `dee8c9a6c13a8658fc0f6dcc07137070b74afcd8ede4f218106812d2731c1cc2`);
- repeated PNGs are byte-identical within each backend;
- the cross-backend 1920x1080 comparison differs at 510 of 2,073,600 pixels
  (0.024594907%), with identical alpha and a sparse edge/raster classification;
- the final GL46 capture exits cleanly under CDB with no unexpected stop or
  exception.

Known limits:

- physical texture storage currently represents level zero as RGBA8. Alpha has
  mipmaps disabled and uses this exercised representation, but uploaded mip
  chains and other legacy internal formats need work before claiming a general
  compatibility implementation;
- `GL_LINE_SMOOTH` is not emulated and wide lines fall back to width 1 if the
  Core driver does not expose the requested width; Alpha does not enable line
  smoothing;
- scissor enable is tracked, but `glScissor` is not in the inventoried stream,
  so the box remains the context default;
- native query validation is unavailable in GL46. Game queries still come only
  from the semantic core;
- framebuffer evidence covers one deterministic scene and one NVIDIA
  GPU/driver. There is not yet a versioned multi-scene golden manifest/corpus.

## Vulkan - working

`src/backends/Vulkan` is the second translated backend. It requires Vulkan 1.1
or newer, obtains the platform's required instance extensions and surface
through the opaque platform bridge, selects graphics/present queues, creates its
device and swapchain, and owns resize/recreation and presentation. Portability
enumeration and the portability-subset device extension are enabled when
advertised. Rendering goes first to an owned `VK_FORMAT_R8G8B8A8_UNORM` colour
target with a depth target that prefers D24S8, then a present pass samples that
target into the selected swapchain image.

Implemented:

- canonical vertex/primitive lowering with explicit flat payloads, cached
  pipeline state, per-draw vertex/uniform storage and descriptor sets;
- the exercised fixed-function shader subset: texturing, two-light colour-
  material lighting, distinct unchanged/rescale/normalize normal modes, eye-Z
  and NV radial fog, all eight alpha comparisons, texture matrices and
  flat/smooth primary colour;
- the single GL-to-Vulkan clip correction in the vertex shader and a negative-
  height viewport for lower-left GL coordinates, without changing the core's GL
  matrices;
- logical texture, buffer and list namespaces, ordered texture uploads, sampler
  state, incomplete-texture behaviour and the same one-texel `GL_CLAMP` gutter
  representation verified in GL46;
- colour/depth-mask-aware clears, including a shader path for partial colour
  masks, and the exercised default-scissor behaviour;
- pack-aligned RGB/RGBA/BGR/BGRA/luminance/alpha readback with format conversion
  and lower-left row order;
- native Vulkan blend, depth, cull and front-face state. Physical-device
  selection requires `logicOp`, because Alpha exercises it, and device creation
  enables it; calibrated polygon depth bias and classified wide-line fallback;
- optional explicit line rasterization, preferring
  `VK_KHR_line_rasterization` and then `VK_EXT_line_rasterization`. When
  Bresenham lines are available it queries `lineSubPixelPrecisionBits` and uses
  one device-subpixel viewport tie bias. Otherwise it reports
  `default-fallback` rather than pretending the raster rule is exact;
- Debug validation-layer integration and a shutdown error count;
- no OpenGL entry point. The inventory scans the Vulkan tree and rejects any
  `gl*` or `glad_gl*` function call.

The six GLSL 450 shaders (legacy vertex/fragment, masked clear, and present) are
compiled to SPIR-V by `glslc` as build dependencies and validated with
`spirv-val` when it is available. CMake discovers the installed SDK through
`find_package`, `VULKAN_SDK`, an explicit `A126_VULKAN_SDK_ROOT`, or the standard
versioned Windows install roots.

Verification on the current machine:

- all 259 headless cases pass and the GPU-enabled CTest run passes 7/7;
- all 82 Vulkan fixture case payloads match NativeGL byte for byte, including
  all ten polygon-offset cases and all four line masks;
- final-frame traces match NativeGL and GL46 byte for byte (1,977 lines,
  SHA-256 `dee8c9a6c13a8658fc0f6dcc07137070b74afcd8ede4f218106812d2731c1cc2`);
- repeated 1920x1080 Vulkan captures are byte-identical, SHA-256
  `56122199e6ef5ba372f3bb9b02a4921a8b80530745941efb6d36f72dc18f0c8e`;
- GL46 and Vulkan differ at 55 of 2,073,600 pixels (0.002652391975%), with
  identical alpha and a sparse edge/raster classification under the existing
  Gate D policy;
- the Debug validation layer reports zero errors at shutdown, and the final
  capture exits cleanly under CDB with no unexpected stop or exception.

Known limits:

- physical sampling currently uses level-zero RGBA8; uploaded mip chains and
  distinct legacy internal formats are not general-purpose complete;
- `GL_LINE_SMOOTH` is not emulated. Unsupported wide lines fall back to width 1
  with a report, and devices without optional Bresenham line support remain
  subject to the existing exact/fallback line classifications;
- scissor enable is tracked, but `glScissor` is not in the inventoried stream,
  so only the default box is exercised. The game also never requests a stencil
  clear; Vulkan does not yet lower `GL_STENCIL_BUFFER_BIT`;
- readback conversion is verified for in-bounds rectangles. A future caller
  that requests pixels outside the target needs explicit clipping before a
  Vulkan image-to-buffer copy;
- the tested Windows surface provides the preferred UNORM format and a variable
  extent. Other WSI colour spaces, mandatory alpha compositing, or fixed/clamped
  extents need backend-specific presentation handling;
- the facade forwards default-enabled dither state, but the verified device does
  not expose `VK_EXT_legacy_dithering`. The backend follows the chosen policy of
  preserving the native oracle and classifying sparse Gate D differences rather
  than inventing a shader dither;
- Vulkan has no compatibility-query validation hooks. Game queries still come
  only from the shared semantic core;
- framebuffer evidence is one deterministic scene on one NVIDIA GPU/driver,
  not a versioned multi-scene, multi-vendor golden corpus;
- the verified runs are Debug correctness runs, with the Vulkan validation
  layer enabled. They do not support a performance conclusion; fair timing is a
  Release build with validation and `A126_LEGACYGL_VALIDATE` off. The current
  correctness-first path also allocates transient vertex and uniform storage per
  draw; a fenced streaming arena is the next performance change, not a semantic
  prerequisite.

## Direct3D 12 - later, not started

Direct3D 12 can consume the same resolved state and primitive/texture
representations through the now-proven backend/platform contract. It has not
been started. The local DirectX PDF set is not an authoritative reference for
that work. Filter the MicrosoftDocs `win32` repository's `docs` branch for the
applicable DirectX/Direct3D source pages and use those Microsoft sources when
the backend starts.

## Milestone check

| requirement | state |
|---|---|
| NativeGL remains the compatibility oracle | yes, selected by default and verified by running the game |
| production backend selection is exact and link-time | yes, `A126_RENDER_BACKEND=NativeGL\|OpenGL46\|Vulkan` |
| platform selection and lifecycle are separate from the renderer | yes, exact `A126_PLATFORM_BACKEND=SDL2` today, with renderer -> window -> platform teardown |
| renderer/game GL call ordering is unchanged | yes, final-frame traces match byte for byte |
| the GL API subset is inventoried | yes, generated and enforced by `a126cpp-gl-inventory` |
| one shared LegacyGL semantic core serves all production backends | yes, `src/legacygl/Context` and resolved commands |
| GL46 runs on a real Core profile without fixed-function calls | yes, verified at startup, by inventory and under CDB |
| Vulkan runs without OpenGL calls | yes, verified by the deny scan, validation layer, capture and CDB |
| focused GPU behaviour matches the oracle | yes, 82/82 GL46 and 82/82 Vulkan case payloads exact on the verified machine |
| deterministic framebuffer comparison exists | yes, preliminary single-scene Gate D with classified differences |
| explicit-API implementation exists | yes, Vulkan; Direct3D 12 remains later |
