#ifndef CAMERA_H
#define CAMERA_H

#include "Globals.h"

// ============================================================================
// CUSTOM lookAt
// ============================================================================
inline glm::mat4 myLookAt(glm::vec3 eye, glm::vec3 center, glm::vec3 up) {
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

// ============================================================================
// CAMERA HELPERS
// ============================================================================
inline glm::vec3 getCameraFront() {
    glm::vec3 front;
    front.x = cos(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    front.y = sin(glm::radians(cameraPitch));
    front.z = sin(glm::radians(cameraYaw)) * cos(glm::radians(cameraPitch));
    return glm::normalize(front);
}

inline glm::vec3 getCameraRight() {
    return glm::normalize(glm::cross(getCameraFront(), glm::vec3(0.0f, 1.0f, 0.0f)));
}

inline glm::vec3 getCameraUp() {
    return glm::normalize(glm::cross(getCameraRight(), getCameraFront()));
}

// Get bus forward direction from yaw
inline glm::vec3 getBusForward() {
    float rad = glm::radians(busYaw);
    return glm::vec3(-cos(rad), 0.0f, sin(rad));
}

inline glm::vec3 getBusRight() {
    float rad = glm::radians(busYaw - 90.0f);
    return glm::vec3(-cos(rad), 0.0f, sin(rad));
}

inline glm::mat4 getViewMatrix() {
    glm::vec3 busRenderPos = busPosition;
    busRenderPos.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;

    if (cameraMode == 1) {
        // === CHASE CAMERA (3rd person behind bus, jet engine visible) ===
        glm::vec3 forward = getBusForward();
        glm::vec3 chaseOffset = -forward * 18.0f + glm::vec3(0.0f, 5.0f, 0.0f);
        cameraPos = busRenderPos + chaseOffset;
        glm::vec3 lookTarget = busRenderPos + glm::vec3(0.0f, 1.5f, 0.0f) - forward * 2.0f;
        return myLookAt(cameraPos, lookTarget, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    if (cameraMode == 2) {
        // === INTERIOR CAMERA (1st person, driver seat) ===
        glm::vec3 forward = getBusForward();
        glm::vec3 right = getBusRight();
        cameraPos = busRenderPos + forward * (-3.0f) + glm::vec3(0.0f, 1.0f, 0.0f) + right * (-0.6f);
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

#endif
