#ifndef COLLISION_H
#define COLLISION_H

#include "Globals.h"

// ============================================================================
// COLLISION DETECTION HELPERS
// ============================================================================
// Bus AABB half-extents (local space): roughly 5x1.5x1.5 (half of 10x3x3)
const float BUS_HALF_X = 5.0f;
const float BUS_HALF_Y = 1.5f;
const float BUS_HALF_Z = 1.5f;

inline AABB getBusAABB() {
    glm::vec3 pos = busPosition;
    pos.y += HOVER_HEIGHT + bus.hoverBobOffset + busAltitude;
    float rad = glm::radians(busYaw);
    float cosA = fabs(cos(rad)), sinA = fabs(sin(rad));
    float extX = BUS_HALF_X * cosA + BUS_HALF_Z * sinA;
    float extZ = BUS_HALF_X * sinA + BUS_HALF_Z * cosA;
    AABB box;
    box.minPt = pos - glm::vec3(extX, BUS_HALF_Y, extZ);
    box.maxPt = pos + glm::vec3(extX, BUS_HALF_Y, extZ);
    return box;
}

inline bool aabbOverlap(const AABB& a, const AABB& b) {
    return (a.minPt.x <= b.maxPt.x && a.maxPt.x >= b.minPt.x) &&
           (a.minPt.y <= b.maxPt.y && a.maxPt.y >= b.minPt.y) &&
           (a.minPt.z <= b.maxPt.z && a.maxPt.z >= b.minPt.z);
}

inline bool checkBusCollision(glm::vec3 newPos) {
    glm::vec3 savedPos = busPosition;
    busPosition = newPos;
    AABB busBox = getBusAABB();
    busPosition = savedPos;

    for (const auto& box : collisionBoxes) {
        if (aabbOverlap(busBox, box)) return true;
    }
    return false;
}

inline void addBuildingCollision(glm::vec3 center, glm::vec3 halfExtents) {
    AABB box;
    box.minPt = center - halfExtents;
    box.maxPt = center + halfExtents;
    collisionBoxes.push_back(box);
}

#endif
