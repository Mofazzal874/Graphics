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
// CAMERA SYSTEM
// ============================================================================
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
glm::vec3 cameraLookAt = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraPitch = -15.0f;
float cameraYaw = -90.0f;
float cameraRoll = 0.0f;

// Orbit
glm::vec3 orbitCenter = glm::vec3(0.0f, 1.0f, 0.0f);
float orbitAngle = 0.0f;
float orbitRadius = 20.0f;
float orbitHeight = 10.0f;
bool orbitMode = false;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Bus bus;
bool fanSpinning = false;
float busMovement = 0.0f;

// ============================================================================
// DRIVING SIMULATION
// ============================================================================
bool isDrivingMode = false;
glm::vec3 busPosition = glm::vec3(0.0f, 0.0f, 0.0f);
float busYaw = 0.0f;
float busSpeed = 0.0f;
float busSteerAngle = 0.0f;

const float ACCELERATION = 15.0f;
const float DECELERATION = 10.0f;
const float MAX_SPEED = 20.0f;
const float STEER_SPEED = 60.0f;
const float MAX_STEER = 35.0f;
const float HOVER_HEIGHT = 1.5f;
const float HOVER_BOB_AMP = 0.15f;
const float HOVER_BOB_FREQ = 2.5f;

// ============================================================================
// PER-VIEWPORT STATE
// ============================================================================
// Each viewport has its own lighting and camera mode
struct ViewportState {
    bool dirLightOn = true;
    bool pointLightsOn = true;
    bool spotLightOn = true;
    bool emissiveLightOn = true;
    bool ambientOn = true;
    bool diffuseOn = true;
    bool specularOn = true;
    // Camera mode: 0=Perspective, 1=Top, 2=Front, 3=Side, 4=Isometric, 5=Inside
    int cameraMode = 0;
};

ViewportState viewports[4];
int activeViewport = 0; // Which viewport is currently selected (0-3)

// Camera mode names for display
const char* cameraModeNames[] = {
    "Perspective", "Top View", "Front View", "Side View", "Isometric", "Inside View"
};
const int NUM_CAMERA_MODES = 6;

// ============================================================================
// CUSTOM lookAt IMPLEMENTATION
// ============================================================================
glm::mat4 myLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
    glm::vec3 f = glm::normalize(center - eye);      // Forward
    glm::vec3 s = glm::normalize(glm::cross(f, up));  // Right
    glm::vec3 u = glm::cross(s, f);                   // True Up

    glm::mat4 result(1.0f);
    result[0][0] = s.x;   result[1][0] = s.y;   result[2][0] = s.z;
    result[0][1] = u.x;   result[1][1] = u.y;   result[2][1] = u.z;
    result[0][2] = -f.x;  result[1][2] = -f.y;  result[2][2] = -f.z;
    result[3][0] = -glm::dot(s, eye);
    result[3][1] = -glm::dot(u, eye);
    result[3][2] =  glm::dot(f, eye);
    return result;
}

// Function declarations
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

glm::vec3 getCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(front);
}

