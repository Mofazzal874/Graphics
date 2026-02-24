#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <cmath>
#include "Shader.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// CUBE CLASS  (vertex: pos3 + normal3 + texcoord2 = 8 floats)
// ============================================================================
class Cube {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init() {
        if (initialized) return;
        // 6 faces, 2 triangles each, 3 verts per tri = 36 vertices
        // Each vertex: x,y,z, nx,ny,nz, s,t
        float vertices[] = {
            // Front face (normal 0,0,1)
            -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,
             0.5f,-0.5f, 0.5f,  0,0,1,  1,0,
             0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
             0.5f, 0.5f, 0.5f,  0,0,1,  1,1,
            -0.5f, 0.5f, 0.5f,  0,0,1,  0,1,
            -0.5f,-0.5f, 0.5f,  0,0,1,  0,0,
            // Back face (normal 0,0,-1)
            -0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,
             0.5f, 0.5f,-0.5f,  0,0,-1,  0,1,
             0.5f,-0.5f,-0.5f,  0,0,-1,  0,0,
             0.5f, 0.5f,-0.5f,  0,0,-1,  0,1,
            -0.5f,-0.5f,-0.5f,  0,0,-1,  1,0,
            -0.5f, 0.5f,-0.5f,  0,0,-1,  1,1,
            // Left face (normal -1,0,0)
            -0.5f, 0.5f, 0.5f,  -1,0,0,  1,1,
            -0.5f, 0.5f,-0.5f,  -1,0,0,  0,1,
            -0.5f,-0.5f,-0.5f,  -1,0,0,  0,0,
            -0.5f,-0.5f,-0.5f,  -1,0,0,  0,0,
            -0.5f,-0.5f, 0.5f,  -1,0,0,  1,0,
            -0.5f, 0.5f, 0.5f,  -1,0,0,  1,1,
            // Right face (normal 1,0,0)
             0.5f, 0.5f, 0.5f,   1,0,0,  0,1,
             0.5f,-0.5f,-0.5f,   1,0,0,  1,0,
             0.5f, 0.5f,-0.5f,   1,0,0,  1,1,
             0.5f,-0.5f,-0.5f,   1,0,0,  1,0,
             0.5f, 0.5f, 0.5f,   1,0,0,  0,1,
             0.5f,-0.5f, 0.5f,   1,0,0,  0,0,
            // Top face (normal 0,1,0)
            -0.5f, 0.5f,-0.5f,  0,1,0,  0,0,
            -0.5f, 0.5f, 0.5f,  0,1,0,  0,1,
             0.5f, 0.5f, 0.5f,  0,1,0,  1,1,
             0.5f, 0.5f, 0.5f,  0,1,0,  1,1,
             0.5f, 0.5f,-0.5f,  0,1,0,  1,0,
            -0.5f, 0.5f,-0.5f,  0,1,0,  0,0,
            // Bottom face (normal 0,-1,0)
            -0.5f,-0.5f,-0.5f,  0,-1,0,  0,1,
             0.5f,-0.5f,-0.5f,  0,-1,0,  1,1,
             0.5f,-0.5f, 0.5f,  0,-1,0,  1,0,
             0.5f,-0.5f, 0.5f,  0,-1,0,  1,0,
            -0.5f,-0.5f, 0.5f,  0,-1,0,  0,0,
            -0.5f,-0.5f,-0.5f,  0,-1,0,  0,1,
        };
        vertexCount = 36;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // position
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // texcoord
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setVec3("objectColor", color);
        shader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void cleanup() {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
        }
    }
};

// ============================================================================
// CYLINDER CLASS - Parametric generation (pos3 + normal3 + texcoord2)
// ============================================================================
class Cylinder {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init(int sectors = 36) {
        if (initialized) return;
        std::vector<float> vertices;
        float halfH = 0.5f;
        float sectorStep = 2.0f * (float)M_PI / sectors;

        // --- Side surface ---
        for (int i = 0; i < sectors; i++) {
            float a0 = i * sectorStep;
            float a1 = (i + 1) * sectorStep;
            float x0 = cos(a0), z0 = sin(a0);
            float x1 = cos(a1), z1 = sin(a1);
            float u0 = (float)i / sectors;
            float u1 = (float)(i + 1) / sectors;

            // Triangle 1
            vertices.insert(vertices.end(), {x0, -halfH, z0, x0, 0, z0, u0, 0.0f});
            vertices.insert(vertices.end(), {x1, -halfH, z1, x1, 0, z1, u1, 0.0f});
            vertices.insert(vertices.end(), {x1,  halfH, z1, x1, 0, z1, u1, 1.0f});
            // Triangle 2
            vertices.insert(vertices.end(), {x1,  halfH, z1, x1, 0, z1, u1, 1.0f});
            vertices.insert(vertices.end(), {x0,  halfH, z0, x0, 0, z0, u0, 1.0f});
            vertices.insert(vertices.end(), {x0, -halfH, z0, x0, 0, z0, u0, 0.0f});
        }

