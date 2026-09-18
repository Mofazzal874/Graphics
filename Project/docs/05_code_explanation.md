# Code Explanation — Hover Bus (Lab 3)

This document provides a **detailed walkthrough** of every major code segment in the project, explaining how each part works, what the data types mean, and how they connect together. The project files covered are:

| File | Role |
|------|------|
| `assignment.cpp` | Main application — window, render loop, camera, input, viewports |
| `Bus.h` | Bus model — all geometry draw calls, jet engine, flame, hover pads |
| `Primitives.h` | Low-level shape classes — Cube, Cylinder, Torus |
| `Shader.h` | Shader loading/compilation + uniform setter helpers |
| `shader.vert` | Vertex shader — transforms positions + computes normals |
| `shader.frag` | Fragment shader — Phong lighting, emissive bypass, blending |

---

## 1. Data Types Used Throughout the Codebase

Before diving into the code, here is a reference for every major data type you will encounter:

### 1.1 GLM Vector Types

```cpp
glm::vec3(x, y, z)
```

| Parameter | Meaning |
|-----------|---------|
| `x` | **Red** (for colors) or **Left/Right** position (for 3D coordinates) |
| `y` | **Green** (for colors) or **Up/Down** position |
| `z` | **Blue** (for colors) or **Forward/Backward** position |

`glm::vec3` is a 3-component floating-point vector from the GLM (OpenGL Mathematics) library. It is used for **two very different purposes** in the code:
- **Positions** — `glm::vec3(0.0f, 5.0f, 20.0f)` means "X=0, Y=5 units up, Z=20 units toward camera"
- **Colors** — `glm::vec3(1.0f, 0.0f, 0.0f)` means "full red, no green, no blue" = pure red

Other GLM vector variants:
- `glm::vec2(x, y)` — 2-component vector (rarely used here)
- `glm::vec4(x, y, z, w)` — 4-component vector (used for the `w` homogeneous coordinate internally)

### 1.2 GLM Matrix Types

```cpp
glm::mat4 model = glm::mat4(1.0f);
```

`glm::mat4` is a **4×4 floating-point matrix**. The argument `1.0f` creates an **identity matrix** — the "do nothing" transform:

```
| 1  0  0  0 |
| 0  1  0  0 |
| 0  0  1  0 |
| 0  0  0  1 |
```

Matrices are multiplied **right-to-left**. When you write:
```cpp
model = glm::translate(model, position);
model = glm::rotate(model, angle, axis);
model = glm::scale(model, size);
```
The **scale** is applied first, then **rotation**, then **translation** — reading bottom-to-top.

### 1.3 Transformation Functions

| Function | Signature | What It Does |
|----------|-----------|--------------|
| `glm::translate` | `glm::translate(mat4 m, vec3 offset)` | Moves an object by `offset.x`, `offset.y`, `offset.z` |
| `glm::rotate` | `glm::rotate(mat4 m, float radians, vec3 axis)` | Rotates around the given `axis` by `radians` |
| `glm::scale` | `glm::scale(mat4 m, vec3 factors)` | Stretches along X, Y, Z by the given `factors` |
| `glm::radians` | `glm::radians(float degrees)` | Converts degrees → radians (OpenGL uses radians) |
| `glm::perspective` | `glm::perspective(float fov, float aspect, float near, float far)` | Creates a perspective projection matrix |

### 1.4 GLM Utility Functions

| Function | What It Does | Example |
|----------|-------------|---------|
| `glm::normalize(vec3 v)` | Scales vector to length 1.0, keeping direction | Used for light/view directions |
| `glm::cross(vec3 a, vec3 b)` | Returns a vector perpendicular to both `a` and `b` | Used in `myLookAt` for camera axes |
| `glm::dot(vec3 a, vec3 b)` | Returns the cosine-weighted overlap (scalar) | Used in lighting math |
| `glm::clamp(val, min, max)` | Constrains a value within a range | Used for speed limits |
| `glm::length(vec3 v)` | Returns the magnitude of a vector | Used for distance calculations |

### 1.5 OpenGL Types

| Type | Meaning |
|------|---------|
| `GLFWwindow*` | Pointer to the GLFW window (handles the OS window) |
| `unsigned int VAO` | **Vertex Array Object** — stores the layout of vertex data on the GPU |
| `unsigned int VBO` | **Vertex Buffer Object** — stores the actual vertex data (positions, normals) on the GPU |
| `GL_TRIANGLES` | Tells OpenGL to interpret every 3 vertices as one triangle |
| `GL_FLOAT` | 32-bit floating-point number |

---

## 2. Application Initialization (`assignment.cpp`, Lines 177–208)

```cpp
glfwInit();
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

This block initializes GLFW and requests **OpenGL 3.3 Core Profile**:
- `GLFW_CONTEXT_VERSION_MAJOR, 3` — we want OpenGL version **3**.x
- `GLFW_CONTEXT_VERSION_MINOR, 3` — we want OpenGL version 3.**3**
- `GLFW_OPENGL_CORE_PROFILE` — use the modern "core" API (no deprecated functions)

```cpp
GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
    "Hover Bus - Advanced Lighting & 4-Viewport", NULL, NULL);
