# Parity testing

Four independent gates. All four now have executable coverage. Gate D has one
deterministic scene and focused GPU fixtures, but not yet the authoritative
multi-scene, multi-driver golden corpus described below.

Production comparisons use separate configure/build passes with the exact cache
values `-DA126_RENDER_BACKEND=NativeGL`,
`-DA126_RENDER_BACKEND=OpenGL46` and `-DA126_RENDER_BACKEND=Vulkan`. Backend
choice is link-time, not an environment variable. The platform is selected
independently; its current exact value is `-DA126_PLATFORM_BACKEND=SDL2`. The
opt-in `A126_ENABLE_GL_GPU_TESTS=ON` build instead creates three backend-specific
fixture executables so CTest can record and compare them in one serialized run.

## Gate A - call-stream parity

`A126_LEGACYGL_TRACE=<path>` records every frontend call in order:

```text
1 glGenLists(1)
2 glGenLists(786432)
4 glPushMatrix()
5 glNewList(786434, 4864)
# compile begin list=786434 mode=0x1300
6 glVertexPointer(3, 5126, 32, ptr)
7 glEnableClientState(32884)
8 glDrawArrays(4, 0, 4680)
9 glDisableClientState(32884)
10 glEndList()
# compile end
```

Each line is `<sequence> <function>(<arguments>)`. Pointer arguments print as
`ptr`/`null` rather than an address so two runs compare byte for byte. Pixel
uploads print an FNV-1a hash of the payload after unpack interpretation. Comment
lines mark display-list compile and execute boundaries, GL errors and uses of an
indeterminate current attribute.

What this gate is for: proving that installing a different backend does not
change what the game asks for. A trace captured with the native backend and one
captured with a translated backend must be identical for the same scripted
workload.

For `--capture`, tracing is armed before initialization but enabled only around
the final production render. Setup, warm-up, `updateAllChunks` and readback are
excluded, and sequence numbers are relative to that frame. The capture fixture
also freezes `currentTimeMillis` only for this render so `renderHit` cannot put
wall-clock time into the texture-matrix trace. Normal gameplay is unchanged.

The verified NativeGL, GL46 and Vulkan final-frame traces contain 1,977 lines
and are byte identical:

```text
dee8c9a6c13a8658fc0f6dcc07137070b74afcd8ede4f218106812d2731c1cc2
```

Interactive tracing still records the whole run. It is expensive - an earlier
title-screen trace produced 5.9 million lines in 76 seconds - so tracing remains
off unless the variable is set, and each call site costs one load and branch
when it is off.

## Gate B - semantic state and dispatch parity

65 LegacyGL GPU-free tests run inside the 259-case headless suite with
`ctest -R headless`:

| suite | tests | covers |
|---|---|---|
| `legacygl_state` | 19 | context defaults, postmultiplication, stack overflow/underflow, independent stacks, `glColor3f` alpha, byte-normal normalization, alpha reference clamping, all eight comparisons, enable rejection, error latching, call-time light transform, colour material tracking and persistence, fog state versus the NV distance mode, pixel store, queries, degenerate projection volumes, inverse-transpose normals, rescale-normal remaining distinct from normalization |
| `legacygl_lists` | 14 | name reservation, compile versus compile-and-execute, nesting errors, client state not being compiled, compile-time vertex capture with the source destroyed afterwards, unsupplied attributes resolving at execution, immediate-mode vertices reading the list's own colour, immediate-mode errors, nested call ordering, `glCallLists` element types, redefinition and deletion, the nesting limit, texture binds resolving at execution |
| `legacygl_primitives` | 13 | the whole provoking-vertex table, quad and quad-strip conversion, strip winding, fan conversion, line-loop closure, degenerate counts, the `Tesselator` interleaved layout, stride zero, disabled arrays, array validation, immediate-mode per-vertex snapshots, current attributes surviving an array draw |
| `legacygl_textures` | 10 | name states, object defaults, per-object parameter isolation, object zero, deletion rebinding zero, level definition and completeness, subimage clipping, buffer data ownership, buffer-sourced draws, readback validation, clear mask validation |
| `legacygl_dispatch` | 3 | every inventoried entry point reaching the backend exactly once, rejected calls not reaching it at all, the call stream still being forwarded while a display list compiles, and a list execution not re-sending its compiled commands |
| `legacygl_resolved` | 5 | array and immediate draws carrying current resolved state, compile-only lists deferring resolved work, upload identity/alignment/pixels, readback pack state and destination |
| `legacygl_trace` | 1 | capture-frame deferral and relative sequence numbering |

