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
# Note: FirstEngine_Editor does NOT link FirstEngine_Core to avoid circular dependency
# RenderDocHelper is implemented in header file, so no library link is needed
# FirstEngine_Editor (STATIC) will be linked into FirstEngine_Core (SHARED),
# so FirstEngine_Core will get EditorAPI symbols
target_link_libraries(FirstEngine_Editor
    PRIVATE
        # FirstEngine_Core - NOT linked (RenderDocHelper is header-only)
        FirstEngine_Device
        FirstEngine_Renderer
        FirstEngine_Resources
        FirstEngine_RHI
        FirstEngine_Shader
)

# Link GLFW (needed for Window.h which is now header-only)
# EditorAPI.cpp uses Window.h, so GLFW include path is required
if(TARGET glfw)
    target_link_libraries(FirstEngine_Editor PUBLIC glfw)
elseif(TARGET glfw3)
    target_link_libraries(FirstEngine_Editor PUBLIC glfw3)
elseif(glfw3_FOUND)
    target_link_libraries(FirstEngine_Editor PUBLIC ${GLFW3_LIBRARY})
    target_include_directories(FirstEngine_Editor PUBLIC ${GLFW3_INCLUDE_DIR})
elseif(glfw_FOUND)
    target_link_libraries(FirstEngine_Editor PUBLIC ${GLFW_LIBRARIES})
    target_include_directories(FirstEngine_Editor PUBLIC ${GLFW_INCLUDE_DIRS})
endif()

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