```

Creates a **1200×800** window. Parameters:
- `SCR_WIDTH` (1200) — window width in pixels
- `SCR_HEIGHT` (800) — window height in pixels
- `"Hover Bus..."` — window title text
- `NULL, NULL` — no fullscreen monitor, no shared context

```cpp
Shader ourShader("shader.vert", "shader.frag");
bus.init();
```

Loads and compiles both shader files into a GPU program, then initializes all primitive shapes (Cube, Cylinder, Torus) with their vertex data uploaded to the GPU.

---

## 3. The Render Loop (`assignment.cpp`, Lines 237–383)

The render loop runs **every frame** (~60 times per second) and has this structure:

### 3.1 Time Calculation (Lines 240–242)

```cpp
float currentFrame = static_cast<float>(glfwGetTime());
deltaTime = currentFrame - lastFrame;
lastFrame = currentFrame;
```

- `glfwGetTime()` — returns seconds since the program started (e.g., `3.456789`)
- `deltaTime` — time between this frame and the last (e.g., `0.016` for 60 FPS)
- Used to make all movement **frame-rate independent** (move by speed × time, not speed × frames)

### 3.2 Animation Updates (Lines 244–246)

```cpp
processInput(window);
bus.updateFan(deltaTime, fanSpinning);
bus.updateJetFlame(deltaTime);
```

These three calls happen before any drawing:
1. **`processInput`** — reads keyboard for camera movement, driving controls
2. **`updateFan`** — spins ceiling fans if toggled on (adds `200 * deltaTime` to rotation angle)
3. **`updateJetFlame`** — advances the flame flicker timer and hover bob offset

### 3.3 Screen Clear (Lines 248–255)

```cpp
int fbWidth, fbHeight;
glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

glViewport(0, 0, fbWidth, fbHeight);
glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

| Call | What It Does |
|------|-------------|
| `glfwGetFramebufferSize` | Gets the actual pixel size (handles high-DPI / Retina scaling) |
| `glViewport(0, 0, w, h)` | Tells OpenGL the entire framebuffer is our drawing area |
| `glClearColor(0.08, 0.08, 0.12, 1.0)` | Sets the background to **dark blue-gray** (R=8%, G=8%, B=12%) |
| `glClear(COLOR \| DEPTH)` | Wipes both the color and depth buffers clean for the new frame |

---

## 4. Light Setup (`assignment.cpp`, Lines 259–316)

All lighting parameters are sent to the GPU as **uniforms** (shader variables). The project uses **four types** of light:

### 4.1 Directional Light (Lines 261–264)

```cpp
ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
ourShader.setVec3("dirLight.ambient",   0.15f, 0.15f, 0.15f);
ourShader.setVec3("dirLight.diffuse",   0.7f,  0.7f,  0.6f);
ourShader.setVec3("dirLight.specular",  0.5f,  0.5f,  0.5f);
```

| Uniform | Parameters (R, G, B) | Meaning |
|---------|----------------------|---------|
| `direction` | `(-0.2, -1.0, -0.3)` | Sunlight coming from slightly left, **straight down**, and slightly forward |
| `ambient` | `(0.15, 0.15, 0.15)` | 15% gray base illumination everywhere |
| `diffuse` | `(0.7, 0.7, 0.6)` | Main sunlight color — warm white (slightly less blue) |
| `specular` | `(0.5, 0.5, 0.5)` | Highlight reflection brightness — medium white |

### 4.2 Point Lights (Lines 266–300)

Four colored point lights orbit with the bus:

```cpp
ourShader.setVec3("pointLights[0].position", busPosition + glm::vec3(5.0f, 5.0f, 5.0f));
ourShader.setVec3("pointLights[0].ambient",  0.05f, 0.0f, 0.0f);
ourShader.setVec3("pointLights[0].diffuse",  0.8f, 0.1f, 0.1f);
ourShader.setVec3("pointLights[0].specular", 1.0f, 0.2f, 0.2f);
ourShader.setFloat("pointLights[0].constant",  1.0f);
ourShader.setFloat("pointLights[0].linear",    0.09f);
ourShader.setFloat("pointLights[0].quadratic", 0.032f);
```

| Parameter | Value | Meaning |
|-----------|-------|---------|
| `position` | `busPosition + vec3(5,5,5)` | 5 units right, up, and forward from the bus center |
| `diffuse (0.8, 0.1, 0.1)` | Mostly red, tiny green/blue | The **color** this light casts — **Red** |
| `constant = 1.0` | Always 1.0 | Base value in attenuation formula (prevents division by zero) |
| `linear = 0.09` | Small | How quickly brightness drops with distance (linear term) |
| `quadratic = 0.032` | Smaller | How quickly brightness drops at larger distances (squared) |

**Attenuation formula** (computed in the fragment shader):
```
attenuation = 1.0 / (constant + linear × distance + quadratic × distance²)
```

The four point lights and their colors:
| Index | Color | Position Offset |
|-------|-------|----------------|
| 0 | Red | `(+5, +5, +5)` — front-right |
| 1 | Green | `(-5, +5, +5)` — front-left |
| 2 | Blue | `(+5, +5, -5)` — rear-right |
| 3 | White | `(-5, +5, -5)` — rear-left |

### 4.3 Spot Light (Lines 302–313)

```cpp
ourShader.setVec3("spotLight.position", cameraPos);
ourShader.setVec3("spotLight.direction",
    isDrivingMode ? glm::normalize(cameraLookAt - cameraPos) : getCameraFront());
ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));
```

The spotlight acts as a **flashlight** attached to the camera:
- **Position** = wherever the camera is
- **Direction** = wherever the camera is facing
- **`cutOff = cos(12.5°)`** = defines the cone angle. Fragments inside this cone get full light; outside get only ambient. The value is stored as a cosine because the shader compares `dot(lightDir, spotDir)` (which gives cosine) directly — avoiding an expensive `acos()` call.

### 4.4 Material (Lines 315–316)

```cpp
ourShader.setFloat("shininess", 32.0f);
ourShader.setVec3("viewPos", cameraPos);
```