glm::vec3 getCameraRight() {
    return glm::normalize(glm::cross(getCameraFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

glm::vec3 getCameraUp() {
    return glm::normalize(glm::cross(getCameraRight(), getCameraFront()));
}

glm::mat4 getViewMatrix() {
    if (isDrivingMode) {
        return myLookAt(cameraPos, cameraLookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    glm::vec3 front = getCameraFront();
    glm::vec3 up = getCameraUp();
    if (cameraRoll != 0.0f) {
        glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRoll), front);
        up = glm::vec3(rollMat * glm::vec4(up, 0.0f));
    }
    return myLookAt(cameraPos, cameraPos + front, up);
}

// Get view matrix for a specific camera mode
glm::mat4 getViewForMode(int mode) {
    glm::vec3 target = busPosition + glm::vec3(0.0f, HOVER_HEIGHT, 0.0f);
    switch (mode) {
        case 0: // Perspective — user-controlled camera
            return getViewMatrix();
        case 1: // Top view
            return myLookAt(busPosition + glm::vec3(0.0f, 25.0f, 0.01f), target,
                            glm::vec3(0.0f, 0.0f, -1.0f));
        case 2: // Front view
            return myLookAt(busPosition + glm::vec3(-20.0f, 3.0f, 0.0f), target,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        case 3: // Side view
            return myLookAt(busPosition + glm::vec3(0.0f, 3.0f, 20.0f), target,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        case 4: // Isometric view
            return myLookAt(busPosition + glm::vec3(15.0f, 12.0f, 15.0f), target,
                            glm::vec3(0.0f, 1.0f, 0.0f));
        case 5: // Inside view
            return myLookAt(glm::vec3(-3.0f, HOVER_HEIGHT + 0.5f, 0.0f),
                            glm::vec3(3.0f, HOVER_HEIGHT + 0.5f, 0.0f),
                            glm::vec3(0.0f, 1.0f, 0.0f));
        default:
            return getViewMatrix();
    }
}

void printViewportStatus() {
    const char* vpNames[] = { "Top-Left", "Top-Right", "Bottom-Left", "Bottom-Right" };
    std::cout << "\n--- Viewport Status ---" << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << (i == activeViewport ? " >> " : "    ");
        std::cout << vpNames[i] << ": Camera=" << cameraModeNames[viewports[i].cameraMode];
        std::cout << " | Dir=" << (viewports[i].dirLightOn ? "ON" : "OFF");
        std::cout << " Pt=" << (viewports[i].pointLightsOn ? "ON" : "OFF");
        std::cout << " Spot=" << (viewports[i].spotLightOn ? "ON" : "OFF");
        std::cout << " Emis=" << (viewports[i].emissiveLightOn ? "ON" : "OFF");
        std::cout << " | A=" << (viewports[i].ambientOn ? "ON" : "OFF");
        std::cout << " D=" << (viewports[i].diffuseOn ? "ON" : "OFF");
        std::cout << " S=" << (viewports[i].specularOn ? "ON" : "OFF");
        std::cout << std::endl;
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Hover Bus - Advanced Lighting & 4-Viewport", NULL, NULL);
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

    Shader ourShader("shader.vert", "shader.frag");
    bus.init();

    // Set default viewport configurations
    viewports[0].cameraMode = 0; // Perspective (combined)
    viewports[1].cameraMode = 1; // Top view
    viewports[2].cameraMode = 2; // Front view
    viewports[3].cameraMode = 3; // Side view

    // Print controls
    std::cout << "============================================" << std::endl;
    std::cout << "    HOVER BUS - ADVANCED LIGHTING SYSTEM     " << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << "--- Viewport Selection ---" << std::endl;
    std::cout << "  TAB : Cycle active viewport (highlighted with >>)" << std::endl;
    std::cout << "    8 : Cycle camera mode for active viewport" << std::endl;
    std::cout << "        (Perspective/Top/Front/Side/Isometric/Inside)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Light Type Toggles (active viewport) ---" << std::endl;
    std::cout << "  1 : Toggle Directional Light" << std::endl;
    std::cout << "  2 : Toggle Point Lights (4 colored)" << std::endl;
    std::cout << "  3 : Toggle Spot Light (flashlight)" << std::endl;
    std::cout << "  4 : Toggle Emissive Light (self-glow)" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Light Component Toggles (active viewport) ---" << std::endl;
    std::cout << "  5 : Toggle Ambient component" << std::endl;
    std::cout << "  6 : Toggle Diffuse component" << std::endl;
    std::cout << "  7 : Toggle Specular component" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "--- Bus Controls ---" << std::endl;
    std::cout << "  K : Driving Mode | W/S/A/D : Move/Steer" << std::endl;
    std::cout << "  B : Door | G : Fan | L : Headlights" << std::endl;
    std::cout << "  WASD/XYZ : Free camera (when not driving)" << std::endl;
    std::cout << "============================================" << std::endl;
    printViewportStatus();

    // ==================== RENDER LOOP ====================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        bus.updateFan(deltaTime, fanSpinning);
        bus.updateJetFlame(deltaTime);

        // Get ACTUAL framebuffer size (handles DPI scaling)
        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        // Clear entire screen
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();

        // ==================== LIGHT SETUP (same for all viewports) ====================
        // Directional Light (Sun)
        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        ourShader.setVec3("dirLight.ambient",  0.15f, 0.15f, 0.15f);
        ourShader.setVec3("dirLight.diffuse",  0.7f, 0.7f, 0.6f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // Point Light 0 (Red — front-right)
        ourShader.setVec3("pointLights[0].position", busPosition + glm::vec3(5.0f, 5.0f, 5.0f));
        ourShader.setVec3("pointLights[0].ambient",  0.05f, 0.0f, 0.0f);
        ourShader.setVec3("pointLights[0].diffuse",  0.8f, 0.1f, 0.1f);
        ourShader.setVec3("pointLights[0].specular", 1.0f, 0.2f, 0.2f);
        ourShader.setFloat("pointLights[0].constant",  1.0f);
        ourShader.setFloat("pointLights[0].linear",    0.09f);
        ourShader.setFloat("pointLights[0].quadratic", 0.032f);

        // Point Light 1 (Green — front-left)
        ourShader.setVec3("pointLights[1].position", busPosition + glm::vec3(-5.0f, 5.0f, 5.0f));
        ourShader.setVec3("pointLights[1].ambient",  0.0f, 0.05f, 0.0f);
        ourShader.setVec3("pointLights[1].diffuse",  0.1f, 0.8f, 0.1f);
        ourShader.setVec3("pointLights[1].specular", 0.2f, 1.0f, 0.2f);
        ourShader.setFloat("pointLights[1].constant",  1.0f);
        ourShader.setFloat("pointLights[1].linear",    0.09f);
        ourShader.setFloat("pointLights[1].quadratic", 0.032f);

        // Point Light 2 (Blue — rear-right)
        ourShader.setVec3("pointLights[2].position", busPosition + glm::vec3(5.0f, 5.0f, -5.0f));
        ourShader.setVec3("pointLights[2].ambient",  0.0f, 0.0f, 0.05f);
        ourShader.setVec3("pointLights[2].diffuse",  0.1f, 0.1f, 0.8f);
        ourShader.setVec3("pointLights[2].specular", 0.2f, 0.2f, 1.0f);
        ourShader.setFloat("pointLights[2].constant",  1.0f);
        ourShader.setFloat("pointLights[2].linear",    0.09f);
        ourShader.setFloat("pointLights[2].quadratic", 0.032f);

        // Point Light 3 (White — rear-left)
        ourShader.setVec3("pointLights[3].position", busPosition + glm::vec3(-5.0f, 5.0f, -5.0f));
        ourShader.setVec3("pointLights[3].ambient",  0.05f, 0.05f, 0.05f);
        ourShader.setVec3("pointLights[3].diffuse",  0.6f, 0.6f, 0.6f);
        ourShader.setVec3("pointLights[3].specular", 0.6f, 0.6f, 0.6f);
        ourShader.setFloat("pointLights[3].constant",  1.0f);
        ourShader.setFloat("pointLights[3].linear",    0.09f);
        ourShader.setFloat("pointLights[3].quadratic", 0.032f);

        // Spot Light (Camera flashlight)
        ourShader.setVec3("spotLight.position", cameraPos);
        ourShader.setVec3("spotLight.direction",
            isDrivingMode ? glm::normalize(cameraLookAt - cameraPos) : getCameraFront());
        ourShader.setVec3("spotLight.ambient",  0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse",  1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant",  1.0f);
        ourShader.setFloat("spotLight.linear",    0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));

        // Material
        ourShader.setFloat("shininess", 32.0f);
        ourShader.setVec3("viewPos", cameraPos);

        // Bus transform (shared across viewports)
        glm::mat4 busTransform = glm::mat4(1.0f);
        glm::vec3 renderPos = busPosition;
        renderPos.y += HOVER_HEIGHT + bus.hoverBobOffset;
        busTransform = glm::translate(busTransform, renderPos);
        busTransform = glm::rotate(busTransform, glm::radians(busYaw), glm::vec3(0.0f, 1.0f, 0.0f));

        // ==================== RENDER 4 VIEWPORTS ====================
        int hw = fbWidth / 2;
        int hh = fbHeight / 2;

        // Viewport positions: [0]=top-left, [1]=top-right, [2]=bottom-left, [3]=bottom-right
        int vpX[] = { 0, hw, 0, hw };
        int vpY[] = { hh, hh, 0, 0 };

        for (int v = 0; v < 4; v++) {
            glViewport(vpX[v], vpY[v], hw, hh);

            // Scissor test to confine clear to this viewport
            glEnable(GL_SCISSOR_TEST);
            glScissor(vpX[v], vpY[v], hw, hh);
            glClear(GL_DEPTH_BUFFER_BIT);
            glDisable(GL_SCISSOR_TEST);

            ViewportState& vs = viewports[v];

            // Set per-viewport light toggles
            ourShader.setBool("dirLightOn",    vs.dirLightOn);
            ourShader.setBool("pointLightsOn", vs.pointLightsOn);
            ourShader.setBool("spotLightOn",   vs.spotLightOn);
            ourShader.setBool("ambientOn",     vs.ambientOn);
            ourShader.setBool("diffuseOn",     vs.diffuseOn);
            ourShader.setBool("specularOn",    vs.specularOn);

            // Emissive default (overridden by Bus draw for flames)
            ourShader.setBool("isEmissive", false);
            ourShader.setFloat("alpha", 1.0f);

            // View & Projection for this viewport
            float aspect = (float)hw / (float)hh;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);
            glm::mat4 view = getViewForMode(vs.cameraMode);

            ourShader.setMat4("projection", projection);
            ourShader.setMat4("view", view);

            // Draw bus
            // If emissive light is toggled off for this viewport, we suppress emissive draws
            // by temporarily disabling it in the bus
            bool savedJetOn = bus.jetEngineOn;
            if (!vs.emissiveLightOn) {
                bus.jetEngineOn = false; // Suppress flame emissive
            }
            bus.draw(ourShader, busTransform);
            bus.jetEngineOn = savedJetOn; // Restore
        }

        // Draw viewport selection indicator (optional: border lines)
        // The active viewport is indicated in the console output

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float speed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        speed *= 2.0f;

    // Driving mode
    if (isDrivingMode) {
        float appliedAcc = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) appliedAcc = ACCELERATION;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) appliedAcc = -ACCELERATION;

        float rad = glm::radians(busYaw);
        glm::vec3 forwardDir(-cos(rad), 0.0f, sin(rad));

        if (appliedAcc != 0.0f)
            busSpeed += appliedAcc * deltaTime;
        else {
            if (busSpeed > 0.0f) busSpeed = std::max(0.0f, busSpeed - DECELERATION * deltaTime);
            if (busSpeed < 0.0f) busSpeed = std::min(0.0f, busSpeed + DECELERATION * deltaTime);
        }
        busSpeed = glm::clamp(busSpeed, -MAX_SPEED, MAX_SPEED);

        float turnInput = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) turnInput = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) turnInput = -1.0f;
        if (turnInput != 0.0f)
            busSteerAngle += turnInput * STEER_SPEED * deltaTime;
        else {
            if (busSteerAngle > 0.0f) busSteerAngle = std::max(0.0f, busSteerAngle - STEER_SPEED * deltaTime);
            if (busSteerAngle < 0.0f) busSteerAngle = std::min(0.0f, busSteerAngle + STEER_SPEED * deltaTime);
        }
        busSteerAngle = glm::clamp(busSteerAngle, -MAX_STEER, MAX_STEER);

        if (busSpeed != 0.0f) busYaw += (busSteerAngle * busSpeed * deltaTime * 0.1f);
        busPosition += forwardDir * busSpeed * deltaTime;
        bus.steeringAngle = busSteerAngle;
        bus.jetEngineOn = (busSpeed > 0.1f);

        // Chase camera
        glm::vec3 camOffset(-12.0f, 5.0f, 0.0f);
        glm::vec3 rotOff;
        rotOff.x = camOffset.x * cos(rad) - camOffset.z * sin(rad);
        rotOff.z = camOffset.x * sin(rad) + camOffset.z * cos(rad);
        rotOff.y = camOffset.y;
        cameraPos = busPosition + rotOff;
        cameraPos.y += HOVER_HEIGHT;
        cameraLookAt = busPosition + glm::vec3(0.0f, HOVER_HEIGHT + 1.0f, 0.0f);
        return;
    }

    // Free camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += speed * getCameraFront();
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= speed * getCameraFront();
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(getCameraFront(), getCameraUp())) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(getCameraFront(), getCameraUp())) * speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) cameraPos += speed * getCameraUp();
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS) cameraPos -= speed * getCameraUp();

    float rotSpeed = 100.0f * deltaTime;
    bool shiftHeld = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) cameraPitch += (shiftHeld ? -rotSpeed : rotSpeed);
    if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) cameraYaw += (shiftHeld ? rotSpeed : -rotSpeed);
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) cameraRoll += (shiftHeld ? -rotSpeed : rotSpeed);

    // Orbit mode
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

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action != GLFW_PRESS) return;

    ViewportState& vs = viewports[activeViewport];

    switch (key) {
        // --- VIEWPORT SELECTION ---
        case GLFW_KEY_TAB:
            activeViewport = (activeViewport + 1) % 4;
            std::cout << "Active Viewport: " << activeViewport << " ("
                      << cameraModeNames[viewports[activeViewport].cameraMode] << ")" << std::endl;
            printViewportStatus();
            break;

        // --- LIGHT TYPE TOGGLES (per active viewport) ---
        case GLFW_KEY_1:
            vs.dirLightOn = !vs.dirLightOn;
            std::cout << "VP " << activeViewport << " Directional: "
                      << (vs.dirLightOn ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_2:
            vs.pointLightsOn = !vs.pointLightsOn;
            std::cout << "VP " << activeViewport << " Point Lights: "
                      << (vs.pointLightsOn ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_3:
            vs.spotLightOn = !vs.spotLightOn;
            std::cout << "VP " << activeViewport << " Spot Light: "
                      << (vs.spotLightOn ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_4:
            vs.emissiveLightOn = !vs.emissiveLightOn;
            std::cout << "VP " << activeViewport << " Emissive: "
                      << (vs.emissiveLightOn ? "ON" : "OFF") << std::endl;
            break;

        // --- LIGHT COMPONENT TOGGLES (per active viewport) ---
        case GLFW_KEY_5:
            vs.ambientOn = !vs.ambientOn;
            std::cout << "VP " << activeViewport << " Ambient: "
                      << (vs.ambientOn ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_6:
            vs.diffuseOn = !vs.diffuseOn;
            std::cout << "VP " << activeViewport << " Diffuse: "
                      << (vs.diffuseOn ? "ON" : "OFF") << std::endl;
            break;
        case GLFW_KEY_7:
            vs.specularOn = !vs.specularOn;
            std::cout << "VP " << activeViewport << " Specular: "
                      << (vs.specularOn ? "ON" : "OFF") << std::endl;
            break;

        // --- CAMERA MODE CYCLE (per active viewport) ---
        case GLFW_KEY_8:
            vs.cameraMode = (vs.cameraMode + 1) % NUM_CAMERA_MODES;
            std::cout << "VP " << activeViewport << " Camera: "
                      << cameraModeNames[vs.cameraMode] << std::endl;
            break;

        // --- BUS CONTROLS ---
        case GLFW_KEY_B: bus.toggleFrontDoor(); break;
        case GLFW_KEY_G: fanSpinning = !fanSpinning; break;
        case GLFW_KEY_L: bus.toggleLight(); break;
        case GLFW_KEY_K:
            isDrivingMode = !isDrivingMode;
            if (isDrivingMode) {
                std::cout << "DRIVING MODE ON (W/S = Thrust, A/D = Steer)" << std::endl;
            } else {
                std::cout << "DRIVING MODE OFF" << std::endl;
                busSpeed = 0.0f;
                busSteerAngle = 0.0f;
            }
            break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // Viewport is set per-frame in the render loop
}