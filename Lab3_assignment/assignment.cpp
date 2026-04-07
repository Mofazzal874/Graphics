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

// Fractal collectible score system
#include <unordered_set>
#include <cstdio>
#include <cstring>
#include <cstdint>
int  fractalScore = 0;
std::unordered_set<int> collectedSponges;

// =====================================================================
// GAMIFICATION — score, ring/cube checkpoints, HUD, hit feedback
// =====================================================================
int   gameScore = 0;
float scoreFlashTimer = 0.0f;     // counts down; while > 0 the HUD "jumps"
bool  scoreFlashIsHit = false;    // true => last change was a loss (red)
std::unordered_set<int> passedRings;     // ring indices already scored
float buildingHitCooldown = 0.0f;        // prevents spamming -2 every frame
Cube  hudCube;                           // single quad/cube reused for HUD glyph pixels

static inline void awardScore(int delta) {
    gameScore += delta;
    scoreFlashTimer = 0.45f;
    scoreFlashIsHit = (delta < 0);
}

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

// Bark + leaf textures for the fractal forest
unsigned int texBark = 0, texLeaf = 0;

// Skybox
unsigned int skyboxVAO = 0, skyboxVBO = 0;
unsigned int cubemapTexture = 0;

Sphere sceneSphere;
Cone sceneCone;

// Bezier/Spline surface of revolution objects
BezierSurface bezierVase;         // Decorative vase along the road
// [REMOVED] BezierSurface bezierWaterTower;
SplineSurface splineLamp;         // Street lamp post with smooth curves
// [REMOVED] SplineSurface splineBollard;
RuledSurface  ruledCanopy;        // Canopy/awning between two curves

// Ring checkpoints the hover vehicle flies through
Torus ringCheckpoint;
PolygonRing hexRing, triRing, squareRing, pentRing;

// [REMOVED] Eiffel Tower components

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

// [REMOVED] towerPosition

int sceneTextureMode = 1;

// ============================================================================
// CITY ENVIRONMENT CONSTANTS
// ============================================================================
const float ROAD_WIDTH = 14.0f;
const float ROAD_SEGMENT_LEN = 40.0f;
const int   VISIBLE_SEGMENTS = 20;        // segments ahead + behind
const float GRASS_WIDTH = 100.0f;
const float BUILDING_ZONE_START = 10.0f;  // distance from road center
const float BUILDING_ZONE_END = 70.0f;
const int   BUILDINGS_PER_SEGMENT = 6;    // (legacy, unused)

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

// ----------------------------------------------------------------------------
// loadTextureRGBA - keeps the alpha channel (for leaf cutout PNGs).
// ----------------------------------------------------------------------------
unsigned int loadTextureRGBA(const char* path, GLenum wrapMode, GLenum filterMode) {
    std::cout << "  Loading: " << path << "..." << std::flush;
    {
        std::ifstream testFile(path, std::ios::binary);
        if (!testFile.good()) { std::cout << " [--] not found" << std::endl; return 0; }
    }
    int width = 0, height = 0, channels = 0;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path, &width, &height, &channels, 4);
    if (!data) {
        std::cout << " [FAIL] " << stbi_failure_reason() << std::endl;
        return 0;
    }
    std::cout << " " << width << "x" << height << " RGBA..." << std::flush;

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filterMode);
    stbi_image_free(data);
    std::cout << " [OK]" << std::endl;
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
// MENGER SPONGE FRACTAL
// A 3D self-similar fractal: at each iteration, a cube is divided into 27
// sub-cubes and the 7 "axis-cross" cubes (face-centers + true center) are
// removed, leaving 20. Recursion is BAKED ONCE at startup into a list of
// (offset, size) child cubes — drawing is then a flat loop with no recursion
// per frame, so it stays fast even with many sponges visible.
// ============================================================================
struct MengerCube {
    glm::vec3 offset;   // center in unit-cube local space [-0.5, 0.5]
    float     size;     // side length (fraction of unit cube)
};
std::vector<MengerCube> mengerCubes;

// GPU-instanced rendering of the Menger sponge.
// One draw call per sponge instead of 8000 — massive speedup.
unsigned int mengerVAO    = 0;
unsigned int mengerInstVBO = 0;

static bool mengerKept(int x, int y, int z) {
    // Keep a 3x3x3 sub-cell unless it lies on the axis cross
    // (center of a face, edge-middle on axis, or the dead center).
    int centers = (x == 1 ? 1 : 0) + (y == 1 ? 1 : 0) + (z == 1 ? 1 : 0);
    return centers <= 1;
}

void buildMengerSponge(int iterations) {
    mengerCubes.clear();
    // Start from a single unit cube
    std::vector<MengerCube> current;
    current.push_back({ glm::vec3(0.0f), 1.0f });

    for (int it = 0; it < iterations; it++) {
        std::vector<MengerCube> next;
        next.reserve(current.size() * 20);
        for (const auto& c : current) {
            float s = c.size / 3.0f;
            for (int x = 0; x < 3; x++)
            for (int y = 0; y < 3; y++)
            for (int z = 0; z < 3; z++) {
                if (!mengerKept(x, y, z)) continue;
                glm::vec3 off = c.offset + glm::vec3((x - 1) * s, (y - 1) * s, (z - 1) * s);
                next.push_back({ off, s });
            }
        }
        current.swap(next);
    }
    mengerCubes = std::move(current);
}