These drive the real `gl*` entry points with a null or recording backend
installed, so they exercise the same code the game does. They need no GPU and
run in under a second.

`legacygl_dispatch` exists because of a real regression during this work: an
editing mistake removed the `activeSink->drawArrays` forward from
`Context::drawArrays`. Every state test still passed - the semantic core was
correct - and the game rendered an empty sky with no HUD, because nothing was
being submitted. State correctness and dispatch correctness are separate
properties and both need a test.

With `A126_ENABLE_GL_GPU_TESTS=ON`, CMake additionally builds three small
executables, linked separately to NativeGL, OpenGL46 and Vulkan. They record and
compare 82 cases covering all alpha comparisons, texture sampling and legacy
clamp, mask-aware clear/readback, polygon offset, line width, shader state
(texture matrix, colour, two-light colour material, all three normal modes and
fog), and logical texture/buffer/list name collisions. On the verified machine,
all 82 GL46 case payloads and all 82 Vulkan case payloads match the native
records byte for byte.

That exact result does not widen or erase the portable comparison policy.
Linear texture samples retain their one-unit-per-channel allowance; smooth
primary colour, lighting, normal and fog samples retain two; width-2 lines may
only classify as an actual width-1 fallback or a one-pixel boundary. Width-1
line masks remain exact. Vulkan's tested device exposes Bresenham line
rasterization. Using one reported line-subpixel quantum as a viewport tie bias
makes all four horizontal/diagonal masks at widths 1 and 2 exact. If the
optional KHR/EXT line feature is
absent, the backend logs `default-fallback` and the same comparison rules expose
or classify the result rather than granting a new tolerance.

The serialized CTest run passes 7/7: headless, inventory, three backend record
steps, native-versus-GL46 comparison and native-versus-Vulkan comparison.

## Gate C - query and state divergence against the oracle

`A126_LEGACYGL_VALIDATE=1` compares every query the renderer makes against a
backend that exposes oracle query hooks (currently NativeGL), and prints a
summary at exit:

```text
legacygl validation: 11028 queries compared against the backend, 5514 one-ulp and
33084 larger component divergences
  query 0xba7 component 10: 5514 one-ulp, 0 larger, worst 1 ulp
  query 0xba6 component 0: 0 one-ulp, 5514 larger, worst 4 ulp
  ...
  indeterminate current attribute used 482529 times: 0 colour, 2753 normal,
  479776 texture coordinate
  post-array current colour in the backend: 676199 preserved the pre-draw value,
  0 took the last array element, 0 were neither
```

Divergences are classified by units in the last place instead of being averaged
or hidden behind an epsilon. The first divergence larger than one ulp is printed
as it happens, with both bit patterns, because a decimal print of a last-bit
difference is useless.

The current state of this gate is in `semantic-notes.md`: the projection matrix
diverges by exactly one ulp in one element, the model-view rotation submatrix by
three to four, and both are driver arithmetic rather than defects. Everything
else matches exactly.

This gate also answers questions the specification leaves open. The post-array
current-colour counters exist because OpenGL calls that value undefined and the
renderer depends on it; measuring the driver decided the rule.

## Gate D - framebuffer parity

The first deterministic comparison is complete:

