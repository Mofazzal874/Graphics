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

The texture pipeline in this project follows these steps:

```
┌──────────────┐     ┌──────────────┐     ┌──────────────┐     ┌──────────────┐
│  1. Load     │     │ 2. Generate  │     │ 3. Bind &    │     │ 4. Sample in │
│  Image File  │────►│  UV Coords   │────►│  Set Params  │────►│  Shader      │
│  (stb_image) │     │  (per vertex)│     │  (wrap,filt) │     │  (textureMode)│
└──────────────┘     └──────────────┘     └──────────────┘     └──────────────┘
```

### Step-by-step:

1. **Load**: `stbi_load()` reads the image file from disk into a CPU buffer
2. **Upload**: `glTexImage2D()` uploads pixel data to a GPU texture object
3. **Mipmaps**: `glGenerateMipmap()` creates downscaled versions for distance rendering
4. **Parameters**: Wrap and filter modes are set with `glTexParameteri()`
5. **Bind**: Before drawing, the texture is bound to a texture unit
6. **Sample**: The fragment shader reads from the texture using the interpolated UV

---

## 3. UV Coordinate Generation per Primitive

Each primitive class (`Cube`, `Cylinder`, `Cone`, `Sphere`) generates UV coordinates differently based on its geometry. The vertex layout for all primitives is:

```
┌─────────┬───────────┬──────────┐
│ Position │  Normal   │ TexCoord │
│ (x,y,z) │ (nx,ny,nz)│  (s, t)  │
│ 3 floats │ 3 floats  │ 2 floats │
└─────────┴───────────┴──────────┘
         Total: 8 floats per vertex
```

Setup in OpenGL:
```cpp
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

**Technique**: The cylinder's side surface is "unwrapped" into a rectangle. The U coordinate wraps around the circumference [0→1], and the V coordinate goes from bottom (0) to top (1).

```
    Unwrapped cylinder side:
    
    V=1 ┌──────────────────────────────┐ Top edge
        │                              │
        │     Texture wraps around     │
        │     the full circumference   │
        │                              │
    V=0 └──────────────────────────────┘ Bottom edge
        U=0                          U=1
        (angle=0°)               (angle=360°)
```

**Formula for UV generation**:

```cpp
for (int i = 0; i < sectors; i++) {
    float a0 = i * sectorStep;         // angle for this sector
    float a1 = (i + 1) * sectorStep;   // angle for next sector
    
    float x0 = cos(a0), z0 = sin(a0);  // position on unit circle
    float x1 = cos(a1), z1 = sin(a1);
    
    float u0 = (float)i / sectors;      // U = fraction around circle
    float u1 = (float)(i + 1) / sectors;
    
    // Side triangle 1 (bottom-left, bottom-right, top-right)
    {x0, -halfH, z0,  x0,0,z0,  u0, 0.0f}   // bottom, left sector edge
    {x1, -halfH, z1,  x1,0,z1,  u1, 0.0f}   // bottom, right sector edge
    {x1,  halfH, z1,  x1,0,z1,  u1, 1.0f}   // top, right sector edge
    
    // Side triangle 2
    {x1,  halfH, z1,  x1,0,z1,  u1, 1.0f}
    {x0,  halfH, z0,  x0,0,z0,  u0, 1.0f}
    {x0, -halfH, z0,  x0,0,z0,  u0, 0.0f}
}
```

**Numerical example** (with 36 sectors):

| Sector `i` | Angle (°) | `u0` | `u1` | `v` (bottom/top) |
|------------|-----------|------|------|------------------|
| 0 | 0° → 10° | 0.000 | 0.028 | 0.0 / 1.0 |
| 1 | 10° → 20° | 0.028 | 0.056 | 0.0 / 1.0 |
| ... | ... | ... | ... | ... |
| 17 | 170° → 180° | 0.472 | 0.500 | 0.0 / 1.0 |
| ... | ... | ... | ... | ... |
| 35 | 350° → 360° | 0.972 | 1.000 | 0.0 / 1.0 |

**Top/Bottom Caps** use planar (polar) UV mapping:

```cpp
// Top cap — center at (0.5, 0.5), radius maps to 0.5
{0,  halfH, 0,   0,1,0,  0.5f, 0.5f}                         // center
{x0, halfH, z0,  0,1,0,  0.5f + 0.5f*x0, 0.5f + 0.5f*z0}   // edge point 1
{x1, halfH, z1,  0,1,0,  0.5f + 0.5f*x1, 0.5f + 0.5f*z1}   // edge point 2
```

This projects the circle onto the texture like looking down from above:

```
    ┌───────────────┐
    │    .  .  .    │ V=1
    │  .        .   │
    │ .   center  . │ (0.5, 0.5)
    │  .        .   │
    │    .  .  .    │ V=0
    └───────────────┘
   U=0             U=1
