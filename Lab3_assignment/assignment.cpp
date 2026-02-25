#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include "Shader.h"
#include "Bus.h"

// ============================================================================
// STB_IMAGE for texture loading
// ============================================================================
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// ============================================================================
// CAMERA SYSTEM
// ============================================================================
// Camera modes: 0=Free, 1=Chase (3rd person), 2=Interior (1st person)
int cameraMode = 1; // Start in chase cam
const int NUM_CAMERA_MODES = 3;
const char* cameraModeNames[] = { "FREE CAMERA", "CHASE CAMERA (3rd person)", "INTERIOR CAMERA (1st person)" };

// Free camera
glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
float cameraPitch = -15.0f;
float cameraYaw = -90.0f;
float cameraRoll = 0.0f;
float cameraFOV = 45.0f;  // Zoom via scroll

// Mouse state
bool mouseCaptured = false;  // Press M to capture/release mouse
bool firstMouse = true;
float lastMouseX = SCR_WIDTH / 2.0f;
float lastMouseY = SCR_HEIGHT / 2.0f;
float mouseSensitivity = 0.1f;

// Orbit
float orbitAngle = 0.0f;
float orbitRadius = 20.0f;
float orbitHeight = 10.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

Bus bus;
bool fanSpinning = false;

// ============================================================================
// DRIVING SIMULATION
// ============================================================================
bool isDrivingMode = true; // Driving is the default
glm::vec3 busPosition = glm::vec3(0.0f, 0.0f, 0.0f);
float busAltitude = 0.0f;   // User-controlled hover altitude
float busVerticalSpeed = 0.0f;
float busYaw = 0.0f;
float busSpeed = 0.0f;
float busSteerAngle = 0.0f;

const float ACCELERATION = 15.0f;
const float DECELERATION = 10.0f;
const float MAX_SPEED = 20.0f;
const float STEER_SPEED = 60.0f;
const float MAX_STEER = 35.0f;
const float HOVER_HEIGHT = 1.5f;
const float VERTICAL_ACCEL = 12.0f;
const float MAX_ALTITUDE = 50.0f;

// ============================================================================
// GLOBAL LIGHTING STATE
// ============================================================================
bool dirLightOn = true;
bool pointLightsOn = true;
bool spotLightOn = true;
bool emissiveLightOn = true;
bool ambientOn = true;
bool diffuseOn = true;
bool specularOn = true;

// ============================================================================
// TEXTURE STATE
// ============================================================================
unsigned int texFloor = 0, texCarpet = 0, texFabric = 0;
unsigned int texWall = 0, texDashboard = 0, texBusBody = 0;
unsigned int texSphere = 0, texCone = 0;

// City environment textures
unsigned int texRoad = 0, texGrass = 0;
unsigned int texContainer = 0, texEmoji = 0;

Sphere sceneSphere;
Cone sceneCone;

int sceneTextureMode = 1;

// ============================================================================
// CITY ENVIRONMENT CONSTANTS
// ============================================================================
const float ROAD_WIDTH = 8.0f;
const float ROAD_SEGMENT_LEN = 20.0f;
const int   VISIBLE_SEGMENTS = 30;        // segments ahead + behind
const float GRASS_WIDTH = 50.0f;
const float BUILDING_ZONE_START = 6.0f;   // distance from road center
const float BUILDING_ZONE_END = 40.0f;
const int   BUILDINGS_PER_SEGMENT = 6;    // buildings per side per segment

