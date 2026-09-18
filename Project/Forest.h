#ifndef FOREST_H
#define FOREST_H

#include "Globals.h"

// ============================================================================
// FRACTAL FOREST
// ============================================================================

// Helper: wire 4 vec4 attribs (locations 4-7) as a per-instance mat4
static inline void setupInstanceMat4Attribs() {
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4),
                              (void*)(i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 1);
    }
}

// Build a low-poly, flat-shaded, radius-jittered cylinder for forest branches.
inline void buildForestBranchGeometry() {
    const int SIDES = 9;
    float radii[SIDES];
    for (int i = 0; i < SIDES; i++) {
        radii[i] = 0.82f + 0.36f * ((cityHash(i + 1, 991) % 1000) / 1000.0f);
    }

    std::vector<float> v;
    v.reserve(SIDES * 6 * 8);
    for (int i = 0; i < SIDES; i++) {
        float a0 = (2.0f * (float)M_PI * i)       / SIDES;
        float a1 = (2.0f * (float)M_PI * (i + 1)) / SIDES;
        float r0 = radii[i];
        float r1 = radii[(i + 1) % SIDES];
        float x0 = cosf(a0) * r0, z0 = sinf(a0) * r0;
        float x1 = cosf(a1) * r1, z1 = sinf(a1) * r1;

        float midA = (a0 + a1) * 0.5f;
        float nx = cosf(midA), nz = sinf(midA);

        float u0 = (float)i / SIDES;
        float u1 = (float)(i + 1) / SIDES;

        float quad[] = {
            x0, -0.5f, z0,  nx, 0.0f, nz,  u0, 0.0f,
            x1, -0.5f, z1,  nx, 0.0f, nz,  u1, 0.0f,
            x1,  0.5f, z1,  nx, 0.0f, nz,  u1, 1.0f,
            x1,  0.5f, z1,  nx, 0.0f, nz,  u1, 1.0f,
            x0,  0.5f, z0,  nx, 0.0f, nz,  u0, 1.0f,
            x0, -0.5f, z0,  nx, 0.0f, nz,  u0, 0.0f,
        };
        v.insert(v.end(), quad, quad + 48);
    }
    forest.branchVertCount = (int)(v.size() / 8);

    glGenBuffers(1, &forest.branchGeomVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchGeomVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
}

static inline void bakeFractalBranch(std::vector<glm::mat4>& branches,
                              std::vector<glm::mat4>& leaves,
                              const glm::mat4& base,
                              float length, float radius,
                              int depth, unsigned int seed)
{
    glm::mat4 m = glm::translate(base, glm::vec3(0.0f, length * 0.5f, 0.0f));
    m = glm::scale(m, glm::vec3(radius, length, radius));
    branches.push_back(m);

    glm::mat4 tip = glm::translate(base, glm::vec3(0.0f, length, 0.0f));

    if (depth <= 0) {
        const int LEAVES_PER_TIP = 12;
        for (int k = 0; k < LEAVES_PER_TIP; k++) {
            unsigned int ks = seed * 131u + (unsigned int)(k * 7919u);
            float ox = ((cityHash(ks, 1) % 1000) / 1000.0f - 0.5f) * length * 1.6f;
            float oy = ((cityHash(ks, 2) % 1000) / 1000.0f) * length * 1.4f;
            float oz = ((cityHash(ks, 3) % 1000) / 1000.0f - 0.5f) * length * 1.6f;
            float scl = length * (0.45f + 0.35f * ((cityHash(ks, 4) % 1000) / 1000.0f));
            float ry  = ((cityHash(ks, 5) % 1000) / 1000.0f) * 360.0f;
            float rx  = ((cityHash(ks, 6) % 1000) / 1000.0f) * 60.0f;

            glm::mat4 lm = glm::translate(tip, glm::vec3(ox, oy, oz));
            lm = glm::rotate(lm, glm::radians(ry), glm::vec3(0, 1, 0));
            lm = glm::rotate(lm, glm::radians(rx), glm::vec3(1, 0, 0));
            lm = glm::scale(lm, glm::vec3(scl, scl * 0.6f, scl));
            leaves.push_back(lm);
        }
        return;
    }

    const int N = 3;
    for (int i = 0; i < N; i++) {
        unsigned int cs = seed * 1664525u + (unsigned int)(i * 1013904223u + depth * 2654435761u);
        float r1 = (cityHash(cs, 1) % 1000) / 1000.0f;
        float r2 = (cityHash(cs, 2) % 1000) / 1000.0f;
        float r3 = (cityHash(cs, 3) % 1000) / 1000.0f;

        float yaw   = (i * (360.0f / N)) + (r1 - 0.5f) * 35.0f;
        float pitch = 22.0f + r2 * 22.0f;
        float lenScale = 0.68f + r3 * 0.10f;
        float radScale = 0.62f + r3 * 0.08f;

        glm::mat4 child = glm::rotate(tip,   glm::radians(yaw),   glm::vec3(0, 1, 0));
        child           = glm::rotate(child, glm::radians(pitch), glm::vec3(1, 0, 0));

        bakeFractalBranch(branches, leaves, child,
                          length * lenScale, radius * radScale,
                          depth - 1, cs);
    }
}

inline void buildForest() {
    forest.branchInstances.clear();
    forest.leafInstances.clear();

    const int TREES_PER_ROW = 12;
    const int ROWS_PER_SIDE = 3;
    const float bandStart = BUILDING_ZONE_END + 4.0f;
    const float bandEnd   = GRASS_WIDTH - 6.0f;
    const float bandDepth = bandEnd - bandStart;

    for (int side = -1; side <= 1; side += 2) {
        for (int row = 0; row < ROWS_PER_SIDE; row++) {
            for (int i = 0; i < TREES_PER_ROW; i++) {
                unsigned int s = (unsigned int)(((i * 31 + row * 7919) * 2 + (side + 1)) * 374761393u + 7u);
                float r1 = (cityHash(s, 11) % 1000) / 1000.0f;
                float r2 = (cityHash(s, 12) % 1000) / 1000.0f;
                float r3 = (cityHash(s, 13) % 1000) / 1000.0f;
                float r4 = (cityHash(s, 14) % 1000) / 1000.0f;

                float xStep   = FOREST_TILE_LEN / TREES_PER_ROW;
                float xOffset = (row % 2 == 0) ? 0.0f : xStep * 0.5f;
                float tx = (i + 0.15f + r1 * 0.7f) * xStep + xOffset;

                float rowFrac = (row + 0.2f + r2 * 0.6f) / ROWS_PER_SIDE;
                float tz = side * (bandStart + rowFrac * bandDepth);

                float scale = 0.9f + r3 * 0.6f;
                float trunkLen = 3.0f * scale;
                float trunkRad = 0.36f * scale;

                glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(tx, 0.0f, tz));
                base = glm::rotate(base, glm::radians((r1 - 0.5f) * 10.0f), glm::vec3(1, 0, 0));
                base = glm::rotate(base, glm::radians((r4 - 0.5f) * 10.0f), glm::vec3(0, 0, 1));
                base = glm::rotate(base, glm::radians(r2 * 360.0f),         glm::vec3(0, 1, 0));

                bakeFractalBranch(forest.branchInstances, forest.leafInstances,
                                  base, trunkLen, trunkRad, 5, s);
            }
        }
    }
}

