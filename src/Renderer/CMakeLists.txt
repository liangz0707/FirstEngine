cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Renderer VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Renderer_EXPORTS)
endif()

# Source files
set(RENDERER_SOURCES
    FrameGraph.cpp
    FrameGraphExecutionPlan.cpp
    PipelineConfig.cpp
    PipelineBuilder.cpp
    IRenderPipeline.cpp
    DeferredRenderPipeline.cpp
    RenderBatch.cpp
    RenderCommandList.cpp
    CommandRecorder.cpp
    RenderConfig.cpp
    SceneRenderer.cpp
    RenderResource.cpp
    RenderPassTypes.cpp
    IRenderPass.cpp
    GeometryPass.cpp
    LightingPass.cpp
    PostProcessPass.cpp
    ResourceDescription.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(RENDERER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/FrameGraph.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/FrameGraphExecutionPlan.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/PipelineBuilder.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/PipelineConfig.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/IRenderPipeline.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/DeferredRenderPipeline.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/RenderBatch.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/RenderCommandList.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/CommandRecorder.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/RenderConfig.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/SceneRenderer.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/RenderPassTypes.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/ResourceTypes.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/IRenderPass.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/GeometryPass.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/LightingPass.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Renderer/PostProcessPass.h
)

# Create shared library
add_library(FirstEngine_Renderer SHARED
    ${RENDERER_SOURCES}
    ${RENDERER_HEADERS}
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
        FirstEngine_RHI
        FirstEngine_Resources
    PRIVATE
        glm::glm
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
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Renderer POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Renderer>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
