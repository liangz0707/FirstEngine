cmake_minimum_required(VERSION 3.20)
project(FirstEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Set debug postfix for Windows (Debug configuration will append 'd' to library names)
if(WIN32)
    set(CMAKE_DEBUG_POSTFIX "d")
endif()

# Ensure proper debug flags for MSVC
if(MSVC)
    # Set debug compilation flags
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Zi /Od /D_DEBUG")
    # Set debug linker flags
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /DEBUG")
    set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} /DEBUG")
    set(CMAKE_STATIC_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} /DEBUG")
endif()

# Find required packages
# Try to find GLFW3 - support multiple methods
find_package(glfw3 QUIET)
if(NOT glfw3_FOUND)
    # Try alternative method
    find_package(glfw QUIET)
    if(NOT glfw_FOUND)
        # Use local GLFW from third_party
        set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
        add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glfw glfw)
        # Organize GLFW in Visual Studio solution folder
        if(TARGET glfw)
            set_target_properties(glfw PROPERTIES FOLDER "ThirdPart/GLFW")
        endif()
    endif()
endif()

# Use local GLM from third_party (header-only library)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/glm glm)
# Organize GLM in Visual Studio solution folder
if(TARGET glm)
    set_target_properties(glm PROPERTIES FOLDER "ThirdPart/GLM")
endif()

# Find Vulkan SDK - try local external directory first
set(VULKAN_LOCAL_PATH "${CMAKE_SOURCE_DIR}/external/vulkan")
if(EXISTS "${VULKAN_LOCAL_PATH}")
    message(STATUS "Using local Vulkan SDK: ${VULKAN_LOCAL_PATH}")
    set(VULKAN_SDK "${VULKAN_LOCAL_PATH}" CACHE PATH "Vulkan SDK path")
    set(VULKAN_INCLUDE_DIR "${VULKAN_LOCAL_PATH}/Include")
    set(VULKAN_LIBRARY_DIR "${VULKAN_LOCAL_PATH}/Lib")
    set(VULKAN_BIN_DIR "${VULKAN_LOCAL_PATH}/Bin")
    
    # Set Vulkan variables
    set(Vulkan_INCLUDE_DIRS "${VULKAN_INCLUDE_DIR}")
    set(Vulkan_LIBRARIES "${VULKAN_LIBRARY_DIR}/vulkan-1.lib")
    set(Vulkan_FOUND TRUE)
    
    # Check for vulkan-1.dll (new SDK versions don't include it, check system directory)
    set(VULKAN_DLL_PATH "${VULKAN_LOCAL_PATH}/vulkan-1.dll")
    if(EXISTS "${VULKAN_DLL_PATH}")
        message(STATUS "Found vulkan-1.dll at: ${VULKAN_DLL_PATH}")
        set(VULKAN_DLL_FOUND TRUE)
    elseif(EXISTS "${VULKAN_BIN_DIR}/vulkan-1.dll")
        set(VULKAN_DLL_PATH "${VULKAN_BIN_DIR}/vulkan-1.dll")
        message(STATUS "Found vulkan-1.dll at: ${VULKAN_DLL_PATH}")
        set(VULKAN_DLL_FOUND TRUE)
    elseif(WIN32 AND EXISTS "C:/Windows/System32/vulkan-1.dll")
        # New SDK versions don't include vulkan-1.dll, use system DLL
        set(VULKAN_DLL_PATH "C:/Windows/System32/vulkan-1.dll")
        message(STATUS "Found vulkan-1.dll in system directory: ${VULKAN_DLL_PATH}")
        message(STATUS "Note: New Vulkan SDK versions don't include vulkan-1.dll")
        message(STATUS "Using system DLL from Windows/System32")
        set(VULKAN_DLL_FOUND TRUE)
    else()
        message(WARNING "vulkan-1.dll not found in ${VULKAN_LOCAL_PATH}, ${VULKAN_BIN_DIR}, or system directory")
        message(WARNING "Runtime may fail if vulkan-1.dll is not in PATH or output directory")
        message(WARNING "Please install Vulkan Runtime or copy vulkan-1.dll to external/vulkan/")
        set(VULKAN_DLL_FOUND FALSE)
    endif()
    
    # Don't add Vulkan include directory globally to avoid conflicts with third_party/glslang
    # Vulkan includes will be added per-target where needed
    # include_directories(${Vulkan_INCLUDE_DIRS})
