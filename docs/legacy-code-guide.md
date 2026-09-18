# 🚌 OpenGL Bus Simulation - Complete Codebase Documentation

A comprehensive 3D Bus Simulation built with **Modern OpenGL 3.3**, featuring a fully interactive coach bus with a flying simulator camera system and driving mode.

---

## 📁 Project Structure

```
Lab3_assignment/
├── assignment.cpp      # Main application (entry point, render loop, input handling)
├── Bus.h               # Bus class (3D model, all components, animations)
├── Primitives.h        # Geometric primitives (Cube, Cylinder, Torus)
├── Shader.h            # Shader program loader and uniform management
├── shader.vert         # Vertex shader (GLSL)
├── shader.frag         # Fragment shader (GLSL)
└── README.md           # This documentation
```

---

## 🎯 Understanding Modern OpenGL: Key Concepts

### What is Modern OpenGL?

Modern OpenGL (version 3.3+) uses a **programmable rendering pipeline** instead of the fixed-function pipeline of legacy OpenGL. This means:

1. **You write shaders** - Small GPU programs that define how vertices and pixels are processed
2. **You manage data explicitly** - Using Vertex Buffer Objects (VBO) and Vertex Array Objects (VAO)
3. **You control transformations** - Using matrix math (via GLM library)

### The OpenGL Rendering Pipeline (Simplified)

```
┌─────────────────────────────────────────────────────────────────────────┐
│                        YOUR C++ APPLICATION                              │
│   • Define vertex data (positions, normals, colors)                      │
│   • Create VAO/VBO and upload data to GPU                                │
│   • Set up transformation matrices                                       │
│   • Issue draw calls                                                     │
└────────────────────────────────────┬────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                      VERTEX SHADER (runs per vertex)                     │
│   • Transform vertex from local → world → view → clip space             │
│   • Pass data to fragment shader                                         │
│   • In this project: 36 vertices for cube = 36 shader executions        │
└────────────────────────────────────┬────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                         RASTERIZATION (GPU)                              │
│   • Convert triangles to pixel fragments                                 │
│   • Interpolate data between vertices                                    │
│   • A single triangle can generate thousands of pixels!                  │
└────────────────────────────────────┬────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                   FRAGMENT SHADER (runs per pixel)                       │
│   • Calculate final color for each pixel                                 │
│   • Apply lighting, textures, effects                                    │
│   • 1200×800 window = up to 960,000 pixels per frame!                   │
└────────────────────────────────────┬────────────────────────────────────┘
                                 │
                                 ▼
┌─────────────────────────────────────────────────────────────────────────┐
│                        FRAMEBUFFER (Screen)                              │
│   • Final image displayed to user                                        │
│   • At 60 FPS, this entire pipeline runs 60 times per second!           │
└─────────────────────────────────────────────────────────────────────────┘
```

---

## 🌐 Coordinate Spaces Explained

Understanding coordinate spaces is **CRUCIAL** in 3D graphics. Here's the journey of a vertex:

```
LOCAL SPACE → MODEL MATRIX → WORLD SPACE → VIEW MATRIX → VIEW SPACE → PROJECTION MATRIX → CLIP SPACE → SCREEN
```

### 1. Local/Object Space (Model Space)

The coordinate system where vertices are defined **relative to the object's center**. Every 3D model is created with its center at origin (0, 0, 0).

**Example: Our Cube in Primitives.h**
```cpp
float vertices[] = {
    // Each vertex has: position (x,y,z) + normal (nx,ny,nz)
    // Back face (z = -0.5)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Bottom-left-back   (vertex 0)
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Bottom-right-back  (vertex 1)
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Top-right-back     (vertex 2)
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Top-right-back     (vertex 3)
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Top-left-back      (vertex 4)
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  // Bottom-left-back   (vertex 5)
    // ... 30 more vertices for other 5 faces
};
```

**Visual representation of the unit cube (8 corner points):**
```
                Y (up)
                │
                │    
        4●──────┼─────●5
        /│      │    /│         Vertex Coordinates:
       / │      │   / │         ─────────────────────
      /  │      │  /  │         0: (-0.5, -0.5, -0.5)
    7●───┼──────┼─●6  │         1: (+0.5, -0.5, -0.5)
     │   │      │ │   │         2: (+0.5, +0.5, -0.5)
     │  0●──────┼─┼───●1────X   3: (-0.5, +0.5, -0.5)
     │  /       │ │  /          4: (-0.5, +0.5, +0.5)
     │ /        │ │ /           5: (+0.5, +0.5, +0.5)
     │/         │ │/            6: (+0.5, -0.5, +0.5)
    3●──────────┼─●2            7: (-0.5, -0.5, +0.5)
                │
                Z (toward you)
   
   The cube spans from -0.5 to +0.5 on each axis
   Total size: 1.0 × 1.0 × 1.0 unit cube
   Center: (0, 0, 0) - the origin
```

**Why 36 vertices instead of 8?**
A cube has 8 corners, but OpenGL draws triangles. Each face needs:
- 2 triangles × 3 vertices = 6 vertices per face
- 6 faces × 6 vertices = **36 vertices total**

Also, each face needs its own normal direction for proper lighting!

### 2. World Space

After applying the **Model Matrix**, objects are positioned in the 3D world. The Model Matrix combines Translation, Rotation, and Scale.

**Example: Placing the bus body in Bus.h**
```cpp
// Main body placement
glm::mat4 model = glm::mat4(1.0f);  // Start with identity matrix
model = parent * glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));  // Move up
model = glm::scale(model, glm::vec3(10.0f, 3.0f, 3.0f));              // Stretch
cube.draw(shader, model, bodyColor);
```

**Numerical Example - Transforming a vertex:**
```
Original vertex in LOCAL space: (-0.5, -0.5, -0.5)

Step 1: Apply SCALE (10, 3, 3)
─────────────────────────────
   │ 10  0  0  0 │   │ -0.5 │   │ -5.0 │
   │  0  3  0  0 │ × │ -0.5 │ = │ -1.5 │
   │  0  0  3  0 │   │ -0.5 │   │ -1.5 │
   │  0  0  0  1 │   │  1.0 │   │  1.0 │
   
   Result: (-5.0, -1.5, -1.5)

Step 2: Apply TRANSLATION (0, 0.5, 0)
────────────────────────────────────
   │ 1  0  0  0   │   │ -5.0 │   │ -5.0 │
   │ 0  1  0  0.5 │ × │ -1.5 │ = │ -1.0 │  ← Added 0.5 to Y
   │ 0  0  1  0   │   │ -1.5 │   │ -1.5 │
   │ 0  0  0  1   │   │  1.0 │   │  1.0 │
   
   Final WORLD position: (-5.0, -1.0, -1.5)
```

**Before and after visualization:**
```
LOCAL SPACE (1×1×1 cube):     WORLD SPACE (10×3×3 bus body):
                              
    ┌───┐                          ┌─────────────────────┐
    │   │  1 unit                  │                     │  3 units
    └───┘                          │                     │  tall
    1 unit                         └─────────────────────┘
                                          10 units long
                                   
    Center at (0,0,0)              Center at (0, 0.5, 0)
    Bottom at Y = -0.5             Bottom at Y = -1.0
    Top at Y = +0.5                Top at Y = +2.0
```

### 3. View Space (Camera Space)

The world transformed so the camera is at origin, looking down -Z axis. Created by the **View Matrix**.

**From assignment.cpp - Camera setup:**
```cpp
// Camera position and orientation
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);  // X=0, Y=5 (above ground), Z=20 (behind bus)
float cameraPitch = -15.0f;   // Looking 15° downward
float cameraYaw = -90.0f;     // Facing toward -Z (toward the bus)

// The view matrix is created using glm::lookAt():
glm::mat4 view = glm::lookAt(
    cameraPos,              // Eye position: (0, 5, 20)
    cameraPos + front,      // Look-at target: (0, 5, 20) + front_vector
    glm::vec3(0, 1, 0)      // Up vector: Y is up
);
```

**What lookAt() does:**
```
WORLD SPACE:                    VIEW SPACE (after lookAt):
                                
     Camera                           Camera is NOW at origin
        ●──────────→                        ●
       /│  front                           /│
      / │                                 / │
     /  │                                /  │
    Bus ▼                              Bus ▼
    ┌───┐                              ┌───┐
                                       
    Camera at (0, 5, 20)              Camera at (0, 0, 0)
    Bus at origin                     Bus at (0, -5, -20) relative to camera
```

### 4. Clip Space and NDC (Normalized Device Coordinates)

After the **Projection Matrix** is applied, coordinates are transformed into a cube from (-1, -1, -1) to (1, 1, 1).

**From assignment.cpp - Perspective projection:**
```cpp
glm::mat4 projection = glm::perspective(
    glm::radians(45.0f),                              // FOV: 45 degrees vertical
    (float)SCR_WIDTH / (float)SCR_HEIGHT,             // Aspect: 1200/800 = 1.5
    0.1f,                                              // Near plane: 0.1 units
    100.0f                                             // Far plane: 100 units
);
```

