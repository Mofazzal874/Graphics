#ifndef MENGERSPONGE_H
#define MENGERSPONGE_H

#include "Globals.h"

// ============================================================================
// MENGER SPONGE FRACTAL
// ============================================================================

static inline bool mengerKept(int x, int y, int z) {
    int centers = (x == 1 ? 1 : 0) + (y == 1 ? 1 : 0) + (z == 1 ? 1 : 0);
    return centers <= 1;
}

inline void buildMengerSponge(int iterations) {
    mengerCubes.clear();
    std::vector<MengerCube> current;
    current.push_back({ glm::vec3(0.0f), 1.0f });

    for (int it = 0; it < iterations; it++) {
        std::vector<MengerCube> next;
        next.reserve(current.size() * 20);
        for (const auto& c : current) {
            float s = c.size / 3.0f;
            for (int x = 0; x < 3; x++)
            for (int y = 0; y < 3; y++)
            for (int z = 0; z < 3; z++) {
                if (!mengerKept(x, y, z)) continue;
                glm::vec3 off = c.offset + glm::vec3((x - 1) * s, (y - 1) * s, (z - 1) * s);
                next.push_back({ off, s });
            }
        }
        current.swap(next);
    }
    mengerCubes = std::move(current);
}

inline void initMengerInstancing() {
    std::vector<glm::vec4> instData;
    instData.reserve(mengerCubes.size());
    for (const auto& c : mengerCubes)
        instData.push_back(glm::vec4(c.offset, c.size));

    glGenVertexArrays(1, &mengerVAO);
    glBindVertexArray(mengerVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bus.cube.VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &mengerInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mengerInstVBO);
    glBufferData(GL_ARRAY_BUFFER, instData.size() * sizeof(glm::vec4),
                 instData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    glBindVertexArray(0);

    glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 1.0f);
}

inline void drawMengerSponge(const Shader& sh, const glm::mat4& worldTransform,
                             glm::vec3 baseColor, bool emissive)
{
    if (emissive) sh.setBool("isEmissive", true);
    sh.setVec3("objectColor", baseColor);
    sh.setMat4("model", worldTransform);

    glBindVertexArray(mengerVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, bus.cube.vertexCount,
                          (GLsizei)mengerCubes.size());
    glBindVertexArray(0);

    if (emissive) sh.setBool("isEmissive", false);
}

#endif
