#ifndef GLOBALS_H
#define GLOBALS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <unordered_set>
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "Shader.h"
#include "Bus.h"

// ============================================================================
// SCREEN SETTINGS
// ============================================================================
extern const unsigned int SCR_WIDTH;
extern const unsigned int SCR_HEIGHT;

// ============================================================================
// CAMERA SYSTEM
// ============================================================================
extern int cameraMode;
extern const int NUM_CAMERA_MODES;
extern const char* cameraModeNames[];

// Free camera
extern glm::vec3 cameraPos;
extern float cameraPitch;
extern float cameraYaw;
extern float cameraRoll;
extern float cameraFOV;

// Mouse state
extern bool mouseCaptured;
extern bool firstMouse;
extern float lastMouseX;
extern float lastMouseY;
extern float mouseSensitivity;

// Orbit
extern float orbitAngle;
extern float orbitRadius;
extern float orbitHeight;

extern float deltaTime;
extern float lastFrame;

// ============================================================================
// BUS & DRIVING
// ============================================================================
extern Bus bus;
extern bool fanSpinning;

extern bool isDrivingMode;
extern glm::vec3 busPosition;
extern float busAltitude;
extern float busVerticalSpeed;
extern float busYaw;
extern float busSpeed;
extern float busSteerAngle;

extern const float ACCELERATION;
extern const float DECELERATION;
extern const float MAX_SPEED;
extern const float STEER_SPEED;
extern const float MAX_STEER;
extern const float HOVER_HEIGHT;
extern const float VERTICAL_ACCEL;
extern const float MAX_ALTITUDE;

// ============================================================================
// GAMIFICATION
// ============================================================================
extern int fractalScore;
extern std::unordered_set<int> collectedSponges;

extern int   gameScore;
extern float scoreFlashTimer;
extern bool  scoreFlashIsHit;
extern std::unordered_set<int> passedRings;
extern float buildingHitCooldown;
extern Cube  hudCube;

inline void awardScore(int delta) {
    gameScore += delta;
    scoreFlashTimer = 0.45f;
    scoreFlashIsHit = (delta < 0);
}

// ============================================================================
// GLOBAL LIGHTING STATE
// ============================================================================
extern bool dirLightOn;
extern bool pointLightsOn;
extern bool spotLightOn;
extern bool emissiveLightOn;
extern bool ambientOn;
extern bool diffuseOn;
extern bool specularOn;

// ============================================================================
// TEXTURE STATE
// ============================================================================
extern unsigned int texFloor, texCarpet, texFabric;
extern unsigned int texWall, texDashboard, texBusBody;
extern unsigned int texSphere, texCone;
extern unsigned int texRoad, texGrass;
extern unsigned int texContainer, texEmoji;
extern unsigned int texStoneWall, texRoofTile, texBrickWall;
extern unsigned int texCarpetTile, texEarthTone;
extern unsigned int texBark, texLeaf;

// Skybox
extern unsigned int skyboxVAO, skyboxVBO;
extern unsigned int cubemapTexture;

// Scene primitives
extern Sphere sceneSphere;
extern Cone sceneCone;

// Bezier/Spline surface objects
extern BezierSurface bezierVase;
extern SplineSurface splineLamp;
extern RuledSurface  ruledCanopy;

// Ring checkpoints
extern Torus ringCheckpoint;
extern PolygonRing hexRing, triRing, squareRing, pentRing;

extern int sceneTextureMode;

// Texture mode / wrap / filter
extern int currentWrapIndex;
extern GLenum wrapModes[];
extern const char* wrapNames[];
extern const int NUM_WRAP_MODES;

extern int currentFilterIndex;
extern GLenum filterModes[];
extern const char* filterNames[];
extern const int NUM_FILTER_MODES;

extern const char* textureModeNames[];

// ============================================================================
// CITY ENVIRONMENT CONSTANTS
// ============================================================================
extern const float ROAD_WIDTH;
extern const float ROAD_SEGMENT_LEN;
extern const int   VISIBLE_SEGMENTS;
extern const float GRASS_WIDTH;
extern const float BUILDING_ZONE_START;
extern const float BUILDING_ZONE_END;
extern const int   BUILDINGS_PER_SEGMENT;

// Building palette
extern glm::vec3 buildingPalette[];
extern const int NUM_PALETTE_COLORS;

// ============================================================================
// UTILITY FUNCTIONS (used by multiple modules)
// ============================================================================
inline unsigned int cityHash(int x, int y) {
    unsigned int h = static_cast<unsigned int>(x) * 374761393u + static_cast<unsigned int>(y) * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177;
    return h ^ (h >> 16);
}

inline float cityRand(int seed, int id) {
    return (float)(cityHash(seed, id) % 10000) / 10000.0f;
}

// ============================================================================
// COLLISION SYSTEM
// ============================================================================
struct AABB {
    glm::vec3 minPt;
    glm::vec3 maxPt;
};

extern std::vector<AABB> collisionBoxes;

struct RingCheckpoint {
    glm::vec3 position;
    float yaw;
    float radius;
    bool passed;
};
extern std::vector<RingCheckpoint> ringPositions;

// ============================================================================
// MENGER SPONGE DATA
// ============================================================================
struct MengerCube {
    glm::vec3 offset;
    float     size;
};
extern std::vector<MengerCube> mengerCubes;
extern unsigned int mengerVAO;
extern unsigned int mengerInstVBO;

// ============================================================================
// FOREST DATA
// ============================================================================
struct ForestData {
    std::vector<glm::mat4> branchInstances;
    std::vector<glm::mat4> leafInstances;
    unsigned int branchVAO = 0, branchInstVBO = 0;
    unsigned int leafVAO   = 0, leafInstVBO   = 0;
    unsigned int leafQuadVBO   = 0;
    unsigned int branchGeomVBO = 0;
    int branchVertCount = 0;
};
extern ForestData forest;
extern const float FOREST_TILE_LEN;

#endif
