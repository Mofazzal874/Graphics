# Illumination & Shading Models

## What is Illumination?

Illumination (or lighting) determines **how much light hits a surface**. It calculates the color/brightness of each point on an object based on light sources, surface orientation, and viewer position.

## The Phong Illumination Model

The Phong model breaks lighting into **3 components** that add together:

```
Final Color = Ambient + Diffuse + Specular
```

---

### 1. Ambient Light

**What:** Constant, uniform light everywhere — simulates light bouncing off walls/environment.

**Formula:**
```
Ambient = lightAmbient × objectColor
```

**Example:**
```
lightAmbient = (0.2, 0.2, 0.2)    // dim white
objectColor  = (0.9, 0.1, 0.1)    // red bus body
Ambient      = (0.18, 0.02, 0.02) // dim red everywhere
```

**In our shader (`shader.frag`):**
```glsl
vec3 ambient = vec3(0.0);
if (ambientOn)
    ambient = light.ambient * objectColor;
```

**Visual effect:** Without ambient, unlit surfaces are **completely black**. With it, you can always see the object faintly. Toggle with **key 5** to see the difference.

---

### 2. Diffuse Light (Lambert's Cosine Law)

**What:** Light that depends on the **angle between the surface normal and the light direction**. A surface facing the light is bright; a surface facing away is dark.

**Formula:**
```
diff    = max(dot(N, L), 0.0)
Diffuse = lightDiffuse × diff × objectColor
```

Where:
- `N` = surface normal (perpendicular to the surface)
- `L` = direction FROM surface TO light (normalized)
- `dot(N, L)` = cosine of the angle between them

**Example:**
```
N = (0, 1, 0)   // surface faces straight up
L = (0, 1, 0)   // light is directly above
dot(N, L) = 1.0  // maximum brightness (cos 0° = 1)

N = (0, 1, 0)   // surface faces up
L = (1, 0, 0)   // light is to the side
dot(N, L) = 0.0  // no diffuse light (cos 90° = 0)

N = (0, 1, 0)   // surface faces up
L = (0, -1, 0)  // light is below
dot(N, L) = -1.0 → clamped to 0.0  // cannot have negative light
```

**In our shader:**
```glsl
vec3 lightDir = normalize(-light.direction);  // direction TO light
float diff = max(dot(normal, lightDir), 0.0); // Lambert's cosine
vec3 diffuse = light.diffuse * diff * objectColor;
```

**Visual effect:** Creates the main "shading" — you can see the shape of the bus because sides facing light are brighter. Toggle with **key 6**.

---

### 3. Specular Light (Phong Reflection)

**What:** Shiny highlights — the bright spot you see on glossy surfaces when the light reflection hits your eye.

**Formula:**
```
R    = reflect(-L, N)          // mirror reflection of light
spec = pow(max(dot(V, R), 0.0), shininess)
Specular = lightSpecular × spec
```

Where:
- `V` = direction from surface to viewer/camera
- `R` = reflected light direction (mirror of L around N)
- `shininess` = how tight the highlight is (higher = smaller, sharper spot)

**Example:**
```
shininess = 2   → large, soft highlight (matte plastic)
shininess = 32  → medium highlight (our bus — default)
shininess = 256 → tiny, sharp highlight (polished metal)
```

**In our shader:**
```glsl
vec3 reflectDir = reflect(-lightDir, normal);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
vec3 specular = light.specular * spec;
```

**Visual effect:** Creates white/bright spots on the bus body when viewed from the right angle relative to the light. Toggle with **key 7**.

---

## The Complete Phong Formula

```
Color = Σ for each light:
          (Ambient_i + Diffuse_i + Specular_i)
```

All lights contribute independently, and their results are summed.

---

## Phong Shading vs Gouraud Shading

These are two different **shading techniques** — they use the SAME illumination formula (Phong) but apply it at different stages:

### Gouraud Shading (Per-Vertex)

```
Vertex Shader                    Fragment Shader
┌─────────────────────┐          ┌──────────────────────┐
│ Calculate lighting   │          │ Receive interpolated │
│ at each VERTEX       │ ──────▶ │ color, output it     │
│ (3 vertices/triangle)│          │ directly             │
└─────────────────────┘          └──────────────────────┘
```

- Lighting is calculated **3 times per triangle** (once per vertex)
- Colors are **interpolated** across the surface
- **Fast** but can miss specular highlights if they fall between vertices
- Highlights look blurry or disappear on large polygons

### Phong Shading (Per-Fragment) ← **What We Use**

```
Vertex Shader                    Fragment Shader
┌─────────────────────┐          ┌──────────────────────┐
│ Pass vertex NORMALS  │          │ Interpolate normals, │
│ to fragment shader   │ ──────▶ │ calculate lighting   │
│                      │          │ for EVERY PIXEL      │
└─────────────────────┘          └──────────────────────┘
```

- Lighting is calculated **once per pixel** (thousands of times per triangle)
- Normals are interpolated, then lighting formula applied per pixel
- **More accurate** specular highlights — captures them everywhere
- Slower but looks much better

### Comparison

| Feature | Gouraud | Phong |
|---------|---------|-------|
| Calculation location | Vertex shader | Fragment shader |
| Times calculated | 3 per triangle | Once per pixel |
| Speed | Faster | Slower |
| Specular quality | Can miss highlights | Accurate highlights |
| Our project | Not used | ✅ This is what we use |

**In our vertex shader** (`shader.vert`):
```glsl
// We pass position and normal to the fragment shader
FragPos = vec3(model * vec4(aPos, 1.0));
Normal = mat3(transpose(inverse(model))) * aNormal;
```
The normal transformation `mat3(transpose(inverse(model)))` ensures normals stay correct even when the model is scaled non-uniformly.

**In our fragment shader** (`shader.frag`):
```glsl
// Lighting calculated per-pixel using interpolated normals
vec3 norm = normalize(Normal);     // Interpolated & renormalized
vec3 viewDir = normalize(viewPos - FragPos);
result += CalcDirLight(dirLight, norm, viewDir);   // Full Phong per pixel
```
