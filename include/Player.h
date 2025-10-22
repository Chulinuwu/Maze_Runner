#pragma once
#include <glm/glm.hpp>
#include "Model.h"

class Player {
public:
    glm::vec3 position;
    glm::vec3 front;
    float yaw;
    float speed;
    Model model;
    
    // Maze Runner specific
    int keysCollected;
    bool hasAllKeys;
    
    Player();
    void LoadModel(const std::string& path);
    void Update(float dt, bool w, bool s, bool a, bool d);
    glm::mat4 GetModelMatrix() const;
    
    // Check if can open door
    bool CanOpenDoor() const { return keysCollected >= 3; }
};