// Build a dedicated instanced VAO that wires up the existing cube VBO plus
// a per-instance buffer holding (offset.xyz, size.w) for every sub-cube.
// Must be called AFTER buildMengerSponge() and AFTER bus.cube.init().
void initMengerInstancing() {
    // Pack mengerCubes into vec4 instance data
    std::vector<glm::vec4> instData;
    instData.reserve(mengerCubes.size());
    for (const auto& c : mengerCubes)
        instData.push_back(glm::vec4(c.offset, c.size));

    glGenVertexArrays(1, &mengerVAO);
    glBindVertexArray(mengerVAO);

    // Re-bind the cube's existing vertex buffer (pos/normal/tex)
    glBindBuffer(GL_ARRAY_BUFFER, bus.cube.VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Per-instance buffer at location 3, divisor 1
    glGenBuffers(1, &mengerInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, mengerInstVBO);
    glBufferData(GL_ARRAY_BUFFER, instData.size() * sizeof(glm::vec4),
                 instData.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);  // advance once per instance

    glBindVertexArray(0);

    // Make sure ALL non-instanced draws see identity for attrib 3.
    // Bound VAOs without attrib 3 enabled will use this generic constant.
    glVertexAttrib4f(3, 0.0f, 0.0f, 0.0f, 1.0f);
}

// ============================================================================
// FRACTAL FOREST - Recursive textured fractal trees, fully GPU-instanced.
//
// At startup we recursively bake a small "forest tile" worth of trees into
// two big lists of per-instance mat4 transforms:
//   * branchInstances - one transform per branch cylinder
//   * leafInstances   - one transform per leaf cube
// At render time the forest is drawn as repeating tiles around the bus, and
// each tile costs only ONE instanced draw call for branches + ONE for leaves.
// Branches are textured with stone_wall (bark) and leaves with the grass
// texture (foliage), so they're properly texture-mapped.
// ============================================================================
struct ForestData {
    std::vector<glm::mat4> branchInstances;
    std::vector<glm::mat4> leafInstances;
    unsigned int branchVAO = 0, branchInstVBO = 0;
    unsigned int leafVAO   = 0, leafInstVBO   = 0;
    unsigned int leafQuadVBO   = 0;   // crossed billboard geometry (12 verts)
    unsigned int branchGeomVBO = 0;   // low-poly flat-shaded ragged trunk
    int branchVertCount = 0;
};
ForestData forest;

// Build a low-poly, flat-shaded, radius-jittered cylinder used ONLY by the
// forest branches. Few sides + flat normals + asymmetric radii give the
// ragged, rough look of a real bark surface (no smooth highlights).
void buildForestBranchGeometry() {
    const int SIDES = 9;             // few sides -> visible facets
    float radii[SIDES];
    for (int i = 0; i < SIDES; i++) {
        // Asymmetric per-side radii in [0.82 .. 1.18] for the ragged silhouette
        radii[i] = 0.82f + 0.36f * ((cityHash(i + 1, 991) % 1000) / 1000.0f);
    }

    std::vector<float> v;
    v.reserve(SIDES * 6 * 8);
    for (int i = 0; i < SIDES; i++) {
        float a0 = (2.0f * (float)M_PI * i)       / SIDES;
        float a1 = (2.0f * (float)M_PI * (i + 1)) / SIDES;
        float r0 = radii[i];
        float r1 = radii[(i + 1) % SIDES];
        float x0 = cosf(a0) * r0, z0 = sinf(a0) * r0;
        float x1 = cosf(a1) * r1, z1 = sinf(a1) * r1;

        // Flat face normal (average of the two corner radial directions)
        float midA = (a0 + a1) * 0.5f;
        float nx = cosf(midA), nz = sinf(midA);

        float u0 = (float)i / SIDES;
        float u1 = (float)(i + 1) / SIDES;

        float quad[] = {
            x0, -0.5f, z0,  nx, 0.0f, nz,  u0, 0.0f,
            x1, -0.5f, z1,  nx, 0.0f, nz,  u1, 0.0f,
            x1,  0.5f, z1,  nx, 0.0f, nz,  u1, 1.0f,
            x1,  0.5f, z1,  nx, 0.0f, nz,  u1, 1.0f,
            x0,  0.5f, z0,  nx, 0.0f, nz,  u0, 1.0f,
            x0, -0.5f, z0,  nx, 0.0f, nz,  u0, 0.0f,
        };
        v.insert(v.end(), quad, quad + 48);
    }
    forest.branchVertCount = (int)(v.size() / 8);

    glGenBuffers(1, &forest.branchGeomVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchGeomVBO);
    glBufferData(GL_ARRAY_BUFFER, v.size() * sizeof(float), v.data(), GL_STATIC_DRAW);
}
const float FOREST_TILE_LEN = 200.0f;

static void bakeFractalBranch(std::vector<glm::mat4>& branches,
                              std::vector<glm::mat4>& leaves,
                              const glm::mat4& base,
                              float length, float radius,
                              int depth, unsigned int seed)
{
    // Branch transform: translate up half-length, scale (radius, length, radius)
    glm::mat4 m = glm::translate(base, glm::vec3(0.0f, length * 0.5f, 0.0f));
    m = glm::scale(m, glm::vec3(radius, length, radius));
    branches.push_back(m);

    glm::mat4 tip = glm::translate(base, glm::vec3(0.0f, length, 0.0f));

    if (depth <= 0) {
        // Leaf cluster: many small textured cubes packed around the tip.
        // More leaves = denser, more realistic foliage silhouette.
        const int LEAVES_PER_TIP = 12;
        for (int k = 0; k < LEAVES_PER_TIP; k++) {
            unsigned int ks = seed * 131u + (unsigned int)(k * 7919u);
            float ox = ((cityHash(ks, 1) % 1000) / 1000.0f - 0.5f) * length * 1.6f;
            float oy = ((cityHash(ks, 2) % 1000) / 1000.0f) * length * 1.4f;
            float oz = ((cityHash(ks, 3) % 1000) / 1000.0f - 0.5f) * length * 1.6f;
            float scl = length * (0.45f + 0.35f * ((cityHash(ks, 4) % 1000) / 1000.0f));
            float ry  = ((cityHash(ks, 5) % 1000) / 1000.0f) * 360.0f;
            float rx  = ((cityHash(ks, 6) % 1000) / 1000.0f) * 60.0f;

            glm::mat4 lm = glm::translate(tip, glm::vec3(ox, oy, oz));
            lm = glm::rotate(lm, glm::radians(ry), glm::vec3(0, 1, 0));
            lm = glm::rotate(lm, glm::radians(rx), glm::vec3(1, 0, 0));
            lm = glm::scale(lm, glm::vec3(scl, scl * 0.6f, scl));
            leaves.push_back(lm);
        }
        return;
    }

    const int N = 3;
    for (int i = 0; i < N; i++) {
        unsigned int cs = seed * 1664525u + (unsigned int)(i * 1013904223u + depth * 2654435761u);
        float r1 = (cityHash(cs, 1) % 1000) / 1000.0f;
        float r2 = (cityHash(cs, 2) % 1000) / 1000.0f;
        float r3 = (cityHash(cs, 3) % 1000) / 1000.0f;

        float yaw   = (i * (360.0f / N)) + (r1 - 0.5f) * 35.0f;
        float pitch = 22.0f + r2 * 22.0f;
        float lenScale = 0.68f + r3 * 0.10f;
        float radScale = 0.62f + r3 * 0.08f;

        glm::mat4 child = glm::rotate(tip,   glm::radians(yaw),   glm::vec3(0, 1, 0));
        child           = glm::rotate(child, glm::radians(pitch), glm::vec3(1, 0, 0));

        bakeFractalBranch(branches, leaves, child,
                          length * lenScale, radius * radScale,
                          depth - 1, cs);
    }
}

void buildForest() {
    forest.branchInstances.clear();
    forest.leafInstances.clear();

    // A "planned forest": grid of trees in 3 staggered rows on each side of
    // the road, with per-tree jitter so it doesn't look mechanical.
    const int TREES_PER_ROW = 12;        // along X
    const int ROWS_PER_SIDE = 3;         // depth into the grass field
    const float bandStart = BUILDING_ZONE_END + 4.0f;
    const float bandEnd   = GRASS_WIDTH - 6.0f;
    const float bandDepth = bandEnd - bandStart;

    for (int side = -1; side <= 1; side += 2) {
        for (int row = 0; row < ROWS_PER_SIDE; row++) {
            for (int i = 0; i < TREES_PER_ROW; i++) {
                unsigned int s = (unsigned int)(((i * 31 + row * 7919) * 2 + (side + 1)) * 374761393u + 7u);
                float r1 = (cityHash(s, 11) % 1000) / 1000.0f;
                float r2 = (cityHash(s, 12) % 1000) / 1000.0f;
                float r3 = (cityHash(s, 13) % 1000) / 1000.0f;
                float r4 = (cityHash(s, 14) % 1000) / 1000.0f;

                // Stagger every other row by half-spacing for a planted look
                float xStep   = FOREST_TILE_LEN / TREES_PER_ROW;
                float xOffset = (row % 2 == 0) ? 0.0f : xStep * 0.5f;
                float tx = (i + 0.15f + r1 * 0.7f) * xStep + xOffset;

                // Each row sits in its own depth slice of the grass band
                float rowFrac = (row + 0.2f + r2 * 0.6f) / ROWS_PER_SIDE;
                float tz = side * (bandStart + rowFrac * bandDepth);

                float scale = 0.9f + r3 * 0.6f;
                float trunkLen = 3.0f * scale;
                float trunkRad = 0.36f * scale;

                glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(tx, 0.0f, tz));
                base = glm::rotate(base, glm::radians((r1 - 0.5f) * 10.0f), glm::vec3(1, 0, 0));
                base = glm::rotate(base, glm::radians((r4 - 0.5f) * 10.0f), glm::vec3(0, 0, 1));
                base = glm::rotate(base, glm::radians(r2 * 360.0f),         glm::vec3(0, 1, 0));

                // Depth 5 -> 364 branches per tree
                bakeFractalBranch(forest.branchInstances, forest.leafInstances,
                                  base, trunkLen, trunkRad, 5, s);
            }
        }
    }
}

