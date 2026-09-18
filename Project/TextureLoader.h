#ifndef TEXTURELOADER_H
#define TEXTURELOADER_H

#include "Globals.h"
#include "stb_image.h"
#include <fstream>

// ============================================================================
// TEXTURE LOADING
// ============================================================================
const int MAX_TEXTURE_DIM = 2048;

inline unsigned int loadTexture(const char* path, GLenum wrapMode, GLenum filterMode) {
    std::cout << "  Loading: " << path << "..." << std::flush;

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

    int infoOk = stbi_info(path, &width, &height, &nrChannels);
    if (!infoOk || width <= 0 || height <= 0) {
        std::cout << " [SKIP] invalid/corrupt image" << std::endl;
        std::cout.flush();
        return 0;
    }
    std::cout << " " << width << "x" << height << "..." << std::flush;

    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    if (!data) {
        std::cout << " [FAIL] " << stbi_failure_reason() << std::endl;
        std::cout.flush();
        return 0;
    }

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
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
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
inline unsigned int loadTextureRGBA(const char* path, GLenum wrapMode, GLenum filterMode) {
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
inline unsigned int loadCubemapFromCross(const char* path) {
    std::cout << "  Loading cubemap cross: " << path << "..." << std::flush;

    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 3);
    if (!data) {
        std::cout << " [FAIL] " << stbi_failure_reason() << std::endl;
        return 0;
    }
    std::cout << " " << width << "x" << height << "..." << std::flush;

    int faceW = width / 4;
    int faceH = height / 3;
    int faceSize = (faceW < faceH) ? faceW : faceH;

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

    unsigned char* faceData = (unsigned char*)malloc(faceSize * faceSize * 3);

    for (int i = 0; i < 6; i++) {
        int srcX = faces[i].col * faceW;
        int srcY = faces[i].row * faceH;

        for (int y = 0; y < faceSize; y++) {
            for (int x = 0; x < faceSize; x++) {
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
    stbi_set_flip_vertically_on_load(true);
    return textureID;
}

// ============================================================================
// CUBEMAP LOADING FROM 6 INDIVIDUAL FACE IMAGES
// ============================================================================
inline unsigned int loadCubemapFromFaces() {
    std::cout << "  Loading cubemap from individual faces..." << std::flush;

    const char* facePaths[6] = {
        "textures/skybox/right.jpg",
        "textures/skybox/left.jpg",
        "textures/skybox/top.jpg",
        "textures/skybox/bottom.jpg",
        "textures/skybox/front.jpg",
        "textures/skybox/back.jpg"
    };

    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    stbi_set_flip_vertically_on_load(false);

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
    stbi_set_flip_vertically_on_load(true);
    return textureID;
}

inline void updateSceneTextureParams() {
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

inline void printStatus() {
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

#endif
