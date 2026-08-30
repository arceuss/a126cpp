# Semantic notes

Decisions the semantic core makes, why, and what evidence backs them. Anything
measured is labelled with how it was measured. Anything unresolved says so.

## Measured against the native driver

Measurements below come from `A126_LEGACYGL_VALIDATE=1` sessions on:

| | |
|---|---|
| GPU | NVIDIA GeForce RTX 5070 |
| OS | Windows 10 IoT Enterprise LTSC 2021 (10.0.19044) |
| context | SDL2 requests OpenGL 1.1 compatibility; the driver returns a full compatibility context |
| build | MSVC 14.44.35207, x64, Debug |
| workload | title screen, world load, in-world rendering, pause menu, quit |

### Current attributes after an array draw

OpenGL leaves the current colour, normal and texture coordinate **undefined**
after a client-array draw whose corresponding array was enabled. Alpha depends on
whatever the driver leaves behind, so the behaviour was measured rather than
assumed.

Result, from a full session (title screen, world load, gameplay, pause menu,
graceful quit):

```text
post-array current colour in the backend: 676199 preserved the pre-draw value,
0 took the last array element, 0 were neither
```

The driver preserves the pre-draw value, in every one of 676199 draws. The
semantic core does the same: it never writes a value it did not receive from an
explicit setter. It does mark the attribute indeterminate, purely so validation
can report how often the renderer leans on the behaviour:

```text
indeterminate current attribute used 482529 times:
0 colour, 2753 normal, 479776 texture coordinate
```

Colour is never used indeterminately: the renderer always calls a `glColor*`
before the next draw that needs one. Texture coordinates are, heavily, but almost
always with texturing disabled, where the value has no effect.

**Consequence for translated backends:** an attribute a draw does not supply
must be resolved from the preserved current value. `Context::executeGeometry`
already does this when a backend asks for canonical geometry. A backend that
substituted the last array element instead would diverge from the oracle.

### Matrix precision

Query comparison over the same session, 11028 comparisons:

| query | component | divergence |
|---|---|---|
| `GL_PROJECTION_MATRIX` | 10 | exactly 1 ulp, every frame |
| `GL_MODELVIEW_MATRIX` | 0, 1, 2, 8, 9, 10 | 3 to 4 ulp |

The projection element is `-(zFar + zNear) / (zFar - zNear)`. For the values the
game uses (`zNear = 0.05f`, `zFar = 256`) the exact quotient is
`-1.0003907013146724`, which lies between the floats `0xBF800CCD` and
`0xBF800CCE`. Correctly rounded is `0xBF800CCD`, which is what the core produces;
the driver returns `0xBF800CCE`. Every plausible arrangement of the expression -
float or double operands, division or reciprocal multiply, negation before or
after - was checked and all produce the correctly rounded value, so the driver's
matrix path is doing something approximate (most likely an SSE reciprocal). The
core keeps the correctly rounded result. Reproducing one vendor's approximation
would be over-fitting and would not transfer to AMD, Intel or Mesa.

The model-view divergences are the rotation submatrix. OpenGL does not define
the precision of `glRotatef`'s sine and cosine. Computing them in double and
narrowing only the finished elements reduced the divergence from 7 ulp to 4 ulp,
so that is what `legacygl::rotation` does.

Consequences: `Frustum` culling planes differ from the driver's matrices in the
seventh significant digit, which cannot move a culling decision that matters.
The translated backends render from the core's matrices, so their vertex
positions can differ from the oracle's by the same amount - orders of magnitude
below a pixel. `parity-testing.md` records this as a documented tolerance class
rather than something to hide behind a global epsilon.

### Translated-backend measurements

OpenGL 4.6, Vulkan and D3D12 were exercised on the same RTX 5070. The GL46
startup log reported NVIDIA driver 610.88, OpenGL 4.6 and `profile=core`;
compatibility fallback is explicitly rejected. Vulkan reported loader 1.4.357
and device API 1.4.341, plus `line rasterization=bresenham, subpixelBits=8`.
Vulkan Debug runs enabled `VK_LAYER_KHRONOS_validation` and shut down with zero
validation errors. A full Debug D3D12 capture also reported zero validation
errors; its 463 messages were performance-only clear warning ID 820.

