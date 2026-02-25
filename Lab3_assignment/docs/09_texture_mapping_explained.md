# Texture Mapping — Theory, Techniques & Code Explanation

This document explains every aspect of texture mapping in the Hover Bus project: what types of mapping are used, how UV coordinates are generated for each primitive, how the shaders process textures, and how each object in the scene is mapped.

---

## Table of Contents

1. [What Is Texture Mapping?](#1-what-is-texture-mapping)
2. [The Texture Pipeline](#2-the-texture-pipeline)
3. [UV Coordinate Generation per Primitive](#3-uv-coordinate-generation-per-primitive)
4. [Texture Loading — `loadTexture()`](#4-texture-loading--loadtexture)
5. [Wrapping Modes](#5-wrapping-modes)
6. [Filter Modes](#6-filter-modes)
7. [Shader Texture Modes](#7-shader-texture-modes)
8. [Per-Object Texture Mapping Summary](#8-per-object-texture-mapping-summary)
9. [Common Pitfalls & Solutions](#9-common-pitfalls--solutions)

---

## 1. What Is Texture Mapping?

Texture mapping is the process of applying a 2D image (the **texture**) onto a 3D surface. Each vertex of the 3D mesh has a **texture coordinate** (also called **UV coordinate**) that tells the GPU which pixel of the image should appear at that vertex.

```
    Texture Image (2D)                 3D Surface
    ┌─────────────┐                   ╱╲
    │(0,1)  (1,1) │                  ╱  ╲
    │             │      UV Map     ╱    ╲
    │   IMAGE     │  ──────────►  ╱ mapped ╲
    │             │              ╱  surface  ╲
    │(0,0)  (1,0) │            ╱──────────────╲
    └─────────────┘
```

**UV Space** is a normalized 2D coordinate system:
- **U** = horizontal axis, range `[0.0, 1.0]`
- **V** = vertical axis, range `[0.0, 1.0]`
- `(0,0)` = bottom-left corner of the image
- `(1,1)` = top-right corner of the image

---

## 2. The Texture Pipeline

> All pipeline steps are implemented across [assignment.cpp](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp), [Primitives.h](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h), [shader.vert](file:///d:/Academics/Graphics/Lab3_assignment/shader.vert), and [shader.frag](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag).

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  1. Load     │     │ 2. Generate  │     │ 3. Bind &    │     │ 4. Sample in │
│  Image File  │────►│  UV Coords   │────►│  Set Params  │────►│  Shader      │
│  (stb_image) │     │  (per vertex)│     │  (wrap,filt) │     │  (textureMode)│
└──────────────┘     └──────────────┘     └──────────────┘     └──────────────┘
```

### Step-by-step (with code locations):

| Step | What Happens | Where in Code |
|------|-------------|---------------|
| 1. Load | `stbi_load()` reads the image file from disk into a CPU buffer | [assignment.cpp:L251–L336](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L251-L336) — `loadTexture()` |
| 2. Upload | `glTexImage2D()` uploads pixel data to a GPU texture object | [assignment.cpp:L321](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L321) |
| 3. Mipmaps | `glGenerateMipmap()` creates downscaled versions for distance rendering | [assignment.cpp:L322](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L322) |
| 4. Parameters | Wrap and filter modes are set with `glTexParameteri()` | [assignment.cpp:L323–L326](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L323-L326) |
| 5. Bind | Before drawing, the texture is bound to a texture unit | Render loop, e.g. [assignment.cpp:L607–L611](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L607-L611) |
| 6. Sample | The fragment shader reads from the texture using the interpolated UV | [shader.frag:L109](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag#L109) — `texture(textureSampler, TexCoord)` |

---

## 3. UV Coordinate Generation per Primitive

> All primitives are defined in [Primitives.h](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h) (445 lines total).

Each primitive class (`Cube`, `Cylinder`, `Cone`, `Sphere`) generates UV coordinates differently based on its geometry. The vertex layout for all primitives is:

```
┌─────────┬───────────┬──────────┐
│ Position │  Normal   │ TexCoord │
│ (x,y,z) │ (nx,ny,nz)│  (s, t)  │
│ 3 floats │ 3 floats  │ 2 floats │
└─────────┴───────────┴──────────┘
         Total: 8 floats per vertex
```

Setup in OpenGL (identical pattern used in all primitives' `init()` methods):
```cpp
// Primitives.h — common vertex attribute setup (e.g. Cube.init() at L76–L86)
// Position attribute (location = 0)
glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
glEnableVertexAttribArray(0);

// Normal attribute (location = 1)
glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
glEnableVertexAttribArray(1);

// Texture coordinate attribute (location = 2)
glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
glEnableVertexAttribArray(2);
```

---

### 3.1 Cube — Planar Mapping (Per-Face)

> **Source**: [Cube.init()](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L24-L91) in `Primitives.h` — lines 24–91.

**Technique**: Each face of the cube gets the full `[0,1] × [0,1]` UV range independently. This is called **planar mapping** or **box mapping**.

```
           (0,1)────────(1,1)
             │              │
             │    TEXTURE   │
             │    IMAGE     │
             │              │
           (0,0)────────(1,0)
```

**Each face has exactly 6 vertices (2 triangles)** with UVs covering the full texture:

```cpp
// Primitives.h:L30–L41 — Front face vertices
// Front face (z = +0.5, normal = 0,0,1)
// Triangle 1:
{-0.5f,-0.5f, 0.5f,  0,0,1,  0,0},   // bottom-left  → UV (0,0)
{ 0.5f,-0.5f, 0.5f,  0,0,1,  1,0},   // bottom-right → UV (1,0)
{ 0.5f, 0.5f, 0.5f,  0,0,1,  1,1},   // top-right    → UV (1,1)
// Triangle 2:
{ 0.5f, 0.5f, 0.5f,  0,0,1,  1,1},   // top-right    → UV (1,1)
{-0.5f, 0.5f, 0.5f,  0,0,1,  0,1},   // top-left     → UV (0,1)
{-0.5f,-0.5f, 0.5f,  0,0,1,  0,0},   // bottom-left  → UV (0,0)
```

**Why this works**: A unit cube spans `[-0.5, 0.5]` in all axes. When scaled (e.g., `scale(3, 3, 3)`), the UV coordinates stay `[0,1]` — the texture stretches to fill each face.

```
                ┌─────────┐
               ╱  Top     ╱│
              ╱  (0,0)   ╱ │
             ╱ to (1,1) ╱  │
            ┌─────────┐   │
            │  Front   │   │
            │  (0,0)   │   ┘
            │ to (1,1) │  ╱
            │          │ ╱
            └─────────┘╱
      Each face: full [0,1]×[0,1] UV
```

**Texture Distortion on Cubes**: When a cube is scaled non-uniformly (e.g., `scale(3, 12, 3)` for a tall building), the texture stretches vertically. This is inherent to planar mapping — each face always maps `[0,1]×[0,1]` regardless of the physical size.

| Scale | Face Size | UV Range | Effect |
|-------|-----------|----------|--------|
| `(1,1,1)` | 1×1 | `[0,1]²` | 1:1 mapping, no distortion |
| `(3,3,3)` | 3×3 | `[0,1]²` | Texture stretched 3× in both directions |
| `(4,12,4)` | 4×12 (side) | `[0,1]²` | Texture stretched more vertically (3:1 ratio) |

---

### 3.2 Cylinder — Cylindrical Mapping

> **Source**: [Cylinder.init()](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L118-L180) in `Primitives.h` — lines 118–180.

**Technique**: The cylinder's side surface is "unwrapped" into a rectangle. The U coordinate wraps around the circumference [0→1], and the V coordinate goes from bottom (0) to top (1).

#### Prerequisite: How Angles Describe Points on a Circle

Before understanding sectors, you need to understand how **angles** work on a circle.

A **unit circle** is a circle with **radius = 1**, centered at the origin `(0, 0)`. Any point on this circle can be described by a single **angle** — the rotation from the positive X-axis, going **counterclockwise**.

The formula to convert an angle to a point is:
- **x = cos(angle)** — how far right/left the point is
- **z = sin(angle)** — how far up/down the point is (in our XZ plane)

```
    THE UNIT CIRCLE — viewed from ABOVE the cylinder (looking down the Y-axis):
    
    We see the cylinder's circular cross-section in the XZ plane.
    The angle is measured counterclockwise from the +X axis.
    
                         +Z axis
                           ↑
                           │
              90°  ●───────│     ← (cos90°, sin90°) = (0, 1)
                 ╱         │
               ╱           │
      135°  ●╱             │
           ╱               │
    ──────●────────────────●──────→  +X axis
    180°  │          (center)     0° start  ← (cos0°, sin0°) = (1, 0)
    (-1,0)│                ╲
          │                  ╲
          │                   ● 315°  ← (cos315°, sin315°) = (0.707, -0.707)
          │                 ╱
          │        270° ●──╱     ← (cos270°, sin270°) = (0, -1)
                           │
                          -Z
    
    Angle goes COUNTERCLOCKWISE: 0° → 90° → 180° → 270° → 360° (=0°)
```

**Concrete examples:**

| Angle | cos(angle) = X | sin(angle) = Z | Where on the circle |
|:-----:|:--------------:|:--------------:|:-------------------:|
| 0° | 1.000 | 0.000 | Rightmost point (start) |
| 30° | 0.866 | 0.500 | Upper-right |
| 90° | 0.000 | 1.000 | Top |
| 180° | -1.000 | 0.000 | Leftmost |
| 270° | 0.000 | -1.000 | Bottom |
| 360° | 1.000 | 0.000 | Back to start (same as 0°) |

> **Key insight**: As the angle sweeps from 0° to 360°, `(cos(angle), sin(angle))` traces out the entire circle. This is how we generate all the points around the cylinder's circumference.

#### How does the top-down view relate to the 3D cylinder?

The cylinder stands upright along the **Y-axis**. When we look **down from above**, we see its circular cross-section in the **XZ plane**:

```
    3D SIDE VIEW:                        TOP-DOWN VIEW (looking down Y-axis):
    
         Y axis (up)                          +Z
         ↑                                     ↑
         │   ┌──────────┐ top cap              │   ╱ ──── ╲
         │   │          │                      │ ╱    ●     ╲
         │   │ CYLINDER │ height               ●  center     ● → +X
         │   │ (side)   │                      │ ╲          ╱
         │   └──────────┘ bottom cap           │   ╲ ──── ╱
         └──────→ X                            │
    
    Y-axis = height (up/down)
    XZ plane = the circular cross-section
    The circle is the SAME at every Y height
```

#### What is a "sector"?

We divide this circle into **N equal pie slices** (default N=36). Each slice is a **sector**:

```
    Top-down view: 12 sectors shown (for clarity):
    Each sector spans 360°/12 = 30°
    
                     +Z
                      ↑
                  2   │   3
                ╱  ╲  │  ╱  ╲
              ╱  1  ╲ │ ╱  4  ╲
            ╱────────╲│╱────────╲
       0  ●────────────●────────────●  5 → +X
            ╲────────╱│╲────────╱
              ╲ 11 ╱  │  ╲  6 ╱
                ╲╱    │    ╲╱
                 10   │    7
                  9   │   8
    
    Sector 0  = from   0° to  30°   (starts on the +X axis)
    Sector 1  = from  30° to  60°   (moving counterclockwise)
    Sector 2  = from  60° to  90°   (approaching the +Z axis)
    ...
    Sector 11 = from 330° to 360°   (completes the loop)
    
    With the actual 36 sectors: each is only 10° wide → smoother circle
```

Each sector has **two boundary angles** — we call them `a0` (start) and `a1` (end). These define the "edges" of that pie slice:

```
    ZOOMING INTO sector i=2 (from 60° to 90°):
    
              +Z
               ↑
               │
      P1 ●─────┤         P0 = point at STARTING angle a0 = 60°
       ╱   ╲   │              (x0, z0) = (cos60°, sin60°) = (0.5, 0.866)
      ╱      ╲ │
     ╱ sector ╲│         P1 = point at ENDING angle a1 = 90°
    ╱   i=2    ╲              (x1, z1) = (cos90°, sin90°) = (0.0, 1.0)
    ────────────● P0 → +X
                │        The sector is the PIE SLICE between a0 and a1.
                │        It spans 30° of the circle.
```

#### From pie-slice to 3D surface strip

Each sector represents a **vertical strip** on the cylinder's side wall — like one plank of a barrel:

```
    3D view of ONE SECTOR's strip on the cylinder wall:
    
          Y = +0.5  (top)
            D────────────────C
            │╲               │     D, C = at the top edge
            │  ╲  Triangle 2 │     A, B = at the bottom edge
            │    ╲           │
            │      ╲         │     A, D are at angle a0 (start)
            │ Triangle 1  ╲  │     B, C are at angle a1 (end)
            │               ╲│
            A────────────────B
          Y = -0.5  (bottom)
    
         angle a0           angle a1
```

#### The sector loop — step by step

> **Source**: [Primitives.h:L122–L141](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L122-L141)

**Setup** — how wide is each sector?
```cpp
// Primitives.h:L122
float sectorStep = 2.0f * M_PI / sectors;
// Full circle = 2π radians = 360°
// With 36 sectors: 2π/36 ≈ 0.1745 radians ≈ 10° per sector
```

**The loop** runs `i = 0, 1, 2, ..., 35`:

**Step 1 — What angle range does this sector span?**
```cpp
// Primitives.h:L126–L127
float a0 = i * sectorStep;       // start angle = i × 10°
float a1 = (i + 1) * sectorStep; // end angle   = (i+1) × 10°
```
```
    i=0:  a0 =  0°,  a1 = 10°    (first sector, starts at +X axis)
    i=1:  a0 = 10°,  a1 = 20°    (next sector, counterclockwise)
    i=9:  a0 = 90°,  a1 = 100°   (near the +Z axis)
    i=35: a0 = 350°, a1 = 360°   (last sector, back to start)
```

**Step 2 — Where are the two edge points on the circle?**
```cpp
// Primitives.h:L128–L129
float x0 = cos(a0), z0 = sin(a0);  // point at start angle
float x1 = cos(a1), z1 = sin(a1);  // point at end angle
```
```
    Example: sector i=0 (a0=0°, a1=10°):
    
         +Z
          ↑       ● (x1,z1) = (cos10°, sin10°) = (0.985, 0.174)
          │      ╱
          │     ╱  ← only 10° apart (very narrow strip)
          │    ╱
          │   ╱ 10°
    ──────●──────● (x0,z0) = (cos0°, sin0°) = (1.0, 0.0)  → +X
       center
    
    These 2 points define the left and right edges of the strip.
```

**Step 3 — What slice of the texture does this sector get?**

Think of it like wrapping a **label around a can**:
```
    The label (texture image):           Wrapped onto the can:
    ┌─────────────────────────┐              ╭──────╮
    │                         │             ╱        ╲
    │    TEXTURE IMAGE        │            │  can     │
    │                         │            │  with    │
    │    U=0 ← left edge      │            │  label   │
    │    U=1 ← right edge     │             ╲        ╱
    └─────────────────────────┘              ╰──────╯
    
    Sector 0  gets the leftmost  1/36 of the label
    Sector 1  gets the next      1/36 of the label
    Sector 35 gets the rightmost 1/36 of the label
```

```cpp
// Primitives.h:L130–L131
float u0 = (float)i / sectors;       // left edge of this sector's texture slice
float u1 = (float)(i + 1) / sectors; // right edge
```

**Why `u = i / sectors`?** Because it's a **fraction**: "how far around the circle are we?"

| Sector `i` | `u0 = i/36` | `u1 = (i+1)/36` | Meaning |
|:----------:|:-----------:|:---------------:|:-------:|
| 0 | 0.000 | 0.028 | First 2.8% of texture |
| 1 | 0.028 | 0.056 | Next 2.8% |
| 9 | 0.250 | 0.278 | Quarter-way around |
| 18 | 0.500 | 0.528 | Halfway around |
| 35 | 0.972 | 1.000 | Last 2.8% |

Each sector gets exactly **1/36 = 2.78%** of the texture. All 36 together = 100%.

For **V** (vertical): `V = 0` at the bottom, `V = 1` at the top. Simple height mapping.

**Step 4 — Build two triangles for this strip:**
```cpp
// Primitives.h:L134–L141
// Each vertex: {posX, posY, posZ,  normalX, normalY, normalZ,  U, V}

// Triangle 1 (lower-left):
{x0, -halfH, z0,  x0, 0, z0,  u0, 0.0f}  // A: bottom-left
{x1, -halfH, z1,  x1, 0, z1,  u1, 0.0f}  // B: bottom-right
{x1,  halfH, z1,  x1, 0, z1,  u1, 1.0f}  // C: top-right

// Triangle 2 (upper-right):
{x1,  halfH, z1,  x1, 0, z1,  u1, 1.0f}  // C: top-right
{x0,  halfH, z0,  x0, 0, z0,  u0, 1.0f}  // D: top-left
{x0, -halfH, z0,  x0, 0, z0,  u0, 0.0f}  // A: bottom-left
```
```
    The UV rectangle for this sector:
    
    D (u0, 1.0) ──────── C (u1, 1.0)     ← top of cylinder (Y = +0.5)
      │  ╲                │
      │    ╲  Tri 2       │
      │      ╲            │
      │        ╲          │
      │    Tri 1  ╲       │
      │             ╲     │
    A (u0, 0.0) ──────── B (u1, 0.0)     ← bottom of cylinder (Y = -0.5)
```

> **Normals**: Each vertex normal = `(x, 0, z)` = its position on the unit circle. This points **outward** from the center axis — correct for cylindrical lighting.

#### All sectors unwrapped together

```
    Cut the cylinder along one edge and lay it flat:
    
           Sector 0    Sector 1    Sector 2              Sector 35
    V=1  ┌──────────┬──────────┬──────────┬─── ─ ───┬──────────┐  ← top
         │  D    C  │  D    C  │  D    C  │         │  D    C  │
         │   ╲      │   ╲      │   ╲      │         │   ╲      │
         │     ╲    │     ╲    │     ╲    │         │     ╲    │
         │  A    B  │  A    B  │  A    B  │         │  A    B  │
    V=0  └──────────┴──────────┴──────────┴─── ─ ───┴──────────┘  ← bottom
        U=0.00    0.028     0.056     0.083                  1.00
        angle=0°   10°       20°       30°                  360°
    
    The ENTIRE texture image stretches across this rectangle.
    Then this rectangle rolls back into a cylinder shape.
```

#### Top/Bottom Caps — Polar UV Mapping

> **Source**: [Primitives.h:L143–L163](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L143-L163)

The caps are flat discs. Each sector creates one **pie-slice triangle** fanning from center:

```cpp
// Primitives.h:L149–L152 — Top cap
{0, halfH, 0,  0, 1, 0,  0.5f, 0.5f}                         // center UV
{x0, halfH, z0, 0, 1, 0, 0.5f + 0.5f * x0, 0.5f + 0.5f * z0} // edge 1
{x1, halfH, z1, 0, 1, 0, 0.5f + 0.5f * x1, 0.5f + 0.5f * z1} // edge 2
```

Formula `UV = (0.5 + 0.5*x, 0.5 + 0.5*z)` maps the unit circle `[-1,1]` into `[0,1]`:
```
   Unit circle (1, 0)  → UV (1.0, 0.5)   — right edge of texture
   Unit circle (0, 1)  → UV (0.5, 1.0)   — top edge of texture
   Unit circle (-1, 0) → UV (0.0, 0.5)   — left edge of texture
   Unit circle (0, 0)  → UV (0.5, 0.5)   — center of texture
```

#### Scaling consideration

The cylinder has `radius=1.0` and `height=1.0` (`-0.5` to `+0.5`) in local space. When scaled with `glm::scale(model, glm::vec3(R*2, H, R*2))`, **UV stays the same** — only the geometry stretches.

---

### 3.3 Cone — Conical Mapping

> **Source**: [Cone.init()](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L367-L426) in `Primitives.h` — lines 367–426.

**Technique**: Similar to the cylinder, but the top converges to a single apex point. Each triangular sector fans from the apex to the base circle.

```
         Apex (0, +0.5, 0)
           ╱╲
          ╱  ╲       UV for apex:
         ╱    ╲      u = (u0+u1)/2, v = 1.0
        ╱      ╲
       ╱        ╲    UV for base:
      ╱──────────╲   u = u0 or u1, v = 0.0
    Base circle (y = -0.5)
```

**Code**:

```cpp
// Primitives.h:L384–L415 — Cone side UV generation
for (int i = 0; i < sectors; i++) {
    float a0 = i * sectorStep;
    float a1 = (i + 1) * sectorStep;
    float u0 = (float)i / sectors;
    float u1 = (float)(i + 1) / sectors;
    
    // Apex vertex (top center)
    {0.0f, halfH, 0.0f,  nxa, ny, nza,  (u0+u1)*0.5f, 1.0f}
    
    // Base vertices
    {x0, -halfH, z0,  nx0, ny, nz0,  u0, 0.0f}
    {x1, -halfH, z1,  nx1, ny, nz1,  u1, 0.0f}
}
```

**Key geometry details**:
- Apex (tip) is at `(0, +0.5, 0)` in local space
- Base radius = `1.0` at `y = -0.5`
- When scaled with `glm::scale(model, glm::vec3(R, H, R))`:
  - Base radius becomes `R`
  - Height becomes `H`
  - Apex moves to `y = H/2` (in local), which in world = `translate.y + H/2`

---

### 3.4 Sphere — Spherical (Latitude-Longitude) Mapping

> **Source**: [Sphere.init()](file:///d:/Academics/Graphics/Lab3_assignment/Primitives.h#L288-L340) in `Primitives.h` — lines 288–340.

**Technique**: UV Sphere maps U along longitude (0°→360°) and V along latitude (south pole→north pole).

```
    V=1 (north pole)  ──────────────
        │        ╱    ╲        │
        │      ╱        ╲      │
    V=0.5     │  equator  │    │
        │      ╲        ╱      │
        │        ╲    ╱        │
    V=0 (south pole)  ──────────────
       U=0                   U=1
```

**Formula**:

```cpp
// Primitives.h:L296–L320 — Sphere UV generation
for (int i = 0; i <= stacks; i++) {         // latitude rings
    float phi = M_PI/2 - i * (M_PI/stacks); // from +90° to -90°
    float y = sin(phi);                       // height
    float r = cos(phi);                       // ring radius
    float v = (float)i / stacks;             // V: 0 at top, 1 at bottom
    
    for (int j = 0; j <= sectors; j++) {     // longitude slices
        float theta = j * (2*M_PI/sectors);   // 0° to 360°
        float x = r * cos(theta);
        float z = r * sin(theta);
        float u = (float)j / sectors;         // U: 0 to 1 around equator
        
        // Vertex: {x, y, z,  x, y, z,  u, v}
    }
}
```

---

## 4. Texture Loading — `loadTexture()`

> **Source**: [loadTexture()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L251-L336) in `assignment.cpp` — lines 251–336.
> **Called at**: [assignment.cpp:L415–L428](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L415-L428) — all texture loading calls.

The `loadTexture()` function handles the full image→GPU pipeline:

```cpp
// assignment.cpp:L251–L336
unsigned int loadTexture(const char* path, GLenum wrapMode, GLenum filterMode) {
    // 1. Check file exists (L255–L262)
    std::ifstream testFile(path, std::ios::binary);
    if (!testFile.good()) return 0;  // skip missing files gracefully
    
    // 2. Load with stb_image — flip vertically for OpenGL convention (L265–L277)
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    
    // 3. Downscale oversized images (> 2048px) to prevent GPU memory issues (L284–L316)
    if (outW > MAX_TEXTURE_DIM || outH > MAX_TEXTURE_DIM) {
        // Simple nearest-neighbor downscale
        float scale = min(MAX_TEXTURE_DIM / outW, MAX_TEXTURE_DIM / outH);
        // ... resize loop ...
    }
    
    // 4. Upload to GPU (L318–L321)
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outW, outH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, finalData);
    
    // 5. Generate mipmaps (L322)
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // 6. Set texture parameters (L323–L326)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);    // horizontal
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);    // vertical
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    
    return textureID;
}
```

### Texture loading calls

> **Source**: [assignment.cpp:L413–L429](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L413-L429) — inside `main()`.

```cpp
// assignment.cpp:L415–L428
texFloor     = loadTexture("textures/floor.jpg",     GL_REPEAT,          GL_LINEAR);
texCarpet    = loadTexture("textures/carpet.jpg",    GL_REPEAT,          GL_NEAREST);
texFabric    = loadTexture("textures/fabric.jpg",    GL_CLAMP_TO_EDGE,   GL_LINEAR);
texWall      = loadTexture("textures/wall.jpg",      GL_MIRRORED_REPEAT, GL_LINEAR);
texDashboard = loadTexture("textures/dashboard.jpg", GL_REPEAT,          GL_NEAREST);
texBusBody   = loadTexture("textures/busbody.jpg",   GL_CLAMP_TO_EDGE,   GL_NEAREST);
texSphere    = loadTexture("textures/sphere.jpg",    GL_REPEAT,          GL_LINEAR);
texCone      = loadTexture("textures/cone.jpg",      GL_MIRRORED_REPEAT, GL_NEAREST);

// City environment textures (L425–L428)
texRoad      = loadTexture("textures/road.jpg",      GL_REPEAT,          GL_LINEAR);
texGrass     = loadTexture("textures/grass.jpg",     GL_REPEAT,          GL_LINEAR);
texContainer = loadTexture("textures/container2.png", GL_REPEAT,         GL_LINEAR);
texEmoji     = loadTexture("textures/emoji.png",     GL_CLAMP_TO_EDGE,   GL_LINEAR);
```

### Why `stbi_set_flip_vertically_on_load(true)`?

Most image formats store pixels top-to-bottom (row 0 = top of image), but OpenGL expects the origin at the **bottom-left**. Without flipping, textures appear upside-down.

```
    Image file memory:        OpenGL expectation:
    Row 0 (TOP)               Row 0 (BOTTOM)
    Row 1                     Row 1
    Row 2                     Row 2
    ...                       ...
    Row N (BOTTOM)            Row N (TOP)
```

### Why force 3 channels?

```cpp
// assignment.cpp:L277
stbi_load(path, &width, &height, &nrChannels, 3);  // force RGB
```

Some images are RGBA (4 channels) or grayscale (1 channel). Forcing 3 channels ensures a consistent `GL_RGB` format for all textures, simplifying the upload code.

---

## 5. Wrapping Modes

When UV coordinates go **outside** the `[0, 1]` range (e.g., due to tiling), the wrapping mode determines what happens:

### Available modes in this project:

> **Source**: Wrap mode arrays at [assignment.cpp:L141–L147](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L141-L147).

| Mode | OpenGL Constant | Behavior | When It's Used |
|------|----------------|----------|----------------|
| **Repeat** | `GL_REPEAT` | Tiles the texture infinitely | `road.jpg`, `grass.jpg`, `floor.jpg`, `carpet.jpg` |
| **Clamp to Edge** | `GL_CLAMP_TO_EDGE` | Edge pixels stretch outward | `fabric.jpg`, `busbody.jpg`, `emoji.png` |
| **Mirrored Repeat** | `GL_MIRRORED_REPEAT` | Tiles but flips every other copy | `wall.jpg`, `cone.jpg` |

### Visual comparison:

```
    GL_REPEAT:                        GL_CLAMP_TO_EDGE:
    ┌────┬────┬────┐                  ┌────┬────────────┐
    │ AB │ AB │ AB │                  │ AB │ BBBBBBBBBBB│
    │ CD │ CD │ CD │                  │ CD │ DDDDDDDDDDD│
    └────┴────┴────┘                  └────┴────────────┘
    
    GL_MIRRORED_REPEAT:
    ┌────┬────┬────┐
    │ AB │ BA │ AB │
    │ CD │ DC │ CD │
    └────┴────┴────┘
```

### Setting wrap mode in code:

```cpp
// assignment.cpp:L323–L324 — inside loadTexture()
// Applied to both S (horizontal) and T (vertical) axes
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // U axis
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // V axis
```

### Runtime toggle:

> **Source**: [updateSceneTextureParams()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L340-L352) — called when key **8** is pressed at [assignment.cpp:L974](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L974).

Press **8** to cycle between wrap modes. The `updateSceneTextureParams()` function re-applies the selected wrap mode to the scene sphere and cone textures:

```cpp
// assignment.cpp:L340–L352
void updateSceneTextureParams() {
    GLenum wrap = wrapModes[currentWrapIndex];
    GLenum filter = filterModes[currentFilterIndex];
    unsigned int ids[] = { texSphere, texCone };
    for (auto id : ids) {
        if (id != 0) {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }
    }
}
```

---

## 6. Filter Modes

> **Source**: Filter mode arrays at [assignment.cpp:L148–L152](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L148-L152).

Filtering determines how texels (texture pixels) are sampled when the rendered surface is larger or smaller than the texture image.

### Magnification filter (`GL_TEXTURE_MAG_FILTER`)

Used when the surface is **closer** than the texture resolution (texture is magnified):

| Mode | Effect | Quality | Speed |
|------|--------|---------|-------|
| `GL_LINEAR` | Bilinear interpolation of 4 nearest texels | Smooth | Slightly slower |
| `GL_NEAREST` | Uses the single closest texel | Pixelated/blocky | Fastest |

```
    GL_LINEAR (smooth):          GL_NEAREST (blocky):
    ┌──────────────┐             ┌──────────────┐
    │ ░░▒▒▓▓██    │             │ ░░░░▓▓▓▓████│
    │ ░░▒▒▓▓██    │             │ ░░░░▓▓▓▓████│
    │ ░░▒▒▓▓██    │             │ ░░░░▓▓▓▓████│
    └──────────────┘             └──────────────┘
```

### Minification filter (`GL_TEXTURE_MIN_FILTER`)

> **Source**: [assignment.cpp:L325](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L325) — always set to `GL_LINEAR_MIPMAP_LINEAR`.

Used when the surface is **farther** (texture is minified). This project always uses:

```cpp
// assignment.cpp:L325 — inside loadTexture()
GL_LINEAR_MIPMAP_LINEAR   // trilinear filtering — highest quality
```

This blends between two mipmap levels AND uses bilinear interpolation within each level. See the dedicated [Mipmapping document](./10_mipmapping_filtering.md) for full details.

### Per-texture filter assignments in this project:

> **Source**: [assignment.cpp:L415–L428](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L415-L428) — loading calls with filter parameter.

| Texture | Mag Filter | Why |
|---------|-----------|-----|
| `floor.jpg` | `GL_LINEAR` | Smooth floor surface |
| `carpet.jpg` | `GL_NEAREST` | Retro/pixelated carpet look |
| `fabric.jpg` | `GL_LINEAR` | Smooth fabric |
| `wall.jpg` | `GL_LINEAR` | Smooth brick wall |
| `dashboard.jpg` | `GL_NEAREST` | Retro dashboard style |
| `busbody.jpg` | `GL_NEAREST` | Pixel-art bus body |
| `road.jpg` | `GL_LINEAR` | Smooth road surface |
| `grass.jpg` | `GL_LINEAR` | Smooth grass |
| `container2.png` | `GL_LINEAR` | Smooth container box |
| `emoji.png` | `GL_LINEAR` | Smooth emoji |

---

## 7. Shader Texture Modes

> **Source**: [shader.frag](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag) (195 lines) and [shader.vert](file:///d:/Academics/Graphics/Lab3_assignment/shader.vert) (132 lines).

The project implements **4 texture modes** that control how texture and lighting interact. The mode is set via the uniform `textureMode` integer.

### Mode 0 — No Texture (Pure Phong Lighting)

> **Source**: [shader.frag:L157–L180](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag#L157-L180) — default Phong path.

```glsl
// shader.frag:L157–L180 — Mode 0
vec3 result = vec3(0.0);
if (dirLightOn)    result += CalcDirLight(dirLight, norm, viewDir, objectColor);
if (pointLightsOn) { for (int i = 0; i < 4; i++) result += CalcPointLight(...); }
if (spotLightOn)   result += CalcSpotLight(...);
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = Phong(objectColor)`

The object is lit using only its solid `objectColor` — no texture sampling.

### Mode 1 — Pure Texture (Texture Replaces Color)

> **Source**: [shader.frag:L109–L125](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag#L109-L125) — texture replaces material.

```glsl
// shader.frag:L109–L125 — Mode 1
texColor = texture(textureSampler, TexCoord).rgb;  // sample texture
result = vec3(0.0);
if (dirLightOn)    result += CalcDirLight(dirLight, norm, viewDir, texColor);  // use texColor as material
if (pointLightsOn) { ... CalcPointLight(..., texColor); }
if (spotLightOn)   result += CalcSpotLight(..., texColor);
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = Phong(texColor)`

The texture completely replaces `objectColor` in all lighting calculations.

### Mode 2 — Vertex-Blended (Gouraud Lighting × Texture)

Lighting is computed **per-vertex** in the vertex shader (Gouraud shading), then multiplied by the texture in the fragment shader.

> **Vertex shader**: [shader.vert:L111–L130](file:///d:/Academics/Graphics/Lab3_assignment/shader.vert#L111-L130) — Gouraud lighting computation.
> **Fragment shader**: [shader.frag:L128–L133](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag#L128-L133) — texture × vertex lighting.

**Vertex shader** (computes `VertexLightColor`):
```glsl
// shader.vert:L111–L130 — Mode 2
if (textureMode == 2) {
    vec3 result = vec3(0.0);
    if (dirLightOn)    result += CalcDirLightV(dirLight, norm, viewDir);
    if (pointLightsOn) { ... CalcPointLightV(...); }
    if (spotLightOn)   result += CalcSpotLightV(...);
    VertexLightColor = clamp(result, 0.0, 1.0);
}
```

**Fragment shader** (multiplies):
```glsl
// shader.frag:L128–L133 — Mode 2
texColor = texture(textureSampler, TexCoord).rgb;
vec3 result = texColor * VertexLightColor;  // texture × per-vertex lighting
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = texColor × GouraudLighting(objectColor)`

This produces **cheaper** but **less accurate** lighting (flat shading artifacts on large faces).

### Mode 3 — Fragment-Blended (Phong Lighting × Texture)

> **Source**: [shader.frag:L136–L154](file:///d:/Academics/Graphics/Lab3_assignment/shader.frag#L136-L154) — full per-fragment blend.

Lighting is computed **per-fragment** (Phong), then multiplied by the texture.

```glsl
// shader.frag:L136–L154 — Mode 3
texColor = texture(textureSampler, TexCoord).rgb;
vec3 phongResult = vec3(0.0);
if (dirLightOn)    phongResult += CalcDirLight(dirLight, norm, viewDir, objectColor);
if (pointLightsOn) { ... CalcPointLight(..., objectColor); }
if (spotLightOn)   phongResult += CalcSpotLight(..., objectColor);
vec3 result = texColor * clamp(phongResult, 0.0, 1.0);
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = texColor × Phong(objectColor)`

This is the **highest quality** mode — accurate per-pixel lighting blended with the texture. The `objectColor` tints the lighting, and the texture provides visual detail.

### Comparison Table

| Mode | Name | Lighting Stage | Material Source | Quality | Cost |
|------|------|---------------|-----------------|---------|------|
| 0 | None | Fragment (Phong) | `objectColor` | N/A (no texture) | Low |
| 1 | Pure Texture | Fragment (Phong) | `texColor` | High | Medium |
| 2 | Vertex-Blended | Vertex (Gouraud) | `objectColor` × `texColor` | Medium | Lower |
| 3 | Fragment-Blended | Fragment (Phong) | `objectColor` × `texColor` | Highest | Higher |

### How to set the mode in C++ code:

> **Source**: Example from city rendering at [assignment.cpp:L606–L618](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L606-L618).

```cpp
// assignment.cpp — example from road segment rendering (L606–L618)
// Before drawing an object:
ourShader.setInt("textureMode", 1);        // choose mode
glActiveTexture(GL_TEXTURE0);              // activate texture unit 0
glBindTexture(GL_TEXTURE_2D, texRoad);     // bind texture
ourShader.setInt("textureSampler", 0);     // tell shader to use unit 0

// Draw the object
bus.cube.draw(ourShader, model, glm::vec3(0.08f, 0.08f, 0.08f));

// After drawing, reset to no texture
ourShader.setInt("textureMode", 0);
```

---

## 8. Per-Object Texture Mapping Summary

### City Environment Objects

> **Source**: City rendering at [assignment.cpp:L592–L810](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L592-L810).

| Object | Primitive | Texture | Mode | Wrap | Where in Code |
|--------|-----------|---------|------|------|---------------|
| Road segments | Cube (flat) | `road.jpg` | 1 (pure tex) | `GL_REPEAT` | [L603–L617](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L603-L617) |
| Grass strips | Cube (flat) | `grass.jpg` | 3 (Phong blend) | `GL_REPEAT` | [L632–L646](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L632-L646) |
| Center divider | Cube (thin) | None | 0 | — | [L619–L631](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L619-L631) |
| Stacked cubes | Cube | `container2.png` | 1 (pure tex) | `GL_REPEAT` | [L660–L700](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L660-L700) |
| Tall building | Cube | `wall.jpg` or `container2.png` | varies | varies | [L701–L750](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L701-L750) |
| Windows | Cube (thin) | None | 0 | — | [L730–L747](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L730-L747) |
| Tower (cylinder) | Cylinder | `container2.png` | varies | `GL_REPEAT` | [L769–L779](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L769-L779) |
| Tower roof (cone) | Cone | None | 0 | — | [L781–L790](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L781-L790) |

### Bus Interior Objects

> **Source**: Bus drawing at [HoverBus.h](file:///d:/Academics/Graphics/Lab3_assignment/HoverBus.h) — `draw()` method. Textures assigned at [assignment.cpp:L432–L437](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L432-L437).

| Object | Texture | Mode | Wrap | Filter |
|--------|---------|------|------|--------|
| Floor | `floor.jpg` | varies | `GL_REPEAT` | `GL_LINEAR` |
| Carpet | `carpet.jpg` | varies | `GL_REPEAT` | `GL_NEAREST` |
| Seats (fabric) | `fabric.jpg` | varies | `GL_CLAMP_TO_EDGE` | `GL_LINEAR` |
| Walls | `wall.jpg` | varies | `GL_MIRRORED_REPEAT` | `GL_LINEAR` |
| Dashboard | `dashboard.jpg` | varies | `GL_REPEAT` | `GL_NEAREST` |
| Body | `busbody.jpg` | varies | `GL_CLAMP_TO_EDGE` | `GL_NEAREST` |

---

## 9. Common Pitfalls & Solutions

### Problem: Texture appears as colored stripes on cylinder/sphere

**Cause**: Non-square texture mapped with cylindrical UV wrapping. If the image is 130×126 pixels, the aspect ratio mismatch causes visible banding.

**Solution**: Use a square texture (e.g., 512×512 `container2.png` — see [assignment.cpp:L769](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L769)) or use `textureMode=3` (fragment-blended) which mixes the texture with the object color, reducing visible distortion.

### Problem: Texture stretches on a long flat surface

**Cause**: A single cube scaled to 200 units maps UV `[0,1]` across the entire 200 units — the texture stretches 200×.

**Solution**: Use **segmented rendering** — tile the road into 20-unit segments so each segment maps UV `[0,1]` naturally. This is the approach used at [assignment.cpp:L600–L647](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L600-L647), where road/grass segments loop around the bus position.

```cpp
// assignment.cpp:L600 — segmented rendering loop
for (int seg = -VISIBLE_SEGMENTS / 2; seg <= VISIBLE_SEGMENTS / 2; seg++) {
    float segX = segStart + seg * ROAD_SEGMENT_LEN;  // ROAD_SEGMENT_LEN = 20.0f
    // Each segment is only 20 units wide → texture maps [0,1] on 20 units
    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, ROAD_WIDTH));
}
```

### Problem: Cone overlaps with cylinder in tower

**Cause**: Both primitives are centered at their Y position. If the cylinder top is at `Y = towerH` and the cone center is also at `Y = towerH`, the cone's base clips into the cylinder.

**Solution**: Position the cone at `Y = towerH + coneH/2` so its base sits exactly on top of the cylinder. See [assignment.cpp:L781–L790](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L781-L790):

```cpp
// assignment.cpp:L769–L790 — tower rendering
// Cylinder: center at towerH/2, top at towerH
model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH * 0.5f, bz));
model = glm::scale(model, glm::vec3(radius * 2.0f, towerH, radius * 2.0f));
bus.cylinder.draw(ourShader, model, color);

// Cone: center at towerH + coneH/2, base sits exactly on cylinder top
model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH + coneH * 0.5f, bz));
model = glm::scale(model, glm::vec3(radius * 2.8f, coneH, radius * 2.8f));
sceneCone.draw(ourShader, model, roofColor);
```