| Uniform | Value | Purpose |
|---------|-------|---------|
| `shininess` (32.0) | Exponent in `pow()` | Controls how **tight** specular highlights are. Higher = smaller, sharper highlight. 32 is moderately shiny. |
| `viewPos` | Camera position | Needed to calculate the view direction for specular reflections |

---

## 5. Four-Viewport Rendering System (`assignment.cpp`, Lines 325–373)

### 5.1 Viewport Layout

```cpp
int hw = fbWidth / 2;
int hh = fbHeight / 2;

int vpX[] = { 0, hw, 0, hw };
int vpY[] = { hh, hh, 0, 0 };
```

The screen is divided into a **2×2 grid**:

```
┌──────────────┬──────────────┐
│   VP 0       │   VP 1       │
│  Top-Left    │  Top-Right   │
│  Perspective │  Top View    │
├──────────────┼──────────────┤
│   VP 2       │   VP 3       │
│  Bottom-Left │  Bottom-Right│
│  Front View  │  Side View   │
└──────────────┴──────────────┘
```

- `hw` = half the framebuffer width → each viewport is half-width
- `hh` = half the framebuffer height → each viewport is half-height
- `vpX[v], vpY[v]` = bottom-left corner of each viewport (OpenGL's origin is bottom-left)

### 5.2 Per-Viewport Rendering (Lines 333–373)

```cpp
for (int v = 0; v < 4; v++) {
    glViewport(vpX[v], vpY[v], hw, hh);

    glEnable(GL_SCISSOR_TEST);
    glScissor(vpX[v], vpY[v], hw, hh);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
```

| Call | Purpose |
|------|---------|
| `glViewport(x, y, w, h)` | Tells OpenGL where to draw this viewport |
| `glScissor + glClear(DEPTH)` | Clears ONLY this viewport's depth buffer (so depth from other viewports doesn't interfere) |

Each viewport has **independent** light toggles and camera:

```cpp
ViewportState& vs = viewports[v];
ourShader.setBool("dirLightOn",    vs.dirLightOn);
ourShader.setBool("pointLightsOn", vs.pointLightsOn);
ourShader.setBool("spotLightOn",   vs.spotLightOn);
```

The `ViewportState` struct:
```cpp
struct ViewportState {
    bool dirLightOn = true;      // Directional light
    bool pointLightsOn = true;   // All 4 point lights
    bool spotLightOn = true;     // Camera flashlight
    bool emissiveLightOn = true; // Flame / glow effects
    bool ambientOn = true;       // Ambient component
    bool diffuseOn = true;       // Diffuse component
    bool specularOn = true;      // Specular component
    int cameraMode = 0;          // 0=Perspective, 1=Top, 2=Front, 3=Side, 4=Iso, 5=Inside
};
```

---

## 6. Jet Engine Construction (`Bus.h`, Lines 339–501)

The jet engine is the most complex part of the model. It is drawn by `drawJetEngine()` and consists of **static hardware** plus **animated flame**.

### 6.1 Engine Housing (Lines 347–377)

The engine is positioned at the **rear** of the bus (bus body ends at X = +5.0):

```cpp
glm::mat4 engineBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.8f, 0.5f, 0.0f));
```

| Parameter | Value | Meaning |
|-----------|-------|---------|
| `5.8f` | X position | 0.8 units behind the bus rear face — engine protrudes backward |
| `0.5f` | Y position | Vertically centered on the bus body (body center is Y=0.5) |
| `0.0f` | Z position | Horizontally centered (no offset left or right) |

The engine casing is a **horizontal cylinder** rotated 90° so it points along the X-axis:

```cpp
model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
model = glm::scale(model, glm::vec3(1.4f, 1.8f, 1.4f));
cylinder.draw(shader, model, jetHousingColor);
```

| Scale Parameter | Axis | Value | Effect |
|----------------|------|-------|--------|
| `1.4f` | X (was Y before rotation) | Radius in one direction | Makes the cylinder **1.4 units in diameter** |
| `1.8f` | Y (was X before rotation) | Length | Makes the cylinder **1.8 units long** |
| `1.4f` | Z | Radius in other direction | Matches X for circular cross-section |

The engine is built from **five components**:

```
  Bus Body                Engine Housing         Nozzle      Nozzle Ring
  ┌──────┐  Intake Ring  ┌────────────┐  ┌──────┐  ╭──╮
  │      │  ╭──╮         │            │  │      │  │  │
  │      │──│  │─────────│            │──│      │──│  │
  │      │  ╰──╯         │            │  │      │  │  │
  │      │               └────────────┘  └──────┘  ╰──╯
  └──────┘
  X=5.0    X=5.0           X=5.8         X=6.9    X=7.1
```

Each component drawn in code:

| Component | Position (X) | Shape | Color |
|-----------|-------------|-------|-------|
| Intake Ring | 5.0 | Cylinder (1.5 × 0.3) | `jetInnerRingColor` — lighter metallic |
| Main Casing | 5.8 | Cylinder (1.4 × 1.8) | `jetHousingColor` — dark metallic |
| Nozzle Cone | 6.9 | Cylinder (1.1 × 0.4) | `jetNozzleColor` — darker metallic |
| Nozzle Ring | 7.1 | Torus (2.8 scale) | `jetNozzleColor` |
| Inner Exhaust | 6.5 | Cylinder (0.5 × 1.2) | Very dark gray `(0.15, 0.15, 0.18)` |

### 6.2 Support Struts (Lines 379–398)

Four rectangular **struts** connect the engine to the bus body at top, bottom, left, and right:

```cpp
// Top strut
model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 1.4f, 0.0f));
model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.3f));
cube.draw(shader, model, metalColor);
```

- `glm::vec3(0.8f, 0.15f, 0.3f)` means the strut is **0.8 wide × 0.15 tall × 0.3 deep** — a flat rectangular beam connecting the engine casing to the bus body.