// Helper: wire 4 vec4 attribs (locations 4-7) as a per-instance mat4
static void setupInstanceMat4Attribs() {
    for (int i = 0; i < 4; i++) {
        glVertexAttribPointer(4 + i, 4, GL_FLOAT, GL_FALSE,
                              sizeof(glm::mat4),
                              (void*)(i * sizeof(glm::vec4)));
        glEnableVertexAttribArray(4 + i);
        glVertexAttribDivisor(4 + i, 1);
    }
}

void initForestInstancing() {
    // ---- Branch VAO: ragged low-poly VBO + branch instance buffer ----
    buildForestBranchGeometry();
    glGenVertexArrays(1, &forest.branchVAO);
    glBindVertexArray(forest.branchVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchGeomVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &forest.branchInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.branchInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 forest.branchInstances.size() * sizeof(glm::mat4),
                 forest.branchInstances.data(), GL_STATIC_DRAW);
    setupInstanceMat4Attribs();
    glBindVertexArray(0);

    // ---- Leaf VAO: crossed-billboard quad VBO + leaf instance buffer ----
    // Two perpendicular quads (XY plane + YZ plane), 6 verts each = 12 verts.
    // Far cheaper than a 36-vert cube and looks fluffier with a leaf cutout.
    float leafQuadVerts[] = {
        // pos              normal       uv
        // Quad 1 (XY plane, normal +Z)
        -0.5f,-0.5f, 0.0f,  0,0,1,  0,0,
         0.5f,-0.5f, 0.0f,  0,0,1,  1,0,
         0.5f, 0.5f, 0.0f,  0,0,1,  1,1,
         0.5f, 0.5f, 0.0f,  0,0,1,  1,1,
        -0.5f, 0.5f, 0.0f,  0,0,1,  0,1,
        -0.5f,-0.5f, 0.0f,  0,0,1,  0,0,
        // Quad 2 (YZ plane, normal +X)
         0.0f,-0.5f,-0.5f,  1,0,0,  0,0,
         0.0f,-0.5f, 0.5f,  1,0,0,  1,0,
         0.0f, 0.5f, 0.5f,  1,0,0,  1,1,
         0.0f, 0.5f, 0.5f,  1,0,0,  1,1,
         0.0f, 0.5f,-0.5f,  1,0,0,  0,1,
         0.0f,-0.5f,-0.5f,  1,0,0,  0,0,
    };
    glGenBuffers(1, &forest.leafQuadVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(leafQuadVerts), leafQuadVerts, GL_STATIC_DRAW);

    glGenVertexArrays(1, &forest.leafVAO);
    glBindVertexArray(forest.leafVAO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafQuadVBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenBuffers(1, &forest.leafInstVBO);
    glBindBuffer(GL_ARRAY_BUFFER, forest.leafInstVBO);
    glBufferData(GL_ARRAY_BUFFER,
                 forest.leafInstances.size() * sizeof(glm::mat4),
                 forest.leafInstances.data(), GL_STATIC_DRAW);
    setupInstanceMat4Attribs();
    glBindVertexArray(0);

    // Identity defaults for instance mat4 attribs (used by NON-instanced draws)
    glVertexAttrib4f(4, 1.0f, 0.0f, 0.0f, 0.0f);
    glVertexAttrib4f(5, 0.0f, 1.0f, 0.0f, 0.0f);
    glVertexAttrib4f(6, 0.0f, 0.0f, 1.0f, 0.0f);
    glVertexAttrib4f(7, 0.0f, 0.0f, 0.0f, 1.0f);
}

// Draw the forest as repeating tiles around the bus.
// Branches: stone_wall texture (bark). Leaves: grass texture (foliage).
// Cost = 2 draw calls per visible tile.
void drawForest(const Shader& sh, float busX) {
    const float visibleRange = 400.0f;
    int tStart = (int)floor((busX - visibleRange) / FOREST_TILE_LEN);
    int tEnd   = (int)ceil ((busX + visibleRange) / FOREST_TILE_LEN);

    // ---- BRANCHES (textured bark) ----
    if (texBark != 0) {
        sh.setInt("textureMode", 3);
        sh.setVec2("texScale", glm::vec2(4.0f, 6.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texBark);
        sh.setInt("textureSampler", 0);
    } else {
        sh.setInt("textureMode", 0);
    }
    sh.setVec3("objectColor", glm::vec3(0.45f, 0.30f, 0.18f));
    glBindVertexArray(forest.branchVAO);
    for (int ti = tStart; ti <= tEnd; ti++) {
        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(ti * FOREST_TILE_LEN, 0.0f, 0.0f));
        sh.setMat4("model", model);
        glDrawArraysInstanced(GL_TRIANGLES, 0,
                              forest.branchVertCount,
                              (GLsizei)forest.branchInstances.size());
    }

    // ---- LEAVES (alpha-cutout textured billboards) ----
    if (texLeaf != 0) {
        sh.setInt("textureMode", 1);          // pure texture path
        sh.setBool("alphaTest", true);        // discard transparent pixels
        sh.setVec2("texScale", glm::vec2(1.0f, 1.0f));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texLeaf);
        sh.setInt("textureSampler", 0);
        // Slight green tint multiplier
        sh.setVec3("objectColor", glm::vec3(1.0f, 1.0f, 1.0f));
        glBindVertexArray(forest.leafVAO);
        for (int ti = tStart; ti <= tEnd; ti++) {
            glm::mat4 model = glm::translate(glm::mat4(1.0f),
                                             glm::vec3(ti * FOREST_TILE_LEN, 0.0f, 0.0f));
            sh.setMat4("model", model);
            // 12 verts per leaf (crossed billboard quads)
            glDrawArraysInstanced(GL_TRIANGLES, 0, 12,
                                  (GLsizei)forest.leafInstances.size());
        }
        sh.setBool("alphaTest", false);       // restore for the rest of the scene
    }

    glBindVertexArray(0);
    sh.setInt("textureMode", 0);
    sh.setVec2("texScale", glm::vec2(1.0f, 1.0f));
}