**Perspective projection explained:**
```
                    Near Plane          Far Plane
                    (z = 0.1)          (z = 100)
                        │                  │
       Camera ●─────────┼──────────────────┼───────────→ Z
              │\        │                  │
              │ \       │  ╔═══════════╗   │
         FOV  │  \      │  ║  Visible  ║   │
         45°  │   \     │  ║   Volume  ║   │
              │    \    │  ║ (frustum) ║   │
              │     \   │  ╚═══════════╝   │
              │      \  │                  │
                      \ │                  │
                       \│                  │
                        
   Objects closer than 0.1 or farther than 100 are CLIPPED (not rendered)
   Objects inside frustum are projected onto 2D screen
```

**Field of View impact on view:**
```
FOV = 45°                    FOV = 90°
┌─────────────────┐          ┌─────────────────┐
│                 │          │                 │
│    ┌─────┐      │          │  ┌───────────┐  │
│    │ Bus │      │          │  │    Bus    │  │
│    └─────┘      │          │  └───────────┘  │
│   Normal view   │          │   Wide angle    │
└─────────────────┘          └─────────────────┘
                             (bus appears smaller, see more)
```

### 5. Screen Space (Final Pixels)

After clip space, OpenGL:
1. Divides by W component (perspective divide)
2. Maps to viewport: (-1,-1) → (0,0), (+1,+1) → (1200, 800)

```
Clip Space:                    Screen Space:
(-1, 1) ●─────────● (1, 1)     (0, 0) ●─────────● (1200, 0)
        │         │                   │         │
        │    ●    │    ────→         │    ●    │
        │  (0,0)  │    viewport      │ (600,   │
        │         │    transform     │   400)  │
(-1,-1) ●─────────● (1,-1)     (0, 800) ●─────────● (1200, 800)
```

---

## 📐 How Vertices Are Drawn: Step-by-Step

### Step 1: Define Vertex Data (Primitives.h)

Each vertex contains **6 floats**: 3 for position + 3 for normal = **24 bytes per vertex**.

```cpp
// Cube vertices layout - BACK FACE example (z = -0.5):
// Position (x, y, z)       Normal (nx, ny, nz)
// ─────────────────        ─────────────────
   -0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 0: bottom-left
    0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 1: bottom-right
    0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 2: top-right
    0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 3: top-right (repeated)
   -0.5f,  0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 4: top-left
   -0.5f, -0.5f, -0.5f,     0.0f,  0.0f, -1.0f,  // Vertex 5: bottom-left (repeated)
   
// These 6 vertices form 2 TRIANGLES:
// Triangle 1: vertices 0, 1, 2
// Triangle 2: vertices 3, 4, 5
```

**Why triangles? How the back face is drawn:**
```
    (-0.5, 0.5, -0.5)          (0.5, 0.5, -0.5)
           4●────────────────────●2,3
            │                  ╱│
            │    Triangle 2  ╱  │
            │              ╱    │
            │            ╱      │
            │          ╱        │
            │        ╱          │
            │      ╱  Triangle 1│
            │    ╱              │
            │  ╱                │
            │╱                  │
          0,5●────────────────────●1
    (-0.5, -0.5, -0.5)         (0.5, -0.5, -0.5)

    Triangle 1: (0 → 1 → 2) counter-clockwise
    Triangle 2: (3 → 4 → 5) counter-clockwise
    
    Normal for ALL vertices: (0, 0, -1) = pointing toward -Z
    This tells the shader "this face faces backward"
```

**Why normals matter - Lighting depends on surface direction:**
```
                Light Source ☀
                     │
                     │
                     ▼
    ┌────────────────────────────┐
    │  Normal: (0, 0, -1)        │  ← This face is BRIGHT
    │  Facing toward light       │     (normal · light_dir = big positive)
    └────────────────────────────┘
    
    ┌────────────────────────────┐
    │  Normal: (0, 0, +1)        │  ← This face is DARK  
    │  Facing away from light    │     (normal · light_dir = 0 or negative)
    └────────────────────────────┘
```

### Step 2: Upload to GPU (VBO and VAO)

```cpp
// Create OpenGL buffer objects
unsigned int VAO, VBO;
glGenVertexArrays(1, &VAO);  // VAO: stores the vertex attribute configuration
glGenBuffers(1, &VBO);       // VBO: stores the actual vertex data

// Bind VAO first - it will "record" all the following settings
glBindVertexArray(VAO);

// Upload vertex data to GPU memory
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
// sizeof(vertices) = 36 vertices × 6 floats × 4 bytes = 864 bytes

// Configure vertex attribute 0: POSITION (layout location = 0 in shader)
glVertexAttribPointer(
    0,                     // Attribute index (matches shader layout)
    3,                     // Number of components (x, y, z)
    GL_FLOAT,              // Data type
    GL_FALSE,              // Don't normalize
    6 * sizeof(float),     // Stride: 24 bytes to next vertex
    (void*)0               // Offset: position starts at byte 0
);
glEnableVertexAttribArray(0);

// Configure vertex attribute 1: NORMAL (layout location = 1 in shader)
glVertexAttribPointer(
    1,                           // Attribute index
    3,                           // Number of components (nx, ny, nz)
    GL_FLOAT,                    // Data type
    GL_FALSE,                    // Don't normalize
    6 * sizeof(float),           // Stride: 24 bytes
    (void*)(3 * sizeof(float))   // Offset: normal starts at byte 12
);
glEnableVertexAttribArray(1);

glBindVertexArray(0);  // Unbind VAO
```

**Memory layout visualization (GPU memory):**
```
VBO Memory (864 bytes total for cube):
┌─────────────────────────────────────────────────────────────────────────────┐
│                              VERTEX 0 (24 bytes)                            │
├───────────────────────────────────┬─────────────────────────────────────────┤
│ Position (12 bytes)               │ Normal (12 bytes)                       │
│ byte 0-3  │ byte 4-7 │ byte 8-11  │ byte 12-15 │ byte 16-19 │ byte 20-23   │
│   x=-0.5  │  y=-0.5  │   z=-0.5   │   nx=0.0   │   ny=0.0   │   nz=-1.0    │
├───────────┴──────────┴────────────┴────────────┴────────────┴──────────────┤
│                              VERTEX 1 (24 bytes)                            │
│   x=0.5   │  y=-0.5  │   z=-0.5   │   nx=0.0   │   ny=0.0   │   nz=-1.0    │
├─────────────────────────────────────────────────────────────────────────────┤
│                              ... 34 more vertices ...                       │
└─────────────────────────────────────────────────────────────────────────────┘

Stride = 24 bytes (distance from one vertex start to next vertex start)
Position offset = 0 bytes (position is at the beginning)
Normal offset = 12 bytes (normal starts after 3 floats)
```

### Step 3: Draw the Geometry

```cpp
void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
    shader.setMat4("model", model);        // Send 4×4 transformation matrix (64 bytes)
    shader.setVec3("objectColor", color);  // Send RGB color (12 bytes)
    glBindVertexArray(VAO);                // Activate this object's VAO
    glDrawArrays(GL_TRIANGLES, 0, 36);     // Draw 36 vertices as triangles
}

// GL_TRIANGLES means: every 3 consecutive vertices form one triangle
// 36 vertices ÷ 3 = 12 triangles (2 per face × 6 faces)
```

---

## 🎨 The Shader Pipeline

### Vertex Shader (shader.vert) - Detailed Breakdown

Runs **once per vertex** (36 times for a cube). Transforms vertex position and prepares data for fragment shader.

```glsl
#version 330 core    // OpenGL 3.3 core profile

// INPUT: Vertex attributes from VBO (per-vertex data)
layout (location = 0) in vec3 aPos;     // Position - matches glVertexAttribPointer(0, ...)
layout (location = 1) in vec3 aNormal;  // Normal - matches glVertexAttribPointer(1, ...)

// OUTPUT: Data to send to fragment shader (will be interpolated across triangle)
out vec3 FragPos;   // World-space position of this vertex
out vec3 Normal;    // World-space normal of this vertex

// UNIFORMS: Data sent from CPU (same value for ALL vertices in this draw call)
uniform mat4 model;       // Model matrix: local → world transformation
uniform mat4 view;        // View matrix: world → camera transformation  
uniform mat4 projection;  // Projection matrix: camera → clip transformation

void main() {
    // 1. Transform position from LOCAL space to WORLD space
    //    model * aPos multiplies the 4×4 matrix by the position vector
    FragPos = vec3(model * vec4(aPos, 1.0));  // 1.0 means "this is a point"
    
    // 2. Transform normal to WORLD space
    //    We use the "Normal Matrix" = transpose(inverse(model))
    //    This handles non-uniform scaling correctly
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    // 3. Transform position all the way to CLIP space
    //    This is the final position OpenGL uses
    gl_Position = projection * view * vec4(FragPos, 1.0);
    
    // Note: multiplication order is right-to-left:
    // FragPos is ALREADY in world space
    // view transforms to view space
    // projection transforms to clip space
}
```

