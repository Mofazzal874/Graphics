# Light Source Types

Our project implements **4 types** of light sources. Each type models a different real-world lighting scenario.

---

## 1. Directional Light (Sun)

**Real-world analogy:** The sun — so far away that all rays are parallel.

**Key property:** Has a **direction** but no position. Every point in the scene receives light from the same direction.

```
    ↓  ↓  ↓  ↓  ↓  ↓    Parallel rays
    ↓  ↓  ↓  ↓  ↓  ↓    (all same direction)
  ┌──────────────────┐
  │     BUS BODY     │
  └──────────────────┘
```

**No attenuation** — light intensity doesn't decrease with distance (the sun is effectively infinitely far away).

### In our shader:

```glsl
struct DirLight {
    vec3 direction;   // Direction the light is shining (e.g. -0.2, -1.0, -0.3)
    vec3 ambient;     // Ambient color/intensity
    vec3 diffuse;     // Diffuse color/intensity
    vec3 specular;    // Specular color/intensity
};

vec3 CalcDirLight(DirLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction); // FLIP: we need direction TO light
    // ... ambient + diffuse + specular calculation
}
```

### In our C++ code (`assignment.cpp`):

```cpp
ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);  // Shining down-left
ourShader.setVec3("dirLight.ambient",   0.15f, 0.15f, 0.15f);  // Dim white fill
ourShader.setVec3("dirLight.diffuse",   0.7f,  0.7f,  0.6f);   // Warm white
ourShader.setVec3("dirLight.specular",  0.5f,  0.5f,  0.5f);   // White highlights
```

**Toggle:** Key **1**

---

## 2. Point Light (Light Bulb)

**Real-world analogy:** A bare light bulb — radiates light equally in **all directions** from a single point.

**Key property:** Has a **position** and light **attenuates** (weakens) with distance.

```
         💡 (position)
        / | \
       /  |  \     Radiates in all directions
      /   |   \
     /    |    \
  ┌──────────────┐
  │   BUS BODY   │
  └──────────────┘
```

### Attenuation Formula

Light intensity decreases with distance:

```
                        1
attenuation = ─────────────────────────────────
              constant + linear×d + quadratic×d²
```

| Parameter | Value | Effect |
|-----------|-------|--------|
| constant | 1.0 | Base intensity (always 1.0) |
| linear | 0.09 | Gentle falloff over distance |
| quadratic | 0.032 | Sharper falloff at far distances |

**Example at different distances:**
```
d = 1   → attenuation = 1/(1 + 0.09 + 0.032)  = 0.89  (89% strength)
d = 10  → attenuation = 1/(1 + 0.9 + 3.2)      = 0.20  (20% strength)
d = 50  → attenuation = 1/(1 + 4.5 + 80)        = 0.012 (1.2% — barely visible)
```

### In our shader:

```glsl
struct PointLight {
    vec3 position;     // Where the light is in the world
    vec3 ambient, diffuse, specular;
    float constant;    // Attenuation parameters
    float linear;
    float quadratic;
};

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos); // Direction TO light
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance
                             + light.quadratic * distance * distance);
    // ... ambient + diffuse + specular, all multiplied by attenuation
}
```

### Our 4 Point Lights:

```cpp
// Red light (front-right of bus)
pointLights[0].position = busPosition + (5, 5, 5)
pointLights[0].diffuse  = (0.8, 0.1, 0.1)  // Red

// Green light (front-left)
pointLights[1].position = busPosition + (-5, 5, 5)
pointLights[1].diffuse  = (0.1, 0.8, 0.1)  // Green

// Blue light (rear-right)
pointLights[2].position = busPosition + (5, 5, -5)
pointLights[2].diffuse  = (0.1, 0.1, 0.8)  // Blue

// White light (rear-left)
pointLights[3].position = busPosition + (-5, 5, -5)
pointLights[3].diffuse  = (0.6, 0.6, 0.6)  // White
```

**Toggle:** Key **2**

---

## 3. Spot Light (Flashlight)

**Real-world analogy:** A flashlight or stage spotlight — a cone of light from a specific point in a specific direction.

**Key property:** Has position, direction, AND a **cutoff angle** defining the cone.

```
     🔦 position
      \  |  /
       \ | /    cutOff angle
        \|/
    ─────●─────  cone boundary
         |
    lit area only inside cone
```

### Cutoff Angle

- `cutOff = cos(12.5°) ≈ 0.976`
- For each fragment, calculate `theta = dot(lightDir, spotDirection)`
- If `theta > cutOff` → inside cone (lit)
- If `theta ≤ cutOff` → outside cone (only ambient)

### In our shader:

```glsl
struct SpotLight {
    vec3 position;
    vec3 direction;     // Where the spotlight points
    vec3 ambient, diffuse, specular;
    float cutOff;       // cos(cutoff angle) — NOT the angle itself!
    float constant, linear, quadratic;
};

vec3 CalcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position - fragPos);
    float theta = dot(lightDir, normalize(-light.direction));

    if (theta > light.cutOff) {
        // INSIDE the cone — full lighting
        // calculate ambient + diffuse + specular with attenuation
    } else {
        // OUTSIDE the cone — ambient only
    }
}
```

### In our C++ code:

```cpp
// Flashlight follows the camera
ourShader.setVec3("spotLight.position", cameraPos);
ourShader.setVec3("spotLight.direction", getCameraFront());  // Points where you look
ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));  // 12.5° cone
```

**Toggle:** Key **3**

---

## 4. Emissive Light (Self-Glow)

**Real-world analogy:** Neon signs, fire, glowing screens — objects that **emit their own light**.

**Key property:** Not affected by external lighting. The object's color IS its light output.

```
Normal object:              Emissive object:
  Looks dark without        Always bright regardless
  any light source          of light sources

  ⬛ (in shadow)             🟧 (always glowing)
```

### In our shader:

```glsl
if (isEmissive) {
    // Skip ALL lighting calculations
    // Output the color directly at full brightness
    FragColor = vec4(objectColor, alpha);
    return;
}
```

### Where we use it:

- **Jet engine flame** — 9 overlapping layers with additive blending
- **Hover pad glow** — pulsating blue energy discs
- **Afterburner glow disc** — hot white at nozzle exit

### Additive Blending (used with emissive)

When emissive objects overlap, their colors **add together** instead of replacing:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Source + Destination (additive)
glDepthMask(GL_FALSE);              // Don't occlude behind

// ... draw flame layers ...

glDepthMask(GL_TRUE);
glDisable(GL_BLEND);
```

This makes overlapping flame layers brighter at the center, creating a realistic glow.

**Toggle:** Key **4**

---

## Summary Table

| Light Type | Position | Direction | Attenuation | Cone | Toggle |
|------------|----------|-----------|-------------|------|--------|
| Directional | ✗ | ✓ | ✗ | ✗ | Key 1 |
| Point | ✓ | ✗ (all dirs) | ✓ | ✗ | Key 2 |
| Spot | ✓ | ✓ | ✓ | ✓ | Key 3 |
| Emissive | N/A | N/A | N/A | N/A | Key 4 |
