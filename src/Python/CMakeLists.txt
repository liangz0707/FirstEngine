cmake_minimum_required(VERSION 3.20)

project(FirstEngine_Python VERSION 1.0.0 LANGUAGES CXX)

# Set export macro for this module
if(WIN32)
    add_definitions(-DFirstEngine_Python_EXPORTS)
endif()

# Use local pybind11 from third_party
set(PYBIND11_NOPYTHON ON CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/third_party/pybind11 pybind11)

# Find Python 3 (required for pybind11)
find_package(Python3 COMPONENTS Interpreter Development QUIET)
if(NOT Python3_FOUND)
    message(WARNING "Python3 not found. FirstEngine_Python module will be disabled.")
    message(STATUS "Please install Python 3.x to enable Python scripting support.")
    return()
endif()

# Source files
set(PYTHON_SOURCES
    PythonEngine.cpp
)

# Create shared library
add_library(FirstEngine_Python SHARED
    ${PYTHON_SOURCES}
)

# Include directories
target_include_directories(FirstEngine_Python
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(FirstEngine_Python
    PUBLIC
        pybind11::embed
        Python3::Python
    PRIVATE
        pybind11::module
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Python PRIVATE /W4)
    target_compile_definitions(FirstEngine_Python PUBLIC FirstEngine_Python_EXPORTS)
else()
    target_compile_options(FirstEngine_Python PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set library output name
set_target_properties(FirstEngine_Python PROPERTIES
    OUTPUT_NAME "FirstEngine_Python"
    VERSION ${PROJECT_VERSION}
)

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Python POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Python>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()
