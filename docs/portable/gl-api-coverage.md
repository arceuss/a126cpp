# GL API coverage

The frontend surface is defined by what the renderer actually issues, not by a
guess at "OpenGL 1.1". `a126cpp-gl-inventory` scans every source file, compares
the GL-shaped entry points and enums it finds against the declarations in
`src/legacygl/LegacyGL.h`, and fails when the source reaches for something the
frontend does not implement. It runs as the `gl-inventory` ctest target.

Regenerate the usage columns with:

```text
bin/a126cpp-gl-inventory --table
```

The usage columns below are from the latest verified scan. The checker skips
`src/external`, production backend code and platform translation units that
legitimately sit below the frontend boundary. Test calls are included in the
usage counts; the first-use column therefore names a test when no earlier
game/tool call site exists.

`gluPerspective` and `gluFrustum` are project helpers in `src/util/GLU.h`, not
GLU: `gluPerspective` lowers directly to `glFrustum`, and `gluFrustum` has a
declaration but no call site.

## Backend column meaning

- **core** - implemented in the shared semantic core (`legacygl/Context`).
- **native** - forwarded verbatim by the native compatibility-GL backend, which
  is the behavioural oracle.
- **tests** - covered by a `legacygl_*` headless test.

OpenGL 4.6, Vulkan and D3D12 support the complete inventoried stream through
four resolved commands: draw, texture upload, clear and readback. Logical
object names and `glFinish` are handled directly by each sink. Per-entry-point
columns for the translated backends would therefore repeat the core column
without showing where translation actually occurs; the 129-case GPU fixture
and backend deny scans are their coverage gates. The shared/Core OpenGL scans
reject fixed-function and compatibility-era entry points. The Vulkan and D3D12
scans reject every `gl*` or `glad_gl*` function call.

## Entry points

