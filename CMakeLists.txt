cmake_minimum_required(VERSION 3.16)
project(MazeRunner)

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
add_executable(MazeRunner ${SRC} ${IMGUI_SOURCES})

# link libraries
target_link_libraries(MazeRunner PRIVATE glfw glad assimp)

if (WIN32)
    target_link_libraries(MazeRunner PRIVATE opengl32)
else()
    find_package(OpenGL REQUIRED)
    target_link_libraries(MazeRunner PRIVATE OpenGL::GL)
endif()

# =============================================================================
# CRITICAL: Set working directory to project root (not Build/)
# =============================================================================
set_target_properties(MazeRunner PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# Also set for other IDEs
set_property(TARGET MazeRunner PROPERTY 
    XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Source directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary directory: ${CMAKE_BINARY_DIR}")
message(STATUS "Runtime working directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Executable output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")

# =============================================================================
# OPTIONAL: Copy assets and shaders to build directory (backup method)
# =============================================================================
# Uncomment these if working directory setting doesn't work

# Copy shaders folder
add_custom_command(TARGET MazeRunner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/shaders
    ${CMAKE_BINARY_DIR}/shaders
    COMMENT "Copying shaders to build directory..."
)

# Copy assets folder
add_custom_command(TARGET MazeRunner POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    ${CMAKE_BINARY_DIR}/assets
    COMMENT "Copying assets to build directory..."
)

# =============================================================================
# Create symbolic links (Linux/macOS alternative)
# =============================================================================
if(UNIX)
    add_custom_command(TARGET MazeRunner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/shaders
        ${CMAKE_BINARY_DIR}/shaders
        COMMENT "Creating symbolic link for shaders..."
    )
    
    add_custom_command(TARGET MazeRunner POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets
        COMMENT "Creating symbolic link for assets..."
    )
endif()