```

**Scaling consideration**: The cylinder in local space has `radius = 1.0` and `height = 1.0` (from `-0.5` to `+0.5`). When you scale it with `glm::scale(model, glm::vec3(radius*2, height, radius*2))`, the **UV coordinates don't change** — only the geometry stretches. A tall, thin cylinder will stretch the texture vertically.

---

### 3.3 Cone — Conical Mapping

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

The `loadTexture()` function handles the full image→GPU pipeline:

```cpp
unsigned int loadTexture(const char* path, GLenum wrapMode, GLenum filterMode) {
    // 1. Check file exists
    std::ifstream testFile(path, std::ios::binary);
    if (!testFile.good()) return 0;  // skip missing files gracefully
    
    // 2. Load with stb_image (flip vertically for OpenGL convention)
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    
    // 3. Downscale oversized images (> 2048px) to prevent GPU memory issues
    if (outW > MAX_TEXTURE_DIM || outH > MAX_TEXTURE_DIM) {
        // Simple nearest-neighbor downscale
        float scale = min(MAX_TEXTURE_DIM / outW, MAX_TEXTURE_DIM / outH);
        // ... resize loop ...
    }
    
    // 4. Upload to GPU
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outW, outH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, finalData);
    
    // 5. Generate mipmaps
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // 6. Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);    // horizontal
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);    // vertical
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    
    return textureID;
}
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
stbi_load(path, &width, &height, &nrChannels, 3);  // force RGB
```

Some images are RGBA (4 channels) or grayscale (1 channel). Forcing 3 channels ensures a consistent `GL_RGB` format for all textures, simplifying the upload code.

---

## 5. Wrapping Modes

When UV coordinates go **outside** the `[0, 1]` range (e.g., due to tiling), the wrapping mode determines what happens:

### Available modes in this project:

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
// Applied to both S (horizontal) and T (vertical) axes
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // U axis
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);  // V axis
```

### Runtime toggle:

Press **8** to cycle between wrap modes. The `updateSceneTextureParams()` function re-applies the selected wrap mode to the scene sphere and cone textures:

```cpp
GLenum wrapModes[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };

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

Used when the surface is **farther** (texture is minified). This project always uses:

```cpp
GL_LINEAR_MIPMAP_LINEAR   // trilinear filtering — highest quality
```

This blends between two mipmap levels AND uses bilinear interpolation within each level. See the dedicated [Mipmapping document](./10_mipmapping_filtering.md) for full details.

### Per-texture filter assignments in this project:

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

The project implements **4 texture modes** that control how texture and lighting interact. The mode is set via the uniform `textureMode` integer.

### Mode 0 — No Texture (Pure Phong Lighting)

```glsl
// Fragment shader — Mode 0
vec3 result = vec3(0.0);
if (dirLightOn)    result += CalcDirLight(dirLight, norm, viewDir, objectColor);
if (pointLightsOn) { for (int i = 0; i < 4; i++) result += CalcPointLight(...); }
if (spotLightOn)   result += CalcSpotLight(...);
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = Phong(objectColor)`

The object is lit using only its solid `objectColor` — no texture sampling.

### Mode 1 — Pure Texture (Texture Replaces Color)

```glsl
// Fragment shader — Mode 1
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

