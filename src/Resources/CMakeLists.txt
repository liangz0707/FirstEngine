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
    MaterialLoader.cpp
    MeshLoader.cpp
    ModelLoader.cpp
    ResourceXMLParser.cpp
    Scene.cpp
    Component.cpp
    SceneLevel.cpp
    TextureResource.cpp
    MeshResource.cpp
    MaterialResource.cpp
    MaterialParameter.cpp
    ModelResource.cpp
    ResourceProvider.cpp
    ModelComponent.cpp
    EffectComponent.cpp
    ResourceDependency.cpp
    ResourceID.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(RESOURCES_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ImageLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/TextureLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MeshLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceXMLParser.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MeshLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceXMLParser.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Scene.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Component.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/SceneLevel.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelComponent.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/LightComponent.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/EffectComponent.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceTypes.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceProvider.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/TextureResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MeshResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceDependency.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceID.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialParameter.h
)

# Create static library (changed from SHARED to allow circular dependency with FirstEngine_Renderer)
# Both libraries need each other, and CMake allows circular dependencies between STATIC libraries
add_library(FirstEngine_Resources STATIC
    ${RESOURCES_SOURCES}
    ${RESOURCES_HEADERS}
)

# Organize files in Visual Studio solution explorer
# Resource classes
source_group("Resource" FILES
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceTypes.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceProvider.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceDependency.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceID.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/TextureResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MeshResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialResource.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialParameter.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelResource.h
    TextureResource.cpp
    MeshResource.cpp
    MaterialResource.cpp
    MaterialParameter.cpp
    ModelResource.cpp
    ResourceProvider.cpp
    ResourceDependency.cpp
    ResourceID.cpp
)

# Loader classes
source_group("Loader" FILES
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ImageLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/TextureLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MaterialLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/MeshLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ResourceXMLParser.h
    ImageLoader.cpp
    TextureLoader.cpp
    MaterialLoader.cpp
    MeshLoader.cpp
    ModelLoader.cpp
    ResourceXMLParser.cpp
)

# Scene management
source_group("Scene" FILES
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Scene.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/SceneLevel.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Component.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/ModelComponent.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/LightComponent.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/EffectComponent.h
    Scene.cpp
    SceneLevel.cpp
    Component.cpp
    ModelComponent.cpp
    EffectComponent.cpp
)

# Common headers
source_group("Common" FILES
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Resources/Export.h
)

# Include directories
target_include_directories(FirstEngine_Resources
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Include directories for stb (header-only) and pugixml
target_include_directories(FirstEngine_Resources
    PRIVATE
        ${CMAKE_SOURCE_DIR}/third_party
        ${CMAKE_SOURCE_DIR}/third_party/assimp/contrib/pugixml/src
)

# Link dependencies
target_link_libraries(FirstEngine_Resources
    PUBLIC
        ${Vulkan_LIBRARIES}
        assimp::assimp
    PRIVATE
        FirstEngine_Renderer  # Component needs RenderItem, RenderFlags, and ShadingMaterial (implementation only)
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
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)

# Organize assimp projects in Visual Studio solution
if(TARGET assimp)
    set_target_properties(assimp PROPERTIES FOLDER "ThirdPart/Assimp")
endif()
if(TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "ThirdPart/Assimp")
endif()
if(TARGET UpdateAssimpLibsDebugSymbolsAndDLLs)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "ThirdPart/Assimp")
endif()

# No need to copy DLL for static library
# Static libraries are linked directly into the executable
