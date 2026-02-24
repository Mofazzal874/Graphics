#ifndef BUS_H
#define BUS_H

#include "Primitives.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Bus {
public:
    // Primitives (shared across components)
    Cube cube;
    Cylinder cylinder;
    Torus torus;

    // State variables for interactive features
    float frontDoorAngle = 0.0f;   // 0 = closed, 90 = open
    float middleDoorAngle = 0.0f;
    float windowOpenAmount[12] = {0};  // 6 left + 6 right windows
    float fanRotation = 0.0f;
    float wheelRotation = 0.0f;    // Kept for compatibility but unused
    float steeringAngle = 0.0f;    // Kept for compatibility but unused
    bool lightOn = true;

    // Jet engine / hover state
    bool jetEngineOn = false;       // True when bus moves forward
    float jetFlameFlicker = 0.0f;   // Oscillating value for flame animation
    float hoverBobOffset = 0.0f;    // Subtle vertical bobbing
    float hoverTime = 0.0f;         // Time accumulator for hover effects

    // Colors
    glm::vec3 bodyColor = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 roofColor = glm::vec3(0.95f, 0.95f, 0.95f);
    glm::vec3 windowColor = glm::vec3(0.3f, 0.5f, 0.7f);
    glm::vec3 doorColor = glm::vec3(0.7f, 0.7f, 0.7f);
    glm::vec3 seatColor = glm::vec3(0.2f, 0.3f, 0.6f);
    glm::vec3 floorColor = glm::vec3(0.4f, 0.35f, 0.3f);
    glm::vec3 steeringColor = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 dashboardColor = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 fanColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.8f);
    glm::vec3 lightOffColor = glm::vec3(0.3f, 0.3f, 0.3f);

    // Jet engine colors
    glm::vec3 jetHousingColor = glm::vec3(0.35f, 0.35f, 0.38f);
    glm::vec3 jetNozzleColor = glm::vec3(0.25f, 0.25f, 0.28f);
    glm::vec3 jetInnerRingColor = glm::vec3(0.5f, 0.5f, 0.55f);
    glm::vec3 flameColorCore = glm::vec3(1.0f, 0.85f, 0.2f);
    glm::vec3 flameColorMid = glm::vec3(1.0f, 0.5f, 0.1f);
    glm::vec3 flameColorOuter = glm::vec3(0.9f, 0.2f, 0.05f);
    glm::vec3 hoverPadColor = glm::vec3(0.3f, 0.6f, 0.9f);
    glm::vec3 hoverGlowColor = glm::vec3(0.4f, 0.7f, 1.0f);

    // ==================== TEXTURE IDs (set from assignment.cpp) ====================
    unsigned int texFloor = 0;
    unsigned int texCarpet = 0;
    unsigned int texFabric = 0;
    unsigned int texWall = 0;
    unsigned int texDashboard = 0;
    unsigned int texBusBody = 0;

    void init() {
        cube.init();
        cylinder.init(36);
        torus.init(0.3f, 0.05f, 24, 12);
    }

    // Helper: draw with texture bound
    void drawTextured(const Shader& shader, glm::mat4 model, glm::vec3 color,
                      unsigned int texID, int mode,
                      Cube& shape) {
        if (texID != 0) {
            shader.setInt("textureMode", mode);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID);
            shader.setInt("textureSampler", 0);
        }
        shape.draw(shader, model, color);
        if (texID != 0) {
            shader.setInt("textureMode", 0);
        }
    }

    void drawTexturedCyl(const Shader& shader, glm::mat4 model, glm::vec3 color,
                         unsigned int texID, int mode) {
        if (texID != 0) {
            shader.setInt("textureMode", mode);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texID);
            shader.setInt("textureSampler", 0);
        }
        cylinder.draw(shader, model, color);
        if (texID != 0) {
            shader.setInt("textureMode", 0);
        }
    }

    void draw(const Shader& shader, glm::mat4 parentTransform) {
        drawExterior(shader, parentTransform);
        drawInterior(shader, parentTransform);
        drawJetEngine(shader, parentTransform);
        drawHoverSkirts(shader, parentTransform);
    }

    void drawExterior(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;

        // ==================== MAIN BODY (Coach Bus - Flat Front) ====================
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(10.0f, 3.0f, 3.0f));
        cube.draw(shader, model, bodyColor);

        // Roof
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.15f, 0.0f));
        model = glm::scale(model, glm::vec3(10.2f, 0.3f, 3.1f));
        cube.draw(shader, model, roofColor);

        // ==================== BUS SIDE PANELS (textured with bus name) ====================
        // Left side panel overlay
        if (texBusBody != 0) {
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -1.52f));
            model = glm::scale(model, glm::vec3(9.8f, 1.8f, 0.02f));
            drawTextured(shader, model, bodyColor, texBusBody, 1, cube);

            // Right side panel overlay
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 1.52f));
            model = glm::scale(model, glm::vec3(9.8f, 1.8f, 0.02f));
            drawTextured(shader, model, bodyColor, texBusBody, 1, cube);
        }

        // ==================== WINDOWS ====================
        // Left side windows
        for (int i = 0; i < 5; i++) {
            float yOffset = windowOpenAmount[i] * 0.4f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.8f + i * 1.5f, 1.2f - yOffset, -1.51f));
            model = glm::scale(model, glm::vec3(1.2f, 1.0f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Right side windows
        for (int i = 0; i < 5; i++) {
            float yOffset = windowOpenAmount[5 + i] * 0.4f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.8f + i * 1.5f, 1.2f - yOffset, 1.51f));
            model = glm::scale(model, glm::vec3(1.2f, 1.0f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Front windshield
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.8f, 2.5f));
        cube.draw(shader, model, windowColor);

        // Rear window
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.01f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.5f, 2.2f));
        cube.draw(shader, model, windowColor);

        // ==================== DOOR ====================
        glm::mat4 frontDoorPivot = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, 0.0f, 1.5f));
        frontDoorPivot = glm::rotate(frontDoorPivot, glm::radians(frontDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(frontDoorPivot, glm::vec3(0.5f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.8f, 0.08f));
        cube.draw(shader, model, doorColor);

        // ==================== HEADLIGHTS & TAILLIGHTS ====================
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.01f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(0.8f, 0.1f, 0.1f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.01f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(0.8f, 0.1f, 0.1f));
    }

    void drawInterior(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;

        // Colors for interior elements
        glm::vec3 fabricColor = glm::vec3(0.15f, 0.25f, 0.45f);
        glm::vec3 cushionColor = glm::vec3(0.2f, 0.35f, 0.55f);
        glm::vec3 armrestColor = glm::vec3(0.25f, 0.25f, 0.25f);
        glm::vec3 metalColor = glm::vec3(0.7f, 0.7f, 0.75f);
        glm::vec3 carpetColor = glm::vec3(0.3f, 0.25f, 0.2f);
        glm::vec3 rackColor = glm::vec3(0.5f, 0.5f, 0.52f);

        // ==================== INTERIOR CEILING (covers exterior roof, prevents z-fighting) ====================
    model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.92f, 0.0f));
    model = glm::scale(model, glm::vec3(9.5f, 0.05f, 2.55f));
    cube.draw(shader, model, glm::vec3(0.92f, 0.90f, 0.88f));  // Off-white ceiling

    // ==================== FLOOR (textured) ====================
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.9f, 0.0f));
        model = glm::scale(model, glm::vec3(9.5f, 0.1f, 2.6f));
        drawTextured(shader, model, floorColor, texFloor, 3, cube);

        // Aisle carpet (textured)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.84f, 0.0f));
        model = glm::scale(model, glm::vec3(9.0f, 0.02f, 0.6f));
        drawTextured(shader, model, carpetColor, texCarpet, 1, cube);

        // ==================== INTERIOR WALL PANELS (textured) ====================
        // Left wall panel
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, -1.49f));
        model = glm::scale(model, glm::vec3(9.5f, 2.5f, 0.02f));
        drawTextured(shader, model, glm::vec3(0.85f, 0.85f, 0.85f), texWall, 3, cube);

        // Right wall panel
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.3f, 1.49f));
        model = glm::scale(model, glm::vec3(9.5f, 2.5f, 0.02f));
        drawTextured(shader, model, glm::vec3(0.85f, 0.85f, 0.85f), texWall, 3, cube);

        // ==================== PASSENGER SEATS (textured cushions) ====================
        float seatY = -0.5f;
        float seatSpacing = 1.1f;
        int numSeats = 8;

        for (int side = 0; side < 2; side++) {
            float zPos = (side == 0) ? -0.85f : 0.85f;
            float zBack = (side == 0) ? -1.1f : 1.1f;
            float zArm = (side == 0) ? -0.55f : 0.55f;
            
            for (int i = 0; i < numSeats; i++) {
                float xPos = -3.2f + i * seatSpacing;

                // Seat cushion (textured fabric)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY, zPos));
                model = glm::scale(model, glm::vec3(0.8f, 0.25f, 0.7f));
                drawTextured(shader, model, cushionColor, texFabric, 3, cube);

                // Seat frame/base
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY - 0.2f, zPos));
                model = glm::scale(model, glm::vec3(0.75f, 0.15f, 0.65f));
                cube.draw(shader, model, armrestColor);

                // Seat back (textured fabric)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.55f, zBack));
                model = glm::scale(model, glm::vec3(0.75f, 0.85f, 0.12f));
                drawTextured(shader, model, fabricColor, texFabric, 3, cube);

                // Backrest cushion (textured fabric)
                float cushionZ = (side == 0) ? zBack + 0.08f : zBack - 0.08f;
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.5f, cushionZ));
                model = glm::scale(model, glm::vec3(0.65f, 0.7f, 0.08f));
                drawTextured(shader, model, cushionColor, texFabric, 3, cube);

                // Headrest
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 1.1f, zBack));
                model = glm::scale(model, glm::vec3(0.4f, 0.25f, 0.15f));
                drawTextured(shader, model, fabricColor, texFabric, 3, cube);

                // Inner armrest
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.15f, zArm));
                model = glm::scale(model, glm::vec3(0.7f, 0.08f, 0.1f));
                cube.draw(shader, model, armrestColor);

                // Seat leg
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY - 0.45f, zPos));
                model = glm::scale(model, glm::vec3(0.08f, 0.35f, 0.08f));
                cylinder.draw(shader, model, metalColor);
            }
        }

        // ==================== HANDRAILS (horizontal only, no pillars) ====================
        for (int side = 0; side < 2; side++) {
            float zRail = (side == 0) ? -0.3f : 0.3f;
            // Horizontal grab bar only — no vertical support posts
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.6f, zRail));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(0.05f, 8.0f, 0.05f));
            cylinder.draw(shader, model, metalColor);
            // Removed vertical posts (they looked like unwanted pillars)
        }

        // ==================== LUGGAGE RACKS ====================
        for (int side = 0; side < 2; side++) {
            float zRack = (side == 0) ? -1.2f : 1.2f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, zRack));
            model = glm::scale(model, glm::vec3(8.5f, 0.05f, 0.4f));
            cube.draw(shader, model, rackColor);

            float backZ = (side == 0) ? zRack - 0.15f : zRack + 0.15f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.65f, backZ));
            model = glm::scale(model, glm::vec3(8.5f, 0.35f, 0.05f));
            cube.draw(shader, model, rackColor);
        }

        // ==================== DRIVER AREA (textured dashboard) ====================
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.3f, 0.3f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 1.2f, 2.4f));
        drawTextured(shader, model, dashboardColor, texDashboard, 1, cube);

        // Instrument panel (textured dashboard)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.6f, -0.3f));
        model = glm::scale(model, glm::vec3(0.3f, 0.4f, 0.8f));
        drawTextured(shader, model, glm::vec3(0.1f, 0.1f, 0.1f), texDashboard, 1, cube);

        // Driver seat
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.8f, seatY + 0.1f, -0.6f));
        model = glm::scale(model, glm::vec3(0.9f, 0.25f, 0.8f));
        drawTextured(shader, model, cushionColor, texFabric, 3, cube);

        // Driver seat back
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.8f, seatY + 0.65f, -1.0f));
        model = glm::scale(model, glm::vec3(0.85f, 1.0f, 0.15f));
        drawTextured(shader, model, fabricColor, texFabric, 3, cube);

        // Driver seat legs
        for (int s = -1; s <= 1; s += 2) {
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.8f + s * 0.35f, seatY - 0.3f, -0.6f));
            model = glm::scale(model, glm::vec3(0.07f, 0.45f, 0.07f));
            cube.draw(shader, model, glm::vec3(0.25f, 0.25f, 0.25f));
        }

        // Driver seat headrest
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.8f, seatY + 1.35f, -1.0f));
        model = glm::scale(model, glm::vec3(0.45f, 0.28f, 0.13f));
        drawTextured(shader, model, fabricColor, texFabric, 3, cube);

        // ==================== STEERING WHEEL ====================
        // Column — tilted toward driver (toward -X and up)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.1f, 0.55f, -0.6f));
        model = glm::rotate(model, glm::radians(35.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // tilt forward
        model = glm::scale(model, glm::vec3(0.06f, 0.45f, 0.06f));
        cylinder.draw(shader, model, steeringColor);

        // Torus ring — faces the driver (lies in XZ plane, rotated so ring is upright toward driver)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.85f, 0.9f, -0.6f));
        model = glm::rotate(model, glm::radians(55.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // match column tilt
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));  // face driver
        model = glm::scale(model, glm::vec3(0.65f, 0.65f, 0.65f));
        torus.draw(shader, model, steeringColor);

        // Center hub
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.85f, 0.9f, -0.6f));
        model = glm::scale(model, glm::vec3(0.08f, 0.08f, 0.08f));
        cylinder.draw(shader, model, steeringColor);

        // ==================== CEILING FANS ====================
        glm::vec3 metalColorLocal = glm::vec3(0.7f, 0.7f, 0.75f);
        for (int f = 0; f < 2; f++) {
            float fanX = -1.5f + f * 3.0f;
            glm::mat4 fanBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(fanX, 1.85f, 0.0f));
            fanBase = glm::rotate(fanBase, glm::radians(fanRotation + f * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            model = glm::scale(fanBase, glm::vec3(0.15f, 0.1f, 0.15f));
            cylinder.draw(shader, model, metalColorLocal);

            for (int i = 0; i < 4; i++) {
                glm::mat4 blade = glm::rotate(fanBase, glm::radians(90.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));
                blade = glm::translate(blade, glm::vec3(0.3f, 0.0f, 0.0f));
                blade = glm::scale(blade, glm::vec3(0.45f, 0.03f, 0.12f));
                cube.draw(shader, blade, fanColor);
            }
        }

        // ==================== INTERIOR LIGHTS (pulled down to avoid z-fighting with roof) ====================
        glm::vec3 currentLightColor = lightOn ? lightColor : lightOffColor;
        for (int side = 0; side < 2; side++) {
            float zLight = (side == 0) ? -0.8f : 0.8f;
            // Y=1.88 instead of 1.95 — avoids z-fighting with exterior roof at Y~2.0
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.88f, zLight));
            model = glm::scale(model, glm::vec3(8.0f, 0.04f, 0.15f));
            cube.draw(shader, model, currentLightColor);
        }

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.88f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.08f, 0.5f));
        cylinder.draw(shader, model, currentLightColor);

        // ==================== ENTRY STEPS ====================
        if (frontDoorAngle > 45.0f) {
            glm::vec3 metalColorSteps = glm::vec3(0.7f, 0.7f, 0.75f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, -1.2f, 1.8f));
            model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.5f));
            cube.draw(shader, model, metalColorSteps);
            
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, -0.9f, 1.6f));
            model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.5f));
            cube.draw(shader, model, metalColorSteps);
        }
    }

    // ==================== JET ENGINE (rear mounted) ====================
    void drawJetEngine(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;
        glm::vec3 metalColor = glm::vec3(0.7f, 0.7f, 0.75f);

        glm::mat4 engineBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.8f, 0.5f, 0.0f));

        model = engineBase;
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.4f, 1.8f, 1.4f));
        cylinder.draw(shader, model, jetHousingColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.5f, 0.3f, 1.5f));
        cylinder.draw(shader, model, jetInnerRingColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.9f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.1f, 0.4f, 1.1f));
        cylinder.draw(shader, model, jetNozzleColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(7.1f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(2.8f, 2.8f, 2.8f));
        torus.draw(shader, model, jetNozzleColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.5f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.5f, 1.2f, 0.5f));
        cylinder.draw(shader, model, glm::vec3(0.15f, 0.15f, 0.18f));

        // Support struts
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 1.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.3f));
        cube.draw(shader, model, metalColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, -0.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.3f));
        cube.draw(shader, model, metalColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 0.5f, -0.9f));
        model = glm::scale(model, glm::vec3(0.8f, 0.3f, 0.15f));
        cube.draw(shader, model, metalColor);

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 0.5f, 0.9f));
        model = glm::scale(model, glm::vec3(0.8f, 0.3f, 0.15f));
        cube.draw(shader, model, metalColor);

        // Fin
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 1.5f, 0.0f));
        model = glm::scale(model, glm::vec3(1.5f, 0.4f, 0.08f));
        cube.draw(shader, model, jetHousingColor);

        // --- JET FLAME ---
        if (jetEngineOn) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glDepthMask(GL_FALSE);

            shader.setBool("isEmissive", true);

            float t = jetFlameFlicker;
            float nozzleX = 7.15f;

            float glowPulse = 0.85f + 0.15f * sin(t * 25.0f);
            shader.setFloat("alpha", 0.9f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(nozzleX, 0.5f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(0.95f * glowPulse, 0.08f, 0.95f * glowPulse));
            cylinder.draw(shader, model, glm::vec3(1.0f, 0.95f, 0.85f));

            struct FlameLayer {
                float lengthScale;
                float radiusScale;
                float freqOffset;
                float alphaVal;
                glm::vec3 color;
            };

            FlameLayer layers[] = {
                { 1.0f,  0.20f, 0.0f, 0.95f, glm::vec3(1.0f, 0.97f, 0.85f) },
                { 0.92f, 0.28f, 2.1f, 0.85f, glm::vec3(1.0f, 0.92f, 0.55f) },
                { 0.85f, 0.38f, 4.3f, 0.75f, flameColorCore },
                { 0.78f, 0.45f, 6.7f, 0.65f, glm::vec3(1.0f, 0.75f, 0.2f) },
                { 0.68f, 0.55f, 8.9f, 0.55f, flameColorMid },
                { 0.60f, 0.65f, 11.3f, 0.45f, glm::vec3(1.0f, 0.4f, 0.08f) },
                { 0.50f, 0.78f, 13.7f, 0.35f, flameColorOuter },
                { 0.40f, 0.90f, 16.1f, 0.25f, glm::vec3(0.8f, 0.15f, 0.03f) },
                { 0.30f, 1.05f, 18.9f, 0.15f, glm::vec3(0.5f, 0.08f, 0.02f) },
            };

            int numLayers = sizeof(layers) / sizeof(layers[0]);

            for (int i = 0; i < numLayers; i++) {
                FlameLayer& L = layers[i];
                float baseLen = 3.0f * L.lengthScale;
                float turbulence = 0.5f * sin(t * (14.0f + L.freqOffset))
                                 + 0.25f * sin(t * (21.0f + L.freqOffset * 0.7f))
                                 + 0.15f * sin(t * (33.0f + L.freqOffset * 1.3f));
                float len = baseLen + turbulence * L.lengthScale;
                if (len < 0.2f) len = 0.2f;

                float rad = L.radiusScale * (0.45f + 0.06f * sin(t * (17.0f + L.freqOffset * 0.5f)));
                float yOff = 0.03f * sin(t * (9.0f + L.freqOffset * 0.3f));
                float zOff = 0.03f * sin(t * (7.0f + L.freqOffset * 0.6f));

                shader.setFloat("alpha", L.alphaVal);
                model = parent * glm::translate(glm::mat4(1.0f),
                    glm::vec3(nozzleX + len * 0.5f, 0.5f + yOff, zOff));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, glm::vec3(rad, len, rad));
                cylinder.draw(shader, model, L.color);
            }

            // Sparks
            shader.setFloat("alpha", 0.9f);
            for (int s = 0; s < 5; s++) {
                float sparkPhase = t * (20.0f + s * 7.3f) + s * 1.7f;
                float sparkX = nozzleX + 0.5f + fmod(sparkPhase * 0.8f, 2.5f);
                float sparkY = 0.5f + 0.15f * sin(sparkPhase * 3.0f);
                float sparkZ = 0.12f * sin(sparkPhase * 2.5f + s * 0.9f);
                float sparkSize = 0.04f + 0.02f * sin(sparkPhase * 5.0f);

                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(sparkX, sparkY, sparkZ));
                model = glm::scale(model, glm::vec3(sparkSize, sparkSize, sparkSize));
                cylinder.draw(shader, model, glm::vec3(1.0f, 0.95f, 0.7f));
            }

            shader.setBool("isEmissive", false);
            shader.setFloat("alpha", 1.0f);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    // ==================== HOVER SKIRTS / PADS ====================
    void drawHoverSkirts(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;

        float padPositions[4][2] = {
            {-3.5f, -1.3f}, {-3.5f,  1.3f},
            { 3.5f, -1.3f}, { 3.5f,  1.3f}
        };

        float glowPulse = 0.8f + 0.2f * sin(hoverTime * 5.0f);
        float padBrightness = 0.7f + 0.3f * sin(hoverTime * 3.0f);

        for (int i = 0; i < 4; i++) {
            float px = padPositions[i][0];
            float pz = padPositions[i][1];
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.1f, pz));
            model = glm::scale(model, glm::vec3(1.0f, 0.15f, 0.8f));
            cylinder.draw(shader, model, jetHousingColor);
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        shader.setBool("isEmissive", true);

        for (int i = 0; i < 4; i++) {
            float px = padPositions[i][0];
            float pz = padPositions[i][1];

            shader.setFloat("alpha", 0.7f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.25f, pz));
            model = glm::scale(model, glm::vec3(0.85f * glowPulse, 0.06f, 0.65f * glowPulse));
            cylinder.draw(shader, model, hoverPadColor * padBrightness);

            shader.setFloat("alpha", 0.5f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.2f, pz));
            model = glm::scale(model, glm::vec3(2.0f * glowPulse, 1.5f * glowPulse, 2.0f * glowPulse));
            torus.draw(shader, model, hoverGlowColor * padBrightness);

            shader.setFloat("alpha", 0.85f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.3f, pz));
            model = glm::scale(model, glm::vec3(0.35f, 0.04f, 0.35f));
            cylinder.draw(shader, model, glm::vec3(0.6f, 0.85f, 1.0f) * glowPulse);
        }

        float bellyGlow = 0.6f + 0.15f * sin(hoverTime * 4.0f);
        shader.setFloat("alpha", 0.4f);
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.15f, 0.0f));
        model = glm::scale(model, glm::vec3(8.0f, 0.04f, 1.0f));
        cube.draw(shader, model, hoverPadColor * bellyGlow);

        shader.setBool("isEmissive", false);
        shader.setFloat("alpha", 1.0f);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }

    // ==================== INTERACTIVE METHODS ====================
    void toggleFrontDoor() {
        frontDoorAngle = (frontDoorAngle < 45.0f) ? 90.0f : 0.0f;
    }

    void toggleMiddleDoor() {
        middleDoorAngle = (middleDoorAngle < 45.0f) ? 90.0f : 0.0f;
    }

    void toggleWindow(int index) {
        if (index >= 0 && index < 12) {
            windowOpenAmount[index] = (windowOpenAmount[index] < 0.5f) ? 1.0f : 0.0f;
        }
    }

    void updateFan(float deltaTime, bool spinning) {
        if (spinning) {
            fanRotation += 200.0f * deltaTime;
            if (fanRotation > 360.0f) fanRotation -= 360.0f;
        }
    }

    void toggleLight() {
        lightOn = !lightOn;
    }

    void updateJetFlame(float deltaTime) {
        hoverTime += deltaTime;
        if (jetEngineOn) {
            jetFlameFlicker += deltaTime * 8.0f;
            if (jetFlameFlicker > 100.0f) jetFlameFlicker -= 100.0f;
        }
        hoverBobOffset = 0.15f * sin(hoverTime * 2.5f);
    }

    void updateWheels(float movementSpeed) {
        // No-op: hover vehicle has no wheels
    }

    void cleanup() {
        cube.cleanup();
        cylinder.cleanup();
        torus.cleanup();
    }
};

#endif
