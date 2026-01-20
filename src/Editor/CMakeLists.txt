# Editor API module - provides C interface for C# editor

set(EDITOR_SOURCES
    EditorAPI.cpp
)

# Create static library for Editor API
add_library(FirstEngine_Editor STATIC
    ${EDITOR_SOURCES}
)

# Include directories
target_include_directories(FirstEngine_Editor
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# Link dependencies
target_link_libraries(FirstEngine_Editor
    PRIVATE
        FirstEngine_Core
        FirstEngine_Device
        FirstEngine_Renderer
        FirstEngine_Resources
        FirstEngine_RHI
        FirstEngine_Shader
)

# Compiler-specific options
if(MSVC)
    target_compile_options(FirstEngine_Editor PRIVATE /W4)
    target_compile_features(FirstEngine_Editor PRIVATE cxx_std_17)
else()
    target_compile_options(FirstEngine_Editor PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_features(FirstEngine_Editor PRIVATE cxx_std_17)
endif()

# Organize in Visual Studio solution
set_target_properties(FirstEngine_Editor PROPERTIES
    FOLDER "FirstEngine/Editor"
)