**Numerical Example - Full vertex transformation:**
```
INPUT vertex (local space):
  aPos = (-0.5, -0.5, -0.5)
  
MODEL MATRIX (translate y+0.5, scale 10×3×3):
  ┌ 10   0   0   0 ┐
  │  0   3   0 1.5 │   (translation is combined into the matrix)
  │  0   0   3   0 │
  └  0   0   0   1 ┘

Step 1 - Apply model matrix:
  model × vec4(-0.5, -0.5, -0.5, 1.0) = vec4(-5.0, -1.0, -1.5, 1.0)
  FragPos = (-5.0, -1.0, -1.5)  ← WORLD space position

VIEW MATRIX (camera at (0, 5, 20), looking at origin):
  (simplified representation)

Step 2 - Apply view matrix:
  The vertex is now relative to camera position
  Approximately: (−5.0, −6.0, −21.5) in view space

PROJECTION MATRIX (45° FOV, aspect 1.5, near 0.1, far 100):

Step 3 - Apply projection matrix:
  gl_Position = some vec4 in clip space
  After perspective divide, coordinates are in range [-1, 1]
  
Step 4 - Viewport transform (OpenGL does this automatically):
  Maps [-1, 1] to [0, 1200] for X and [0, 800] for Y
  Final pixel position on screen!
```

### Fragment Shader (shader.frag) - Detailed Breakdown

Runs **once per pixel** (potentially millions of times per frame). Calculates final color using Phong lighting model.

```glsl
#version 330 core

// OUTPUT: The final color of this pixel
out vec4 FragColor;  // RGBA format (Red, Green, Blue, Alpha)

// INPUT: Interpolated data from vertex shader
in vec3 FragPos;     // World-space position (interpolated between 3 triangle vertices)
in vec3 Normal;      // World-space normal (interpolated)

// UNIFORMS: Data from CPU
uniform vec3 objectColor;  // Base color of object (e.g., bus body: 0.9, 0.9, 0.9)
uniform vec3 lightPos;     // Light position in world space
uniform vec3 viewPos;      // Camera position (for specular - not used here)

void main() {
    vec3 lightColor = vec3(1.0, 1.0, 1.0);  // White light (RGB = 1,1,1)
    
    // ═══════════════════════════════════════════════════════════════
    // AMBIENT LIGHTING: Simulates indirect light bouncing everywhere
    // ═══════════════════════════════════════════════════════════════
    float ambientStrength = 0.4;  // 40% base brightness
    vec3 ambient = ambientStrength * lightColor;  // = (0.4, 0.4, 0.4)
    
    // ═══════════════════════════════════════════════════════════════
    // DIFFUSE LIGHTING: Light that hits surface directly
    // ═══════════════════════════════════════════════════════════════
    vec3 norm = normalize(Normal);                    // Make unit length
    vec3 lightDir = normalize(lightPos - FragPos);   // Direction TO light
    float diff = max(dot(norm, lightDir), 0.0);      // Cosine of angle (clamped)
    vec3 diffuse = diff * lightColor;                // Scale by angle
    
    // ═══════════════════════════════════════════════════════════════
    // FINAL COLOR: Combine lighting with object color
    // ═══════════════════════════════════════════════════════════════
    vec3 result = (ambient + diffuse) * objectColor;
    FragColor = vec4(result, 1.0);  // Alpha = 1.0 (fully opaque)
}
```

**Numerical Lighting Example:**
```
Given:
  objectColor = (0.9, 0.9, 0.9)  // Light gray bus body
  lightPos = (10, 15, 10)         // Light above and to the side
  FragPos = (0, 1, 0)             // Point on bus surface
  Normal = (0, 1, 0)              // Surface facing UP

Step 1 - Calculate light direction:
  lightDir = normalize((10, 15, 10) - (0, 1, 0))
           = normalize((10, 14, 10))
           = (0.52, 0.73, 0.52)  // approximately

Step 2 - Calculate diffuse factor (dot product):
  diff = dot((0, 1, 0), (0.52, 0.73, 0.52))
       = 0×0.52 + 1×0.73 + 0×0.52
       = 0.73  // Surface receives 73% of max light

Step 3 - Calculate ambient:
  ambient = 0.4 × (1, 1, 1) = (0.4, 0.4, 0.4)

Step 4 - Calculate diffuse:
  diffuse = 0.73 × (1, 1, 1) = (0.73, 0.73, 0.73)

Step 5 - Combine with object color:
  result = (0.4 + 0.73) × (0.9, 0.9, 0.9)
         = 1.13 × (0.9, 0.9, 0.9)
         = (1.02, 1.02, 1.02)
         
         (clamped to (1.0, 1.0, 1.0) = pure white - very bright!)

For a surface facing AWAY from light (e.g., normal = (0, -1, 0)):
  diff = max(dot((0,-1,0), lightDir), 0) = max(-0.73, 0) = 0
  result = 0.4 × (0.9, 0.9, 0.9) = (0.36, 0.36, 0.36)  // Much darker!
```

**Lighting visualization:**
```
         Light Source ☀ at (10, 15, 10)
              │
              │ lightDir = direction toward light
              ▼
         ┌────●────┐
         │ Normal  │
         │    ↑    │  Surface at (0, 1, 0)
         └─────────┘
         
    Angle between Normal and lightDir:
    ────────────────────────────────────
    θ = 0°   → dot = 1.0 → Maximum brightness (surface directly faces light)
    θ = 45°  → dot = 0.7 → Good brightness
    θ = 90°  → dot = 0.0 → No diffuse (light grazes surface)
    θ > 90°  → dot < 0   → Clamped to 0 (surface faces away)
```

---

## 🚌 The Bus Model: Complete Numeric Details

### World Coordinate System in This Project

```
                    +Y (UP)
                     │
                     │
                     │    +Z (RIGHT side of bus)
                     │   /
                     │  /
                     │ /
                     │/
    ─────────────────●─────────────────→ +X (REAR of bus)
   -X (FRONT)       /│ Origin (0,0,0)
                   / │
                  /  │
                 /   │
                /    -Z (LEFT side of bus)
               /
    Ground plane at Y = -1 (where wheels touch)
```

### Bus Component Dimensions (Exact Values from Code)

| Component | Position (x, y, z) | Scale (x, y, z) | Final Bounds (World Space) |
|-----------|-------------------|-----------------|---------------------------|
| **Main Body** | (0, 0.5, 0) | (10, 3, 3) | X: -5 to +5, Y: -1 to +2, Z: -1.5 to +1.5 |
| **Roof** | (0, 2.15, 0) | (10.2, 0.3, 3.1) | X: -5.1 to +5.1, Y: 2.0 to 2.3, Z: -1.55 to +1.55 |
| **Floor** | (0, -0.9, 0) | (9.5, 0.1, 2.6) | Interior floor level |
| **Front Windshield** | (-5.01, 1.0, 0) | (0.05, 1.8, 2.5) | Covers front face |
| **Rear Window** | (5.01, 1.0, 0) | (0.05, 1.5, 2.2) | Covers rear face |

### Wheel Positions and Dimensions

```cpp
// From Bus.h - drawWheel() is called for each wheel:
drawWheel(-3.5f, -1.7f, true);   // Front Left  (steerable)
drawWheel(-3.5f,  1.7f, true);   // Front Right (steerable)  
drawWheel( 3.5f, -1.7f, false);  // Rear Left   (fixed)
drawWheel( 3.5f,  1.7f, false);  // Rear Right  (fixed)

// Wheel components:
// Tire:     scale (1.2, 0.35, 1.2) → diameter 1.2, width 0.35
// Rim:      scale (0.9, 0.05, 0.9) → diameter 0.9, thin disk
// Hub cap:  scale (0.3, 0.06, 0.3) → diameter 0.3, center cap
```

**Bus layout (top view with measurements):**
```
                           -X (FRONT)
                              ↓
     Z=-1.7 ←──────────────────────────────────→ Z=+1.7
            │                                   │
      ⊙ ────┼───────────────────────────────────┼──── ⊙   X=-3.5 (front wheels)
            │ ┌─────────────────────────────┐   │
            │ │  ●  Driver                  │   │
            │ │  ○ ○ ○ ○ ○ ○ ○ ○  Seats    │   │
            │ │  ○ ○ ○ ○ ○ ○ ○ ○           │   │
            │ └─────────────────────────────┘   │
      ⊙ ────┼───────────────────────────────────┼──── ⊙   X=+3.5 (rear wheels)
            │                                   │
            └───────────────────────────────────┘
            
     Wheel spacing: Front to rear = 7.0 units (from -3.5 to +3.5)
     Wheel width spacing: 3.4 units (from -1.7 to +1.7)
     Bus body width: 3.0 units
     Bus body length: 10.0 units
```