### 6.3 Decorative Fin (Lines 400–403)

```cpp
model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 1.5f, 0.0f));
model = glm::scale(model, glm::vec3(1.5f, 0.4f, 0.08f));
cube.draw(shader, model, jetHousingColor);
```

A thin flat **dorsal fin** sitting on top of the engine housing, purely decorative.

---

## 7. Jet Flame Animation (`Bus.h`, Lines 405–501)

This is the **most technically interesting** part of the project. The flame only appears when `jetEngineOn == true` (the bus is moving forward).

### 7.1 Enabling Emissive + Additive Blending (Lines 408–414)

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE);   // Additive blending
glDepthMask(GL_FALSE);               // Don't write to depth buffer
shader.setBool("isEmissive", true);  // Skip lighting calculations
```

| Call | What It Does | Why |
|------|-------------|-----|
| `glEnable(GL_BLEND)` | Turns on color blending | Flame layers overlap and should **mix colors** |
| `glBlendFunc(SRC_ALPHA, ONE)` | **Additive** blending | Instead of replacing colors, it **adds** the flame color on top. White flame + orange flame = brighter orange. This creates a natural **glow** effect. |
| `glDepthMask(GL_FALSE)` | Disables depth writes | Prevents transparent flame layers from occluding each other |
| `isEmissive = true` | Tells fragment shader to skip Phong | Flame emits its own light — applying directional/point/spot lighting to it would be wrong |

In the fragment shader, emissive mode does this:
```glsl
if (isEmissive) {
    FragColor = vec4(objectColor, alpha);   // Output color directly, no lighting math
    return;
}
```

### 7.2 Afterburner Glow Disc (Lines 419–425)

```cpp
float glowPulse = 0.85f + 0.15f * sin(t * 25.0f);
```

- `t` is `jetFlameFlicker`, which increases by `8.0 * deltaTime` each frame
- `sin(t * 25.0f)` oscillates between -1 and +1, but multiplied by 0.15 and offset by 0.85, the result oscillates between **0.70** and **1.00**
- This creates a rapid **pulsing** effect at the nozzle exit — a bright white-blue disc

```cpp
model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(nozzleX, 0.5f, 0.0f));
model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
model = glm::scale(model, glm::vec3(0.95f * glowPulse, 0.08f, 0.95f * glowPulse));
cylinder.draw(shader, model, glm::vec3(1.0f, 0.95f, 0.85f));
```

This draws a very **flat cylinder** (only 0.08 units thick) at the nozzle exit point (`nozzleX = 7.15`). The color `(1.0, 0.95, 0.85)` is near-white with a slight warm tint.

### 7.3 Multi-Layer Flame System (Lines 427–480)

The flame is built from **9 concentric cylinders overlapping**, each larger and more transparent. This is the `FlameLayer` struct:

```cpp
struct FlameLayer {
    float lengthScale;   // How long this layer extends (1.0 = full, 0.3 = short)
    float radiusScale;   // How wide this layer is (0.2 = thin, 1.05 = wide)
    float freqOffset;    // Unique frequency so it flickers independently
    float alphaVal;      // Transparency (0.95 = nearly opaque, 0.15 = almost invisible)
    glm::vec3 color;     // The RGB color of this layer
};
```

The 9 layers from **inside to outside**:

| # | Layer | Length | Radius | Alpha | Color Description |
|---|-------|--------|--------|-------|-------------------|
| 0 | Inner core | 1.00 | 0.20 | 0.95 | Near-white `(1.0, 0.97, 0.85)` |
| 1 | Inner core | 0.92 | 0.28 | 0.85 | Warm white-yellow `(1.0, 0.92, 0.55)` |
| 2 | Hot core | 0.85 | 0.38 | 0.75 | Bright yellow-orange `flameColorCore` |
| 3 | Hot core | 0.78 | 0.45 | 0.65 | Golden `(1.0, 0.75, 0.2)` |
| 4 | Mid zone | 0.68 | 0.55 | 0.55 | Orange `flameColorMid` |
| 5 | Mid zone | 0.60 | 0.65 | 0.45 | Deep orange `(1.0, 0.4, 0.08)` |
| 6 | Outer zone | 0.50 | 0.78 | 0.35 | Red-orange `flameColorOuter` |
| 7 | Outer zone | 0.40 | 0.90 | 0.25 | Dark red `(0.8, 0.15, 0.03)` |
| 8 | Wispy edge | 0.30 | 1.05 | 0.15 | Deep red, nearly invisible `(0.5, 0.08, 0.02)` |

**Key pattern**: inner layers are *longer, thinner, brighter, and more opaque*; outer layers are *shorter, wider, dimmer, and more transparent*. This mimics real jet flames where the hottest gas is in the narrow core.

### 7.4 Turbulence Animation (Lines 459–468)

Each layer flickers independently:

```cpp
float baseLen = 3.0f * L.lengthScale;
float turbulence = 0.5f * sin(t * (14.0f + L.freqOffset))
                 + 0.25f * sin(t * (21.0f + L.freqOffset * 0.7f))
                 + 0.15f * sin(t * (33.0f + L.freqOffset * 1.3f));
