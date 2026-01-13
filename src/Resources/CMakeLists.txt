cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Resources VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Resources_EXPORTS)
endif()

# Use local stb from third_party (header-only library)
# stb is header-only, we just need to include it

# Use local assimp from third_party
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/assimp assimp)

# Source files
set(RESOURCES_SOURCES
    TextureLoader.cpp
    ImageLoader.cpp
    ModelLoader.cpp
)

# Create shared library
add_library(FirstEngine_Resources SHARED
    ${RESOURCES_SOURCES}
)

# Include directories
target_include_directories(FirstEngine_Resources
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Include directories for stb (header-only)
target_include_directories(FirstEngine_Resources
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
)

# Link dependencies
target_link_libraries(FirstEngine_Resources
    PUBLIC
        ${Vulkan_LIBRARIES}
        assimp::assimp
    PRIVATE
        glm::glm
)

target_include_directories(FirstEngine_Resources PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Resources PRIVATE /W4)
    target_compile_definitions(FirstEngine_Resources PUBLIC FirstEngine_Resources_EXPORTS)
else()
    target_compile_options(FirstEngine_Resources PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Resources PROPERTIES
    OUTPUT_NAME "FirstEngine_Resources"
    VERSION ${PROJECT_VERSION}
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Resources POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Resources>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
