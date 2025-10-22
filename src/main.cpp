#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/gtc/matrix_transform.hpp>
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

// ---- ส่วนที่ 1: Setup ----
int SCR_WIDTH = 1280;
int SCR_HEIGHT = 720;
bool keys[1024];

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

// ---- ส่วนที่ 2: Maze Layout ----
struct Wall {
    glm::vec3 position;
    glm::vec3 scale;  // กว้าง x หนา x สูง
};

std::vector<Wall> createMaze() {
    std::vector<Wall> walls;
    
    // สร้างกำแพงรอบนอก
    // ฝั่งบน
    walls.push_back({{0, 0, -10}, {20, 1, 0.5f}});
    // ฝั่งล่าง
    walls.push_back({{0, 0, 10}, {20, 1, 0.5f}});
    // ฝั่งซ้าย
    walls.push_back({{-10, 0, 0}, {0.5f, 1, 20}});
    // ฝั่งขวา
    walls.push_back({{10, 0, 0}, {0.5f, 1, 20}});
    
    // กำแพงภายใน (สร้างเขาวงกต)
    walls.push_back({{-5, 0, -5}, {8, 1, 0.5f}});
    walls.push_back({{5, 0, -2}, {0.5f, 1, 10}});
    walls.push_back({{-3, 0, 3}, {10, 1, 0.5f}});
    walls.push_back({{-7, 0, 0}, {0.5f, 1, 8}});
    walls.push_back({{2, 0, 5}, {6, 1, 0.5f}});
    
    return walls;
}

// ---- ส่วนที่ 3: Main Function ----
int main() {
    // GLFW Init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, 
                                          "Maze Runner", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Load Shaders
    Shader modelShader("shaders/model.vert", "shaders/model.frag");
    Shader skyShader("shaders/skybox.vert", "shaders/skybox.frag");

    // Load Models
    Player player;
    player.LoadModel("assets/models/Player.fbx");
    
    Model keyModel("assets/models/Key.fbx");
    Model wallModel("assets/models/Wall.fbx");
    Model doorModel("assets/models/Door.fbx");

    // Setup Camera
    Camera camera(glm::vec3(0, 5, 15));

    // Create Maze
    std::vector<Wall> walls = createMaze();

    // Key Positions
    std::vector<glm::vec3> keyPositions = {
        {-6.0f, 0.3f, -6.0f},
        {7.0f, 0.3f, 3.0f},
        {-3.0f, 0.3f, 7.0f}
    };
    std::vector<bool> keysCollected(3, false);

    // Door Position
    glm::vec3 doorPos = {8.0f, 0.0f, 8.0f};
    bool gameWon = false;

    // Game Loop
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
        bool collided = false;
        for (const auto& wall : walls) {
            AABB wallBox = Collision::FromPositionSize(wall.position, wall.scale);
            if (Collision::TestAABB(playerBox, wallBox)) {
                collided = true;
                break;
            }
        }
        if (collided) player.position = prevPos;

        // Collision: Player vs Keys
        for (int i = 0; i < 3; ++i) {
            if (!keysCollected[i]) {
                AABB keyBox = Collision::FromPositionSize(keyPositions[i], 
                                                         glm::vec3(0.3f));
                if (Collision::TestAABB(playerBox, keyBox)) {
                    keysCollected[i] = true;
                    player.keysCollected++;
                }
            }
        }

        // Collision: Player vs Door (if has all keys)
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

        modelShader.use();
        modelShader.setMat4("projection", projection);
        modelShader.setMat4("view", view);
        modelShader.setVec3("viewPos", camera.Position);
        modelShader.setVec3("lightPos", glm::vec3(0, 10, 0));
        modelShader.setVec3("lightColor", glm::vec3(1.0f));

        // Draw Walls
        modelShader.setInt("useObjectColor", 1);
        modelShader.setVec3("objectColor", glm::vec3(0.5f, 0.5f, 0.5f));
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
                m = glm::scale(m, glm::vec3(0.1f));
                modelShader.setMat4("model", m);
                keyModel.Draw();
            }
        }

        // Draw Door
        glm::vec3 doorColor = player.hasAllKeys ? 
                              glm::vec3(0, 1, 0) : glm::vec3(1, 0, 0);
        modelShader.setVec3("objectColor", doorColor);
        glm::mat4 doorM(1.0f);
        doorM = glm::translate(doorM, doorPos);
        doorM = glm::scale(doorM, glm::vec3(1.0f));
        modelShader.setMat4("model", doorM);
        doorModel.Draw();

        // Draw Player
        modelShader.setInt("useObjectColor", 0);
        modelShader.setMat4("model", player.GetModelMatrix());
        player.model.Draw();

        // ImGui UI
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiCond_Always);
        ImGui::Begin("Game Status", nullptr, 
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
        ImGui::Text("Keys: %d/3", player.keysCollected);
        if (player.hasAllKeys) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Door Unlocked!");
        }
        if (gameWon) {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "YOU WIN!");
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}