Each backend-specific GPU fixture records the same 129 alpha, texture, clamp,
clear, readback, polygon, line, cull, depth, blend, logic-op, transform, shader,
array, frame-lifetime and logical-object cases. All six pairwise comparisons
pass on this machine. That includes distinct none/rescale/normalize signatures,
all 16 logic operations, client- and buffer-backed 32-byte interleaved arrays,
texture redefinition across asynchronous frame-slot reuse and deleting texture
name zero without changing its default object. Exact cases remain exact;
documented interpolation allowances and the width-2 line fallback remain
case-specific classifications rather than a global tolerance.

The deterministic 1920x1080 captures produced byte-identical PNGs on repeated
runs of each backend. The six pairwise mismatch counts range from 55 pixels for
GL46-to-Vulkan to 2,211 for NativeGL-to-D3D12, and alpha is exact in every pair.
D3D12's differences are sparse edge/line components, including its classified
width-2-to-width-1 fallback; they are not a state divergence or a reason to
widen a tolerance. The final-render traces are byte-identical across all four
backends; the complete measurements and hashes are in `parity-testing.md`.

## Specification decisions

### Provoking vertices

Flat shading takes the primary colour from a topology-specific provoking vertex,
so decomposing a legacy primitive has to carry the *original* primitive's
provoking vertex into every generated triangle or line.
`legacygl::canonicalizePrimitives` implements the OpenGL 1.1 flatshading table
(section 2.7):

| topology | provoking vertex of primitive i (0-based) |
|---|---|
| points | i |
| lines | 2i+1 |
| line strip | i+1 |
| line loop | i+1, and 0 for the closing segment |
| triangles | 3i+2 |
| triangle strip | i+2 |
| triangle fan | i+2 |
| quads | 4i+3, for both generated triangles |
| quad strip | 2i+3 |
| polygon | 0 |

Two entries are worth calling out.

`GL_QUADS` is the case a naive conversion gets wrong: splitting quad
`0,1,2,3` into `(0,1,2)` and `(0,2,3)` and using each triangle's own last vertex
gives the first half vertex 2's colour instead of vertex 3's.

`GL_POLYGON` provokes from the **first** vertex, not the last. The fact-checked
research pack's table says "final vertex"; the OpenGL 1.1 specification's
flatshading table says vertex 1. The specification is followed here. Nothing in
the renderer uses `GL_POLYGON`, so there is no oracle capture either way, and
this is the one provoking rule in the table with no in-game evidence behind it.

### Attribute normalization

OpenGL 1.1 table 2.6 converts signed fixed point with `(2c+1)/(2^b - 1)`, not the
`c/(2^(b-1)-1)` rule OpenGL 4.2 adopted. So `glNormal3b(127, 0, -128)` gives
`(1.0, 1/255, -1.0)`, and `Tesselator`'s packed byte normals decode the same way
through `glNormalPointer`. Unsigned colours use `c/255`.

This matters because `Tesselator::normal` packs x with `x * 128.0f` and y/z with
`* 127.0f` - a quirk of the original code that the port reproduces. Both the
immediate-mode and array paths must decode it identically, which they do:
`Geometry.h` has the one conversion.

### Legacy GL_CLAMP

`GL_CLAMP` is stored as `GL_CLAMP`. It is not rewritten to `GL_CLAMP_TO_EDGE`:
with linear filtering, legacy clamping blends against the border colour at the
edge, and edge clamping repeats the edge texel instead. `Textures.cpp` selects it
for `%clamp%` resources.

All three translated backends represent this with a one-texel gutter around level
zero and remap a clamped source coordinate with
`(u * width + 1) / (width + 2)` (and the corresponding height expression). The
upper endpoint is biased down by one ulp so nearest filtering still selects the
last source texel. Repeat and clamp can be selected independently per axis. This
matched the focused native fixture in all three backends; using
`GL_CLAMP_TO_EDGE`, or remapping to a half-texel range without a gutter, did
not.

### Texture object defaults

A freshly created legacy 2D texture minifies with `GL_NEAREST_MIPMAP_LINEAR` and
magnifies with `GL_LINEAR`, wraps `GL_REPEAT` on both axes and has a transparent
black border. The mipmapped default matters: a level-zero-only object is
*incomplete* until the application selects a non-mipmapped filter.
`Textures::loadTexture` does exactly that, immediately after binding.
`TextureObject::complete()` implements the completeness rule so a backend cannot
silently sample level zero of an incomplete texture.

