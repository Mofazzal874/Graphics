// ============================================================================
// STB_IMAGE for texture loading (must be defined exactly once)
// ============================================================================
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

// ============================================================================
// MODULE INCLUDES
// ============================================================================
#include "Globals.h"
#include "Camera.h"
#include "TextureLoader.h"
#include "MengerSponge.h"
#include "Forest.h"
#include "Collision.h"
#include "HUD.h"
#include "InputHandler.h"

// ============================================================================
// GLOBAL VARIABLE DEFINITIONS
// ============================================================================

// Screen settings
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

// Camera system
int cameraMode = 1;
const int NUM_CAMERA_MODES = 3;
const char* cameraModeNames[] = { "FREE CAMERA", "CHASE CAMERA (3rd person)", "INTERIOR CAMERA (1st person)" };

glm::vec3 cameraPos = glm::vec3(0.0f, 5.0f, 20.0f);
float cameraPitch = -15.0f;
float cameraYaw = -90.0f;
float cameraRoll = 0.0f;
float cameraFOV = 45.0f;

bool mouseCaptured = false;
bool firstMouse = true;
float lastMouseX = SCR_WIDTH / 2.0f;
float lastMouseY = SCR_HEIGHT / 2.0f;
float mouseSensitivity = 0.1f;

float orbitAngle = 0.0f;
float orbitRadius = 20.0f;
float orbitHeight = 10.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Bus & driving
Bus bus;
bool fanSpinning = false;

bool isDrivingMode = true;
glm::vec3 busPosition = glm::vec3(0.0f, 0.0f, 0.0f);
float busAltitude = 0.0f;
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

// Gamification
int fractalScore = 0;
std::unordered_set<int> collectedSponges;

int   gameScore = 0;
float scoreFlashTimer = 0.0f;
bool  scoreFlashIsHit = false;
std::unordered_set<int> passedRings;
float buildingHitCooldown = 0.0f;
Cube  hudCube;

// Lighting state
bool dirLightOn = true;
bool pointLightsOn = true;
bool spotLightOn = true;
bool emissiveLightOn = true;
bool ambientOn = true;
bool diffuseOn = true;
bool specularOn = true;

// Texture state
unsigned int texFloor = 0, texCarpet = 0, texFabric = 0;
unsigned int texWall = 0, texDashboard = 0, texBusBody = 0;
unsigned int texSphere = 0, texCone = 0;
unsigned int texRoad = 0, texGrass = 0;
unsigned int texContainer = 0, texEmoji = 0;
unsigned int texStoneWall = 0, texRoofTile = 0, texBrickWall = 0;
unsigned int texCarpetTile = 0, texEarthTone = 0;
unsigned int texBark = 0, texLeaf = 0;

unsigned int skyboxVAO = 0, skyboxVBO = 0;
unsigned int cubemapTexture = 0;

Sphere sceneSphere;
Cone sceneCone;

BezierSurface bezierVase;
SplineSurface splineLamp;
RuledSurface  ruledCanopy;

Torus ringCheckpoint;
PolygonRing hexRing, triRing, squareRing, pentRing;

int sceneTextureMode = 1;

int currentWrapIndex = 0;
GLenum wrapModes[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_MIRRORED_REPEAT };
const char* wrapNames[] = { "GL_REPEAT", "GL_CLAMP_TO_EDGE", "GL_MIRRORED_REPEAT" };
const int NUM_WRAP_MODES = 3;

int currentFilterIndex = 0;
GLenum filterModes[] = { GL_LINEAR, GL_NEAREST };
const char* filterNames[] = { "GL_LINEAR", "GL_NEAREST" };
const int NUM_FILTER_MODES = 2;

const char* textureModeNames[] = { "OFF", "PURE TEXTURE", "VERTEX-BLENDED (Gouraud)", "FRAGMENT-BLENDED (Phong)", "MULTI-TEXTURE BLEND" };

// City environment constants
const float ROAD_WIDTH = 14.0f;
const float ROAD_SEGMENT_LEN = 40.0f;
const int   VISIBLE_SEGMENTS = 20;
const float GRASS_WIDTH = 100.0f;
const float BUILDING_ZONE_START = 10.0f;
const float BUILDING_ZONE_END = 70.0f;
const int   BUILDINGS_PER_SEGMENT = 6;