float len = baseLen + turbulence * L.lengthScale;
```

This uses **multi-frequency sine wave noise** — three sine waves of different speeds are added together:

| Term | Frequency Range | Amplitude | Effect |
|------|----------------|-----------|--------|
| `sin(t * 14...)` | 14–33 Hz | 0.50 | Main **large** oscillation |
| `sin(t * 21...)` | 21–36 Hz | 0.25 | Medium **detail** variation |
| `sin(t * 33...)` | 33–58 Hz | 0.15 | Fast **shimmer** detail |

The `freqOffset` of each layer (0.0, 2.1, 4.3, 6.7, ...) ensures that **no two layers flicker in sync**, creating organic-looking turbulence.

Similarly, the flame wobbles in Y and Z:
```cpp
float yOff = 0.03f * sin(t * (9.0f + L.freqOffset * 0.3f));
float zOff = 0.03f * sin(t * (7.0f + L.freqOffset * 0.6f));
```

These tiny offsets (±0.03 units) make the flame subtly dance side to side and up/down.

### 7.5 How Each Flame Cylinder Is Drawn (Lines 474–479)

```cpp
model = parent * glm::translate(glm::mat4(1.0f),
    glm::vec3(nozzleX + len * 0.5f, 0.5f + yOff, zOff));
model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
model = glm::scale(model, glm::vec3(rad, len, rad));
cylinder.draw(shader, model, L.color);
```

| Transform | Value | Meaning |
|-----------|-------|---------|
| **Translate** X | `nozzleX + len * 0.5` | Centers the cylinder at the midpoint behind the nozzle |
| **Translate** Y | `0.5 + yOff` | Vertical center of bus body + tiny wobble |
| **Translate** Z | `zOff` | Tiny horizontal wobble |
| **Rotate** 90° around Z | — | Lays the cylinder on its side (pointing along X-axis, like the engine) |
| **Scale** X | `rad` | Vertical radius (after rotation) |
| **Scale** Y | `len` | Length of the flame |
| **Scale** Z | `rad` | Horizontal radius |

### 7.6 Flickering Sparks (Lines 482–494)

```cpp
for (int s = 0; s < 5; s++) {
    float sparkPhase = t * (20.0f + s * 7.3f) + s * 1.7f;
    float sparkX = nozzleX + 0.5f + fmod(sparkPhase * 0.8f, 2.5f);
    float sparkY = 0.5f + 0.15f * sin(sparkPhase * 3.0f);
    float sparkZ = 0.12f * sin(sparkPhase * 2.5f + s * 0.9f);
    float sparkSize = 0.04f + 0.02f * sin(sparkPhase * 5.0f);
```

Five tiny bright dots are scattered within the flame:
- **`sparkPhase`** — each spark has its own time offset (`s * 7.3` and `s * 1.7`) so they move independently
- **`fmod(sparkPhase * 0.8, 2.5)`** — the `fmod` (modulo) wraps the X position between 0 and 2.5, making sparks loop from the nozzle outward and then snap back
- **`sparkSize`** — oscillates between 0.02 and 0.06 units (tiny pulsing dots)
- **Color** `(1.0, 0.95, 0.7)` — bright warm white

### 7.7 Restoring Render State (Lines 496–500)

```cpp
shader.setBool("isEmissive", false);
shader.setFloat("alpha", 1.0f);
glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
```

After drawing the flame, everything is reset so that subsequent objects (hover pads, next viewport) render with normal lighting and opaque depth.

---

## 8. Flame Animation Update Function (`Bus.h`, Lines 600–609)

```cpp
void updateJetFlame(float deltaTime) {
    hoverTime += deltaTime;
    if (jetEngineOn) {
        jetFlameFlicker += deltaTime * 8.0f;  // Fast flickering
        if (jetFlameFlicker > 100.0f) jetFlameFlicker -= 100.0f;
    }
    hoverBobOffset = 0.15f * sin(hoverTime * 2.5f);
}
```

| Variable | Update Rate | Purpose |
|----------|-------------|---------|
| `hoverTime` | `+= deltaTime` (1.0/sec) | General time accumulator for hover glow pulsation |
| `jetFlameFlicker` | `+= deltaTime * 8.0` (8.0/sec) | Drives all flame sine wave calculations; the `× 8` makes it 8× faster than real time |
| `hoverBobOffset` | `0.15 * sin(hoverTime * 2.5)` | The bus bobs up and down ±0.15 units at 2.5 Hz (gentle floating effect) |

The `if (jetFlameFlicker > 100.0f)` prevents floating-point overflow — it wraps back to reset but since `sin()` is periodic this doesn't cause a visual jump.

---

## 9. Hover Skirts / Pads (`Bus.h`, Lines 504–572)

Four anti-gravity hover pads replace the wheels:

```cpp
float padPositions[4][2] = {
    {-3.5f, -1.3f},   // Front Left
    {-3.5f,  1.3f},   // Front Right
    { 3.5f, -1.3f},   // Rear Left
    { 3.5f,  1.3f}    // Rear Right
};
```

Each pad has **3 glowing layers** rendered with the same emissive + additive blending technique as the jet flame:

| Component | Alpha | Color | Effect |
|-----------|-------|-------|--------|
| Glow disc (flat cylinder) | 0.7 | `hoverPadColor` (blue `0.3, 0.6, 0.9`) | Main blue glow underneath |
| Glow ring (torus) | 0.5 | `hoverGlowColor` (bright blue `0.4, 0.7, 1.0`) | Halo ring around the pad |
| Inner core (small cylinder) | 0.85 | White-blue `(0.6, 0.85, 1.0)` | Brightest center point |

All three layers pulse with:
```cpp
float glowPulse = 0.8f + 0.2f * sin(hoverTime * 5.0f);     // 5 Hz pulsation
float padBrightness = 0.7f + 0.3f * sin(hoverTime * 3.0f);  // 3 Hz brightness wave
```

Additionally, a **blue underglow strip** runs along the bottom of the bus:
```cpp
float bellyGlow = 0.6f + 0.15f * sin(hoverTime * 4.0f);
```

---

## 10. Vertex Shader (`shader.vert`)

```glsl
#version 330 core
layout (location = 0) in vec3 aPos;      // Attribute 0: vertex position
layout (location = 1) in vec3 aNormal;   // Attribute 1: vertex normal

out vec3 FragPos;    // World-space position → sent to fragment shader
out vec3 Normal;     // World-space normal → sent to fragment shader

uniform mat4 model;       // Object → World transform
uniform mat4 view;        // World → Camera transform
uniform mat4 projection;  // Camera → Screen (perspective) transform

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
```

### Line-by-line Explanation:

| Line | Code | What It Does |
|------|------|-------------|
| `layout (location = 0) in vec3 aPos` | Reads vertex position from attribute 0 of the VBO | Each vertex has 6 floats: `[x,y,z, nx,ny,nz]`. The first 3 are position. |
| `layout (location = 1) in vec3 aNormal` | Reads vertex normal from attribute 1 | The last 3 floats are the normal vector (perpendicular to the surface) |
| `FragPos = vec3(model * vec4(aPos, 1.0))` | Transforms position into **world space** | `vec4(aPos, 1.0)` — the `1.0` is the `w` component needed for matrix multiplication with translation |
| `Normal = mat3(transpose(inverse(model))) * aNormal` | Transforms normal correctly | **Why not just `model * normal`?** Because non-uniform scaling would distort normals. `transpose(inverse(model))` is the **normal matrix** that cancels out scaling distortion. |
| `gl_Position = projection * view * vec4(FragPos, 1.0)` | Final screen-space position | Applies view (camera) then projection (perspective). This is what OpenGL uses to position the pixel on screen. |

---

## 11. Fragment Shader (`shader.frag`)

### 11.1 Light Struct Definitions (Lines 8–35)

```glsl
struct DirLight {
    vec3 direction;   // The direction light rays travel
    vec3 ambient;     // Color of ambient (everywhere) component
    vec3 diffuse;     // Color of diffuse (angle-dependent) component
    vec3 specular;    // Color of specular (highlight) component
};
```

```glsl
struct PointLight {
    vec3 position;    // Where the light is in world space
    vec3 ambient;     // ...(same as above)
    vec3 diffuse;
    vec3 specular;
    float constant;   // Attenuation: constant term (always 1.0)
    float linear;     // Attenuation: linear falloff rate
    float quadratic;  // Attenuation: quadratic falloff rate
};
```

```glsl
struct SpotLight {
    vec3 position;    // Where the spotlight is
    vec3 direction;   // Where it's pointing
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float cutOff;     // cos(cone angle) — fragments outside this angle get no light
    float constant;
    float linear;
    float quadratic;
};
```

### 11.2 Directional Light Calculation (`CalcDirLight`, Lines 70–92)

```glsl
vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction);
```

The `direction` is the direction light travels (e.g., "down"). We **negate** it to get the direction **toward** the light (which is what the dot product needs).

**Diffuse** (Lambert's Law):
```glsl
float diff = max(dot(normal, lightDir), 0.0);
vec3 diffuse = light.diffuse * diff * objectColor;
```
- `dot(normal, lightDir)` — cosine of angle between surface and light
- `max(..., 0.0)` — clamp to zero (surfaces facing away from light get no diffuse)
- Multiply by `objectColor` — the surface's material color

**Specular** (Phong Reflection):
```glsl
vec3 reflectDir = reflect(-lightDir, normal);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
vec3 specular = light.specular * spec;
```
- `reflect(-lightDir, normal)` — mirrors the light direction around the surface normal
- `dot(viewDir, reflectDir)` — how closely the reflected ray points at the camera (1.0 = perfect mirror)
- `pow(..., shininess)` — raises to the 32nd power → only near-perfect reflections produce a visible highlight
- Note: specular is **NOT** multiplied by `objectColor` — highlights are the light's own color

### 11.3 Point Light Calculation (`CalcPointLight`, Lines 94–125)

Same as directional, but with **distance attenuation**:

```glsl
float distance = length(light.position - fragPos);
float attenuation = 1.0 / (light.constant + light.linear * distance
                           + light.quadratic * (distance * distance));
