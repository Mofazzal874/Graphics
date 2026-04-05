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

// New textures for cylinders, cones, and buildings
unsigned int texStoneWall = 0, texRoofTile = 0, texBrickWall = 0;

// Carpet textures for city ground
unsigned int texCarpetTile = 0, texEarthTone = 0;

// Skybox
unsigned int skyboxVAO = 0, skyboxVBO = 0;
unsigned int cubemapTexture = 0;

Sphere sceneSphere;
Cone sceneCone;

// Bezier/Spline surface of revolution objects
BezierSurface bezierVase;         // Decorative vase along the road
BezierSurface bezierWaterTower;   // Water tower (large bulb on top)
SplineSurface splineLamp;         // Street lamp post with smooth curves
SplineSurface splineBollard;      // Rounded bollard/post
RuledSurface  ruledCanopy;        // Canopy/awning between two curves

// Ring checkpoints the hover vehicle flies through
Torus ringCheckpoint;
PolygonRing hexRing, triRing, squareRing, pentRing;

// Eiffel Tower components (built from Bezier curves + ruled surfaces)
BezierSurface eiffelLeg;          // One curved leg (Bezier revolution - tapered)
SplineSurface eiffelUpperShaft;   // Upper narrow shaft (spline revolution)
BezierSurface eiffelTopBulb;      // Top observation dome (Bezier revolution)
RuledSurface  eiffelArch[4];      // Decorative arches between legs (ruled surfaces)
RuledSurface  eiffelPlatform;     // Observation platform (ruled surface)

// ============================================================================
// COLLISION SYSTEM - AABB-based
// ============================================================================
struct AABB {
    glm::vec3 minPt;
    glm::vec3 maxPt;
};

std::vector<AABB> collisionBoxes; // filled each frame from visible buildings + objects

// Ring checkpoint positions (fixed world positions along the road)
struct RingCheckpoint {
    glm::vec3 position;
    float yaw;       // rotation around Y
    float radius;    // ring outer radius for collision pass-through
    bool passed;
};
std::vector<RingCheckpoint> ringPositions;

// Tower position
glm::vec3 towerPosition = glm::vec3(50.0f, 0.0f, -25.0f);

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

const char* textureModeNames[] = { "OFF", "PURE TEXTURE", "VERTEX-BLENDED (Gouraud)", "FRAGMENT-BLENDED (Phong)", "MULTI-TEXTURE BLEND" };

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
void addBuildingCollision(glm::vec3 center, glm::vec3 halfExtents);
bool checkBusCollision(glm::vec3 newPos);

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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // RGB rows may not be 4-byte aligned
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

// ============================================================================
// CUBEMAP LOADING FROM HORIZONTAL CROSS LAYOUT
// ============================================================================
// sky.png cross layout (4 cols × 3 rows):
//   Col0   Col1   Col2   Col3
//   ---    Top    ---    ---     Row 0
//   Left   Front  Right  Back   Row 1
//   ---    Bottom ---    ---     Row 2
unsigned int loadCubemapFromCross(const char* path) {
    std::cout << "  Loading cubemap cross: " << path << "..." << std::flush;
    
    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(false);  // Cubemaps don't flip
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    if (!data) {
        std::cout << " [FAIL] " << stbi_failure_reason() << std::endl;
        return 0;
    }
    std::cout << " " << width << "x" << height << "..." << std::flush;
    
    int faceW = width / 4;
    int faceH = height / 3;
    // Use the smaller dimension for square faces
    int faceSize = (faceW < faceH) ? faceW : faceH;
    
    // Face positions in the cross (col, row)
    // OpenGL cubemap face order: +X, -X, +Y, -Y, +Z, -Z
    // Mapping: Right=+X, Left=-X, Top=+Y, Bottom=-Y, Front=+Z, Back=-Z
    struct FaceInfo { int col; int row; };
    FaceInfo faces[6] = {
        {2, 1},  // GL_TEXTURE_CUBE_MAP_POSITIVE_X = Right
        {0, 1},  // GL_TEXTURE_CUBE_MAP_NEGATIVE_X = Left
        {1, 0},  // GL_TEXTURE_CUBE_MAP_POSITIVE_Y = Top
        {1, 2},  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y = Bottom
        {1, 1},  // GL_TEXTURE_CUBE_MAP_POSITIVE_Z = Front
        {3, 1},  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z = Back
    };
    
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    
    // Allocate temporary buffer for one face (square, resampled)
    unsigned char* faceData = (unsigned char*)malloc(faceSize * faceSize * 3);
    
    for (int i = 0; i < 6; i++) {
        int srcX = faces[i].col * faceW;
        int srcY = faces[i].row * faceH;
        
        // Extract and resample face to square
        for (int y = 0; y < faceSize; y++) {
            for (int x = 0; x < faceSize; x++) {
                // Map to source coordinates
                int sx = srcX + (int)(x * (float)faceW / faceSize);
                int sy = srcY + (int)(y * (float)faceH / faceSize);
                if (sx >= width) sx = width - 1;
                if (sy >= height) sy = height - 1;
                
                int srcIdx = (sy * width + sx) * 3;
                int dstIdx = (y * faceSize + x) * 3;
                faceData[dstIdx]     = data[srcIdx];
                faceData[dstIdx + 1] = data[srcIdx + 1];
                faceData[dstIdx + 2] = data[srcIdx + 2];
            }
        }
        
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                     faceSize, faceSize, 0, GL_RGB, GL_UNSIGNED_BYTE, faceData);
    }
    
    free(faceData);
    stbi_image_free(data);
    
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    std::cout << " [OK] face=" << faceSize << "x" << faceSize << std::endl;
    stbi_set_flip_vertically_on_load(true);  // Restore for other textures
    return textureID;
}

