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
    float wheelRotation = 0.0f;    // Wheel rotation angle (degrees)
    float steeringAngle = 0.0f;    // Front wheel steering angle (-45 to 45)
    bool lightOn = true;

    // Colors
    glm::vec3 bodyColor = glm::vec3(0.9f, 0.9f, 0.9f);
    glm::vec3 roofColor = glm::vec3(0.95f, 0.95f, 0.95f);
    glm::vec3 windowColor = glm::vec3(0.3f, 0.5f, 0.7f);
    glm::vec3 doorColor = glm::vec3(0.7f, 0.7f, 0.7f);
    glm::vec3 wheelColor = glm::vec3(0.15f, 0.15f, 0.15f);
    glm::vec3 seatColor = glm::vec3(0.2f, 0.3f, 0.6f);
    glm::vec3 floorColor = glm::vec3(0.4f, 0.35f, 0.3f);
    glm::vec3 steeringColor = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 dashboardColor = glm::vec3(0.25f, 0.25f, 0.25f);
    glm::vec3 fanColor = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.8f);
    glm::vec3 lightOffColor = glm::vec3(0.3f, 0.3f, 0.3f);

    void init() {
        cube.init();
        cylinder.init(36);
        torus.init(0.3f, 0.05f, 24, 12);
    }

    void draw(const Shader& shader, glm::mat4 parentTransform) {
        drawExterior(shader, parentTransform);
        drawInterior(shader, parentTransform);
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

        // ==================== WHEELS (Cylinders with rotation) ====================
        glm::vec3 tireColor = glm::vec3(0.1f, 0.1f, 0.1f);       // Black tire
        glm::vec3 rimColor = glm::vec3(0.6f, 0.6f, 0.65f);       // Silver rim
        glm::vec3 hubColor = glm::vec3(0.4f, 0.4f, 0.45f);       // Dark silver hub

        // Helper lambda to draw a complete wheel with rotation and steering
        auto drawWheel = [&](float x, float z, bool isFront) {
            // Apply steering only to front wheels
            float steer = isFront ? steeringAngle : 0.0f;
            
            // Base wheel transform with steering rotation
            glm::mat4 wheelBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(x, -1.0f, z));
            wheelBase = glm::rotate(wheelBase, glm::radians(steer), glm::vec3(0.0f, 1.0f, 0.0f));

            // Tire (outer black cylinder)
            model = wheelBase;
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Lay flat
            model = glm::rotate(model, glm::radians(wheelRotation), glm::vec3(0.0f, 1.0f, 0.0f));  // Spin
            model = glm::scale(model, glm::vec3(1.2f, 0.35f, 1.2f));
            cylinder.draw(shader, model, tireColor);

            // Rim (inner silver cylinder) - both sides
            for (float side : {-0.18f, 0.18f}) {
                model = wheelBase * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, side));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(wheelRotation), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.9f, 0.05f, 0.9f));
                cylinder.draw(shader, model, rimColor);
            }

            // Hub cap (center) - both sides
            for (float side : {-0.19f, 0.19f}) {
                model = wheelBase * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, side));
                model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(wheelRotation), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(0.3f, 0.06f, 0.3f));
                cylinder.draw(shader, model, hubColor);
            }
        };

        // Draw all 4 wheels
        drawWheel(-3.5f, -1.7f, true);   // Front Left (Steering)
        drawWheel(-3.5f, 1.7f, true);    // Front Right (Steering)
        drawWheel(3.5f, -1.7f, false);   // Rear Left
        drawWheel(3.5f, 1.7f, false);    // Rear Right

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

    // Update wheel rotation based on movement (call with positive speed for forward, negative for backward)
    void updateWheels(float movementSpeed) {
        // Wheel circumference affects rotation speed
        // Assuming wheel radius ~0.6 units, circumference = 2 * PI * 0.6 = ~3.77
        // Convert linear movement to angular rotation
        float rotationSpeed = movementSpeed * 100.0f;  // Scale factor for visible rotation
        wheelRotation += rotationSpeed;
        
        // Keep in range
        if (wheelRotation > 360.0f) wheelRotation -= 360.0f;
        if (wheelRotation < -360.0f) wheelRotation += 360.0f;
    }

    void cleanup() {
        cube.cleanup();
        cylinder.cleanup();
        torus.cleanup();
    }
};

#endif
