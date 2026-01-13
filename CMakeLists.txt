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
        # Organize GLFW in Visual Studio solution folder
        if(TARGET glfw)
            set_target_properties(glfw PROPERTIES FOLDER "ThirdPart/GLFW")
        endif()
    endif()
endif()

# Use local GLM from third_party (header-only library)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm glm)
# Organize GLM in Visual Studio solution folder
if(TARGET glm)
    set_target_properties(glm PROPERTIES FOLDER "ThirdPart/GLM")
endif()

# Find Vulkan SDK - try local external directory first
set(VULKAN_LOCAL_PATH "${CMAKE_SOURCE_DIR}/external/vulkan")
if(EXISTS "${VULKAN_LOCAL_PATH}")
    message(STATUS "Using local Vulkan SDK: ${VULKAN_LOCAL_PATH}")
    set(VULKAN_SDK "${VULKAN_LOCAL_PATH}" CACHE PATH "Vulkan SDK path")
    set(VULKAN_INCLUDE_DIR "${VULKAN_LOCAL_PATH}/Include")
    set(VULKAN_LIBRARY_DIR "${VULKAN_LOCAL_PATH}/Lib")
    
    # Set Vulkan variables
    set(Vulkan_INCLUDE_DIRS "${VULKAN_INCLUDE_DIR}")
    set(Vulkan_LIBRARIES "${VULKAN_LIBRARY_DIR}/vulkan-1.lib")
    set(Vulkan_FOUND TRUE)
    
    # Add include directory
    include_directories(${Vulkan_INCLUDE_DIRS})
else()
    # Use standard find_package
    find_package(Vulkan REQUIRED)
endif()

# Add subdirectories for each module
# Core modules (FirstEngine自己的模块)
add_subdirectory(src/Shader)
add_subdirectory(src/Resources)
add_subdirectory(src/Device)
add_subdirectory(src/Core)
add_subdirectory(src/Python)
add_subdirectory(src/Application)

# Tools (工具项目)
add_subdirectory(src/Tools/ShaderManager)