inline void initForestInstancing() {
    // ---- Branch VAO ----
    buildForestBranchGeometry();
    glGenVertexArrays(1, &forest.branchVAO);
    glBindVertexArray(forest.branchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchGeomVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &forest.branchInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 forest.branchInstances.size() * sizeof(glm::mat4),
                 forest.branchInstances.data(), GL_STATIC_DRAW);
    setupInstanceMat4Attribs();
    glBindVertexArray(0);

    // ---- Leaf VAO ----
    float leafQuadVerts[] = {
        // pos              normal       uv
        // Quad 1 (XY plane, normal +Z)
        -0.5f,-0.5f, 0.0f,  0,0,1,  0,0,
         0.5f,-0.5f, 0.0f,  0,0,1,  1,0,
         0.5f, 0.5f, 0.0f,  0,0,1,  1,1,
         0.5f, 0.5f, 0.0f,  0,0,1,  1,1,
        -0.5f, 0.5f, 0.0f,  0,0,1,  0,1,
        -0.5f,-0.5f, 0.0f,  0,0,1,  0,0,
        // Quad 2 (YZ plane, normal +X)
         0.0f,-0.5f,-0.5f,  1,0,0,  0,0,
         0.0f,-0.5f, 0.5f,  1,0,0,  1,0,
         0.0f, 0.5f, 0.5f,  1,0,0,  1,1,
         0.0f, 0.5f, 0.5f,  1,0,0,  1,1,
         0.0f, 0.5f,-0.5f,  1,0,0,  0,1,
         0.0f,-0.5f,-0.5f,  1,0,0,  0,0,
    };
    glGenBuffers(1, &forest.leafQuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(leafQuadVerts), leafQuadVerts, GL_STATIC_DRAW);

    glGenVertexArrays(1, &forest.leafVAO);
    glBindVertexArray(forest.leafVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafQuadVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &forest.leafInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 forest.leafInstances.size() * sizeof(glm::mat4),
                 forest.leafInstances.data(), GL_STATIC_DRAW);
    setupInstanceMat4Attribs();
    glBindVertexArray(0);

    // Identity defaults for instance mat4 attribs (used by NON-instanced draws)
    glVertexAttrib4f(4, 1.0f, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(5, 0.0f, 1.0f, 0.0f, 0.0f);
    glVertexAttrib4f(6, 0.0f, 0.0f, 1.0f, 0.0f);
    glVertexAttrib4f(7, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline void drawForest(const Shader& sh, float busX) {
    const float visibleRange = 400.0f;
    int tStart = (int)floor((busX - visibleRange) / FOREST_TILE_LEN);
    int tEnd   = (int)ceil ((busX + visibleRange) / FOREST_TILE_LEN);

    // ---- BRANCHES (textured bark) ----
    if (texBark != 0) {
        sh.setInt("textureMode", 3);
        sh.setVec2("texScale", glm::vec2(4.0f, 6.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBark);
        sh.setInt("textureSampler", 0);
    } else {
        sh.setInt("textureMode", 0);
    }
    sh.setVec3("objectColor", glm::vec3(0.45f, 0.30f, 0.18f));
    glBindVertexArray(forest.branchVAO);
    for (int ti = tStart; ti <= tEnd; ti++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(ti * FOREST_TILE_LEN, 0.0f, 0.0f));
        sh.setMat4("model", model);
        glDrawArraysInstanced(GL_TRIANGLES, 0,
                              forest.branchVertCount,
                              (GLsizei)forest.branchInstances.size());
    }

    // ---- LEAVES (alpha-cutout textured billboards) ----
    if (texLeaf != 0) {
        sh.setInt("textureMode", 1);
        sh.setBool("alphaTest", true);
        sh.setVec2("texScale", glm::vec2(1.0f, 1.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texLeaf);
        sh.setInt("textureSampler", 0);
        sh.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        glBindVertexArray(forest.leafVAO);
        for (int ti = tStart; ti <= tEnd; ti++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(ti * FOREST_TILE_LEN, 0.0f, 0.0f));
            sh.setMat4("model", model);
            glDrawArraysInstanced(GL_TRIANGLES, 0, 12,
                                  (GLsizei)forest.leafInstances.size());
        }
        sh.setBool("alphaTest", false);
    }

    glBindVertexArray(0);
    sh.setInt("textureMode", 0);
    sh.setVec2("texScale", glm::vec2(1.0f, 1.0f));
}

#endif