else()
    # Use standard find_package
    find_package(Vulkan REQUIRED)
    
    # Try to find vulkan-1.dll from standard Vulkan SDK installation
    if(WIN32)
        # find_package(Vulkan) sets Vulkan_SDK, not VULKAN_SDK
        if(DEFINED Vulkan_SDK)
            set(VULKAN_SDK "${Vulkan_SDK}")
        elseif(DEFINED ENV{VULKAN_SDK})
            set(VULKAN_SDK "$ENV{VULKAN_SDK}")
        endif()
        
        if(VULKAN_SDK)
            set(VULKAN_DLL_PATH "${VULKAN_SDK}/Bin/vulkan-1.dll")
            if(EXISTS "${VULKAN_DLL_PATH}")
                message(STATUS "Found vulkan-1.dll at: ${VULKAN_DLL_PATH}")
                set(VULKAN_DLL_FOUND TRUE)
            else()
                # Try root directory
                set(VULKAN_DLL_PATH "${VULKAN_SDK}/vulkan-1.dll")
                if(EXISTS "${VULKAN_DLL_PATH}")
                    message(STATUS "Found vulkan-1.dll at: ${VULKAN_DLL_PATH}")
                    set(VULKAN_DLL_FOUND TRUE)
                else()
                    # New SDK versions don't include vulkan-1.dll, try system directory
                    if(EXISTS "C:/Windows/System32/vulkan-1.dll")
                        set(VULKAN_DLL_PATH "C:/Windows/System32/vulkan-1.dll")
                        message(STATUS "Found vulkan-1.dll in system directory: ${VULKAN_DLL_PATH}")
                        message(STATUS "Note: New Vulkan SDK versions don't include vulkan-1.dll")
                        set(VULKAN_DLL_FOUND TRUE)
                    else()
                        message(WARNING "vulkan-1.dll not found in Vulkan SDK directory or system directory")
                        set(VULKAN_DLL_FOUND FALSE)
                    endif()
                endif()
            endif()
        else()
            # Try system directory if VULKAN_SDK not set
            if(EXISTS "C:/Windows/System32/vulkan-1.dll")
                set(VULKAN_DLL_PATH "C:/Windows/System32/vulkan-1.dll")
                message(STATUS "Found vulkan-1.dll in system directory: ${VULKAN_DLL_PATH}")
                set(VULKAN_DLL_FOUND TRUE)
            else()
                message(WARNING "VULKAN_SDK not set and system DLL not found")
                message(WARNING "Cannot locate vulkan-1.dll")
                set(VULKAN_DLL_FOUND FALSE)
            endif()
        endif()
    else()
        set(VULKAN_DLL_FOUND FALSE)
    endif()
endif()

# Function to copy vulkan-1.dll to output directory
function(copy_vulkan_dll target)
    if(WIN32 AND VULKAN_DLL_FOUND AND EXISTS "${VULKAN_DLL_PATH}")
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${VULKAN_DLL_PATH}"
            $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying vulkan-1.dll to output directory"
        )
        message(STATUS "Configured ${target} to copy vulkan-1.dll to output directory")
    endif()
endfunction()

# Add subdirectories for each module
# Core modules (FirstEngine自己的模块)
add_subdirectory(src/Shader)
add_subdirectory(src/Resources)
add_subdirectory(src/Core)
add_subdirectory(src/Python)
add_subdirectory(src/Application)

# Graphics modules (图形相关模块)
add_subdirectory(src/RHI)
add_subdirectory(src/Device)
add_subdirectory(src/Renderer)

# Editor module (编辑器API)
add_subdirectory(src/Editor)

# C# Editor Project (添加到Visual Studio解决方案)
add_subdirectory(Editor)

# Tools (工具项目)
add_subdirectory(src/Tools/ShaderManager)
add_subdirectory(src/Tools/ResourceImport)