Names have three states: unused, reserved by `glGenTextures`, and an object
created by the first bind. Binding an unused nonzero name also creates an object.
Deleting the bound object rebinds zero; deleting zero or an unknown name is
ignored. The GPU case `texture.zero-delete-preserves-default` was added after it
caught a translated-backend regression that released the physical default
texture for name zero; all four recorders now preserve the same texel before and
after that delete.

### Matrix stack depths

The stacks use the OpenGL minimums: 32 model-view, 2 projection, 2 texture.
Alpha pushes the projection matrix at most one deep
(`GameRenderer::renderItemInHand`, `TitleScreen`), so the minimum is enough, and
a program that exceeded it would be relying on driver-specific headroom.
Overflow and underflow set `GL_STACK_OVERFLOW`/`GL_STACK_UNDERFLOW` and leave the
stack untouched.

### Matrix element precision

The stacks hold single-precision floats because that is what `glGetFloatv`
returns and what `Frustum.cpp` consumes. `glOrtho`, `glFrustum` and `glScaled`
take doubles; their arithmetic runs in double and only the finished elements are
narrowed. `glScaled` therefore loses the difference between a double scale factor
and its float rounding, which is the same thing any driver storing float matrices
does.

### Display lists

- `GL_COMPILE` records without executing; `GL_COMPILE_AND_EXECUTE` does both.
- Client array enables and pointer setters are **not** compiled; they execute
  immediately even while a list is open. `Font`'s glyph lists depend on this:
  the `Tesselator` batch inside the list sets pointers and enables that must not
  be replayed.
- A compiled `glDrawArrays` copies the referenced vertices at compile time.
  Mutating or freeing the source afterwards cannot change the list.
- Immediate-mode commands are compiled individually, not captured as finished
  geometry, so vertices read the current attributes the list installs while it
  runs.
- `glTexImage2D` and `glTexSubImage2D` are list-compilable. Their pixel bytes and
  issue-time unpack alignment are copied into the list; no caller pointer is
  retained. The copy includes padding between rows but not padding after the
  final row: `(height - 1) * alignedRowStride + rowBytes`.
- Texture binding, level definition and sub-image bounds are resolved when the
  list executes. This lets a list bind a first-use texture, define its image and
  then patch it in command order. `GL_COMPILE` changes no texture state;
  `GL_COMPILE_AND_EXECUTE` applies the upload once immediately and records it for
  later calls.
- Nesting is capped at the OpenGL minimum of 64; exceeding it sets
  `GL_STACK_OVERFLOW` instead of recursing without bound.
- `glCallLists` decodes the six integer element types. `GL_2_BYTES`,
  `GL_3_BYTES` and `GL_4_BYTES` are rejected; the renderer only uses
  `GL_UNSIGNED_INT`. `glListBase` is not in the surface, so the list base is
  always zero.

### Display-list vertex capture and the active backend

Capturing vertices costs memory that mirrors what the driver already stores. A
chunk list holds up to 4680 vertices, and a far render distance keeps thousands
of chunk lists alive, so capture is conditional on the active backend asking for
canonical geometry. The native oracle does not, and pays nothing; all three
translated backends do, and get the data the specification requires.

The consequence is that the backend has to be installed before display lists are
built. `GLContext::instantiate()` does that before any renderer code runs. The
provider is selected by `--backend` before that call; switching after startup is
not supported.

Captured geometry may omit colour, normal or texture coordinates and resolve
those attributes from current state at list execution. A resident backend cache
must therefore key each geometry variant by the execution-time values of every
missing attribute as well as the residency identity. The
`list.execution-current-color-variants` GPU case directly checks this by drawing
one colourless list under red and then green current colour.

### Deterministic capture timing

The final capture render freezes `currentTimeMillis` while the call stream is
recorded. `renderHit` otherwise embeds wall-clock time in a texture transform,
which makes two semantically identical final-frame traces differ. Setup, warm-up
and chunk preparation still run normally, and regular gameplay never uses the
frozen clock. This is capture-fixture control, not a renderer behaviour change.

### Dithering

