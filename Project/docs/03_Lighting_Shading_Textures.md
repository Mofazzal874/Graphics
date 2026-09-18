# 03 — Lighting, Shading & Texture Mapping

This document covers the rendering pipeline: which lights exist, how they are computed, what the five texture modes do, and how the toggles in [InputHandler.h](../InputHandler.h) wire it all together. The shaders live in [shader.vert](../shader.vert) and [shader.frag](../shader.frag).

---

## 1. The Lighting Model in One Sentence

Each lit fragment computes

```
color = Σ ( ambient_i + diffuse_i + specular_i ) · attenuation_i
        over all enabled lights
```

using **Phong** (per-fragment, in `shader.frag`) for most modes, and a **Gouraud** path (per-vertex, in `shader.vert`) for texture mode 2. The key uniforms are `dirLightOn / pointLightsOn / spotLightOn` (which lights are active) and `ambientOn / diffuseOn / specularOn` (which Phong components are active). Every flag is bound to a number key in [InputHandler.h:194-207](../InputHandler.h#L194-L207).

---

## 2. The Light Sources

### 2.1 Directional light (key `1`)

Acts like the sun — infinitely far away, parallel rays, no attenuation.
Configured in [assignment.cpp:448-451](../assignment.cpp#L448-L451):

```cpp
dirLight.direction = (-0.2, -1.0, -0.3)   // pointing slightly forward + right + down
dirLight.ambient   = (0.15, 0.15, 0.15)
dirLight.diffuse   = (0.70, 0.70, 0.60)   // warm tint
dirLight.specular  = (0.50, 0.50, 0.50)
```

In the shader ([shader.frag:86-95](../shader.frag#L86-L95)) it computes:

```
L  = -normalize(direction)            // light → fragment
A  = light.ambient * matColor
D  = max(N·L, 0) · light.diffuse · matColor
R  = reflect(-L, N)
S  = max(V·R, 0)^shininess · light.specular
```

with `shininess = 32.0` ([assignment.cpp:503](../assignment.cpp#L503)).

### 2.2 Four point lights (key `2`)

Four colored lamps that **follow the bus** (their positions are recomputed each frame as `busPosition + offset`):

| Light | Offset relative to bus | Diffuse colour |
| ----- | ---------------------- | -------------- |
| 0     | `(+5, +5, +5)`         | red `(0.8, 0.1, 0.1)` |
| 1     | `(-5, +5, +5)`         | green `(0.1, 0.8, 0.1)` |
| 2     | `(+5, +5, -5)`         | blue `(0.1, 0.1, 0.8)` |
| 3     | `(-5, +5, -5)`         | white `(0.6, 0.6, 0.6)` |

Each one uses physically-motivated **distance attenuation**:

```
attenuation = 1 / (constant + linear·d + quadratic·d²)
```

with the typical OpenGL-cookbook values `(1.0, 0.09, 0.032)`. The full computation is in [shader.frag:97-111](../shader.frag#L97-L111).

### 2.3 Spotlight / flashlight (key `3`)

A cone light **anchored at the camera** pointing where you look. Defined in [assignment.cpp:493-497](../assignment.cpp#L493-L497):

```cpp
spotLight.position  = cameraPos
spotLight.direction = camera forward vector
spotLight.cutOff    = cos(cutoff angle)   // hard cone edge
```

In `CalcSpotLight` ([shader.frag:113-131](../shader.frag#L113-L131)), the angle between the light direction and the surface-to-light direction is compared against `cutOff`:

```
θ = dot(L, -dir)
if (θ > cutOff) → full Phong contribution
else            → ambient only
```

That hard cutoff is what gives the flashlight its sharp circular edge.

### 2.4 Emissive mode (key `4`)

Emissive surfaces **bypass lighting entirely** ([shader.frag:135-139](../shader.frag#L135-L139)):

```glsl
if (isEmissive) {
    FragColor = vec4(objectColor, alpha);
    return;
}
```

Used for the flame plume on the bus jet engine, the HUD score digits, and the cyan-magenta tint on collected Menger sponges. Anything you want to *glow* gets `isEmissive = true`.

### 2.5 Component toggles (`5`, `6`, `7`)

Inside each lighting function, the three components can be individually killed:

```glsl
vec3 ambient  = ambientOn  ? light.ambient * matColor   : vec3(0);
vec3 diffuse  = diffuseOn  ? light.diffuse * diff *  m  : vec3(0);
vec3 specular = specularOn ? light.specular * spec      : vec3(0);
```

So pressing `5` shows you "what does the world look like with no ambient term?" — surfaces in shadow go pitch black; pressing `7` shows the matte-only world with no glossy highlights. This is the standard demonstration of the Phong decomposition.

---

## 3. The Two Shading Models

### 3.1 Phong (per-fragment, the default)

Lighting is computed in `shader.frag`, *once per pixel*, using the interpolated `Normal` and `FragPos`. This is the higher-quality path and is used by texture modes `0`, `1`, `3`, `4`.

### 3.2 Gouraud (per-vertex, texture mode 2)

Lighting is computed in `shader.vert` and the **resulting colour** `VertexLightColor` is interpolated across the triangle. Then the fragment shader simply does:

```glsl
FragColor = texture(...) * VertexLightColor;
```

Per-vertex lighting is cheaper but smears specular highlights over whole triangles. Pressing `T` to cycle texture modes lets you see Gouraud (mode 2) and Phong (mode 3) side-by-side on the same model.

---

## 4. The Five Texture Modes

Cycled with the **`T`** key. The relevant uniform is `int textureMode`.

### 4.1 Mode 0 — No texture

Pure Phong on the object's `objectColor`. Used for the bus body when textures are off.

### 4.2 Mode 1 — Pure texture

```glsl
texColor = texture(textureSampler, TexCoord).rgb * objectColor;   // optional tint
result = Σ Phong(..., texColor)
```

The texture **replaces** the material color. This is the standard "wall-with-a-photo-on-it" mode. It also handles the **alpha-test billboard** path: when `alphaTest == true`, fragments with `alpha < 0.5` are `discard`-ed and the back-face normal is flipped so that double-sided leaf billboards stay lit:

```glsl
if (texSample.a < 0.5) discard;
if (!gl_FrontFacing) norm = -norm;
```
([shader.frag:148-170](../shader.frag#L148-L170))

### 4.3 Mode 2 — Texture × Gouraud

```glsl
result = textureColor * VertexLightColor    // VertexLightColor came from shader.vert
```

Cheap, smooth, but loses sharp highlights.

### 4.4 Mode 3 — Texture × Phong (blended with object color)

```glsl
blendedMat = mix(objectColor, textureColor, 0.7)   // 70 % texture, 30 % tint
result = Σ Phong(..., blendedMat)
```

So the texture and the underlying color are mixed *before* lighting. Used for the textured forest bark in [Forest.h:233-239](../Forest.h#L233-L239).

### 4.5 Mode 4 — Multi-texture blend with smooth transition

Two textures (`textureSampler` and `textureSampler2`) are blended along a **world-space axis** with a `smoothstep` ramp:

```glsl
coord = WorldPos.{x|y|z}              // axis chosen by uniform
factor = smoothstep(blendEdge - blendWidth,
                    blendEdge + blendWidth,
                    abs(coord));
blendedTex = mix(tex1, tex2, factor); // road → grass
blendedMat = mix(objectColor, blendedTex, 0.75);
result = Σ Phong(..., blendedMat)
```

This is what lets the road *fade smoothly* into grass at the shoulder instead of having a hard texture seam — the blend center sits at the road edge and the `smoothstep` gives a few units of soft transition. See [shader.frag:201-233](../shader.frag#L201-L233).

---

## 5. Texture Loading Pipeline

Source: [TextureLoader.h](../TextureLoader.h)

1. `stbi_load(path, &w, &h, &nrChannels, 0)` decodes PNG/JPG/BMP.
2. If either dimension exceeds `MAX_TEXTURE_DIM = 2048`, the image is **CPU-side downscaled** to keep VRAM use sane (`TextureLoader.h:46-58`).
3. Standard `glTexImage2D` upload + `glGenerateMipmap`.
4. Wrap and filter modes are *globally* settable via `currentWrapIndex` / `currentFilterIndex`, cycled by keys `8` and `9`. The wrap modes wired up are `REPEAT`, `MIRRORED_REPEAT`, `CLAMP_TO_EDGE`, `CLAMP_TO_BORDER`, and the filters are the standard 6 (`NEAREST`, `LINEAR`, plus their mipmap variants).

`updateSceneTextureParams()` walks every loaded texture and re-applies the current wrap+filter settings, so the user sees immediate visual feedback when cycling.

---

## 6. The Skybox (Cubemap)

A separate program ([skybox.vert](../skybox.vert) / [skybox.frag](../skybox.frag)) draws a cube **centered on the camera** with depth comparison set to `GL_LEQUAL` and depth write off. The cubemap can be loaded two ways:

* **Six separate face files** via `loadCubemapFromFaces`.
* A single **horizontal/vertical cross PNG** via `loadCubemapFromCross`, which slices the image into 6 face textures on the CPU and uploads them.

The fragment shader simply samples the cubemap with the un-normalised view direction:

```glsl
FragColor = texture(skyboxSampler, TexCoord);
```

Because the view matrix has its translation stripped (`mat4(mat3(view))`), the skybox always feels infinitely far away.

---

## 7. Putting It Together — A Single Frame

For one render of, say, a textured building wall under all lights:

1. CPU sets uniforms: `textureMode = 1`, `dirLightOn = pointLightsOn = spotLightOn = true`, all components on, `objectColor` = building tint, `shininess = 32`.
2. CPU binds the wall texture to unit 0, calls `glDrawArrays`.
3. Vertex shader transforms position, passes `FragPos`, `Normal`, `TexCoord`, `WorldPos` to the fragment shader.
4. Fragment shader:
   * Samples the texture, multiplies by `objectColor` tint → `texColor`.
   * Calls `CalcDirLight` (sun) → `result += ...`.
   * Loops `CalcPointLight` over the 4 bus lights → adds their attenuated contributions.
   * Calls `CalcSpotLight` (camera flashlight) → adds its conic contribution.
   * `clamp(result, 0, 1)` → `FragColor`.

Toggle off any one of the seven keys (1–7) and the corresponding term zeroes out — that is the end-to-end interactivity of the lighting demo.