**Bus layout (side view with heights):**
```
    Y=2.3  ──┬─══════════════════════════════════════┬──  Roof top
             │▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓│
    Y=2.0  ──┼──────────────────────────────────────┼──  Roof bottom / Interior ceiling
             │          Interior Space              │
    Y=1.7  ──│  ═══════════════════════════════════ │──  Handrails
             │                                      │
    Y=1.2  ──│  ┌────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐ │──  Window tops
             │  │    │ │    │ │    │ │    │ │    │ │
    Y=0.7  ──│  └────┘ └────┘ └────┘ └────┘ └────┘ │──  Window bottoms
             │                                      │
    Y=0.0  ──│──────────────────────────────────────│──  Seat level
             │  ████████████████████████████████    │    (seats at Y=-0.5)
    Y=-0.9 ──│░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░│──  Floor
    Y=-1.0 ──○────────────────────────────────────○──  Ground / Wheel centers
             │                                      │
         X=-5.0                                 X=+5.0
         (FRONT)                                (REAR)
```

### Window Positions (From Code)

```cpp
// Left side windows (5 windows, evenly spaced):
for (int i = 0; i < 5; i++) {
    float xPos = -2.8f + i * 1.5f;  // X positions: -2.8, -1.3, 0.2, 1.7, 3.2
    // Position: (xPos, 1.2, -1.51)  // Z=-1.51 = left side (outside body)
    // Scale: (1.2, 1.0, 0.05)       // Width 1.2, Height 1.0, thin
}

// Window positions on left side:
// Window 0: X = -2.8 (near front)
// Window 1: X = -1.3
// Window 2: X =  0.2 (center)
// Window 3: X =  1.7
// Window 4: X =  3.2 (near rear)
```

### Seat Layout (From Code)

```cpp
// 8 rows of seats on each side = 16 seats total (32 passenger capacity)
float seatY = -0.5f;        // Seat height
float seatSpacing = 1.1f;   // Distance between rows
int numSeats = 8;           // Seats per side

for (int side = 0; side < 2; side++) {
    float zPos = (side == 0) ? -0.85f : 0.85f;  // Left: Z=-0.85, Right: Z=+0.85
    
    for (int i = 0; i < numSeats; i++) {
        float xPos = -3.2f + i * seatSpacing;
        // Seat positions: -3.2, -2.1, -1.0, 0.1, 1.2, 2.3, 3.4, 4.5
        
        // Each seat has:
        // - Cushion: scale (0.8, 0.25, 0.7)
        // - Frame: scale (0.75, 0.15, 0.65)
        // - Backrest: scale (0.75, 0.85, 0.12)
        // - Headrest: scale (0.4, 0.25, 0.15)
        // - Armrest: scale (0.7, 0.08, 0.1)
    }
}
```

### How Wheel Transformation Works (Step-by-Step)

```cpp
auto drawWheel = [&](float x, float z, bool isFront) {
    // 1. Get steering angle (only front wheels steer)
    float steer = isFront ? steeringAngle : 0.0f;  // steeringAngle: -35° to +35°
    
    // 2. Create base transformation: position wheel in world
    glm::mat4 wheelBase = parent;  // Start with bus transform
    wheelBase = glm::translate(wheelBase, glm::vec3(x, -1.0f, z));
    
    // 3. Apply steering rotation (Y-axis = turn left/right)
    wheelBase = glm::rotate(wheelBase, glm::radians(steer), glm::vec3(0, 1, 0));

    // 4. Draw the tire
    glm::mat4 model = wheelBase;
    
    // 5. Rotate 90° around X to lay the cylinder on its side
    //    Cylinder is created standing up (Y-axis), but wheel should spin around Z
    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1, 0, 0));
    
    // 6. Apply wheel rotation (spinning animation)
    //    Now Y-axis (after the 90° rotation) is the wheel's spin axis
    model = glm::rotate(model, glm::radians(wheelRotation), glm::vec3(0, 1, 0));
    
    // 7. Scale to wheel dimensions
    model = glm::scale(model, glm::vec3(1.2f, 0.35f, 1.2f));
    // X scale = 1.2 → diameter
    // Y scale = 0.35 → width (thickness)
    // Z scale = 1.2 → diameter (makes it circular, not elliptical)
    
    cylinder.draw(shader, model, tireColor);
};
```

**Transformation visualization:**
```
Step 1: Original cylinder        Step 2: Rotate 90° around X
(standing up)                    (laying on its side)
                                 
       │▓│                            ╔═══════╗
       │▓│                            ║▓▓▓▓▓▓▓║
       │▓│  Y-axis up                 ╚═══════╝  Now Z-axis is "up"
       │▓│
       │▓│                       
    ───●───                           ───●───
       
Step 3: Rotate around new Y      Step 4: Scale (1.2, 0.35, 1.2)
(wheel spinning)                 (flatten into disk shape)
                                 
    ╔═══════╗                         ╔═════════════════╗
    ║▓▓▓▓▓▓▓║  ← This rotates         ║▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓║  Final wheel!
    ╚═══════╝    for animation        ╚═════════════════╝
                                       1.2 units diameter
                                       0.35 units thick
```

---

## 📷 Camera System: Detailed Breakdown

### Camera State Variables (From assignment.cpp)

```cpp
// Camera position in world space
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
// Initial position: X=0 (centered), Y=5 (5 units above ground), Z=20 (20 units behind bus)

// Camera orientation (Euler angles in degrees)
float cameraPitch = -15.0f;   // Rotation around X-axis: -15° = looking slightly DOWN
float cameraYaw = -90.0f;     // Rotation around Y-axis: -90° = looking toward -Z
float cameraRoll = 0.0f;      // Rotation around Z-axis: 0° = no tilt

// Orbit mode (F key)
float orbitRadius = 20.0f;    // Distance from bus center when orbiting
float orbitHeight = 10.0f;    // Height above bus when orbiting
float orbitAngle = 0.0f;      // Current angle around bus (0° to 360°)
```

### Camera Direction Calculation (Spherical to Cartesian)

```cpp
glm::vec3 getCameraFront() {
    glm::vec3 front;
    // Convert spherical coordinates (yaw, pitch) to Cartesian (x, y, z)
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(front);
}
```

**Numerical example with initial values:**
```
yaw = -90°, pitch = -15°

front.x = cos(-90°) × cos(-15°) = 0 × 0.966 = 0.0
front.y = sin(-15°) = -0.259
front.z = sin(-90°) × cos(-15°) = -1 × 0.966 = -0.966

front = (0.0, -0.259, -0.966)

After normalize: front ≈ (0.0, -0.26, -0.97)
This means: looking toward -Z (forward) and slightly down
```

**Yaw and Pitch visualization:**
```
TOP VIEW (Yaw rotation):          SIDE VIEW (Pitch rotation):
                                  
        -Z (forward)                     Y (up)
           │                              │    
           │   yaw = 0°                   │   pitch = 0°
           │   (looking +X)               │   (looking horizontal)
           │                              │
   +X ─────●───── -X                 ──────●────── 
          /│                             /│\
         / │                            / │ \
        /  │                           /  │  \
    yaw = -90°                    pitch=-15°  pitch=+15°
    (looking -Z)                  (looking    (looking
                                    down)       up)
```

### View Matrix Construction

```cpp
glm::mat4 getViewMatrix() {
    glm::vec3 front = getCameraFront();
    glm::vec3 right = getCameraRight();  // Cross product of front and world up
    glm::vec3 up = getCameraUp();        // Cross product of right and front
    
    // Apply roll (tilt head left/right)
    if (cameraRoll != 0.0f) {
        glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRoll), front);
        up = glm::vec3(rollMat * glm::vec4(up, 0.0f));
    }
    
    // Create view matrix
    return glm::lookAt(cameraPos, cameraPos + front, up);
}
```

**lookAt() creates a matrix that transforms world coordinates to camera space:**
```
Parameters:
  eye    = cameraPos           = (0, 5, 20)
  center = cameraPos + front   = (0, 5, 20) + (0, -0.26, -0.97) = (0, 4.74, 19.03)
  up     = (0, 1, 0)

The resulting view matrix positions the camera at (0, 5, 20) looking toward (0, 4.74, 19.03)
```

### Preset Camera Positions

| Key | Position | Pitch | Yaw | Description |
|-----|----------|-------|-----|-------------|
| 0 | (0, 5, 20) | -15° | -90° | Default outside view |
| 2 | (-3, 0.5, 0) | 0° | 90° | Inside bus (passenger view) |
| 9 | (-3.5, 0.5, -0.6) | -5° | -90° | Driver seat |

---

## 🎮 Driving Physics System

### Physics Constants (From assignment.cpp)

