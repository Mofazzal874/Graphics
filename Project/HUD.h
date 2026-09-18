#ifndef HUD_H
#define HUD_H

#include "Globals.h"

// ============================================================================
// HUD - tiny 3x5 bitmap font, drawn with a screen-space cube primitive
// ============================================================================
static inline uint16_t hudGlyph(char c) {
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

inline void drawHUD(Shader& shader) {
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
    float jump = 1.0f + (flashing ? scoreFlashTimer * 1.4f : 0.0f);
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

#endif
