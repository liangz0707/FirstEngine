cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Shader VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Shader_EXPORTS)
endif()

# Use local SPIRV-Cross from third_party (SPIR-V → GLSL/HLSL/MSL)
set(SPIRV_CROSS_SKIP_INSTALL ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/spirv-cross spirv_cross)

# Use local glslang from third_party (GLSL/HLSL → SPIR-V)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_WEBMINIFIER OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ALLOW_EXTERNAL_SPIRV_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glslang glslang)

# Source files
set(SHADER_SOURCES
    ShaderLoader.cpp
    ShaderCompiler.cpp
    ShaderSourceCompiler.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(SHADER_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Shader/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Shader/ShaderCompiler.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Shader/ShaderLoader.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Shader/ShaderSourceCompiler.h
)

# Create shared library
add_library(FirstEngine_Shader SHARED
    ${SHADER_SOURCES}
    ${SHADER_HEADERS}
)

# Include directories
target_include_directories(FirstEngine_Shader
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies (no Vulkan dependency - graphics API agnostic)
target_link_libraries(FirstEngine_Shader
    PUBLIC
        spirv-cross-core
        spirv-cross-glsl
        spirv-cross-hlsl
        spirv-cross-msl
        spirv-cross-reflect
        glslang
        SPIRV
    PRIVATE
        spirv-cross-util
)

# No Vulkan includes needed - graphics API agnostic

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
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)

# Organize third-party dependencies in Visual Studio solution
if(TARGET spirv-cross-core)
    set_target_properties(spirv-cross-core PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-glsl)
    set_target_properties(spirv-cross-glsl PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-hlsl)
    set_target_properties(spirv-cross-hlsl PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-msl)
    set_target_properties(spirv-cross-msl PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-reflect)
    set_target_properties(spirv-cross-reflect PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-util)
    set_target_properties(spirv-cross-util PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-cpp)
    set_target_properties(spirv-cross-cpp PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross-c)
    set_target_properties(spirv-cross-c PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()
if(TARGET spirv-cross)
    set_target_properties(spirv-cross PROPERTIES FOLDER "ThirdPart/SPIRV-Cross")
endif()

# Organize glslang projects
if(TARGET glslang)
    set_target_properties(glslang PROPERTIES FOLDER "ThirdPart/glslang")
endif()
if(TARGET SPIRV)
    set_target_properties(SPIRV PROPERTIES FOLDER "ThirdPart/glslang")
endif()
if(TARGET GenericCodeGen)
    set_target_properties(GenericCodeGen PROPERTIES FOLDER "ThirdPart/glslang")
endif()
if(TARGET MachineIndependent)
    set_target_properties(MachineIndependent PROPERTIES FOLDER "ThirdPart/glslang")
endif()
if(TARGET OSDependent)
    set_target_properties(OSDependent PROPERTIES FOLDER "ThirdPart/glslang")
endif()
if(TARGET glslang-default-resource-limits)
    set_target_properties(glslang-default-resource-limits PROPERTIES FOLDER "ThirdPart/glslang")
endif()

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Shader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Shader>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
