# Controls Reference — Hover Bus Simulation

This document lists every keyboard and mouse control available in the Hover Bus application.

---

## 1. Bus Driving Controls (Always Active)

| Key | Action | Details |
|-----|--------|---------|
| **W** | Accelerate / Thrust | Applies forward acceleration (`ACCELERATION = 20.0`) |
| **S** | Brake / Reverse | Applies negative acceleration (reverse thrust) |
| **A** | Steer Left | Turns the front wheels and yaws the bus leftward |
| **D** | Steer Right | Turns the front wheels and yaws the bus rightward |
| **Space** | Hover Up | Increases vertical speed; bus rises (max altitude = 50 units) |
| **Left Ctrl** | Hover Down | Decreases vertical speed; bus descends (min altitude = 0) |

### Driving Physics

- **Speed** is clamped to `±MAX_SPEED` (= 30 units/sec).
- When no thrust is applied, natural **deceleration** (`DECELERATION = 10.0`) slows the bus.
- **Steering** has auto-center: when A/D are released, `busSteerAngle` returns to 0.
- **Vertical movement** has smooth acceleration/deceleration and is clamped to `[0, 50]` for altitude.

```
busSpeed += ACCELERATION * deltaTime;        // W pressed
busSpeed = glm::clamp(busSpeed, -MAX_SPEED, MAX_SPEED);
busYaw  += busSteerAngle * busSpeed * deltaTime * 0.1f;
busPosition += forwardDir * busSpeed * deltaTime;
```

---

## 2. Camera Controls

### 2.1 Camera Mode Switching

| Key | Action |
|-----|--------|
| **V** | Cycle camera mode: Free → Chase → Interior → Free → ... |
| **K** | Toggle driving mode (Chase Cam ↔ Free Cam) |

### 2.2 Camera Modes

| Mode | Index | Description |
|------|-------|-------------|
| **Free Camera** | 0 | Fly freely using Arrow keys; WASD still drives the bus |
| **Chase Camera** | 1 | Camera follows behind the bus at a fixed offset |
| **Interior Camera** | 2 | Camera placed at driver seat position, looking forward |

### 2.3 Free Camera Movement

| Key | Action |
|-----|--------|
| **↑ Arrow** | Move camera forward |
| **↓ Arrow** | Move camera backward |
| **← Arrow** | Strafe camera left |
| **→ Arrow** | Strafe camera right |
| **Space** | Move camera up |
| **Left Ctrl** | Move camera down |
| **Left Shift** | Speed boost (2.5×) |
| **F (hold)** | Orbit around the bus |

### 2.4 Mouse Controls

| Input | Action |
|-------|--------|
| **Mouse Move** | Look around (yaw + pitch) — only when captured |
| **Scroll Wheel** | Zoom in/out (changes FOV: 15°–90°) |
| **M** | Toggle mouse capture (captured = FPS-style look) |

---

## 3. Texture Controls

| Key | Action | Cycles Through |
|-----|--------|---------------|
| **T** | Cycle texture blending mode | OFF → Pure Texture → Vertex-Blended (Gouraud) → Fragment-Blended (Phong) |
| **8** | Cycle texture wrapping mode | `GL_REPEAT` → `GL_CLAMP_TO_EDGE` → `GL_MIRRORED_REPEAT` |
| **9** | Cycle texture filter mode | `GL_LINEAR` → `GL_NEAREST` |
| **0** | Toggle all textures ON/OFF | Removes or restores all bus textures |

---

## 4. Lighting Controls

| Key | Action | Default |
|-----|--------|---------|
| **1** | Toggle Directional Light | ON |
| **2** | Toggle Point Lights (×4) | ON |
| **3** | Toggle Spot Light | ON |
| **4** | Toggle Emissive Light | ON |
| **5** | Toggle Ambient Component | ON |
| **6** | Toggle Diffuse Component | ON |
| **7** | Toggle Specular Component | ON |

---

## 5. Bus Feature Controls

| Key | Action |
|-----|--------|
| **B** | Toggle front door (open/close animation) |
| **G** | Toggle ceiling fan spinning |
| **L** | Toggle bus headlights |

---

## 6. Utility

| Key | Action |
|-----|--------|
| **Tab** | Print full status to console (camera, texture, lighting states) |
| **Esc** | Close the application |

---

## Control Flow Diagram

```
┌─────────────────────────────────────────────────────┐
│                    INPUT LOOP                        │
│                                                     │
│   processInput()     ← Continuous keys (every frame)│
│   ├── WASD           → Bus driving                  │
│   ├── Space/Ctrl     → Hover up/down                │
│   ├── Arrow keys     → Free camera movement         │
│   ├── Shift          → Speed boost                  │
│   └── F (hold)       → Orbit camera                 │
│                                                     │
│   key_callback()     ← Single-press keys            │
│   ├── V, K, M        → Camera / mouse               │
│   ├── T, 8, 9, 0     → Texture settings             │
│   ├── 1-7            → Lighting toggles              │
│   ├── B, G, L        → Bus features                  │
│   └── Tab            → Print status                  │
│                                                     │
│   mouse_callback()   ← Mouse movement               │
│   └── dx, dy         → Yaw & Pitch                  │
│                                                     │
│   scroll_callback()  ← Mouse wheel                  │
│   └── yoffset        → FOV zoom                     │
└─────────────────────────────────────────────────────┘
```
