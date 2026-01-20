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

        // ==================== MAIN BODY ====================
        // Lower chassis
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(8.0f, 2.0f, 3.0f));
        cube.draw(shader, model, bodyColor);

        // Upper body (passenger area)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 1.75f, 0.0f));
        model = glm::scale(model, glm::vec3(7.0f, 1.5f, 2.8f));
        cube.draw(shader, model, glm::vec3(0.85f, 0.85f, 0.85f));

        // Front cabin (driver area)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, 0.5f, 0.0f));
        model = glm::scale(model, glm::vec3(1.5f, 1.8f, 2.8f));
        cube.draw(shader, model, glm::vec3(0.88f, 0.88f, 0.88f));

        // Roof
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 2.7f, 0.0f));
        model = glm::scale(model, glm::vec3(7.2f, 0.3f, 3.0f));
        cube.draw(shader, model, roofColor);

        // ==================== WHEELS (Cylinders) ====================
        // Front Left
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, -1.0f, -1.7f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.2f, 0.4f, 1.2f));
        cylinder.draw(shader, model, wheelColor);

        // Front Right
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, -1.0f, 1.7f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.2f, 0.4f, 1.2f));
        cylinder.draw(shader, model, wheelColor);

        // Rear Left
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, -1.0f, -1.7f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.2f, 0.4f, 1.2f));
        cylinder.draw(shader, model, wheelColor);

        // Rear Right
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(3.0f, -1.0f, 1.7f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.2f, 0.4f, 1.2f));
        cylinder.draw(shader, model, wheelColor);

        // ==================== WINDOWS ====================
        // Left side windows (6)
        for (int i = 0; i < 6; i++) {
            float yOffset = windowOpenAmount[i] * 0.4f; // slide down when open
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f + i * 1.2f, 1.8f - yOffset, -1.41f));
            model = glm::scale(model, glm::vec3(1.0f, 0.9f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Right side windows (6)
        for (int i = 0; i < 6; i++) {
            float yOffset = windowOpenAmount[6 + i] * 0.4f;
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-2.0f + i * 1.2f, 1.8f - yOffset, 1.41f));
            model = glm::scale(model, glm::vec3(1.0f, 0.9f - yOffset, 0.05f));
            cube.draw(shader, model, windowColor);
        }

        // Front windshield
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.26f, 1.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.5f, 2.2f));
        cube.draw(shader, model, windowColor);

        // Rear window
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(4.01f, 1.8f, 0.0f));
        model = glm::scale(model, glm::vec3(0.05f, 1.2f, 2.0f));
        cube.draw(shader, model, windowColor);

        // ==================== DOORS ====================
        // Front door (hinged on left edge, opens outward)
        glm::mat4 frontDoorPivot = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-3.5f, 0.0f, 1.5f));
        frontDoorPivot = glm::rotate(frontDoorPivot, glm::radians(frontDoorAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::translate(frontDoorPivot, glm::vec3(0.5f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(1.0f, 1.8f, 0.08f));
        cube.draw(shader, model, doorColor);

        // ==================== HEADLIGHTS & TAILLIGHTS ====================
        // Headlights (front)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.3f, 0.2f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.3f, 0.4f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-5.3f, 0.2f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.3f, 0.4f));
        cube.draw(shader, model, glm::vec3(1.0f, 1.0f, 0.7f));

        // Taillights (rear)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(4.01f, 0.2f, -1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.3f, 0.4f));
        cube.draw(shader, model, glm::vec3(0.8f, 0.1f, 0.1f));

        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(4.01f, 0.2f, 1.0f));
        model = glm::scale(model, glm::vec3(0.1f, 0.3f, 0.4f));
        cube.draw(shader, model, glm::vec3(0.8f, 0.1f, 0.1f));
    }

    void drawInterior(const Shader& shader, glm::mat4 parent) {
        glm::mat4 model;

        // ==================== FLOOR ====================
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.9f, 0.0f));
        model = glm::scale(model, glm::vec3(6.8f, 0.1f, 2.6f));
        cube.draw(shader, model, floorColor);

        // ==================== SEATS (2 rows on each side) ====================
        float seatY = -0.3f;
        
        // Left row of seats
        for (int i = 0; i < 5; i++) {
            // Seat base
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f + i * 1.1f, seatY, -0.9f));
            model = glm::scale(model, glm::vec3(0.8f, 0.4f, 0.8f));
            cube.draw(shader, model, seatColor);
            // Seat back
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f + i * 1.1f, seatY + 0.5f, -1.15f));
            model = glm::scale(model, glm::vec3(0.8f, 0.6f, 0.15f));
            cube.draw(shader, model, seatColor);
        }

        // Right row of seats
        for (int i = 0; i < 5; i++) {
            // Seat base
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f + i * 1.1f, seatY, 0.9f));
            model = glm::scale(model, glm::vec3(0.8f, 0.4f, 0.8f));
            cube.draw(shader, model, seatColor);
            // Seat back
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f + i * 1.1f, seatY + 0.5f, 1.15f));
            model = glm::scale(model, glm::vec3(0.8f, 0.6f, 0.15f));
            cube.draw(shader, model, seatColor);
        }

        // ==================== DRIVER AREA ====================
        // Dashboard
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.8f, 0.3f, 0.0f));
        model = glm::scale(model, glm::vec3(0.5f, 0.8f, 2.0f));
        cube.draw(shader, model, dashboardColor);

        // Driver seat
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, seatY, -0.5f));
        model = glm::scale(model, glm::vec3(0.9f, 0.5f, 0.9f));
        cube.draw(shader, model, seatColor);
        // Driver seat back
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.0f, seatY + 0.6f, -0.9f));
        model = glm::scale(model, glm::vec3(0.9f, 0.7f, 0.15f));
        cube.draw(shader, model, seatColor);

        // ==================== STEERING WHEEL ====================
        // Steering column (cylinder)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.5f, 0.6f, -0.5f));
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(0.08f, 0.5f, 0.08f));
        cylinder.draw(shader, model, steeringColor);

        // Steering wheel (torus)
        model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-4.35f, 0.85f, -0.5f));
        model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
        torus.draw(shader, model, steeringColor);

        // ==================== CEILING FAN ====================
        glm::mat4 fanBase = parent * glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, 2.4f, 0.0f));
        fanBase = glm::rotate(fanBase, glm::radians(fanRotation), glm::vec3(0.0f, 1.0f, 0.0f));

        // Fan hub
        model = glm::scale(fanBase, glm::vec3(0.2f, 0.15f, 0.2f));
        cylinder.draw(shader, model, glm::vec3(0.3f, 0.3f, 0.3f));

        // Fan blades (4 blades)
        for (int i = 0; i < 4; i++) {
            glm::mat4 blade = glm::rotate(fanBase, glm::radians(90.0f * i), glm::vec3(0.0f, 1.0f, 0.0f));
            blade = glm::translate(blade, glm::vec3(0.4f, 0.0f, 0.0f));
            blade = glm::scale(blade, glm::vec3(0.6f, 0.05f, 0.15f));
            cube.draw(shader, blade, fanColor);
        }

        // ==================== INTERIOR LIGHTS ====================
        glm::vec3 currentLightColor = lightOn ? lightColor : lightOffColor;
        
        // Ceiling lights (3 along the bus)
        for (int i = 0; i < 3; i++) {
            model = parent * glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f + i * 2.0f, 2.45f, 0.0f));
            model = glm::scale(model, glm::vec3(0.8f, 0.1f, 0.3f));
            cube.draw(shader, model, currentLightColor);
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

    void cleanup() {
        cube.cleanup();
        cylinder.cleanup();
        torus.cleanup();
    }
};

#endif