| result | native | OpenGL 4.6 Core | Vulkan |
|---|---|---|---|
| repeated 1920x1080 PNG SHA-256 | `a90fa3fba585eaf0e9c24d914d29fcfdbda04b762af41e7f1484255dd0b06cd5` | `ed34bb7054916e3496429d6ff4f17c6da87db08c5b6b0db47684d492f0307092` | `56122199e6ef5ba372f3bb9b02a4921a8b80530745941efb6d36f72dc18f0c8e` |
| repeat determinism | byte-identical | byte-identical | byte-identical |

NativeGL and GL46 differ at 510 of 2,073,600 pixels
(0.024594907%). Alpha is identical. There are 505 8-connected components, 500
of them singletons. The sparse isolated pattern is classified as
rasterization/interpolation edge behaviour; it is not a license for a global
epsilon or for widening semantic tolerances.

GL46 and Vulkan differ at only 55 of 2,073,600 pixels (0.002652391975%), again
with identical alpha and a sparse edge pattern. Their frontend traces are
identical to each other and to NativeGL, with SHA-256
`dee8c9a6c13a8658fc0f6dcc07137070b74afcd8ede4f218106812d2731c1cc2`.
This is an additional classified result under the existing policy, not a new
55-pixel allowance.

This is preliminary Gate D evidence, not a finished golden system. The next
steps remain:

- render deterministic scenes with the native backend and each translated one;
- compare with exact equality where justified, per-channel maximum and mismatch
  counts where interpolation or contraction can move a threshold, and edge masks
  only for rasterization-dependent geometry;
- record the oracle configuration in every golden manifest: commit, resource
  hash, world seed, GPU, driver, OS, context profile, colour/depth formats,
  sample count, window size, vsync, dither policy.

The scene corpus to build, from the renderer paths that exist: terrain, water,
lava, clouds, sky, fog including the nether's radial mode, entities, held items,
third-person view, inventory and container GUIs, font with colour codes, signs
with colour codes, particles, block selection outline, the block-breaking
overlay, transparent blocks, day/night, chunk boundaries, the title screen's
rotating logo blocks.

## Inventory gate

`ctest -R gl-inventory` scans the source for GL-shaped entry points and enums and
fails when one is not covered by the frontend. This is what keeps the surface
honest as the renderer changes; see `gl-api-coverage.md`.

## Manual verification performed

NativeGL and GL46 were verified interactively, and Vulkan was verified with the
full deterministic scene capture rather than tests alone:

| check | result |
|---|---|
| title screen | renders: background texture, logo built from display lists and `Tesselator` batches, rotating logo blocks, buttons, font with colour codes |
| world load and in-world rendering | renders: terrain chunk display lists, sky, clouds, leaves under alpha test, held item with rescale-normal lighting, hotbar, hearts, crosshair |
| pause menu over the world | renders: darkening gradient with smooth shading and blending, night lighting |
| `Minecraft::checkGlError` | no GL errors reported at startup, pre-render or post-render |
| graceful quit | clean exit, validation summary printed |
| GL46 context | NVIDIA 610.88, OpenGL 4.6, `profile=core`; compatibility fallback refused |
| Vulkan runtime | loader 1.4.357, NVIDIA device API 1.4.341, `line rasterization=bresenham, subpixelBits=8`; Debug validation shutdown count zero |
| debugger | final GL46 and Vulkan captures exited cleanly under CDB with no unexpected stop or exception |

Screenshots were captured from the window without activating it, using the
Win32 helper, so the captures are of the real client area.

These correctness runs used Debug builds, and the Vulkan runs also enabled the
validation layer. They are not performance measurements. Backend performance
must be measured in Release with validation and `A126_LEGACYGL_VALIDATE` off.

## Dithering policy

Preserve the native default-enabled behaviour and classify measured framebuffer
differences. Legacy GL enables dithering by default and the renderer never
changes it. Vulkan receives that tracked state, but the verified RTX 5070 does
not expose `VK_EXT_legacy_dithering`; inventing an ordered shader dither or
suppressing the oracle below the facade would change behaviour without
evidence. The policy and actual driver/API capability belong in every golden
manifest. Gate D remains preliminary multi-backend evidence, not a widened
tolerance or a final multi-driver golden.
