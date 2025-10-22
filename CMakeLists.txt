cmake_minimum_required(VERSION 3.16)
project(MazeRunner)  # ✅ เปลี่ยนตรงนี้

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# where to put binary
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

# GLFW
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.3.8
)
FetchContent_MakeAvailable(glfw)

# GLAD
FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.35
)
FetchContent_MakeAvailable(glad)

# glm
FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        0.9.9.8
)
FetchContent_MakeAvailable(glm)

# stb (single header)
FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# assimp
FetchContent_Declare(
  assimp
  GIT_REPOSITORY https://github.com/assimp/assimp.git
  GIT_TAG        v5.2.5
)
FetchContent_MakeAvailable(assimp)

# imgui
FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.89.9
)
FetchContent_MakeAvailable(imgui)

# imgui sources
set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# include directories
include_directories(
  ${glad_SOURCE_DIR}/include
  ${glfw_SOURCE_DIR}/include
  ${glm_SOURCE_DIR}
  ${stb_SOURCE_DIR}
  ${assimp_SOURCE_DIR}/include
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${CMAKE_SOURCE_DIR}/include
)

# source files
file(GLOB SRC "src/*.cpp")
add_executable(MazeRunner ${SRC} ${IMGUI_SOURCES})  # ✅ เปลี่ยนตรงนี้

# link libraries
target_link_libraries(MazeRunner PRIVATE glfw glad assimp)  # ✅ เปลี่ยนตรงนี้

if (WIN32)
    target_link_libraries(MazeRunner PRIVATE opengl32)  # ✅ เปลี่ยนตรงนี้
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(MazeRunner PRIVATE OpenGL::GL)  # ✅ เปลี่ยนตรงนี้
endif()

# --- Make the runtime look directly at the source dir (no copy) ---
set_target_properties(MazeRunner PROPERTIES  # ✅ เปลี่ยนตรงนี้
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "Runtime working directory set to: ${CMAKE_SOURCE_DIR}") 