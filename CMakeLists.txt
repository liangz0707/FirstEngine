cmake_minimum_required(VERSION 3.20)
project(FirstEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Find required packages
# Try to find GLFW3 - support multiple methods
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # Try alternative method
    find_package(glfw QUIET)
    if(NOT glfw_FOUND)
        # Use local GLFW from third_party
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw glfw)
    endif()
endif()

# Use local GLM from third_party (header-only library)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm glm)

find_package(Vulkan REQUIRED)

# Add subdirectories for each module
add_subdirectory(src/Shader)
add_subdirectory(src/Resources)
add_subdirectory(src/Device)
add_subdirectory(src/Core)
add_subdirectory(src/Python)
add_subdirectory(src/Application)
add_subdirectory(src/ShaderManager)