```

All three components (ambient, diffuse, specular) are multiplied by `attenuation`:
```glsl
ambient  *= attenuation;
diffuse  *= attenuation;
specular *= attenuation;
```

This makes point lights **fade with distance** — brighter up close, dim far away.

### 11.4 Spot Light Calculation (`CalcSpotLight`, Lines 127–165)

Combines the point light attenuation with a **cone test**:

```glsl
float theta = dot(lightDir, normalize(-light.direction));

if (theta > light.cutOff) {
    // Inside the cone → full lighting
} else {
    // Outside the cone → ambient only
}
```

- `theta` = cosine of the angle between the fragment direction and the spotlight's center direction
- Since `cos(0°) = 1.0` and `cos(12.5°) ≈ 0.976`, `theta > cutOff` means the fragment is **within** the cone

### 11.5 Main Fragment Shader (Lines 168–198)

```glsl
void main() {
    if (isEmissive) {
        FragColor = vec4(objectColor, alpha);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 result = vec3(0.0);

    if (dirLightOn)    result += CalcDirLight(dirLight, norm, viewDir);
    if (pointLightsOn) for (int i = 0; i < 4; i++)
                           result += CalcPointLight(pointLights[i], norm, FragPos, viewDir);
    if (spotLightOn)   result += CalcSpotLight(spotLight, norm, FragPos, viewDir);

    result = clamp(result, 0.0, 1.0);
    FragColor = vec4(result, alpha);
}
```

**Flow:**
1. **Emissive check** — if the object is a flame/glow, output its color directly and skip all lighting
2. **Normalize** inputs — interpolation across the triangle can denormalize the normal
3. **Accumulate** all enabled light contributions by adding them together
4. **Clamp** to [0, 1] to prevent oversaturated (>1.0) values
5. **Output** as RGBA with the `alpha` transparency value

---

## 12. Primitive Geometry Classes (`Primitives.h`)

### 12.1 Cube (Lines 18–100)

The cube is a hardcoded array of **36 vertices** (6 faces × 2 triangles × 3 vertices). Each vertex has 6 floats:

```
[position.x, position.y, position.z, normal.x, normal.y, normal.z]
```

Example — back face top-right vertex:
```cpp
0.5f, 0.5f, -0.5f,  0.0f, 0.0f, -1.0f
//                   ↑ Normal pointing toward -Z (backward)
```

The `draw()` method:
```cpp
void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
    shader.setMat4("model", model);       // Upload transform matrix to GPU
    shader.setVec3("objectColor", color); // Upload color to GPU
    glBindVertexArray(VAO);               // Activate this cube's vertex data
    glDrawArrays(GL_TRIANGLES, 0, 36);    // Draw all 36 vertices as 12 triangles
}
```

### 12.2 Cylinder (Lines 105–198)

Generated **parametrically** with configurable sectors (default 36):

```cpp
void init(int sectors = 36) {
    float sectorStep = 2.0f * M_PI / sectors;  // Angle per sector (10° for 36 sectors)
```

**Three parts** are generated:
1. **Side surface** — a strip of rectangles (each split into 2 triangles) wrapping around the cylinder
2. **Top cap** — fan of triangles from center to edge
3. **Bottom cap** — same as top, facing downward

The normals for the side surface point **radially outward**:
```cpp
float nx1 = cos(angle1), nz1 = sin(angle1);  // Unit vector pointing outward at this angle
```

### 12.3 Torus (Lines 203–280)

The torus (donut shape) is defined by **two radii**:

```cpp
void init(float mainRadius = 0.4f, float tubeRadius = 0.1f,
          int mainSegments = 24, int tubeSegments = 12)
```

| Parameter | Default | Meaning |
|-----------|---------|---------|
| `mainRadius` | 0.4 | Distance from the center of the donut to the center of the tube |
| `tubeRadius` | 0.1 | Radius of the tube cross-section |
| `mainSegments` | 24 | Subdivisions around the main ring |
| `tubeSegments` | 12 | Subdivisions around the tube cross-section |

The parametric formula:
```cpp
float x = (mainRadius + tubeRadius * cos(phi)) * cos(theta);
float y = tubeRadius * sin(phi);
float z = (mainRadius + tubeRadius * cos(phi)) * sin(theta);
```
- `theta` sweeps around the main ring (0 to 2π)
- `phi` sweeps around the tube cross-section (0 to 2π)

---

## 13. Shader Class (`Shader.h`)

### 13.1 Constructor — Load, Compile, Link (Lines 19–72)

```cpp
Shader(const char* vertexPath, const char* fragmentPath)
```

Steps:
1. **Read** both shader source files from disk using `ifstream`
2. **Compile** each as a separate shader object (`GL_VERTEX_SHADER`, `GL_FRAGMENT_SHADER`)
3. **Link** both into a single shader **program** (`glCreateProgram`)
4. **Delete** the individual shader objects — they are now embedded in the program

### 13.2 Uniform Setters (Lines 81–136)

The `Shader` class provides type-safe wrappers around OpenGL uniform upload functions:

```cpp
void setBool(const std::string& name, bool value) const {
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}
```

| Method | OpenGL Call | Data Uploaded |
|--------|-----------|---------------|
| `setBool(name, val)` | `glUniform1i` | Boolean as integer (0 or 1) |
| `setInt(name, val)` | `glUniform1i` | Integer value |
| `setFloat(name, val)` | `glUniform1f` | Single float |
| `setVec3(name, x, y, z)` | `glUniform3f` | Three floats (color or position) |
| `setVec3(name, vec3)` | `glUniform3fv` | `vec3` object |
| `setMat4(name, mat)` | `glUniformMatrix4fv` | 4×4 matrix (16 floats) |

`glGetUniformLocation(ID, name)` finds the memory slot for the named variable inside the compiled shader program on the GPU.

---

## 14. Camera System (`assignment.cpp`, Lines 16–131)

### 14.1 Free Camera Variables (Lines 18–22)

```cpp
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
float cameraPitch = -15.0f;    // Up/Down rotation (degrees)
float cameraYaw = -90.0f;      // Left/Right rotation (degrees)
float cameraRoll = 0.0f;       // Tilt rotation (degrees)
```

- Starting position: **X=0 (centered), Y=5 (5 units up), Z=20 (20 units back)**
- Starting angles: **Yaw=-90°** (facing forward along -Z), **Pitch=-15°** (looking slightly down)

### 14.2 Camera Direction Calculation (Lines 104–118)

```cpp
glm::vec3 getCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(front);
}
```

This converts **Euler angles** (pitch + yaw) into a **direction vector** using spherical coordinates:
- `cos(yaw) * cos(pitch)` → X component (left/right)
- `sin(pitch)` → Y component (up/down)
- `sin(yaw) * cos(pitch)` → Z component (forward/back)

### 14.3 Custom lookAt (Lines 84–97)

```cpp
glm::mat4 myLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
    glm::vec3 f = glm::normalize(center - eye);      // Forward axis
    glm::vec3 s = glm::normalize(glm::cross(f, up));  // Right axis
    glm::vec3 u = glm::cross(s, f);                   // Recomputed up axis