// ============================================================================
// CUBEMAP LOADING FROM 6 INDIVIDUAL FACE IMAGES
// ============================================================================
unsigned int loadCubemapFromFaces() {
    std::cout << "  Loading cubemap from individual faces..." << std::flush;

    // OpenGL cubemap face order: +X, -X, +Y, -Y, +Z, -Z
    const char* facePaths[6] = {
        "textures/skybox/right.jpg",   // GL_TEXTURE_CUBE_MAP_POSITIVE_X
        "textures/skybox/left.jpg",    // GL_TEXTURE_CUBE_MAP_NEGATIVE_X
        "textures/skybox/top.jpg",     // GL_TEXTURE_CUBE_MAP_POSITIVE_Y
        "textures/skybox/bottom.jpg",  // GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
        "textures/skybox/front.jpg",   // GL_TEXTURE_CUBE_MAP_POSITIVE_Z
        "textures/skybox/back.jpg"     // GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
    };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);  // Cubemaps must NOT be flipped

    for (int i = 0; i < 6; i++) {
        int width = 0, height = 0, nrChannels = 0;
        unsigned char* data = stbi_load(facePaths[i], &width, &height, &nrChannels, 3);
        if (data) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                         width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            std::cout << " [" << facePaths[i] << " " << width << "x" << height << " OK]" << std::flush;
            stbi_image_free(data);
        } else {
            std::cout << " [FAIL: " << facePaths[i] << " - " << stbi_failure_reason() << "]" << std::flush;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::cout << " [DONE]" << std::endl;
    stbi_set_flip_vertically_on_load(true);  // Restore for other textures
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

    // Mouse starts free — press M to capture for look-around
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    Shader ourShader("shader.vert", "shader.frag");
    Shader skyboxShader("skybox.vert", "skybox.frag");
    bus.init();
    bus.jetEngineOn = true;  // Flame always visible
    sceneSphere.init(30, 36);
    sceneCone.init(36);

    // ==================== BEZIER/SPLINE CURVE OBJECTS ====================
    // Vase profile: wide base, narrow neck, flared top (Bezier surface of revolution)
    {
        std::vector<glm::vec2> vaseProfile = {
            glm::vec2(0.8f, 0.0f),   // base
            glm::vec2(1.0f, 0.3f),   // wide belly
            glm::vec2(0.3f, 0.7f),   // narrow neck
            glm::vec2(0.6f, 1.0f)    // flared lip
        };
        bezierVase.init(vaseProfile, 20, 24);
    }

    // Water tower profile: thin stem, big bulb on top (Bezier)
    {
        std::vector<glm::vec2> towerProfile = {
            glm::vec2(0.3f, 0.0f),   // narrow base
            glm::vec2(0.3f, 0.5f),   // stem
            glm::vec2(1.2f, 0.6f),   // bulge out
            glm::vec2(1.0f, 0.85f),  // round top
            glm::vec2(0.0f, 1.0f)    // apex
        };
        bezierWaterTower.init(towerProfile, 25, 30);
    }

    // Street lamp profile (Catmull-Rom spline - smooth through all points)
    {
        std::vector<glm::vec2> lampProfile = {
            glm::vec2(0.3f, 0.0f),   // base
            glm::vec2(0.15f, 0.1f),  // taper
            glm::vec2(0.08f, 0.5f),  // thin pole
            glm::vec2(0.08f, 0.85f), // pole continues
            glm::vec2(0.25f, 0.92f), // lamp housing bulge
            glm::vec2(0.2f, 1.0f)    // lamp top
        };
        splineLamp.init(lampProfile, 8, 20);
    }

    // Bollard (short rounded post) - Spline
    {
        std::vector<glm::vec2> bollardProfile = {
            glm::vec2(0.4f, 0.0f),
            glm::vec2(0.5f, 0.2f),
            glm::vec2(0.45f, 0.5f),
            glm::vec2(0.3f, 0.8f),
            glm::vec2(0.0f, 1.0f)
        };
        splineBollard.init(bollardProfile, 6, 16);
    }

    // Ruled surface canopy (between two curved rails)
    {
        std::vector<glm::vec3> topCurve = {
            glm::vec3(-3.0f, 4.0f, 0.0f),
            glm::vec3(-1.0f, 5.0f, 0.0f),
            glm::vec3(1.0f, 5.0f, 0.0f),
            glm::vec3(3.0f, 4.0f, 0.0f)
        };
        std::vector<glm::vec3> bottomCurve = {
            glm::vec3(-3.0f, 4.0f, 4.0f),
            glm::vec3(-1.0f, 4.5f, 4.0f),
            glm::vec3(1.0f, 4.5f, 4.0f),
            glm::vec3(3.0f, 4.0f, 4.0f)
        };
        ruledCanopy.init(topCurve, bottomCurve, 20, 8);
    }

    // Ring checkpoint (large torus for flying through)
    // mainRadius=6 gives 12-unit diameter hole, tubeRadius=0.6 makes it clearly visible
    ringCheckpoint.init(6.0f, 0.6f, 36, 18);

    // Polygon ring shapes for variety
    std::cout << "  Initializing polygon rings..." << std::flush;
    hexRing.init(6, 6.0f, 0.5f, 10, 3);    // Hexagon
    std::cout << " hex" << std::flush;
    triRing.init(3, 6.0f, 0.55f, 10, 3);   // Triangle
    std::cout << " tri" << std::flush;
    squareRing.init(4, 6.0f, 0.5f, 10, 3); // Square
    std::cout << " sq" << std::flush;
    pentRing.init(5, 6.0f, 0.5f, 10, 3);   // Pentagon
    std::cout << " pent [OK]" << std::endl;

    // Eiffel Tower construction from curves
    {
        // Leg profile: wide curved base that tapers inward (Bezier revolution, quarter-profile)
        std::vector<glm::vec2> legProfile = {
            glm::vec2(1.2f, 0.0f),    // wide foot
            glm::vec2(1.0f, 1.0f),    // lower curve
            glm::vec2(0.5f, 3.0f),    // mid taper
            glm::vec2(0.3f, 5.0f)     // top of leg (meets shaft)
        };
        eiffelLeg.init(legProfile, 15, 12);

        // Upper shaft: narrow column from 1st platform to top (Spline revolution)
        std::vector<glm::vec2> shaftProfile = {
            glm::vec2(0.8f, 0.0f),    // base (at 1st platform)
            glm::vec2(0.6f, 2.0f),    // slight taper
            glm::vec2(0.45f, 5.0f),   // 2nd platform level
            glm::vec2(0.3f, 8.0f),    // narrowing
            glm::vec2(0.2f, 11.0f),   // near top
            glm::vec2(0.15f, 13.0f),  // spire base
            glm::vec2(0.05f, 15.0f)   // spire tip
        };
        eiffelUpperShaft.init(shaftProfile, 8, 20);

        // Top observation bulb (small dome at top)
        std::vector<glm::vec2> topProfile = {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(0.4f, 0.1f),
            glm::vec2(0.35f, 0.4f),
            glm::vec2(0.0f, 0.6f)
        };
        eiffelTopBulb.init(topProfile, 10, 16);

        // Arches: curved ruled surfaces connecting each pair of legs
        // Arch 0: front arch (between front-left and front-right legs)
        float archH = 4.0f;   // arch height
        float legSpread = 6.0f; // distance of legs from center at base
        float legTopSpread = 1.5f; // where legs meet the shaft

        // 4 arches, one per face (front, back, left, right)
        // Each arch is a ruled surface between a top rail and bottom curved rail
        for (int a = 0; a < 4; a++) {
            float angle = a * 90.0f;
            float rad = glm::radians(angle);
            float cosA = cos(rad), sinA = sin(rad);

            // Bottom curve: arcs from one leg base across to the other
            std::vector<glm::vec3> bottomCurve = {
                glm::vec3(-legSpread * sinA + legSpread * cosA, 0.0f,
                           legSpread * cosA + legSpread * sinA),
                glm::vec3(-legSpread * 0.3f * sinA, archH * 0.5f,
                           legSpread * 0.3f * cosA),
                glm::vec3(legSpread * sinA + legSpread * cosA, 0.0f,
                          -legSpread * cosA + legSpread * sinA)
            };

            // Top rail: flat line at the 1st platform level
            std::vector<glm::vec3> topCurve = {
                glm::vec3(-legTopSpread * sinA + legTopSpread * cosA, archH + 1.0f,
                           legTopSpread * cosA + legTopSpread * sinA),
                glm::vec3(0.0f, archH + 1.5f, 0.0f),
                glm::vec3(legTopSpread * sinA + legTopSpread * cosA, archH + 1.0f,
                          -legTopSpread * cosA + legTopSpread * sinA)
            };
            eiffelArch[a].init(topCurve, bottomCurve, 15, 5);
        }

        // Platform: flat ruled surface at the 1st observation deck
        {
            std::vector<glm::vec3> platTop = {
                glm::vec3(-3.0f, 0.0f, -3.0f),
                glm::vec3(0.0f, 0.2f, -3.0f),
                glm::vec3(3.0f, 0.0f, -3.0f)
            };
            std::vector<glm::vec3> platBot = {
                glm::vec3(-3.0f, 0.0f, 3.0f),
                glm::vec3(0.0f, 0.2f, 3.0f),
                glm::vec3(3.0f, 0.0f, 3.0f)
            };
            eiffelPlatform.init(platTop, platBot, 10, 10);
        }
    }

    // Ring positions are now generated procedurally in the render loop (infinite)

    // ==================== SKYBOX CUBE VAO ====================
    float skyboxVertices[] = {
        // positions          
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

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

    // New textures for cone/cylinder structures and buildings
    texStoneWall = loadTexture("textures/stone_wall.jpg",                  GL_REPEAT, GL_LINEAR);
    texRoofTile  = loadTexture("textures/roof_tile.jpg",                   GL_REPEAT, GL_LINEAR);
    texBrickWall = loadTexture("textures/Seamless brick wall texture.jpg", GL_REPEAT, GL_LINEAR);

    // Carpet textures for urban city ground
    texCarpetTile = loadTexture("textures/commercial-carpet-tiles.jpg", GL_REPEAT, GL_LINEAR);
    texEarthTone  = loadTexture("textures/earth-tone-tiles.png",         GL_REPEAT, GL_LINEAR);

    // Skybox cubemap
    cubemapTexture = loadCubemapFromFaces();
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

        // Clear collision boxes - rebuilt each frame from visible objects
        collisionBoxes.clear();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.53f, 0.72f, 0.92f, 1.0f);  // Light blue sky
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ourShader.use();
        ourShader.setInt("textureMode", 0);

        // Default texture uniforms
        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
        ourShader.setFloat("blendWidth", 2.0f);
        ourShader.setFloat("blendEdge", 4.0f);
        ourShader.setInt("blendAxis", 2);

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
        float busZ = busPosition.z;
        // Snap to nearest segment boundary
        float segStart = floor(busX / ROAD_SEGMENT_LEN) * ROAD_SEGMENT_LEN;

        // ==================== LARGE GROUND PLANE ====================
        // Covers the entire visible area so the skybox lake is never seen.
        // Follows the bus position so it always extends past the horizon.
        {
            const float GROUND_SIZE = 1000.0f;
            if (texGrass != 0) {
                ourShader.setInt("textureMode", 3);
                ourShader.setVec2("texScale", glm::vec2(GROUND_SIZE / 5.0f, GROUND_SIZE / 5.0f));
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texGrass);
                ourShader.setInt("textureSampler", 0);
            }
            glm::mat4 groundModel = glm::translate(glm::mat4(1.0f),
                glm::vec3(busX, -0.25f, busZ));
            groundModel = glm::scale(groundModel, glm::vec3(GROUND_SIZE, 0.1f, GROUND_SIZE));
            bus.cube.draw(ourShader, groundModel, glm::vec3(0.12f, 0.38f, 0.08f));
            ourShader.setInt("textureMode", 0);
            ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
        }

        for (int seg = -VISIBLE_SEGMENTS / 2; seg <= VISIBLE_SEGMENTS / 2; seg++) {
            float segX = segStart + seg * ROAD_SEGMENT_LEN;
            int segID = (int)floor(segX / ROAD_SEGMENT_LEN);

            // --- ROAD SEGMENT ---
            {
                ourShader.setInt("textureMode", 0);
                if (texRoad != 0) {
                    ourShader.setInt("textureMode", 3);
                    // Tile road texture proportionally: aspect-preserving repeat
                    ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / ROAD_WIDTH, 1.0f));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texRoad);
                    ourShader.setInt("textureSampler", 0);
                }
                glm::mat4 model = glm::translate(glm::mat4(1.0f), 
                    glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.05f, 0.0f));
                model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, ROAD_WIDTH));
                bus.cube.draw(ourShader, model, glm::vec3(0.08f, 0.08f, 0.08f));
                ourShader.setInt("textureMode", 0);
                ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
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

            // --- GROUND STRIPS (both sides): carpet in building zone, grass beyond ---
            for (int side = -1; side <= 1; side += 2) {
                // Inner strip: carpet/tile in the building zone (from road edge to BUILDING_ZONE_END)
                float carpetWidth = BUILDING_ZONE_END - ROAD_WIDTH * 0.5f;
                float carpetZ = side * (ROAD_WIDTH * 0.5f + carpetWidth * 0.5f);
                {
                    // Alternate carpet textures per segment for variety
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex == 0) carpTex = texEarthTone;
                    if (carpTex != 0) {
                        ourShader.setInt("textureMode", 3);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 4.0f, carpetWidth / 4.0f));
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, carpTex);
                        ourShader.setInt("textureSampler", 0);
                    }
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.1f, carpetZ));
                    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, carpetWidth));
                    bus.cube.draw(ourShader, model, glm::vec3(0.55f, 0.42f, 0.32f));
                    ourShader.setInt("textureMode", 0);
                    ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                }

                // Outer strip: grass beyond the building zone
                float outerGrassWidth = GRASS_WIDTH - carpetWidth;
                if (outerGrassWidth > 0.0f) {
                    float grassZ = side * (BUILDING_ZONE_END + outerGrassWidth * 0.5f);
                    if (texGrass != 0) {
                        ourShader.setInt("textureMode", 3);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 5.0f, outerGrassWidth / 5.0f));
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texGrass);
                        ourShader.setInt("textureSampler", 0);
                    }
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.12f, grassZ));
                    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, outerGrassWidth));
                    bus.cube.draw(ourShader, model, glm::vec3(0.15f, 0.45f, 0.1f));
                    ourShader.setInt("textureMode", 0);
                    ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                }
            }

            // --- BUILDINGS (both sides) - urban grid layout with entry paths ---
            // Reduced to 3 buildings per side for less crowding, placed in orderly rows
            int buildingsPerSide = 3;
            for (int side = -1; side <= 1; side += 2) {
                for (int b = 0; b < buildingsPerSide; b++) {
                    int bSeed = segID * 100 + side * 50 + b;

                    // Grid-like placement: evenly spaced along segment, staggered rows
                    float spacing = ROAD_SEGMENT_LEN / (buildingsPerSide + 1);
                    float bx = segX + spacing * (b + 1);
                    // Two rows: near row and far row, alternating per building
                    float nearDist = BUILDING_ZONE_START + 4.0f;
                    float farDist = BUILDING_ZONE_START + 14.0f + cityRand(bSeed, 20) * 8.0f;
                    float bz = side * ((b % 2 == 0) ? nearDist : farDist);

                    // Random building type: 0=stacked cubes, 1=tall building, 2=cone tower
                    int bType = (int)(cityRand(bSeed, 3) * 3.0f);
                    int colorIdx = (int)(cityRand(bSeed, 4) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;
                    glm::vec3 bColor = buildingPalette[colorIdx];

                    // Choose texture
                    int texChoice = (int)(cityRand(bSeed, 10) * 6.0f);
                    unsigned int bTex = 0;
                    int bTexMode = 3;
                    if (texChoice == 0 && texContainer != 0) { bTex = texContainer; }
                    else if (texChoice == 1 && texWall != 0) { bTex = texWall; }
                    else if (texChoice == 2 && texEmoji != 0) { bTex = texEmoji; }
                    else if (texChoice == 3 && texStoneWall != 0) { bTex = texStoneWall; }
                    else if (texChoice == 4 && texBrickWall != 0) { bTex = texBrickWall; }

                    // --- CARPET ENTRY PATH from building to road edge ---
                    {
                        float roadEdgeZ = side * (ROAD_WIDTH * 0.5f + 0.5f);
                        float pathLen = fabs(bz - roadEdgeZ);
                        float pathCenterZ = (bz + roadEdgeZ) * 0.5f;
                        float pathWidth = 1.2f;

                        // Alternate entry path texture
                        unsigned int pathTex = (b % 2 == 0) ? texEarthTone : texCarpetTile;
                        if (pathTex == 0) pathTex = texCarpetTile;
                        if (pathTex == 0) pathTex = texEarthTone;
                        if (pathTex != 0) {
                            ourShader.setInt("textureMode", 3);
                            ourShader.setVec2("texScale", glm::vec2(pathWidth / 1.5f, pathLen / 1.5f));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, pathTex);
                            ourShader.setInt("textureSampler", 0);
                        }
                        glm::mat4 pathModel = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, -0.08f, pathCenterZ));
                        pathModel = glm::scale(pathModel, glm::vec3(pathWidth, 0.06f, pathLen));
                        bus.cube.draw(ourShader, pathModel, glm::vec3(0.6f, 0.5f, 0.38f));
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                    }

                    if (bType == 0) {
                        // ---- STACKED CUBES ----
                        int numCubes = 1 + (int)(cityRand(bSeed, 5) * 3.0f);
                        float yOffset = 0.0f;
                        for (int c = 0; c < numCubes; c++) {
                            float cw = 2.0f + cityRand(bSeed * 10 + c, 6) * 2.0f;
                            float ch = 1.5f + cityRand(bSeed * 10 + c, 7) * 3.0f;
                            float cd = 2.0f + cityRand(bSeed * 10 + c, 8) * 2.0f;
                            int cc = (int)(cityRand(bSeed * 10 + c, 9) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;

                            if (bTex != 0) {
                                ourShader.setInt("textureMode", bTexMode);
                                float maxDim = std::max({cw, ch, cd});
                                ourShader.setVec2("texScale", glm::vec2(cw / maxDim, ch / maxDim));
                                glActiveTexture(GL_TEXTURE0);
                                glBindTexture(GL_TEXTURE_2D, bTex);
                                ourShader.setInt("textureSampler", 0);
                            }

                            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                glm::vec3(bx, yOffset + ch * 0.5f, bz));
                            model = glm::scale(model, glm::vec3(cw, ch, cd));
                            bus.cube.draw(ourShader, model, buildingPalette[cc]);
                            ourShader.setInt("textureMode", 0);
                            ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                            addBuildingCollision(glm::vec3(bx, yOffset + ch * 0.5f, bz),
                                                 glm::vec3(cw * 0.5f, ch * 0.5f, cd * 0.5f));
                            yOffset += ch;
                        }
                    }
                    else if (bType == 1) {
                        // ---- TALL BUILDING ----
                        float bw = 2.5f + cityRand(bSeed, 5) * 3.0f;
                        float bh = 5.0f + cityRand(bSeed, 6) * 12.0f;
                        float bd = 2.5f + cityRand(bSeed, 7) * 3.0f;

                        if (bTex != 0) {
                            ourShader.setInt("textureMode", bTexMode);
                            float maxDim = std::max({bw, bh, bd});
                            ourShader.setVec2("texScale", glm::vec2(bw / maxDim, bh / maxDim));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, bTex);
                            ourShader.setInt("textureSampler", 0);
                        }

                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, bh * 0.5f, bz));
                        model = glm::scale(model, glm::vec3(bw, bh, bd));
                        bus.cube.draw(ourShader, model, bColor);
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                        addBuildingCollision(glm::vec3(bx, bh * 0.5f, bz),
                                             glm::vec3(bw * 0.5f, bh * 0.5f, bd * 0.5f));

                        // Windows
                        int wRows = (int)(bh / 1.5f);
                        int wCols = (int)(bw / 1.2f);
                        if (wCols < 1) wCols = 1;
                        for (int wr = 0; wr < wRows && wr < 6; wr++) {
                            for (int wc = 0; wc < wCols && wc < 3; wc++) {
                                float wx = bx - bw * 0.3f + wc * (bw * 0.6f / std::max(wCols - 1, 1));
                                float wy = 1.5f + wr * 1.5f;
                                float wz = bz + (side > 0 ? -bd * 0.52f : bd * 0.52f);
                                glm::mat4 wModel = glm::translate(glm::mat4(1.0f),
                                    glm::vec3(wx, wy, wz));
                                wModel = glm::scale(wModel, glm::vec3(0.6f, 0.8f, 0.05f));
                                bus.cube.draw(ourShader, wModel, glm::vec3(0.05f, 0.08f, 0.15f));
                            }
                        }
                    }
                    else {
                        // ---- CONE-TOPPED TOWER ----
                        float radius = 1.0f + cityRand(bSeed, 5) * 1.5f;
                        float towerH = 3.0f + cityRand(bSeed, 6) * 8.0f;
                        float coneH = 1.5f + cityRand(bSeed, 7) * 2.0f;

                        {
                            unsigned int cylTex = (bSeed % 2 == 0 && texStoneWall != 0) ? texStoneWall :
                                                  (texBrickWall != 0 ? texBrickWall : bTex);
                            if (cylTex != 0) {
                                ourShader.setInt("textureMode", 3);
                                float circumference = 2.0f * 3.14159f * radius;
                                ourShader.setVec2("texScale", glm::vec2(circumference / 2.0f, towerH / 2.0f));
                                glActiveTexture(GL_TEXTURE0);
                                glBindTexture(GL_TEXTURE_2D, cylTex);
                                ourShader.setInt("textureSampler", 0);
                            }
                        }

                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, towerH * 0.5f, bz));
                        model = glm::scale(model, glm::vec3(radius, towerH, radius));
                        bus.cylinder.draw(ourShader, model, bColor);
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                        addBuildingCollision(glm::vec3(bx, (towerH + coneH) * 0.5f, bz),
                                             glm::vec3(radius, (towerH + coneH) * 0.5f, radius));

                        int roofColor = (colorIdx + 3) % NUM_PALETTE_COLORS;
                        if (texRoofTile != 0) {
                            ourShader.setInt("textureMode", 3);
                            float circumference = 2.0f * 3.14159f * radius;
                            ourShader.setVec2("texScale", glm::vec2(circumference / 2.0f, coneH / 2.0f));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, texRoofTile);
                            ourShader.setInt("textureSampler", 0);
                        }
                        model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, towerH + coneH * 0.5f, bz));
                        model = glm::scale(model, glm::vec3(radius, coneH, radius));
                        sceneCone.draw(ourShader, model, buildingPalette[roofColor]);
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                    }
                }
            }
        }
        ourShader.setInt("textureMode", 0);

        // ==================== CURVE OBJECTS: BEZIER / SPLINE / RULED ====================
        {
            float time = (float)glfwGetTime();

            // --- BEZIER VASES along the road (infinite, based on bus position) ---
            {
                float vaseSpacing = 25.0f;
                int vaseStart = (int)floor((busX - VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / vaseSpacing);
                int vaseEnd = (int)ceil((busX + VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / vaseSpacing);
                for (int i = vaseStart; i <= vaseEnd; i++) {
                    float vaseX = i * vaseSpacing;
                    for (int side = -1; side <= 1; side += 2) {
                        float vaseZ = side * (ROAD_WIDTH * 0.5f + 1.5f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(vaseX, 0.0f, vaseZ));
                        model = glm::scale(model, glm::vec3(1.2f, 2.0f, 1.2f));
                        bezierVase.draw(ourShader, model, glm::vec3(0.7f, 0.25f, 0.1f));
                        addBuildingCollision(glm::vec3(vaseX, 1.0f, vaseZ),
                                             glm::vec3(0.7f, 1.0f, 0.7f));
                    }
                }
            }

            // --- BEZIER WATER TOWERS (infinite, one every 150 units) ---
            {
                float wtSpacing = 150.0f;
                int wtStart = (int)floor((busX - 300.0f) / wtSpacing);
                int wtEnd = (int)ceil((busX + 300.0f) / wtSpacing);
                for (int i = wtStart; i <= wtEnd; i++) {
                    float wtX = i * wtSpacing + 50.0f;
                    int wtSeed = i * 7919;
                    float wtZ = ((cityHash(wtSeed, 0) % 2 == 0) ? 1.0f : -1.0f) *
                                (18.0f + cityRand(wtSeed, 1) * 8.0f);
                    glm::vec3 wtp(wtX, 0.0f, wtZ);
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), wtp);
                    model = glm::scale(model, glm::vec3(3.0f, 10.0f, 3.0f));
                    bezierWaterTower.draw(ourShader, model, glm::vec3(0.5f, 0.5f, 0.6f));
                    addBuildingCollision(wtp + glm::vec3(0, 5, 0),
                                         glm::vec3(2.0f, 5.0f, 2.0f));
                }
            }

            // --- SPLINE STREET LAMPS along road (infinite) ---
            {
                float lampSpacing = 20.0f;
                int lampStart = (int)floor((busX - VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / lampSpacing);
                int lampEnd = (int)ceil((busX + VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / lampSpacing);
                for (int i = lampStart; i <= lampEnd; i++) {
                    float lampX = i * lampSpacing + 10.0f;
                    for (int side = -1; side <= 1; side += 2) {
                        float lampZ = side * (ROAD_WIDTH * 0.5f + 0.8f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(lampX, 0.0f, lampZ));
                        model = glm::scale(model, glm::vec3(0.6f, 5.0f, 0.6f));
                        splineLamp.draw(ourShader, model, glm::vec3(0.3f, 0.3f, 0.35f));

                        if (emissiveLightOn) {
                            ourShader.setBool("isEmissive", true);
                            glm::mat4 glowModel = glm::translate(glm::mat4(1.0f),
                                glm::vec3(lampX, 4.8f, lampZ));
                            glowModel = glm::scale(glowModel, glm::vec3(0.4f, 0.4f, 0.4f));
                            sceneSphere.draw(ourShader, glowModel, glm::vec3(1.0f, 0.9f, 0.5f));
                            ourShader.setBool("isEmissive", false);
                        }

                        addBuildingCollision(glm::vec3(lampX, 2.5f, lampZ),
                                             glm::vec3(0.3f, 2.5f, 0.3f));
                    }
                }
            }

            // --- SPLINE BOLLARDS at road intersections (infinite) ---
            {
                float bollardSpacing = 50.0f;
                int bStart = (int)floor((busX - 200.0f) / bollardSpacing);
                int bEnd = (int)ceil((busX + 200.0f) / bollardSpacing);
                for (int i = bStart; i <= bEnd; i++) {
                    float bx = i * bollardSpacing;
                    for (int side = -1; side <= 1; side += 2) {
                        float bz = side * (ROAD_WIDTH * 0.5f + 0.3f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, 0.0f, bz));
                        model = glm::scale(model, glm::vec3(0.5f, 1.0f, 0.5f));
                        splineBollard.draw(ourShader, model, glm::vec3(0.8f, 0.7f, 0.1f));
                        addBuildingCollision(glm::vec3(bx, 0.5f, bz),
                                             glm::vec3(0.3f, 0.5f, 0.3f));
                    }
                }
            }

            // --- RULED SURFACE CANOPIES (bus stop shelters, infinite every 200 units) ---
            {
                float canopySpacing = 200.0f;
                int cStart = (int)floor((busX - 400.0f) / canopySpacing);
                int cEnd = (int)ceil((busX + 400.0f) / canopySpacing);
                for (int ci = cStart; ci <= cEnd; ci++) {
                    float cx = ci * canopySpacing + 40.0f;
                    float cside = (ci % 2 == 0) ? 1.0f : -1.0f;
                    glm::vec3 cp(cx, 0.0f, cside * (ROAD_WIDTH * 0.5f + 3.0f));
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), cp);
                    ruledCanopy.draw(ourShader, model, glm::vec3(0.6f, 0.65f, 0.7f));

                    for (int p = -1; p <= 1; p += 2) {
                        glm::mat4 pillar = glm::translate(glm::mat4(1.0f),
                            cp + glm::vec3(p * 2.8f, 2.0f, 2.0f));
                        pillar = glm::scale(pillar, glm::vec3(0.15f, 4.0f, 0.15f));
                        bus.cube.draw(ourShader, pillar, glm::vec3(0.4f, 0.4f, 0.45f));
                        addBuildingCollision(cp + glm::vec3(p * 2.8f, 2.0f, 2.0f),
                                             glm::vec3(0.15f, 2.0f, 0.15f));
                    }
                }
            }

            // --- EIFFEL TOWER (built from Bezier legs, Spline shaft, Ruled arches) ---
            {
                glm::vec3 tp = towerPosition;
                glm::vec3 eiffelColor(0.45f, 0.38f, 0.30f); // Dark iron/bronze
                float legSpread = 6.0f;
                float legHeight = 5.0f;   // matches leg profile max Y
                float platformY = 5.5f;   // 1st observation deck
                float shaftScale = 2.5f;

                // Apply stone texture to all Eiffel parts
                if (texStoneWall != 0) {
                    ourShader.setInt("textureMode", 3);
                    ourShader.setVec2("texScale", glm::vec2(2.0f, 4.0f));
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, texStoneWall);
                    ourShader.setInt("textureSampler", 0);
                }

                // 4 curved legs at corners (Bezier surface of revolution)
                float legOffsets[4][2] = {
                    {-legSpread, -legSpread},
                    { legSpread, -legSpread},
                    { legSpread,  legSpread},
                    {-legSpread,  legSpread}
                };
                float legAngles[4] = { 45.0f, -45.0f, -135.0f, 135.0f };

                for (int l = 0; l < 4; l++) {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(legOffsets[l][0], 0.0f, legOffsets[l][1]));
                    // Tilt each leg inward toward center
                    float tiltAngle = 15.0f;
                    float tiltRad = glm::radians(legAngles[l]);
                    model = glm::rotate(model, glm::radians(tiltAngle) * cos(tiltRad), glm::vec3(0, 0, 1));
                    model = glm::rotate(model, glm::radians(tiltAngle) * sin(tiltRad), glm::vec3(1, 0, 0));
                    model = glm::scale(model, glm::vec3(1.5f, legHeight, 1.5f));
                    eiffelLeg.draw(ourShader, model, eiffelColor);

                    // Collision for each leg
                    addBuildingCollision(
                        tp + glm::vec3(legOffsets[l][0], legHeight * 0.5f, legOffsets[l][1]),
                        glm::vec3(1.5f, legHeight * 0.5f, 1.5f));
                }

                // 4 decorative arches between legs (Ruled surfaces)
                for (int a = 0; a < 4; a++) {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), tp);
                    eiffelArch[a].draw(ourShader, model, glm::vec3(0.5f, 0.42f, 0.35f));
                }

                // 1st observation platform (Ruled surface - flat deck)
                {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(0.0f, platformY, 0.0f));
                    model = glm::scale(model, glm::vec3(1.5f, 1.0f, 1.5f));
                    eiffelPlatform.draw(ourShader, model, glm::vec3(0.4f, 0.35f, 0.28f));
                }

                // Platform railing (thin cubes around edge)
                for (int s = 0; s < 4; s++) {
                    float angle = s * 90.0f;
                    float r = glm::radians(angle);
                    glm::vec3 railPos = tp + glm::vec3(cos(r) * 4.2f, platformY + 0.5f, sin(r) * 4.2f);
                    glm::mat4 railing = glm::translate(glm::mat4(1.0f), railPos);
                    railing = glm::rotate(railing, r, glm::vec3(0, 1, 0));
                    railing = glm::scale(railing, glm::vec3(8.0f, 0.3f, 0.1f));
                    bus.cube.draw(ourShader, railing, glm::vec3(0.35f, 0.3f, 0.25f));
                }

                // Upper shaft (Spline surface of revolution)
                {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(0.0f, platformY, 0.0f));
                    model = glm::scale(model, glm::vec3(shaftScale, shaftScale, shaftScale));
                    eiffelUpperShaft.draw(ourShader, model, eiffelColor);
                }

                // 2nd observation deck (smaller platform)
                {
                    float deck2Y = platformY + 5.0f * shaftScale;
                    glm::mat4 deck2 = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(0.0f, deck2Y, 0.0f));
                    deck2 = glm::scale(deck2, glm::vec3(2.5f, 0.15f, 2.5f));
                    bus.cube.draw(ourShader, deck2, glm::vec3(0.4f, 0.35f, 0.28f));
                }

                // Top observation bulb (Bezier revolution)
                {
                    float topY = platformY + 14.0f * shaftScale;
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(0.0f, topY, 0.0f));
                    model = glm::scale(model, glm::vec3(1.5f, 2.0f, 1.5f));
                    eiffelTopBulb.draw(ourShader, model, glm::vec3(0.5f, 0.45f, 0.35f));
                }

                ourShader.setInt("textureMode", 0);
                ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                // Central collision for the whole tower
                float totalHeight = platformY + 15.0f * shaftScale;
                addBuildingCollision(tp + glm::vec3(0, totalHeight * 0.5f, 0),
                                     glm::vec3(2.0f, totalHeight * 0.5f, 2.0f));

                // Beacon light on top
                if (emissiveLightOn) {
                    ourShader.setBool("isEmissive", true);
                    float beacon = 0.5f + 0.5f * sin(time * 3.0f);
                    float topY = platformY + 15.0f * shaftScale + 1.0f;
                    glm::mat4 beaconModel = glm::translate(glm::mat4(1.0f),
                        tp + glm::vec3(0.0f, topY, 0.0f));
                    beaconModel = glm::scale(beaconModel, glm::vec3(1.0f, 1.0f, 1.0f));
                    sceneSphere.draw(ourShader, beaconModel,
                        glm::vec3(1.0f, 0.3f, 0.1f) * beacon);
                    ourShader.setBool("isEmissive", false);
                }
            }

            // --- RING CHECKPOINTS (infinite, sparse, varied shapes) ---
            // Rings every 120 units (sparse), with varied shapes
            {
                float ringSpacing = 120.0f;
                int ringStart = (int)floor((busX - 400.0f) / ringSpacing);
                int ringEnd = (int)ceil((busX + 400.0f) / ringSpacing);
                for (int ri = ringStart; ri <= ringEnd; ri++) {
                    float ringX = ri * ringSpacing;
                    int ringSeed = ri * 4919;
                    // Vary height between 8-16 units
                    float ringY = 10.0f + 4.0f * sin(ri * 0.9f);
                    // Slight Z offset for variety
                    float ringZ = sin(ri * 1.7f) * 3.0f;

                    float bobY = sin(time * 1.5f + ri * 0.8f) * 0.5f;
                    glm::vec3 ringPos(ringX, ringY + bobY, ringZ);

                    glm::mat4 model = glm::translate(glm::mat4(1.0f), ringPos);
                    // Rotate ring to face along X-axis (perpendicular to road)
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));

                    // Pick shape based on hash of ring index
                    int shapeType = (int)(cityHash(ringSeed, 3) % 5); // 0=circle, 1=hex, 2=tri, 3=square, 4=pent

                    // Color varies by shape
                    glm::vec3 ringColors[] = {
                        glm::vec3(1.0f, 0.7f, 0.0f),  // gold (circle)
                        glm::vec3(0.0f, 0.8f, 1.0f),  // cyan (hex)
                        glm::vec3(1.0f, 0.3f, 0.3f),  // red (triangle)
                        glm::vec3(0.5f, 1.0f, 0.3f),  // lime (square)
                        glm::vec3(0.8f, 0.4f, 1.0f),  // purple (pentagon)
                    };
                    float pulse = 0.7f + 0.3f * sin(time * 4.0f + ri);
                    glm::vec3 ringColor = ringColors[shapeType] * pulse;

                    ourShader.setBool("isEmissive", true);
                    ourShader.setFloat("alpha", 0.85f);

                    switch (shapeType) {
                        case 0: ringCheckpoint.draw(ourShader, model, ringColor); break;
                        case 1: hexRing.draw(ourShader, model, ringColor); break;
                        case 2: triRing.draw(ourShader, model, ringColor); break;
                        case 3: squareRing.draw(ourShader, model, ringColor); break;
                        case 4: pentRing.draw(ourShader, model, ringColor); break;
                    }

                    ourShader.setFloat("alpha", 1.0f);
                    ourShader.setBool("isEmissive", false);

                    // Check pass-through
                    glm::vec3 busCenter = busPosition;
                    busCenter.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
                    float distXZ = fabs(busCenter.x - ringPos.x);
                    float distYZ = sqrt((busCenter.y - ringPos.y) * (busCenter.y - ringPos.y) +
                                        (busCenter.z - ringPos.z) * (busCenter.z - ringPos.z));
                    if (distXZ < 2.0f && distYZ < 6.0f * 0.8f) {
                        std::cout << ">> RING " << ri << " PASSED! <<" << std::endl;
                    }
                }
            }
        }

        // ==================== DRAW SKYBOX ====================
        if (cubemapTexture != 0) {
            glDepthFunc(GL_LEQUAL);  // Skybox passes depth test at z=1.0
            skyboxShader.use();
            // Remove translation from view matrix so skybox stays centered on camera
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
            skyboxShader.setMat4("view", skyboxView);
            skyboxShader.setMat4("projection", projection);
            // Bind cubemap
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            skyboxShader.setInt("skybox", 0);
            // Draw skybox cube
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS);  // Restore default
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    bus.cleanup();
    sceneSphere.cleanup();
    sceneCone.cleanup();
    bezierVase.cleanup();
    bezierWaterTower.cleanup();
    splineLamp.cleanup();
    splineBollard.cleanup();
    ruledCanopy.cleanup();
    ringCheckpoint.cleanup();
    hexRing.cleanup();
    triRing.cleanup();
    squareRing.cleanup();
    pentRing.cleanup();
    eiffelLeg.cleanup();
    eiffelUpperShaft.cleanup();
    eiffelTopBulb.cleanup();
    for (int i = 0; i < 4; i++) eiffelArch[i].cleanup();
    eiffelPlatform.cleanup();
    if (skyboxVAO) { glDeleteVertexArrays(1, &skyboxVAO); glDeleteBuffers(1, &skyboxVBO); }
    unsigned int allTex[] = { texFloor, texCarpet, texFabric, texWall, texDashboard, texBusBody, texSphere, texCone,
                              texStoneWall, texRoofTile, texBrickWall };
    for (auto t : allTex) { if (t) glDeleteTextures(1, &t); }
    if (cubemapTexture) glDeleteTextures(1, &cubemapTexture);
    glfwTerminate();
    return 0;
}

