#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <glad/glad.h>
#include "Shader.h"

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class Mesh {
public:
    struct Texture {
        unsigned int id;
        std::string type;
    };

    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures = {});
    void Draw();

private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
};