| function | call sites | files | first use | core | native | tests |
|---|---|---|---|---|---|---|
| `glAlphaFunc` | 10 | 5 | client/Minecraft.cpp:145 | yes | yes | yes |
| `glBegin` | 16 | 6 | client/gui/Font.cpp:372 | yes | yes | yes |
| `glBindBufferARB` | 7 | 4 | client/renderer/Tesselator.cpp:58 | yes | yes | yes |
| `glBindTexture` | 54 | 19 | client/gui/Button.cpp:38 | yes | yes | yes |
| `glBlendFunc` | 28 | 14 | client/gui/Gui.cpp:50 | yes | yes | yes |
| `glBufferDataARB` | 7 | 3 | client/renderer/Tesselator.cpp:59 | yes | yes | yes |
| `glCallList` | 27 | 6 | client/model/Cube.cpp:122 | yes | yes | yes |
| `glCallLists` | 8 | 4 | client/gui/Font.cpp:230 | yes | yes | yes |
| `glClear` | 32 | 7 | client/Minecraft.cpp:180 | tracked | yes | yes |
| `glClearColor` | 19 | 6 | client/Minecraft.cpp:192 | yes | yes | yes |
| `glClearDepth` | 9 | 4 | client/Minecraft.cpp:141 | yes | yes | yes |
| `glColor3f` | 13 | 9 | client/renderer/entity/EntityRenderDispatcher.cpp:187 | yes | yes | yes |
| `glColor4f` | 90 | 32 | client/gui/Button.cpp:39 | yes | yes | yes |
| `glColorMask` | 16 | 4 | client/renderer/GameRenderer.cpp:391 | yes | yes | yes |
| `glColorMaterial` | 8 | 5 | client/Lighting.cpp:23 | yes | yes | yes |
| `glColorPointer` | 4 | 3 | client/renderer/Tesselator.cpp:73 | yes | yes | yes |
| `glCullFace` | 6 | 4 | client/Minecraft.cpp:146 | yes | yes | yes |
| `glDeleteLists` | 7 | 5 | client/MemoryTracker.cpp:25 | yes | yes | yes |
| `glDeleteTextures` | 10 | 5 | client/MemoryTracker.cpp:34 | yes | yes | yes |
| `glDepthFunc` | 11 | 5 | client/Minecraft.cpp:143 | yes | yes | yes |
| `glDepthMask` | 27 | 8 | client/renderer/entity/EntityRenderer.cpp:90 | yes | yes | yes |
| `glDisable` | 143 | 30 | client/gui/ChestScreen.cpp:163 | yes | yes | yes |
| `glDisableClientState` | 13 | 4 | client/renderer/Tesselator.cpp:93 | yes | yes | yes |
| `glDrawArrays` | 18 | 5 | client/renderer/Tesselator.cpp:88 | yes | yes | yes |
| `glEnable` | 151 | 30 | client/gui/ChestScreen.cpp:81 | yes | yes | yes |
| `glEnableClientState` | 17 | 5 | client/renderer/Tesselator.cpp:68 | yes | yes | yes |
| `glEnd` | 15 | 6 | client/gui/Font.cpp:432 | yes | yes | yes |
| `glEndList` | 28 | 8 | client/gui/Font.cpp:111 | yes | yes | yes |
| `glFinish` | 16 | 3 | tools/headless/tests/LegacyGLForwardingTests.cpp:278 | forwarded | yes | yes |
| `glFogf` | 17 | 4 | client/renderer/GameRenderer.cpp:672 | yes | yes | yes |
| `glFogfv` | 5 | 4 | client/renderer/GameRenderer.cpp:656 | yes | yes | yes |
| `glFogi` | 13 | 4 | client/renderer/GameRenderer.cpp:669 | yes | yes | yes |
| `glFrustum` | 4 | 3 | tools/headless/tests/LegacyGLForwardingTests.cpp:199 | yes | yes | yes |
| `glGenBuffersARB` | 5 | 4 | client/renderer/Tesselator.cpp:35 | yes | yes | yes |
| `glGenLists` | 22 | 5 | client/MemoryTracker.cpp:10 | yes | yes | yes |
| `glGenTextures` | 15 | 5 | client/MemoryTracker.cpp:19 | yes | yes | yes |
| `glGetError` | 8 | 4 | client/Minecraft.cpp:282 | yes | compared | yes |
| `glGetFloatv` | 11 | 2 | client/renderer/culling/Frustum.cpp:38 | yes | compared | yes |
| `glLightModelfv` | 4 | 3 | client/Lighting.cpp:42 | yes | yes | yes |
| `glLightfv` | 16 | 4 | client/Lighting.cpp:30 | yes | yes | yes |
| `glLineWidth` | 9 | 5 | client/Minecraft.cpp:606 | yes | yes | yes |
| `glLoadIdentity` | 52 | 8 | client/Minecraft.cpp:148 | yes | yes | yes |
| `glLogicOp` | 3 | 2 | client/gui/GuiTextField.cpp:375 | yes | yes | yes |
| `glMatrixMode` | 54 | 9 | client/Minecraft.cpp:147 | yes | yes | yes |
| `glNewList` | 30 | 8 | client/gui/Font.cpp:91 | yes | yes | yes |
| `glNormal3b` | 3 | 3 | client/model/Polygon.cpp:78 | yes | yes | yes |
| `glNormal3f` | 11 | 7 | client/renderer/entity/ArrowRenderer.cpp:47 | yes | yes | yes |
| `glNormalPointer` | 3 | 3 | client/renderer/Tesselator.cpp:79 | yes | yes | yes |
| `glOrtho` | 10 | 6 | client/Minecraft.cpp:184 | yes | yes | yes |
| `glPixelStorei` | 18 | 6 | client/Minecraft.cpp:420 | yes | yes | yes |
| `glPolygonOffset` | 6 | 4 | client/renderer/LevelRenderer.cpp:1132 | yes | yes | yes |
| `glPopMatrix` | 65 | 32 | client/gui/ChestScreen.cpp:76 | yes | yes | yes |
| `glPushMatrix` | 64 | 32 | client/gui/ChestScreen.cpp:73 | yes | yes | yes |
| `glReadPixels` | 12 | 5 | client/Minecraft.cpp:421 | validated | yes | yes |
| `glRotatef` | 108 | 25 | client/gui/ChestScreen.cpp:74 | yes | yes | yes |
| `glScaled` | 2 | 2 | client/renderer/GameRenderer.cpp:262 | yes | yes | yes |
| `glScalef` | 55 | 29 | client/gui/DeathScreen.cpp:51 | yes | yes | yes |
| `glShadeModel` | 12 | 7 | client/gui/GuiComponent.cpp:46 | yes | yes | yes |
| `glTexCoord2f` | 16 | 5 | client/gui/Font.cpp:422 | yes | yes | yes |
| `glTexCoordPointer` | 3 | 3 | client/renderer/Tesselator.cpp:67 | yes | yes | yes |
| `glTexImage2D` | 16 | 4 | client/renderer/Textures.cpp:154 | yes | yes | yes |
| `glTexParameteri` | 40 | 4 | client/renderer/Textures.cpp:95 | yes | yes | yes |
| `glTexSubImage2D` | 11 | 4 | client/renderer/Textures.cpp:363 | yes | yes | yes |
| `glTranslatef` | 130 | 36 | client/gui/ChestScreen.cpp:79 | yes | yes | yes |
| `glVertex3f` | 44 | 6 | client/gui/Font.cpp:422 | yes | yes | yes |
| `glVertexPointer` | 15 | 5 | client/renderer/Tesselator.cpp:83 | yes | yes | yes |
| `glViewport` | 11 | 6 | client/Minecraft.cpp:190 | yes | yes | yes |
| `gluFrustum` | 1 | 1 | util/GLU.h:6 | helper | helper | no |
| `gluPerspective` | 6 | 4 | client/renderer/GameRenderer.cpp:263 | helper | helper | no |