// Simple deterministic hash for building placement
unsigned int cityHash(int x, int y) {
    unsigned int h = (unsigned int)(x * 374761393 + y * 668265263);
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

float cityRand(int seed, int id) {
    return (float)(cityHash(seed, id) % 10000) / 10000.0f;
}

// Pre-defined bright color palette for buildings
glm::vec3 buildingPalette[] = {
    glm::vec3(0.85f, 0.2f, 0.2f),   // Red
    glm::vec3(0.2f, 0.65f, 0.9f),   // Blue
    glm::vec3(0.2f, 0.8f, 0.3f),    // Green
    glm::vec3(0.9f, 0.85f, 0.1f),   // Yellow
    glm::vec3(0.7f, 0.3f, 0.85f),   // Purple
    glm::vec3(0.95f, 0.55f, 0.1f),  // Orange
    glm::vec3(0.1f, 0.85f, 0.75f),  // Cyan
    glm::vec3(0.85f, 0.15f, 0.55f), // Pink
    glm::vec3(0.5f, 0.5f, 0.85f),   // Periwinkle
    glm::vec3(0.3f, 0.75f, 0.5f),   // Teal
};
const int NUM_PALETTE_COLORS = 10;

int currentWrapIndex = 0;
GLenum wrapModes[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };
const char* wrapNames[] = { "GL_REPEAT", "GL_CLAMP_TO_EDGE", "GL_MIRRORED_REPEAT" };
const int NUM_WRAP_MODES = 3;

int currentFilterIndex = 0;
GLenum filterModes[] = { GL_LINEAR, GL_NEAREST };
const char* filterNames[] = { "GL_LINEAR", "GL_NEAREST" };
const int NUM_FILTER_MODES = 2;

const char* textureModeNames[] = { "OFF", "PURE TEXTURE", "VERTEX-BLENDED (Gouraud)", "FRAGMENT-BLENDED (Phong)" };

// ============================================================================
// CUSTOM lookAt
// ============================================================================
glm::mat4 myLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
    glm::vec3 f = glm::normalize(center - eye);
    glm::vec3 s = glm::normalize(glm::cross(f, up));
    glm::vec3 u = glm::cross(s, f);
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
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow* window);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

// ============================================================================
// CAMERA HELPERS
// ============================================================================
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

// Get bus forward direction from yaw
glm::vec3 getBusForward() {
    float rad = glm::radians(busYaw);
    return glm::vec3(-cos(rad), 0.0f, sin(rad));
}

glm::vec3 getBusRight() {
    float rad = glm::radians(busYaw - 90.0f);
    return glm::vec3(-cos(rad), 0.0f, sin(rad));
}

