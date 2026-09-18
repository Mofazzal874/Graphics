#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "Globals.h"
#include "Camera.h"
#include "Collision.h"
#include "TextureLoader.h"

// ============================================================================
// MOUSE CALLBACK - look around (FPS-style)
// ============================================================================
inline void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!mouseCaptured) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = (xpos - lastMouseX) * mouseSensitivity;
    float yoffset = (lastMouseY - ypos) * mouseSensitivity;
    lastMouseX = xpos;
    lastMouseY = ypos;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

// ============================================================================
// SCROLL CALLBACK - zoom in/out (change FOV)
// ============================================================================
inline void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraFOV -= (float)yoffset * 2.0f;
    if (cameraFOV < 15.0f) cameraFOV = 15.0f;
    if (cameraFOV > 90.0f) cameraFOV = 90.0f;
}

// ============================================================================
// PROCESS INPUT - continuous key handling
// ============================================================================
inline void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // ================================================================
    // WASD always drives the bus
    // ================================================================
    {
        float appliedAcc = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) appliedAcc = ACCELERATION;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) appliedAcc = -ACCELERATION;

        glm::vec3 forwardDir = getBusForward();
        if (appliedAcc != 0.0f)
            busSpeed += appliedAcc * deltaTime;
        else {
            if (busSpeed > 0) busSpeed = std::max(0.0f, busSpeed - DECELERATION * deltaTime);
            if (busSpeed < 0) busSpeed = std::min(0.0f, busSpeed + DECELERATION * deltaTime);
        }
        busSpeed = glm::clamp(busSpeed, -MAX_SPEED, MAX_SPEED);

        float turnInput = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) turnInput = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) turnInput = -1.0f;
        if (turnInput != 0.0f)
            busSteerAngle += turnInput * STEER_SPEED * deltaTime;
        else {
            if (busSteerAngle > 0) busSteerAngle = std::max(0.0f, busSteerAngle - STEER_SPEED * deltaTime);
            if (busSteerAngle < 0) busSteerAngle = std::min(0.0f, busSteerAngle + STEER_SPEED * deltaTime);
        }
        busSteerAngle = glm::clamp(busSteerAngle, -MAX_STEER, MAX_STEER);
        if (busSpeed != 0.0f) busYaw += busSteerAngle * busSpeed * deltaTime * 0.1f;

        glm::vec3 newPos = busPosition + forwardDir * busSpeed * deltaTime;
        if (!checkBusCollision(newPos)) {
            busPosition = newPos;
        } else {
            busSpeed *= -0.3f;
            if (buildingHitCooldown <= 0.0f) {
                awardScore(-2);
                buildingHitCooldown = 0.6f;
                std::cout << ">> HIT! -2 (Score: " << gameScore << ") <<" << std::endl;
            }
        }

        bus.steeringAngle = busSteerAngle;
        bus.jetEngineOn = true;

        // Up/Down hover control
        float vertInput = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) vertInput = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) vertInput = -1.0f;
        if (vertInput != 0.0f)
            busVerticalSpeed += vertInput * VERTICAL_ACCEL * deltaTime;
        else {
            if (busVerticalSpeed > 0) busVerticalSpeed = std::max(0.0f, busVerticalSpeed - VERTICAL_ACCEL * 0.7f * deltaTime);
            if (busVerticalSpeed < 0) busVerticalSpeed = std::min(0.0f, busVerticalSpeed + VERTICAL_ACCEL * 0.7f * deltaTime);
        }
        busVerticalSpeed = glm::clamp(busVerticalSpeed, -15.0f, 15.0f);
        busAltitude += busVerticalSpeed * deltaTime;
        busAltitude = glm::clamp(busAltitude, 0.0f, MAX_ALTITUDE);
    }

    // ================================================================
    // FREE CAMERA movement with Arrow Keys (only in free cam mode)
    // ================================================================
    if (!isDrivingMode && cameraMode == 0) {
        float camSpeed = 15.0f * deltaTime;
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) camSpeed *= 2.5f;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cameraPos += camSpeed * getCameraFront();
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cameraPos -= camSpeed * getCameraFront();
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cameraPos -= getCameraRight() * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cameraPos += getCameraRight() * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += glm::vec3(0, 1, 0) * camSpeed;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) cameraPos -= glm::vec3(0, 1, 0) * camSpeed;

        // Orbit (hold F)
        if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
            orbitAngle += 50.0f * deltaTime;
            if (orbitAngle > 360.0f) orbitAngle -= 360.0f;
            cameraPos.x = busPosition.x + orbitRadius * sin(glm::radians(orbitAngle));
            cameraPos.z = busPosition.z + orbitRadius * cos(glm::radians(orbitAngle));
            cameraPos.y = busPosition.y + orbitHeight;
            glm::vec3 dir = glm::normalize(busPosition - cameraPos);
            cameraYaw = glm::degrees(atan2(dir.z, dir.x));
            cameraPitch = -20.0f;
        }
    }
}