```cpp
const float ACCELERATION = 15.0f;   // Units per second² when pressing W
const float DECELERATION = 10.0f;   // Units per second² friction/braking
const float MAX_SPEED = 20.0f;      // Maximum forward speed (units/second)
const float STEER_SPEED = 60.0f;    // Degrees per second steering rate
const float MAX_STEER = 35.0f;      // Maximum steering angle (±35°)
```

### Physics Update Loop (Simplified)

```cpp
// In processInput(), when isDrivingMode == true:

// 1. STEERING (A/D keys)
if (keyA_pressed) busSteerAngle += STEER_SPEED * deltaTime;  // Turn left
if (keyD_pressed) busSteerAngle -= STEER_SPEED * deltaTime;  // Turn right
busSteerAngle = clamp(busSteerAngle, -MAX_STEER, MAX_STEER); // Limit to ±35°

// 2. ACCELERATION (W/S keys)
if (keyW_pressed) busSpeed += ACCELERATION * deltaTime;      // Speed up
if (keyS_pressed) busSpeed -= ACCELERATION * deltaTime;      // Slow down/reverse
busSpeed = clamp(busSpeed, -MAX_SPEED/2, MAX_SPEED);         // Limit speed

// 3. FRICTION (when no keys pressed)
if (busSpeed > 0) busSpeed -= DECELERATION * deltaTime;      // Slow down naturally
if (busSpeed < 0) busSpeed += DECELERATION * deltaTime;

// 4. TURNING (only when moving)
if (abs(busSpeed) > 0.1f) {
    float turnFactor = busSpeed * deltaTime * 0.05f;
    busYaw += busSteerAngle * turnFactor;  // Turn more when going faster
}

// 5. MOVEMENT (forward direction based on yaw)
float rad = glm::radians(busYaw);
glm::vec3 forwardDir = glm::vec3(-cos(rad), 0.0f, -sin(rad));  // Bus faces -X
busPosition += forwardDir * busSpeed * deltaTime;

// 6. UPDATE WHEEL ROTATION
bus.updateWheels(busSpeed * deltaTime);  // Rotate wheels based on distance traveled
```

**Numerical example - One frame of driving:**
```
Starting state:
  busPosition = (0, 0, 0)
  busYaw = 0°
  busSpeed = 10 units/second
  busSteerAngle = 20° (turning left)
  deltaTime = 0.016 seconds (60 FPS)

Calculations:
  forwardDir = (-cos(0°), 0, -sin(0°)) = (-1, 0, 0)
  
  Movement this frame:
    distance = 10 × 0.016 = 0.16 units
    busPosition += (-1, 0, 0) × 0.16 = (-0.16, 0, 0)
  
  Turn this frame:
    turnFactor = 10 × 0.016 × 0.05 = 0.008
    yawChange = 20° × 0.008 = 0.16°
    busYaw = 0° + 0.16° = 0.16°
  
  Wheel rotation:
    rotation += 0.16 × 100 = 16° of wheel spin

After this frame:
  busPosition = (-0.16, 0, 0)
  busYaw = 0.16° (slightly turned left)
  wheelRotation increased by 16°
```

### Chase Camera in Driving Mode

```cpp
// Camera follows behind the bus
float camDist = 25.0f;   // 25 units behind
float camHeight = 8.0f;  // 8 units above ground

// Calculate "behind" direction (opposite of forward)
float rad = glm::radians(busYaw);
glm::vec3 forwardDir = glm::vec3(-cos(rad), 0, -sin(rad));
glm::vec3 behindDir = -forwardDir;

// Position camera behind and above bus
cameraPos = busPosition + behindDir * camDist;
cameraPos.y = busPosition.y + camHeight;

// Look at the bus (slightly above ground)
cameraLookAt = busPosition + glm::vec3(0, 1.5f, 0);
```

---

## 🔧 Primitives Generation: Cylinder and Torus

### Cylinder Generation (Parametric)

The cylinder is created by generating vertices around circles at the top and bottom.

```cpp
void Cylinder::init(int sectors = 36) {
    float radius = 0.5f;
    float height = 1.0f;
    float halfHeight = 0.5f;
    float sectorStep = 2.0f * M_PI / sectors;  // 360° / 36 = 10° per sector
    
    // SIDE SURFACE: Create rectangles around the cylinder
    for (int i = 0; i < sectors; i++) {
        float angle1 = i * sectorStep;           // Current angle
        float angle2 = (i + 1) * sectorStep;     // Next angle
        
        // Two points on the bottom circle
        float x1 = radius * cos(angle1);  // e.g., 0.5 × cos(0°) = 0.5
        float z1 = radius * sin(angle1);  // e.g., 0.5 × sin(0°) = 0.0
        float x2 = radius * cos(angle2);  // e.g., 0.5 × cos(10°) = 0.49
        float z2 = radius * sin(angle2);  // e.g., 0.5 × sin(10°) = 0.087
        
        // Create 2 triangles (a quad) connecting top and bottom
        // Triangle 1: bottom1, bottom2, top1
        // Triangle 2: bottom2, top2, top1
    }
    
    // TOP CAP: Fan of triangles from center to edge
    // BOTTOM CAP: Same, but normals point down
}
```

**Cylinder vertex layout:**
```
                    TOP CAP (y = 0.5)
                    ↙ triangles fan from center
        ●───────────●
       /│          /│
      / │         / │
     /  │        /  │
    ●   │       ●   │   ← SIDE SURFACE
    │   ●───────│───●      (2 triangles per sector × 36 sectors = 72 triangles)
    │  /        │  /
    │ /         │ /
    │/          │/
    ●───────────●
        ↘ BOTTOM CAP (y = -0.5)
          triangles fan from center
          
Total triangles: 72 (sides) + 36 (top) + 36 (bottom) = 144 triangles
Total vertices: 144 × 3 = 432 vertices
```

### Torus Generation (For Steering Wheel)

A torus is a donut shape defined by two radii: main radius (circle center to tube center) and tube radius.

```cpp
void Torus::init(float mainRadius = 0.4f, float tubeRadius = 0.1f, 
                 int mainSegments = 24, int tubeSegments = 12) {
    // mainRadius: distance from center to tube center = 0.4
    // tubeRadius: radius of the tube = 0.1
    // Total outer diameter: (0.4 + 0.1) × 2 = 1.0
    // Total inner diameter: (0.4 - 0.1) × 2 = 0.6
    
    for (int i = 0; i < mainSegments; i++) {       // 24 segments around donut
        for (int j = 0; j < tubeSegments; j++) {   // 12 segments around tube
            // Calculate position on torus surface using two angles
            float theta = (float)i / mainSegments * 2 * PI;  // Main circle angle
            float phi = (float)j / tubeSegments * 2 * PI;    // Tube circle angle
            
            // Parametric equations for torus:
            float x = (mainRadius + tubeRadius * cos(phi)) * cos(theta);
            float y = tubeRadius * sin(phi);
            float z = (mainRadius + tubeRadius * cos(phi)) * sin(theta);
        }
    }
}
```

**Torus cross-section:**
```
         TOP VIEW                    SIDE VIEW (cross-section)
         
      ╭───────────╮                       tubeRadius
     ╱             ╲                          ↕
    │   ╭─────╮    │                    ╭─────────╮
    │  ╱       ╲   │                    │         │
    │ │  HOLE   │  │     ←────→        │  ●      │ ← tube center
    │  ╲       ╱   │     mainRadius     │         │
    │   ╰─────╯    │                    ╰─────────╯
     ╲             ╱
      ╰───────────╯
      
    mainRadius = 0.4 (center of hole to center of tube)
    tubeRadius = 0.1 (thickness of the donut)
```

---

## 🔧 Technical Requirements

### Libraries Used

| Library | Purpose | Key Functions Used |
|---------|---------|-------------------|
| **GLFW** | Window, input, OpenGL context | `glfwCreateWindow`, `glfwPollEvents`, `glfwGetKey` |
| **GLAD** | OpenGL function loader | `gladLoadGLLoader` |
| **GLM** | Math (vectors, matrices) | `glm::mat4`, `glm::translate`, `glm::rotate`, `glm::scale`, `glm::perspective`, `glm::lookAt` |

### OpenGL Functions Used

| Function | Purpose |
|----------|---------|
| `glGenVertexArrays` | Create VAO |
| `glGenBuffers` | Create VBO |
| `glBindVertexArray` | Activate VAO |
| `glBindBuffer` | Activate buffer |
| `glBufferData` | Upload data to GPU |
| `glVertexAttribPointer` | Define vertex layout |
| `glEnableVertexAttribArray` | Enable vertex attribute |
| `glDrawArrays` | Draw primitives |
| `glClear` | Clear screen |
| `glEnable(GL_DEPTH_TEST)` | Enable depth testing |

---

## 🎮 Complete Controls Reference

