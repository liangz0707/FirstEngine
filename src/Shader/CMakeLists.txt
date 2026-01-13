cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Shader VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Shader_EXPORTS)
endif()

# Source files
set(SHADER_SOURCES
    ShaderLoader.cpp
)

# Create shared library
add_library(FirstEngine_Shader SHARED
    ${SHADER_SOURCES}
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Export.h
)

# Include directories
target_include_directories(FirstEngine_Shader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link Vulkan (for shader module creation)
target_link_libraries(FirstEngine_Shader
    PUBLIC
        ${Vulkan_LIBRARIES}
)

target_include_directories(FirstEngine_Shader PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Shader PRIVATE /W4)
    target_compile_definitions(FirstEngine_Shader PUBLIC FirstEngine_Shader_EXPORTS)
else()
    target_compile_options(FirstEngine_Shader PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Shader PROPERTIES
    OUTPUT_NAME "FirstEngine_Shader"
    VERSION ${PROJECT_VERSION}
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Shader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Shader>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