        // --- Top cap ---
        for (int i = 0; i < sectors; i++) {
            float a0 = i * sectorStep;
            float a1 = (i + 1) * sectorStep;
            float x0 = cos(a0), z0 = sin(a0);
            float x1 = cos(a1), z1 = sin(a1);
            // Planar UV for cap
            vertices.insert(vertices.end(), {0, halfH, 0,  0, 1, 0,  0.5f, 0.5f});
            vertices.insert(vertices.end(), {x0, halfH, z0, 0, 1, 0, 0.5f + 0.5f * x0, 0.5f + 0.5f * z0});
            vertices.insert(vertices.end(), {x1, halfH, z1, 0, 1, 0, 0.5f + 0.5f * x1, 0.5f + 0.5f * z1});
        }

        // --- Bottom cap ---
        for (int i = 0; i < sectors; i++) {
            float a0 = i * sectorStep;
            float a1 = (i + 1) * sectorStep;
            float x0 = cos(a0), z0 = sin(a0);
            float x1 = cos(a1), z1 = sin(a1);
            vertices.insert(vertices.end(), {0, -halfH, 0,  0, -1, 0,  0.5f, 0.5f});
            vertices.insert(vertices.end(), {x1, -halfH, z1, 0, -1, 0, 0.5f + 0.5f * x1, 0.5f + 0.5f * z1});
            vertices.insert(vertices.end(), {x0, -halfH, z0, 0, -1, 0, 0.5f + 0.5f * x0, 0.5f + 0.5f * z0});
        }

        vertexCount = (int)vertices.size() / 8;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setVec3("objectColor", color);
        shader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void cleanup() {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
        }
    }
};

// ============================================================================
// TORUS CLASS - Parametric generation (pos3 + normal3 + texcoord2)
// ============================================================================
class Torus {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init(float mainRadius = 0.4f, float tubeRadius = 0.1f,
              int mainSegments = 24, int tubeSegments = 12)
    {
        if (initialized) return;
        std::vector<float> vertices;

        for (int i = 0; i < mainSegments; i++) {
            float theta0 = 2.0f * (float)M_PI * i / mainSegments;
            float theta1 = 2.0f * (float)M_PI * (i + 1) / mainSegments;
            float u0 = (float)i / mainSegments;
            float u1 = (float)(i + 1) / mainSegments;

            for (int j = 0; j < tubeSegments; j++) {
                float phi0 = 2.0f * (float)M_PI * j / tubeSegments;
                float phi1 = 2.0f * (float)M_PI * (j + 1) / tubeSegments;
                float v0 = (float)j / tubeSegments;
                float v1 = (float)(j + 1) / tubeSegments;

                // 4 corners of the quad
                auto torusVert = [&](float theta, float phi, float u, float v) {
                    float x = (mainRadius + tubeRadius * cos(phi)) * cos(theta);
                    float y = tubeRadius * sin(phi);
                    float z = (mainRadius + tubeRadius * cos(phi)) * sin(theta);
                    float nx = cos(phi) * cos(theta);
                    float ny = sin(phi);
                    float nz = cos(phi) * sin(theta);
                    vertices.insert(vertices.end(), {x, y, z, nx, ny, nz, u, v});
                };

                // Triangle 1
                torusVert(theta0, phi0, u0, v0);
                torusVert(theta1, phi0, u1, v0);
                torusVert(theta1, phi1, u1, v1);
                // Triangle 2
                torusVert(theta1, phi1, u1, v1);
                torusVert(theta0, phi1, u0, v1);
                torusVert(theta0, phi0, u0, v0);
            }
        }

        vertexCount = (int)vertices.size() / 8;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setVec3("objectColor", color);
        shader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void cleanup() {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
        }
    }
};

// ============================================================================
// SPHERE CLASS - UV Sphere (pos3 + normal3 + texcoord2)
// ============================================================================
class Sphere {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init(int stacks = 20, int sectors = 36) {
        if (initialized) return;
        std::vector<float> vertices;
        float radius = 0.5f;

