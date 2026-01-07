# OpenGL Aeroplane Project - Complete Guide

## ?? Table of Contents

1. [Project Overview](#-project-overview)
2. [Prerequisites & Dependencies](#-prerequisites--dependencies)
3. [Project Structure](#-project-structure)
4. [How OpenGL Works (Beginner's Guide)](#-how-opengl-works-beginners-guide)
5. [File-by-File Explanation](#-file-by-file-explanation)
   - [Assignment.cpp - Main Program](#1-assignmentcpp---main-program)
   - [Shader.h - Shader Management](#2-shaderh---shader-management)
   - [shader.vs - Vertex Shader](#3-shadervs---vertex-shader)
   - [shader.fs - Fragment Shader](#4-shaderfs---fragment-shader)
6. [Controls](#-controls)
7. [Common Terms Glossary](#-common-terms-glossary)

---

## ?? Project Overview

This project creates an interactive 2D aeroplane using OpenGL. The aeroplane can be:
- **Rotated** - Spin the plane around its center
- **Scaled** - Zoom in and out
- **Translated** - Move the plane around the screen
- **Disassembled** - View an "exploded" diagram showing all parts separated

### What You'll Learn
- OpenGL basics (vertices, shaders, buffers)
- 2D transformations (rotation, scaling, translation)
- Real-time user input handling
- Building complex shapes from triangles

---

## ?? Prerequisites & Dependencies

### Libraries Used

| Library | Purpose | Description |
|---------|---------|-------------|
| **GLFW** | Window Management | Creates windows, handles input (keyboard, mouse) |
| **GLAD** | OpenGL Loader | Loads OpenGL functions for your graphics card |
| **OpenGL** | Graphics API | Renders graphics on your GPU |

### Why These Libraries?

```
???????????????????????????????????????????????????????????????
?                      Your Application                        ?
???????????????????????????????????????????????????????????????
?  GLFW          ?  GLAD           ?  Your Code               ?
?  - Create      ?  - Load OpenGL  ?  - Define shapes         ?
?    window      ?    functions    ?  - Handle logic          ?
?  - Handle      ?  - Connect to   ?  - Send to GPU           ?
?    input       ?    GPU driver   ?                          ?
???????????????????????????????????????????????????????????????
?                    OpenGL (GPU Driver)                       ?
???????????????????????????????????????????????????????????????
?                    Graphics Card (GPU)                       ?
???????????????????????????????????????????????????????????????
```

---

## ?? Project Structure

```
test/
??? Assignment.cpp    # Main program - contains airplane geometry and logic
??? Shader.h          # Helper class to manage shaders
??? shader.vs         # Vertex shader - processes each vertex position
??? shader.fs         # Fragment shader - determines pixel colors
??? glad.c            # GLAD library implementation
??? includes/         # Header files for GLAD and GLFW
```

---

## ?? How OpenGL Works (Beginner's Guide)

### The Graphics Pipeline

When you draw something in OpenGL, your data goes through a "pipeline":

```
????????????????    ????????????????    ????????????????    ????????????????
?   Vertices   ??????    Vertex    ??????  Rasterizer  ??????   Fragment   ????? Screen
?   (Points)   ?    ?    Shader    ?    ? (Makes Pixels)?    ?    Shader    ?
????????????????    ????????????????    ????????????????    ????????????????
      CPU                GPU                  GPU                 GPU
```

### Step-by-Step Example

Let's say we want to draw a simple triangle:

**Step 1: Define Vertices (CPU)**
```cpp
// Each vertex has: X, Y, Z, R, G, B
float vertices[] = {
    // Position        // Color
    0.0f,  0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // Top vertex (Red)
   -0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // Bottom-left (Green)
    0.5f, -0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // Bottom-right (Blue)
};
```

**Step 2: Upload to GPU**
```cpp
// Create a buffer on the GPU
unsigned int VBO;
glGenBuffers(1, &VBO);
glBindBuffer(GL_ARRAY_BUFFER, VBO);
glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
```

**Step 3: Vertex Shader Processes Each Point**
```glsl
// This runs for EACH vertex (3 times for a triangle)
gl_Position = vec4(aPos, 1.0);  // Set position
ourColor = aColor;               // Pass color to next stage
```

**Step 4: Fragment Shader Colors Each Pixel**
```glsl
// This runs for EACH pixel inside the triangle
FragColor = vec4(ourColor, 1.0);  // Set pixel color
```

### OpenGL Coordinate System

OpenGL uses **Normalized Device Coordinates (NDC)**:

```
                    Y
                    ?
                    ? (0, 1)
                    ?
                    ?
    (-1, 0) ?????????????????? X (1, 0)
                    ?
                    ?
                    ? (0, -1)
                    
    Range: X from -1 to +1
           Y from -1 to +1
           
    Center of screen = (0, 0)
```

---

## ?? File-by-File Explanation

---

## 1. Assignment.cpp - Main Program

This is the heart of the application. Let's go through it section by section.

### 1.1 Include Headers

```cpp
#include <glad/glad.h>      // MUST come first - loads OpenGL functions
#include <GLFW/glfw3.h>     // Window and input handling

#include <iostream>         // Console output (std::cout)
#include <cmath>            // Math functions (cos, sin)
#include <vector>           // Dynamic arrays

#include "Shader.h"         // Our custom shader class
```

**Why does GLAD come first?**
GLAD defines OpenGL function pointers. If GLFW is included first, it might try to use OpenGL functions that haven't been loaded yet, causing errors.

### 1.2 Function Declarations (Forward Declarations)

```cpp
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
```

**What is a Forward Declaration?**
It tells the compiler "this function exists somewhere in the code" so you can use it before its actual definition.

```cpp
// Example:
void sayHello();  // Forward declaration - "this function exists"

int main() {
    sayHello();   // We can use it here
    return 0;
}

void sayHello() { // Actual definition
    std::cout << "Hello!";
}
```

### 1.3 Constants

```cpp
const unsigned int SCR_WIDTH = 800;   // Window width in pixels
const unsigned int SCR_HEIGHT = 600;  // Window height in pixels
const float PI = 3.14159265359f;      // Pi for angle calculations
```

**Why `const`?**
`const` means the value cannot be changed. It prevents accidental modifications and helps the compiler optimize.

**Why `unsigned int`?**
`unsigned` means only positive numbers (0 and above). Window dimensions can't be negative.

### 1.4 Global State Variables

```cpp
bool isDisassembled = false;    // Is plane in exploded view?
bool dKeyPressed = false;       // Is D key currently pressed?
bool rKeyPressed = false;       // Is R key currently pressed?
float rotationAngle = 0.0f;     // Current rotation (radians)
float scaleFactor = 1.0f;       // Current scale (1.0 = normal)
float translateX = 0.0f;        // Horizontal position offset
float translateY = 0.0f;        // Vertical position offset
```

**Why track `dKeyPressed` and `rKeyPressed`?**

Without these, holding a key would trigger the action every frame (60+ times per second!):

```cpp
// BAD - Without key tracking:
if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    rotationAngle += 15;  // Adds 15° every frame = spins wildly!
}

// GOOD - With key tracking:
if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) {
    if (!rKeyPressed) {           // Only if not already pressed
        rKeyPressed = true;       // Mark as pressed
        rotationAngle += 15;      // Rotate once
    }
} else {
    rKeyPressed = false;          // Reset when released
}
```

---

### 1.5 Helper Functions

#### `addTriangle` - The Foundation

Every shape in OpenGL is made of triangles. This function adds one triangle to our vertex array.

```cpp
void addTriangle(std::vector<float>& v, 
                 float x1, float y1,     // Vertex 1 position
                 float x2, float y2,     // Vertex 2 position
                 float x3, float y3,     // Vertex 3 position
                 float r, float g, float b,  // Color (0.0 to 1.0)
                 float offsetX = 0.0f,   // Optional X offset
                 float offsetY = 0.0f)   // Optional Y offset
```

**Parameter Breakdown:**

| Parameter | Type | Description |
|-----------|------|-------------|
| `v` | `std::vector<float>&` | Reference to vertex array (& means we modify the original) |
| `x1, y1` | `float` | First corner position |
| `x2, y2` | `float` | Second corner position |
| `x3, y3` | `float` | Third corner position |
| `r, g, b` | `float` | Color (Red, Green, Blue) from 0.0 to 1.0 |
| `offsetX` | `float` | Shift entire triangle horizontally |
| `offsetY` | `float` | Shift entire triangle vertically |

**How it works:**

```cpp
// For each vertex, we add 6 values:
v.push_back(x1 + offsetX);  // X position (with offset)
v.push_back(y1 + offsetY);  // Y position (with offset)
v.push_back(0.0f);          // Z position (always 0 for 2D)
v.push_back(r);             // Red color
v.push_back(g);             // Green color
v.push_back(b);             // Blue color
// Repeat for vertices 2 and 3...
```

**Visual Example:**

```
Drawing a red triangle at (0,0), (1,0), (0.5,1):

addTriangle(v, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f);
                ?      ?     ?      ?     ?      ?     ?     ?     ?
              x1,y1   x2,y2   x3,y3        R     G     B

Result:
        (0.5, 1.0)
           /\
          /  \
         /    \
        /______\
    (0,0)    (1,0)
```

**With Offset:**

```cpp
// Same triangle, shifted right by 0.5 and up by 0.3
addTriangle(v, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.3f);
                                                                    ?      ?
                                                                offsetX  offsetY

Result:
           (1.0, 1.3)
              /\
             /  \
            /    \
           /______\
       (0.5,0.3) (1.5,0.3)
```

#### `addQuad` - Drawing Rectangles

A quad (rectangle) is made of 2 triangles:

```cpp
void addQuad(std::vector<float>& v, 
             float x1, float y1,  // Top-left
             float x2, float y2,  // Top-right
             float x3, float y3,  // Bottom-right
             float x4, float y4,  // Bottom-left
             float r, float g, float b,
             float offsetX = 0.0f, float offsetY = 0.0f) 
{
    addTriangle(v, x1, y1, x2, y2, x3, y3, r, g, b, offsetX, offsetY);
    addTriangle(v, x1, y1, x3, y3, x4, y4, r, g, b, offsetX, offsetY);
}
```

**Visual Breakdown:**

```
(x1,y1)?????????(x2,y2)     Triangle 1: (x1,y1), (x2,y2), (x3,y3)
   ? \            ?         Triangle 2: (x1,y1), (x3,y3), (x4,y4)
   ?   \          ?
   ?     \        ?
   ?       \      ?
   ?         \    ?
(x4,y4)?????????(x3,y3)

Why 2 triangles? OpenGL can ONLY draw triangles!
```

#### `addEllipse` - Drawing Circles/Ovals

Creates an ellipse by drawing triangular "pie slices" from the center:

```cpp
void addEllipse(std::vector<float>& v, 
                float cx, float cy,    // Center position
                float rx, float ry,    // X and Y radius
                int seg,               // Number of segments (smoothness)
                float r, float g, float b,
                float offsetX = 0.0f, float offsetY = 0.0f)
```

**How it works:**

```cpp
for (int i = 0; i < seg; i++) {
    // Calculate angle for this slice
    float a1 = 2.0f * PI * i / seg;        // Start angle
    float a2 = 2.0f * PI * (i + 1) / seg;  // End angle
    
    // Create triangle from center to two edge points
    addTriangle(v, 
        cx, cy,                                    // Center
        cx + rx * cos(a1), cy + ry * sin(a1),     // Point 1 on edge
        cx + rx * cos(a2), cy + ry * sin(a2),     // Point 2 on edge
        r, g, b, offsetX, offsetY);
}
```

**Visual (8 segments shown):**

```
         ____
       /\ | /\
      /  \|/  \
     |----?----|   ? = center (cx, cy)
      \  /|\  /    Each slice = 1 triangle
       \/_|_\/
         
More segments = smoother circle
seg=8  ? octagon-ish
seg=16 ? smooth circle
seg=32 ? very smooth circle
```

**Circle vs Ellipse:**
- Circle: `rx == ry` (equal radius)
- Ellipse: `rx != ry` (stretched in one direction)

```cpp
addEllipse(v, 0, 0, 0.5f, 0.5f, 32, 1, 0, 0);  // Circle
addEllipse(v, 0, 0, 0.5f, 0.3f, 32, 1, 0, 0);  // Horizontal ellipse
```

#### `addNose` - Pointed Cone (Upward)

Creates a pointed nose shape that tapers to a point at the top:

```cpp
void addNose(std::vector<float>& v, 
             float cx, float baseY,    // Center X, base Y position
             float width, float height, // Max width, total height
             int seg,                   // Number of horizontal slices
             float r, float g, float b,
             float offsetX = 0.0f, float offsetY = 0.0f)
```

**The Math - Detailed Breakdown:**

The nose is built by stacking horizontal strips from bottom to top, with each strip getting narrower.

```cpp
for (int i = 0; i < seg; i++) {
    // STEP 1: Calculate progress (0.0 to 1.0)
    float t1 = (float)i / seg;        // Progress at bottom of this strip
    float t2 = (float)(i + 1) / seg;  // Progress at top of this strip
```

**What is `t`?**
`t` represents **how far along the nose we are**:
- `t = 0.0` ? At the base (bottom)
- `t = 0.5` ? Halfway up
- `t = 1.0` ? At the tip (top)

```
Example with seg = 4 (4 strips):

Strip 0: t1 = 0/4 = 0.00, t2 = 1/4 = 0.25  ? Bottom 25%
Strip 1: t1 = 1/4 = 0.25, t2 = 2/4 = 0.50  ? 25% to 50%
Strip 2: t1 = 2/4 = 0.50, t2 = 3/4 = 0.75  ? 50% to 75%
Strip 3: t1 = 3/4 = 0.75, t2 = 4/4 = 1.00  ? Top 25%
```

```cpp
    // STEP 2: Calculate Y positions
    float y1 = baseY + height * t1;   // Y at bottom of strip
    float y2 = baseY + height * t2;   // Y at top of strip
```

**How `y1` and `y2` work:**

| Strip | t1 | t2 | y1 (if baseY=0.42, height=0.25) | y2 |
|-------|-----|-----|--------------------------------|-----|
| 0 | 0.00 | 0.25 | 0.42 + 0.25×0.00 = **0.420** | 0.42 + 0.25×0.25 = **0.483** |
| 1 | 0.25 | 0.50 | **0.483** | **0.545** |
| 2 | 0.50 | 0.75 | **0.545** | **0.608** |
| 3 | 0.75 | 1.00 | **0.608** | **0.670** (tip) |

```cpp
    // STEP 3: Calculate widths (the key to the pointed shape!)
    float w1 = width * (1.0f - t1 * t1);  // Width at bottom of strip
    float w2 = width * (1.0f - t2 * t2);  // Width at top of strip
```

**Why `(1 - t²)` for width?**

This formula creates a **smooth curved taper** instead of straight sides:

| t | t² | 1 - t² | Width (if width=0.07) | Visual |
|---|-----|--------|----------------------|--------|
| 0.00 | 0.00 | **1.00** | 0.070 | ???????? Full width |
| 0.25 | 0.06 | **0.94** | 0.066 | ???????? |
| 0.50 | 0.25 | **0.75** | 0.053 | ???????? |
| 0.75 | 0.56 | **0.44** | 0.031 | ???????? |
| 1.00 | 1.00 | **0.00** | 0.000 | Point! |

**Linear vs Quadratic Taper:**

```
Linear (1-t):              Quadratic (1-t²):
     /\                         ?
    /  \                       / \
   /    \                     |   |
  /      \                    |   |
 /________\                  |_____|

Straight sides             Curved, bullet-like
(looks artificial)         (looks natural)
```

```cpp
    // STEP 4: Draw two triangles to form a trapezoid strip
    addTriangle(v, cx - w1, y1, cx + w1, y1, cx + w2, y2, r, g, b, offsetX, offsetY);
    addTriangle(v, cx - w1, y1, cx + w2, y2, cx - w2, y2, r, g, b, offsetX, offsetY);
}
```

**How the two triangles form each strip:**

```
        (-w2, y2)?????????????(+w2, y2)    ? Narrower (top of strip)
                 |\         /|
                 | \   2   / |
                 |  \     /  |
                 |   \   /   |
                 |    \ /    |
                 |     ?     |
                 |    / \    |
                 |   /   \   |
                 |  /  1  \  |
                 | /       \ |
                 |/         \|
        (-w1, y1)?????????????(+w1, y1)    ? Wider (bottom of strip)

Triangle 1: Bottom-left ? Bottom-right ? Top-right
Triangle 2: Bottom-left ? Top-right ? Top-left
```

**Complete Nose Assembly:**

```
                      ?  ? t=1.0, width=0 (point)
                     /|\
                    / | \
    Strip 3 ?      /  |  \
                  /   |   \
    Strip 2 ?    /    |    \
                /     |     \
    Strip 1 ? /      |      \
             /       |       \
    Strip 0 ?/       |        \
            /________|_________\  ? t=0, full width (base)
                   baseY

Each strip is a trapezoid made of 2 triangles.
More segments (seg) = smoother curve.
```

#### `addTail` - Pointed Cone (Downward)

Same as `addNose`, but points DOWN:

```cpp
float y1 = baseY - height * t1;  // Note: MINUS (going DOWN)
float y2 = baseY - height * t2;
```

---

### 1.6 The `buildAeroplane` Function

This is the main drawing function that creates all airplane parts.

#### Color Definitions

```cpp
void buildAeroplane(std::vector<float>& v, bool exploded) {
    // Main colors (RGB values from 0.0 to 1.0)
    float bodyR = 0.88f, bodyG = 0.90f, bodyB = 0.92f;      // Light silver
    float shadowR = 0.70f, shadowG = 0.72f, shadowB = 0.76f; // Dark gray
    float wingR = 0.78f, wingG = 0.80f, wingB = 0.84f;       // Medium gray
    float cockpitR = 0.28f, cockpitG = 0.33f, cockpitB = 0.42f; // Dark blue
    // ... more colors
```

**Color Scale:**
- `0.0` = No color (black)
- `1.0` = Full color
- `(1.0, 0.0, 0.0)` = Pure Red
- `(0.0, 1.0, 0.0)` = Pure Green
- `(0.0, 0.0, 1.0)` = Pure Blue
- `(1.0, 1.0, 1.0)` = White
- `(0.5, 0.5, 0.5)` = Gray

#### Dimension Variables

```cpp
float fuselageW = 0.07f;      // Half-width of airplane body
float fuselageTop = 0.42f;    // Top Y position
float fuselageBot = -0.52f;   // Bottom Y position
float noseHeight = 0.25f;     // Height of nose cone
float tailHeight = 0.20f;     // Height of tail cone
```

**Coordinate System Reference:**

```
        Y = 1.0
           ?
           ?
           ?  fuselageTop = 0.42
           ?    ?????
           ?    ?   ? ? fuselageW = 0.07
           ?    ?   ?
    ?????????????????????? X
   -1.0    ?    ?   ?    1.0
           ?    ?   ?
           ?    ?????
           ?  fuselageBot = -0.52
           ?
           ?
        Y = -1.0
```

#### Exploded View Offsets

The `exploded` parameter controls whether parts are separated:

```cpp
// When exploded=true, parts move apart
float leftWingOffX = exploded ? -0.20f : 0.0f;   // Move left
float leftWingOffY = exploded ? 0.08f : 0.0f;    // Move up

float rightWingOffX = exploded ? 0.20f : 0.0f;   // Move right
float rightWingOffY = exploded ? 0.08f : 0.0f;   // Move up

float noseOffY = exploded ? 0.15f : 0.0f;        // Move up
float rearTailOffY = exploded ? -0.15f : 0.0f;   // Move down
```

**The `? :` Operator (Ternary Operator):**

```cpp
// Syntax: condition ? value_if_true : value_if_false

float offset = exploded ? -0.20f : 0.0f;

// Is equivalent to:
float offset;
if (exploded) {
    offset = -0.20f;
} else {
    offset = 0.0f;
}
```

**Visual - Normal vs Exploded:**

```
NORMAL VIEW (exploded = false)         EXPLODED VIEW (exploded = true)
All offsets = 0                        Parts separated

      ? nose                                 ? nose (moved up)
     ???                                    ???
   ??????? wings                     ???   ???   ??? wings apart
     ? ?                                    ? ?
     ???                                    ???
      ? tail                                 ? tail (moved down)
```

#### Drawing Layers

The airplane is drawn in layers (back to front):

**Layer 1: Shadows**
```cpp
if (!exploded) {  // Only draw shadows in normal view
    float sOff = 0.018f;  // Shadow offset
    
    // Shadow is same shape as object, but:
    // - Darker color
    // - Slightly offset (right and down)
    addQuad(v, -fuselageW+sOff, fuselageTop-sOff, ...shadowColor);
}
```

**Layer 2: Engines**
```cpp
float engineY = 0.12f;

// Left engine = rectangle + triangle + ellipse
addQuad(v, ...);      // Engine body
addTriangle(v, ...);  // Bottom point
addEllipse(v, ...);   // Front intake
```

**Layer 3-7: Wings, Tail, Fuselage, Cockpit, Stabilizer**

Each layer uses the helper functions with appropriate coordinates and colors.

---

### 1.7 The `main` Function

#### GLFW Initialization

```cpp
int main()
{
    // Initialize GLFW library
    glfwInit();
    
    // Tell GLFW what OpenGL version we want
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);  // OpenGL 3.x
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);  // OpenGL x.3
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
```

**What is OpenGL Core Profile?**
- **Core Profile**: Modern OpenGL (3.3+), no deprecated functions
- **Compatibility Profile**: Includes old deprecated functions

#### Creating the Window

```cpp
    GLFWwindow* window = glfwCreateWindow(
        SCR_WIDTH,           // Width in pixels
        SCR_HEIGHT,          // Height in pixels
        "Window Title",      // Title bar text
        NULL,                // Monitor (NULL = windowed mode)
        NULL                 // Share (for multi-window apps)
    );
    
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;  // Exit with error
    }
    
    glfwMakeContextCurrent(window);  // Set as active window
```

**What is `glfwMakeContextCurrent`?**
OpenGL commands go to the "current context". This sets which window receives the drawing commands.

#### Loading OpenGL with GLAD

```cpp
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
```

**Why is this needed?**
OpenGL functions are loaded at runtime from your graphics driver. GLAD finds and loads all these functions.

#### Creating Shader and Buffers

```cpp
    // Load shaders from files
    Shader ourShader("shader.vs", "shader.fs");

    // Create buffer objects
    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);  // Generate 1 VAO
    glGenBuffers(1, &VBO);        // Generate 1 VBO
```

**What are VAO and VBO?**

| Object | Name | Purpose |
|--------|------|---------|
| VBO | Vertex Buffer Object | Stores vertex data on GPU |
| VAO | Vertex Array Object | Stores how to interpret vertex data |

**Analogy:**
- VBO = A filing cabinet full of documents (raw data)
- VAO = A label saying "Column A is Name, Column B is Age" (format)

#### Building and Uploading Vertex Data

```cpp
    // Create vertices on CPU
    std::vector<float> vertices;
    buildAeroplane(vertices, isDisassembled);
    
    // Bind VAO first
    glBindVertexArray(VAO);
    
    // Bind and fill VBO
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, 
                 vertices.size() * sizeof(float),  // Size in bytes
                 vertices.data(),                   // Pointer to data
                 GL_STATIC_DRAW);                   // Usage hint
```

**What is `GL_STATIC_DRAW`?**
- `GL_STATIC_DRAW`: Data won't change (or rarely)
- `GL_DYNAMIC_DRAW`: Data changes often
- `GL_STREAM_DRAW`: Data changes every frame

This helps OpenGL optimize memory placement.

#### Configuring Vertex Attributes

```cpp
    // Attribute 0: Position (3 floats)
    glVertexAttribPointer(
        0,                  // Attribute index (location = 0 in shader)
        3,                  // Number of components (x, y, z)
        GL_FLOAT,           // Data type
        GL_FALSE,           // Normalize?
        6 * sizeof(float),  // Stride (bytes between vertices)
        (void*)0            // Offset (where to start)
    );
    glEnableVertexAttribArray(0);
    
    // Attribute 1: Color (3 floats)
    glVertexAttribPointer(
        1,                          // Attribute index (location = 1)
        3,                          // Number of components (r, g, b)
        GL_FLOAT,                   // Data type
        GL_FALSE,                   // Normalize?
        6 * sizeof(float),          // Stride
        (void*)(3 * sizeof(float))  // Offset (after 3 floats)
    );
    glEnableVertexAttribArray(1);
```

**Memory Layout Visualization:**

```
One Vertex = 6 floats:
???????????????????????????????
? X  ? Y  ? Z  ? R  ? G  ? B  ?
???????????????????????????????
  ???? Attribute 0 ????  ?? Attribute 1 ??
  Position (3 floats)    Color (3 floats)

Array of vertices:
??????????????????????????????????????????????????????????????????????...
? X1 Y1 Z1 R1 G1 B1    ? X2 Y2 Z2 R2 G2 B2    ? X3 Y3 Z3 R3 G3 B3    ?
??????????????????????????????????????????????????????????????????????...
?????? Stride = 6 ??????????? Stride = 6 ??????
       floats                  floats

Attribute 0 offset = 0 (starts at beginning)
Attribute 1 offset = 3 * sizeof(float) = 12 bytes (starts after position)
```

#### The Render Loop

```cpp
    while (!glfwWindowShouldClose(window)) {
        // 1. Calculate delta time
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 2. Process input
        // ... (key handling)
        
        // 3. Render
        glClearColor(0.98f, 0.98f, 0.99f, 1.0f);  // Background color
        glClear(GL_COLOR_BUFFER_BIT);              // Clear screen
        
        ourShader.use();                           // Activate shader
        ourShader.setFloat("rotation", rotationAngle);
        ourShader.setFloat("scale", scaleFactor);
        ourShader.setVec2("translation", translateX, translateY);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);
        
        // 4. Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
```

**What is Delta Time?**

Delta time = time since last frame. It ensures consistent speed regardless of frame rate:

```cpp
// Without delta time (BAD):
translateX += 0.01f;  // Moves 0.01 per frame
// At 60 FPS: moves 0.6 per second
// At 30 FPS: moves 0.3 per second (slower!)

// With delta time (GOOD):
translateX += 0.8f * deltaTime;  // Moves 0.8 per second
// At 60 FPS: deltaTime ? 0.0167, move = 0.0133 per frame
// At 30 FPS: deltaTime ? 0.0333, move = 0.0267 per frame
// Both result in 0.8 units per second!
```

**What is Double Buffering?**

```
???????????????????    ???????????????????
?  Front Buffer   ?    ?  Back Buffer    ?
?  (Displayed)    ?    ?  (Being drawn)  ?
???????????????????    ???????????????????
        ?                      ?
    On Screen            Hidden from user

glfwSwapBuffers() swaps them:

???????????????????    ???????????????????
?  Back Buffer    ??????  Front Buffer   ?
?  (Now displayed)?    ?  (Now hidden)   ?
???????????????????    ???????????????????

This prevents flickering/tearing.
```

#### Cleanup

```cpp
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glfwTerminate();
    return 0;
}
```

Always free GPU resources when done!

---

## 2. Shader.h - Shader Management

The `Shader` class simplifies working with OpenGL shaders.

### What is a Shader?

A shader is a small program that runs on the GPU. There are two main types:

| Shader Type | Runs For Each... | Purpose |
|-------------|------------------|---------|
| Vertex Shader | Vertex (point) | Transform positions |
| Fragment Shader | Pixel (fragment) | Determine colors |

### Class Structure

```cpp
class Shader
{
public:
    unsigned int ID;  // OpenGL program ID
    
    // Constructor
    Shader(const char* vertexPath, const char* fragmentPath);
    
    // Methods
    void use();                                        // Activate shader
    void setBool(const std::string &name, bool value); // Set uniform
    void setInt(const std::string &name, int value);
    void setFloat(const std::string &name, float value);
    void setVec2(const std::string &name, float x, float y);
    
private:
    void checkCompileErrors(unsigned int shader, std::string type);
};
```

### Constructor - Reading and Compiling Shaders

```cpp
Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. Read shader files
    std::string vertexCode, fragmentCode;
    std::ifstream vShaderFile, fShaderFile;
    
    // Enable exceptions for file errors
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try {
        // Open and read files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();  // Read entire file
        fShaderStream << fShaderFile.rdbuf();
        
        vShaderFile.close();
        fShaderFile.close();
        
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e) {
        std::cout << "ERROR: Could not read shader file" << std::endl;
    }
```

**What is `std::stringstream`?**
A stream that reads/writes to a string. `rdbuf()` gets the raw buffer for efficient reading.

### Compiling Shaders

```cpp
    // 2. Compile vertex shader
    unsigned int vertex = glCreateShader(GL_VERTEX_SHADER);
    const char* vCode = vertexCode.c_str();
    glShaderSource(vertex, 1, &vCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");
    
    // 3. Compile fragment shader (same process)
    unsigned int fragment = glCreateShader(GL_FRAGMENT_SHADER);
    // ...
    
    // 4. Link into program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");
    
    // 5. Delete individual shaders (no longer needed)
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}
```

**Shader Compilation Pipeline:**

```
????????????????     ????????????????
? shader.vs    ?     ? shader.fs    ?
? (text file)  ?     ? (text file)  ?
????????????????     ????????????????
       ?                    ?
       ?                    ?
????????????????     ????????????????
?   Compile    ?     ?   Compile    ?
?   Vertex     ?     ?   Fragment   ?
????????????????     ????????????????
       ?                    ?
       ??????????????????????
                ?
                ?
        ????????????????
        ?    Link      ?
        ?   Program    ?
        ????????????????
               ?
               ?
        ????????????????
        ?   Shader     ?
        ?   Program    ?
        ?   (ID)       ?
        ????????????????
```

### Uniform Functions

```cpp
void setFloat(const std::string &name, float value) const
{ 
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
}

void setVec2(const std::string &name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y);
}
```

**What is a Uniform?**
A uniform is a variable you send from C++ to the shader. Unlike attributes (per-vertex), uniforms are the same for all vertices/fragments.

```cpp
// In C++:
ourShader.setFloat("rotation", 1.57f);  // 90 degrees

// In shader:
uniform float rotation;  // Receives the value
```

---

## 3. shader.vs - Vertex Shader

The vertex shader processes each vertex position.

```glsl
#version 330 core
```

**Version Declaration**: Use GLSL version 3.30 (matches OpenGL 3.3)

```glsl
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
```

**Input Variables**:
- `layout (location = 0)`: Matches `glVertexAttribPointer(0, ...)`
- `in`: This is an input from the vertex buffer
- `vec3`: A vector with 3 components (x, y, z)
- `aPos`: Position attribute
- `aColor`: Color attribute

```glsl
out vec3 ourColor;
```

**Output Variable**: Passed to fragment shader

```glsl
uniform float rotation;
uniform float scale;
uniform vec2 translation;
```

**Uniform Variables**: Set from C++ code, same for all vertices

### The Transformation Math

```glsl
void main()
{
    // Step 1: Scale
    float scaledX = aPos.x * scale;
    float scaledY = aPos.y * scale;
```

**Scaling**: Multiply coordinates by scale factor
- `scale = 2.0` ? Object becomes 2x larger
- `scale = 0.5` ? Object becomes half size

```glsl
    // Step 2: Rotate
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    float rotatedX = scaledX * cosR - scaledY * sinR;
    float rotatedY = scaledX * sinR + scaledY * cosR;
```

**Rotation Formula** (2D rotation around origin):
```
x' = x * cos(?) - y * sin(?)
y' = x * sin(?) + y * cos(?)
```

Visual example:
```
Before rotation (0°):     After rotation (90°):
       Y                        Y
       ?                        ?
       ?  ?P(1,0)               ?
???????????????? X       ???????????????? X
       ?                 P(0,1) ?
       ?                        ?

cos(90°) = 0, sin(90°) = 1
x' = 1*0 - 0*1 = 0
y' = 1*1 + 0*0 = 1
Point moved from (1,0) to (0,1)
```

```glsl
    // Step 3: Translate
    float finalX = rotatedX + translation.x;
    float finalY = rotatedY + translation.y;
```

**Translation**: Add offset to move position

```glsl
    // Step 4: Output position
    gl_Position = vec4(finalX, finalY, aPos.z, 1.0);
    ourColor = aColor;
}
```

**`gl_Position`**: Built-in output variable that sets the final vertex position
- `vec4(x, y, z, w)` where `w` is usually 1.0

### Why This Order? (Scale ? Rotate ? Translate)

```
Order matters! Different orders give different results.

Scale then Rotate then Translate (SRT) - CORRECT:
1. Scale object at origin
2. Rotate object at origin  
3. Move object to final position

vs.

Translate then Rotate then Scale (TRS) - DIFFERENT:
1. Move object away from origin
2. Rotate (object orbits around origin!)
3. Scale

SRT keeps the object rotating around its own center.
```

---

## 4. shader.fs - Fragment Shader

The fragment shader determines the color of each pixel.

```glsl
#version 330 core
out vec4 FragColor;
in vec3 ourColor;

void main()
{
    FragColor = vec4(ourColor, 1.0);
}
```

**Line-by-line:**

| Line | Explanation |
|------|-------------|
| `#version 330 core` | GLSL version 3.30 |
| `out vec4 FragColor` | Output: final pixel color (RGBA) |
| `in vec3 ourColor` | Input: color from vertex shader |
| `FragColor = vec4(ourColor, 1.0)` | Set pixel color (RGB + Alpha=1.0) |

**What is Alpha?**
Alpha = opacity (transparency)
- `1.0` = Fully opaque (solid)
- `0.5` = 50% transparent
- `0.0` = Fully transparent (invisible)

**Color Interpolation:**

When colors are set per-vertex, OpenGL automatically blends them across the triangle:

```
     Red (1,0,0)
        /\
       /  \
      / ?  \  ? This pixel gets a blend of all 3 colors
     /______\
Green      Blue
(0,1,0)    (0,0,1)

The center pixel might be approximately (0.33, 0.33, 0.33) - grayish
```

---

## ?? Controls

| Key | Action | Code |
|-----|--------|------|
| **D** | Toggle disassembly view | `isDisassembled = !isDisassembled` |
| **R** | Rotate 15 degrees | `rotationAngle += PI/12` |
| **+** | Zoom in | `scaleFactor += 1.5 * deltaTime` |
| **-** | Zoom out | `scaleFactor -= 1.5 * deltaTime` |
| **?** | Move up | `translateY += 0.8 * deltaTime` |
| **?** | Move down | `translateY -= 0.8 * deltaTime` |
| **?** | Move left | `translateX -= 0.8 * deltaTime` |
| **?** | Move right | `translateX += 0.8 * deltaTime` |
| **ESC** | Exit | `glfwSetWindowShouldClose(window, true)` |

---

## ?? Common Terms Glossary

| Term | Definition |
|------|------------|
| **Vertex** | A point in space (x, y, z) with attributes (color, texture coords, etc.) |
| **Fragment** | A potential pixel - may or may not make it to the screen |
| **Shader** | A small program that runs on the GPU |
| **Uniform** | A variable sent from CPU to GPU, same for all vertices |
| **Attribute** | Per-vertex data (position, color, etc.) |
| **VBO** | Vertex Buffer Object - stores vertex data on GPU |
| **VAO** | Vertex Array Object - stores vertex attribute configuration |
| **NDC** | Normalized Device Coordinates (-1 to +1 range) |
| **GLSL** | OpenGL Shading Language - language for writing shaders |
| **Render Loop** | Continuous loop that draws frames |
| **Double Buffering** | Drawing to hidden buffer, then swapping to prevent flicker |
| **Delta Time** | Time since last frame, for consistent animation speed |
| **Rasterization** | Converting vector shapes to pixels |
| **Pipeline** | Series of stages data passes through |
| **Context** | OpenGL state machine - all settings and buffers |

---

## ?? Building the Project

### Requirements
- C++14 compatible compiler
- GLFW library
- GLAD (included)
- OpenGL 3.3+ capable graphics card

### Visual Studio
1. Open the `.vcxproj` file
2. Build ? Build Solution (Ctrl+Shift+B)
3. Debug ? Start Without Debugging (Ctrl+F5)

---

## ?? Summary

This project demonstrates:

1. **OpenGL Basics**: Vertices, buffers, shaders
2. **2D Drawing**: Building shapes from triangles
3. **Transformations**: Scale, rotate, translate
4. **Real-time Input**: Keyboard controls
5. **Shader Programming**: GLSL vertex and fragment shaders
6. **Code Organization**: Separating concerns (Shader class)

The airplane is built by:
1. Defining colors and dimensions
2. Creating helper functions for shapes
3. Drawing layers from back to front
4. Applying transformations in shaders
5. Responding to user input

---

## ????? Author

Created for CSE 4208 - Computer Graphics

---

*Happy Coding! ??*
