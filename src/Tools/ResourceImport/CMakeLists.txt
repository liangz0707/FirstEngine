cmake_minimum_required(VERSION 3.20)

# Source files
set(RESOURCE_IMPORT_SOURCES
    main.cpp
    ResourceImport.cpp
)

# Create executable
add_executable(ResourceImport
    ${RESOURCE_IMPORT_SOURCES}
)

# Include directories
target_include_directories(ResourceImport
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(ResourceImport
    PRIVATE
        FirstEngine_Resources
        FirstEngine_Shader
)

# Link Assimp for mesh/model loading
if(TARGET assimp)
    target_link_libraries(ResourceImport PRIVATE assimp)
endif()

# Compiler-specific options
if(MSVC)
    target_compile_options(ResourceImport PRIVATE /W4)
else()
    target_compile_options(ResourceImport PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Set output directory
set_target_properties(ResourceImport PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
)

# Copy DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ResourceImport POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Resources>
        $<TARGET_FILE:FirstEngine_Shader>
        $<TARGET_FILE_DIR:ResourceImport>
    )
    # Copy Assimp DLL if available
    if(TARGET assimp)
        add_custom_command(TARGET ResourceImport POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:assimp>
            $<TARGET_FILE_DIR:ResourceImport>
        )
    endif()
endif()

# Organize in Visual Studio solution
set_target_properties(ResourceImport PROPERTIES
    FOLDER "Tools"  # Organize in Visual Studio solution
)
