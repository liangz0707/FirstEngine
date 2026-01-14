cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Device VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Device_EXPORTS)
endif()

# Source files
set(DEVICE_SOURCES
    VulkanRenderer.cpp
    VulkanDevice.cpp
    VulkanRHIWrappers.cpp
    Pipeline.cpp
    ShaderModule.cpp
    DeviceContext.cpp
    MemoryManager.cpp
    RenderPass.cpp
    RenderTarget.cpp
    Framebuffer.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(DEVICE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/DeviceContext.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/Framebuffer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/MemoryManager.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/Pipeline.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/RenderPass.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/RenderTarget.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/ShaderModule.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/VulkanDevice.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/VulkanRenderer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Device/VulkanRHIWrappers.h
)

# Create shared library
add_library(FirstEngine_Device SHARED
    ${DEVICE_SOURCES}
    ${DEVICE_HEADERS}
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
        FirstEngine_RHI
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

# Set library output name and directories
# Enable Windows export all symbols to ensure import library is generated
set_target_properties(FirstEngine_Device PROPERTIES
    OUTPUT_NAME "FirstEngine_Device"
    VERSION ${PROJECT_VERSION}
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
    ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug
    ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Device POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Device>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
