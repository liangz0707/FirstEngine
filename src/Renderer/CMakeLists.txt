cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Renderer VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Renderer_EXPORTS)
endif()

# Source files
set(RENDERER_SOURCES
    VulkanRenderer.cpp
    Swapchain.cpp
    Pipeline.cpp
)

# Create shared library
add_library(FirstEngine_Renderer SHARED
    ${RENDERER_SOURCES}
)

# Include directories
target_include_directories(FirstEngine_Renderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(FirstEngine_Renderer
    PUBLIC
        FirstEngine_Core
        FirstEngine_Shader
        FirstEngine_Resources
        ${Vulkan_LIBRARIES}
)

# GLFW include directories (needed for Window.h)
if(TARGET glfw)
    target_link_libraries(FirstEngine_Renderer PUBLIC glfw)
elseif(TARGET glfw3)
    target_link_libraries(FirstEngine_Renderer PUBLIC glfw3)
elseif(glfw3_FOUND)
    target_link_libraries(FirstEngine_Renderer PUBLIC ${GLFW3_LIBRARY})
    target_include_directories(FirstEngine_Renderer PUBLIC ${GLFW3_INCLUDE_DIR})
elseif(glfw_FOUND)
    target_link_libraries(FirstEngine_Renderer PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(FirstEngine_Renderer PUBLIC ${GLFW_INCLUDE_DIRS})
endif()

target_include_directories(FirstEngine_Renderer PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Renderer PRIVATE /W4)
    target_compile_definitions(FirstEngine_Renderer PUBLIC FirstEngine_Renderer_EXPORTS)
else()
    target_compile_options(FirstEngine_Renderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Renderer PROPERTIES
    OUTPUT_NAME "FirstEngine_Renderer"
    VERSION ${PROJECT_VERSION}
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Renderer>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
