# Code Architecture — How Everything Connects

## File-by-File Breakdown

```
Lab3_assignment/
├── shader.vert          ← Vertex shader (transforms positions & normals)
├── shader.frag          ← Fragment shader (ALL lighting calculations)
├── Shader.h             ← C++ class to load/compile/use shaders
├── Primitives.h         ← Cube, Cylinder, Torus geometry classes
├── Bus.h                ← Bus model (body, jet engine, hover pads)
└── assignment.cpp       ← Main app (cameras, viewports, input, render loop)
```

---

## Data Flow: From Vertex to Pixel Color

```
┌─────────────────────────────────────────────────────────────┐
│                    C++ (assignment.cpp)                      │
│                                                             │
│  1. Set light uniforms (positions, colors, toggles)         │
│  2. Set view/projection matrices                            │
│  3. For each viewport:                                      │
│     └─ bus.draw(shader, transform)                          │
│        └─ For each primitive (cube, cylinder):              │
│           ├─ Set model matrix (position, rotation, scale)   │
│           ├─ Set objectColor                                │
│           └─ glDrawArrays() ────────────────────────────┐   │
└─────────────────────────────────────────────────────────│───┘
                                                          │
                                                          ▼
┌─────────────────────────────────────────────────────────────┐
│                   Vertex Shader (shader.vert)                │
│                                                              │
│  For each vertex:                                            │
│  ├─ FragPos = model × vertex position  (world space)         │
│  ├─ Normal  = normalMatrix × vertex normal                   │
│  └─ gl_Position = projection × view × FragPos (screen space) │
│                                                              │
│  Outputs interpolated FragPos and Normal to fragment shader  │
└──────────────────────────────┬───────────────────────────────┘
                               │ (rasterization: interpolates
                               │  per-vertex data across pixels)
                               ▼
┌─────────────────────────────────────────────────────────────┐
│                 Fragment Shader (shader.frag)                 │
│                                                              │
│  For each PIXEL:                                             │
│  ├─ if (isEmissive) → output objectColor directly            │
│  └─ else:                                                    │
│     ├─ norm = normalize(interpolated Normal)                 │
│     ├─ viewDir = normalize(viewPos - FragPos)                │
│     ├─ if (dirLightOn)   result += CalcDirLight(...)         │
│     ├─ if (pointLightsOn) result += CalcPointLight(...)  ×4  │
│     ├─ if (spotLightOn)  result += CalcSpotLight(...)        │
│     └─ FragColor = vec4(result, alpha)                       │
└──────────────────────────────────────────────────────────────┘
```

---

## Key Functions Explained

### `CalcDirLight(light, normal, viewDir)`

**Purpose:** Calculate directional light contribution for one pixel.

```
Input:  light.direction = (-0.2, -1.0, -0.3)  // Sun direction
        normal = the surface normal at this pixel
        viewDir = direction from pixel to camera

Step 1: lightDir = normalize(-direction) = (0.12, 0.6, 0.18) // Flip: toward light
Step 2: ambient = light.ambient × objectColor
Step 3: diff = max(dot(normal, lightDir), 0.0)
         diffuse = light.diffuse × diff × objectColor
Step 4: reflectDir = reflect(-lightDir, normal)
         spec = pow(max(dot(viewDir, reflectDir), 0.0), 32)
         specular = light.specular × spec

Output: ambient + diffuse + specular
```

### `CalcPointLight(light, normal, fragPos, viewDir)`

**Same as directional, plus:**
```
Extra Step: distance = length(light.position - fragPos)
            attenuation = 1.0 / (1.0 + 0.09×d + 0.032×d²)
            Apply attenuation to ambient, diffuse, specular
```

### `CalcSpotLight(light, normal, fragPos, viewDir)`

**Same as point light, plus:**
```
Extra Step: theta = dot(lightDir, -spotDirection)
            if theta > cutOff:
                → full lighting (ambient + diffuse + specular)
            else:
                → ambient only (outside the cone)
```

---

## The Render Loop — Frame by Frame

```cpp
while (!glfwWindowShouldClose(window))
{
    // 1. Timing
    deltaTime = currentFrame - lastFrame;

    // 2. Input processing
    processInput(window);      // Camera movement, driving
    bus.updateFan(deltaTime);  // Animate fan
    bus.updateJetFlame(dt);    // Animate flame + hover bob

    // 3. Get actual window size (handles DPI scaling)
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

    // 4. Clear entire screen
    glViewport(0, 0, fbWidth, fbHeight);
    glClear(COLOR | DEPTH);

    // 5. Set light uniforms (same for all viewports)
    // ... dirLight, pointLights[0-3], spotLight ...

    // 6. Render 4 viewports
    for (v = 0 to 3) {
        glViewport(x, y, w/2, h/2);      // Set viewport area
        setBool("dirLightOn", vs.dirLightOn);  // Per-viewport state
        setMat4("view", camera[v]);       // Per-viewport camera
        bus.draw(shader, transform);       // Draw bus
    }

    // 7. Swap buffers
    glfwSwapBuffers(window);
}
```

---

## How Primitive Drawing Works

When `bus.draw()` is called, it calls `drawExterior()`, `drawInterior()`, `drawJetEngine()`, `drawHoverSkirts()`. Each function does:

```cpp
// Example: drawing the bus body
glm::mat4 model = parent;                              // Start with bus transform
model = glm::translate(model, glm::vec3(0, 0.5f, 0));  // Position body
model = glm::scale(model, glm::vec3(9.0f, 2.5f, 3.0f)); // Size it
cube.draw(shader, model, bodyColor);                    // Draw!
```

Inside `Cube::draw()`:
```cpp
void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
    shader.setMat4("model", model);           // Upload transform to GPU
    shader.setVec3("objectColor", color);     // Upload color to GPU
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);        // Draw 36 vertices (12 triangles)
}
```

This triggers the vertex shader → rasterization → fragment shader pipeline for each primitive.