// Draw one entire Menger sponge in a SINGLE instanced draw call.
void drawMengerSponge(const Shader& sh, const glm::mat4& worldTransform,
                      glm::vec3 baseColor, bool emissive)
{
    if (emissive) sh.setBool("isEmissive", true);
    sh.setVec3("objectColor", baseColor);
    sh.setMat4("model", worldTransform);

    glBindVertexArray(mengerVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, bus.cube.vertexCount,
                          (GLsizei)mengerCubes.size());
    glBindVertexArray(0);

    if (emissive) sh.setBool("isEmissive", false);
}

// ============================================================================
// HUD — tiny 3x5 bitmap font, drawn with a screen-space cube primitive
// ============================================================================
// Bit layout: row 0 = top row, bit index = row*3 + col, col 0 = leftmost.
static uint16_t hudGlyph(char c) {
    switch (c) {
        case '0': return 0b111'101'101'101'111;
        case '1': return 0b010'110'010'010'111;
        case '2': return 0b111'001'111'100'111;
        case '3': return 0b111'001'111'001'111;
        case '4': return 0b101'101'111'001'001;
        case '5': return 0b111'100'111'001'111;
        case '6': return 0b111'100'111'101'111;
        case '7': return 0b111'001'010'010'010;
        case '8': return 0b111'101'111'101'111;
        case '9': return 0b111'101'111'001'111;
        case 'S': return 0b111'100'111'001'111;
        case 'C': return 0b111'100'100'100'111;
        case 'O': return 0b111'101'101'101'111;
        case 'R': return 0b110'101'110'101'101;
        case 'E': return 0b111'100'110'100'111;
        case ':': return 0b000'010'000'010'000;
        case '-': return 0b000'000'111'000'000;
        default:  return 0;
    }
}