Column notes:

- `glClear` is **tracked**, not emulated: the core validates the mask, records
  the clear values and forwards the call. It has no framebuffer of its own, so
  mask- and scissor-aware clearing is a backend obligation.
- `glFinish` is **forwarded**. There is nothing to reorder in the core.
- `glGetError` and `glGetFloatv` are answered from the core. **compared** means
  the native backend's answer is diffed against the core's when
  `A126_LEGACYGL_VALIDATE=1`; it is never used to answer the query.
- `glReadPixels` is **validated** in the core (format, type, rectangle,
  destination) and performed by the backend.

## Enums

The frontend defines 175 `GL_*` tokens. The surplus over the current usage scan
is the rest of each family the semantic core implements: all eight comparison
functions, all sixteen logic operations, every blend factor, the remaining
light and material parameters and the query names the tests use. Every token in
use is defined, which is what the checker enforces.

Deliberately excluded, and rejected with a GL error if a future renderer path
uses them:

| area | excluded | reason |
|---|---|---|
| pixel store | row length, skip pixels/rows, swap bytes, LSB first | never issued; accepting them without an implementation would corrupt uploads |
| textures | targets other than `GL_TEXTURE_2D`, borders, `glTexParameterfv` border colour, proxy targets | never issued |
| pixel formats | anything other than `GL_UNSIGNED_BYTE` with RGB/RGBA/BGR/BGRA/luminance/alpha | never issued |
| display lists | `glListBase`, `GL_2_BYTES`/`GL_3_BYTES`/`GL_4_BYTES` element encodings | never issued; `glCallLists` uses `GL_UNSIGNED_INT` |
| enables | everything outside the tracked set in `Context::enableSlot` | an untracked enable would silently do nothing |
| buffers | targets other than `GL_ARRAY_BUFFER_ARB` | never issued |

## Below-boundary implementation

The following backend and platform trees are excluded from the Alpha-facing
surface scan because they sit below the facade. Exclusion does not grant them
unrestricted GL access; each translated tree is checked separately:

| file | why |
|---|---|
| `backends/NativeGL/` | the native compatibility backend; this is where frontend calls become compatibility-driver calls |
| `backends/OpenGL/` | OpenGL context creation, loader initialization and profile verification shared by NativeGL and GL46 |
| `backends/OpenGL46/` | the translated backend; only modern Core entry points are permitted here |
| `backends/Vulkan/` | the translated Vulkan backend; no OpenGL entry point is permitted here |
| `backends/D3D12/` | the translated Direct3D backend; no OpenGL entry point is permitted here |
| `backends/Platform/` | platform lifetime, window/event operations and graphics-surface plumbing; it does not expand the frontend GL surface |

`src/legacygl/LegacyGL.h` refuses to compile in a translation unit that also
includes a GL loader header, so this list cannot grow by accident.

The inventory separately scans every `.cpp` and `.h` below the translated
backend trees. It rejects fixed-function entry points, legacy client-array or
display-list calls and compatibility-era ARB aliases in the shared/Core OpenGL
trees, and rejects all GL-shaped function calls in Vulkan and D3D12. The current
deny scan passes for both explicit-API trees. Modern calls made below the OpenGL
boundary do not expand the Alpha-facing inventory.

## Loader

`external/glad` is a glad 0.1.36 loader generated for `gl=4.6` with the
**compatibility** profile and all extensions. One loader covers both the
fixed-function entry points the oracle backend needs and the core-profile entry
points the OpenGL 4.6 backend uses, which avoids linking two loaders whose symbols
would collide. Regenerate with:

```text
python -m glad --profile compatibility --api gl=4.6 --generator c --spec gl --out-path <dir> --reproducible
```

The generator is a build-time convenience, not a project dependency: the
generated `glad.h`/`glad.c` are checked in.

The Vulkan backend is compiled with `VK_NO_PROTOTYPES` and resolves its
exercised global, instance and device functions from the loader entry point
provided by the platform bridge. The SDK remains a header and shader-tool build
requirement, but `Vulkan::Vulkan` is not linked into the production executable;
native, GL46 and D3D12 runtime selections therefore do not load `vulkan-1.dll`.
