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
        # Use FetchContent as fallback
        include(FetchContent)
        FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
        )
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        FetchContent_MakeAvailable(glfw)
    endif()
endif()

find_package(glm QUIET)
if(NOT glm_FOUND)
    # GLM is header-only, so we can use FetchContent
    include(FetchContent)
    FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
endif()

find_package(Vulkan REQUIRED)

# Add subdirectories for each module
add_subdirectory(src/Shader)
add_subdirectory(src/Resources)
add_subdirectory(src/Renderer)
add_subdirectory(src/Core)
add_subdirectory(src/Application)