// ============================================================================
// KEY CALLBACK - discrete key presses
// ============================================================================
inline void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        // --- CAMERA ---
        case GLFW_KEY_V:
            cameraMode = (cameraMode + 1) % NUM_CAMERA_MODES;
            std::cout << "Camera: " << cameraModeNames[cameraMode] << std::endl;
            if (cameraMode == 2) {
                cameraYaw = busYaw + 180.0f;
                cameraPitch = 0.0f;
            }
            break;
        case GLFW_KEY_M:
            mouseCaptured = !mouseCaptured;
            glfwSetInputMode(window, GLFW_CURSOR,
                mouseCaptured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            firstMouse = true;
            std::cout << "Mouse: " << (mouseCaptured ? "CAPTURED" : "FREE") << std::endl;
            break;

        // --- TEXTURE ---
        case GLFW_KEY_T:
            sceneTextureMode = (sceneTextureMode + 1) % 5;
            std::cout << "Texture Mode: " << textureModeNames[sceneTextureMode] << std::endl;
            break;
        case GLFW_KEY_8:
            currentWrapIndex = (currentWrapIndex + 1) % NUM_WRAP_MODES;
            updateSceneTextureParams();
            std::cout << "Wrap: " << wrapNames[currentWrapIndex] << std::endl;
            break;
        case GLFW_KEY_9:
            currentFilterIndex = (currentFilterIndex + 1) % NUM_FILTER_MODES;
            updateSceneTextureParams();
            std::cout << "Filter: " << filterNames[currentFilterIndex] << std::endl;
            break;
        case GLFW_KEY_0:
            if (sceneTextureMode != 0) {
                sceneTextureMode = 0;
                bus.texFloor = 0; bus.texCarpet = 0; bus.texFabric = 0;
                bus.texWall = 0; bus.texDashboard = 0; bus.texBusBody = 0;
                std::cout << "All Textures: OFF" << std::endl;
            } else {
                sceneTextureMode = 1;
                bus.texFloor = texFloor; bus.texCarpet = texCarpet;
                bus.texFabric = texFabric; bus.texWall = texWall;
                bus.texDashboard = texDashboard; bus.texBusBody = texBusBody;
                std::cout << "All Textures: ON" << std::endl;
            }
            break;

        // --- LIGHTING ---
        case GLFW_KEY_1: dirLightOn = !dirLightOn;
            std::cout << "Directional: " << (dirLightOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_2: pointLightsOn = !pointLightsOn;
            std::cout << "Point Lights: " << (pointLightsOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_3: spotLightOn = !spotLightOn;
            std::cout << "Spot Light: " << (spotLightOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_4: emissiveLightOn = !emissiveLightOn;
            std::cout << "Emissive: " << (emissiveLightOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_5: ambientOn = !ambientOn;
            std::cout << "Ambient: " << (ambientOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_6: diffuseOn = !diffuseOn;
            std::cout << "Diffuse: " << (diffuseOn ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_7: specularOn = !specularOn;
            std::cout << "Specular: " << (specularOn ? "ON" : "OFF") << std::endl; break;

        // --- BUS ---
        case GLFW_KEY_B: bus.toggleFrontDoor(); break;
        case GLFW_KEY_G: fanSpinning = !fanSpinning; break;
        case GLFW_KEY_L: bus.toggleLight(); break;
        case GLFW_KEY_N: bus.toggleWings();
            std::cout << "Wings: " << (bus.wingsEnabled ? "ON" : "OFF") << std::endl; break;
        case GLFW_KEY_K:
            isDrivingMode = !isDrivingMode;
            if (!isDrivingMode) {
                cameraMode = 0;
                std::cout << "FREE CAM ON | Arrow keys = fly | WASD still drives bus" << std::endl;
            } else {
                cameraMode = 1;
                std::cout << "CHASE CAM | WASD=Drive | V=cycle camera" << std::endl;
            }
            break;

        // --- STATUS ---
        case GLFW_KEY_TAB: printStatus(); break;
    }
}

inline void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Viewport set per-frame
}

#endif