// ============================================================================
// MOUSE CALLBACK — look around (FPS-style)
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
// SCROLL CALLBACK — zoom in/out (change FOV)
// ============================================================================
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraFOV -= (float)yoffset * 2.0f;
    if (cameraFOV < 15.0f) cameraFOV = 15.0f;
    if (cameraFOV > 90.0f) cameraFOV = 90.0f;
}

// ============================================================================
// COLLISION DETECTION HELPERS
// ============================================================================
// Bus AABB half-extents (local space): roughly 5x1.5x1.5 (half of 10x3x3)
const float BUS_HALF_X = 5.0f;
const float BUS_HALF_Y = 1.5f;
const float BUS_HALF_Z = 1.5f;

// Get bus AABB in world space (axis-aligned bounding box around rotated bus)
AABB getBusAABB() {
    glm::vec3 pos = busPosition;
    pos.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
    // For a rotated box, compute the encompassing AABB
    float rad = glm::radians(busYaw);
    float cosA = fabs(cos(rad)), sinA = fabs(sin(rad));
    float extX = BUS_HALF_X * cosA + BUS_HALF_Z * sinA;
    float extZ = BUS_HALF_X * sinA + BUS_HALF_Z * cosA;
    AABB box;
    box.minPt = pos - glm::vec3(extX, BUS_HALF_Y, extZ);
    box.maxPt = pos + glm::vec3(extX, BUS_HALF_Y, extZ);
    return box;
}

