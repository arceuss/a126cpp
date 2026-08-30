# Parity testing

Four independent gates. All four now have executable coverage. Gate D has one
deterministic scene and focused GPU fixtures, but not yet the authoritative
multi-scene, multi-driver golden corpus described below.

Production comparisons use one executable and select the provider before
graphics initialization with `--backend nativegl|gl46|vulkan|d3d12`.
`nativegl` remains the default and selection cannot change after startup. The
platform is selected independently; its current exact value is
`-DA126_PLATFORM_BACKEND=SDL2`. The opt-in `A126_ENABLE_GL_GPU_TESTS=ON` build
creates four backend-specific fixture executables, each linked to one provider,
so CTest can record and compare them in one serialized run.

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

Fresh NativeGL, GL46, Vulkan and D3D12 final-frame traces contain 2,165 lines
and 73,251 bytes each, and are byte identical:

```text
c0ce3c8ad48c62118323ac66dc2e79415317dde794f76cad804c6ee5eae9a278
```

Interactive tracing still records the whole run. It is expensive - an earlier
title-screen trace produced 5.9 million lines in 76 seconds - so tracing remains
off unless the variable is set, and each call site costs one load and branch
when it is off.

## Gate B - semantic state and dispatch parity

70 LegacyGL GPU-free tests run inside the 269-case headless suite with
`ctest -R headless`:

| suite | tests | covers |
|---|---|---|
| `legacygl_state` | 19 | context defaults, postmultiplication, stack overflow/underflow, independent stacks, `glColor3f` alpha, byte-normal normalization, alpha reference clamping, all eight comparisons, enable rejection, error latching, call-time light transform, colour material tracking and persistence, fog state versus the NV distance mode, pixel store, queries, degenerate projection volumes, inverse-transpose normals, rescale-normal remaining distinct from normalization |
| `legacygl_lists` | 18 | name reservation, compile versus compile-and-execute, nesting errors, client state not being compiled, compile-time vertex and texture-pixel capture, issue-time unpack alignment, ordered texture definition/sub-image replay, stored-command error timing, unsupplied attributes resolving at execution, immediate-mode vertices reading the list's own colour, immediate-mode errors, nested call ordering, `glCallLists` element types, redefinition and deletion, the nesting limit, texture binds resolving at execution |
| `legacygl_primitives` | 13 | the whole provoking-vertex table, quad and quad-strip conversion, strip winding, fan conversion, line-loop closure, degenerate counts, the `Tesselator` interleaved layout, stride zero, disabled arrays, array validation, immediate-mode per-vertex snapshots, current attributes surviving an array draw |
| `legacygl_textures` | 10 | name states, object defaults, per-object parameter isolation, object zero, deletion rebinding zero, level definition and completeness, subimage clipping, buffer data ownership, buffer-sourced draws, readback validation, clear mask validation |
| `legacygl_dispatch` | 3 | every inventoried entry point reaching the backend exactly once, rejected calls not reaching it at all, the call stream still being forwarded while a display list compiles, and a list execution not re-sending its compiled commands |
| `legacygl_resolved` | 5 | array and immediate draws carrying current resolved state, compile-only lists deferring resolved work, upload identity/alignment/pixels, readback pack state and destination |
| `legacygl_trace` | 2 | capture-frame deferral and relative sequence numbering, exact texture-upload byte footprints without final-row padding |

These drive the real `gl*` entry points with a null or recording backend
installed, so they exercise the same code the game does. They need no GPU and
run in under a second.

`legacygl_dispatch` exists because of a real regression during this work: an
editing mistake removed the `activeSink->drawArrays` forward from
`Context::drawArrays`. Every state test still passed - the semantic core was
correct - and the game rendered an empty sky with no HUD, because nothing was
being submitted. State correctness and dispatch correctness are separate
properties and both need a test.

With `A126_ENABLE_GL_GPU_TESTS=ON`, CMake additionally builds four small
executables, linked separately to NativeGL, OpenGL46, Vulkan and D3D12. Each
records the same 129 cases. Coverage includes all alpha and depth comparisons,
cull/front-face combinations, the exercised blend factors, all 16 logic
operations, texture sampling and legacy clamp, mask-aware clear/readback,
polygon offset, line width, an asymmetric model-view/projection/viewport case,
shader state (texture matrix, colour, two-light colour material, all three
normal modes and linear/exponential fog), logical texture/buffer/list name
collisions, the production 32-byte interleaved client-array layout, display-list
texture definition, and resource lifetime across asynchronous frame-slot reuse.

Two regression cases deserve explicit names. `texture.zero-delete-preserves-default`
proves that deleting name zero does not release a translated backend's physical
default texture. `list.execution-current-color-variants` calls one colourless
display list under red and then green current colour; it reproduces the white-
sign resident-cache failure and requires a residency variant key that includes
execution-time values for attributes absent from captured geometry. All four
recorders now produce `ff000000ff00` for that case.