Legacy GL enables dithering by default, the facade tracks that state, and the
renderer never changes it. NativeGL and GL46 preserve the driver's behaviour.
Vulkan receives the enabled state in resolved draws, but the verified RTX 5070
does not expose `VK_EXT_legacy_dithering`; D3D12 likewise has no equivalent
fixed-function pipeline switch. Neither backend invents a shader dither.
The parity policy is still unresolved, so current Gate D measurements record
the API/device behaviour without treating sparse differences as a widened
tolerance. The choices that must be settled before framebuffer goldens are
listed in `parity-testing.md`.

### Polygon offset and line width

The OpenGL 4.6 backend applies the canonical polygon-offset factor and units at
draw lowering. Vulkan maps the canonical units to `depthBiasConstantFactor`, the
factor to `depthBiasSlopeFactor`, and uses a zero clamp. D3D12 lowers the same
canonical values into its rasterizer state. All ten coplanar cases pass all six
backend comparisons on the tested NVIDIA configuration. A future API/device
combination still has to pass that calibration; this result is not a portable
constant asserted without evidence.

For Vulkan lines, device creation prefers `VK_KHR_line_rasterization`, then
`VK_EXT_line_rasterization`, and enables explicit Bresenham rasterization only
when the feature is advertised. It queries `lineSubPixelPrecisionBits` and adds
one device line-subpixel quantum to the negative-height viewport's Y origin for
line draws. That tie bias selects the same side of exact pixel-boundary cases as
the GL diamond-exit rule. Width-1 horizontal/diagonal and width-2
horizontal/diagonal masks all matched the native oracle exactly on the tested
device.

The line-rasterization extension is optional. If neither spelling supplies the
Bresenham feature, Vulkan logs `default-fallback`; Gate B still requires the
width-1 masks to match exactly and only permits the existing width-2
width-one-fallback or one-pixel-boundary classifications. OpenGL 4.6 and Vulkan
use a requested wide line when the device range supports it. D3D12 has no line-
width rasterizer state, reports a requested width greater than 1 once and uses
the explicitly classified width-1 fallback. Line smoothing is not emulated and
is unused by Alpha.

### Rejected rather than approximated

The frontend raises `GL_INVALID_ENUM` for capabilities, enums, pixel-store
controls, texture targets, formats and types outside the inventoried set. A
renderer path that starts using one fails loudly at the `gl-inventory` test or at
`Minecraft::checkGlError`, instead of silently doing nothing inside a backend.
`gl-api-coverage.md` lists the exclusions.

## Unresolved

- **Spot direction transform.** `glLightfv(..., GL_SPOT_DIRECTION, ...)` is
  transformed by the upper-left 3x3 of the model-view matrix at call time. Alpha
  never sets a spot direction, so there is no capture confirming the rule against
  the driver.
- **`GL_POLYGON` provoking vertex.** Specification-derived, no in-game evidence.
- **Mipmaps and internal formats.** The translated texture representations
  sample level zero as RGBA8; they do not yet reproduce uploaded mip chains or
  distinct legacy internal formats. Alpha has `Textures::MIPMAP=false` and its
  exercised uploads use the supported RGBA representation, so neither limitation
  affects the verified scene or fixture.
- **Scissor rectangle.** The call stream contains no `glScissor` call, and the
  resolved ABI therefore carries only the enable plus the context's default
  box. Non-default scissor rectangles need an inventoried call and resolved
  state before any translated backend can claim them.
- **Out-of-bounds readback.** Vulkan and D3D12 readback is verified for in-bounds
  rectangles. A future out-of-bounds `glReadPixels` call needs explicit GL
  clipping and untouched destination regions before an image-to-buffer copy.
- **Dithering policy.** The translated explicit APIs have no verified equivalent
  for the oracle's default-enabled state. The golden policy remains to be
  selected; current sparse image differences do not decide it implicitly.
- **Coverage breadth.** Framebuffer comparison currently covers one deterministic
  scene, one NVIDIA GPU/driver and no authoritative golden corpus. Sparse
  rasterization differences are classified, not promoted into a broad tolerance.
- **Translated query validation.** `A126_LEGACYGL_VALIDATE=1` is an oracle tool
  for the native backend. GL46, Vulkan and D3D12 do not expose compatibility-
  driver query hooks; game queries are still answered only from the shared core.
