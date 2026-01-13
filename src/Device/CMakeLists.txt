cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Device VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Device_EXPORTS)
endif()

# Source files
set(DEVICE_SOURCES
    VulkanRenderer.cpp
    Swapchain.cpp
    Pipeline.cpp
    ShaderModule.cpp
)

# Create shared library
add_library(FirstEngine_Device SHARED
    ${DEVICE_SOURCES}
)

# Include directories
target_include_directories(FirstEngine_Device
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(FirstEngine_Device
    PUBLIC
        FirstEngine_Core
        FirstEngine_Shader
        FirstEngine_Resources
        ${Vulkan_LIBRARIES}
)

# GLFW include directories (needed for Window.h)
if(TARGET glfw)
    target_link_libraries(FirstEngine_Device PUBLIC glfw)
elseif(TARGET glfw3)
    target_link_libraries(FirstEngine_Device PUBLIC glfw3)
elseif(glfw3_FOUND)
    target_link_libraries(FirstEngine_Device PUBLIC ${GLFW3_LIBRARY})
    target_include_directories(FirstEngine_Device PUBLIC ${GLFW3_INCLUDE_DIR})
elseif(glfw_FOUND)
    target_link_libraries(FirstEngine_Device PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(FirstEngine_Device PUBLIC ${GLFW_INCLUDE_DIRS})
endif()

target_include_directories(FirstEngine_Device PUBLIC
    ${Vulkan_INCLUDE_DIRS}
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Device PRIVATE /W4)
    target_compile_definitions(FirstEngine_Device PUBLIC FirstEngine_Device_EXPORTS)
else()
    target_compile_options(FirstEngine_Device PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Device PROPERTIES
    OUTPUT_NAME "FirstEngine_Device"
    VERSION ${PROJECT_VERSION}
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Device POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Device>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
