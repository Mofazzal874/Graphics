#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include "Shader.h"
#include "Bus.h"

// Settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// ============================================================================
// CAMERA SYSTEM - Flying Simulator Style
// ============================================================================
// Camera position in world space
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
glm::vec3 cameraLookAt = glm::vec3(0.0f, 1.0f, 0.0f);  // Camera look-at target

// Camera orientation angles (in degrees)
float cameraPitch = -15.0f;   // Pitch: rotation around X-axis (look up/down)
float cameraYaw = -90.0f;     // Yaw: rotation around Y-axis (look left/right)
float cameraRoll = 0.0f;      // Roll: rotation around Z-axis (tilt head)

// Orbit mode parameters (for F key - rotating around look-at point)
glm::vec3 orbitCenter = glm::vec3(0.0f, 1.0f, 0.0f);  // Bus center
float orbitAngle = 0.0f;
float orbitRadius = 20.0f;
float orbitHeight = 10.0f;
bool orbitMode = false;  // Toggle between free-fly and orbit mode

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Global objects
Bus bus;
bool fanSpinning = false;
float busMovement = 0.0f;  // Track movement for wheel rotation this frame

// ============================================================================
// DRIVING SIMULATION STATE
// ============================================================================
bool isDrivingMode = false;
glm::vec3 busPosition = glm::vec3(0.0f, 0.0f, 0.0f);
float busYaw = 0.0f;        // Bus orientation (degrees)
float busSpeed = 0.0f;      // Current speed
float busSteerAngle = 0.0f; // Current steering angle

// Physics parameters
const float ACCELERATION = 15.0f;
const float DECELERATION = 10.0f;
const float MAX_SPEED = 20.0f;
const float STEER_SPEED = 60.0f;
const float MAX_STEER = 35.0f;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Calculate camera front vector from pitch and yaw
glm::vec3 getCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(front);
}

// Calculate camera right vector
glm::vec3 getCameraRight() {
    glm::vec3 front = getCameraFront();
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    return glm::normalize(glm::cross(front, worldUp));
}

// Calculate camera up vector
glm::vec3 getCameraUp() {
    glm::vec3 front = getCameraFront();
    glm::vec3 right = getCameraRight();
    return glm::normalize(glm::cross(right, front));
}

// Calculate view matrix with roll applied
glm::mat4 getViewMatrix() {
    // In driving mode, use lookAt directly to follow the bus
    if (isDrivingMode) {
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
        return glm::lookAt(cameraPos, cameraLookAt, up);
    }
    
    glm::vec3 front = getCameraFront();
    glm::vec3 right = getCameraRight();
    glm::vec3 up = getCameraUp();
    
    // Apply roll rotation to the up vector
    if (cameraRoll != 0.0f) {
        glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRoll), front);
        up = glm::vec3(rollMat * glm::vec4(up, 0.0f));
    }
    
    return glm::lookAt(cameraPos, cameraPos + front, up);
}

