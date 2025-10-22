#include "Player.h"
#include <glm/gtc/matrix_transform.hpp>

Player::Player() 
    : position(0.0f, 0.5f, 0.0f)
    , front(0.0f, 0.0f, -1.0f)
    , yaw(-90.0f)
    , speed(5.0f)  // ช้ากว่า Dog game นิดนึง
    , keysCollected(0)
    , hasAllKeys(false)
{}

void Player::LoadModel(const std::string& path) {
    model = Model(path);
}

void Player::Update(float dt, bool w, bool s, bool a, bool d) {
    // คำนวณทิศทาง
    float rad = glm::radians(yaw);
    front = glm::normalize(glm::vec3(cos(rad), 0.0f, sin(rad)));
    
    glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0, 1, 0)));
    
    // เคลื่อนที่ (ไม่หมุนตอน strafe - ต่างจาก Dog game!)
    if(w) position += front * speed * dt;
    if(s) position -= front * speed * dt;
    if(a) position -= right * speed * dt;
    if(d) position += right * speed * dt;
    
    // Check if collected all keys
    if(keysCollected >= 3) hasAllKeys = true;
}

glm::mat4 Player::GetModelMatrix() const {
    glm::mat4 m(1.0f);
    m = glm::translate(m, position);
    m = glm::rotate(m, glm::radians(-yaw - 90.0f), glm::vec3(0, 1, 0));
    m = glm::scale(m, glm::vec3(0.3f));  // ขนาดต่างจาก Dog
    return m;
}