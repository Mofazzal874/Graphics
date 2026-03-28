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

// ============================================================================
// BEZIER SURFACE OF REVOLUTION CLASS
// Generates a 3D surface by revolving a Bezier curve around the Y-axis.
// Control points define the 2D profile (x=radius, y=height).
// ============================================================================
class BezierSurface {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    // Compute binomial coefficient C(n, k)
    static long long binomial(int n, int k) {
        if (k > n - k) k = n - k;
        long long result = 1;
        for (int i = 0; i < k; i++) {
            result = result * (n - i) / (i + 1);
        }
        return result;
    }

    // Evaluate Bezier curve at parameter t given control points (2D: x=radius, y=height)
    static glm::vec2 evaluateBezier(const std::vector<glm::vec2>& controlPoints, float t) {
        int n = (int)controlPoints.size() - 1;
        glm::vec2 point(0.0f);
        for (int i = 0; i <= n; i++) {
            float blend = (float)(binomial(n, i) * pow(t, (float)i) * pow(1.0f - t, (float)(n - i)));
            point += blend * controlPoints[i];
        }
        return point;
    }

    // Evaluate Bezier curve tangent at parameter t
    static glm::vec2 evaluateBezierTangent(const std::vector<glm::vec2>& controlPoints, float t) {
        int n = (int)controlPoints.size() - 1;
        if (n < 1) return glm::vec2(0.0f, 1.0f);
        glm::vec2 tangent(0.0f);
        for (int i = 0; i < n; i++) {
            float blend = (float)(binomial(n - 1, i) * pow(t, (float)i) * pow(1.0f - t, (float)(n - 1 - i)));
            tangent += blend * (controlPoints[i + 1] - controlPoints[i]);
        }
        tangent *= (float)n;
        return tangent;
    }

