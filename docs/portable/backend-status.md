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
src/backends/OpenGL21/Backend.cpp
src/backends/OpenGL33/Backend.cpp
src/backends/OpenGL33/Shaders.{h,cpp}
src/backends/Vulkan/Backend.cpp
src/backends/Vulkan/Shaders.{h,cpp}
src/backends/Vulkan/shaders/*.{vert,frag}
src/backends/D3D12/Backend.cpp
src/backends/D3D12/Shaders.{h,cpp}
```

The production executable links the providers enabled for its platform and
selects exactly one before graphics initialization with
`--backend nativegl|gl21|gl33|vulkan|d3d12`.
`A126_DEFAULT_RENDER_BACKEND` is a compile definition selected by CMake and
defaults to `gl21`; executable filenames are cosmetic and never select a
backend. Unknown, unavailable, duplicate or missing backend arguments fail
before a window or graphics context is created. Selection cannot change after
startup, and only one sink is installed per process.

The Vulkan provider obtains `vkGetInstanceProcAddr` from SDL only after a
Vulkan window is selected. The all-backend executable and the Vulkan parity
recorder do not import `vulkan-1.dll`; hidden module checks confirmed that
native and D3D12 runs leave it unloaded while a Vulkan run loads it on demand.

Choose the platform independently with `A126_PLATFORM_BACKEND=SDL2`. `SDL2` is
currently the default and only accepted exact value; any other value is a
configure error. Renderer selection does not imply platform selection.

`A126_ENABLE_GL_GPU_TESTS=ON` creates separate native, GL2.1, GL33, Vulkan and
D3D12 parity executables. Unlike production, each fixture links exactly one
provider; this keeps recording independent of the run-time selector.

## Portable platform and renderer lifecycle - working

`src/backends/Backend.h` defines configuration, initialize, present, shutdown,
capability and sink operations. `src/backends/Platform/Platform.h` separately
defines platform lifetime, window state, events, drawable size, cursor
placement, an opaque Vulkan instance-extension/surface bridge and a Win32-only
opaque native-window handle. Vulkan and Win32 types do not leak into the
generic header.

Main selects the provider before `GLContext::instantiate()`. That function
initializes the platform, initializes the selected renderer and installs its
sink. Exit order is renderer shutdown, window destruction, then platform
shutdown. The production executable and all GPU fixtures use this same path.
SDL initialization retains the production video, events, timer and audio
subsystems and is idempotent. The SDL2 implementation obtains the HWND with
`SDL_GetWindowWMInfo` and exposes it only through the opaque Win32 bridge used
by D3D12.

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

## OpenGL 2.1 compatibility with VBO residency - working

`src/backends/OpenGL21/Backend.cpp` consumes the same resolved state as the
explicit/Core backends but lowers it through OpenGL 2.1 compatibility
operations. Every transient canonical draw uses a reusable `GL_STREAM_DRAW`
VBO. Nonzero display-list residency identities use per-current-attribute
variant `GL_STATIC_DRAW` VBOs and are deleted on list redefinition, deletion or
context teardown. NativeGL remains a separate raw-call sink and pays no
canonical decoding or VBO-cache cost.

Current direct evidence:

- the 153-case GL2.1 record compares successfully with NativeGL, including
  current-colour list variants, format conversion, release and line cases;
- two repeated 1920x1080 captures are byte-identical at SHA-256
  `b86792a82bcff507fa4828e9c242d342aee709b5ca6641cc075b56c70436cc84`;
- that capture differs from the unchanged NativeGL oracle at 477 of 2,073,600
  pixels (0.023003472%), with exact alpha and the existing sparse
  edge/interpolation classification;
- the final-frame NativeGL and GL2.1 traces are byte-identical at SHA-256
  `d6e7ddc855bbab312e19d0ee30a4cfa2ff62621ea65f9165421a503e6ffd525f`;
- the hidden capture observed 3,809 static resident uploads, 101,708 cache hits
  and 3,809 explicit releases. The equivalent repeat varied only in
  chunk-preparation counts while producing the same PNG.

## Null and recording backends - working

The null backend renders nothing; the semantic core is unchanged. It lets 71
LegacyGL tests run without a GPU and hands out deterministic object names so the
tests can assert on them. Those tests are part of the currently passing
274-case headless suite.

The recording backend lives in the dispatch tests and asserts that every
inventoried entry point reaches the backend exactly once, in order. It exists
because a semantic core can be perfectly correct while the backend receives
nothing and the screen stays empty.

## OpenGL 3.3 Core - working

`src/backends/OpenGL33` is the first translated backend. It requests and
requires a real 4.6 Core context; a compatibility fallback is rejected. The
verified startup log was NVIDIA 610.88, OpenGL 3.3, `profile=core`.

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
- the shared `legacygl::PixelFormat` table distinguishes intended RGB/RGBA
  semantics from physical RGBA8 storage and owns all seven unsigned-byte
  upload/readback conversions;
- startup rejects a missing 4.6 Core function set or unsupported `GL_RGBA8`
  texture path and emits one machine-readable capability/fallback record;
- polygon offset and classified line-width fallback;
- no fixed-function entry point. The inventory recursively scans every GL33
  `.cpp` and `.h` and rejects compatibility calls and ARB aliases.

Verification on the current machine:

- all 274 headless cases pass;
- the combined Release CTest run passes 17/17 and the 153-case NativeGL/GL33
  fixture comparison passes;
- all four final-frame traces are byte-identical (2,165 lines, 73,251 bytes,
  SHA-256 `c0ce3c8ad48c62118323ac66dc2e79415317dde794f76cad804c6ee5eae9a278`);
- repeated PNGs are byte-identical within each backend;
- the cross-backend 1920x1080 comparison differs at 512 of 2,073,600 pixels
  (0.024691358%), with identical alpha and a sparse edge/raster classification;
- the final hidden GL33 capture exits cleanly.

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
- native query validation is unavailable in GL33. Game queries still come only
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
  pipeline state, retained mapped streams for dynamic geometry and a device-
  resident display-list geometry cache. Its variant key includes execution-time
  current colour, normal and texture coordinates when captured geometry omits
  those attributes, preventing the reproduced white-sign cache alias;
- the exercised fixed-function shader subset: texturing, two-light colour-
  material lighting, distinct unchanged/rescale/normalize normal modes, eye-Z
  and NV radial fog, all eight alpha comparisons, texture matrices and
  flat/smooth primary colour;
- the single GL-to-Vulkan clip correction in the vertex shader and a negative-
  height viewport for lower-left GL coordinates, without changing the core's GL
  matrices;
- logical texture, buffer and list namespaces, ordered texture uploads, sampler
  state, incomplete-texture behaviour and the same one-texel `GL_CLAMP` gutter
  representation verified in GL33;
- colour/depth-mask-aware clears, including a shader path for partial colour
  masks, and the exercised default-scissor behaviour;
- pack-aligned RGB/RGBA/BGR/BGRA/luminance/alpha readback with format conversion
  and lower-left row order;
- physical-device selection requires RGBA8 optimal-tiling sampled, linear-
  filter, colour-attachment, blend, transfer-source and transfer-destination
  support before the backend relies on that representation;
- each owned image records its current layout, access mask and pipeline stages.
  The transition helper derives barriers from that production state and updates
  it only when the command is recorded; frame slots retain retired images until
  their fence completes;
- startup emits one machine-readable record naming the chosen RGBA8, logic-op,
  line-rasterization, wide-line, dither and dynamic-state paths;
- native Vulkan blend, depth, cull and front-face state. Physical-device
  selection requires `logicOp`, because Alpha exercises it, and device creation
  enables it; calibrated polygon depth bias and classified wide-line fallback;
- optional explicit line rasterization, preferring
  `VK_KHR_line_rasterization` and then `VK_EXT_line_rasterization`. When
  Bresenham lines are available it queries `lineSubPixelPrecisionBits` and uses
  one device-subpixel viewport tie bias. Otherwise it reports
  `default-fallback` rather than pretending the raster rule is exact;
- Debug validation-layer integration and a shutdown error count;
- uncapped presentation prefers advertised immediate mode, then mailbox, with
  mandatory FIFO as the final fallback. Present-wait semaphores are indexed by
  acquired swapchain image so they can be reused without a per-frame queue-idle;
- no OpenGL entry point. The inventory scans the Vulkan tree and rejects any
  `gl*` or `glad_gl*` function call.

The six GLSL 450 shaders (legacy vertex/fragment, masked clear, and present) are
compiled to SPIR-V by `glslc` as build dependencies and validated with
`spirv-val` when it is available. CMake discovers the installed SDK through
`find_package`, `VULKAN_SDK`, an explicit `A126_VULKAN_SDK_ROOT`, or the standard
versioned Windows install roots.

Verification on the current machine:

- all 274 GPU-free headless cases and the complete Release CTest run pass 17/17;
- all four 153-case comparisons involving Vulkan pass, including first-use
  texture definition inside a display list, texture redefinition across
  asynchronous frame-slot reuse, current-colour resident variants, client- and
  buffer-backed 32-byte interleaved arrays, all ten polygon-offset cases and all
  four line masks;
- final-frame traces match all three other providers byte for byte (2,165 lines,
  73,251 bytes, SHA-256
  `c0ce3c8ad48c62118323ac66dc2e79415317dde794f76cad804c6ee5eae9a278`);
- repeated 1920x1080 Vulkan captures are byte-identical, SHA-256
  `7A8794F8A5FFB3D2DA87A8EDA65F9340727E43ED30828BD9D0035EBBF5FFDBA9`;
- GL33 and Vulkan differ at 55 of 2,073,600 pixels (0.002652392%), with
  identical alpha and a sparse edge/raster classification under the existing
  Gate D policy;
- the Debug validation layer reports zero errors at shutdown, and the final
  hidden capture exits cleanly.

Known limits:

- physical sampling currently uses level-zero RGBA8; uploaded mip chains and
  distinct legacy internal formats are not general-purpose complete;
- `GL_LINE_SMOOTH` is not emulated. Unsupported wide lines fall back to width 1
  with a report, and devices without optional Bresenham line support remain
  subject to the existing exact/fallback line classifications;
- scissor enable is tracked, but `glScissor` is not in the inventoried stream,
  so only the default box is exercised. The game never requests a stencil
  clear; Vulkan nevertheless lowers `GL_STENCIL_BUFFER_BIT` when the selected
  depth format includes stencil;
- readback conversion is verified for in-bounds rectangles. A future caller
  that requests pixels outside the target needs explicit clipping before a
  Vulkan image-to-buffer copy;
- the tested Windows surface provides the preferred UNORM format and a variable
  extent. Other WSI colour spaces, mandatory alpha compositing, or fixed/clamped
  extents need backend-specific presentation handling;
- the facade forwards default-enabled dither state, but the verified device does
  not expose `VK_EXT_legacy_dithering`. The selected golden policy preserves
  this native API behaviour and records `unavailable-no-emulation`;
- Vulkan has no compatibility-query validation hooks. Game queries still come
  only from the shared semantic core;
- framebuffer evidence is one deterministic scene on one NVIDIA GPU/driver,
  not a versioned multi-scene, multi-vendor golden corpus;
- correctness is also checked in Debug with the Vulkan validation layer enabled.
  Fair timing remains Release with validation and `A126_LEGACYGL_VALIDATE` off.
  Draw vertices and uniforms now use retained, persistently mapped 4 MiB stream
  chunks. Cursors reset only after the submission fence completes; staging and
  readback buffers remain transient.
- command recording uses a fixed two-slot frame ring. Command buffers, fences,
  acquire semaphores, descriptor pools and caches, stream cursors, transient
  buffers and retired images are frame-owned. A slot is reclaimed only after its
  fence completes; present-wait semaphores stay indexed by swapchain image.
  `glFinish`, readback, swapchain recreation and shutdown drain both slots.
- on the verified RTX 5070, `--sign-bench 120 256 0` improved from one measured
  pre-arena run at 341.85 ms/frame (2.93 FPS, p95 385.17 ms) to a five-run
  post-arena median of 22.79 ms/frame (43.88 FPS, median p95 31.51 ms). Immediate
  presentation, image-indexed presentation semaphores, the descriptor cache and
  direct mapped-stream vertex expansion reduce the current five-run median to
  18.08 ms/frame (55.31 FPS, median p95 26.31 ms). The same Release fixture with
  zero signs now measures 14.47 ms/frame (69.10 FPS, median p95 21.32 ms).
  Descriptor-cache hit rates were about 99.0% and 97.8%, respectively. The
  fixture intentionally calls `glFinish` every frame, so these measure completed
  work rather than future frames-in-flight overlap and are not a cross-machine
  performance guarantee.

## Direct3D 12 - working

`src/backends/D3D12` is the third translated backend. On Windows it obtains an
opaque HWND from the platform contract, creates the device and a flip-discard
swap chain, and owns a two-slot command allocator/list/fence ring. Rendering
uses an offscreen RGBA8 colour target plus depth; presentation samples the
offscreen target into the acquired swap-chain buffer. Tearing is enabled when
supported for uncapped presentation, and normal present does not wait for the
device to become idle. Hardware adapters are tried in high-performance order
and accepted only when the required output-merger logic operations are
supported; capability-checked WARP is the fallback.

Implemented:

- canonical vertex/primitive lowering, duplicated provoking payloads and an
  HLSL fixed-function subset covering texturing, two-light colour-material
  lighting, distinct unchanged/rescale/normalize normal modes, eye-Z and NV
  radial fog, all eight alpha comparisons, texture matrices and flat/smooth
  primary colour;
- one GL-to-D3D clip-space correction at draw lowering while the shared core's
  matrices remain in GL convention;
- PSO, sampler and descriptor caching. Replaced texture SRV slots return to the
  free list only after the retaining frame fence has completed;
- fence-owned upload allocation and 16 MiB default-heap buffer pages for the
  resident display-list geometry cache. The cache owns each shared range, and
  every frame that references it holds another share. After cache invalidation,
  the range returns to a coalescing free list only once the last referencing
  frame fence has completed. Oversized meshes receive a correspondingly larger
  page. The residency variant key includes execution-time current attributes
  absent from captured geometry, as required by
  `list.execution-current-color-variants`;
- per-object texture state, incomplete-texture handling and the one-texel
  `GL_CLAMP` gutter representation used by GL33 and Vulkan;
- native depth, cull, blend and polygon-offset state. All 16 logic operations
  use a UINT view of the typeless RGBA8 target plus an integer-output shader,
  because UNORM render targets do not support D3D12 output-merger logic ops;
- colour/depth-mask-aware clears and the exercised default-scissor behaviour;
- pack-aligned RGB/RGBA/BGR/BGRA/luminance/alpha readback, format conversion
  and lower-left row order;
- adapter selection requires the typeless RGBA8 resource, sampled/renderable
  UNORM view, renderable UINT logic-op view and D24S8 depth format through
  `CheckFeatureSupport`, in addition to output-merger logic operations;
- texture, resident-buffer and offscreen-target transitions update their
  production state through one tracked helper. Replaced resources, descriptors
  and resident ranges remain retained by the owning frame fence;
- startup emits one machine-readable record naming the selected formats,
  feature level, logic-op, line-width and dither paths;
- no fixed-function or other OpenGL entry point. The backend-specific inventory
  deny scan rejects every `gl*` and `glad_gl*` call.

Verification on the current machine:

- the complete serialized Release CTest run passes 17/17; all four 153-case
  comparisons involving D3D12 pass, including cull/depth/blend/logic state,
  texture-zero deletion and resident current-colour variants;
- an ephemeral fault-injection build reduced the SRV heap to four dynamic
  slots. The 129-case recorder crossed five sequential GPU-backed texture
  lifetimes without exhaustion or validation errors; the production capacity
  was then restored to 16,382 dynamic slots and rebuilt;
- final-frame traces match NativeGL, GL33 and Vulkan byte for byte (2,165 lines,
  73,251 bytes, SHA-256
  `c0ce3c8ad48c62118323ac66dc2e79415317dde794f76cad804c6ee5eae9a278`);
- repeated 1920x1080 captures are byte-identical, SHA-256
  `630dc77d443cfeb74cc2e7ac2d48942affd0bd79c512acec1f2c2d2d1616b17e`.
  Converting the resident cache to page suballocation preserved that exact
  image while packing 625,374,288 peak resident payload bytes into 38 buffers.
  Byte-identical runs observed 3,812 through 3,814 cache misses depending on
  chunk-update timing;
- scene mismatches are 2,211 pixels versus NativeGL, 1,812 versus GL33 and
  1,773 versus Vulkan, with exact alpha. They are sparse edge/line components,
  including the classified width-2 fallback, not a widened tolerance;
- a full Debug 1920x1080 capture reports zero validation errors. Its 463
  messages, and 459 in the representative smaller run, are performance-only
  clear warning ID 820.

Known limits:

- D3D12 is currently Windows-only and depends on the SDL2 platform's HWND
  bridge; this proves the platform seam but is not a console implementation;
- physical sampling currently represents level zero as RGBA8; uploaded mip
  chains and distinct legacy internal formats remain outside verified scope;
- D3D12 has no line-width rasterizer state. Requests greater than 1 are reported
  once and use the fixture's explicit width-1 fallback classification;
- the resolved ABI has no non-default scissor box because the inventoried stream
  contains no `glScissor` call;
- readback is verified for in-bounds rectangles. Out-of-bounds
  `glReadPixels` needs explicit clipping and untouched destination handling;
- debug-layer warning ID 820 reports the non-optimized clear value as a
  performance warning; it is not suppressed and is not counted as an error;
- D3D12 has no compatibility-query validation hooks or dither equivalent.
  Queries remain core-owned; the selected golden policy records
  `unavailable-no-emulation` without adding a shader approximation;
- framebuffer evidence covers one scene and one NVIDIA device/driver rather
  than a multi-scene, multi-vendor golden corpus.

## ANGLE/Zink phase disposition

| design phase | Vulkan | D3D12 |
|---|---|---|
| backend state synchronization | current logical/native pipeline fast path plus exact viewport, scissor, line-width and depth-bias suppression | command-list-scoped PSO, root-signature, topology, viewport, scissor and sampler-table suppression |
| pipeline cache/prewarm | versioned driver cache is loaded, identity-checked, atomically saved and measured cold/warm | the measured World4 corpus creates 16 PSOs in 1.47 ms total over 2,600 frames; persistent libraries/background prewarm are intentionally not added because creation is not the measured frame bottleneck |
| intended/physical formats | shared intended RGB/RGBA table, physical RGBA8 choice and device feature checks | shared intended RGB/RGBA table, typeless/UNORM/UINT physical views and `CheckFeatureSupport` gates |
| resource state/lifetime | tracked image layout/access/stage plus fence-owned images, streams and resident allocations | tracked resource states plus fence-owned descriptors, resources, uploads and resident allocations |
| capability/fallback report | machine-readable format, logic-op, line, dither, dynamic-state, cache and present paths | machine-readable feature-level, format, logic-op, line, dither and presentation paths |

The no-prewarm D3D12 decision is profile-gated, not an unfinished placeholder.
The same diagnostics identify millions of resolved draws and per-draw state
copies as the actual workload cost; resident-geometry borrowing and backend
fast paths address that cost without changing the frontend stream.

## Repeatable subsystem profiling

`tools/Profile-Renderers.ps1` configures one isolated Release client and runs
the same hidden `--sign-bench` workload through the selected providers.
Defaults are five repetitions, 2,000 World4 warm-up frames, 600 measured
frames, generated sign-control/board/text workloads, and both normal and
forced-`glFinish` attribution. One deterministic tick every ten frames gives
every backend the same game-state sequence and tick count. The frame argument
means measured frames; warm-up is added rather than silently consuming the
requested sample count.

Each run preserves its raw backend log and writes a JSONL record containing the
executable hash, backend/device/capability report, workload, resolution,
validation/diagnostic flags, warm-up and measured counts, cache cold/warm class,
frame/tick/light/render/finish distributions, chunk updates, sign mode and the
backend's pipeline, descriptor/sampler, dynamic-state, barrier/transition,
pass-break, upload, retirement, residency and fence counters where applicable.
`profile-summary.csv` reports per-workload medians without discarding the raw
count evidence.

## Comparable Release timing

Five isolated diagnostics-off runs used the same `World4`, 854x480 hidden
window, one tick per ten frames, 2,000 warm-up frames and 600 measured frames.
The medians in
`profile-results/final-world-deterministic/profile-summary.csv` are:

| backend | median mean ms | median p95 ms | median FPS |
|---|---:|---:|---:|
| NativeGL | 2.797180 | 5.1267 | 357.503 |
| GL2.1 resident VBO | 4.790767 | 6.4093 | 208.735 |
| Vulkan, warm driver cache | 5.793105 | 7.1477 | 172.619 |
| D3D12 | 5.245803 | 6.7110 | 190.629 |

The translated targets remain slower than the compatibility oracle, but their
normal frame latency is within 2.0--3.0 ms and all exceed 170 FPS.
This is the selected meaning of the user-requested close-or-better target; no
backend, validation, resolution or workload difference is hidden.

Forced-finish medians are recorded separately in
`profile-results/final-world-finish-deterministic/profile-summary.csv`:

| backend | median mean ms | median p95 ms | median FPS |
|---|---:|---:|---:|
| NativeGL | 3.116589 | 5.4052 | 320.864 |
| GL2.1 | 5.259637 | 6.9315 | 190.127 |
| Vulkan, warm cache | 16.169296 | 49.3351 | 61.846 |
| D3D12 | 43.643555 | 100.7169 | 22.913 |

Those explicit-API stalls are intentionally visible rather than averaged into
normal frame overlap. The diagnostics-enabled sign corpus in
`profile-results/final-sign-subsystems-deterministic` separates control,
blank-board and text costs. Normal median means for
control/boards/text respectively are NativeGL 4.175/4.543/5.410 ms, GL2.1
10.496/16.400/18.067 ms, Vulkan 8.333/9.698/11.947 ms, and D3D12
7.910/9.375/11.633 ms; each raw log retains pipeline, descriptor, transition,
pass, upload, retirement, residency and wait counters.

## Milestone check

| requirement | state |
|---|---|
| NativeGL remains the compatibility oracle | yes, kept separate and verified by running the game; GL2.1 is the default |
| production backend selection is startup-only | yes, `--backend nativegl\|gl21\|gl33\|vulkan\|d3d12` is parsed before graphics initialization; CMake defaults to GL2.1 and filenames are ignored |
| platform selection and lifecycle are separate from the renderer | yes, exact `A126_PLATFORM_BACKEND=SDL2` today, with renderer -> window -> platform teardown |
| renderer/game GL call ordering is unchanged | yes, final-frame traces match byte for byte |
| the GL API subset is inventoried | yes, generated and enforced by `a126cpp-gl-inventory` |
| one shared LegacyGL semantic core serves all production backends | yes, `src/legacygl/Context` and resolved commands |
| GL33 runs on a real Core profile without fixed-function calls | yes, verified at startup, by inventory and by hidden capture |
| Vulkan and D3D12 run without OpenGL calls | yes, verified by both deny scans, validation and hidden captures |
| focused GPU behaviour matches the oracle | yes, all ten 153-case pairwise comparisons pass under the documented exact/tolerant/fallback classes |
| deterministic framebuffer comparison exists | yes, preliminary single-scene Gate D with classified differences |
| explicit-API implementations exist | yes, Vulkan and Direct3D 12 |
