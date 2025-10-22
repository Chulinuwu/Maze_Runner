#pragma once
#include <vector>
#include <string>
#include "Mesh.h"
#include <glm/glm.hpp>

class Model {
public:
    std::vector<Mesh> meshes;
    std::string directory;
    Model() {}
    Model(const std::string &path);
    void Draw();
    // helper to create a simple cube if no model found
    static Model CreateCube();
};
