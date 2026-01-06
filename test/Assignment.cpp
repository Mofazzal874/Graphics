#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>
#include <vector>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float PI = 3.14159265359f;

// Global state for disassembly view
bool isDisassembled = false;
bool tKeyPressed = false;

const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 ourColor;
void main()
{
    gl_Position = vec4(aPos, 1.0);
    ourColor = aColor;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;
in vec3 ourColor;
void main()
{
    FragColor = vec4(ourColor, 1.0);
}
)";

// Helper function to add a triangle with offset
void addTriangle(std::vector<float>& v, 
                 float x1, float y1, float x2, float y2, float x3, float y3,
                 float r, float g, float b,
                 float offsetX = 0.0f, float offsetY = 0.0f) {
    v.push_back(x1 + offsetX); v.push_back(y1 + offsetY); v.push_back(0.0f);
    v.push_back(r); v.push_back(g); v.push_back(b);
    v.push_back(x2 + offsetX); v.push_back(y2 + offsetY); v.push_back(0.0f);
    v.push_back(r); v.push_back(g); v.push_back(b);
    v.push_back(x3 + offsetX); v.push_back(y3 + offsetY); v.push_back(0.0f);
    v.push_back(r); v.push_back(g); v.push_back(b);
}

void addQuad(std::vector<float>& v, float x1, float y1, float x2, float y2, 
             float x3, float y3, float x4, float y4, float r, float g, float b,
             float offsetX = 0.0f, float offsetY = 0.0f) {
    addTriangle(v, x1, y1, x2, y2, x3, y3, r, g, b, offsetX, offsetY);
    addTriangle(v, x1, y1, x3, y3, x4, y4, r, g, b, offsetX, offsetY);
}

void addEllipse(std::vector<float>& v, float cx, float cy, 
                float rx, float ry, int seg, float r, float g, float b,
                float offsetX = 0.0f, float offsetY = 0.0f) {
    for (int i = 0; i < seg; i++) {
        float a1 = 2.0f * PI * i / seg;
        float a2 = 2.0f * PI * (i + 1) / seg;
        addTriangle(v, cx, cy,
                    cx + rx * cos(a1), cy + ry * sin(a1),
                    cx + rx * cos(a2), cy + ry * sin(a2), r, g, b, offsetX, offsetY);
    }
}

// Pointed nose shape
void addNose(std::vector<float>& v, float cx, float baseY, float width, float height, 
             int seg, float r, float g, float b, float offsetX = 0.0f, float offsetY = 0.0f) {
    for (int i = 0; i < seg; i++) {
        float t1 = (float)i / seg;
        float t2 = (float)(i + 1) / seg;
        float y1 = baseY + height * t1;
        float y2 = baseY + height * t2;
        float w1 = width * (1.0f - t1 * t1);
        float w2 = width * (1.0f - t2 * t2);
        addTriangle(v, cx - w1, y1, cx + w1, y1, cx + w2, y2, r, g, b, offsetX, offsetY);
        addTriangle(v, cx - w1, y1, cx + w2, y2, cx - w2, y2, r, g, b, offsetX, offsetY);
    }
}

// Pointed tail shape
void addTail(std::vector<float>& v, float cx, float baseY, float width, float height,
             int seg, float r, float g, float b, float offsetX = 0.0f, float offsetY = 0.0f) {
    for (int i = 0; i < seg; i++) {
        float t1 = (float)i / seg;
        float t2 = (float)(i + 1) / seg;
        float y1 = baseY - height * t1;
        float y2 = baseY - height * t2;
        float w1 = width * (1.0f - t1 * t1);
        float w2 = width * (1.0f - t2 * t2);
        addTriangle(v, cx - w1, y1, cx + w1, y1, cx + w2, y2, r, g, b, offsetX, offsetY);
        addTriangle(v, cx - w1, y1, cx + w2, y2, cx - w2, y2, r, g, b, offsetX, offsetY);
    }
}

