// Minimal, model-free OpenGL app: rotating colored cube
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

static int SCR_WIDTH = 1280;
static int SCR_HEIGHT = 720;

static const char* VERT_SRC = R"GLSL(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

out vec3 vColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main() {
    vColor = aColor;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)GLSL";

static const char* FRAG_SRC = R"GLSL(
#version 330 core
in vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)GLSL";

static void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    SCR_WIDTH = width; SCR_HEIGHT = height;
    glViewport(0, 0, width, height);
}

static GLuint makeShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){
        char log[1024]; GLsizei n=0; glGetShaderInfoLog(s, 1024, &n, log);
        std::cerr << "Shader compile error:\n" << log << std::endl;
    }
    return s;
}

static GLuint makeProgram(const char* vs, const char* fs){
    GLuint v = makeShader(GL_VERTEX_SHADER, vs);
    GLuint f = makeShader(GL_FRAGMENT_SHADER, fs);
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    GLint ok=0; glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){
        char log[1024]; GLsizei n=0; glGetProgramInfoLog(p, 1024, &n, log);
        std::cerr << "Program link error:\n" << log << std::endl;
    }
    glDeleteShader(v); glDeleteShader(f);
    return p;
}

int main(){
    if(!glfwInit()){ std::cerr << "GLFW init failed" << std::endl; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* win = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "MazeRunnerBasic", nullptr, nullptr);
    if(!win){ std::cerr << "Failed to create window" << std::endl; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win, framebuffer_size_callback);
    glfwSwapInterval(1); // vsync

    if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        std::cerr << "Failed to init GLAD" << std::endl; return -1;
    }

    GLuint prog = makeProgram(VERT_SRC, FRAG_SRC);

    // Cube vertices: position(x,y,z), color(r,g,b)
    float verts[] = {
        // front (z+)
        -0.5f,-0.5f, 0.5f, 1,0,0,
         0.5f,-0.5f, 0.5f, 0,1,0,
         0.5f, 0.5f, 0.5f, 0,0,1,
        -0.5f, 0.5f, 0.5f, 1,1,0,
        // back (z-)
         0.5f,-0.5f,-0.5f, 1,0,1,
        -0.5f,-0.5f,-0.5f, 0,1,1,
        -0.5f, 0.5f,-0.5f, 1,1,1,
         0.5f, 0.5f,-0.5f, 0.2f,0.2f,0.2f
    };
    unsigned int idx[] = {
        0,1,2, 2,3,0, // front
        4,5,6, 6,7,4, // back
        3,2,7, 7,6,3, // top
        5,4,1, 1,0,5, // bottom
        1,4,7, 7,2,1, // right
        5,0,3, 3,6,5  // left
    };

    GLuint vao,vbo,ebo; glGenVertexArrays(1,&vao); glGenBuffers(1,&vbo); glGenBuffers(1,&ebo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo); glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,6*sizeof(float),(void*)(3*sizeof(float)));
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    float t0 = (float)glfwGetTime();
    while(!glfwWindowShouldClose(win)){
        float t = (float)glfwGetTime();
        float dt = t - t0; t0 = t; (void)dt;

        if(glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(win,true);

        glViewport(0,0,SCR_WIDTH,SCR_HEIGHT);
        glClearColor(0.15f, 0.2f, 0.35f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 model(1.0f);
        model = glm::rotate(model, t*1.3f, glm::vec3(0.3f, 1.0f, 0.2f));
        glm::mat4 view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.5f), glm::vec3(0,0,0), glm::vec3(0,1,0));
        glm::mat4 proj = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH/(float)SCR_HEIGHT, 0.1f, 100.0f);

        glUseProgram(prog);
        glUniformMatrix4fv(glGetUniformLocation(prog,"model"),1,GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(prog,"view"),1,GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(prog,"projection"),1,GL_FALSE, glm::value_ptr(proj));

        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(win);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