bool aabbOverlap(const AABB& a, const AABB& b) {
    return (a.minPt.x <= b.maxPt.x && a.maxPt.x >= b.minPt.x) &&
           (a.minPt.y <= b.maxPt.y && a.maxPt.y >= b.minPt.y) &&
           (a.minPt.z <= b.maxPt.z && a.maxPt.z >= b.minPt.z);
}

bool checkBusCollision(glm::vec3 newPos) {
    // Temporarily compute bus AABB at newPos
    glm::vec3 savedPos = busPosition;
    busPosition = newPos;
    AABB busBox = getBusAABB();
    busPosition = savedPos;

    for (const auto& box : collisionBoxes) {
        if (aabbOverlap(busBox, box)) return true;
    }
    return false;
}

// Build collision boxes for a given building (called during rendering)
void addBuildingCollision(glm::vec3 center, glm::vec3 halfExtents) {
    AABB box;
    box.minPt = center - halfExtents;
    box.maxPt = center + halfExtents;
    collisionBoxes.push_back(box);
}

// ============================================================================
// PROCESS INPUT — continuous key handling
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

        // Collision-aware movement: try new position, revert if blocked
        glm::vec3 newPos = busPosition + forwardDir * busSpeed * deltaTime;
        if (!checkBusCollision(newPos)) {
            busPosition = newPos;
        } else {
            // Hit something - stop and bounce back slightly
            busSpeed *= -0.3f;
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
// KEY CALLBACK — discrete key presses
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