### Flying Camera Mode (Default)
| Key | Action | Details |
|-----|--------|---------|
| W | Move forward | Along camera front direction |
| S | Move backward | Opposite of front |
| A | Strafe left | Perpendicular to front |
| D | Strafe right | Perpendicular to front |
| E | Move up | Along Y axis |
| R | Move down | Along Y axis |
| X | Pitch up/down | Shift = opposite direction |
| Y | Yaw left/right | Shift = opposite direction |
| Z | Roll tilt | Shift = opposite direction |
| F | Orbit bus | Hold to orbit around bus center |
| 0 | Outside view | Reset to default position |
| 2 | Enter bus | Passenger interior view |
| 9 | Driver seat | First-person driver view |

### Driving Mode (Press K to toggle)
| Key | Action | Details |
|-----|--------|---------|
| K | Toggle mode | Switch between fly/drive |
| W | Accelerate | Max 20 units/second |
| S | Brake/Reverse | Max -10 units/second reverse |
| A | Steer left | Up to 35° |
| D | Steer right | Up to 35° |

### Bus Interactions
| Key | Action | Details |
|-----|--------|---------|
| 1 | Toggle door | Opens to 90°, closes to 0° |
| 3-8 | Toggle windows | 6 windows, each toggles open/closed |
| G | Toggle fan | Ceiling fans spin/stop |
| L | Toggle lights | Interior lights on/off |

---

## 🧮 GLM Data Types: Complete Guide

GLM (OpenGL Mathematics) is a header-only C++ library that provides math types and functions matching GLSL (shader language). Let's understand each type in depth.

---

### 📦 glm::vec2, glm::vec3, glm::vec4 - Vector Types

Vectors are ordered collections of numbers. They represent **points**, **directions**, **colors**, or any grouped data.

#### glm::vec2 - 2D Vector (2 components)
```cpp
glm::vec2 v = glm::vec2(3.0f, 4.0f);  // x=3, y=4

// Memory layout (8 bytes):
// ┌───────────┬───────────┐
// │  x = 3.0  │  y = 4.0  │
// │ (4 bytes) │ (4 bytes) │
// └───────────┴───────────┘

// Access components:
v.x  // 3.0
v.y  // 4.0
v[0] // 3.0 (same as v.x)
v[1] // 4.0 (same as v.y)

// Use cases:
// - 2D positions: glm::vec2(screenX, screenY)
// - Texture coordinates: glm::vec2(u, v)
```

#### glm::vec3 - 3D Vector (3 components) ⭐ Most Common
```cpp
glm::vec3 position = glm::vec3(1.0f, 2.0f, 3.0f);  // x=1, y=2, z=3
glm::vec3 color = glm::vec3(0.9f, 0.1f, 0.1f);     // R=0.9, G=0.1, B=0.1 (red)

// Memory layout (12 bytes):
// ┌───────────┬───────────┬───────────┐
// │  x = 1.0  │  y = 2.0  │  z = 3.0  │
// │ (4 bytes) │ (4 bytes) │ (4 bytes) │
// └───────────┴───────────┴───────────┘

// Access components (multiple ways - all equivalent):
position.x, position.y, position.z   // Position naming
position.r, position.g, position.b   // Color naming (same memory!)
position.s, position.t, position.p   // Texture naming (same memory!)
position[0], position[1], position[2] // Array indexing

// VISUALIZATION - 3D Point:
//
//        Y
//        │   ● position (1, 2, 3)
//        │  /│
//        │ / │ z=3
//        │/  │
//        ●───┼─────── X
//       /    │
//      /     │ y=2
//     Z      │
//            └── x=1

// Common vec3 operations:
glm::vec3 a(1, 0, 0);
glm::vec3 b(0, 1, 0);

a + b           // (1, 1, 0) - component-wise addition
a - b           // (1, -1, 0) - subtraction
a * 2.0f        // (2, 0, 0) - scalar multiplication
a * b           // (0, 0, 0) - component-wise multiplication (NOT dot product!)

glm::dot(a, b)   // 0.0 - dot product (perpendicular = 0)
glm::cross(a, b) // (0, 0, 1) - cross product (perpendicular vector)
glm::length(a)   // 1.0 - magnitude/length
glm::normalize(a) // (1, 0, 0) - unit vector (length = 1)
```

**Use Cases for vec3:**
| Purpose | Example | Values |
|---------|---------|--------|
| 3D Position | `glm::vec3(0, 5, 20)` | Camera at X=0, Y=5, Z=20 |
| Direction | `glm::vec3(0, 1, 0)` | Pointing UP |
| RGB Color | `glm::vec3(0.9, 0.9, 0.9)` | Light gray |
| Scale factors | `glm::vec3(10, 3, 3)` | Scale 10× in X, 3× in Y, 3× in Z |
| Surface Normal | `glm::vec3(0, 0, -1)` | Face pointing toward -Z |

#### glm::vec4 - 4D Vector (4 components)
```cpp
glm::vec4 pos = glm::vec4(1.0f, 2.0f, 3.0f, 1.0f);   // Homogeneous position
glm::vec4 color = glm::vec4(1.0f, 0.0f, 0.0f, 0.5f); // RGBA: red with 50% transparency

// Memory layout (16 bytes):
// ┌───────────┬───────────┬───────────┬───────────┐
// │  x = 1.0  │  y = 2.0  │  z = 3.0  │  w = 1.0  │
// │ (4 bytes) │ (4 bytes) │ (4 bytes) │ (4 bytes) │
// └───────────┴───────────┴───────────┴───────────┘

// Access components:
pos.x, pos.y, pos.z, pos.w  // Position
pos.r, pos.g, pos.b, pos.a  // Color (a = alpha/transparency)

// THE W COMPONENT IS CRUCIAL FOR MATRIX MATH:
// w = 1.0 → This is a POINT (affected by translation)
// w = 0.0 → This is a DIRECTION (NOT affected by translation)

// Converting between vec3 and vec4:
glm::vec3 v3(1, 2, 3);
glm::vec4 v4 = glm::vec4(v3, 1.0f);  // Add w=1 for point
glm::vec3 back = glm::vec3(v4);       // Drop w component
```

---

### 📊 glm::mat4 - 4×4 Matrix (16 floats)

A **mat4** is a 4×4 grid of numbers used to transform 3D points. It can encode **translation**, **rotation**, **scale**, and **perspective** all in one!

#### Memory Layout
```cpp
glm::mat4 M = glm::mat4(1.0f);  // Identity matrix

// A mat4 contains 16 floats (64 bytes) arranged in COLUMNS (column-major order):
//
// Mathematical view:          Memory layout (how GLM stores it):
//                             
// ┌ m[0][0]  m[1][0]  m[2][0]  m[3][0] ┐    Column 0: m[0][0], m[0][1], m[0][2], m[0][3]
// │ m[0][1]  m[1][1]  m[2][1]  m[3][1] │    Column 1: m[1][0], m[1][1], m[1][2], m[1][3]
// │ m[0][2]  m[1][2]  m[2][2]  m[3][2] │    Column 2: m[2][0], m[2][1], m[2][2], m[2][3]
// └ m[0][3]  m[1][3]  m[2][3]  m[3][3] ┘    Column 3: m[3][0], m[3][1], m[3][2], m[3][3]
//   ↑         ↑         ↑         ↑
//   Column0   Column1   Column2   Column3

// Access elements:
M[0]      // Column 0 as vec4
M[0][0]   // Element at column 0, row 0
M[3][0]   // Translation X (column 3, row 0)
M[3][1]   // Translation Y (column 3, row 1)
M[3][2]   // Translation Z (column 3, row 2)
```

#### Identity Matrix - "Do Nothing" Transformation
```cpp
glm::mat4 I = glm::mat4(1.0f);

// Creates:
// ┌ 1  0  0  0 ┐
// │ 0  1  0  0 │
// │ 0  0  1  0 │
// └ 0  0  0  1 ┘
//
// When you multiply any point by this matrix, it stays the same:
// I × (x, y, z, 1) = (x, y, z, 1)

// VISUALIZATION:
// Before: ● (2, 3, 0)      After: ● (2, 3, 0)  (unchanged!)
```

#### Why 4×4 for 3D? (Homogeneous Coordinates)

We use 4×4 matrices (not 3×3) because:
1. **Translation requires addition**, but matrices do multiplication
2. By adding a 4th coordinate (w), we can encode translation as multiplication!

```cpp
// 3×3 matrix CANNOT do translation (only rotation/scale)
// 4×4 matrix CAN do translation by putting values in the 4th column

// Point (x, y, z) becomes (x, y, z, 1) for transformation
// Direction (x, y, z) becomes (x, y, z, 0) - won't be translated!
```

---

### 🔄 glm::translate() - Translation Matrix

Creates a matrix that **moves** points by (tx, ty, tz).

