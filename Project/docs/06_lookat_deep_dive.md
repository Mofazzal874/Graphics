# Deep Dive: How `myLookAt` Works

This document explains the **vector mathematics** behind the custom `myLookAt` function in `assignment.cpp`. It breaks down `normalize`, `cross`, and `dot` products and how they build a camera view matrix.

---

## 1. The Core Concepts

### 1.1 `glm::normalize(v)` — Making a Unit Vector
**What it does:** Takes a vector and shrinks/stretches it so its length becomes exactly **1.0**, while keeping the same direction.
**Why we need it:** In a rotation matrix, the axes (Right, Up, Forward) must be "unit vectors" (length = 1). If they are not length 1, the world will look squashed or stretched.

**Example:**
- Vector `v = (0, 3, 4)` has length $\sqrt{0^2 + 3^2 + 4^2} = \sqrt{25} = 5$.
- `normalize(v)` divides each component by 5: `(0, 0.6, 0.8)`.
- The new length is $\sqrt{0^2 + 0.6^2 + 0.8^2} = \sqrt{0.36 + 0.64} = 1$.

### 1.2 `glm::cross(a, b)` — Finding a Perpendicular Vector
**What it does:** Takes two vectors `a` and `b` and produces a third vector that is **perpendicular (90°)** to *both* of them.
**Why we need it:** We know the camera's "Forward" direction and the world's "Up" direction. The "Right" direction must be perpendicular to both. The cross product gives us exactly that.

**Right-Hand Rule:**
- Point index finger along `a` (Forward)
- Point middle finger along `b` (Up)
- Your thumb points along `cross(a, b)` (Right)

### 1.3 `glm::dot(a, b)` — Measuring Alignment
**What it does:** Multiplies corresponding components: $a.x \times b.x + a.y \times b.y + a.z \times b.z$.
**Visual meaning:** Measures how much two vectors point in the same direction.
- If aligned: Positive value
- If perpendicular (90°): **Zero**
- If opposite: Negative value
**Why we need it:** Used in the matrix translation part to project the camera's position onto the new camera axes.

---

## 2. Step-by-Step Walkthrough of `myLookAt`

### Inputs
```cpp
glm::vec3 eye    = (0, 5, 20);   // Camera position
glm::vec3 center = (0, 0, 0);    // Looking at origin
glm::vec3 up     = (0, 1, 0);    // World "Up" is Y-axis
```

### Step 1: Calculate the Forward Axis (`f`)
```cpp
glm::vec3 f = glm::normalize(center - eye);
```
- `center - eye` gives the vector pointing **FROM** the camera **TO** the target.
- `(0,0,0) - (0,5,20) = (0, -5, -20)`
- We normalize it to get a pure direction.
- Note: In standard OpenGL `lookAt`, the forward axis is usually calculated as `eye - center` (positive Z points out of screen). My implementation uses `center - eye` (forward) and negates it in the matrix, which achieves the same standard result (Camera looks down -Z).

### Step 2: Calculate the Right Axis (`s`)
```cpp
glm::vec3 s = glm::normalize(glm::cross(f, up));
```
- We want a vector pointing to the **Right** of the camera.
- `f` points Forward. `up` points Up.
- `cross(Forward, Up)` gives a vector perpendicular to both → **Right**.
- We normalize it just to be safe (though if f and up are perpendicular unit vectors, it's already unit length).

### Step 3: Calculate the True Up Axis (`u`)
```cpp
glm::vec3 u = glm::cross(s, f);
```
- The world "Up" `(0,1,0)` is just a suggestion. If you look down, your camera's "top" isn't pointing straight up at the sky anymore; it tilts.
- This step recalculates the **actual** Up vector relative to the camera.
- `cross(Right, Forward)` → **True Up**.
- No normalization needed because `s` and `f` are already perpendicular unit vectors.

Now we have 3 orthogonal (90° apart) unit vectors:
- `s` (Right) = X axis for camera
- `u` (Up)    = Y axis for camera
- `-f` (Back) = Z axis for camera (OpenGL conventions: camera looks down -Z)

---

## 3. Constructing the Matrix

We build a 4×4 matrix that does two things:
1. **Translates** the world so the camera moves to `(0,0,0)`.
2. **Rotates** the world so the camera points down the -Z axis.

```cpp
glm::mat4 result(1.0f);
// Rotation Part (Top-Left 3x3)
result[0][0] = s.x;   result[1][0] = s.y;   result[2][0] = s.z;  // Row 0: Right
result[0][1] = u.x;   result[1][1] = u.y;   result[2][1] = u.z;  // Row 1: Up
result[0][2] = -f.x;  result[1][2] = -f.y;  result[2][2] = -f.z; // Row 2: Back (-Forward)

// Translation Part (Bottom Row / Last Column)
result[3][0] = -glm::dot(s, eye);
result[3][1] = -glm::dot(u, eye);
result[3][2] =  glm::dot(f, eye);  // effectively -dot(-f, eye)
```

The dot products calculate "how far is the camera from the origin along this axis?".
- `dot(s, eye)`: Position along Right axis.
- We negate it because if the camera moved **Right** by 5, the world must move **Left** by 5 to compensate.
