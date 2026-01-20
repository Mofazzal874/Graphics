#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
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
    std::cout << "--- Orbit Mode ---" << std::endl;
    std::cout << "  F = Rotate around bus (hold)" << std::endl;
    std::cout << "  Shift+F = Opposite direction" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Bus Controls ---" << std::endl;
    std::cout << "  1 = Toggle Door" << std::endl;
    std::cout << "  G = Toggle Fan" << std::endl;
    std::cout << "  L = Toggle Lights" << std::endl;
    std::cout << "  3-8 = Toggle Windows" << std::endl;
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

        glClearColor(0.53f, 0.81f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // Matrices
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = getViewMatrix();
        
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
        ourShader.setVec3("lightPos", glm::vec3(10.0f, 15.0f, 10.0f));
        ourShader.setVec3("viewPos", cameraPos);

        // Draw bus with identity parent transform
        glm::mat4 busTransform = glm::mat4(1.0f);
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

    float rotSpeed = 50.0f * deltaTime;  // Rotation speed
    float moveSpeed = 8.0f * deltaTime;  // Movement speed

    // ========================================
    // ROTATION CONTROLS (Pitch/Yaw/Roll)
    // ========================================
    
    // Pitch rotation (X key) - look up/down
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraPitch -= rotSpeed;  // Look down
        else
            cameraPitch += rotSpeed;  // Look up
        
        // Clamp pitch to prevent flipping
        if (cameraPitch > 89.0f) cameraPitch = 89.0f;
        if (cameraPitch < -89.0f) cameraPitch = -89.0f;
    }

    // Yaw rotation (Y key) - look left/right
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraYaw -= rotSpeed;  // Look left
        else
            cameraYaw += rotSpeed;  // Look right
    }

    // Roll rotation (Z key) - tilt head
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            cameraRoll -= rotSpeed;  // Tilt left
        else
            cameraRoll += rotSpeed;  // Tilt right
    }

    // ========================================
    // FLYING SIMULATOR MOVEMENT (W/S/A/D/E/R)
    // ========================================
    glm::vec3 front = getCameraFront();
    glm::vec3 right = getCameraRight();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);  // World up for E/R

    // W = Move forward (in direction camera is facing)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += front * moveSpeed;
    
    // S = Move backward
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= front * moveSpeed;
    
    // A = Strafe left
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= right * moveSpeed;
    
    // D = Strafe right
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += right * moveSpeed;
    
    // E = Move up (world up)
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraPos += up * moveSpeed;
    
    // R = Move down (world down)
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cameraPos -= up * moveSpeed;

    // ========================================
    // ORBIT MODE (F key - rotate around bus)
    // ========================================
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) {
        float orbitSpeed = 60.0f * deltaTime;
        
        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || 
            glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS)
            orbitAngle -= orbitSpeed;  // Counter-clockwise
        else
            orbitAngle += orbitSpeed;  // Clockwise
        
        // Keep angle in range
        if (orbitAngle > 360.0f) orbitAngle -= 360.0f;
        if (orbitAngle < 0.0f) orbitAngle += 360.0f;
        
        // Calculate new position on orbit circle using FIXED radius
        // Use orbitRadius (constant 20.0f) instead of dynamic distance
        cameraPos.x = orbitCenter.x + orbitRadius * sin(glm::radians(orbitAngle));
        cameraPos.z = orbitCenter.z + orbitRadius * cos(glm::radians(orbitAngle));
        cameraPos.y = orbitHeight;  // Maintain fixed height
        
        // Update yaw to face the center
        cameraYaw = -orbitAngle - 90.0f;
        
        // Reset pitch to look slightly down at bus
        cameraPitch = -20.0f;
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS) {
        switch (key) {
            case GLFW_KEY_1:
                bus.toggleFrontDoor();
                break;
            case GLFW_KEY_G:
                fanSpinning = !fanSpinning;
                break;
            case GLFW_KEY_L:
                bus.toggleLight();
                break;
            // Window toggles
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