int main()
{
    // GLFW init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bus Simulation - Flying Simulator", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // Build and compile shader program
    Shader ourShader("shader.vert", "shader.frag");

    // Initialize bus
    bus.init();

    std::cout << "========================================" << std::endl;
    std::cout << "       BUS SIMULATION CONTROLS          " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "--- Flying Simulator Movement ---" << std::endl;
    std::cout << "  W = Forward    S = Backward" << std::endl;
    std::cout << "  A = Strafe Left    D = Strafe Right" << std::endl;
    std::cout << "  E = Move Up    R = Move Down" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Camera Rotation ---" << std::endl;
    std::cout << "  X = Pitch (up/down look)" << std::endl;
    std::cout << "  Y = Yaw (left/right look)" << std::endl;
    std::cout << "  Z = Roll (tilt head)" << std::endl;
    std::cout << "  (Hold Shift for opposite direction)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Camera Positions ---" << std::endl;
    std::cout << "  2 = Enter Bus Interior" << std::endl;
    std::cout << "  9 = Driver Seat View" << std::endl;
    std::cout << "  0 = Exit Bus (Outside View)" << std::endl;
    std::cout << "  F = Orbit around bus (hold)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Bus Controls ---" << std::endl;
    std::cout << "  1 = Toggle Door" << std::endl;
    std::cout << "  G = Toggle Fan" << std::endl;
    std::cout << "  L = Toggle Lights" << std::endl;
    std::cout << "  3-8 = Toggle Windows" << std::endl;
    std::cout << "--- Driving Mode ---" << std::endl;
    std::cout << "  K = Toggle Driving Mode ON/OFF" << std::endl;
    std::cout << "  W/S = Gas/Brake" << std::endl;
    std::cout << "  A/D = Steer" << std::endl;
    std::cout << "========================================" << std::endl;

    // Render loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update fan rotation
        bus.updateFan(deltaTime, fanSpinning);
        
        // Note: Wheel rotation is now updated inside processInput when driving

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = getViewMatrix();
        
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("lightPos", glm::vec3(busPosition.x + 10.0f, busPosition.y + 15.0f, busPosition.z + 10.0f)); // Follow light
        ourShader.setVec3("viewPos", cameraPos);

        // Draw bus with Physics Transform
        glm::mat4 busTransform = glm::mat4(1.0f);
        busTransform = glm::translate(busTransform, busPosition);
        busTransform = glm::rotate(busTransform, glm::radians(busYaw), glm::vec3(0.0f, 1.0f, 0.0f));
        // Note: bus model faces -X, physics calculations assume -X is forward, so no extra rotation needed
        
        bus.draw(ourShader, busTransform);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    bus.cleanup();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float dt = deltaTime;

    // ========================================
    // DRIVE MODE LOGIC
    // ========================================
    if (isDrivingMode) {
        // Steering (A/D) - only works effectively when moving, but wheels turn anyway
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
            busSteerAngle += STEER_SPEED * dt;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
            busSteerAngle -= STEER_SPEED * dt;
        
        // Auto-center steering if no key pressed
        if (glfwGetKey(window, GLFW_KEY_A) != GLFW_PRESS && glfwGetKey(window, GLFW_KEY_D) != GLFW_PRESS) {
            if (busSteerAngle > 0.0f) {
                busSteerAngle -= STEER_SPEED * dt;
                if (busSteerAngle < 0.0f) busSteerAngle = 0.0f;
            } else if (busSteerAngle < 0.0f) {
                busSteerAngle += STEER_SPEED * dt;
                if (busSteerAngle > 0.0f) busSteerAngle = 0.0f;
            }
        }
        
        // Clamp steering
        if (busSteerAngle > MAX_STEER) busSteerAngle = MAX_STEER;
        if (busSteerAngle < -MAX_STEER) busSteerAngle = -MAX_STEER;

        // Acceleration/Braking (W/S)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
            busSpeed += ACCELERATION * dt;
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
            busSpeed -= ACCELERATION * dt;
        else {
            // Natural friction/drag
            if (busSpeed > 0.0f) {
                busSpeed -= DECELERATION * dt;
                if (busSpeed < 0.0f) busSpeed = 0.0f;
            } else if (busSpeed < 0.0f) {
                busSpeed += DECELERATION * dt;
                if (busSpeed > 0.0f) busSpeed = 0.0f;
            }
        }

        // Clamp speed
        if (busSpeed > MAX_SPEED) busSpeed = MAX_SPEED;
        if (busSpeed < -MAX_SPEED / 2.0f) busSpeed = -MAX_SPEED / 2.0f; // Slower reverse

        // Apply physics - Update bus yaw based on speed and steering
        // Only turn if moving
        if (std::abs(busSpeed) > 0.1f) {
            float turnFactor = busSpeed * dt * 0.05f;
            busYaw += busSteerAngle * turnFactor;
        }

        // Calculate forward direction (bus model faces -X direction)
        float rad = glm::radians(busYaw);
        glm::vec3 forwardDir = glm::vec3(-cos(rad), 0.0f, -sin(rad));

        // Update position
        busPosition += forwardDir * busSpeed * dt;

        // Add a slight wobble/zigzag to wheels when moving to show motion
        float wobble = 0.0f;
        if (std::abs(busSpeed) > 0.5f) {
            wobble = sin((float)glfwGetTime() * 15.0f) * 2.0f * (std::abs(busSpeed) / MAX_SPEED);
        }
        bus.steeringAngle = busSteerAngle + wobble;
        bus.updateWheels(busSpeed * dt);

        // Chase Camera Logic - Position camera behind and above the bus
        float camDist = 25.0f;
        float camHeight = 8.0f;
        glm::vec3 behindDir = -forwardDir;
        
        cameraPos = busPosition + behindDir * camDist;
        cameraPos.y = busPosition.y + camHeight;
        
        // Look at bus (slightly above ground level for better view)
        cameraLookAt = busPosition + glm::vec3(0.0f, 1.5f, 0.0f);
        
        return; // Skip other camera controls
    }

    // ========================================
    // FREE CAMERA CONTROLS (Standard)
    // ========================================
    float rotSpeed = 50.0f * dt;
    float moveSpeed = 15.0f * dt;

    // Pitch rotation (X key) - look up/down
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraPitch -= rotSpeed;
        else
            cameraPitch += rotSpeed;
        
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    }

    // Yaw rotation (Y key) - look left/right
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraYaw -= rotSpeed;
        else
            cameraYaw += rotSpeed;
    }

    // Roll rotation (Z key) - tilt head
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraRoll -= rotSpeed;
        else
            cameraRoll += rotSpeed;
    }

    // ========================================
    // FLYING SIMULATOR MOVEMENT (W/S/A/D/E/R)
    // ========================================
    glm::vec3 front = getCameraFront();
    glm::vec3 right = getCameraRight();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += front * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= front * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += up * moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cameraPos -= up * moveSpeed;

    // ========================================
    // ORBIT MODE (F key - rotate around bus)
    // ========================================
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        float orbitSpeed = 60.0f * dt;
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            orbitAngle -= orbitSpeed;
        else
            orbitAngle += orbitSpeed;
        
        if (orbitAngle > 360.0f) orbitAngle -= 360.0f;
        if (orbitAngle < 0.0f) orbitAngle += 360.0f;
        
        glm::vec3 currentBusPos = busPosition;
        cameraPos.x = currentBusPos.x + orbitRadius * sin(glm::radians(orbitAngle));
        cameraPos.z = currentBusPos.z + orbitRadius * cos(glm::radians(orbitAngle));
        cameraPos.y = currentBusPos.y + orbitHeight;
        
        glm::vec3 direction = glm::normalize(currentBusPos - cameraPos);
        cameraYaw = glm::degrees(atan2(direction.z, direction.x));
        cameraPitch = -20.0f;
        cameraLookAt = currentBusPos;
    } else if (!isDrivingMode) {
        cameraLookAt = cameraPos + getCameraFront(); 
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                bus.toggleFrontDoor();
                break;
            case GLFW_KEY_2:
                cameraPos = glm::vec3(-3.0f, 0.5f, 0.0f);
                cameraPitch = 0.0f;
                cameraYaw = 90.0f;
                cameraRoll = 0.0f;
                std::cout << "Entered bus interior. Use WASD to move, XYZ to look around." << std::endl;
                break;
            case GLFW_KEY_0:
                cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
                cameraPitch = -15.0f;
                cameraYaw = -90.0f;
                cameraRoll = 0.0f;
                orbitAngle = 0.0f;
                std::cout << "Exited bus. Outside view restored." << std::endl;
                break;
            case GLFW_KEY_K:
                isDrivingMode = !isDrivingMode;
                if (isDrivingMode) {
                    std::cout << "DRIVING MODE: ON. Use W/S to drive, A/D to steer." << std::endl;
                    cameraPitch = -15.0f;
                    cameraRoll = 0.0f;
                } else {
                    std::cout << "DRIVING MODE: OFF. Free camera enabled." << std::endl;
                    busSpeed = 0.0f;
                    busSteerAngle = 0.0f;
                    bus.steeringAngle = 0.0f;
                }
                break;
            case GLFW_KEY_9:
                cameraPos = glm::vec3(-3.5f, 0.5f, -0.6f);
                cameraPitch = -5.0f;
                cameraYaw = -90.0f;
                cameraRoll = 0.0f;
                std::cout << "Driver seat view." << std::endl;
                break;
            case GLFW_KEY_G:
                fanSpinning = !fanSpinning;
                break;
            case GLFW_KEY_L:
                bus.toggleLight();
                break;
            case GLFW_KEY_3:
                bus.toggleWindow(0);
                break;
            case GLFW_KEY_4:
                bus.toggleWindow(1);
                break;
            case GLFW_KEY_5:
                bus.toggleWindow(2);
                break;
            case GLFW_KEY_6:
                bus.toggleWindow(3);
                break;
            case GLFW_KEY_7:
                bus.toggleWindow(4);
                break;
            case GLFW_KEY_8:
                bus.toggleWindow(5);
                break;
        }
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}