void drawHUD(Shader& shader) {
    if (!hudCube.initialized) hudCube.init();

    glDisable(GL_DEPTH_TEST);

    shader.use();
    glm::mat4 ortho = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT, -10.0f, 10.0f);
    shader.setMat4("projection", ortho);
    shader.setMat4("view", glm::mat4(1.0f));
    shader.setInt("textureMode", 0);
    shader.setBool("isEmissive", true);
    shader.setFloat("alpha", 1.0f);

    char buf[32];
    snprintf(buf, sizeof(buf), "SCORE:%d", gameScore);

    bool flashing = scoreFlashTimer > 0.0f;
    glm::vec3 color = (flashing && scoreFlashIsHit)
                          ? glm::vec3(1.0f, 0.1f, 0.1f)
                          : glm::vec3(1.0f, 0.92f, 0.05f);
    float jump = 1.0f + (flashing ? scoreFlashTimer * 1.4f : 0.0f);   // peak ~1.6
    float bounce = flashing ? sinf(scoreFlashTimer * 18.0f) * 6.0f : 0.0f;

    float pixelSize = 7.0f * jump;
    float charW = 3.0f * pixelSize;
    float charH = 5.0f * pixelSize;
    float spacing = pixelSize;
    int n = (int)strlen(buf);
    float totalW = n * (charW + spacing) - spacing;

    float startX = (float)SCR_WIDTH - totalW - 24.0f;
    float startY = (float)SCR_HEIGHT - charH - 24.0f - bounce;

    for (int ci = 0; ci < n; ci++) {
        uint16_t g = hudGlyph(buf[ci]);
        float gx = startX + ci * (charW + spacing);
        for (int row = 0; row < 5; row++) {
            for (int col = 0; col < 3; col++) {
                if (g & (1 << (14 - (row * 3 + col)))) {
                    float px = gx + col * pixelSize;
                    float py = startY + (4 - row) * pixelSize;
                    glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                                 glm::vec3(px + pixelSize * 0.5f,
                                                           py + pixelSize * 0.5f, 0.0f));
                    m = glm::scale(m, glm::vec3(pixelSize * 0.5f));
                    hudCube.draw(shader, m, color);
                }
            }
        }
    }

    shader.setBool("isEmissive", false);
    glEnable(GL_DEPTH_TEST);
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

    // [REMOVED] Water tower init

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

    // [REMOVED] Bollard init

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

    // Pre-bake the Menger sponge fractal (iteration 2 = 400 child cubes per sponge)
    std::cout << "  Building Menger sponge fractal..." << std::flush;
    buildMengerSponge(4);
    initMengerInstancing();
    std::cout << " done (" << mengerCubes.size() << " cubes, instanced)" << std::endl;

    // Pre-bake the fractal forest tile (recursive trees, GPU-instanced)
    std::cout << "  Building fractal forest..." << std::flush;
    buildForest();
    initForestInstancing();
    std::cout << " done (" << forest.branchInstances.size() << " branches, "
              << forest.leafInstances.size() << " leaves)" << std::endl;

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

    // [REMOVED] Eiffel Tower init

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

    // Forest textures: real bark (RGB) + leaf cutout (RGBA with alpha)
    texBark = loadTexture("textures/tree_bark.jpg",   GL_REPEAT, GL_LINEAR);
    if (texBark == 0)
        texBark = loadTexture("textures/tree_bark_2.jpg", GL_REPEAT, GL_LINEAR);
    texLeaf = loadTextureRGBA("textures/leaf.png", GL_CLAMP_TO_EDGE, GL_LINEAR);

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

        // Tick gamification timers
        if (scoreFlashTimer    > 0.0f) scoreFlashTimer    = std::max(0.0f, scoreFlashTimer    - deltaTime);
        if (buildingHitCooldown> 0.0f) buildingHitCooldown= std::max(0.0f, buildingHitCooldown- deltaTime);

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
        ourShader.setBool("alphaTest", false);
        ourShader.setFloat("alpha", 1.0f);

        // View & Projection
        float aspect = (float)fbWidth / (float)fbHeight;
        glm::mat4 projection = glm::perspective(glm::radians(cameraFOV), aspect, 0.1f, 1000.0f);
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
            const float GROUND_SIZE = 1500.0f;
            if (texGrass != 0) {
                ourShader.setInt("textureMode", 3);
                // Tile grass: 1 repeat per 10 world units for natural look at scale
                ourShader.setVec2("texScale", glm::vec2(GROUND_SIZE / 10.0f, GROUND_SIZE / 10.0f));
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
                    // Tile road texture: ~1 repeat per 8 world units along length
                    ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 8.0f, ROAD_WIDTH / 8.0f));
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
                int numDashes = 6;
                float dashLen = ROAD_SEGMENT_LEN / (numDashes * 2.0f);
                for (int d = 0; d < numDashes; d++) {
                    float dx = segX + d * (dashLen * 2.0f) + dashLen * 0.5f;
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(dx, 0.01f, 0.0f));
                    model = glm::scale(model, glm::vec3(dashLen * 0.8f, 0.02f, 0.25f));
                    bus.cube.draw(ourShader, model, glm::vec3(1.0f, 1.0f, 1.0f));
                }
            }

            // --- GROUND STRIPS (both sides) ---
            // Three zones per side, blended smoothly:
            //   1) Sidewalk strip (road edge → building zone start): road→carpet blend
            //   2) Building zone (BUILDING_ZONE_START → BUILDING_ZONE_END): carpet texture
            //   3) Outskirts (BUILDING_ZONE_END → GRASS_WIDTH): carpet→grass blend
            for (int side = -1; side <= 1; side += 2) {
                // --- Zone 1: Sidewalk/transition strip (road → carpet blend) ---
                float sidewalkWidth = BUILDING_ZONE_START - ROAD_WIDTH * 0.5f;
                if (sidewalkWidth > 0.0f) {
                    float swZ = side * (ROAD_WIDTH * 0.5f + sidewalkWidth * 0.5f);
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex == 0) carpTex = texEarthTone;
                    if (carpTex != 0 && texRoad != 0) {
                        // Smooth road→carpet blend using textureMode 4
                        ourShader.setInt("textureMode", 4);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 8.0f, sidewalkWidth / 3.0f));
                        ourShader.setFloat("blendEdge", ROAD_WIDTH * 0.5f + 1.0f);
                        ourShader.setFloat("blendWidth", sidewalkWidth * 0.6f);
                        ourShader.setInt("blendAxis", 2); // Z axis
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texRoad);
                        ourShader.setInt("textureSampler", 0);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, carpTex);
                        ourShader.setInt("textureSampler2", 1);
                    } else if (carpTex != 0) {
                        ourShader.setInt("textureMode", 3);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 8.0f, sidewalkWidth / 3.0f));
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, carpTex);
                        ourShader.setInt("textureSampler", 0);
                    }
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.08f, swZ));
                    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, sidewalkWidth));
                    bus.cube.draw(ourShader, model, glm::vec3(0.45f, 0.38f, 0.30f));
                    ourShader.setInt("textureMode", 0);
                    ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                }

                // --- Zone 2: Building zone (pure carpet) ---
                float buildingZoneWidth = BUILDING_ZONE_END - BUILDING_ZONE_START;
                float bzZ = side * (BUILDING_ZONE_START + buildingZoneWidth * 0.5f);
                {
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex == 0) carpTex = texEarthTone;
                    if (carpTex != 0) {
                        ourShader.setInt("textureMode", 3);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 8.0f, buildingZoneWidth / 8.0f));
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, carpTex);
                        ourShader.setInt("textureSampler", 0);
                    }
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.1f, bzZ));
                    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, buildingZoneWidth));
                    bus.cube.draw(ourShader, model, glm::vec3(0.50f, 0.40f, 0.30f));
                    ourShader.setInt("textureMode", 0);
                    ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                }

                // --- Zone 3: Outskirts (carpet → grass blend) ---
                float outerWidth = GRASS_WIDTH - (BUILDING_ZONE_END - ROAD_WIDTH * 0.5f);
                if (outerWidth > 0.0f) {
                    float outerZ = side * (BUILDING_ZONE_END + outerWidth * 0.5f);
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex != 0 && texGrass != 0) {
                        // Smooth carpet→grass blend
                        ourShader.setInt("textureMode", 4);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 10.0f, outerWidth / 10.0f));
                        ourShader.setFloat("blendEdge", BUILDING_ZONE_END + 3.0f);
                        ourShader.setFloat("blendWidth", 8.0f);
                        ourShader.setInt("blendAxis", 2);
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, carpTex);
                        ourShader.setInt("textureSampler", 0);
                        glActiveTexture(GL_TEXTURE1);
                        glBindTexture(GL_TEXTURE_2D, texGrass);
                        ourShader.setInt("textureSampler2", 1);
                    } else if (texGrass != 0) {
                        ourShader.setInt("textureMode", 3);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 10.0f, outerWidth / 10.0f));
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, texGrass);
                        ourShader.setInt("textureSampler", 0);
                    }
                    glm::mat4 model = glm::translate(glm::mat4(1.0f),
                        glm::vec3(segX + ROAD_SEGMENT_LEN * 0.5f, -0.12f, outerZ));
                    model = glm::scale(model, glm::vec3(ROAD_SEGMENT_LEN, 0.1f, outerWidth));
                    bus.cube.draw(ourShader, model, glm::vec3(0.15f, 0.45f, 0.1f));
                    ourShader.setInt("textureMode", 0);
                    ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                }
            }

            // --- BUILDINGS (both sides) - urban grid layout with entry paths ---
            int buildingsPerSide = 4;
            for (int side = -1; side <= 1; side += 2) {
                for (int b = 0; b < buildingsPerSide; b++) {
                    int bSeed = segID * 100 + side * 50 + b;

                    // Grid-like placement: evenly spaced along segment, staggered rows
                    float spacing = ROAD_SEGMENT_LEN / (buildingsPerSide + 1);
                    float bx = segX + spacing * (b + 1);
                    // Three depth rows for urban feel
                    float rowDists[] = {
                        BUILDING_ZONE_START + 6.0f,
                        BUILDING_ZONE_START + 22.0f + cityRand(bSeed, 20) * 10.0f,
                        BUILDING_ZONE_START + 42.0f + cityRand(bSeed, 21) * 8.0f
                    };
                    float bz = side * rowDists[b % 3];

                    // Random building type: 0=stacked cubes, 1=tall building, 2=cone tower
                    int bType = (int)(cityRand(bSeed, 3) * 3.0f);
                    int colorIdx = (int)(cityRand(bSeed, 4) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;
                    glm::vec3 bColor = buildingPalette[colorIdx];

                    // Choose texture (no emoji - doesn't tile well at scale)
                    int texChoice = (int)(cityRand(bSeed, 10) * 5.0f);
                    unsigned int bTex = 0;
                    int bTexMode = 3;
                    if (texChoice == 0 && texContainer != 0) { bTex = texContainer; }
                    else if (texChoice == 1 && texWall != 0) { bTex = texWall; }
                    else if (texChoice == 2 && texStoneWall != 0) { bTex = texStoneWall; }
                    else if (texChoice == 3 && texBrickWall != 0) { bTex = texBrickWall; }
                    else if (texChoice == 4 && texRoofTile != 0) { bTex = texRoofTile; }

                    // --- CARPET ENTRY PATH from building to road edge ---
                    {
                        float roadEdgeZ = side * (ROAD_WIDTH * 0.5f + 0.5f);
                        float pathLen = fabs(bz - roadEdgeZ);
                        float pathCenterZ = (bz + roadEdgeZ) * 0.5f;
                        float pathWidth = 2.2f;

                        unsigned int pathTex = (b % 2 == 0) ? texEarthTone : texCarpetTile;
                        if (pathTex == 0) pathTex = texCarpetTile;
                        if (pathTex == 0) pathTex = texEarthTone;
                        if (pathTex != 0) {
                            ourShader.setInt("textureMode", 3);
                            // Tile proportionally: 1 texture repeat per ~3 world units
                            ourShader.setVec2("texScale", glm::vec2(pathWidth / 3.0f, pathLen / 3.0f));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, pathTex);
                            ourShader.setInt("textureSampler", 0);
                        }
                        glm::mat4 pathModel = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, -0.06f, pathCenterZ));
                        pathModel = glm::scale(pathModel, glm::vec3(pathWidth, 0.06f, pathLen));
                        bus.cube.draw(ourShader, pathModel, glm::vec3(0.55f, 0.45f, 0.35f));
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                    }

                    // --- BUILDING FOUNDATION (raised solid platform) ---
                    {
                        float padW, padD;
                        if (bType == 0) {
                            padW = 6.0f + cityRand(bSeed, 6) * 4.0f;
                            padD = 6.0f + cityRand(bSeed, 8) * 4.0f;
                        } else if (bType == 1) {
                            padW = 7.0f + cityRand(bSeed, 5) * 6.0f;
                            padD = 7.0f + cityRand(bSeed, 7) * 6.0f;
                        } else {
                            float r = 2.0f + cityRand(bSeed, 5) * 3.0f;
                            padW = r * 3.0f;
                            padD = r * 3.0f;
                        }
                        float padMargin = 2.5f;
                        float pw = padW + padMargin * 2.0f;
                        float pd = padD + padMargin * 2.0f;
                        float padH = 0.5f; // visible raised slab

                        // Main solid platform
                        unsigned int padTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                        if (padTex == 0) padTex = texCarpetTile;
                        if (padTex == 0) padTex = texEarthTone;
                        if (padTex != 0) {
                            ourShader.setInt("textureMode", 3);
                            ourShader.setVec2("texScale", glm::vec2(pw / 4.0f, pd / 4.0f));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, padTex);
                            ourShader.setInt("textureSampler", 0);
                        }
                        glm::mat4 padModel = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, padH * 0.5f, bz));
                        padModel = glm::scale(padModel, glm::vec3(pw, padH, pd));
                        bus.cube.draw(ourShader, padModel, glm::vec3(0.45f, 0.38f, 0.30f));
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));
                    }

                    float padH = 0.5f; // must match foundation height above

                    if (bType == 0) {
                        // ---- STACKED CUBES (bigger) ----
                        int numCubes = 1 + (int)(cityRand(bSeed, 5) * 3.0f);
                        float yOffset = padH;
                        for (int c = 0; c < numCubes; c++) {
                            float cw = 4.0f + cityRand(bSeed * 10 + c, 6) * 5.0f;
                            float ch = 3.0f + cityRand(bSeed * 10 + c, 7) * 7.0f;
                            float cd = 4.0f + cityRand(bSeed * 10 + c, 8) * 5.0f;
                            int cc = (int)(cityRand(bSeed * 10 + c, 9) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;

                            if (bTex != 0) {
                                ourShader.setInt("textureMode", bTexMode);
                                // Tile texture: ~1 repeat per 4 world units
                                ourShader.setVec2("texScale", glm::vec2(cw / 4.0f, ch / 4.0f));
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
                        // ---- TALL BUILDING (bigger) ----
                        float bw = 5.0f + cityRand(bSeed, 5) * 6.0f;
                        float bh = 10.0f + cityRand(bSeed, 6) * 25.0f;
                        float bd = 5.0f + cityRand(bSeed, 7) * 6.0f;

                        if (bTex != 0) {
                            ourShader.setInt("textureMode", bTexMode);
                            ourShader.setVec2("texScale", glm::vec2(bw / 4.0f, bh / 4.0f));
                            glActiveTexture(GL_TEXTURE0);
                            glBindTexture(GL_TEXTURE_2D, bTex);
                            ourShader.setInt("textureSampler", 0);
                        }

                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(bx, padH + bh * 0.5f, bz));
                        model = glm::scale(model, glm::vec3(bw, bh, bd));
                        bus.cube.draw(ourShader, model, bColor);
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                        addBuildingCollision(glm::vec3(bx, padH + bh * 0.5f, bz),
                                             glm::vec3(bw * 0.5f, bh * 0.5f, bd * 0.5f));

                        // Windows (scaled up)
                        int wRows = (int)(bh / 3.0f);
                        int wCols = (int)(bw / 2.5f);
                        if (wCols < 1) wCols = 1;
                        for (int wr = 0; wr < wRows && wr < 8; wr++) {
                            for (int wc = 0; wc < wCols && wc < 4; wc++) {
                                float wx = bx - bw * 0.35f + wc * (bw * 0.7f / std::max(wCols - 1, 1));
                                float wy = padH + 3.0f + wr * 3.0f;
                                float wz = bz + (side > 0 ? -bd * 0.52f : bd * 0.52f);
                                glm::mat4 wModel = glm::translate(glm::mat4(1.0f),
                                    glm::vec3(wx, wy, wz));
                                wModel = glm::scale(wModel, glm::vec3(1.2f, 1.6f, 0.08f));
                                bus.cube.draw(ourShader, wModel, glm::vec3(0.05f, 0.08f, 0.15f));
                            }
                        }
                    }
                    else {
                        // ---- CONE-TOPPED TOWER (bigger) ----
                        float radius = 2.0f + cityRand(bSeed, 5) * 3.0f;
                        float towerH = 8.0f + cityRand(bSeed, 6) * 16.0f;
                        float coneH = 3.0f + cityRand(bSeed, 7) * 4.0f;

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
                            glm::vec3(bx, padH + towerH * 0.5f, bz));
                        model = glm::scale(model, glm::vec3(radius, towerH, radius));
                        bus.cylinder.draw(ourShader, model, bColor);
                        ourShader.setInt("textureMode", 0);
                        ourShader.setVec2("texScale", glm::vec2(1.0f, 1.0f));

                        addBuildingCollision(glm::vec3(bx, padH + (towerH + coneH) * 0.5f, bz),
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
                            glm::vec3(bx, padH + towerH + coneH * 0.5f, bz));
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

            // --- BEZIER VASES along the road (infinite, scaled up) ---
            {
                float vaseSpacing = 40.0f;
                int vaseStart = (int)floor((busX - VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / vaseSpacing);
                int vaseEnd = (int)ceil((busX + VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / vaseSpacing);
                for (int i = vaseStart; i <= vaseEnd; i++) {
                    float vaseX = i * vaseSpacing;
                    for (int side = -1; side <= 1; side += 2) {
                        float vaseZ = side * (ROAD_WIDTH * 0.5f + 2.0f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(vaseX, 0.0f, vaseZ));
                        model = glm::scale(model, glm::vec3(1.8f, 3.5f, 1.8f));
                        bezierVase.draw(ourShader, model, glm::vec3(0.7f, 0.25f, 0.1f));
                        addBuildingCollision(glm::vec3(vaseX, 1.5f, vaseZ),
                                             glm::vec3(1.0f, 1.5f, 1.0f));
                    }
                }
            }

            // [REMOVED] Water towers

            // --- SPLINE STREET LAMPS along road (infinite, scaled up) ---
            {
                float lampSpacing = 30.0f;
                int lampStart = (int)floor((busX - VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / lampSpacing);
                int lampEnd = (int)ceil((busX + VISIBLE_SEGMENTS * ROAD_SEGMENT_LEN * 0.5f) / lampSpacing);
                for (int i = lampStart; i <= lampEnd; i++) {
                    float lampX = i * lampSpacing + 15.0f;
                    for (int side = -1; side <= 1; side += 2) {
                        float lampZ = side * (ROAD_WIDTH * 0.5f + 1.2f);
                        glm::mat4 model = glm::translate(glm::mat4(1.0f),
                            glm::vec3(lampX, 0.0f, lampZ));
                        model = glm::scale(model, glm::vec3(1.0f, 8.0f, 1.0f));
                        splineLamp.draw(ourShader, model, glm::vec3(0.3f, 0.3f, 0.35f));

                        if (emissiveLightOn) {
                            ourShader.setBool("isEmissive", true);
                            glm::mat4 glowModel = glm::translate(glm::mat4(1.0f),
                                glm::vec3(lampX, 7.8f, lampZ));
                            glowModel = glm::scale(glowModel, glm::vec3(0.6f, 0.6f, 0.6f));
                            sceneSphere.draw(ourShader, glowModel, glm::vec3(1.0f, 0.9f, 0.5f));
                            ourShader.setBool("isEmissive", false);
                        }

                        addBuildingCollision(glm::vec3(lampX, 4.0f, lampZ),
                                             glm::vec3(0.5f, 4.0f, 0.5f));
                    }
                }
            }

            // [REMOVED] Bollards

            // --- RULED SURFACE CANOPIES (bus stop shelters, infinite, scaled up) ---
            {
                float canopySpacing = 300.0f;
                int cStart = (int)floor((busX - 600.0f) / canopySpacing);
                int cEnd = (int)ceil((busX + 600.0f) / canopySpacing);
                for (int ci = cStart; ci <= cEnd; ci++) {
                    float cx = ci * canopySpacing + 60.0f;
                    float cside = (ci % 2 == 0) ? 1.0f : -1.0f;
                    glm::vec3 cp(cx, 0.0f, cside * (ROAD_WIDTH * 0.5f + 4.0f));
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), cp);
                    model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
                    ruledCanopy.draw(ourShader, model, glm::vec3(0.6f, 0.65f, 0.7f));

                    for (int p = -1; p <= 1; p += 2) {
                        glm::mat4 pillar = glm::translate(glm::mat4(1.0f),
                            cp + glm::vec3(p * 4.2f, 3.0f, 3.0f));
                        pillar = glm::scale(pillar, glm::vec3(0.2f, 6.0f, 0.2f));
                        bus.cube.draw(ourShader, pillar, glm::vec3(0.4f, 0.4f, 0.45f));
                        addBuildingCollision(cp + glm::vec3(p * 4.2f, 3.0f, 3.0f),
                                             glm::vec3(0.2f, 3.0f, 0.2f));
                    }
                }
            }

            // [REMOVED] Eiffel Tower

            // --- FRACTAL FOREST (textured trees, fully instanced) ---
            drawForest(ourShader, busX);

            // --- MENGER SPONGE FRACTAL COLLECTIBLES (high-altitude, iter 3) ---
            // 3-iteration Menger sponges floating HIGH above the road and rings.
            // Each is a 20^3 = 8000-cube self-similar fractal, baked once at
            // startup. Sparse spacing keeps only ~3 in the visible window so
            // total cube draws stay manageable. Flying THROUGH one collects it
            // and awards bonus points (worth more than ring checkpoints).
            {
                float spongeSpacing = 260.0f;  // sparse: ~3 visible at a time
                int sStart = (int)floor((busX - 400.0f) / spongeSpacing);
                int sEnd   = (int)ceil ((busX + 400.0f) / spongeSpacing);
                for (int si = sStart; si <= sEnd; si++) {
                    // Position HIGH above the road, centered over the road on X axis
                    float sx = si * spongeSpacing + 90.0f;

                    unsigned int sseed = (unsigned int)(si * 374761393);
                    float r1 = (cityHash(sseed, 1) % 1000) / 1000.0f;
                    float r2 = (cityHash(sseed, 2) % 1000) / 1000.0f;
                    float r3 = (cityHash(sseed, 3) % 1000) / 1000.0f;

                    // Centered above the road, slight Z wobble (player must steer)
                    float sz = (r3 - 0.5f) * 6.0f;
                    // High altitude band 22..36, varying so player must climb/dive
                    float baseY = 22.0f + r1 * 14.0f;
                    float bobY  = sin(time * 1.0f + si * 0.7f) * 1.2f;
                    float sy    = baseY + bobY;

                    float spongeSize = 4.0f + r2 * 1.5f;     // 4..5.5 units
                    // Continuous slow tumble on TWO axes for visual complexity
                    float rotY = time * (20.0f + r1 * 15.0f);
                    float rotX = time * (12.0f + r2 * 10.0f);

                    bool collected = collectedSponges.count(si) > 0;

                    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
                    m = glm::rotate(m, glm::radians(rotY), glm::vec3(0, 1, 0));
                    m = glm::rotate(m, glm::radians(rotX), glm::vec3(1, 0, 0));
                    m = glm::scale(m, glm::vec3(spongeSize));

                    // Bright pulsing color; collected ones go dim grey
                    glm::vec3 palette[] = {
                        glm::vec3(0.20f, 0.85f, 1.00f),  // cyan
                        glm::vec3(1.00f, 0.40f, 0.85f),  // magenta
                        glm::vec3(0.55f, 1.00f, 0.35f),  // lime
                        glm::vec3(1.00f, 0.75f, 0.20f),  // amber
                        glm::vec3(0.70f, 0.45f, 1.00f),  // violet
                    };
                    int colorIdx = (((si % 5) + 5) % 5);
                    float pulse = 0.75f + 0.25f * sin(time * 3.0f + si);
                    glm::vec3 col = collected ? glm::vec3(0.25f, 0.25f, 0.25f)
                                              : palette[colorIdx] * pulse;

                    drawMengerSponge(ourShader, m, col, emissiveLightOn && !collected);

                    // --- Pass-through detection (no solid collision) ---
                    if (!collected) {
                        glm::vec3 busCenter = busPosition;
                        busCenter.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
                        glm::vec3 d = busCenter - glm::vec3(sx, sy, sz);
                        float reach = spongeSize * 0.9f;  // generous hit volume
                        if (fabs(d.x) < reach && fabs(d.y) < reach && fabs(d.z) < reach) {
                            collectedSponges.insert(si);
                            fractalScore += 100;
                            awardScore(15);
                            std::cout << ">>> CUBE COLLECTED! +15 (Score: "
                                      << gameScore << ") <<<" << std::endl;
                        }
                    }
                }
            }

            // --- RING CHECKPOINTS (infinite, sparse, varied shapes) ---
            {
                float ringSpacing = 180.0f;
                int ringStart = (int)floor((busX - 600.0f) / ringSpacing);
                int ringEnd = (int)ceil((busX + 600.0f) / ringSpacing);
                for (int ri = ringStart; ri <= ringEnd; ri++) {
                    float ringX = ri * ringSpacing;
                    int ringSeed = ri * 4919;
                    // Vary height between 12-22 units (bigger world = higher rings)
                    float ringY = 15.0f + 6.0f * sin(ri * 0.9f);
                    // Slight Z offset for variety
                    float ringZ = sin(ri * 1.7f) * 5.0f;

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
                    bool ringPassed = passedRings.count(ri) > 0;
                    float pulse = 0.7f + 0.3f * sin(time * 4.0f + ri);
                    glm::vec3 ringColor = ringPassed
                        ? glm::vec3(0.35f, 0.35f, 0.38f)   // ash/grey once collected
                        : ringColors[shapeType] * pulse;

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
                    if (!ringPassed && distXZ < 2.0f && distYZ < 6.0f * 0.8f) {
                        passedRings.insert(ri);
                        // Circle ring (shapeType 0) = "ring" = +5; polygon rings (hex etc.) = +3
                        int pts = (shapeType == 0) ? 5 : 3;
                        awardScore(pts);
                        std::cout << ">> RING " << ri << " PASSED! +" << pts
                                  << " (Score: " << gameScore << ") <<" << std::endl;
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

        // ==================== HUD (score) ====================
        drawHUD(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    bus.cleanup();
    sceneSphere.cleanup();
    sceneCone.cleanup();
    bezierVase.cleanup();
    // [REMOVED] bezierWaterTower.cleanup();
    splineLamp.cleanup();
    // [REMOVED] splineBollard.cleanup();
    ruledCanopy.cleanup();
    ringCheckpoint.cleanup();
    hexRing.cleanup();
    triRing.cleanup();
    squareRing.cleanup();
    pentRing.cleanup();
    // [REMOVED] Eiffel tower cleanup
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
        case GLFW_KEY_N: bus.toggleWings();
            std::cout << "Wings: " << (bus.wingsEnabled ? "ON" : "OFF") << std::endl; break;
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