All six pairwise comparisons pass on the verified machine. Exact cases remain
byte exact; linear texture and smooth shader cases retain only their documented
per-case channel allowances, and D3D12's width-2 output is accepted only when it
matches the explicit width-1 fallback classification.

The fixture creates the real driver context and surface in an SDL hidden window
but never shows it. A focused unattended run is:

```text
cmd.exe /c b.bat test -L renderer-parity
```

It still requires a working GPU and display driver; "headless" here means no
visible or interactive UI, not a software renderer with no display system.

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

The focused renderer-parity label passes 10/10: four backend record steps and
all six pairwise comparisons. The complete serialized Release CTest run passes
12/12, including the GPU-free headless and inventory gates.

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

| result | native | OpenGL 4.6 Core | Vulkan | D3D12 |
|---|---|---|---|---|
| repeated 1920x1080 PNG SHA-256 | `824e477c0b6725197db04a78f574b6aa583bdd966c1a3b04fbdd474dc2d04159` | `e19d921844011aa2c508be3ec5bf1a56d9c91d563cf91b77c8ac3f43da668db3` | `7a8794f8a5ffb3d2da87a8eda65f9340727e43ed30828bd9d0035ebbf5ffdba9` | `630dc77d443cfeb74cc2e7ac2d48942affd0bd79c512acec1f2c2d2d1616b17e` |
| repeat determinism | byte-identical | byte-identical | byte-identical | byte-identical |

| pair | mismatched pixels | percent | alpha |
|---|---:|---:|---|
| NativeGL - GL46 | 512 | 0.024691358% | exact |
| NativeGL - Vulkan | 490 | 0.023630401% | exact |
| GL46 - Vulkan | 55 | 0.002652392% | exact |
| NativeGL - D3D12 | 2,211 | 0.106626157% | exact |
| GL46 - D3D12 | 1,812 | 0.087384259% | exact |
| Vulkan - D3D12 | 1,773 | 0.085503472% | exact |

NativeGL, GL46 and Vulkan retain their sparse rasterization/interpolation edge
pattern. D3D12's additional differences are sparse edge/line components; its
width-2 line behaviour is the fixture's explicit width-1 fallback class. These
are measured classifications, not a global epsilon or a widened semantic
tolerance. The frontend trace for every image is identical, with SHA-256
`c0ce3c8ad48c62118323ac66dc2e79415317dde794f76cad804c6ee5eae9a278`.

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

Current backend verification is driven by hidden fixtures and logs, without UI
automation. Earlier NativeGL and GL46 manual checks established the menu paths;
the full deterministic world capture now runs through all four providers:

| check | result |
|---|---|
| title screen | renders: background texture, logo built from display lists and `Tesselator` batches, rotating logo blocks, buttons, font with colour codes |
| world load and in-world rendering | renders: terrain chunk display lists, sky, clouds, leaves under alpha test, held item with rescale-normal lighting, hotbar, hearts, crosshair |
| pause menu over the world | renders: darkening gradient with smooth shading and blending, night lighting |
| `Minecraft::checkGlError` | no GL errors reported at startup, pre-render or post-render |
| hidden deterministic capture | NativeGL, GL46, Vulkan and D3D12 render the full 1920x1080 scene and exit cleanly |
| graceful quit | clean exit, validation summary printed |
| GL46 context | NVIDIA 610.88, OpenGL 4.6, `profile=core`; compatibility fallback refused |
| Vulkan runtime | loader 1.4.357, NVIDIA device API 1.4.341, `line rasterization=bresenham, subpixelBits=8`; Debug validation shutdown count zero |
| D3D12 runtime | full Debug 1920x1080 capture reports zero validation errors; 463 messages are performance-only clear warning ID 820 (459 in the representative smaller run) |
| serialized Release CTest | 12/12, including four records and all six pairwise GPU comparisons |

The validation runs are not performance measurements. Backend performance is
measured separately in Release with validation, tracing and
`A126_LEGACYGL_VALIDATE` off and with windows hidden.

## Dithering policy

Legacy GL enables dithering by default and the renderer never changes it.
NativeGL and GL46 preserve driver behaviour; the verified Vulkan device does
not expose `VK_EXT_legacy_dithering`, and D3D12 has no equivalent pipeline
switch. One policy must be selected before authoritative framebuffer goldens:

1. preserve each API's native behaviour, record the capability and classify
   measured dither-dependent pixels;
2. implement one canonical shader/post-pass dither for translated backends and
   define how it is compared with the native oracle;
3. add an explicit parity-fixture mode that suppresses backend dithering
   consistently, without changing normal gameplay.

No option is selected yet. Current Gate D evidence therefore remains
preliminary multi-backend measurement, not a widened tolerance or a final
multi-driver golden.