```

Constructs a **view matrix** from three parameters:
- `eye` — where the camera is
- `center` — what point the camera looks at
- `up` — which direction is "up"

The function builds three orthogonal axes (forward, right, up) and assembles them into a 4×4 matrix that transforms world coordinates into camera-relative coordinates.

---

## 15. Driving Simulation (`assignment.cpp`, Lines 396–437)

### 15.1 Physics Constants

```cpp
const float ACCELERATION = 15.0f;   // Speed increase per second when W is held
const float DECELERATION = 10.0f;   // Speed decrease per second when coasting
const float MAX_SPEED = 20.0f;      // Maximum forward/reverse speed
const float STEER_SPEED = 60.0f;    // Degrees per second of steering change
const float MAX_STEER = 35.0f;      // Maximum steering angle (degrees)
```

### 15.2 Movement Update

```cpp
float rad = glm::radians(busYaw);
glm::vec3 forwardDir(-cos(rad), 0.0f, sin(rad));
```

Converts the bus's heading angle into a directional vector. The bus moves along this direction:

```cpp
busPosition += forwardDir * busSpeed * deltaTime;
```

**Turning** is proportional to both speed and steering angle:
```cpp
busYaw += (busSteerAngle * busSpeed * deltaTime * 0.1f);
```

This means:
- At higher speeds, turns happen faster (realistic)
- At zero speed, steering changes the angle but doesn't turn the bus

### 15.3 Jet Engine Activation

```cpp
bus.jetEngineOn = (busSpeed > 0.1f);
```

The jet flame appears **automatically** when the bus exceeds 0.1 speed units — no manual toggle needed.

### 15.4 Chase Camera (Lines 428–437)

```cpp
glm::vec3 camOffset(-12.0f, 5.0f, 0.0f);
```

In driving mode, the camera positions itself **12 units behind** and **5 units above** the bus, rotated to match the bus's heading. This creates a third-person chase-camera view.

---

## 16. Input Handling (`assignment.cpp`, Lines 469–544)

### 16.1 Key Callback Structure

```cpp
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;   // Only respond to key press, not release
    ViewportState& vs = viewports[activeViewport];
