cmake_minimum_required(VERSION 3.20)

# Source files
set(SHADER_MANAGER_SOURCES
    main.cpp
    ShaderManager.cpp
)

# Create executable
add_executable(ShaderManager
    ${SHADER_MANAGER_SOURCES}
)

# Include directories
target_include_directories(ShaderManager
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(ShaderManager
    PRIVATE
        FirstEngine_Shader
        FirstEngine_Resources
)

# Compiler-specific options
if(MSVC)
    target_compile_options(ShaderManager PRIVATE /W4)
else()
    target_compile_options(ShaderManager PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(ShaderManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# Copy DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ShaderManager POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Shader>
        $<TARGET_FILE:FirstEngine_Resources>
        $<TARGET_FILE_DIR:ShaderManager>
    )
endif()
