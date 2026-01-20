cmake_minimum_required(VERSION 3.20)
project(TestGlslang VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Use glslang from parent project
# Test with static library first to avoid DLL import library issues
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_BINARIES OFF CACHE BOOL "" FORCE)
set(ENABLE_GLSLANG_WEBMINIFIER OFF CACHE BOOL "" FORCE)
set(ENABLE_OPT OFF CACHE BOOL "" FORCE)
set(ALLOW_EXTERNAL_SPIRV_TOOLS OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_SOURCE_DIR}/../third_party/glslang glslang)

# Create a test DLL
add_library(test_glslang SHARED
    test_glslang.cpp
    test_glslang.h
)

# Set export macro
if(WIN32)
    target_compile_definitions(test_glslang PRIVATE TEST_GLSLANG_EXPORTS)
endif()

# Create a test executable to load and test the DLL
add_executable(test_glslang_exe
    test_main.cpp
)

# Link the test DLL
target_link_libraries(test_glslang_exe
    PRIVATE
        test_glslang
)

# Link glslang and SPIRV
# Note: When BUILD_SHARED_LIBS=ON, glslang and SPIRV are DLLs
# SPIRV functions are in SPIRV library, not glslang
# For DLLs, CMake automatically links the import library (.lib)
# The import library is generated alongside the DLL
target_link_libraries(test_glslang
    PRIVATE
        glslang
        SPIRV
)

# Add dependency to ensure SPIRV is built before test_glslang
add_dependencies(test_glslang SPIRV)

# For DLLs on Windows, the import library is in the same directory as the DLL
# Add both bin and lib directories to linker path
target_link_directories(test_glslang
    PRIVATE
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
        ${CMAKE_BINARY_DIR}/lib/$<CONFIG>
)

# Copy DLLs to output directory on Windows
# Note: When BUILD_SHARED_LIBS=OFF, glslang and SPIRV are static libraries, no DLLs to copy
if(WIN32 AND BUILD_SHARED_LIBS)
    add_custom_command(TARGET test_glslang POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:glslang>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
    add_custom_command(TARGET test_glslang POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SPIRV>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
endif()

# Include directories
target_include_directories(test_glslang
    PRIVATE
        ${CMAKE_SOURCE_DIR}/../third_party/glslang
)

# Compiler-specific options
if(MSVC)
    target_compile_options(test_glslang PRIVATE /W4)
    # Test with /Zp1 for struct alignment
    # target_compile_options(test_glslang PRIVATE /Zp1)
endif()