glm::mat4 getViewMatrix() {
    glm::vec3 busRenderPos = busPosition;
    busRenderPos.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;

    if (cameraMode == 1) {
        // === CHASE CAMERA (3rd person behind bus, jet engine visible) ===
        glm::vec3 forward = getBusForward();
        // Position camera behind the bus (where the jet is)
        glm::vec3 chaseOffset = -forward * 18.0f + glm::vec3(0.0f, 5.0f, 0.0f);
        cameraPos = busRenderPos + chaseOffset;
        // Look at the rear of the bus (where the jet engine is) + slight upward bias
        glm::vec3 lookTarget = busRenderPos + glm::vec3(0.0f, 1.5f, 0.0f) - forward * 2.0f;
        return myLookAt(cameraPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    if (cameraMode == 2) {
        // === INTERIOR CAMERA (1st person, driver seat) ===
        glm::vec3 forward = getBusForward();
        glm::vec3 right = getBusRight();
        // Driver position inside bus
        cameraPos = busRenderPos + forward * (-3.0f) + glm::vec3(0.0f, 1.0f, 0.0f) + right * (-0.6f);
        // Look forward through windshield, with mouse-look adjustment
        glm::vec3 lookDir;
        lookDir.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        lookDir.y = sin(glm::radians(cameraPitch));
        lookDir.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
        lookDir = glm::normalize(lookDir);
        return myLookAt(cameraPos, cameraPos + lookDir, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // === FREE CAMERA ===
    glm::vec3 front = getCameraFront();
    glm::vec3 up = getCameraUp();
    if (cameraRoll != 0.0f) {
        glm::mat4 rollMat = glm::rotate(glm::mat4(1.0f), glm::radians(cameraRoll), front);
        up = glm::vec3(rollMat * glm::vec4(up, 0.0f));
    }
    return myLookAt(cameraPos, cameraPos + front, up);
}

// ============================================================================
// TEXTURE LOADING
// ============================================================================
const int MAX_TEXTURE_DIM = 2048;

unsigned int loadTexture(const char* path, GLenum wrapMode, GLenum filterMode) {
    std::cout << "  Loading: " << path << "..." << std::flush;

    // Pre-check: can we open the file?
    {
        std::ifstream testFile(path, std::ios::binary);
        if (!testFile.good()) {
            std::cout << " [--] not found" << std::endl;
            std::cout.flush();
            return 0;
        }
    }

    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(true);

    // Query info without loading pixel data
    int infoOk = stbi_info(path, &width, &height, &nrChannels);
    if (!infoOk || width <= 0 || height <= 0) {
        std::cout << " [SKIP] invalid/corrupt image" << std::endl;
        std::cout.flush();
        return 0;
    }
    std::cout << " " << width << "x" << height << "..." << std::flush;

    // Force 3 channels (RGB)
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    if (!data) {
        std::cout << " [FAIL] " << stbi_failure_reason() << std::endl;
        std::cout.flush();
        return 0;
    }

    // Downscale if too large (simple box filter)
    int outW = width, outH = height;
    unsigned char* finalData = data;
    bool didResize = false;
    if (outW > MAX_TEXTURE_DIM || outH > MAX_TEXTURE_DIM) {
        float scale = std::min((float)MAX_TEXTURE_DIM / outW, (float)MAX_TEXTURE_DIM / outH);
        int newW = (int)(outW * scale);
        int newH = (int)(outH * scale);
        if (newW < 1) newW = 1;
        if (newH < 1) newH = 1;
        unsigned char* resized = (unsigned char*)malloc(newW * newH * 3);
        if (resized) {
            for (int y = 0; y < newH; y++) {
                for (int x = 0; x < newW; x++) {
                    int srcX = (int)(x / scale);
                    int srcY = (int)(y / scale);
                    if (srcX >= outW) srcX = outW - 1;
                    if (srcY >= outH) srcY = outH - 1;
                    int si = (srcY * outW + srcX) * 3;
                    int di = (y * newW + x) * 3;
                    resized[di] = data[si];
                    resized[di+1] = data[si+1];
                    resized[di+2] = data[si+2];
                }
            }
            stbi_image_free(data);
            finalData = resized;
            outW = newW;
            outH = newH;
            didResize = true;
            std::cout << " resized->" << outW << "x" << outH << "..." << std::flush;
        }
    }

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outW, outH, 0, GL_RGB, GL_UNSIGNED_BYTE, finalData);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    std::cout << " [OK]" << std::endl;
    std::cout.flush();

    if (didResize)
        free(finalData);
    else
        stbi_image_free(finalData);

    return textureID;
}



void updateSceneTextureParams() {
    GLenum wrap = wrapModes[currentWrapIndex];
    GLenum filter = filterModes[currentFilterIndex];
    unsigned int ids[] = { texSphere, texCone };
    for (auto id : ids) {
        if (id != 0) {
            glBindTexture(GL_TEXTURE_2D, id);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }
    }
}

void printStatus() {
    std::cout << "\n========== STATUS ==========" << std::endl;
    std::cout << "  Camera:   " << cameraModeNames[cameraMode] << std::endl;
    std::cout << "  FOV:      " << cameraFOV << " deg" << std::endl;
    std::cout << "  Mouse:    " << (mouseCaptured ? "CAPTURED (press M to release)" : "FREE (press M to capture)") << std::endl;
    std::cout << "  Driving:  " << (isDrivingMode ? "ON" : "OFF") << std::endl;
    std::cout << "  Texture:  " << textureModeNames[sceneTextureMode] << std::endl;
    std::cout << "  Wrap:     " << wrapNames[currentWrapIndex] << std::endl;
    std::cout << "  Filter:   " << filterNames[currentFilterIndex] << std::endl;
    std::cout << "  Lights:   Dir=" << (dirLightOn ? "ON" : "OFF")
              << " Pt=" << (pointLightsOn ? "ON" : "OFF")
              << " Spot=" << (spotLightOn ? "ON" : "OFF")
              << " Emis=" << (emissiveLightOn ? "ON" : "OFF") << std::endl;
    std::cout << "  Shading:  A=" << (ambientOn ? "ON" : "OFF")
              << " D=" << (diffuseOn ? "ON" : "OFF")
              << " S=" << (specularOn ? "ON" : "OFF") << std::endl;
    std::cout << "============================" << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Hover Bus - Texture Mapped", NULL, NULL);
    if (!window) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);

    // Mouse starts free â€” press M to capture for look-around
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader ourShader("shader.vert", "shader.frag");
    bus.init();
    bus.jetEngineOn = true;  // Flame always visible
    sceneSphere.init(30, 36);
    sceneCone.init(36);

    // ==================== LOAD TEXTURES ====================
    std::cout << "\n=== Loading Textures ===" << std::endl;
    texFloor     = loadTexture("textures/floor.jpg",     GL_REPEAT,          GL_LINEAR);
    texCarpet    = loadTexture("textures/carpet.jpg",    GL_REPEAT,          GL_NEAREST);
    texFabric    = loadTexture("textures/fabric.jpg",    GL_CLAMP_TO_EDGE,   GL_LINEAR);
    texWall      = loadTexture("textures/wall.jpg",      GL_MIRRORED_REPEAT, GL_LINEAR);
    texDashboard = loadTexture("textures/dashboard.jpg", GL_REPEAT,          GL_NEAREST);
    texBusBody   = loadTexture("textures/busbody.jpg",   GL_CLAMP_TO_EDGE,   GL_NEAREST);
    texSphere    = loadTexture("textures/sphere.jpg",    GL_REPEAT,          GL_LINEAR);
    texCone      = loadTexture("textures/cone.jpg",      GL_MIRRORED_REPEAT, GL_NEAREST);

    // City environment textures
    texRoad      = loadTexture("textures/road.jpg",      GL_REPEAT,          GL_LINEAR);
    texGrass     = loadTexture("textures/grass.jpg",     GL_REPEAT,          GL_LINEAR);
    texContainer = loadTexture("textures/container2.png", GL_REPEAT,         GL_LINEAR);
    texEmoji     = loadTexture("textures/emoji.png",     GL_CLAMP_TO_EDGE,   GL_LINEAR);
    std::cout << "========================" << std::endl;

    // Assign to bus
    bus.texFloor = texFloor;
    bus.texCarpet = texCarpet;
    bus.texFabric = texFabric;
    bus.texWall = texWall;
    bus.texDashboard = texDashboard;
    bus.texBusBody = texBusBody;


    // Print controls
    std::cout << "=====================================================" << std::endl;
    std::cout << "       HOVER BUS - GAME CONTROLS                     " << std::endl;
    std::cout << "=====================================================" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- MOVEMENT (Free Camera) ---" << std::endl;
    std::cout << "  W/S         Forward / Backward" << std::endl;
    std::cout << "  A/D         Strafe Left / Right" << std::endl;
    std::cout << "  Space       Move Up" << std::endl;
    std::cout << "  Left Ctrl   Move Down" << std::endl;
    std::cout << "  Shift       Speed Boost (2x)" << std::endl;
    std::cout << "  Mouse       Look Around" << std::endl;
    std::cout << "  Scroll      Zoom In / Out" << std::endl;
    std::cout << "  Q / E       Roll Left / Right" << std::endl;
    std::cout << "  F (hold)    Orbit Around Bus" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- DRIVING MODE ---" << std::endl;
    std::cout << "  K           Toggle Driving Mode" << std::endl;
    std::cout << "  W/S         Thrust / Brake" << std::endl;
    std::cout << "  A/D         Steer Left / Right" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- CAMERA ---" << std::endl;
    std::cout << "  V           Cycle Camera (Free/Chase/Interior)" << std::endl;
    std::cout << "  M           Toggle Mouse Capture" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- BUS ---" << std::endl;
    std::cout << "  B           Open/Close Front Door" << std::endl;
    std::cout << "  G           Toggle Ceiling Fan" << std::endl;
    std::cout << "  L           Toggle Interior Lights" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- TEXTURE ---" << std::endl;
    std::cout << "  T           Cycle Texture Mode (Off/Pure/Vertex/Fragment)" << std::endl;
    std::cout << "  8           Cycle Wrap Mode" << std::endl;
    std::cout << "  9           Cycle Filter Mode" << std::endl;
    std::cout << "  0           Toggle ALL Textures On/Off" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  --- LIGHTING ---" << std::endl;
    std::cout << "  1           Directional Light" << std::endl;
    std::cout << "  2           Point Lights" << std::endl;
    std::cout << "  3           Spotlight (flashlight)" << std::endl;
    std::cout << "  4           Emissive Glow" << std::endl;
    std::cout << "  5/6/7       Ambient / Diffuse / Specular" << std::endl;
    std::cout << "" << std::endl;
    std::cout << "  TAB         Print Status" << std::endl;
    std::cout << "  ESC         Exit" << std::endl;
    std::cout << "=====================================================" << std::endl;
    std::cout << "\nTIP: Press V to switch to Interior Camera to see" << std::endl;
    std::cout << "     the textured seats, floor, and walls inside!" << std::endl;
    std::cout << "     Press K to start driving.\n" << std::endl;

    // ==================== RENDER LOOP ====================
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        bus.updateFan(deltaTime, fanSpinning);
        bus.updateJetFlame(deltaTime);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.53f, 0.72f, 0.92f, 1.0f);  // Light blue sky
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        ourShader.setInt("textureMode", 0);

        // ==================== LIGHT SETUP ====================
        ourShader.setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
        ourShader.setVec3("dirLight.ambient",  0.15f, 0.15f, 0.15f);
        ourShader.setVec3("dirLight.diffuse",  0.7f, 0.7f, 0.6f);
        ourShader.setVec3("dirLight.specular", 0.5f, 0.5f, 0.5f);

        // Point Lights
        glm::vec3 bp = busPosition;
        ourShader.setVec3("pointLights[0].position", bp + glm::vec3(5, 5, 5));
        ourShader.setVec3("pointLights[0].ambient",  0.05f, 0.0f, 0.0f);
        ourShader.setVec3("pointLights[0].diffuse",  0.8f, 0.1f, 0.1f);
        ourShader.setVec3("pointLights[0].specular", 1.0f, 0.2f, 0.2f);
        ourShader.setFloat("pointLights[0].constant",  1.0f);
        ourShader.setFloat("pointLights[0].linear",    0.09f);
        ourShader.setFloat("pointLights[0].quadratic", 0.032f);

        ourShader.setVec3("pointLights[1].position", bp + glm::vec3(-5, 5, 5));
        ourShader.setVec3("pointLights[1].ambient",  0.0f, 0.05f, 0.0f);
        ourShader.setVec3("pointLights[1].diffuse",  0.1f, 0.8f, 0.1f);
        ourShader.setVec3("pointLights[1].specular", 0.2f, 1.0f, 0.2f);
        ourShader.setFloat("pointLights[1].constant",  1.0f);
        ourShader.setFloat("pointLights[1].linear",    0.09f);
        ourShader.setFloat("pointLights[1].quadratic", 0.032f);

        ourShader.setVec3("pointLights[2].position", bp + glm::vec3(5, 5, -5));
        ourShader.setVec3("pointLights[2].ambient",  0.0f, 0.0f, 0.05f);
        ourShader.setVec3("pointLights[2].diffuse",  0.1f, 0.1f, 0.8f);
        ourShader.setVec3("pointLights[2].specular", 0.2f, 0.2f, 1.0f);
        ourShader.setFloat("pointLights[2].constant",  1.0f);
        ourShader.setFloat("pointLights[2].linear",    0.09f);
        ourShader.setFloat("pointLights[2].quadratic", 0.032f);

        ourShader.setVec3("pointLights[3].position", bp + glm::vec3(-5, 5, -5));
        ourShader.setVec3("pointLights[3].ambient",  0.05f, 0.05f, 0.05f);
        ourShader.setVec3("pointLights[3].diffuse",  0.6f, 0.6f, 0.6f);
        ourShader.setVec3("pointLights[3].specular", 0.6f, 0.6f, 0.6f);
        ourShader.setFloat("pointLights[3].constant",  1.0f);
        ourShader.setFloat("pointLights[3].linear",    0.09f);
        ourShader.setFloat("pointLights[3].quadratic", 0.032f);

        ourShader.setVec3("spotLight.position", cameraPos);
        ourShader.setVec3("spotLight.direction", getCameraFront());
        ourShader.setVec3("spotLight.ambient",  0.0f, 0.0f, 0.0f);
        ourShader.setVec3("spotLight.diffuse",  1.0f, 1.0f, 1.0f);
        ourShader.setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
        ourShader.setFloat("spotLight.constant",  1.0f);
        ourShader.setFloat("spotLight.linear",    0.09f);
        ourShader.setFloat("spotLight.quadratic", 0.032f);
        ourShader.setFloat("spotLight.cutOff", glm::cos(glm::radians(12.5f)));

        ourShader.setFloat("shininess", 32.0f);
        ourShader.setVec3("viewPos", cameraPos);

        ourShader.setBool("dirLightOn",    dirLightOn);
        ourShader.setBool("pointLightsOn", pointLightsOn);
        ourShader.setBool("spotLightOn",   spotLightOn);
        ourShader.setBool("ambientOn",     ambientOn);
        ourShader.setBool("diffuseOn",     diffuseOn);
        ourShader.setBool("specularOn",    specularOn);
        ourShader.setBool("isEmissive", false);
        ourShader.setFloat("alpha", 1.0f);

        // View & Projection
        float aspect = (float)fbWidth / (float)fbHeight;
        glm::mat4 projection = glm::perspective(glm::radians(cameraFOV), aspect, 0.1f, 500.0f);
        glm::mat4 view = getViewMatrix();
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);

        // ==================== DRAW BUS ====================
        glm::mat4 busTransform = glm::mat4(1.0f);
        glm::vec3 renderPos = busPosition;
        renderPos.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
        busTransform = glm::translate(busTransform, renderPos);
        busTransform = glm::rotate(busTransform, glm::radians(busYaw), glm::vec3(0, 1, 0));

        bool savedJetOn = bus.jetEngineOn;
        if (!emissiveLightOn) bus.jetEngineOn = false;
        bus.draw(ourShader, busTransform);
        bus.jetEngineOn = savedJetOn;

        // ==================== CITY ENVIRONMENT ====================
        // The road runs along the X-axis. Bus starts at (0,0,0) facing -X.
        // We generate road segments and buildings relative to the bus X position.

        float busX = busPosition.x;
        // Snap to nearest segment boundary
        float segStart = floor(busX / ROAD_SEGMENT_LEN) * ROAD_SEGMENT_LEN;

        for (int seg = -VISIBLE_SEGMENTS / 2; seg <= VISIBLE_SEGMENTS / 2; seg++) {
            float segX = segStart + seg * ROAD_SEGMENT_LEN;

            // --- ROAD SEGMENT ---
            {
                ourShader.setInt("textureMode", 0);
                if (texRoad != 0) {
                    ourShader.setInt("textureMode", 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texRoad);
                    ourShader.setInt("textureSampler", 0);
                }
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.05f, 0.0f));
                model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, ROAD_WIDTH));
                bus.cube.draw(ourShader, model, glm::vec3(0.08f, 0.08f, 0.08f));
                ourShader.setInt("textureMode", 0);
            }

            // --- WHITE DASHED CENTER DIVIDER ---
            {
                int numDashes = 4;
                float dashLen = ROAD_SEGMENT_LEN / (numDashes * 2.0f);
                for (int d = 0; d < numDashes; d++) {
                    float dx = segX + d * (dashLen * 2.0f) + dashLen * 0.5f;
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(dx, 0.01f, 0.0f));
                    model = glm::scale(model, glm::vec3(dashLen * 0.8f, 0.02f, 0.15f));
                    bus.cube.draw(ourShader, model, glm::vec3(1.0f, 1.0f, 1.0f));
                }
            }

            // --- GRASS STRIPS (both sides) ---
            for (int side = -1; side <= 1; side += 2) {
                float grassZ = side * (ROAD_WIDTH * 0.5f + GRASS_WIDTH * 0.5f);
                if (texGrass != 0) {
                    ourShader.setInt("textureMode", 3);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texGrass);
                    ourShader.setInt("textureSampler", 0);
                }
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.1f, grassZ));
                model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, GRASS_WIDTH));
                bus.cube.draw(ourShader, model, glm::vec3(0.15f, 0.45f, 0.1f));
                ourShader.setInt("textureMode", 0);
            }
        } // end segment loop

        // ==================== BUILDINGS ====================
        // Each building type appears twice (one on each side or at different positions)

        // Helper lambda: draws a stacked-cubes building at (posX, posZ)
        // --- STACKED CUBES #1 (left side, near start) ---
        {
            float bx = -15.0f, bz = -10.0f;
            glm::vec3 colors[] = {
                glm::vec3(0.85f, 0.2f, 0.2f),
                glm::vec3(0.2f, 0.65f, 0.9f),
                glm::vec3(0.9f, 0.85f, 0.1f)
            };
            float yOff = 0.0f;
            float sizes[][3] = { {3.0f, 3.0f, 3.0f}, {2.5f, 2.5f, 2.5f}, {2.0f, 2.0f, 2.0f} };
            for (int c = 0; c < 3; c++) {
                if (texContainer != 0) {
                    ourShader.setInt("textureMode", 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texContainer);
                    ourShader.setInt("textureSampler", 0);
                }
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(bx, yOff + sizes[c][1] * 0.5f, bz));
                model = glm::scale(model, glm::vec3(sizes[c][0], sizes[c][1], sizes[c][2]));
                bus.cube.draw(ourShader, model, colors[c]);
                ourShader.setInt("textureMode", 0);
                yOff += sizes[c][1];
            }
        }

        // --- STACKED CUBES #2 (right side, further along) ---
        {
            float bx = -50.0f, bz = 12.0f;
            glm::vec3 colors[] = {
                glm::vec3(0.95f, 0.55f, 0.1f),
                glm::vec3(0.7f, 0.3f, 0.85f)
            };
            float yOff = 0.0f;
            float sizes[][3] = { {3.5f, 4.0f, 3.5f}, {2.5f, 3.0f, 2.5f} };
            for (int c = 0; c < 2; c++) {
                if (texContainer != 0) {
                    ourShader.setInt("textureMode", 1);
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texContainer);
                    ourShader.setInt("textureSampler", 0);
                }
                glm::mat4 model = glm::translate(glm::mat4(1.0f),
                    glm::vec3(bx, yOff + sizes[c][1] * 0.5f, bz));
                model = glm::scale(model, glm::vec3(sizes[c][0], sizes[c][1], sizes[c][2]));
                bus.cube.draw(ourShader, model, colors[c]);
                ourShader.setInt("textureMode", 0);
                yOff += sizes[c][1];
            }
        }

        // --- TALL BUILDING #1 WITH WINDOWS (right side) ---
        {
            float bx = -25.0f, bz = 10.0f;
            float bw = 4.0f, bh = 12.0f, bd = 4.0f;

            if (texWall != 0) {
                ourShader.setInt("textureMode", 3);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texWall);
                ourShader.setInt("textureSampler", 0);
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, bh * 0.5f, bz));
            model = glm::scale(model, glm::vec3(bw, bh, bd));
            bus.cube.draw(ourShader, model, glm::vec3(0.7f, 0.3f, 0.85f));
            ourShader.setInt("textureMode", 0);

            // Windows on road-facing side
            for (int wr = 0; wr < 5; wr++) {
                for (int wc = 0; wc < 2; wc++) {
                    float wx = bx - 1.0f + wc * 2.0f;
                    float wy = 1.8f + wr * 2.0f;
                    float wz = bz - bd * 0.52f;
                    glm::mat4 wModel = glm::translate(glm::mat4(1.0f), glm::vec3(wx, wy, wz));
                    wModel = glm::scale(wModel, glm::vec3(0.8f, 1.0f, 0.05f));
                    bus.cube.draw(ourShader, wModel, glm::vec3(0.05f, 0.08f, 0.15f));
                }
            }
        }

        // --- TALL BUILDING #2 WITH WINDOWS (left side, further along) ---
        {
            float bx = -60.0f, bz = -11.0f;
            float bw = 5.0f, bh = 15.0f, bd = 5.0f;

            if (texWall != 0) {
                ourShader.setInt("textureMode", 3);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texWall);
                ourShader.setInt("textureSampler", 0);
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, bh * 0.5f, bz));
            model = glm::scale(model, glm::vec3(bw, bh, bd));
            bus.cube.draw(ourShader, model, glm::vec3(0.2f, 0.65f, 0.9f));
            ourShader.setInt("textureMode", 0);

            // Windows on road-facing side
            for (int wr = 0; wr < 6; wr++) {
                for (int wc = 0; wc < 2; wc++) {
                    float wx = bx - 1.2f + wc * 2.4f;
                    float wy = 2.0f + wr * 2.0f;
                    float wz = bz + bd * 0.52f;
                    glm::mat4 wModel = glm::translate(glm::mat4(1.0f), glm::vec3(wx, wy, wz));
                    wModel = glm::scale(wModel, glm::vec3(0.9f, 1.1f, 0.05f));
                    bus.cube.draw(ourShader, wModel, glm::vec3(0.05f, 0.08f, 0.15f));
                }
            }
        }

        // --- CONE-TOPPED TOWER #1 (left side) ---
        // Cylinder: height goes from 0 to towerH (center at towerH/2)
        // Cone: sits on top, center at towerH + coneH/2
        {
            float bx = -40.0f, bz = -12.0f;
            float radius = 2.0f, towerH = 8.0f, coneH = 3.0f;

            // Cylinder body — use container2.png (512x512, square, clean UV)
            if (texContainer != 0) {
                ourShader.setInt("textureMode", 3);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texContainer);
                ourShader.setInt("textureSampler", 0);
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH * 0.5f, bz));
            model = glm::scale(model, glm::vec3(radius * 2.0f, towerH, radius * 2.0f));
            bus.cylinder.draw(ourShader, model, glm::vec3(0.1f, 0.85f, 0.75f));
            ourShader.setInt("textureMode", 0);

            // Cone roof — sits ON TOP of cylinder (no overlap)
            model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH + coneH * 0.5f, bz));
            model = glm::scale(model, glm::vec3(radius * 2.8f, coneH, radius * 2.8f));
            sceneCone.draw(ourShader, model, glm::vec3(0.95f, 0.55f, 0.1f));
        }

        // --- CONE-TOPPED TOWER #2 (right side, further along) ---
        {
            float bx = -75.0f, bz = 13.0f;
            float radius = 1.5f, towerH = 6.0f, coneH = 2.5f;

            if (texContainer != 0) {
                ourShader.setInt("textureMode", 3);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texContainer);
                ourShader.setInt("textureSampler", 0);
            }
            glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH * 0.5f, bz));
            model = glm::scale(model, glm::vec3(radius * 2.0f, towerH, radius * 2.0f));
            bus.cylinder.draw(ourShader, model, glm::vec3(0.85f, 0.15f, 0.55f));
            ourShader.setInt("textureMode", 0);

            // Cone roof — ON TOP
            model = glm::translate(glm::mat4(1.0f), glm::vec3(bx, towerH + coneH * 0.5f, bz));
            model = glm::scale(model, glm::vec3(radius * 2.8f, coneH, radius * 2.8f));
            sceneCone.draw(ourShader, model, glm::vec3(0.2f, 0.8f, 0.3f));
        }

        ourShader.setInt("textureMode", 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    bus.cleanup();
    sceneSphere.cleanup();
    sceneCone.cleanup();
    unsigned int allTex[] = { texFloor, texCarpet, texFabric, texWall, texDashboard, texBusBody, texSphere, texCone };
    for (auto t : allTex) { if (t) glDeleteTextures(1, &t); }
    glfwTerminate();
    return 0;
}

