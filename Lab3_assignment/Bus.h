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
    glm::vec3 jetHousingColor = glm::vec3(0.35f, 0.35f, 0.38f);   // Dark metallic
    glm::vec3 jetNozzleColor = glm::vec3(0.25f, 0.25f, 0.28f);    // Darker metallic
    glm::vec3 jetInnerRingColor = glm::vec3(0.5f, 0.5f, 0.55f);   // Lighter ring
    glm::vec3 flameColorCore = glm::vec3(1.0f, 0.85f, 0.2f);      // Bright yellow-orange
    glm::vec3 flameColorMid = glm::vec3(1.0f, 0.5f, 0.1f);        // Orange
    glm::vec3 flameColorOuter = glm::vec3(0.9f, 0.2f, 0.05f);     // Red-orange
    glm::vec3 hoverPadColor = glm::vec3(0.3f, 0.6f, 0.9f);        // Blue glow
    glm::vec3 hoverGlowColor = glm::vec3(0.4f, 0.7f, 1.0f);       // Brighter blue glow

    void init() {
        cube.init();
        cylinder.init(36);
        torus.init(0.3f, 0.05f, 24, 12);
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
        // Single unified body (no protruding engine hood)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(10.0f, 3.0f, 3.0f));
        cube.draw(shader, model, bodyColor);

        // Roof
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.15f, 0.0f));
        model = glm::scale(model, glm::vec3(10.2f, 0.3f, 3.1f));
        cube.draw(shader, model, roofColor);

        // ==================== WHEELS REMOVED — HOVER VEHICLE ====================
        // (Hover pads are drawn in drawHoverSkirts())

        // ==================== WINDOWS ====================
        // Left side windows (5 windows - properly spaced within body)
        for (int i = 0; i < 5; i++) {
            float yOffset = windowOpenAmount[i] * 0.4f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.8f + i * 1.5f, 1.2f - yOffset, -1.51f));
            model = glm::scale(model, glm::vec3(1.2f, 1.0f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Right side windows (5 windows)
        for (int i = 0; i < 5; i++) {
            float yOffset = windowOpenAmount[5 + i] * 0.4f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.8f + i * 1.5f, 1.2f - yOffset, 1.51f));
            model = glm::scale(model, glm::vec3(1.2f, 1.0f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Front windshield (large, coach style)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.8f, 2.5f));
        cube.draw(shader, model, windowColor);

        // Rear window (within body bounds)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.01f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.5f, 2.2f));
        cube.draw(shader, model, windowColor);

        // ==================== DOOR ====================
        // Front door (positioned between front wheel and first window)
        glm::mat4 frontDoorPivot = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, 0.0f, 1.5f));
        frontDoorPivot = glm::rotate(frontDoorPivot, glm::radians(frontDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(frontDoorPivot, glm::vec3(0.5f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.8f, 0.08f));
        cube.draw(shader, model, doorColor);

        // ==================== HEADLIGHTS & TAILLIGHTS ====================
        // Headlights (front - on flat face)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 0.0f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.01f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.4f, 0.5f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        // Taillights (rear)
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
        glm::vec3 fabricColor = glm::vec3(0.15f, 0.25f, 0.45f);      // Dark blue fabric
        glm::vec3 cushionColor = glm::vec3(0.2f, 0.35f, 0.55f);      // Lighter blue cushion
        glm::vec3 armrestColor = glm::vec3(0.25f, 0.25f, 0.25f);     // Dark gray armrest
        glm::vec3 metalColor = glm::vec3(0.7f, 0.7f, 0.75f);         // Chrome/metal
        glm::vec3 carpetColor = glm::vec3(0.3f, 0.25f, 0.2f);        // Brown carpet aisle
        glm::vec3 rackColor = glm::vec3(0.5f, 0.5f, 0.52f);          // Luggage rack

        // ==================== FLOOR ====================
        // Main floor
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.9f, 0.0f));
        model = glm::scale(model, glm::vec3(9.5f, 0.1f, 2.6f));
        cube.draw(shader, model, floorColor);

        // Aisle carpet (center strip)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.84f, 0.0f));
        model = glm::scale(model, glm::vec3(9.0f, 0.02f, 0.6f));
        cube.draw(shader, model, carpetColor);

        // ==================== PASSENGER SEATS (Coach Style) ====================
        float seatY = -0.5f;
        float seatSpacing = 1.1f;
        int numSeats = 8;

        // Draw seats on both sides
        for (int side = 0; side < 2; side++) {
            float zPos = (side == 0) ? -0.85f : 0.85f;      // Left or right side
            float zBack = (side == 0) ? -1.1f : 1.1f;       // Backrest position
            float zArm = (side == 0) ? -0.55f : 0.55f;      // Inner armrest (aisle side)
            
            for (int i = 0; i < numSeats; i++) {
                float xPos = -3.2f + i * seatSpacing;

                // Seat cushion (main sitting surface)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY, zPos));
                model = glm::scale(model, glm::vec3(0.8f, 0.25f, 0.7f));
                cube.draw(shader, model, cushionColor);

                // Seat frame/base
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY - 0.2f, zPos));
                model = glm::scale(model, glm::vec3(0.75f, 0.15f, 0.65f));
                cube.draw(shader, model, armrestColor);

                // Seat back (backrest)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.55f, zBack));
                model = glm::scale(model, glm::vec3(0.75f, 0.85f, 0.12f));
                cube.draw(shader, model, fabricColor);

                // Backrest cushion (softer part)
                float cushionZ = (side == 0) ? zBack + 0.08f : zBack - 0.08f;
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.5f, cushionZ));
                model = glm::scale(model, glm::vec3(0.65f, 0.7f, 0.08f));
                cube.draw(shader, model, cushionColor);

                // Headrest
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 1.1f, zBack));
                model = glm::scale(model, glm::vec3(0.4f, 0.25f, 0.15f));
                cube.draw(shader, model, fabricColor);

                // Inner armrest (toward aisle)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY + 0.15f, zArm));
                model = glm::scale(model, glm::vec3(0.7f, 0.08f, 0.1f));
                cube.draw(shader, model, armrestColor);

                // Seat leg (metal support)
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(xPos, seatY - 0.45f, zPos));
                model = glm::scale(model, glm::vec3(0.08f, 0.35f, 0.08f));
                cylinder.draw(shader, model, metalColor);
            }
        }

        // ==================== HANDRAILS ====================
        // Overhead handrails (both sides of aisle)
        for (int side = 0; side < 2; side++) {
            float zRail = (side == 0) ? -0.3f : 0.3f;
            
            // Main horizontal rail
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.7f, zRail));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(0.05f, 8.0f, 0.05f));
            cylinder.draw(shader, model, metalColor);

            // Vertical support poles
            for (int i = 0; i < 5; i++) {
                model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.5f + i * 2.0f, 1.3f, zRail));
                model = glm::scale(model, glm::vec3(0.04f, 0.8f, 0.04f));
                cylinder.draw(shader, model, metalColor);
            }
        }

        // ==================== LUGGAGE RACKS ====================
        // Overhead luggage compartments (both sides)
        for (int side = 0; side < 2; side++) {
            float zRack = (side == 0) ? -1.2f : 1.2f;
            
            // Rack bottom
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, zRack));
            model = glm::scale(model, glm::vec3(8.5f, 0.05f, 0.4f));
            cube.draw(shader, model, rackColor);

            // Rack back
            float backZ = (side == 0) ? zRack - 0.15f : zRack + 0.15f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.65f, backZ));
            model = glm::scale(model, glm::vec3(8.5f, 0.35f, 0.05f));
            cube.draw(shader, model, rackColor);
        }

        // ==================== DRIVER AREA ====================
        // Dashboard (larger, more detailed)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.3f, 0.3f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 1.2f, 2.4f));
        cube.draw(shader, model, dashboardColor);

        // Instrument panel
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.6f, -0.3f));
        model = glm::scale(model, glm::vec3(0.3f, 0.4f, 0.8f));
        cube.draw(shader, model, glm::vec3(0.1f, 0.1f, 0.1f));

        // Driver seat (more comfortable style)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.5f, seatY, -0.6f));
        model = glm::scale(model, glm::vec3(0.9f, 0.3f, 0.8f));
        cube.draw(shader, model, cushionColor);
        
        // Driver seat back
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.5f, seatY + 0.6f, -0.95f));
        model = glm::scale(model, glm::vec3(0.85f, 0.9f, 0.15f));
        cube.draw(shader, model, fabricColor);

        // Driver seat headrest
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.5f, seatY + 1.2f, -0.95f));
        model = glm::scale(model, glm::vec3(0.4f, 0.3f, 0.12f));
        cube.draw(shader, model, fabricColor);

        // ==================== STEERING WHEEL ====================
        // Steering column
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, 0.7f, -0.6f));
        model = glm::rotate(model, glm::radians(-25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.06f, 0.4f, 0.06f));
        cylinder.draw(shader, model, steeringColor);

        // Steering wheel (torus)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.85f, 0.95f, -0.6f));
        model = glm::rotate(model, glm::radians(-25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.7f, 0.7f, 0.7f));
        torus.draw(shader, model, steeringColor);

        // ==================== CEILING FANS (2 fans) ====================
        for (int f = 0; f < 2; f++) {
            float fanX = -1.5f + f * 3.0f;
            glm::mat4 fanBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(fanX, 1.85f, 0.0f));
            fanBase = glm::rotate(fanBase, glm::radians(fanRotation + f * 45.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            // Fan hub
            model = glm::scale(fanBase, glm::vec3(0.15f, 0.1f, 0.15f));
            cylinder.draw(shader, model, metalColor);

            // Fan blades (4 blades)
            for (int i = 0; i < 4; i++) {
                glm::mat4 blade = glm::rotate(fanBase, glm::radians(90.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));
                blade = glm::translate(blade, glm::vec3(0.3f, 0.0f, 0.0f));
                blade = glm::scale(blade, glm::vec3(0.45f, 0.03f, 0.12f));
                cube.draw(shader, blade, fanColor);
            }
        }

        // ==================== INTERIOR LIGHTS ====================
        glm::vec3 currentLightColor = lightOn ? lightColor : lightOffColor;
        
        // LED strip lights along ceiling (both sides)
        for (int side = 0; side < 2; side++) {
            float zLight = (side == 0) ? -0.8f : 0.8f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.95f, zLight));
            model = glm::scale(model, glm::vec3(8.0f, 0.05f, 0.15f));
            cube.draw(shader, model, currentLightColor);
        }

        // Center dome light
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.95f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.08f, 0.5f));
        cylinder.draw(shader, model, currentLightColor);

        // ==================== ENTRY STEPS ====================
        // Steps visible when door is open
        if (frontDoorAngle > 45.0f) {
            // Step 1 (lowest)
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, -1.2f, 1.8f));
            model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.5f));
            cube.draw(shader, model, metalColor);
            
            // Step 2
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, -0.9f, 1.6f));
            model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.5f));
            cube.draw(shader, model, metalColor);
        }
    }

    // ==================== JET ENGINE (rear mounted) ====================
    void drawJetEngine(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;
        glm::vec3 metalColor = glm::vec3(0.7f, 0.7f, 0.75f);

        // --- Engine Housing: large cylinder protruding from rear ---
        // Bus body rear face is at X = +5.0. Engine extends backward from there.
        // Centered vertically at bus body center (Y=0.5) and horizontally (Z=0)
        glm::mat4 engineBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.8f, 0.5f, 0.0f));

        // Main engine casing — horizontal cylinder pointing backward (+X)
        model = engineBase;
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f)); // lay along X
        model = glm::scale(model, glm::vec3(1.4f, 1.8f, 1.4f));
        cylinder.draw(shader, model, jetHousingColor);

        // Engine intake ring (front of engine, where it meets bus body)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.5f, 0.3f, 1.5f));
        cylinder.draw(shader, model, jetInnerRingColor);

        // Nozzle cone — narrower cylinder at exhaust end
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.9f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(1.1f, 0.4f, 1.1f));
        cylinder.draw(shader, model, jetNozzleColor);

        // Nozzle ring — torus around the exhaust opening
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(7.1f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(2.8f, 2.8f, 2.8f));
        torus.draw(shader, model, jetNozzleColor);

        // Inner exhaust cone (darker, visible inside the nozzle)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.5f, 0.5f, 0.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.5f, 1.2f, 0.5f));
        cylinder.draw(shader, model, glm::vec3(0.15f, 0.15f, 0.18f));

        // --- Support struts connecting engine to bus body ---
        // Top strut
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 1.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.3f));
        cube.draw(shader, model, metalColor);

        // Bottom strut
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, -0.4f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.15f, 0.3f));
        cube.draw(shader, model, metalColor);

        // Left strut
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 0.5f, -0.9f));
        model = glm::scale(model, glm::vec3(0.8f, 0.3f, 0.15f));
        cube.draw(shader, model, metalColor);

        // Right strut
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(5.3f, 0.5f, 0.9f));
        model = glm::scale(model, glm::vec3(0.8f, 0.3f, 0.15f));
        cube.draw(shader, model, metalColor);

        // --- Decorative fin on top of engine housing ---
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(6.0f, 1.5f, 0.0f));
        model = glm::scale(model, glm::vec3(1.5f, 0.4f, 0.08f));
        cube.draw(shader, model, jetHousingColor);

        // --- JET FLAME (only when engine is on) ---
        // Uses emissive shader mode + additive blending for realistic glow
        if (jetEngineOn) {
            // Enable additive blending for flame glow
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // Additive: colors add up = glow
            glDepthMask(GL_FALSE);               // Don't write depth for transparent flames

            // Set emissive mode on shader
            shader.setBool("isEmissive", true);

            float t = jetFlameFlicker;  // Time variable
            float nozzleX = 7.15f;     // Exhaust exit point

            // --- Afterburner glow disc at nozzle exit (hot white-blue core) ---
            float glowPulse = 0.85f + 0.15f * sin(t * 25.0f);
            shader.setFloat("alpha", 0.9f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(nozzleX, 0.5f, 0.0f));
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, glm::vec3(0.95f * glowPulse, 0.08f, 0.95f * glowPulse));
            cylinder.draw(shader, model, glm::vec3(1.0f, 0.95f, 0.85f));  // Near-white hot

            // --- Multiple flame layers with turbulent offsets ---
            // Each layer has: length, radius, color, alpha, frequency offset
            struct FlameLayer {
                float lengthScale;   // Multiplier for base length
                float radiusScale;   // Multiplier for radius
                float freqOffset;    // Unique frequency for turbulence
                float alphaVal;      // Transparency
                glm::vec3 color;     // Flame color
            };

            FlameLayer layers[] = {
                // Inner core: thin, long, bright white-yellow, high alpha
                { 1.0f,  0.20f, 0.0f, 0.95f, glm::vec3(1.0f, 0.97f, 0.85f) },
                { 0.92f, 0.28f, 2.1f, 0.85f, glm::vec3(1.0f, 0.92f, 0.55f) },
                // Hot core: yellow
                { 0.85f, 0.38f, 4.3f, 0.75f, flameColorCore },
                { 0.78f, 0.45f, 6.7f, 0.65f, glm::vec3(1.0f, 0.75f, 0.2f) },
                // Mid zone: orange  
                { 0.68f, 0.55f, 8.9f, 0.55f, flameColorMid },
                { 0.60f, 0.65f, 11.3f, 0.45f, glm::vec3(1.0f, 0.4f, 0.08f) },
                // Outer zone: red-orange, wider, shorter
                { 0.50f, 0.78f, 13.7f, 0.35f, flameColorOuter },
                { 0.40f, 0.90f, 16.1f, 0.25f, glm::vec3(0.8f, 0.15f, 0.03f) },
                // Outermost wispy layer
                { 0.30f, 1.05f, 18.9f, 0.15f, glm::vec3(0.5f, 0.08f, 0.02f) },
            };

            int numLayers = sizeof(layers) / sizeof(layers[0]);

            for (int i = 0; i < numLayers; i++) {
                FlameLayer& L = layers[i];

                // Turbulent length: base + multi-frequency noise
                float baseLen = 3.0f * L.lengthScale;
                float turbulence = 0.5f * sin(t * (14.0f + L.freqOffset))
                                 + 0.25f * sin(t * (21.0f + L.freqOffset * 0.7f))
                                 + 0.15f * sin(t * (33.0f + L.freqOffset * 1.3f));
                float len = baseLen + turbulence * L.lengthScale;
                if (len < 0.2f) len = 0.2f;

                // Turbulent radius
                float rad = L.radiusScale * (0.45f + 0.06f * sin(t * (17.0f + L.freqOffset * 0.5f)));

                // Slight Y/Z offset for organic wobble
                float yOff = 0.03f * sin(t * (9.0f + L.freqOffset * 0.3f));
                float zOff = 0.03f * sin(t * (7.0f + L.freqOffset * 0.6f));

                shader.setFloat("alpha", L.alphaVal);
                model = parent * glm::translate(glm::mat4(1.0f),
                    glm::vec3(nozzleX + len * 0.5f, 0.5f + yOff, zOff));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, glm::vec3(rad, len, rad));
                cylinder.draw(shader, model, L.color);
            }

            // --- Flickering hot sparks (tiny bright dots scattered in flame) ---
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

            // --- Restore normal rendering state ---
            shader.setBool("isEmissive", false);
            shader.setFloat("alpha", 1.0f);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }

    // ==================== HOVER SKIRTS / PADS ====================
    void drawHoverSkirts(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;

        // Four hover pads at the positions where wheels used to be
        float padPositions[4][2] = {
            {-3.5f, -1.3f},   // Front Left
            {-3.5f,  1.3f},   // Front Right
            { 3.5f, -1.3f},   // Rear Left
            { 3.5f,  1.3f}    // Rear Right
        };

        // Hover glow pulsation
        float glowPulse = 0.8f + 0.2f * sin(hoverTime * 5.0f);
        float padBrightness = 0.7f + 0.3f * sin(hoverTime * 3.0f);

        // Draw metallic housings first (normal lighting)
        for (int i = 0; i < 4; i++) {
            float px = padPositions[i][0];
            float pz = padPositions[i][1];

            // Hover pad housing — flat metallic disc (normal shading)
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.1f, pz));
            model = glm::scale(model, glm::vec3(1.0f, 0.15f, 0.8f));
            cylinder.draw(shader, model, jetHousingColor);
        }

        // Now draw glowing elements with emissive + additive blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glDepthMask(GL_FALSE);
        shader.setBool("isEmissive", true);

        for (int i = 0; i < 4; i++) {
            float px = padPositions[i][0];
            float pz = padPositions[i][1];

            // Hover glow disc — bright blue
            shader.setFloat("alpha", 0.7f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.25f, pz));
            model = glm::scale(model, glm::vec3(0.85f * glowPulse, 0.06f, 0.65f * glowPulse));
            cylinder.draw(shader, model, hoverPadColor * padBrightness);

            // Glow ring — torus around the pad
            shader.setFloat("alpha", 0.5f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.2f, pz));
            model = glm::scale(model, glm::vec3(2.0f * glowPulse, 1.5f * glowPulse, 2.0f * glowPulse));
            torus.draw(shader, model, hoverGlowColor * padBrightness);

            // Inner glow core — smaller, brighter
            shader.setFloat("alpha", 0.85f);
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(px, -1.3f, pz));
            model = glm::scale(model, glm::vec3(0.35f, 0.04f, 0.35f));
            cylinder.draw(shader, model, glm::vec3(0.6f, 0.85f, 1.0f) * glowPulse);
        }

        // Underside glow strip
        float bellyGlow = 0.6f + 0.15f * sin(hoverTime * 4.0f);
        shader.setFloat("alpha", 0.4f);
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.15f, 0.0f));
        model = glm::scale(model, glm::vec3(8.0f, 0.04f, 1.0f));
        cube.draw(shader, model, hoverPadColor * bellyGlow);

        // Restore normal state
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

    // Update jet flame animation (call every frame)
    void updateJetFlame(float deltaTime) {
        hoverTime += deltaTime;
        if (jetEngineOn) {
            jetFlameFlicker += deltaTime * 8.0f;  // Fast flickering
            if (jetFlameFlicker > 100.0f) jetFlameFlicker -= 100.0f;
        }
        // Hover bob — subtle sine wave
        hoverBobOffset = 0.15f * sin(hoverTime * 2.5f);
    }

    // Legacy wheel update — kept for compatibility, does nothing meaningful now
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
