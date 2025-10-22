#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(glm::vec3 position) : Position(position), Front(0.0f, 0.0f, -1.0f), Up(0.0f,1.0f,0.0f), Yaw(-90.0f), Pitch(0.0f) {}

glm::mat4 Camera::GetViewMatrix(){
    return glm::lookAt(Position, Position + Front, Up);
}

void Camera::Follow(const glm::vec3& targetPos, const glm::vec3& targetFront){
    // simple third-person: behind the car and slightly above
    glm::vec3 offset = -glm::normalize(targetFront) * 6.0f + glm::vec3(0.0f, 3.0f, 0.0f);
    Position = targetPos + offset;
    Front = glm::normalize(targetPos - Position + glm::vec3(0.0f,1.0f,0.0f));
}
