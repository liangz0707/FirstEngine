cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Core VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Core_EXPORTS)
endif()

# Source files
set(CORE_SOURCES
    # Window.cpp - removed, implementation is now in header file (Window.h)
    # This avoids circular dependency: FirstEngine_Device can use Window without linking FirstEngine_Core
    CommandLine.cpp
    Thread.cpp
    ThreadManager.cpp
    Barrier.cpp
    # RenderDoc.cpp - removed, implementation is now in header file (RenderDoc.h)
    # This avoids circular dependency: EditorAPI can use RenderDocHelper without linking FirstEngine_Core
    # EditorAPI.cpp - added directly to avoid circular dependency
    # Instead of linking FirstEngine_Editor static library, we compile EditorAPI.cpp directly into FirstEngine_Core.dll
    ${CMAKE_SOURCE_DIR}/src/Editor/EditorAPI.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(CORE_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Application.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/CommandLine.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Window.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Thread.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/ThreadManager.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Task.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/Barrier.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/MathTypes.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Core/RenderDoc.h
)

# Create shared library
add_library(FirstEngine_Core SHARED
    ${CORE_SOURCES}
    ${CORE_HEADERS}
)

# Include directories
target_include_directories(FirstEngine_Core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link GLFW
if(TARGET glfw)
    target_link_libraries(FirstEngine_Core PUBLIC glfw)
elseif(TARGET glfw3)
    target_link_libraries(FirstEngine_Core PUBLIC glfw3)
elseif(glfw3_FOUND)
    target_link_libraries(FirstEngine_Core PUBLIC ${GLFW3_LIBRARY})
    target_include_directories(FirstEngine_Core PUBLIC ${GLFW3_INCLUDE_DIR})
elseif(glfw_FOUND)
    target_link_libraries(FirstEngine_Core PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(FirstEngine_Core PUBLIC ${GLFW_INCLUDE_DIRS})
endif()

# Add Vulkan include directory (needed for Window.h which is now header-only)
# Window.h includes vulkan/vulkan.h, so Vulkan include path must be PUBLIC
target_include_directories(FirstEngine_Core PUBLIC ${Vulkan_INCLUDE_DIRS})

# Link GLM for MathTypes
target_link_libraries(FirstEngine_Core PUBLIC glm::glm)

# Link EditorAPI dependencies
# Instead of linking FirstEngine_Editor static library (which would create a cycle),
# we compile EditorAPI.cpp directly into FirstEngine_Core.dll (see CORE_SOURCES above)
# This breaks the cycle: Core no longer depends on Editor library, only on Editor's dependencies
# FirstEngine_Device links FirstEngine_Core PRIVATE, so no public cycle
target_link_libraries(FirstEngine_Core
    PRIVATE
        # EditorAPI.cpp is compiled directly into FirstEngine_Core.dll
        # We only need to link EditorAPI's dependencies
        FirstEngine_Device
        FirstEngine_Renderer
        FirstEngine_Resources
        FirstEngine_RHI
        FirstEngine_Shader
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Core PRIVATE /W4)
    target_compile_definitions(FirstEngine_Core PUBLIC FirstEngine_Core_EXPORTS)
else()
    target_compile_options(FirstEngine_Core PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Core PROPERTIES
    OUTPUT_NAME "FirstEngine_Core"
    VERSION ${PROJECT_VERSION}
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Core POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Core>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