```cpp
glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 2.0f, -3.0f));

// This creates the matrix:
// ┌ 1  0  0  5 ┐
// │ 0  1  0  2 │   ← Translation values in the rightmost column
// │ 0  0  1 -3 │
// └ 0  0  0  1 ┘

// How it transforms a point (1, 1, 1):
//
// ┌ 1  0  0  5 ┐   ┌ 1 ┐   ┌ 1×1 + 0×1 + 0×1 + 5×1 ┐   ┌ 6 ┐
// │ 0  1  0  2 │ × │ 1 │ = │ 0×1 + 1×1 + 0×1 + 2×1 │ = │ 3 │
// │ 0  0  1 -3 │   │ 1 │   │ 0×1 + 0×1 + 1×1 - 3×1 │   │-2 │
// └ 0  0  0  1 ┘   │ 1 │   │ 0×1 + 0×1 + 0×1 + 1×1 │   │ 1 │
//                   ↑w=1                               Result: (6, 3, -2)

// VISUALIZATION:
//
//     Y
//     │     ● (6, 3, -2) AFTER
//     │    ↗
//     │   /  translate by (5, 2, -3)
//     │  /
//     │ ● (1, 1, 1) BEFORE
//     │/
//     └──────────── X
//    /
//   Z
```

**Function signature:**
```cpp
glm::mat4 glm::translate(
    glm::mat4 matrix,    // Matrix to apply translation to (usually identity)
    glm::vec3 offset     // How much to move (tx, ty, tz)
);
// Returns: A new matrix with translation applied
```

---

### 📐 glm::scale() - Scale Matrix

Creates a matrix that **stretches or shrinks** objects.

```cpp
glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 3.0f, 0.5f));

// This creates the matrix:
// ┌ 2  0  0   0 ┐
// │ 0  3  0   0 │   ← Scale values on the diagonal
// │ 0  0  0.5 0 │
// └ 0  0  0   1 ┘

// How it transforms a point (1, 1, 1):
//
// ┌ 2  0  0   0 ┐   ┌ 1 ┐   ┌ 2×1 ┐   ┌ 2   ┐
// │ 0  3  0   0 │ × │ 1 │ = │ 3×1 │ = │ 3   │
// │ 0  0  0.5 0 │   │ 1 │   │ 0.5×1│   │ 0.5 │
// └ 0  0  0   1 ┘   │ 1 │   │ 1   │   │ 1   │
//
// Result: (2, 3, 0.5) - stretched in X and Y, compressed in Z

// VISUALIZATION:
//
//     Y                              Y
//     │  ┌───┐                       │  ┌───────┐
//     │  │   │  1×1×1 cube          │  │       │  2×3×0.5 box
//     │  └───┘                       │  │       │
//     └────── X                      │  └───────┘
//                                    └────────────── X
//     
//   BEFORE (unit cube)             AFTER (scaled box)
```

**Function signature:**
```cpp
glm::mat4 glm::scale(
    glm::mat4 matrix,    // Matrix to apply scale to
    glm::vec3 factors    // Scale factors (sx, sy, sz)
);

// Special values:
// scale(1, 1, 1) = no change
// scale(2, 2, 2) = double size uniformly
// scale(1, 2, 1) = stretch only in Y direction
// scale(-1, 1, 1) = mirror/flip in X direction
```

---

### 🔁 glm::rotate() - Rotation Matrix

Creates a matrix that **rotates** objects around an axis.

```cpp
glm::mat4 R = glm::rotate(
    glm::mat4(1.0f),           // Start with identity
    glm::radians(45.0f),       // Angle: 45 degrees (must convert to radians!)
    glm::vec3(0.0f, 1.0f, 0.0f) // Axis: rotate around Y axis
);

// Rotation around Y axis (looking down from above):
// ┌ cos(θ)  0  sin(θ)  0 ┐
// │   0     1    0     0 │   For θ = 45°:
// │-sin(θ)  0  cos(θ)  0 │   cos(45°) ≈ 0.707
// └   0     0    0     1 ┘   sin(45°) ≈ 0.707

// VISUALIZATION (top view, rotating around Y):
//
//          Z                           Z
//          │                           │    ● AFTER (rotated 45°)
//          │                           │   /
//          │                           │  /
//          │● BEFORE (1, 0, 0)         │ /  45°
//          │                           │/
//    ──────┴────── X             ──────┴────── X
//
// Point (1, 0, 0) rotates to approximately (0.707, 0, 0.707)
```

**Common rotation axes:**
```cpp
glm::vec3(1, 0, 0)  // X-axis: Pitch (tilt forward/backward)
glm::vec3(0, 1, 0)  // Y-axis: Yaw (turn left/right)
glm::vec3(0, 0, 1)  // Z-axis: Roll (tilt sideways)

// VISUALIZATION of rotation axes:
//
//        Y (yaw)            Pitch around X:    Roll around Z:
//        │                        ↺              
//        │                    ┌───────┐           /\
//        │                    │       │          /  \
//   ─────┼───── X (pitch)     │   ●   │    ●────/────\────●
//       /                     │       │          \  /
//      /                      └───────┘           \/
//     Z (roll)              (nodding head)    (tilting head)
```

**Function signature:**
```cpp
glm::mat4 glm::rotate(
    glm::mat4 matrix,    // Matrix to apply rotation to
    float angle,         // Angle in RADIANS (use glm::radians() to convert!)
    glm::vec3 axis       // Axis to rotate around (should be normalized)
);

// IMPORTANT: Always use glm::radians() for angles!
glm::rotate(mat, 45.0f, axis);              // WRONG! 45 radians is huge!
glm::rotate(mat, glm::radians(45.0f), axis); // CORRECT! 45 degrees
```

---

### 🎥 glm::perspective() - Perspective Projection Matrix

Creates a matrix that simulates how a camera sees the world (farther objects appear smaller).

```cpp
glm::mat4 P = glm::perspective(
    glm::radians(45.0f),  // Field of View: 45 degrees (vertical)
    1.5f,                  // Aspect Ratio: width/height = 1200/800
    0.1f,                  // Near plane: closest visible distance
    100.0f                 // Far plane: farthest visible distance
);

// This creates a perspective projection matrix that:
// 1. Objects farther away appear smaller (perspective divide)
// 2. Only objects between near (0.1) and far (100) planes are visible
// 3. Maps the 3D frustum to a 2D screen

// VISUALIZATION (side view):
//
//                    Near Plane       Far Plane
//                    (z = 0.1)       (z = 100)
//                        │               │
//       Camera ●─────────┼───────────────┼─────────────────→ Z
//              │\        │               │
//              │ \       │╔═════════════╗│
//         FOV  │  \      │║   VISIBLE   ║│
//         45°  │   \     │║   FRUSTUM   ║│
//              │    \    │║  (pyramid)  ║│
//              │     \   │╚═════════════╝│
//              │      \  │               │
//                      \ │               │
//                       \│               │
//
// Only objects inside this frustum (truncated pyramid) are rendered!
```

**Field of View impact:**
```
FOV = 30° (narrow/zoom)    FOV = 45° (normal)      FOV = 90° (wide angle)
┌─────────────────┐        ┌─────────────────┐     ┌─────────────────┐
│                 │        │                 │     │                 │
│    ┌─────┐      │        │   ┌───────┐     │     │ ┌───────────┐   │
│    │ BIG │      │        │   │ Normal│     │     │ │   Small   │   │
│    └─────┘      │        │   └───────┘     │     │ └───────────┘   │
│   Zoomed in     │        │   Standard      │     │   Wide view     │
└─────────────────┘        └─────────────────┘     └─────────────────┘
```

---

### 👁️ glm::lookAt() - View Matrix

Creates a matrix that positions and orients the "camera" in the scene.

```cpp
glm::mat4 V = glm::lookAt(
    glm::vec3(0.0f, 5.0f, 20.0f),   // Eye: Camera position
    glm::vec3(0.0f, 0.0f, 0.0f),    // Center: What to look at
    glm::vec3(0.0f, 1.0f, 0.0f)     // Up: Which direction is "up"
);

// VISUALIZATION:
//
//                Y (up vector)
//                │
//                │      ● Camera at (0, 5, 20)
//                │     /│
//                │    / │
//                │   /  │
//                │  /   │
//                │ /    │ Looking at
//                │/     ▼
//        ────────●──────────── X
//     (0,0,0)   /│ Target
//              / │
//             /  │
//            Z   │
//                │
//          Camera is at Z=20, Y=5, looking at origin

// The view matrix transforms WORLD coordinates to CAMERA coordinates:
// - Camera becomes the origin (0, 0, 0)
// - Camera's forward direction becomes -Z
// - Camera's up direction becomes +Y
// - Camera's right direction becomes +X
```

**Function signature:**
```cpp
glm::mat4 glm::lookAt(
    glm::vec3 eye,       // Camera position in world space
    glm::vec3 center,    // Point to look at
    glm::vec3 up         // Up direction (usually (0, 1, 0))
);
```

---

### 🔗 Combining Transformations (Matrix Multiplication)

