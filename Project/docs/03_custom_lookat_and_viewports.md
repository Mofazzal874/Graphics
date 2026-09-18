# Custom lookAt & 4-Viewport System

## The View Matrix — What Does lookAt Do?

The view matrix transforms world coordinates into **camera coordinates** (what the camera sees). The `lookAt` function builds this matrix from 3 inputs:

```
myLookAt(eye, center, up)
         │     │       │
         │     │       └─ Which direction is "up" in the world
         │     └─ The point the camera is looking at
         └─ Where the camera is positioned
```

---

## Our Custom `myLookAt()` — Step by Step

### Step 1: Calculate 3 orthogonal axes

```cpp
glm::vec3 f = glm::normalize(center - eye);      // Forward vector
glm::vec3 s = glm::normalize(glm::cross(f, up));  // Right vector
glm::vec3 u = glm::cross(s, f);                   // True Up vector
```

**Example:**
```
eye    = (0, 5, 20)   // Camera behind and above bus
center = (0, 0, 0)    // Looking at origin

f = normalize((0,0,0) - (0,5,20)) = normalize(0, -5, -20) = (0, -0.24, -0.97)
s = normalize(cross(f, worldUp))   = (1, 0, 0)    // Points right
u = cross(s, f)                    = (0, 0.97, -0.24)  // True up
```

### Step 2: Build the 4×4 matrix

```cpp
// The matrix combines rotation (axes) and translation (position)
result = | s.x   u.x   -f.x    0 |
         | s.y   u.y   -f.y    0 |
         | s.z   u.z   -f.z    0 |
         | -s·eye -u·eye f·eye  1 |
```

The top-left 3×3 is the **rotation** (aligns world axes to camera axes).
The bottom row is the **translation** (moves the world so the camera is at origin).

### Why `transpose(inverse(model))` for normals?

In the vertex shader:
```glsl
Normal = mat3(transpose(inverse(model))) * aNormal;
```

When a model is **scaled non-uniformly** (e.g., stretched in X but not Y), normals get distorted. The `transpose(inverse())` corrects this:

```
Original cube:        Stretched 2× in X:
    ↑ normal              ↗ WRONG normal (tilted)
 ┌──┐                  ┌────┐
 │  │                  │    │
 └──┘                  └────┘
                           ↑ CORRECT normal (still up)
                             (after transpose(inverse) fix)
```

---

## 4-Viewport Rendering System

### How It Works

Each frame, the scene is rendered **4 times** with different `glViewport` calls:

```
Window (1200 × 800)
┌──────────600──────────┬──────────600──────────┐
│                       │                       │
│  Viewport 0 (TL)      │  Viewport 1 (TR)      │ 400px
│  glViewport(0,400,    │  glViewport(600,400,  │
│             600,400)  │             600,400)  │
│                       │                       │
├───────────────────────┼───────────────────────┤
│                       │                       │
│  Viewport 2 (BL)      │  Viewport 3 (BR)      │ 400px
│  glViewport(0,0,      │  glViewport(600,0,    │
│             600,400)  │             600,400)  │
│                       │                       │
└───────────────────────┴───────────────────────┘
```

### Per-Viewport State

Each viewport has **independent** lighting and camera settings:

```cpp
struct ViewportState {
    bool dirLightOn = true;       // Directional light
    bool pointLightsOn = true;    // Point lights
    bool spotLightOn = true;      // Spot light
    bool emissiveLightOn = true;  // Emissive light
    bool ambientOn = true;        // Ambient component
    bool diffuseOn = true;        // Diffuse component
    bool specularOn = true;       // Specular component
    int cameraMode = 0;           // Camera angle
};

ViewportState viewports[4];   // 4 independent states
int activeViewport = 0;        // Which one receives key inputs
```

### Render Loop (per viewport):

```cpp
for (int v = 0; v < 4; v++) {
    glViewport(vpX[v], vpY[v], hw, hh);    // Set drawing area

    // Scissor ensures depth clear doesn't bleed into other viewports
    glEnable(GL_SCISSOR_TEST);
    glScissor(vpX[v], vpY[v], hw, hh);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);

    // Apply this viewport's own lighting state
    ourShader.setBool("dirLightOn",    viewports[v].dirLightOn);
    ourShader.setBool("pointLightsOn", viewports[v].pointLightsOn);
    // ... etc

    // Apply this viewport's camera
    glm::mat4 view = getViewForMode(viewports[v].cameraMode);

    bus.draw(ourShader, busTransform);   // Draw the bus
}
```

### Camera Modes

| Mode | View | Description |
|------|------|-------------|
| 0 | Perspective | User-controlled free camera |
| 1 | Top View | Looks straight down at bus |
| 2 | Front View | Looks at the front of bus |
| 3 | Side View | Looks at the side of bus |
| 4 | Isometric | Angled 45° view (3D feel) |
| 5 | Inside View | Camera inside the bus |

**Cycle with:** Key **8** (changes only the active viewport)

### Controls Summary

| Key | Action | Scope |
|-----|--------|-------|
| TAB | Select next viewport | Global |
| 1-4 | Toggle light types | Active viewport only |
| 5-7 | Toggle light components | Active viewport only |
| 8 | Cycle camera mode | Active viewport only |
