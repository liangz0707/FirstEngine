cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Core VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Core_EXPORTS)
endif()

# Source files
set(CORE_SOURCES
    Window.cpp
    CommandLine.cpp
    Thread.cpp
    ThreadManager.cpp
    Barrier.cpp
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
