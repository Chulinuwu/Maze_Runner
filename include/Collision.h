#pragma once
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

class Collision {
public:
    static AABB FromPositionSize(const glm::vec3& pos, const glm::vec3& halfSize);
    static bool TestAABB(const AABB& a, const AABB& b);
};