**Vertex shader** (computes `VertexLightColor`):
```glsl
// shader.vert — Mode 2
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
// shader.frag — Mode 2
texColor = texture(textureSampler, TexCoord).rgb;
vec3 result = texColor * VertexLightColor;  // texture × per-vertex lighting
FragColor = vec4(clamp(result, 0.0, 1.0), alpha);
```

**Formula**: `FragColor = texColor × GouraudLighting(objectColor)`

This produces **cheaper** but **less accurate** lighting (flat shading artifacts on large faces).

### Mode 3 — Fragment-Blended (Phong Lighting × Texture)

Lighting is computed **per-fragment** (Phong), then multiplied by the texture.

```glsl
// shader.frag — Mode 3
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

```cpp
// Before drawing an object:
ourShader.setInt("textureMode", 3);        // choose mode
glActiveTexture(GL_TEXTURE0);              // activate texture unit 0
glBindTexture(GL_TEXTURE_2D, texWall);     // bind texture
ourShader.setInt("textureSampler", 0);     // tell shader to use unit 0

// Draw the object
bus.cube.draw(ourShader, model, glm::vec3(0.7f, 0.3f, 0.85f));

// After drawing, reset to no texture
ourShader.setInt("textureMode", 0);
```

---

## 8. Per-Object Texture Mapping Summary

### City Environment Objects

| Object | Primitive | Texture | Mode | Wrap | Image Size | Notes |
|--------|-----------|---------|------|------|------------|-------|
| Road | Cube (flat) | `road.jpg` | 3 (Phong blend) | `GL_REPEAT` | 4288×3216 → resized to 2048 | Fragment-blend avoids stretch on 200-unit strip |
| Grass strips | Cube (flat) | `grass.jpg` | 3 (Phong blend) | `GL_REPEAT` | 612×612 | Square texture tiles well |
| Center divider | Cube (thin) | None | 0 | — | — | Pure white `(1,1,1)` |
| Stacked cubes | Cube | `container2.png` | 1 (pure tex) | `GL_REPEAT` | 512×512 | Square, per-face mapping good for cubes |
| Tall building | Cube | `wall.jpg` | 3 (Phong blend) | `GL_MIRRORED_REPEAT` | 3723×2000 → resized | Mirrored repeat creates seamless brick pattern |
| Windows | Cube (thin) | None | 0 | — | — | Dark blue color only |
| Tower (cylinder) | Cylinder | `container2.png` | 3 (Phong blend) | `GL_REPEAT` | 512×512 | Square texture avoids cylindrical distortion |
| Tower roof (cone) | Cone | None | 0 | — | — | Solid color — apex distortion-free |

### Bus Interior Objects

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

**Solution**: Use a square texture (e.g., 512×512 `container2.png`) or use `textureMode=3` (fragment-blended) which mixes the texture with the object color, reducing visible distortion.

### Problem: Texture stretches on a long flat surface

**Cause**: A cube scaled to 200 units long maps UV `[0,1]` across the entire 200 units. The texture (e.g., 4288×3216 road image) is stretched 200× in one direction.

**Solution**: Use `textureMode=3` (fragment-blended) which multiplies texture × lighting. The tiling of the texture coordinates combined with `GL_REPEAT` wrapping helps, but the base UV is still `[0,1]`. For proper tiling, you'd need to multiply UVs in the shader.

### Problem: Cone overlaps with cylinder in tower

**Cause**: Both primitives are centered at their Y position. If the cylinder top is at `Y = towerH` and the cone center is also at `Y = towerH`, the cone's base (at `-halfH` in local space = `towerH - coneH/2` in world) clips into the cylinder.

**Solution**: Position the cone at `Y = towerH + coneH/2` so its base sits exactly on top of the cylinder:

```cpp
// Cylinder: center at towerH/2, top at towerH
model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH * 0.5f, bz));
model = glm::scale(model, glm::vec3(radius * 2.0f, towerH, radius * 2.0f));
bus.cylinder.draw(ourShader, model, color);

// Cone: center at towerH + coneH/2, base at towerH (sits on cylinder)
model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH + coneH * 0.5f, bz));
model = glm::scale(model, glm::vec3(radius * 2.8f, coneH, radius * 2.8f));
sceneCone.draw(ourShader, model, roofColor);
```
