#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Shader.h"
#include "Camera.h"
#include "Player.h"
#include "Collision.h"
#include "Model.h"
#include "stb_image.h"

// ========================================
// GLOBAL VARIABLES
// ========================================
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;
bool keys[1024];

// ========================================
// CALLBACKS
// ========================================
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    keys[GLFW_KEY_W] = glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS;
    keys[GLFW_KEY_S] = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;
    keys[GLFW_KEY_A] = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
    keys[GLFW_KEY_D] = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;
}

// ========================================
// SKYBOX LOADER
// ========================================
unsigned int loadCubemap(const std::vector<std::string>& faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); ++i) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = nrChannels == 3 ? GL_RGB : GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format,
                width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else {
            std::cerr << "Cubemap load failed: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

// ========================================
// MAZE STRUCTURE
// ========================================
struct Wall {
    glm::vec3 position;
    glm::vec3 scale;
};

std::vector<Wall> createMaze() {
    std::vector<Wall> walls;

    // กำแพงรอบนอก (20x20 units)
    walls.push_back({ {0, 0, -10}, {20, 2, 0.5f} });   // ฝั่งบน
    walls.push_back({ {0, 0, 10}, {20, 2, 0.5f} });    // ฝั่งล่าง
    walls.push_back({ {-10, 0, 0}, {0.5f, 2, 20} });   // ฝั่งซ้าย
    walls.push_back({ {10, 0, 0}, {0.5f, 2, 20} });    // ฝั่งขวา

    // กำแพงภายใน (สร้างเขาวงกต)
    walls.push_back({ {-5, 0, -5}, {8, 2, 0.5f} });
    walls.push_back({ {5, 0, -2}, {0.5f, 2, 10} });
    walls.push_back({ {-3, 0, 3}, {10, 2, 0.5f} });
    walls.push_back({ {-7, 0, 0}, {0.5f, 2, 8} });
    walls.push_back({ {2, 0, 5}, {6, 2, 0.5f} });
    walls.push_back({ {0, 0, -7}, {6, 2, 0.5f} });
    walls.push_back({ {7, 0, 7}, {0.5f, 2, 5} });

    return walls;
}

// ========================================
// MAIN FUNCTION
// ========================================
int main() {
    // ---- GLFW Init ----
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Maze Runner", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ---- GLAD Init ----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ---- ImGui Init ----
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ---- Load Shaders ----
    Shader modelShader("shaders/model.vert", "shaders/model.frag");
    Shader skyShader("shaders/skybox.vert", "shaders/skybox.frag");

    // ---- Load Models ----
    Player player;
    player.LoadModel("assets/models/Player.fbx");

    Model keyModel("assets/models/Key.fbx");
    Model wallModel("assets/models/Wall.fbx");
    Model doorModel("assets/models/Door.fbx");

    // ---- Setup Skybox ----
    float skyVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };

    unsigned int skyVAO, skyVBO;
    glGenVertexArrays(1, &skyVAO);
    glGenBuffers(1, &skyVBO);
    glBindVertexArray(skyVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyVertices), skyVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces = {
        "assets/cubemap/right.png",
        "assets/cubemap/left.png",
        "assets/cubemap/top.png",
        "assets/cubemap/bottom.png",
        "assets/cubemap/front.png",
        "assets/cubemap/back.png"
    };
    unsigned int cubemapTex = loadCubemap(faces);

    skyShader.use();
    skyShader.setInt("skybox", 0);

    // ---- Setup Game Objects ----
    Camera camera(glm::vec3(0.0f, 8.0f, 15.0f));

    std::vector<Wall> walls = createMaze();

    std::vector<glm::vec3> keyPositions = {
        {-6.0f, 0.3f, -6.0f},
        {7.0f, 0.3f, 3.0f},
        {-3.0f, 0.3f, 7.0f}
    };
    std::vector<bool> keysCollected(3, false);

    glm::vec3 doorPos = { 8.5f, 0.0f, 8.5f };
    bool gameWon = false;

    // ---- Game Loop ----
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = (float)glfwGetTime();
        float dt = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // Update Player
        glm::vec3 prevPos = player.position;
        player.Update(dt, keys[GLFW_KEY_W], keys[GLFW_KEY_S],
            keys[GLFW_KEY_A], keys[GLFW_KEY_D]);

        // Collision: Player vs Walls
        AABB playerBox = Collision::FromPositionSize(player.position,
            glm::vec3(0.4f, 0.8f, 0.4f));
        for (const auto& wall : walls) {
            AABB wallBox = Collision::FromPositionSize(wall.position, wall.scale);
            if (Collision::TestAABB(playerBox, wallBox)) {
                player.position = prevPos;
                break;
            }
        }

        // Collision: Player vs Keys
        for (int i = 0; i < 3; ++i) {
            if (!keysCollected[i]) {
                AABB keyBox = Collision::FromPositionSize(keyPositions[i], glm::vec3(0.3f));
                if (Collision::TestAABB(playerBox, keyBox)) {
                    keysCollected[i] = true;
                    player.keysCollected++;
                }
            }
        }

        // Collision: Player vs Door
        if (player.hasAllKeys && !gameWon) {
            AABB doorBox = Collision::FromPositionSize(doorPos, glm::vec3(1.0f, 2.0f, 0.3f));
            if (Collision::TestAABB(playerBox, doorBox)) {
                gameWon = true;
            }
        }

        // Update Camera
        glm::vec3 camOffset = -player.front * 10.0f + glm::vec3(0, 5, 0);
        camera.Position = player.position + camOffset;
        camera.Front = glm::normalize(player.position - camera.Position);

        // Render
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(45.0f),
            (float)SCR_WIDTH / SCR_HEIGHT,
            0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();

        // Draw Scene
        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setVec3("viewPos", camera.Position);
        modelShader.setVec3("lightPos", glm::vec3(0, 15, 0));
        modelShader.setVec3("lightColor", glm::vec3(1.0f));

        // Draw Walls
        modelShader.setInt("useObjectColor", 1);
        modelShader.setVec3("objectColor", glm::vec3(0.6f, 0.6f, 0.65f));
        for (const auto& wall : walls) {
            glm::mat4 m(1.0f);
            m = glm::translate(m, wall.position);
            m = glm::scale(m, wall.scale);
            modelShader.setMat4("model", m);
            wallModel.Draw();
        }

        // Draw Keys
        modelShader.setVec3("objectColor", glm::vec3(1.0f, 0.84f, 0.0f));
        for (int i = 0; i < 3; ++i) {
            if (!keysCollected[i]) {
                glm::mat4 m(1.0f);
                m = glm::translate(m, keyPositions[i]);
                m = glm::rotate(m, currentFrame * 2.0f, glm::vec3(0, 1, 0));
                m = glm::scale(m, glm::vec3(0.15f));
                modelShader.setMat4("model", m);
                keyModel.Draw();
            }
        }

        // Draw Door
        glm::vec3 doorColor = player.hasAllKeys ?
            glm::vec3(0.2f, 0.8f, 0.2f) : glm::vec3(0.8f, 0.2f, 0.2f);
        modelShader.setVec3("objectColor", doorColor);
        glm::mat4 doorM(1.0f);
        doorM = glm::translate(doorM, doorPos);
        doorM = glm::scale(doorM, glm::vec3(1.2f, 2.5f, 0.3f));
        modelShader.setMat4("model", doorM);
        doorModel.Draw();

        // Draw Player
        modelShader.setInt("useObjectColor", 0);
        modelShader.setMat4("model", player.GetModelMatrix());
        player.model.Draw();

        // Draw Skybox
        glDepthFunc(GL_LEQUAL);
        skyShader.use();
        glm::mat4 skyboxView = glm::mat4(glm::mat3(camera.GetViewMatrix()));
        skyShader.setMat4("view", skyboxView);
        skyShader.setMat4("projection", projection);
        glBindVertexArray(skyVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTex);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        // ImGui UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(SCR_WIDTH - 220, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 120), ImGuiCond_Always);
        ImGui::Begin("Game Status", nullptr,
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse);

        ImGui::Text("=== MAZE RUNNER ===");
        ImGui::Separator();
        ImGui::Text("Keys: %d/3", player.keysCollected);

        if (player.hasAllKeys) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Door UNLOCKED!");
            ImGui::Text("Find the exit!");
        }
        else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Collect all keys");
        }

        if (gameWon) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "*** YOU WIN! ***");
        }

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}