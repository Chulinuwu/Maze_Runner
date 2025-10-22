#pragma once
#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    float Yaw, Pitch;

    Camera(glm::vec3 position);
    glm::mat4 GetViewMatrix();
    void Follow(const glm::vec3& targetPos, const glm::vec3& targetFront);
};
