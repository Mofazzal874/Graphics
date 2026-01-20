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

// Camera - Bird's Eye View (orbiting around bus)
float cameraDistance = 20.0f;
float cameraHeight = 10.0f;
float cameraAngleHorizontal = 0.0f;  // Horizontal rotation angle (degrees)
float cameraPitch = 0.0f;            // Pitch angle (degrees)
float cameraYaw = 0.0f;              // Yaw angle (degrees)  
float cameraRoll = 0.0f;             // Roll angle (degrees)
glm::vec3 cameraLookAt = glm::vec3(0.0f, 1.0f, 0.0f);  // Center point (bus center)

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Global objects
Bus bus;
bool fanSpinning = false;

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// Calculate camera position based on orbit parameters
glm::vec3 getCameraPosition() {
    float angleRad = glm::radians(cameraAngleHorizontal);
    float x = cameraDistance * sin(angleRad);
    float z = cameraDistance * cos(angleRad);
    float y = cameraHeight;
    return glm::vec3(x, y, z);
}

// Calculate view matrix with pitch, yaw, roll rotations
glm::mat4 getViewMatrix() {
    glm::vec3 cameraPos = getCameraPosition();
    
    // Start with look-at matrix
    glm::mat4 view = glm::lookAt(cameraPos, cameraLookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    
    // Apply pitch (X-axis rotation), yaw (Y-axis rotation), roll (Z-axis rotation)
    // These are applied in camera space, so we post-multiply
    glm::mat4 pitchMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraPitch), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 yawMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraYaw), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRoll), glm::vec3(0.0f, 0.0f, 1.0f));
    
    return rollMat * pitchMat * yawMat * view;
}

int main()
{
    // GLFW init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Bus Simulation - Bird's Eye View", NULL, NULL);
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

    std::cout << "=== Bus Simulation Controls ===" << std::endl;
    std::cout << "Camera Orbit: F (rotate around bus)" << std::endl;
    std::cout << "Pitch: X  |  Yaw: Y  |  Roll: Z" << std::endl;
    std::cout << "Move: W/S (forward/back), A/D (left/right), E/R (up/down)" << std::endl;
    std::cout << "Door: 1  |  Fan: G  |  Light: L" << std::endl;
    std::cout << "Windows: 3-8" << std::endl;
    std::cout << "===============================" << std::endl;

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

        // Matrices - Bird's Eye View
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = getViewMatrix();
        glm::vec3 cameraPos = getCameraPosition();
        
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

    float speed = 30.0f * deltaTime;
    float moveSpeed = 10.0f * deltaTime;

    // Camera orbit rotation (F key - rotate around bus 360 degrees)
    if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS)
        cameraAngleHorizontal += speed;

    // Pitch rotation (X key)
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS)
        cameraPitch += speed;

    // Yaw rotation (Y key)
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS)
        cameraYaw += speed;

    // Roll rotation (Z key)
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)
        cameraRoll += speed;

    // Flying simulator movement
    // W/S - zoom in/out (move closer/farther)
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraDistance -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraDistance += moveSpeed;
    
    // A/D - strafe the look-at point left/right
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraLookAt.x -= moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraLookAt.x += moveSpeed;
    
    // E/R - move up/down
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        cameraHeight += moveSpeed;
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        cameraHeight -= moveSpeed;

    // Clamp values
    if (cameraDistance < 5.0f) cameraDistance = 5.0f;
    if (cameraDistance > 50.0f) cameraDistance = 50.0f;
    if (cameraHeight < 1.0f) cameraHeight = 1.0f;
    if (cameraHeight > 30.0f) cameraHeight = 30.0f;
    
    // Keep angles in range
    if (cameraAngleHorizontal > 360.0f) cameraAngleHorizontal -= 360.0f;
    if (cameraAngleHorizontal < 0.0f) cameraAngleHorizontal += 360.0f;
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