void buildAeroplane(std::vector<float>& v, bool exploded) {
    // Colors
    float bodyR = 0.88f, bodyG = 0.90f, bodyB = 0.92f;
    float shadowR = 0.70f, shadowG = 0.72f, shadowB = 0.76f;
    float wingR = 0.78f, wingG = 0.80f, wingB = 0.84f;
    float cockpitR = 0.28f, cockpitG = 0.33f, cockpitB = 0.42f;
    float engineR = 0.48f, engineG = 0.51f, engineB = 0.56f;
    float stabilizerR = 0.82f, stabilizerG = 0.84f, stabilizerB = 0.87f;
    
    // Fuselage shading colors
    float highlightR = 0.96f, highlightG = 0.97f, highlightB = 0.98f;
    float midR = 0.90f, midG = 0.91f, midB = 0.93f;
    float edgeR = 0.80f, edgeG = 0.82f, edgeB = 0.85f;
    
    float fuselageW = 0.07f;
    float fuselageTop = 0.42f;
    float fuselageBot = -0.52f;
    float noseHeight = 0.25f;
    float tailHeight = 0.20f;
    
    float sOff = 0.018f;
    
    // Component offsets for exploded view
    float leftWingOffX = exploded ? -0.35f : 0.0f;
    float leftWingOffY = exploded ? 0.15f : 0.0f;
    float rightWingOffX = exploded ? 0.35f : 0.0f;
    float rightWingOffY = exploded ? 0.15f : 0.0f;
    float leftTailOffX = exploded ? -0.25f : 0.0f;
    float leftTailOffY = exploded ? -0.20f : 0.0f;
    float rightTailOffX = exploded ? 0.25f : 0.0f;
    float rightTailOffY = exploded ? -0.20f : 0.0f;
    float fuselageOffX = exploded ? 0.0f : 0.0f;
    float fuselageOffY = exploded ? 0.0f : 0.0f;
    float leftEngineOffX = exploded ? -0.30f : 0.0f;
    float leftEngineOffY = exploded ? 0.35f : 0.0f;
    float rightEngineOffX = exploded ? 0.30f : 0.0f;
    float rightEngineOffY = exploded ? 0.35f : 0.0f;
    float vertStabOffX = exploded ? 0.0f : 0.0f;
    float vertStabOffY = exploded ? -0.25f : 0.0f;
    float cockpitOffX = exploded ? 0.0f : 0.0f;
    float cockpitOffY = exploded ? 0.30f : 0.0f;
    
    // Skip shadows in exploded view for clarity
    if (!exploded) {
        // =====================================================================
        // LAYER 1: DROP SHADOWS
        // =====================================================================
        
        // Fuselage shadow
        addQuad(v, -fuselageW+sOff, fuselageTop-sOff, fuselageW+sOff, fuselageTop-sOff,
                fuselageW+sOff, fuselageBot-sOff, -fuselageW+sOff, fuselageBot-sOff, shadowR, shadowG, shadowB);
        addNose(v, sOff, fuselageTop-sOff, fuselageW, noseHeight, 16, shadowR, shadowG, shadowB);
        addTail(v, sOff, fuselageBot-sOff, fuselageW*0.7f, tailHeight, 12, shadowR, shadowG, shadowB);
        
        // Wing shadows - trapezoid shape
        addTriangle(v, -0.06f-sOff, 0.15f-sOff, -0.06f-sOff, -0.02f-sOff, -0.50f-sOff, -0.08f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, -0.06f-sOff, -0.02f-sOff, -0.50f-sOff, -0.12f-sOff, -0.50f-sOff, -0.08f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, 0.06f+sOff, 0.15f-sOff, 0.06f+sOff, -0.02f-sOff, 0.50f+sOff, -0.08f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, 0.06f+sOff, -0.02f-sOff, 0.50f+sOff, -0.12f-sOff, 0.50f+sOff, -0.08f-sOff, shadowR, shadowG, shadowB);
        
        // Tail wing shadows
        addTriangle(v, -0.04f-sOff, -0.50f-sOff, -0.04f-sOff, -0.56f-sOff, -0.18f-sOff, -0.56f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, -0.04f-sOff, -0.56f-sOff, -0.18f-sOff, -0.58f-sOff, -0.18f-sOff, -0.56f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, 0.04f+sOff, -0.50f-sOff, 0.04f+sOff, -0.56f-sOff, 0.18f+sOff, -0.56f-sOff, shadowR, shadowG, shadowB);
        addTriangle(v, 0.04f+sOff, -0.56f-sOff, 0.18f+sOff, -0.58f-sOff, 0.18f+sOff, -0.56f-sOff, shadowR, shadowG, shadowB);
    }
    
    // =====================================================================
    // LAYER 2: ENGINE NACELLES
    // =====================================================================
    
    float engineY = 0.12f;
    
    // Left engine
    addQuad(v, -0.15f, engineY+0.06f, -0.21f, engineY+0.06f, 
            -0.21f, engineY-0.06f, -0.15f, engineY-0.06f, edgeR, edgeG, edgeB, leftEngineOffX, leftEngineOffY);
    addTriangle(v, -0.15f, engineY-0.06f, -0.21f, engineY-0.06f, -0.18f, engineY-0.10f, edgeR, edgeG, edgeB, leftEngineOffX, leftEngineOffY);
    addEllipse(v, -0.18f, engineY+0.06f, 0.028f, 0.02f, 16, engineR, engineG, engineB, leftEngineOffX, leftEngineOffY);
    
    // Right engine
    addQuad(v, 0.15f, engineY+0.06f, 0.21f, engineY+0.06f,
            0.21f, engineY-0.06f, 0.15f, engineY-0.06f, edgeR, edgeG, edgeB, rightEngineOffX, rightEngineOffY);
    addTriangle(v, 0.15f, engineY-0.06f, 0.21f, engineY-0.06f, 0.18f, engineY-0.10f, edgeR, edgeG, edgeB, rightEngineOffX, rightEngineOffY);
    addEllipse(v, 0.18f, engineY+0.06f, 0.028f, 0.02f, 16, engineR, engineG, engineB, rightEngineOffX, rightEngineOffY);
    
    // =====================================================================
    // LAYER 3: MAIN WINGS
    // =====================================================================
    
    float wingRootFront = 0.18f;
    float wingRootBack = -0.02f;
    float wingTipFront = -0.06f;
    float wingTipBack = -0.10f;
    float wingRootX = 0.06f;
    float wingTipX = 0.52f;
    
    // Left wing
    addTriangle(v, -wingRootX, wingRootFront, -wingRootX, wingRootBack, -wingTipX, wingTipFront, wingR, wingG, wingB, leftWingOffX, leftWingOffY);
    addTriangle(v, -wingRootX, wingRootBack, -wingTipX, wingTipBack, -wingTipX, wingTipFront, wingR, wingG, wingB, leftWingOffX, leftWingOffY);
    
    // Right wing
    addTriangle(v, wingRootX, wingRootFront, wingRootX, wingRootBack, wingTipX, wingTipFront, wingR, wingG, wingB, rightWingOffX, rightWingOffY);
    addTriangle(v, wingRootX, wingRootBack, wingTipX, wingTipBack, wingTipX, wingTipFront, wingR, wingG, wingB, rightWingOffX, rightWingOffY);
    
    // =====================================================================
    // LAYER 4: HORIZONTAL TAIL WINGS
    // =====================================================================
    
    float tailRootX = 0.04f;
    float tailTipX = 0.18f;
    float tailRootFront = -0.50f;
    float tailRootBack = -0.56f;
    float tailTipFront = -0.56f;
    float tailTipBack = -0.58f;
    
    // Left tail wing
    addTriangle(v, -tailRootX, tailRootFront, -tailRootX, tailRootBack, -tailTipX, tailTipFront, wingR, wingG, wingB, leftTailOffX, leftTailOffY);
    addTriangle(v, -tailRootX, tailRootBack, -tailTipX, tailTipBack, -tailTipX, tailTipFront, wingR, wingG, wingB, leftTailOffX, leftTailOffY);
    
    // Right tail wing
    addTriangle(v, tailRootX, tailRootFront, tailRootX, tailRootBack, tailTipX, tailTipFront, wingR, wingG, wingB, rightTailOffX, rightTailOffY);
    addTriangle(v, tailRootX, tailRootBack, tailTipX, tailTipBack, tailTipX, tailTipFront, wingR, wingG, wingB, rightTailOffX, rightTailOffY);
    
    // =====================================================================
    // LAYER 5: FUSELAGE
    // =====================================================================
    
    float stripW = fuselageW / 4.0f;
    
    // Left edge (darkest)
    addQuad(v, -fuselageW, fuselageTop, -fuselageW+stripW, fuselageTop,
            -fuselageW+stripW, fuselageBot, -fuselageW, fuselageBot, edgeR, edgeG, edgeB, fuselageOffX, fuselageOffY);
    // Left-mid
    addQuad(v, -fuselageW+stripW, fuselageTop, -fuselageW+2*stripW, fuselageTop,
            -fuselageW+2*stripW, fuselageBot, -fuselageW+stripW, fuselageBot, midR, midG, midB, fuselageOffX, fuselageOffY);
    // Center-left
    addQuad(v, -fuselageW+2*stripW, fuselageTop, 0.0f, fuselageTop,
            0.0f, fuselageBot, -fuselageW+2*stripW, fuselageBot, highlightR, highlightG, highlightB, fuselageOffX, fuselageOffY);
    // Center-right
    addQuad(v, 0.0f, fuselageTop, fuselageW-2*stripW, fuselageTop,
            fuselageW-2*stripW, fuselageBot, 0.0f, fuselageBot, highlightR, highlightG, highlightB, fuselageOffX, fuselageOffY);
    // Right-mid
    addQuad(v, fuselageW-2*stripW, fuselageTop, fuselageW-stripW, fuselageTop,
            fuselageW-stripW, fuselageBot, fuselageW-2*stripW, fuselageBot, midR, midG, midB, fuselageOffX, fuselageOffY);
    // Right edge (darkest)
    addQuad(v, fuselageW-stripW, fuselageTop, fuselageW, fuselageTop,
            fuselageW, fuselageBot, fuselageW-stripW, fuselageBot, edgeR, edgeG, edgeB, fuselageOffX, fuselageOffY);
    
    // Pointed nose
    addNose(v, 0.0f, fuselageTop, fuselageW, noseHeight, 20, bodyR, bodyG, bodyB, fuselageOffX, fuselageOffY);
    // Pointed tail
    addTail(v, 0.0f, fuselageBot, fuselageW*0.7f, tailHeight, 16, bodyR, bodyG, bodyB, fuselageOffX, fuselageOffY);
    
    // =====================================================================
    // LAYER 6: COCKPIT WINDOWS
    // =====================================================================
    
    float cockpitCenterX = 0.0f;
    float cockpitCenterY = 0.48f;
    float cockpitRadius = 0.055f;
    int cockpitSegments = 16;
    
    for (int i = 0; i < cockpitSegments; i++) {
        float angle1 = PI + PI * i / cockpitSegments;
        float angle2 = PI + PI * (i + 1) / cockpitSegments;
        
        float x1 = cockpitCenterX + cockpitRadius * cos(angle1);
        float y1 = cockpitCenterY + cockpitRadius * sin(angle1);
        float x2 = cockpitCenterX + cockpitRadius * cos(angle2);
        float y2 = cockpitCenterY + cockpitRadius * sin(angle2);
        
        addTriangle(v, cockpitCenterX, cockpitCenterY, x1, y1, x2, y2, cockpitR, cockpitG, cockpitB, cockpitOffX, cockpitOffY);
    }
    
    addTriangle(v, 0.0f, 0.56f, -0.055f, 0.48f, 0.055f, 0.48f, cockpitR, cockpitG, cockpitB, cockpitOffX, cockpitOffY);
    
    // =====================================================================
    // LAYER 7: VERTICAL STABILIZER
    // =====================================================================
    
    float stabW = 0.014f;
    float stabTop = -0.50f;
    float stabBot = -0.68f;
    float stabRad = stabW;
    
    // Main rectangular body
    addQuad(v, -stabW, stabTop-stabRad, stabW, stabTop-stabRad, 
            stabW, stabBot+stabRad, -stabW, stabBot+stabRad, stabilizerR, stabilizerG, stabilizerB, vertStabOffX, vertStabOffY);
    
    // Top rounded cap
    for (int i = 0; i < 12; i++) {
        float a1 = PI * i / 12.0f;
        float a2 = PI * (i + 1) / 12.0f;
        addTriangle(v, 0.0f, stabTop-stabRad,
                    stabW * cos(a1), stabTop-stabRad + stabRad * sin(a1),
                    stabW * cos(a2), stabTop-stabRad + stabRad * sin(a2),
                    stabilizerR, stabilizerG, stabilizerB, vertStabOffX, vertStabOffY);
    }
    
    // Bottom rounded cap
    for (int i = 0; i < 12; i++) {
        float a1 = PI + PI * i / 12.0f;
        float a2 = PI + PI * (i + 1) / 12.0f;
        addTriangle(v, 0.0f, stabBot+stabRad,
                    stabW * cos(a1), stabBot+stabRad + stabRad * sin(a1),
                    stabW * cos(a2), stabBot+stabRad + stabRad * sin(a2),
                    stabilizerR, stabilizerG, stabilizerB, vertStabOffX, vertStabOffY);
    }
    
    // Shadow on stabilizer
    addQuad(v, 0.003f, stabTop-stabRad+0.02f, stabW, stabTop-stabRad+0.02f,
            stabW, stabBot+stabRad-0.02f, 0.003f, stabBot+stabRad-0.02f, 
            stabilizerR-0.05f, stabilizerG-0.05f, stabilizerB-0.05f, vertStabOffX, vertStabOffY);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CSE 4208: Aeroplane - Press T to Toggle Disassembly", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "Vertex shader error: " << infoLog << std::endl;
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "Fragment shader error: " << infoLog << std::endl;
    }
    
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // Initial build
    std::vector<float> vertices;
    buildAeroplane(vertices, isDisassembled);
    
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    std::cout << "Press T to toggle disassembly view" << std::endl;
    std::cout << "Press ESC to exit" << std::endl;

    while (!glfwWindowShouldClose(window)) {
        // Process input
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);
        
        // Toggle disassembly with T key
        if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS) {
            if (!tKeyPressed) {
                tKeyPressed = true;
                isDisassembled = !isDisassembled;
                
                // Rebuild vertices with new state
                vertices.clear();
                buildAeroplane(vertices, isDisassembled);
                
                // Update buffer
                glBindBuffer(GL_ARRAY_BUFFER, VBO);
                glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
                
                std::cout << (isDisassembled ? "Disassembly view ON" : "Assembled view ON") << std::endl;
            }
        } else {
            tKeyPressed = false;
        }
        
        // Render
        glClearColor(0.98f, 0.98f, 0.99f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}