```

The `&` (reference) operator means `vs` directly modifies the active viewport's state — no copy is made.

### 16.2 Control Summary

| Key | Action | Variable Modified |
|-----|--------|-------------------|
| `TAB` | Cycle active viewport (0→1→2→3→0) | `activeViewport` |
| `1` | Toggle directional light | `vs.dirLightOn` |
| `2` | Toggle point lights | `vs.pointLightsOn` |
| `3` | Toggle spot light | `vs.spotLightOn` |
| `4` | Toggle emissive light | `vs.emissiveLightOn` |
| `5` | Toggle ambient component | `vs.ambientOn` |
| `6` | Toggle diffuse component | `vs.diffuseOn` |
| `7` | Toggle specular component | `vs.specularOn` |
| `8` | Cycle camera mode | `vs.cameraMode` |
| `B` | Toggle front door open/close | `frontDoorAngle` |
| `G` | Toggle ceiling fan spin | `fanSpinning` |
| `L` | Toggle interior lights | `lightOn` |
| `K` | Toggle driving mode | `isDrivingMode` |
| `WASD` | Move camera (free) / Drive bus (driving) | `cameraPos` / `busPosition` |
| `XYZ` | Rotate camera pitch/yaw/roll | `cameraPitch`, `cameraYaw`, `cameraRoll` |
| `E/R` | Camera up/down | `cameraPos.y` |
| `F` | Orbit camera around bus | `orbitAngle` |

---

## 17. Emissive Light Toggle — Per Viewport (`assignment.cpp`, Lines 367–372)

```cpp
bool savedJetOn = bus.jetEngineOn;
if (!vs.emissiveLightOn) {
    bus.jetEngineOn = false;   // Suppress flame emissive
}
bus.draw(ourShader, busTransform);
bus.jetEngineOn = savedJetOn;  // Restore
```

When the emissive toggle is **OFF** for a viewport, the code:
1. **Saves** the real `jetEngineOn` state
2. **Temporarily sets** it to `false` so `drawJetEngine()` skips the flame section
3. **Draws** the bus without flame
4. **Restores** the real state for the next viewport

This allows each viewport to independently show or hide the flame effect.

---

## 18. Summary — How Everything Connects

```
┌────────────────────────────────────────────────────────────┐
│                     RENDER LOOP (per frame)                │
│                                                            │
│  1. Calculate deltaTime                                    │
│  2. Process keyboard input                                 │
│  3. Update animations (fan, flame, hover)                  │
│  4. Clear screen                                           │
│  5. Upload light parameters to shader                      │
│  6. Calculate bus transform (position + rotation)          │
│  7. FOR each viewport (0..3):                              │
│     a. Set viewport rectangle (glViewport)                 │
│     b. Upload per-viewport toggles to shader               │
│     c. Set camera mode → compute view matrix               │
│     d. Compute projection matrix                           │
│     e. Upload view + projection                            │
│     f. Call bus.draw() which calls:                         │
│        ├── drawExterior()  → body, windows, doors, lights  │
│        ├── drawInterior()  → seats, rails, dashboard, fans │
│        ├── drawJetEngine() → housing + animated flame      │
│        └── drawHoverSkirts() → hover pads + glow effects   │
│  8. Swap front/back buffers (display result)               │
│  9. Poll window events                                     │
└────────────────────────────────────────────────────────────┘
```

Each `draw()` call on Cube/Cylinder/Torus:
1. Uploads the `model` matrix (position + rotation + scale of this specific part)
2. Uploads the `objectColor` (RGB)
3. Binds the VAO (vertex data on GPU)
4. Calls `glDrawArrays` → GPU processes vertices through the vertex shader → fragment shader → pixels on screen
