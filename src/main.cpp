#include <glad/glad.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <vector>

// Windows-specific for getting current directory
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

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
// PROCEDURAL SKYBOX GENERATOR
// ========================================
unsigned int createProceduralCubemap(int resolution = 512) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    // สร้างข้อมูลสีสำหรับแต่ละด้าน
    std::vector<unsigned char> faceData(resolution * resolution * 3);

    // กำหนดสีของแต่ละด้าน (RGB)
    struct FaceColor {
        unsigned char r, g, b;
    };

    FaceColor faceColors[6] = {
        {135, 206, 235},  // Right  - ฟ้าอ่อน (Sky Blue)
        {135, 206, 235},  // Left   - ฟ้าอ่อน
        {120, 180, 255},  // Top    - ฟ้าเข้มขึ้น (เหมือนท้องฟ้าตอนบน)
        {100, 149, 237},  // Bottom - ฟ้าเข้มกว่า (Cornflower Blue)
        {135, 206, 235},  // Front  - ฟ้าอ่อน
        {135, 206, 235}   // Back   - ฟ้าอ่อน
    };

    for (unsigned int face = 0; face < 6; ++face) {
        FaceColor baseColor = faceColors[face];

        // สร้าง gradient สำหรับแต่ละด้าน
        for (int y = 0; y < resolution; ++y) {
            for (int x = 0; x < resolution; ++x) {
                int idx = (y * resolution + x) * 3;

                // Gradient จากบนลงล่าง
                float gradientFactor = 1.0f;
                if (face == 2) { // Top - ไล่จากฟ้าเข้มไปอ่อน
                    gradientFactor = 0.8f + 0.2f * (float)y / resolution;
                }
                else if (face == 3) { // Bottom - เข้มขึ้น
                    gradientFactor = 0.7f + 0.3f * (1.0f - (float)y / resolution);
                }
                else { // ด้านข้าง - gradient เบาๆ
                    gradientFactor = 0.9f + 0.1f * (1.0f - (float)y / resolution);
                }

                faceData[idx + 0] = (unsigned char)(baseColor.r * gradientFactor);
                faceData[idx + 1] = (unsigned char)(baseColor.g * gradientFactor);
                faceData[idx + 2] = (unsigned char)(baseColor.b * gradientFactor);
            }
        }

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB,
            resolution, resolution, 0, GL_RGB, GL_UNSIGNED_BYTE, faceData.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    std::cout << "✓ Procedural cubemap created successfully!\n";
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
    // ========================================
    // === WORKING DIRECTORY DEBUG TEST ===
    // ========================================
    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║   WORKING DIRECTORY DEBUG TEST         ║\n";
    std::cout << "╚════════════════════════════════════════╝\n";

    // Get current working directory
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    std::cout << "Current Directory: " << buffer << "\n";
#else
    char buffer[1024];
    getcwd(buffer, sizeof(buffer));
    std::cout << "Current Directory: " << buffer << "\n";
#endif

    // Test if files exist
    std::cout << "\n--- File Existence Test ---\n";

    std::ifstream testShader1("shaders/model.vert");
    std::ifstream testShader2("shaders/model.frag");
    std::ifstream testShader3("shaders/skybox.vert");
    std::ifstream testShader4("shaders/skybox.frag");
    std::ifstream testModel1("assets/models/Player.fbx");
    std::ifstream testModel2("assets/models/Key.fbx");

    std::cout << "shaders/model.vert:        " << (testShader1.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";
    std::cout << "shaders/model.frag:        " << (testShader2.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";
    std::cout << "shaders/skybox.vert:       " << (testShader3.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";
    std::cout << "shaders/skybox.frag:       " << (testShader4.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";
    std::cout << "assets/models/Player.fbx:  " << (testModel1.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";
    std::cout << "assets/models/Key.fbx:     " << (testModel2.good() ? "✓ FOUND" : "✗ NOT FOUND") << "\n";

    std::cout << "\n";
    std::cout << "╔════════════════════════════════════════╗\n";
    std::cout << "║   END DEBUG TEST - Starting Game...   ║\n";
    std::cout << "╚════════════════════════════════════════╝\n\n";
    // ========================================

    // ---- GLFW Init ----
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT,
        "Maze Runner - Procedural Skybox", nullptr, nullptr);
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
    std::cout << "Loading shaders...\n";
    Shader modelShader("shaders/model.vert", "shaders/model.frag");
    Shader skyShader("shaders/skybox.vert", "shaders/skybox.frag");

    // ---- Load Models with Fallback ----
    std::cout << "\n=== Loading Models ===\n";

    Player player;
    std::cout << "Loading player model...\n";
    player.LoadModel("assets/models/Player.fbx");

    std::cout << "Loading game object models...\n";
    Model keyModel, wallModel, doorModel;

    // Try loading, fallback to cube if fail
    try {
        keyModel = Model("assets/models/Key.fbx");
        if (keyModel.meshes.empty()) {
            std::cout << "  Key model empty, using cube\n";
            keyModel = Model::CreateCube();
        }
        else {
            std::cout << "  ✓ Key model loaded\n";
        }
    }
    catch (...) {
        std::cout << "  Key model failed, using cube\n";
        keyModel = Model::CreateCube();
    }

    try {
        wallModel = Model("assets/models/Wall.fbx");
        if (wallModel.meshes.empty()) {
            std::cout << "  Wall model empty, using cube\n";
            wallModel = Model::CreateCube();
        }
        else {
            std::cout << "  ✓ Wall model loaded\n";
        }
    }
    catch (...) {
        std::cout << "  Wall model failed, using cube\n";
        wallModel = Model::CreateCube();
    }

    try {
        doorModel = Model("assets/models/Door.fbx");
        if (doorModel.meshes.empty()) {
            std::cout << "  Door model empty, using cube\n";
            doorModel = Model::CreateCube();
        }
        else {
            std::cout << "  ✓ Door model loaded\n";
        }
    }
    catch (...) {
        std::cout << "  Door model failed, using cube\n";
        doorModel = Model::CreateCube();
    }

    // Keep any loaded models. They already fall back to cubes if imports fail.
    std::cout << "=== Models ready (imports or cubes as fallback) ===\n\n";

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

    // สร้าง Procedural Cubemap แทนการโหลดจากไฟล์
    std::cout << "Generating procedural skybox...\n";
    unsigned int cubemapTex = createProceduralCubemap(512);

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

    // Compute normalization scales for imported models so they appear at sane sizes
    auto computeUnitScale = [](const Model& m) -> float {
        if (m.meshes.empty()) return 1.0f;
        glm::vec3 bmin, bmax; m.ComputeAABB(bmin, bmax);
        glm::vec3 size = bmax - bmin;
        float longest = std::max(size.x, std::max(size.y, size.z));
        return (longest > 1e-6f) ? (1.0f / longest) : 1.0f;
    };

    float playerImportScale = computeUnitScale(player.model);
    float keyImportScale    = computeUnitScale(keyModel);
    float wallImportScale   = computeUnitScale(wallModel);
    float doorImportScale   = computeUnitScale(doorModel);

    std::cout << "\n=== Game Started! ===\n";
    std::cout << "Controls: WASD to move, ESC to quit\n";
    std::cout << "Objective: Collect 3 keys and reach the exit door!\n\n";

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
            AABB wallBox = Collision::FromPositionSize(
                wall.position, (wall.scale * wallImportScale) * 0.5f);
            if (Collision::TestAABB(playerBox, wallBox)) {
                player.position = prevPos;
                break;
            }
        }

        // Collision: Player vs Keys
        for (int i = 0; i < 3; ++i) {
            if (!keysCollected[i]) {
                AABB keyBox = Collision::FromPositionSize(
                    keyPositions[i], glm::vec3(0.15f) * keyImportScale * 0.5f);
                if (Collision::TestAABB(playerBox, keyBox)) {
                    keysCollected[i] = true;
                    player.keysCollected++;
                    std::cout << "Key " << (i + 1) << " collected! ("
                        << player.keysCollected << "/3)\n";
                }
            }
        }

        // Collision: Player vs Door
        if (player.hasAllKeys && !gameWon) {
            AABB doorBox = Collision::FromPositionSize(
            doorPos, (glm::vec3(1.2f, 2.5f, 0.3f) * doorImportScale) * 0.5f);
            if (Collision::TestAABB(playerBox, doorBox)) {
                gameWon = true;
                std::cout << "\n🎉 YOU WIN! Congratulations! 🎉\n";
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
            m = glm::scale(m, wall.scale * wallImportScale);
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
                m = glm::scale(m, glm::vec3(0.15f) * keyImportScale);
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
        doorM = glm::scale(doorM, glm::vec3(1.2f, 2.5f, 0.3f) * doorImportScale);
        modelShader.setMat4("model", doorM);
        doorModel.Draw();

        // Draw Player as solid color cube
        modelShader.setInt("useObjectColor", 1);
        modelShader.setVec3("objectColor", glm::vec3(0.2f, 0.6f, 1.0f));
        {
            glm::mat4 pm = player.GetModelMatrix();
            pm = glm::scale(pm, glm::vec3(playerImportScale));
            modelShader.setMat4("model", pm);
        }
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
