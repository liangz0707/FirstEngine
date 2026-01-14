cmake_minimum_required(VERSION 3.20)

project(FirstEngine_RHI VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_RHI_EXPORTS)
endif()

# Source files (RHI is mostly header-only, but we need a source file for DLL export)
set(RHI_SOURCES
    RHI.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(RHI_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IBuffer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/ICommandBuffer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IDevice.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IFramebuffer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IImage.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IPipeline.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IRenderPass.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/IShaderModule.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/ISwapchain.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/RHI/Types.h
)

# Create shared library
add_library(FirstEngine_RHI SHARED
    ${RHI_SOURCES}
    ${RHI_HEADERS}
)

# Include directories
target_include_directories(FirstEngine_RHI
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_RHI PRIVATE /W4)
    target_compile_definitions(FirstEngine_RHI PUBLIC FirstEngine_RHI_EXPORTS)
else()
    target_compile_options(FirstEngine_RHI PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name and directories
# Enable Windows export all symbols to ensure import library is generated
set_target_properties(FirstEngine_RHI PROPERTIES
    OUTPUT_NAME "FirstEngine_RHI"
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
    add_custom_command(TARGET FirstEngine_RHI POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_RHI>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