// ============================================================================
// MOUSE CALLBACK â€” look around (FPS-style)
// ============================================================================
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    if (!mouseCaptured) return;

    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastMouseX = xpos;
        lastMouseY = ypos;
        firstMouse = false;
    }

    float xoffset = (xpos - lastMouseX) * mouseSensitivity;
    float yoffset = (lastMouseY - ypos) * mouseSensitivity; // Inverted Y
    lastMouseX = xpos;
    lastMouseY = ypos;

    cameraYaw += xoffset;
    cameraPitch += yoffset;

    // Clamp pitch to prevent flipping
    if (cameraPitch > 89.0f) cameraPitch = 89.0f;
    if (cameraPitch < -89.0f) cameraPitch = -89.0f;
}

// ============================================================================
// SCROLL CALLBACK â€” zoom in/out (change FOV)
// ============================================================================
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraFOV -= (float)yoffset * 2.0f;
    if (cameraFOV < 15.0f) cameraFOV = 15.0f;
    if (cameraFOV > 90.0f) cameraFOV = 90.0f;
}

// ============================================================================
// PROCESS INPUT â€” continuous key handling
// ============================================================================
void processInput(GLFWwindow* window) {
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
            // Natural deceleration
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
        busPosition += forwardDir * busSpeed * deltaTime;
        bus.steeringAngle = busSteerAngle;
        bus.jetEngineOn = true;

        // Up/Down hover control
        float vertInput = 0.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) vertInput = 1.0f;
        if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS) vertInput = -1.0f;
        if (vertInput != 0.0f)
            busVerticalSpeed += vertInput * VERTICAL_ACCEL * deltaTime;
        else {
            // Dampen vertical speed
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
// KEY CALLBACK â€” discrete key presses
// ============================================================================
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return;

    switch (key) {
        // --- CAMERA ---
        case GLFW_KEY_V:
            cameraMode = (cameraMode + 1) % NUM_CAMERA_MODES;
            std::cout << "Camera: " << cameraModeNames[cameraMode] << std::endl;
            // When switching to interior, point forward
            if (cameraMode == 2) {
                cameraYaw = busYaw + 180.0f; // look forward from driver seat
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
            sceneTextureMode = (sceneTextureMode + 1) % 4;
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
        case GLFW_KEY_K:
            isDrivingMode = !isDrivingMode;
            if (!isDrivingMode) {
                cameraMode = 0; // Switch to free cam
                std::cout << "FREE CAM ON | Arrow keys = fly | WASD still drives bus" << std::endl;
            } else {
                cameraMode = 1; // Back to chase cam
                std::cout << "CHASE CAM | WASD=Drive | V=cycle camera" << std::endl;
            }
            break;

        // --- STATUS ---
        case GLFW_KEY_TAB: printStatus(); break;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    // Viewport set per-frame
}
