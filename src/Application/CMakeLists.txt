cmake_minimum_required(VERSION 3.20)

project(FirstEngine VERSION 1.0.0 LANGUAGES CXX)

# Source files
set(APP_SOURCES
    Application.cpp
    main.cpp
)

# Create executable
add_executable(${PROJECT_NAME}
    ${APP_SOURCES}
)

# Include directories
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/src
)

# Link all modules
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        FirstEngine_Core
        FirstEngine_Renderer
        FirstEngine_Shader
        FirstEngine_Resources
)

# Compiler-specific options
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Copy DLLs to output directory on Windows
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Core>
        $<TARGET_FILE:FirstEngine_Renderer>
        $<TARGET_FILE:FirstEngine_Shader>
        $<TARGET_FILE:FirstEngine_Resources>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
