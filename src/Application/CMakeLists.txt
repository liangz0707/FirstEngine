# Note: This is a subdirectory, project is already defined in root CMakeLists.txt
# Application is the main executable

# Source files
set(APP_SOURCES
    Application.cpp
    RenderApp.cpp  # Contains main() function
)

# Create executable
add_executable(FirstEngine
    ${APP_SOURCES}
)

# Set the entry point
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/RenderApp.cpp")
    # RenderApp.cpp is the main entry point
endif()

# Include directories
target_include_directories(FirstEngine
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link all modules
target_link_libraries(FirstEngine
    PRIVATE
        FirstEngine_Core
        FirstEngine_Device
        FirstEngine_Shader
        FirstEngine_Resources
        FirstEngine_RHI
        FirstEngine_Renderer
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine PRIVATE /W4)
    target_compile_features(FirstEngine PRIVATE cxx_std_17)
else()
    target_compile_options(FirstEngine PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_features(FirstEngine PRIVATE cxx_std_17)
endif()

# Copy DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Core>
        $<TARGET_FILE:FirstEngine_Device>
        $<TARGET_FILE:FirstEngine_Shader>
        $<TARGET_FILE:FirstEngine_Resources>
        $<TARGET_FILE:FirstEngine_RHI>
        $<TARGET_FILE:FirstEngine_Renderer>
        $<TARGET_FILE_DIR:FirstEngine>
    )
endif()

# Organize in Visual Studio solution
set_target_properties(FirstEngine PROPERTIES
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)