Matrices can be **multiplied together** to combine multiple transformations into one!

```cpp
glm::mat4 model = glm::mat4(1.0f);  // Start with identity

// Apply transformations (ORDER MATTERS!)
model = glm::translate(model, glm::vec3(0.0f, 0.5f, 0.0f));  // Step 3: Move up
model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0, 1, 0));  // Step 2: Rotate
model = glm::scale(model, glm::vec3(10.0f, 3.0f, 3.0f));     // Step 1: Scale

// This is equivalent to:
// model = Translation × Rotation × Scale
//
// When applied to a vertex:
// result = model × vertex
//        = T × R × S × vertex
//        = T × (R × (S × vertex))
//
// READ RIGHT-TO-LEFT: Scale first, then Rotate, then Translate!
```

**Why order matters (visual example):**
```
TRANSLATE then ROTATE:              ROTATE then TRANSLATE:
                                    
1. Start at origin                  1. Start at origin
   ●                                   ●
   
2. Translate right by 5             2. Rotate 45°
      ───────→●                        ● (still at origin)
              (5,0)                    
                                    
3. Rotate 45° around origin         3. Translate right by 5
              ╱                           ───────→●
             ╱                                    (5, 0)
            ●  (3.5, 3.5)           
           ↙                        
          (rotates around ORIGIN)   (translates in ORIGINAL direction)
          
DIFFERENT FINAL POSITIONS!
```

**Standard transformation order (TRS):**
```cpp
// Model Matrix = Translation × Rotation × Scale
// This gives intuitive behavior:
// 1. Scale the object to its size (at origin)
// 2. Rotate the object (around its own center)
// 3. Move the object to its position in the world

glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, position);    // 3. Final position
model = glm::rotate(model, angle, axis);    // 2. Orientation
model = glm::scale(model, size);            // 1. Size
```

---

### 📊 Complete Matrix Pipeline Visualization

```
VERTEX DATA                MODEL MATRIX              VIEW MATRIX             PROJECTION MATRIX
(Local Space)              (World Transform)         (Camera Transform)      (3D → 2D)
                           
    ┌─────┐                    ┌─────┐                  ┌─────┐                ┌─────┐
    │-0.5 │                    │-5.0 │                  │-5.0 │                │ 0.3 │
    │-0.5 │    scale(10,3,3)   │-1.0 │    lookAt()      │-6.0 │   perspective()│ 0.4 │
    │-0.5 │  ───────────────→  │-1.5 │  ──────────→    │-21.5│  ────────────→ │ 0.9 │
    │ 1.0 │  translate(0,.5,0) │ 1.0 │                  │ 1.0 │                │21.5 │
    └─────┘                    └─────┘                  └─────┘                └─────┘
    
    Unit cube                  Bus body corner          Relative to camera     Clip coordinates
    vertex                     in world                 (camera at origin)     (ready for screen)
    
    
    gl_Position = projection × view × model × vertex
                  └────────────────────────────────────────────────────────┘
                            All combined into one operation!
```

---

### 🧠 Quick Reference Table

| Type/Function | Size | Purpose | Example |
|---------------|------|---------|---------|
| `glm::vec2` | 8 bytes | 2D point/direction | `vec2(1.0, 0.5)` - texture coord |
| `glm::vec3` | 12 bytes | 3D point/direction/color | `vec3(0, 5, 20)` - camera position |
| `glm::vec4` | 16 bytes | Homogeneous point/RGBA | `vec4(pos, 1.0)` - for matrices |
| `glm::mat4` | 64 bytes | 4×4 transformation matrix | Model, View, Projection |
| `glm::translate()` | - | Create translation matrix | Move object to position |
| `glm::rotate()` | - | Create rotation matrix | Orient object |
| `glm::scale()` | - | Create scale matrix | Size object |
| `glm::perspective()` | - | Create projection matrix | 3D to 2D conversion |
| `glm::lookAt()` | - | Create view matrix | Position camera |
| `glm::radians()` | - | Degrees to radians | `radians(45.0f)` = 0.785 |
| `glm::normalize()` | - | Make vector length = 1 | Unit direction vector |
| `glm::dot()` | - | Dot product | Angle between vectors |
| `glm::cross()` | - | Cross product | Perpendicular vector |

---

## 🧮 Matrix Math Cheat Sheet

### Identity Matrix
```cpp
glm::mat4 I = glm::mat4(1.0f);
// Represents "no transformation"
// ┌ 1 0 0 0 ┐
// │ 0 1 0 0 │
// │ 0 0 1 0 │
// └ 0 0 0 1 ┘
```

### Translation Matrix
```cpp
glm::mat4 T = glm::translate(glm::mat4(1.0f), glm::vec3(tx, ty, tz));
// ┌ 1 0 0 tx ┐
// │ 0 1 0 ty │   Moves point (x,y,z) to (x+tx, y+ty, z+tz)
// │ 0 0 1 tz │
// └ 0 0 0 1  ┘
```

### Scale Matrix
```cpp
glm::mat4 S = glm::scale(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
// ┌ sx 0  0  0 ┐
// │ 0  sy 0  0 │   Scales point (x,y,z) to (x×sx, y×sy, z×sz)
// │ 0  0  sz 0 │
// └ 0  0  0  1 ┘
```

### Rotation Matrix (around Y axis)
```cpp
glm::mat4 R = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0,1,0));
// ┌ cos(θ)  0  sin(θ)  0 ┐
// │   0     1    0     0 │   Rotates around Y axis by θ degrees
// │-sin(θ)  0  cos(θ)  0 │
// └   0     0    0     1 ┘
```

### Combining Transformations
```cpp
// IMPORTANT: Order matters! Read right-to-left!
glm::mat4 M = T * R * S;  // First scale, then rotate, then translate

// Example:
glm::mat4 model = glm::mat4(1.0f);
model = glm::translate(model, glm::vec3(0, 0.5, 0));  // 3rd: Move to position
model = glm::rotate(model, angle, glm::vec3(0,1,0)); // 2nd: Rotate
model = glm::scale(model, glm::vec3(10, 3, 3));       // 1st: Scale to size
```

---

## 📚 Learning Resources

1. **[LearnOpenGL](https://learnopengl.com/)** - The best OpenGL tutorial site
   - Start with "Getting Started" section
   - "Coordinate Systems" chapter is essential
   
2. **[OpenGL Reference Pages](https://www.khronos.org/opengl/)** - Official documentation

3. **[GLM Documentation](https://glm.g-truc.net/)** - Math library reference

4. **[GLFW Documentation](https://www.glfw.org/docs/latest/)** - Window/input handling

---

## 🎓 Complete Rendering Summary

```
┌─────────────────────────────────────────────────────────────────┐
│                    EVERY FRAME (60× per second)                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                 │
│  1. PROCESS INPUT                                               │
│     └─ Check keys, update camera/bus position                   │
│                                                                 │
│  2. UPDATE ANIMATIONS                                           │
│     └─ Fan rotation, wheel spin, door angle                     │
│                                                                 │
│  3. CLEAR SCREEN                                                │
│     └─ glClear(COLOR | DEPTH) - sky blue + depth buffer         │
│                                                                 │
│  4. SET SHADER UNIFORMS                                         │
│     └─ projection, view, lightPos, viewPos                      │
│                                                                 │
│  5. FOR EACH BUS COMPONENT:                                     │
│     ├─ Calculate model matrix (position, rotation, scale)       │
│     ├─ Set model + color uniforms                               │
│     └─ glDrawArrays() → GPU renders triangles                   │
│                                                                 │
│  6. SWAP BUFFERS                                                │
│     └─ glfwSwapBuffers() - show rendered image                  │
│                                                                 │
└─────────────────────────────────────────────────────────────────┘

VERTEX SHADER executes: ~5000+ times per frame (all vertices of bus)
FRAGMENT SHADER executes: ~100,000+ times per frame (all visible pixels)
```

---

## 🔍 Quick Reference: Key Numbers

| Item | Value | Notes |
|------|-------|-------|
| Window size | 1200 × 800 | 960,000 pixels |
| Cube vertices | 36 | 6 faces × 2 triangles × 3 vertices |
| Cylinder sectors | 36 | 10° per sector |
| Bus length | 10 units | X-axis: -5 to +5 |
| Bus width | 3 units | Z-axis: -1.5 to +1.5 |
| Bus height | 3 units | Y-axis: -1 to +2 |
| Wheel diameter | 1.2 units | |
| Seat rows | 8 per side | 16 total passenger seats |
| FOV | 45° | Vertical field of view |
| Near plane | 0.1 | Minimum render distance |
| Far plane | 100 | Maximum render distance |
| Max speed | 20 units/sec | In driving mode |
| Max steer | ±35° | Front wheel turn angle |

---

*Happy learning! 🚀 Modern OpenGL has a steep learning curve, but once you understand the coordinate spaces and shader pipeline, everything clicks into place!*