glm::vec3 buildingPalette[] = {
    glm::vec3(0.85f, 0.2f, 0.2f),
    glm::vec3(0.2f, 0.65f, 0.9f),
    glm::vec3(0.2f, 0.8f, 0.3f),
    glm::vec3(0.9f, 0.85f, 0.1f),
    glm::vec3(0.7f, 0.3f, 0.85f),
    glm::vec3(0.95f, 0.55f, 0.1f),
    glm::vec3(0.1f, 0.85f, 0.75f),
    glm::vec3(0.85f, 0.15f, 0.55f),
    glm::vec3(0.5f, 0.5f, 0.85f),
    glm::vec3(0.3f, 0.75f, 0.5f),
};
const int NUM_PALETTE_COLORS = 10;

// Collision
std::vector<AABB> collisionBoxes;
std::vector<RingCheckpoint> ringPositions;

// Menger sponge
std::vector<MengerCube> mengerCubes;
unsigned int mengerVAO = 0;
unsigned int mengerInstVBO = 0;

// Forest
ForestData forest;
const float FOREST_TILE_LEN = 200.0f;

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
    bus.jetEngineOn = true;
    sceneSphere.init(30, 36);
    sceneCone.init(36);

    // ==================== BEZIER/SPLINE CURVE OBJECTS ====================
    {
        std::vector<glm::vec2> vaseProfile = {
            glm::vec2(0.8f, 0.0f),
            glm::vec2(1.0f, 0.3f),
            glm::vec2(0.3f, 0.7f),
            glm::vec2(0.6f, 1.0f)
        };
        bezierVase.init(vaseProfile, 20, 24);
    }

    {
        std::vector<glm::vec2> lampProfile = {
            glm::vec2(0.3f, 0.0f),
            glm::vec2(0.15f, 0.1f),
            glm::vec2(0.08f, 0.5f),
            glm::vec2(0.08f, 0.85f),
            glm::vec2(0.25f, 0.92f),
            glm::vec2(0.2f, 1.0f)
        };
        splineLamp.init(lampProfile, 8, 20);
    }

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

    ringCheckpoint.init(6.0f, 0.6f, 36, 18);

    // Pre-bake the Menger sponge fractal
    std::cout << "  Building Menger sponge fractal..." << std::flush;
    buildMengerSponge(4);
    initMengerInstancing();
    std::cout << " done (" << mengerCubes.size() << " cubes, instanced)" << std::endl;

    // Pre-bake the fractal forest tile
    std::cout << "  Building fractal forest..." << std::flush;
    buildForest();
    initForestInstancing();
    std::cout << " done (" << forest.branchInstances.size() << " branches, "
              << forest.leafInstances.size() << " leaves)" << std::endl;

    // Polygon ring shapes
    std::cout << "  Initializing polygon rings..." << std::flush;
    hexRing.init(6, 6.0f, 0.5f, 10, 3);
    std::cout << " hex" << std::flush;
    triRing.init(3, 6.0f, 0.55f, 10, 3);
    std::cout << " tri" << std::flush;
    squareRing.init(4, 6.0f, 0.5f, 10, 3);
    std::cout << " sq" << std::flush;
    pentRing.init(5, 6.0f, 0.5f, 10, 3);
    std::cout << " pent [OK]" << std::endl;

    // ==================== SKYBOX CUBE VAO ====================
    float skyboxVertices[] = {
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

    texRoad      = loadTexture("textures/road.jpg",      GL_REPEAT,          GL_LINEAR);
    texGrass     = loadTexture("textures/grass.jpg",     GL_REPEAT,          GL_LINEAR);
    texContainer = loadTexture("textures/container2.png", GL_REPEAT,         GL_LINEAR);
    texEmoji     = loadTexture("textures/emoji.png",     GL_CLAMP_TO_EDGE,   GL_LINEAR);

    texStoneWall = loadTexture("textures/stone_wall.jpg",                  GL_REPEAT, GL_LINEAR);
    texRoofTile  = loadTexture("textures/roof_tile.jpg",                   GL_REPEAT, GL_LINEAR);
    texBrickWall = loadTexture("textures/Seamless brick wall texture.jpg", GL_REPEAT, GL_LINEAR);

    texCarpetTile = loadTexture("textures/commercial-carpet-tiles.jpg", GL_REPEAT, GL_LINEAR);
    texEarthTone  = loadTexture("textures/earth-tone-tiles.png",         GL_REPEAT, GL_LINEAR);

    texBark = loadTexture("textures/tree_bark.jpg",   GL_REPEAT, GL_LINEAR);
    if (texBark == 0)
        texBark = loadTexture("textures/tree_bark_2.jpg", GL_REPEAT, GL_LINEAR);
    texLeaf = loadTextureRGBA("textures/leaf.png", GL_CLAMP_TO_EDGE, GL_LINEAR);

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
        bus.updateFan(deltaTime, fanSpinning || cameraMode == 2);
        bus.updateJetFlame(deltaTime);

        // Tick gamification timers
        if (scoreFlashTimer    > 0.0f) scoreFlashTimer    = std::max(0.0f, scoreFlashTimer    - deltaTime);
        if (buildingHitCooldown> 0.0f) buildingHitCooldown= std::max(0.0f, buildingHitCooldown- deltaTime);

        // Clear collision boxes - rebuilt each frame from visible objects
        collisionBoxes.clear();

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.53f, 0.72f, 0.92f, 1.0f);
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

        float aspect_ = (float)fbWidth / (float)fbHeight;
        glm::mat4 projection = glm::perspective(glm::radians(cameraFOV), aspect_, 0.1f, 1000.0f);
        glm::mat4 view = getViewMatrix();

        // Spotlight points along the actual view direction
        glm::vec3 spotDir = glm::normalize(glm::vec3(glm::inverse(view) * glm::vec4(0, 0, -1, 0)));
        ourShader.setVec3("spotLight.position", cameraPos);
        ourShader.setVec3("spotLight.direction", spotDir);
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
        float busX = busPosition.x;
        float busZ = busPosition.z;
        float segStart = floor(busX / ROAD_SEGMENT_LEN) * ROAD_SEGMENT_LEN;

        // ==================== LARGE GROUND PLANE ====================
        {
            const float GROUND_SIZE = 1500.0f;
            if (texGrass != 0) {
                ourShader.setInt("textureMode", 3);
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
            for (int side = -1; side <= 1; side += 2) {
                // --- Zone 1: Sidewalk/transition strip ---
                float sidewalkWidth = BUILDING_ZONE_START - ROAD_WIDTH * 0.5f;
                if (sidewalkWidth > 0.0f) {
                    float swZ = side * (ROAD_WIDTH * 0.5f + sidewalkWidth * 0.5f);
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex == 0) carpTex = texEarthTone;
                    if (carpTex != 0 && texRoad != 0) {
                        ourShader.setInt("textureMode", 4);
                        ourShader.setVec2("texScale", glm::vec2(ROAD_SEGMENT_LEN / 8.0f, sidewalkWidth / 3.0f));
                        ourShader.setFloat("blendEdge", ROAD_WIDTH * 0.5f + 1.0f);
                        ourShader.setFloat("blendWidth", sidewalkWidth * 0.6f);
                        ourShader.setInt("blendAxis", 2);
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

                // --- Zone 2: Building zone ---
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

                // --- Zone 3: Outskirts ---
                float outerWidth = GRASS_WIDTH - (BUILDING_ZONE_END - ROAD_WIDTH * 0.5f);
                if (outerWidth > 0.0f) {
                    float outerZ = side * (BUILDING_ZONE_END + outerWidth * 0.5f);
                    unsigned int carpTex = (segID % 2 == 0) ? texCarpetTile : texEarthTone;
                    if (carpTex == 0) carpTex = texCarpetTile;
                    if (carpTex != 0 && texGrass != 0) {
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

            // --- BUILDINGS (both sides) ---
            int buildingsPerSide = 4;
            for (int side = -1; side <= 1; side += 2) {
                for (int b = 0; b < buildingsPerSide; b++) {
                    int bSeed = segID * 100 + side * 50 + b;

                    float spacing = ROAD_SEGMENT_LEN / (buildingsPerSide + 1);
                    float bx = segX + spacing * (b + 1);
                    float rowDists[] = {
                        BUILDING_ZONE_START + 6.0f,
                        BUILDING_ZONE_START + 22.0f + cityRand(bSeed, 20) * 10.0f,
                        BUILDING_ZONE_START + 42.0f + cityRand(bSeed, 21) * 8.0f
                    };
                    float bz = side * rowDists[b % 3];

                    int bType = (int)(cityRand(bSeed, 3) * 3.0f);
                    int colorIdx = (int)(cityRand(bSeed, 4) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;
                    glm::vec3 bColor = buildingPalette[colorIdx];

                    int texChoice = (int)(cityRand(bSeed, 10) * 5.0f);
                    unsigned int bTex = 0;
                    int bTexMode = 3;
                    if (texChoice == 0 && texContainer != 0) { bTex = texContainer; }
                    else if (texChoice == 1 && texWall != 0) { bTex = texWall; }
                    else if (texChoice == 2 && texStoneWall != 0) { bTex = texStoneWall; }
                    else if (texChoice == 3 && texBrickWall != 0) { bTex = texBrickWall; }
                    else if (texChoice == 4 && texRoofTile != 0) { bTex = texRoofTile; }

                    // --- CARPET ENTRY PATH ---
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

                    // --- BUILDING FOUNDATION ---
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
                        float padH = 0.5f;

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

                    float padH = 0.5f;

                    if (bType == 0) {
                        int numCubes = 1 + (int)(cityRand(bSeed, 5) * 3.0f);
                        float yOffset = padH;
                        for (int c = 0; c < numCubes; c++) {
                            float cw = 4.0f + cityRand(bSeed * 10 + c, 6) * 5.0f;
                            float ch = 3.0f + cityRand(bSeed * 10 + c, 7) * 7.0f;
                            float cd = 4.0f + cityRand(bSeed * 10 + c, 8) * 5.0f;
                            int cc = (int)(cityRand(bSeed * 10 + c, 9) * NUM_PALETTE_COLORS) % NUM_PALETTE_COLORS;

                            if (bTex != 0) {
                                ourShader.setInt("textureMode", bTexMode);
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

        // ==================== CURVE OBJECTS ====================
        {
            float time = (float)glfwGetTime();

            // --- BEZIER VASES ---
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

            // --- SPLINE STREET LAMPS ---
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

            // --- RULED SURFACE CANOPIES ---
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

            // --- FRACTAL FOREST ---
            drawForest(ourShader, busX);

            // --- MENGER SPONGE COLLECTIBLES ---
            {
                float spongeSpacing = 260.0f;
                int sStart = (int)floor((busX - 400.0f) / spongeSpacing);
                int sEnd   = (int)ceil ((busX + 400.0f) / spongeSpacing);
                for (int si = sStart; si <= sEnd; si++) {
                    float sx = si * spongeSpacing + 90.0f;

                    unsigned int sseed = (unsigned int)(si * 374761393);
                    float r1 = (cityHash(sseed, 1) % 1000) / 1000.0f;
                    float r2 = (cityHash(sseed, 2) % 1000) / 1000.0f;
                    float r3 = (cityHash(sseed, 3) % 1000) / 1000.0f;

                    float sz = (r3 - 0.5f) * 6.0f;
                    float baseY = 22.0f + r1 * 14.0f;
                    float bobY  = sin(time * 1.0f + si * 0.7f) * 1.2f;
                    float sy    = baseY + bobY;

                    float spongeSize = 4.0f + r2 * 1.5f;
                    float rotY = time * (20.0f + r1 * 15.0f);
                    float rotX = time * (12.0f + r2 * 10.0f);

                    bool collected = collectedSponges.count(si) > 0;

                    glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(sx, sy, sz));
                    m = glm::rotate(m, glm::radians(rotY), glm::vec3(0, 1, 0));
                    m = glm::rotate(m, glm::radians(rotX), glm::vec3(1, 0, 0));
                    m = glm::scale(m, glm::vec3(spongeSize));

                    glm::vec3 palette[] = {
                        glm::vec3(0.20f, 0.85f, 1.00f),
                        glm::vec3(1.00f, 0.40f, 0.85f),
                        glm::vec3(0.55f, 1.00f, 0.35f),
                        glm::vec3(1.00f, 0.75f, 0.20f),
                        glm::vec3(0.70f, 0.45f, 1.00f),
                    };
                    int ci2 = (((si % 5) + 5) % 5);
                    float pulse = 0.75f + 0.25f * sin(time * 3.0f + si);
                    glm::vec3 col = collected ? glm::vec3(0.25f, 0.25f, 0.25f)
                                              : palette[ci2] * pulse;

                    drawMengerSponge(ourShader, m, col, emissiveLightOn && !collected);

                    if (!collected) {
                        glm::vec3 busCenter = busPosition;
                        busCenter.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
                        glm::vec3 d = busCenter - glm::vec3(sx, sy, sz);
                        float reach = spongeSize * 0.9f;
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

            // --- RING CHECKPOINTS ---
            {
                float ringSpacing = 180.0f;
                int ringStart = (int)floor((busX - 600.0f) / ringSpacing);
                int ringEnd = (int)ceil((busX + 600.0f) / ringSpacing);
                for (int ri = ringStart; ri <= ringEnd; ri++) {
                    float ringX = ri * ringSpacing;
                    int ringSeed = ri * 4919;
                    float ringY = 15.0f + 6.0f * sin(ri * 0.9f);
                    float ringZ = sin(ri * 1.7f) * 5.0f;

                    float bobY = sin(time * 1.5f + ri * 0.8f) * 0.5f;
                    glm::vec3 ringPos(ringX, ringY + bobY, ringZ);

                    glm::mat4 model = glm::translate(glm::mat4(1.0f), ringPos);
                    model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0, 0, 1));

                    int shapeType = (int)(cityHash(ringSeed, 3) % 5);

                    glm::vec3 ringColors[] = {
                        glm::vec3(1.0f, 0.7f, 0.0f),
                        glm::vec3(0.0f, 0.8f, 1.0f),
                        glm::vec3(1.0f, 0.3f, 0.3f),
                        glm::vec3(0.5f, 1.0f, 0.3f),
                        glm::vec3(0.8f, 0.4f, 1.0f),
                    };
                    bool ringPassed = passedRings.count(ri) > 0;
                    float pulse = 0.7f + 0.3f * sin(time * 4.0f + ri);
                    glm::vec3 ringColor = ringPassed
                        ? glm::vec3(0.35f, 0.35f, 0.38f)
                        : ringColors[shapeType] * pulse;

                    ourShader.setBool("isEmissive", emissiveLightOn);
                    ourShader.setFloat("alpha", 0.85f);
                    if (!emissiveLightOn) ringColor *= 0.18f;

                    switch (shapeType) {
                        case 0: ringCheckpoint.draw(ourShader, model, ringColor); break;
                        case 1: hexRing.draw(ourShader, model, ringColor); break;
                        case 2: triRing.draw(ourShader, model, ringColor); break;
                        case 3: squareRing.draw(ourShader, model, ringColor); break;
                        case 4: pentRing.draw(ourShader, model, ringColor); break;
                    }

                    ourShader.setFloat("alpha", 1.0f);
                    ourShader.setBool("isEmissive", false);

                    glm::vec3 busCenter = busPosition;
                    busCenter.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
                    float distXZ = fabs(busCenter.x - ringPos.x);
                    float distYZ = sqrt((busCenter.y - ringPos.y) * (busCenter.y - ringPos.y) +
                                        (busCenter.z - ringPos.z) * (busCenter.z - ringPos.z));
                    if (!ringPassed && distXZ < 2.0f && distYZ < 6.0f * 0.8f) {
                        passedRings.insert(ri);
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
            glDepthFunc(GL_LEQUAL);
            skyboxShader.use();
            glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
            skyboxShader.setMat4("view", skyboxView);
            skyboxShader.setMat4("projection", projection);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
            skyboxShader.setInt("skybox", 0);
            glBindVertexArray(skyboxVAO);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
            glDepthFunc(GL_LESS);
        }

        // ==================== HUD (score) ====================
        drawHUD(ourShader);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // ==================== CLEANUP ====================
    bus.cleanup();
    sceneSphere.cleanup();
    sceneCone.cleanup();
    bezierVase.cleanup();
    splineLamp.cleanup();
    ruledCanopy.cleanup();
    ringCheckpoint.cleanup();
    hexRing.cleanup();
    triRing.cleanup();
    squareRing.cleanup();
    pentRing.cleanup();
    if (skyboxVAO) { glDeleteVertexArrays(1, &skyboxVAO); glDeleteBuffers(1, &skyboxVBO); }
    unsigned int allTex[] = { texFloor, texCarpet, texFabric, texWall, texDashboard, texBusBody, texSphere, texCone,
                              texStoneWall, texRoofTile, texBrickWall };
    for (auto t : allTex) { if (t) glDeleteTextures(1, &t); }
    if (cubemapTexture) glDeleteTextures(1, &cubemapTexture);
    glfwTerminate();
    return 0;
}
