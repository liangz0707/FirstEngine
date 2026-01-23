# C# Editor Project
# This CMakeLists.txt adds the C# editor project to the Visual Studio solution

if(WIN32 AND MSVC)
    # Get the absolute path to the C# project file
    # CMAKE_CURRENT_SOURCE_DIR is already Editor directory when this file is included
    # (because CMakeLists.txt is in Editor/ directory)
    get_filename_component(EDITOR_PROJECT_DIR "${CMAKE_CURRENT_SOURCE_DIR}" ABSOLUTE)
    set(EDITOR_PROJECT_FILE "${EDITOR_PROJECT_DIR}/FirstEngineEditor.csproj")
    
    # Check if the project file exists
    if(EXISTS "${EDITOR_PROJECT_FILE}")
        # Find dotnet executable
        find_program(DOTNET_EXECUTABLE
            NAMES dotnet
            PATHS
                "C:/Program Files/dotnet"
                "C:/Program Files (x86)/dotnet"
            PATH_SUFFIXES ""
            DOC "Path to dotnet executable"
        )
        
        if(NOT DOTNET_EXECUTABLE)
            message(WARNING "dotnet executable not found. C# Editor project will not be buildable.")
            set(DOTNET_EXECUTABLE "dotnet")
        endif()
        
        # Map CMake configuration to .NET configuration
        # Debug -> Debug, Release -> Release, etc.
        set(DOTNET_CONFIG $<CONFIG>)
        
        # Create a custom target for the C# editor project
        # This will make it appear in Visual Studio solution
        add_custom_target(FirstEngineEditor
            COMMAND ${CMAKE_COMMAND} -E echo "Building FirstEngineEditor (C#) project..."
            COMMAND ${DOTNET_EXECUTABLE} build "${EDITOR_PROJECT_FILE}" 
                -c ${DOTNET_CONFIG} 
                -v minimal
                --no-incremental
            WORKING_DIRECTORY "${EDITOR_PROJECT_DIR}"
            COMMENT "Building C# Editor Project (FirstEngineEditor)"
            SOURCES 
                "${EDITOR_PROJECT_FILE}"
                "${EDITOR_PROJECT_DIR}/App.xaml"
                "${EDITOR_PROJECT_DIR}/App.xaml.cs"
        )
        
        # Set properties to organize in Visual Studio solution
        set_target_properties(FirstEngineEditor PROPERTIES
            FOLDER "Editor"
            EXCLUDE_FROM_DEFAULT_BUILD FALSE
        )
        
        # Configure Visual Studio debugger properties so VS can run this project
        # Use $(Configuration) which is Visual Studio's configuration variable (Debug/Release)
        # This allows Visual Studio to recognize FirstEngineEditor as a runnable project
        set_target_properties(FirstEngineEditor PROPERTIES
            VS_DEBUGGER_COMMAND "${EDITOR_PROJECT_DIR}/bin/$(Configuration)/net8.0-windows/FirstEngineEditor.exe"
            VS_DEBUGGER_WORKING_DIRECTORY "${EDITOR_PROJECT_DIR}/bin/$(Configuration)/net8.0-windows"
            VS_DEBUGGER_ENVIRONMENT "PATH=${EDITOR_PROJECT_DIR}/bin/$(Configuration)/net8.0-windows;%PATH%"
        )
        
        # Add dependency: Editor should build after C++ libraries
        add_dependencies(FirstEngineEditor
            FirstEngine_Core
            FirstEngine_Device
            FirstEngine_RHI
            FirstEngine_Shader
            FirstEngine_Renderer
            FirstEngine_Resources
        )
        
        # Determine output directory based on configuration
        set(EDITOR_OUTPUT_DIR "${EDITOR_PROJECT_DIR}/bin/${DOTNET_CONFIG}/net8.0-windows")
        
        # Copy DLLs to editor output directory after build
        # Note: FirstEngine_Device is STATIC, so no DLL to copy
        # In Debug configuration, CMake adds 'd' suffix (FirstEngine_Cored.dll)
        # but C# P/Invoke expects FirstEngine_Core.dll, so we create both copies
        add_custom_command(TARGET FirstEngineEditor POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Copying C++ DLLs to Editor output directory..."
            COMMAND ${CMAKE_COMMAND} -E make_directory "${EDITOR_OUTPUT_DIR}"
            # Copy FirstEngine_Core.dll (with 'd' suffix in Debug)
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:FirstEngine_Core>
                "${EDITOR_OUTPUT_DIR}/"
            # Copy FirstEngine_RHI.dll
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:FirstEngine_RHI>
                "${EDITOR_OUTPUT_DIR}/"
            # Copy FirstEngine_Shader.dll
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:FirstEngine_Shader>
                "${EDITOR_OUTPUT_DIR}/"
            COMMENT "Copying C++ DLLs to Editor output directory"
        )
        
        # In Debug configuration, create copies without 'd' suffix for C# P/Invoke
        # This is done in a separate command to avoid generator expression issues
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_CONFIGURATION_TYPES)
            add_custom_command(TARGET FirstEngineEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E echo "Creating DLL copies without 'd' suffix for C# P/Invoke..."
                # Copy FirstEngine_Cored.dll to FirstEngine_Core.dll
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/bin/Debug/FirstEngine_Cored.dll"
                    "${EDITOR_OUTPUT_DIR}/FirstEngine_Core.dll"
                # Copy FirstEngine_RHId.dll to FirstEngine_RHI.dll
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/bin/Debug/FirstEngine_RHId.dll"
                    "${EDITOR_OUTPUT_DIR}/FirstEngine_RHI.dll"
                # Copy FirstEngine_Shaderd.dll to FirstEngine_Shader.dll
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${CMAKE_BINARY_DIR}/bin/Debug/FirstEngine_Shaderd.dll"
                    "${EDITOR_OUTPUT_DIR}/FirstEngine_Shader.dll"
                COMMENT "Creating DLL copies without 'd' suffix"
            )
        endif()
        
        # Copy vulkan-1.dll if found
        if(VULKAN_DLL_FOUND AND EXISTS "${VULKAN_DLL_PATH}")
            add_custom_command(TARGET FirstEngineEditor POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${VULKAN_DLL_PATH}"
                    "${EDITOR_OUTPUT_DIR}/"
                COMMENT "Copying vulkan-1.dll to Editor output directory"
            )
        endif()
        
        message(STATUS "C# Editor project added to solution: ${EDITOR_PROJECT_FILE}")
        message(STATUS "  Output directory: ${EDITOR_OUTPUT_DIR}")
        message(STATUS "  .NET executable: ${DOTNET_EXECUTABLE}")
    else()
        message(WARNING "C# Editor project file not found: ${EDITOR_PROJECT_FILE}")
    endif()
else()
    message(STATUS "C# Editor project is only available on Windows with MSVC")
endif()
