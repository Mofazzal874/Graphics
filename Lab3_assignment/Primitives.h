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
// CUBE CLASS
// ============================================================================
class Cube {
public:
    unsigned int VAO, VBO;
    bool initialized = false;

    void init() {
        if (initialized) return;

        float vertices[] = {
            // positions          // normals
            // Back face
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,
            // Front face
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,
            // Left face
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,
            // Right face
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,
            // Bottom face
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,
            // Top face
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
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
// CYLINDER CLASS - Parametric generation
// ============================================================================
class Cylinder {
public:
    unsigned int VAO, VBO;
    int vertexCount = 0;
    bool initialized = false;

    void init(int sectors = 36) {
        if (initialized) return;

        std::vector<float> vertices;
        float radius = 0.5f;
        float height = 1.0f;
        float halfHeight = height / 2.0f;
        float sectorStep = 2.0f * (float)M_PI / sectors;

        // Side surface
        for (int i = 0; i < sectors; i++) {
            float angle1 = i * sectorStep;
            float angle2 = (i + 1) * sectorStep;

            float x1 = radius * cos(angle1);
            float z1 = radius * sin(angle1);
            float x2 = radius * cos(angle2);
            float z2 = radius * sin(angle2);

            // Normal vectors (pointing outward)
            float nx1 = cos(angle1), nz1 = sin(angle1);
            float nx2 = cos(angle2), nz2 = sin(angle2);

            // Triangle 1
            vertices.insert(vertices.end(), {x1, -halfHeight, z1, nx1, 0.0f, nz1});
            vertices.insert(vertices.end(), {x2, -halfHeight, z2, nx2, 0.0f, nz2});
            vertices.insert(vertices.end(), {x1,  halfHeight, z1, nx1, 0.0f, nz1});
            // Triangle 2
            vertices.insert(vertices.end(), {x2, -halfHeight, z2, nx2, 0.0f, nz2});
            vertices.insert(vertices.end(), {x2,  halfHeight, z2, nx2, 0.0f, nz2});
            vertices.insert(vertices.end(), {x1,  halfHeight, z1, nx1, 0.0f, nz1});
        }

        // Top cap
        for (int i = 0; i < sectors; i++) {
            float angle1 = i * sectorStep;
            float angle2 = (i + 1) * sectorStep;
            float x1 = radius * cos(angle1), z1 = radius * sin(angle1);
            float x2 = radius * cos(angle2), z2 = radius * sin(angle2);

            vertices.insert(vertices.end(), {0.0f, halfHeight, 0.0f, 0.0f, 1.0f, 0.0f});
            vertices.insert(vertices.end(), {x1, halfHeight, z1, 0.0f, 1.0f, 0.0f});
            vertices.insert(vertices.end(), {x2, halfHeight, z2, 0.0f, 1.0f, 0.0f});
        }

        // Bottom cap
        for (int i = 0; i < sectors; i++) {
            float angle1 = i * sectorStep;
            float angle2 = (i + 1) * sectorStep;
            float x1 = radius * cos(angle1), z1 = radius * sin(angle1);
            float x2 = radius * cos(angle2), z2 = radius * sin(angle2);

            vertices.insert(vertices.end(), {0.0f, -halfHeight, 0.0f, 0.0f, -1.0f, 0.0f});
            vertices.insert(vertices.end(), {x2, -halfHeight, z2, 0.0f, -1.0f, 0.0f});
            vertices.insert(vertices.end(), {x1, -halfHeight, z1, 0.0f, -1.0f, 0.0f});
        }

        vertexCount = (int)vertices.size() / 6;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
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
// TORUS CLASS - Parametric generation (for steering wheel)
// ============================================================================
class Torus {
public:
    unsigned int VAO, VBO;
    int vertexCount = 0;
    bool initialized = false;

    void init(float mainRadius = 0.4f, float tubeRadius = 0.1f, int mainSegments = 24, int tubeSegments = 12) {
        if (initialized) return;

        std::vector<float> vertices;

        for (int i = 0; i < mainSegments; i++) {
            float theta1 = (float)i / mainSegments * 2.0f * (float)M_PI;
            float theta2 = (float)(i + 1) / mainSegments * 2.0f * (float)M_PI;

            for (int j = 0; j < tubeSegments; j++) {
                float phi1 = (float)j / tubeSegments * 2.0f * (float)M_PI;
                float phi2 = (float)(j + 1) / tubeSegments * 2.0f * (float)M_PI;

                // Four vertices of the quad
                auto getVertex = [&](float theta, float phi) -> std::vector<float> {
                    float x = (mainRadius + tubeRadius * cos(phi)) * cos(theta);
                    float y = tubeRadius * sin(phi);
                    float z = (mainRadius + tubeRadius * cos(phi)) * sin(theta);
                    // Normal
                    float nx = cos(phi) * cos(theta);
                    float ny = sin(phi);
                    float nz = cos(phi) * sin(theta);
                    return {x, y, z, nx, ny, nz};
                };

                auto v1 = getVertex(theta1, phi1);
                auto v2 = getVertex(theta2, phi1);
                auto v3 = getVertex(theta2, phi2);
                auto v4 = getVertex(theta1, phi2);

                // Triangle 1
                vertices.insert(vertices.end(), v1.begin(), v1.end());
                vertices.insert(vertices.end(), v2.begin(), v2.end());
                vertices.insert(vertices.end(), v3.begin(), v3.end());
                // Triangle 2
                vertices.insert(vertices.end(), v1.begin(), v1.end());
                vertices.insert(vertices.end(), v3.begin(), v3.end());
                vertices.insert(vertices.end(), v4.begin(), v4.end());
            }
        }

        vertexCount = (int)vertices.size() / 6;

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glBindVertexArray(0);

        initialized = true;
    }

    void draw(const Shader& shader, glm::mat4 model, glm::vec3 color) {
        shader.setMat4("model", model);
        shader.setVec3("objectColor", color);
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