    void init(const std::vector<glm::vec2>& controlPoints, int curveSegments = 30, int rotSegments = 36) {
        if (initialized) return;
        std::vector<float> vertices;

        // Sample points along the Bezier curve
        std::vector<glm::vec2> curvePoints(curveSegments + 1);
        std::vector<glm::vec2> curveTangents(curveSegments + 1);
        for (int i = 0; i <= curveSegments; i++) {
            float t = (float)i / curveSegments;
            curvePoints[i] = evaluateBezier(controlPoints, t);
            curveTangents[i] = evaluateBezierTangent(controlPoints, t);
        }

        // Revolve around Y-axis
        float rotStep = 2.0f * (float)M_PI / rotSegments;
        for (int i = 0; i < curveSegments; i++) {
            float v0 = (float)i / curveSegments;
            float v1 = (float)(i + 1) / curveSegments;
            for (int j = 0; j < rotSegments; j++) {
                float theta0 = j * rotStep;
                float theta1 = (j + 1) * rotStep;
                float u0 = (float)j / rotSegments;
                float u1 = (float)(j + 1) / rotSegments;

                auto revolveVert = [&](int ci, float theta, float u, float v) {
                    float r = curvePoints[ci].x;
                    float y = curvePoints[ci].y;
                    float x = r * cos(theta);
                    float z = -r * sin(theta);

                    // Normal: cross product of tangent direction with rotation
                    glm::vec2 tang = curveTangents[ci];
                    float len = glm::length(tang);
                    if (len > 0.0001f) tang /= len;
                    // Profile normal in 2D (perpendicular to tangent, pointing outward)
                    glm::vec2 profNormal(tang.y, -tang.x);
                    float nx = profNormal.x * cos(theta);
                    float ny = profNormal.y;
                    float nz = -profNormal.x * sin(theta);
                    float nLen = sqrt(nx * nx + ny * ny + nz * nz);
                    if (nLen > 0.0001f) { nx /= nLen; ny /= nLen; nz /= nLen; }

                    vertices.insert(vertices.end(), { x, y, z, nx, ny, nz, u, v });
                };

                // Two triangles per quad
                revolveVert(i, theta0, u0, v0);
                revolveVert(i + 1, theta0, u0, v1);
                revolveVert(i + 1, theta1, u1, v1);

                revolveVert(i + 1, theta1, u1, v1);
                revolveVert(i, theta1, u1, v0);
                revolveVert(i, theta0, u0, v0);
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
// CATMULL-ROM SPLINE SURFACE OF REVOLUTION CLASS
// Uses Catmull-Rom spline through given points, then revolves around Y-axis.
// ============================================================================
class SplineSurface {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    // Catmull-Rom spline interpolation between p1 and p2, with neighbors p0 and p3
    static glm::vec2 catmullRom(const glm::vec2& p0, const glm::vec2& p1,
                                 const glm::vec2& p2, const glm::vec2& p3, float t) {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * ((2.0f * p1) +
                       (-p0 + p2) * t +
                       (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                       (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    static glm::vec2 catmullRomTangent(const glm::vec2& p0, const glm::vec2& p1,
                                        const glm::vec2& p2, const glm::vec2& p3, float t) {
        float t2 = t * t;
        return 0.5f * ((-p0 + p2) +
                       (4.0f * p0 - 10.0f * p1 + 8.0f * p2 - 2.0f * p3) * t +
                       (-3.0f * p0 + 9.0f * p1 - 9.0f * p2 + 3.0f * p3) * t2);
    }

    void init(const std::vector<glm::vec2>& points, int segmentsPerSpan = 10, int rotSegments = 36) {
        if (initialized) return;
        if (points.size() < 2) return;
        std::vector<float> vertices;

        // Build full curve by sampling each span
        int numSpans = (int)points.size() - 1;
        int totalCurveSegs = numSpans * segmentsPerSpan;
        std::vector<glm::vec2> curvePoints(totalCurveSegs + 1);
        std::vector<glm::vec2> curveTangents(totalCurveSegs + 1);

        for (int span = 0; span < numSpans; span++) {
            int i0 = (span > 0) ? span - 1 : 0;
            int i1 = span;
            int i2 = span + 1;
            int i3 = (span + 2 < (int)points.size()) ? span + 2 : (int)points.size() - 1;

            for (int s = 0; s <= segmentsPerSpan; s++) {
                if (span > 0 && s == 0) continue; // avoid duplicate at span boundaries
                float t = (float)s / segmentsPerSpan;
                int idx = span * segmentsPerSpan + s;
                curvePoints[idx] = catmullRom(points[i0], points[i1], points[i2], points[i3], t);
                curveTangents[idx] = catmullRomTangent(points[i0], points[i1], points[i2], points[i3], t);
            }
        }

        // Revolve around Y-axis (same as BezierSurface)
        float rotStep = 2.0f * (float)M_PI / rotSegments;
        for (int i = 0; i < totalCurveSegs; i++) {
            float v0 = (float)i / totalCurveSegs;
            float v1 = (float)(i + 1) / totalCurveSegs;
            for (int j = 0; j < rotSegments; j++) {
                float theta0 = j * rotStep;
                float theta1 = (j + 1) * rotStep;
                float u0 = (float)j / rotSegments;
                float u1 = (float)(j + 1) / rotSegments;

                auto revolveVert = [&](int ci, float theta, float u, float v) {
                    float r = curvePoints[ci].x;
                    float y = curvePoints[ci].y;
                    float x = r * cos(theta);
                    float z = -r * sin(theta);

                    glm::vec2 tang = curveTangents[ci];
                    float len = glm::length(tang);
                    if (len > 0.0001f) tang /= len;
                    glm::vec2 profNormal(tang.y, -tang.x);
                    float nx = profNormal.x * cos(theta);
                    float ny = profNormal.y;
                    float nz = -profNormal.x * sin(theta);
                    float nLen = sqrt(nx * nx + ny * ny + nz * nz);
                    if (nLen > 0.0001f) { nx /= nLen; ny /= nLen; nz /= nLen; }

                    vertices.insert(vertices.end(), { x, y, z, nx, ny, nz, u, v });
                };

                revolveVert(i, theta0, u0, v0);
                revolveVert(i + 1, theta0, u0, v1);
                revolveVert(i + 1, theta1, u1, v1);

                revolveVert(i + 1, theta1, u1, v1);
                revolveVert(i, theta1, u1, v0);
                revolveVert(i, theta0, u0, v0);
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
// RULED SURFACE CLASS
// Generates a surface by linearly interpolating between two Bezier curves.
// Each curve is defined by its own set of 3D control points.
// ============================================================================
class RuledSurface {
public:
    unsigned int VAO, VBO;
    bool initialized = false;
    int vertexCount = 0;

    void init(const std::vector<glm::vec3>& curve1Points,
              const std::vector<glm::vec3>& curve2Points,
              int uSegments = 30, int vSegments = 10) {
        if (initialized) return;
        std::vector<float> vertices;

        // Sample both curves
        std::vector<glm::vec3> c1(uSegments + 1), c2(uSegments + 1);
        int n1 = (int)curve1Points.size() - 1;
        int n2 = (int)curve2Points.size() - 1;

        for (int i = 0; i <= uSegments; i++) {
            float t = (float)i / uSegments;
            // Evaluate curve 1 (Bezier)
            glm::vec3 p1(0.0f);
            for (int k = 0; k <= n1; k++) {
                float blend = (float)(BezierSurface::binomial(n1, k) * pow(t, (float)k) * pow(1.0f - t, (float)(n1 - k)));
                p1 += blend * curve1Points[k];
            }
            c1[i] = p1;

            // Evaluate curve 2 (Bezier)
            glm::vec3 p2(0.0f);
            for (int k = 0; k <= n2; k++) {
                float blend = (float)BezierSurface::binomial(n2, k) * pow(t, (float)k) * pow(1.0f - t, (float)(n2 - k));
                p2 += blend * curve2Points[k];
            }
            c2[i] = p2;
        }

        // Generate ruled surface: S(u,v) = (1-v)*C1(u) + v*C2(u)
        for (int i = 0; i < uSegments; i++) {
            float u0 = (float)i / uSegments;
            float u1 = (float)(i + 1) / uSegments;
            for (int j = 0; j < vSegments; j++) {
                float v0 = (float)j / vSegments;
                float v1 = (float)(j + 1) / vSegments;

                auto surfVert = [&](int ui, float v, float texU, float texV) {
                    glm::vec3 pos = (1.0f - v) * c1[ui] + v * c2[ui];

                    // Approximate normal via cross product of partial derivatives
                    glm::vec3 dU(0.0f);
                    if (ui > 0 && ui < uSegments) {
                        glm::vec3 pPrev = (1.0f - v) * c1[ui - 1] + v * c2[ui - 1];
                        glm::vec3 pNext = (1.0f - v) * c1[ui + 1] + v * c2[ui + 1];
                        dU = pNext - pPrev;
                    } else if (ui == 0) {
                        glm::vec3 pNext = (1.0f - v) * c1[ui + 1] + v * c2[ui + 1];
                        dU = pNext - pos;
                    } else {
                        glm::vec3 pPrev = (1.0f - v) * c1[ui - 1] + v * c2[ui - 1];
                        dU = pos - pPrev;
                    }
                    glm::vec3 dV = c2[ui] - c1[ui];
                    glm::vec3 normal = glm::cross(dU, dV);
                    float nLen = glm::length(normal);
                    if (nLen > 0.0001f) normal /= nLen;
                    else normal = glm::vec3(0.0f, 1.0f, 0.0f);

                    vertices.insert(vertices.end(), { pos.x, pos.y, pos.z,
                                                       normal.x, normal.y, normal.z,
                                                       texU, texV });
                };

                surfVert(i, v0, u0, v0);
                surfVert(i + 1, v0, u1, v0);
                surfVert(i + 1, v1, u1, v1);

                surfVert(i + 1, v1, u1, v1);
                surfVert(i, v1, u0, v1);
                surfVert(i, v0, u0, v0);
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

#endif