        for (int i = 0; i < stacks; i++) {
            float phi0 = (float)M_PI * i / stacks;
            float phi1 = (float)M_PI * (i + 1) / stacks;
            float v0 = (float)i / stacks;
            float v1 = (float)(i + 1) / stacks;

            for (int j = 0; j < sectors; j++) {
                float theta0 = 2.0f * (float)M_PI * j / sectors;
                float theta1 = 2.0f * (float)M_PI * (j + 1) / sectors;
                float u0 = (float)j / sectors;
                float u1 = (float)(j + 1) / sectors;

                auto sphereVert = [&](float phi, float theta, float u, float v) {
                    float x = radius * sin(phi) * cos(theta);
                    float y = radius * cos(phi);
                    float z = radius * sin(phi) * sin(theta);
                    float nx = sin(phi) * cos(theta);
                    float ny = cos(phi);
                    float nz = sin(phi) * sin(theta);
                    vertices.insert(vertices.end(), {x, y, z, nx, ny, nz, u, v});
                };

                // Triangle 1
                sphereVert(phi0, theta0, u0, v0);
                sphereVert(phi1, theta0, u0, v1);
                sphereVert(phi1, theta1, u1, v1);
                // Triangle 2
                sphereVert(phi1, theta1, u1, v1);
                sphereVert(phi0, theta1, u1, v0);
                sphereVert(phi0, theta0, u0, v0);
            }
        }

        vertexCount = (int)vertices.size() / 8;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setVec3("objectColor", color);
        shader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void cleanup() {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
        }
    }
};

// ============================================================================
// CONE CLASS - Parametric generation (pos3 + normal3 + texcoord2)
// ============================================================================
class Cone {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init(int sectors = 36) {
        if (initialized) return;
        std::vector<float> vertices;
        float halfH = 0.5f;
        float sectorStep = 2.0f * (float)M_PI / sectors;
        float slopeLen = sqrt(1.0f + 1.0f); // for normal calculation

        // --- Side surface ---
        for (int i = 0; i < sectors; i++) {
            float a0 = i * sectorStep;
            float a1 = (i + 1) * sectorStep;
            float x0 = cos(a0), z0 = sin(a0);
            float x1 = cos(a1), z1 = sin(a1);
            float u0 = (float)i / sectors;
            float u1 = (float)(i + 1) / sectors;

            // Normal for cone side: pointing outward and upward
            // The cone goes from radius=1 at y=-0.5 to radius=0 at y=+0.5
            float ny = 1.0f / slopeLen;
            float nxz = 1.0f / slopeLen;

            float nx0 = nxz * x0, nz0 = nxz * z0;
            float nx1 = nxz * x1, nz1 = nxz * z1;
            // Average normal for apex
            float nxa = nxz * cos((a0 + a1) * 0.5f);
            float nza = nxz * sin((a0 + a1) * 0.5f);

            // Apex (top)
            vertices.insert(vertices.end(), {0.0f, halfH, 0.0f, nxa, ny, nza, (u0 + u1) * 0.5f, 1.0f});
            // Base vertices
            vertices.insert(vertices.end(), {x0, -halfH, z0, nx0, ny, nz0, u0, 0.0f});
            vertices.insert(vertices.end(), {x1, -halfH, z1, nx1, ny, nz1, u1, 0.0f});
        }

        // --- Bottom cap ---
        for (int i = 0; i < sectors; i++) {
            float a0 = i * sectorStep;
            float a1 = (i + 1) * sectorStep;
            float x0 = cos(a0), z0 = sin(a0);
            float x1 = cos(a1), z1 = sin(a1);
            vertices.insert(vertices.end(), {0, -halfH, 0,  0, -1, 0,  0.5f, 0.5f});
            vertices.insert(vertices.end(), {x1, -halfH, z1, 0, -1, 0, 0.5f + 0.5f * x1, 0.5f + 0.5f * z1});
            vertices.insert(vertices.end(), {x0, -halfH, z0, 0, -1, 0, 0.5f + 0.5f * x0, 0.5f + 0.5f * z0});
        }

        vertexCount = (int)vertices.size() / 8;
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glBindVertexArray(0);
        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setVec3("objectColor", color);
        shader.setMat4("model", model);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    void cleanup() {
        if (initialized) {
            glDeleteVertexArrays(1, &VAO);
            glDeleteBuffers(1, &VBO);
            initialized = false;
        }
    }
};

#endif
