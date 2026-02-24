# Texture Mapping Integration

## Overview

This document covers the texture mapping system integrated into the Hover Bus project, including all keyboard/mouse controls.

---

## Complete Controls Reference

### Mouse Controls

| Input | Action |
|---|---|
| **Mouse Movement** | Look around (when captured) |
| **Scroll Wheel Up** | Zoom in (decrease FOV) |
| **Scroll Wheel Down** | Zoom out (increase FOV) |
| **M** | Toggle mouse capture (captured = FPS-style, free = normal cursor) |

### Camera Controls

| Key | Action |
|---|---|
| **V** | Cycle camera mode: Free → Chase → Interior |

**Camera Modes:**
- **Free Camera** — Fly anywhere. Use WASD + mouse to navigate. Best for inspecting the scene from any angle.
- **Chase Camera** — 3rd-person view behind the bus. Automatically activates when driving mode starts.
- **Interior Camera** — 1st-person view from the driver's seat. Shows textured seats, floor, walls, and dashboard. Use mouse to look around inside.

### Movement (Free Camera)

| Key | Action |
|---|---|
| **W** | Move forward |
| **S** | Move backward |
| **A** | Strafe left |
| **D** | Strafe right |
| **Space** | Move up |
| **Left Ctrl** | Move down |
| **Q** | Roll left |
| **E** | Roll right |
| **F** (hold) | Orbit around the bus |
| **Shift** | Speed boost (2.5×) |

### Driving Mode

| Key | Action |
|---|---|
| **K** | Toggle driving mode on/off |
| **W** | Thrust forward |
| **S** | Brake / reverse |
| **A** | Steer left |
| **D** | Steer right |

> When driving starts, camera auto-switches to Chase mode. Press **V** to switch to Interior (first-person) while driving!

### Bus Controls

| Key | Action |
|---|---|
| **B** | Open / close front door |
| **G** | Toggle ceiling fan |
| **L** | Toggle interior lights |

### Texture Controls

| Key | Action |
|---|---|
| **T** | Cycle texture mode: Off → Pure → Vertex-Blended (Gouraud) → Fragment-Blended (Phong) |
| **8** | Cycle wrapping: GL_REPEAT → GL_CLAMP_TO_EDGE → GL_MIRRORED_REPEAT |
| **9** | Cycle filtering: GL_LINEAR ↔ GL_NEAREST |
| **0** | Toggle ALL textures on/off |

### Lighting Controls

| Key | Action |
|---|---|
| **1** | Toggle directional light |
| **2** | Toggle point lights (4 colored) |
| **3** | Toggle spotlight (flashlight follows camera) |
| **4** | Toggle emissive glow (jet flames, hover pads) |
| **5** | Toggle ambient component |
| **6** | Toggle diffuse component |
| **7** | Toggle specular component |

### Other

| Key | Action |
|---|---|
| **TAB** | Print full status to console |
| **ESC** | Exit |

---

## How to See the Bus Interior

1. Launch the program
2. Press **V** twice to switch to **Interior Camera**
3. Move the mouse to look around — you'll see:
   - Textured floor panels
   - Textured seat fabric (cushions and backrests)
   - Textured interior wall panels
   - Dashboard
   - Steering wheel, handrails, luggage racks
4. Press **L** to toggle interior lights on/off
5. Press **G** to spin the ceiling fans
6. Press **B** to open/close the door

**Alternative:** In Free Camera mode, fly inside the bus by pressing W to move forward and using the mouse to aim through the door or windshield.

---

## Texture Mapping Technical Details

### Texture Modes

| Mode | Key | Description |
|---|---|---|
| 0 — Off | `T` to cycle | Original Phong lighting, no texture |
| 1 — Pure | `T` to cycle | Texture replaces object color, lit by Phong |
| 2 — Vertex-Blended | `T` to cycle | Texture × Gouraud (vertex-computed) lighting |
| 3 — Fragment-Blended | `T` to cycle | Texture × per-fragment Phong lighting |

### Textured Surfaces

| Surface | Texture File | Wrap | Filter | Mode |
|---|---|---|---|---|
| Interior floor | `floor.jpg` | REPEAT | LINEAR | Fragment-blended |
| Aisle carpet | `carpet.jpg` | REPEAT | NEAREST | Pure |
| Seat cushions & backs | `fabric.jpg` | CLAMP_TO_EDGE | LINEAR | Vertex-blended |
| Interior walls | `wall.jpg` | MIRRORED_REPEAT | LINEAR | Fragment-blended |
| Dashboard | `dashboard.jpg` | REPEAT | NEAREST | Pure |
| Bus side body | `busbody.jpg` | CLAMP_TO_EDGE | NEAREST | Pure |
| Scene sphere | `sphere.jpg` | REPEAT | LINEAR | Toggled with T |
| Scene cone | `cone.jpg` | MIRRORED_REPEAT | NEAREST | Toggled with T |

### Image Files Location

All texture images go in: `textures/` folder (relative to executable).

Files present: `floor.jpg`, `fabric.jpg`, `wall.jpg`, `busbody.jpg`
Optional files: `carpet.jpg`, `dashboard.jpg`, `sphere.jpg`, `cone.jpg` (missing files are handled gracefully — surfaces render with base color only).

### Wrapping Modes Demonstrated

- **GL_REPEAT** — Texture repeats beyond [0,1] range (floor tiles)
- **GL_CLAMP_TO_EDGE** — Texture stretches at edges (seat fabric, bus body)
- **GL_MIRRORED_REPEAT** — Alternating mirror pattern (interior walls)

### Filtering Modes Demonstrated

- **GL_LINEAR** — Smooth interpolation between texels (floor, fabric, walls)
- **GL_NEAREST** — Sharp, pixelated look (carpet, dashboard, bus body)

---

## Files Modified

| File | Changes |
|---|---|
| `shader.vert` | Added texture coordinate passthrough + Gouraud vertex lighting |
| `shader.frag` | Added 4 texture modes with sampler2D uniform |
| `Primitives.h` | Added UV coords (8-float vertex), new Sphere & Cone classes |
| `Bus.h` | Added texture IDs + drawTextured helpers for interior/exterior |
| `assignment.cpp` | Single viewport, stb_image loading, game controls, camera modes |
