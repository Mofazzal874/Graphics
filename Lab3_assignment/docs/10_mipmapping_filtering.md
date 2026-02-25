# Mipmapping & Texture Filtering — Theory, Functions & Implementation

This document covers mipmapping in detail: what it is, why it's needed, how mipmap levels are computed, all the OpenGL filtering functions, and how they're used in this project.

---

## Table of Contents

1. [What Is Mipmapping?](#1-what-is-mipmapping)
2. [Why Mipmapping Is Necessary](#2-why-mipmapping-is-necessary)
3. [Mipmap Level Calculation](#3-mipmap-level-calculation)
4. [Generating Mipmaps in OpenGL](#4-generating-mipmaps-in-opengl)
5. [All OpenGL Filter Modes](#5-all-opengl-filter-modes)
6. [Filter Modes Used in This Project](#6-filter-modes-used-in-this-project)
7. [Wrapping Modes and Mipmapping Interaction](#7-wrapping-modes-and-mipmapping-interaction)
8. [Texture Parameter Functions Reference](#8-texture-parameter-functions-reference)
9. [Performance Comparison](#9-performance-comparison)

---

## 1. What Is Mipmapping?

**Mipmapping** (from the Latin _multum in parvo_ — "much in little") is a technique where a texture image is pre-stored at multiple resolutions. Each level is exactly **half the width and half the height** of the previous level.

```
Level 0:  512 × 512   (original)
Level 1:  256 × 256   (1/4 pixels)
Level 2:  128 × 128   (1/16 pixels)
Level 3:   64 ×  64   (1/64 pixels)
Level 4:   32 ×  32
Level 5:   16 ×  16
Level 6:    8 ×   8
Level 7:    4 ×   4
Level 8:    2 ×   2
Level 9:    1 ×   1   (single pixel)
```

### Mipmap Pyramid Visualization

```
    Level 0 (512×512):
    ┌────────────────────────────────┐
    │                                │
    │         Full Detail            │
    │        (original image)        │
    │                                │
    └────────────────────────────────┘
    
    Level 1 (256×256):
    ┌────────────────┐
    │                │
    │   Half Detail  │
    │                │
    └────────────────┘
    
    Level 2 (128×128):
    ┌────────┐
    │ Quarter│
    │ Detail │
    └────────┘
    
    Level 3 (64×64):    Level 4:   Level 5:   ...   Level 9:
    ┌────┐              ┌──┐       ┌┐               ·
    │    │              │  │       ││
    └────┘              └──┘       └┘
```

### Total number of mipmap levels

For a texture of dimensions `W × H`:

```
numLevels = floor(log₂(max(W, H))) + 1
```

**Example**: A 512×512 texture:
```
numLevels = floor(log₂(512)) + 1 = floor(9) + 1 = 10 levels
```

### Total memory cost

Each mipmap level has 1/4 the pixels of the previous level. The total memory for all levels is:

```
totalMemory = baseMemory × (1 + 1/4 + 1/16 + 1/64 + ...)
            = baseMemory × 4/3
            ≈ 1.33 × baseMemory
```

So **mipmaps add only ~33% more memory** for a major quality and performance improvement.

| Texture | Base Memory | With Mipmaps | Overhead |
|---------|------------|-------------|----------|
| 512×512 RGB | 786 KB | 1,048 KB | +33% |
| 1024×1024 RGB | 3 MB | 4 MB | +33% |
| 2048×2048 RGB | 12 MB | 16 MB | +33% |

---

## 2. Why Mipmapping Is Necessary

### Problem: Aliasing (Moiré Patterns)

Without mipmapping, when a textured surface is viewed from far away, each screen pixel covers **many texels** (texture pixels). The GPU must decide which texel to use, and picking just one creates shimmering/flickering artifacts called **aliasing** or **Moiré patterns**.

```
    Close up (no problem):         Far away (many texels per pixel):
    
    ┌──┬──┬──┬──┐                  ┌──────────────────────┐
    │01│02│03│04│                  │ pixel covers 16 texels│
    ├──┼──┼──┼──┤                  │ which one to choose?  │
    │05│06│07│08│                  │ → aliasing / shimmer  │
    ├──┼──┼──┼──┤                  └──────────────────────┘
    │09│10│11│12│
    └──┴──┴──┴──┘
    1 screen pixel = 1 texel       1 screen pixel = 16 texels
```

### Solution: Pre-filtered Texture Levels

With mipmapping, the GPU selects a **pre-averaged** version of the texture that best matches the screen resolution. The averaging has already been done, so there's no aliasing.

```
    Viewing distance    GPU selects    Why
    ─────────────────   ───────────    ───────────────────────────
    Very close          Level 0        Full detail, 1:1 texel-pixel
    Medium              Level 3        64×64, smooth, no aliasing
    Far away            Level 7        4×4, highly averaged
    Tiny speck          Level 9        1×1, single average color
```

---

## 3. Mipmap Level Calculation

The GPU automatically selects the mipmap level based on the **rate of change of texture coordinates** across the surface (called the **texture gradient**).

### Mathematical Formula

The mipmap level `λ` (lambda) is computed as:

```
λ = log₂(ρ)
```

Where `ρ` (rho) is the **texel-to-pixel ratio** — how many texels are covered by one screen pixel:

```
ρ = max(|∂u/∂x × w|, |∂u/∂y × w|, |∂v/∂x × h|, |∂v/∂y × h|)
```

- `∂u/∂x` = how fast the U coordinate changes per screen pixel horizontally
- `w` = texture width in texels
- `h` = texture height in texels

### Numerical Example

Consider a 512×512 texture on a surface viewed at a distance where each screen pixel covers about 8×8 texels:

```
ρ = 8                       (8 texels per pixel)
λ = log₂(8) = 3            (use mipmap level 3 = 64×64)
```

If the surface is closer, covering only 2×2 texels per pixel:

```
ρ = 2
λ = log₂(2) = 1            (use mipmap level 1 = 256×256)
```

If `λ` falls between two integer levels (e.g., `λ = 2.7`), the behavior depends on the filter mode — see Section 5.

---

## 4. Generating Mipmaps in OpenGL

### Automatic Generation (Used in This Project)

```cpp
// After uploading texture data
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0,
             GL_RGB, GL_UNSIGNED_BYTE, data);

// Generate all mipmap levels automatically
glGenerateMipmap(GL_TEXTURE_2D);
```

`glGenerateMipmap()` creates all levels down to 1×1 using a **box filter** (averaging 2×2 blocks of pixels).

### Manual Generation (Alternative Approach)

You can also upload each level manually:

```cpp
// Level 0 (original)
glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, data0);
// Level 1 (half size)
glTexImage2D(GL_TEXTURE_2D, 1, GL_RGB, 256, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, data1);
// Level 2 (quarter size)
glTexImage2D(GL_TEXTURE_2D, 2, GL_RGB, 128, 128, 0, GL_RGB, GL_UNSIGNED_BYTE, data2);
// ... and so on
```

This is useful when you want custom filtering (e.g., Gaussian, Lanczos) for each level.

### This project's implementation in `loadTexture()`:

```cpp
unsigned int loadTexture(const char* path, GLenum wrapMode, GLenum filterMode) {
    // ... load image data ...
    
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    
    // Upload base level (level 0)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outW, outH, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, finalData);
    
    // Auto-generate all mipmap levels from level 0
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // Set min filter to use mipmaps (CRITICAL — without this, mipmaps are ignored)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    
    return textureID;
}
```

> **Important**: If `GL_TEXTURE_MIN_FILTER` is set to `GL_LINEAR` or `GL_NEAREST` (without `_MIPMAP_`), mipmaps are generated but **never used**. You must use one of the 4 mipmap-aware filter modes.

---

## 5. All OpenGL Filter Modes

### Minification Filter Modes (`GL_TEXTURE_MIN_FILTER`)

These control what happens when the texture is **shrunk** (viewed from far away). There are 6 options: 2 without mipmaps and 4 with mipmaps.

#### Without Mipmaps

| Constant | Behavior | Description |
|----------|----------|-------------|
| `GL_NEAREST` | Single nearest texel | Fastest, most aliased, no mipmaps used |
| `GL_LINEAR` | Bilinear interpolation of 4 texels | Smooth but still aliases at distance |

#### With Mipmaps (RECOMMENDED)

| Constant | Within Level | Between Levels | Quality | Speed |
|----------|-------------|---------------|---------|-------|
| `GL_NEAREST_MIPMAP_NEAREST` | Nearest texel | Nearest mipmap level | Lowest | Fastest |
| `GL_LINEAR_MIPMAP_NEAREST` | Bilinear (4 texels) | Nearest mipmap level | Medium | Fast |
| `GL_NEAREST_MIPMAP_LINEAR` | Nearest texel | Linear blend between 2 levels | Medium | Medium |
| `GL_LINEAR_MIPMAP_LINEAR` | Bilinear (4 texels) | Linear blend between 2 levels | **Highest** (trilinear) | **Slowest** |

### Understanding the Naming Convention

```
GL_[texel sampling]_MIPMAP_[level sampling]

First part:  GL_NEAREST = pick 1 texel
             GL_LINEAR  = blend 4 texels (bilinear)

Second part: MIPMAP_NEAREST = use closest mipmap level
             MIPMAP_LINEAR  = blend between 2 closest levels
```

### Visual Comparison

```
    GL_NEAREST_MIPMAP_NEAREST:           GL_LINEAR_MIPMAP_LINEAR:
    ┌─────────────────────┐              ┌─────────────────────┐
    │ Level 2             │              │ Blend of Level 2    │
    │ ┌───┐               │              │ and Level 3         │
    │ │ X │ ← pick 1 texel│              │ with bilinear       │
    │ └───┘               │              │ interpolation       │
    │ Hard edges, pixelated│              │ Smooth, no seams    │
    └─────────────────────┘              └─────────────────────┘
```

### Trilinear Filtering (GL_LINEAR_MIPMAP_LINEAR) — Step by Step

When `λ = 2.7` (between level 2 and level 3):

```
Step 1: Sample level 2 with bilinear interpolation → color_A
        (blend 4 nearest texels in level 2)

Step 2: Sample level 3 with bilinear interpolation → color_B
        (blend 4 nearest texels in level 3)

Step 3: Blend between levels:
        finalColor = mix(color_A, color_B, fract(2.7))
                   = mix(color_A, color_B, 0.7)
                   = 0.3 × color_A + 0.7 × color_B
```

This requires **8 texel reads** total (4 per level, 2 levels).

### Magnification Filter Modes (`GL_TEXTURE_MAG_FILTER`)

When the texture is **enlarged** (viewed up close), only 2 options exist:

| Constant | Behavior | Effect |
|----------|----------|--------|
| `GL_NEAREST` | Nearest single texel | Blocky/pixelated look |
| `GL_LINEAR` | Bilinear interpolation of 4 texels | Smooth/blurred look |

> Mipmaps are **never used** for magnification — they would make the image blurrier, not sharper.

---

## 6. Filter Modes Used in This Project

### Default Configuration

Every texture in this project uses:

```cpp
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);  // trilinear
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);               // per-texture
```

### Per-Texture Magnification Settings

| Texture File | `filterMode` (Mag) | Design Rationale |
|-------------|-------------------|------------------|
| `floor.jpg` | `GL_LINEAR` | Smooth floor, natural look |
| `carpet.jpg` | `GL_NEAREST` | Deliberate pixelated/retro aesthetics |
| `fabric.jpg` | `GL_LINEAR` | Smooth fabric for seat upholstery |
| `wall.jpg` | `GL_LINEAR` | Smooth brick/stone wall |
| `dashboard.jpg` | `GL_NEAREST` | Retro dashboard gauges |
| `busbody.jpg` | `GL_NEAREST` | Pixel-art style bus exterior |
| `sphere.jpg` | `GL_LINEAR` | Smooth spherical mapping |
| `cone.jpg` | `GL_NEAREST` | Intentional sharp texel edges |
| `road.jpg` | `GL_LINEAR` | Smooth asphalt appearance |
| `grass.jpg` | `GL_LINEAR` | Smooth grass field |
| `container2.png` | `GL_LINEAR` | Smooth crate texture |
| `emoji.png` | `GL_LINEAR` | Smooth emoji face |

### Runtime Filter Switching

Press **9** to toggle magnification filter at runtime:

```cpp
GLenum filterModes[] = { GL_LINEAR, GL_NEAREST };
const char* filterNames[] = { "GL_LINEAR", "GL_NEAREST" };

// In key_callback():
case GLFW_KEY_9:
    currentFilterIndex = (currentFilterIndex + 1) % NUM_FILTER_MODES;
    updateSceneTextureParams();
    break;
```

The `updateSceneTextureParams()` function updates the sphere and cone textures:

```cpp
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

## 7. Wrapping Modes and Mipmapping Interaction

Wrapping modes affect how mipmaps are generated at texture borders:

### GL_REPEAT
- Mipmaps are generated assuming the texture tiles infinitely
- Border pixels blend with the opposite edge
- Best for seamless tiling textures (grass, brick, road)

### GL_CLAMP_TO_EDGE
- Mipmaps use only the edge pixel colors at boundaries
- No wrap-around blending
- Best for textures that shouldn't tile (decals, unique images)

### GL_MIRRORED_REPEAT
- Mipmaps assume alternating mirror images
- Creates seamless borders between tiles
- Best for textures with visible edges that need smooth tiling

```
    GL_REPEAT border blending:        GL_CLAMP_TO_EDGE border:
    
    ...│ A B C│D E F │ A B C│...     ...│ C C C│D E F │ F F F│...
       edge wraps                        edge repeats
       to opposite side                  last pixel
```

---

## 8. Texture Parameter Functions Reference

### Complete list of OpenGL texture functions used:

| Function | Purpose | Parameters |
|----------|---------|------------|
| `glGenTextures(n, &id)` | Create `n` texture objects | Returns texture IDs |
| `glBindTexture(target, id)` | Make texture active for subsequent operations | `GL_TEXTURE_2D`, texture ID |
| `glTexImage2D(...)` | Upload pixel data to GPU | target, level, format, w, h, border, format, type, data |
| `glGenerateMipmap(target)` | Auto-create all mipmap levels | `GL_TEXTURE_2D` |
| `glTexParameteri(target, pname, param)` | Set integer texture parameter | See parameters below |
| `glActiveTexture(unit)` | Select which texture unit to use | `GL_TEXTURE0` through `GL_TEXTURE15` |
| `glDeleteTextures(n, &id)` | Free GPU texture memory | Number of textures, ID pointer |

### `glTexParameteri` parameters:

| `pname` | Valid Values | Default |
|---------|-------------|---------|
| `GL_TEXTURE_WRAP_S` | `GL_REPEAT`, `GL_CLAMP_TO_EDGE`, `GL_MIRRORED_REPEAT` | `GL_REPEAT` |
| `GL_TEXTURE_WRAP_T` | Same as WRAP_S | `GL_REPEAT` |
| `GL_TEXTURE_MIN_FILTER` | Any of the 6 min-filter modes | `GL_NEAREST_MIPMAP_LINEAR` |
| `GL_TEXTURE_MAG_FILTER` | `GL_LINEAR`, `GL_NEAREST` | `GL_LINEAR` |

### `glTexImage2D` parameters explained:

```cpp
glTexImage2D(
    GL_TEXTURE_2D,     // target: 2D texture
    0,                 // mipmap level (0 = base)
    GL_RGB,            // internal format (how GPU stores it)
    width,             // texture width in pixels
    height,            // texture height in pixels
    0,                 // border (always 0 in modern OpenGL)
    GL_RGB,            // format of input data
    GL_UNSIGNED_BYTE,  // data type of each channel (0-255)
    data               // pointer to pixel data
);
```

---

## 9. Performance Comparison

### Filtering Performance (Relative Cost)

| Filter Mode | Texel Reads | Relative Cost | Quality |
|-------------|-------------|--------------|---------|
| `GL_NEAREST` | 1 | 1.0× | ★☆☆☆ |
| `GL_LINEAR` | 4 | 1.5× | ★★☆☆ |
| `GL_NEAREST_MIPMAP_NEAREST` | 1 | 1.2× | ★★☆☆ |
| `GL_LINEAR_MIPMAP_NEAREST` | 4 | 1.8× | ★★★☆ |
| `GL_NEAREST_MIPMAP_LINEAR` | 2 | 1.5× | ★★★☆ |
| `GL_LINEAR_MIPMAP_LINEAR` | 8 | 2.5× | ★★★★ |

### When to Use Each

| Scenario | Recommended Filter | Why |
|----------|-------------------|-----|
| Large flat surfaces (floors, walls) | `GL_LINEAR_MIPMAP_LINEAR` | Eliminates shimmer at distance |
| Pixel art textures | `GL_NEAREST` | Preserves sharp pixel edges |
| Real-time games (mobile) | `GL_LINEAR_MIPMAP_NEAREST` | Good quality, lower cost |
| High-quality rendering | `GL_LINEAR_MIPMAP_LINEAR` | Best visual quality |
| UI textures (always same size) | `GL_LINEAR` | No minification = no mipmaps needed |

### Memory vs. Quality Trade-off

```
                    Memory
                      ↑
     ★★★★            │        ┌─── GL_LINEAR_MIPMAP_LINEAR
     Quality ────────│────────┤    (1.33× memory, 8 reads)
                      │        │
     ★★★             │   ┌────┤── GL_LINEAR_MIPMAP_NEAREST
                      │   │    │   (1.33× memory, 4 reads)
     ★★              │   │    │
                      │   │    └── GL_LINEAR
     ★               │   │        (1.0× memory, 4 reads)
                      │   │
                      └───┴────────────────────► Texel Reads
                          1    2    4    8
```

### This project's choice:

**`GL_LINEAR_MIPMAP_LINEAR` (trilinear) for all minification** — chosen because:
1. The scene has objects at various distances (buildings far vs. near)
2. The bus moves continuously, causing textures to transition between mipmap levels
3. Trilinear filtering eliminates the visible "seam" between mipmap levels
4. Modern GPUs handle trilinear filtering with negligible performance cost
