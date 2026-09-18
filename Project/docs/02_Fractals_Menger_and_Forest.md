# 02 — Fractals: Menger Sponge & Fractal Forest

The project ships **two distinct fractals**, each demonstrating a different recursion strategy and a different way of pushing the result to the GPU:

| Fractal       | Recursion type           | Iterations | Final element count           | Render method                |
| ------------- | ------------------------ | ---------- | ----------------------------- | ---------------------------- |
| Menger sponge | Spatial subdivision      | 4          | **160 000 sub-cubes**         | Single instanced draw call   |
| Forest tree   | Branching L-system style | depth 5    | **~363 branches/tree**, 12 leaves per tip | Per-tile instanced draw |

Both store one *base mesh* on the GPU and feed an **instance buffer** of per-cube/per-branch transforms — the fractal explosion only ever lives in a single VBO, never in CPU draw loops.

---

## 1. Menger Sponge

**Source:** [MengerSponge.h](../MengerSponge.h)
**Triggered from:** [assignment.cpp:248-251](../assignment.cpp#L248-L251)

### 1.1 Construction rule

Start with a unit cube. At each iteration:

1. Subdivide the cube into a `3 × 3 × 3` grid of 27 sub-cubes (each 1/3 the size of the parent).
2. Throw away the sub-cube at the **center of every face** plus the **center of the whole cube** — that is the 7 cubes whose grid coordinates have *2 or more* coordinates equal to `1`.
3. Keep the remaining **20** sub-cubes.

That "keep" predicate is exactly this:

```cpp
static inline bool mengerKept(int x, int y, int z) {
    int centers = (x==1) + (y==1) + (z==1);
    return centers <= 1;       // keep only if at most one axis is centered
}
```
([MengerSponge.h:10-13](../MengerSponge.h#L10-L13))

### 1.2 Iteration count and growth

After `n` iterations the sponge contains `20ⁿ` sub-cubes:

| iterations | cube count |
| ---------- | ---------- |
| 0          | 1          |
| 1          | 20         |
| 2          | 400        |
| 3          | 8 000      |
| 4          | **160 000** |

The project calls `buildMengerSponge(4)` ([assignment.cpp:249](../assignment.cpp#L249)) — so after the build we have **160 000** unit cubes, each at a precomputed `(offset, size)` pair stored in `mengerCubes`. The geometry-build is done in [MengerSponge.h:15-36](../MengerSponge.h#L15-L36):

```cpp
for each existing cube:
    s = c.size / 3
    for x,y,z in {0,1,2}^3:
        if mengerKept(x,y,z):
            push child cube at offset + ((x-1)s, (y-1)s, (z-1)s)
```

The fractal dimension of the Menger sponge is `log(20) / log(3) ≈ 2.7268` — it fills more than a surface but less than a volume.

### 1.3 GPU representation

We do **not** allocate 160 000 separate draw calls. Instead:

1. The base cube geometry is reused from `bus.cube.VBO` (36 vertices). [MengerSponge.h:48](../MengerSponge.h#L48)
2. A new instance VBO `mengerInstVBO` holds 160 000 `vec4` entries — each is `(offset.x, offset.y, offset.z, size)`.
3. Attribute location 3 reads that vec4 with `glVertexAttribDivisor(3, 1)` so each instance gets its own copy. [MengerSponge.h:60-63](../MengerSponge.h#L60-L63)
4. Rendering is exactly **one draw call** per checkpoint:

```cpp
glDrawArraysInstanced(GL_TRIANGLES, 0, bus.cube.vertexCount, 160000);
```

The vertex shader uses attribute 3 to translate+scale the unit cube into world position. So 160 000 cubes × 36 verts = **5.76 million vertices** are processed per draw, but the CPU just submits one command.

### 1.4 Where the sponges live in the world

The sponges are scattered along the road as **collectible "Menger coins"**, drawn with an emissive cyan-magenta tint. Each one has an AABB collision check against the bus; on overlap the sponge is added to `collectedSponges`, the player gains score (`awardScore(+5)`), and that index is suppressed from the draw loop.

---

## 2. Fractal Forest (Recursive Tree)

**Source:** [Forest.h](../Forest.h)
**Triggered from:** [assignment.cpp:253+](../assignment.cpp#L253) via `buildForest(); initForestInstancing();`

This is a **recursive branch tree** — closer to an L-system than a Menger fractal.

### 2.1 Recursion rule

In `bakeFractalBranch(...)` ([Forest.h:62-113](../Forest.h#L62-L113)), each call:

1. **Emits** a stretched cylinder representing the current branch (a `1×1×1` cylinder scaled to `(radius, length, radius)` and translated half-way up so the base sits on the parent's tip).
2. If `depth == 0`, instead of recursing it spawns **`LEAVES_PER_TIP = 12` jittered leaf billboards** at the tip ([Forest.h:75-91](../Forest.h#L75-L91)).
3. Otherwise spawns **`N = 3` child branches**, each:
   * yawed by `i · 120°` with a ±17.5° random jitter,
   * pitched outward by `22°–44°`,
   * shrunk in length by factor `0.68–0.78`,
   * shrunk in radius by factor `0.62–0.70`.

### 2.2 Counts

Branches per tree (geometric series with branching factor 3 over depth 5):

```
depth: 0  1   2   3    4    5
count: 1  3   9  27   81  243   →  total = 364 branches
```

Plus **12 leaves at every depth-5 tip** = `243 × 12 = 2 916 leaves` per tree.

### 2.3 How many trees

`buildForest()` ([Forest.h:115-155](../Forest.h#L115-L155)) places trees on **both sides of the road**:

```
TREES_PER_ROW = 12
ROWS_PER_SIDE = 3
sides         = 2  (left & right)
trees per tile = 12 × 3 × 2 = 72
```

with one tile spanning `FOREST_TILE_LEN = 200.0` units along the X axis. So the forest **per tile** holds:

```
72 trees × 364 branches = 26 208 branch instances
72 trees × 2 916 leaves = 209 952 leaf instances
```

These are *all* baked into two GPU instance buffers and the renderer draws each tile in **two instanced calls** (one for branches, one for leaves) — see [Forest.h:241-269](../Forest.h#L241-L269). The world tiles infinitely along X by re-issuing those two calls at every visible `FOREST_TILE_LEN` offset within `±400` of the bus.

### 2.4 Branch geometry — low-poly noisy cylinder

The branch base mesh is a **9-sided cylinder** built once in `buildForestBranchGeometry()` ([Forest.h:22-60](../Forest.h#L22-L60)), but each side has a **random radius jitter** of `0.82 + 0.36·rand`, so trunks look organic instead of clinical:

```cpp
for (int i = 0; i < SIDES; i++)
    radii[i] = 0.82f + 0.36f * (cityHash(i+1, 991) % 1000)/1000.0f;
```

Per-side normals are computed from the *midpoint* angle of each face, so flat-shading with hard edges falls out for free.

### 2.5 Leaf geometry — crossed billboards

Each leaf is **two perpendicular quads** (XY-plane and YZ-plane) glued at the origin ([Forest.h:179-195](../Forest.h#L179-L195)). This gives a "+ shape" so the leaf has volume from any view angle. The fragment shader uses **alpha-test cutout** mode (see Lighting doc, mode 1 with `alphaTest = true`) so that the texture's transparent pixels are discarded — and `gl_FrontFacing` flips the normal so back faces of the crossed quads still receive correct lighting ([shader.frag:151-155](../shader.frag#L151-L155)).

### 2.6 Why the math gives "tree-shaped" trees

The two key choices are:

* **Pitch angle 22°–44°** — wide enough to spread out, narrow enough to still grow upward.
* **Length scale ~0.73 average** — the trunk being ~3.7× the length of the average tip means most of the tree's height comes from the trunk, which matches real trees.

Hashing-derived randomness (`cityHash`) is used everywhere instead of `rand()`, so the forest is **deterministic** — every run produces the exact same trees. That's important because the collision boxes and the rendered geometry must agree, and we can't afford to re-bake the forest every frame.

---

## 3. Comparing the two fractals

|                      | Menger sponge          | Recursive tree           |
| -------------------- | ---------------------- | ------------------------ |
| **Recursion shape**  | Each cube → 20 children | Each branch → 3 children |
| **Termination rule** | Fixed iteration count `n=4` | Fixed depth `d=5`, then leaves |
| **Output element**   | Axis-aligned cube      | Oriented cylinder + leaves |
| **GPU layout**       | One vec4 per instance  | One mat4 per instance    |
| **Determinism**      | Pure (no rand)         | Hash-based (deterministic) |
| **What it teaches**  | Spatial subdivision    | L-system / branching     |

Together they cover the two big families of geometric fractals.
