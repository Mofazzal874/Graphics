# Controls Reference — Hover Bus Simulation

This document lists every keyboard and mouse control available in the Hover Bus application.
All controls are handled in [assignment.cpp](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp).

---

## 1. Bus Driving Controls (Always Active)

> **Source**: [processInput()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L863-L927) in `assignment.cpp` — called every frame.

| Key | Action | Details |
|-----|--------|---------|
| **W** | Accelerate / Thrust | Applies forward acceleration (`ACCELERATION = 20.0`) |
| **S** | Brake / Reverse | Applies negative acceleration (reverse thrust) |
| **A** | Steer Left | Turns the front wheels and yaws the bus leftward |
| **D** | Steer Right | Turns the front wheels and yaws the bus rightward |
| **Space** | Hover Up | Increases vertical speed; bus rises (max altitude = 50 units) |
| **Left Ctrl** | Hover Down | Decreases vertical speed; bus descends (min altitude = 0) |

### Driving Physics

> **Source**: [assignment.cpp:L871–L899](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L871-L899) — speed, steering, and altitude logic.

- **Speed** is clamped to `±MAX_SPEED` (= 30 units/sec).
- When no thrust is applied, natural **deceleration** (`DECELERATION = 10.0`) slows the bus.
- **Steering** has auto-center: when A/D are released, `busSteerAngle` returns to 0.
- **Vertical movement** has smooth acceleration/deceleration and is clamped to `[0, 50]` for altitude.

```cpp
// assignment.cpp:L877–L896
busSpeed += ACCELERATION * deltaTime;        // W pressed
busSpeed = glm::clamp(busSpeed, -MAX_SPEED, MAX_SPEED);
busYaw  += busSteerAngle * busSpeed * deltaTime * 0.1f;
busPosition += forwardDir * busSpeed * deltaTime;
```

---

## 2. Camera Controls

### 2.1 Camera Mode Switching

> **Source**: [key_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L947-L1018) — `GLFW_KEY_V` at line 952, `GLFW_KEY_K` at line 1019.

| Key | Action |
|-----|--------|
| **V** | Cycle camera mode: Free → Chase → Interior → Free → ... |
| **K** | Toggle driving mode (Chase Cam ↔ Free Cam) |

### 2.2 Camera Modes

> **Source**: [getViewMatrix()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L206-L243) — computes view matrix based on mode.

| Mode | Index | Description |
|------|-------|-------------|
| **Free Camera** | 0 | Fly freely using Arrow keys; WASD still drives the bus |
| **Chase Camera** | 1 | Camera follows behind the bus at a fixed offset |
| **Interior Camera** | 2 | Camera placed at driver seat position, looking forward |

### 2.3 Free Camera Movement

> **Source**: [processInput()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L918-L927) — arrow key movement and orbit.

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

> **Source**: [mouse_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L826-L834), [scroll_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L854-L858)

| Input | Action |
|-------|--------|
| **Mouse Move** | Look around (yaw + pitch) — only when captured |
| **Scroll Wheel** | Zoom in/out (changes FOV: 15°–90°) |
| **M** | Toggle mouse capture (captured = FPS-style look) |

---

## 3. Texture Controls

> **Source**: [key_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L969-L997) — texture keys T, 8, 9, 0.

| Key | Action | Cycles Through |
|-----|--------|---------------|
| **T** | Cycle texture blending mode | OFF → Pure Texture → Vertex-Blended (Gouraud) → Fragment-Blended (Phong) |
| **8** | Cycle texture wrapping mode | `GL_REPEAT` → `GL_CLAMP_TO_EDGE` → `GL_MIRRORED_REPEAT` |
| **9** | Cycle texture filter mode | `GL_LINEAR` → `GL_NEAREST` |
| **0** | Toggle all textures ON/OFF | Removes or restores all bus textures |

---

## 4. Lighting Controls

> **Source**: [key_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L999-L1013) — keys 1–7.

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

> **Source**: [key_callback()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L1015-L1028) — keys B, G, L, K.

| Key | Action |
|-----|--------|
| **B** | Toggle front door (open/close animation) |
| **G** | Toggle ceiling fan spinning |
| **L** | Toggle bus headlights |

---

## 6. Utility

| Key | Action | Source |
|-----|--------|--------|
| **Tab** | Print full status to console | [key_callback:L1031](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L1031) calls [printStatus()](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L354-L371) |
| **Esc** | Close the application | [processInput:L864](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L864) |

---

## Control Flow Diagram

> All callbacks are registered at [assignment.cpp:L391–L394](file:///d:/Academics/Graphics/Lab3_assignment/assignment.cpp#L391-L394).

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           INPUT LOOP                                    │
│                                                                         │
│  processInput() (L863)        ← Continuous keys (every frame)           │
│  ├── WASD (L871–L883)         → Bus driving (speed, steering)           │
│  ├── Space/Ctrl (L900–L913)   → Hover up/down (altitude)               │
│  ├── Arrow keys (L922–L927)   → Free camera movement                   │
│  ├── Shift (L920)             → Speed boost (2.5×)                      │
│  └── F hold (L930–L939)       → Orbit around bus                        │
│                                                                         │
│  key_callback() (L947)        ← Single-press keys (discrete)           │
│  ├── V (L952), K (L1019)      → Camera / driving mode                  │
│  ├── M (L961)                 → Mouse capture toggle                    │
│  ├── T (L970), 8 (L974)      → Texture mode / wrap mode                │
│  ├── 9 (L979), 0 (L984)      → Filter mode / textures on-off           │
│  ├── 1-7 (L999–L1013)        → Lighting toggles                        │
│  ├── B (L1016), G (L1017)    → Bus door / fan                          │
│  └── Tab (L1031)              → Print status                            │
│                                                                         │
│  mouse_callback() (L826)      ← Mouse movement                         │
│  └── dx, dy → cameraYaw, cameraPitch                                   │
│                                                                         │
│  scroll_callback() (L854)     ← Mouse wheel                            │
│  └── yoffset → cameraFOV (zoom 15°–90°)                                │
└─────────────────────────────────────────────────────────────────────────┘
```
