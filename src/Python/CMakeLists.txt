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
# Try local external directory first, then fallback to D:\Python3.9
set(PYTHON_LOCAL_PATH "${CMAKE_SOURCE_DIR}/external/python")
if(EXISTS "${PYTHON_LOCAL_PATH}")
    message(STATUS "Using local Python: ${PYTHON_LOCAL_PATH}")
    set(PYTHON_ROOT_DIR "${PYTHON_LOCAL_PATH}" CACHE PATH "Python root directory")
else()
    # Set default Python path to D:\Python3.9 if not specified
    set(PYTHON_ROOT_DIR "D:/Python3.9" CACHE PATH "Python root directory")
endif()

# If custom path exists, use it directly
if(EXISTS "${PYTHON_ROOT_DIR}")
    message(STATUS "Using configured Python path: ${PYTHON_ROOT_DIR}")
    set(Python3_ROOT_DIR "${PYTHON_ROOT_DIR}")
    # Try find_package first with the custom root
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    
    # If still not found, set manually
    if(NOT Python3_FOUND)
        message(STATUS "Setting Python3 paths manually from ${PYTHON_ROOT_DIR}")
        set(Python3_INCLUDE_DIRS "${PYTHON_ROOT_DIR}/include")
        # Try to find the correct library name
        if(EXISTS "${PYTHON_ROOT_DIR}/libs/python39.lib")
            set(Python3_LIBRARIES "${PYTHON_ROOT_DIR}/libs/python39.lib")
        elseif(EXISTS "${PYTHON_ROOT_DIR}/libs/python3.lib")
            set(Python3_LIBRARIES "${PYTHON_ROOT_DIR}/libs/python3.lib")
        else()
            # Try to find any python*.lib
            file(GLOB PYTHON_LIBS "${PYTHON_ROOT_DIR}/libs/python*.lib")
            if(PYTHON_LIBS)
                list(GET PYTHON_LIBS 0 Python3_LIBRARIES)
                message(STATUS "Found Python library: ${Python3_LIBRARIES}")
            endif()
        endif()
        set(Python3_EXECUTABLE "${PYTHON_ROOT_DIR}/python.exe")
        
        # Set Python3_FOUND manually
        set(Python3_FOUND TRUE)
        set(Python3_Interpreter_FOUND TRUE)
        set(Python3_Development_FOUND TRUE)
        
        # Create imported target if it doesn't exist
        if(NOT TARGET Python3::Python)
            add_library(Python3::Python SHARED IMPORTED)
            # Find the DLL
            if(EXISTS "${PYTHON_ROOT_DIR}/python39.dll")
                set_target_properties(Python3::Python PROPERTIES
                    IMPORTED_LOCATION "${PYTHON_ROOT_DIR}/python39.dll"
                    INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                )
            elseif(EXISTS "${PYTHON_ROOT_DIR}/python3.dll")
                set_target_properties(Python3::Python PROPERTIES
                    IMPORTED_LOCATION "${PYTHON_ROOT_DIR}/python3.dll"
                    INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                )
            else()
                file(GLOB PYTHON_DLLS "${PYTHON_ROOT_DIR}/python*.dll")
                if(PYTHON_DLLS)
                    list(GET PYTHON_DLLS 0 PYTHON_DLL)
                    set_target_properties(Python3::Python PROPERTIES
                        IMPORTED_LOCATION "${PYTHON_DLL}"
                        INTERFACE_INCLUDE_DIRECTORIES "${Python3_INCLUDE_DIRS}"
                    )
                endif()
            endif()
        endif()
    endif()
else()
    # Try standard find_package
    find_package(Python3 COMPONENTS Interpreter Development QUIET)
    if(NOT Python3_FOUND)
        message(WARNING "Python3 not found. FirstEngine_Python module will be created but may not compile.")
        message(STATUS "Please install Python 3.x to enable Python scripting support.")
        message(STATUS "Or set PYTHON_ROOT_DIR to your Python installation directory (e.g., D:/Python3.9).")
        message(STATUS "The project will still appear in the solution, but compilation may fail.")
    endif()
endif()

# Source files
set(PYTHON_SOURCES
    PythonEngine.cpp
    Bindings.cpp
)

# Header files (add to project so they show up in Visual Studio)
set(PYTHON_HEADERS
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Python/Export.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Python/Bindings.h
    ${CMAKE_SOURCE_DIR}/include/FirstEngine/Python/PythonEngine.h
)

# Python扩展模块（用于从Python调用C++）
if(Python3_FOUND)
    # 使用标准方式创建Python扩展模块
    add_library(firstengine_python_module MODULE
        PythonModule.cpp
    )
    
    # 设置Python扩展模块属性
    set_target_properties(firstengine_python_module PROPERTIES
        PREFIX ""
        OUTPUT_NAME "firstengine"
        CXX_VISIBILITY_PRESET "hidden"
    )
    
    # 设置扩展名
    if(MSVC)
        set_target_properties(firstengine_python_module PROPERTIES
            SUFFIX ".pyd"
        )
    else()
        set_target_properties(firstengine_python_module PROPERTIES
            SUFFIX ".so"
        )
    endif()
    
    target_link_libraries(firstengine_python_module PRIVATE
        FirstEngine_Python
        pybind11::module
        pybind11::lto
        pybind11::windows_extras
    )
    
    # 设置输出目录
    set_target_properties(firstengine_python_module PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/$<CONFIG>
    )
    
    # 在Visual Studio中组织
    set_target_properties(firstengine_python_module PROPERTIES
        FOLDER "FirstEngine"
    )
endif()

# Create shared library
add_library(FirstEngine_Python SHARED
    ${PYTHON_SOURCES}
    ${PYTHON_HEADERS}
)

# Link Bindings
target_link_libraries(FirstEngine_Python
    PRIVATE
        pybind11::module
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
if(Python3_FOUND)
    target_link_libraries(FirstEngine_Python
        PUBLIC
            pybind11::embed
            Python3::Python
        PRIVATE
            pybind11::module
    )
else()
    # If Python3 not found, still create the target but it won't link
    # This ensures the project appears in the solution
    message(STATUS "FirstEngine_Python target created but Python3 not found - linking will fail")
    target_link_libraries(FirstEngine_Python
        PUBLIC
            pybind11::embed
        PRIVATE
            pybind11::module
    )
endif()

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
    FOLDER "FirstEngine"  # Organize in Visual Studio solution
)

# Organize pybind11 in Visual Studio solution
if(TARGET pybind11::embed)
    get_target_property(PYBIND11_EMBED_TARGET pybind11::embed ALIASED_TARGET)
    if(PYBIND11_EMBED_TARGET)
        set_target_properties(${PYBIND11_EMBED_TARGET} PROPERTIES FOLDER "ThirdPart/pybind11")
    endif()
endif()
if(TARGET pybind11::module)
    get_target_property(PYBIND11_MODULE_TARGET pybind11::module ALIASED_TARGET)
    if(PYBIND11_MODULE_TARGET)
        set_target_properties(${PYBIND11_MODULE_TARGET} PROPERTIES FOLDER "ThirdPart/pybind11")
    endif()
endif()

# Copy DLL to bin directory on Windows
if(WIN32)
    add_custom_command(TARGET FirstEngine_Python POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:FirstEngine_Python>
        ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
    )
    
    # 复制Python扩展模块
    if(TARGET firstengine_python_module)
        add_custom_command(TARGET firstengine_python_module POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:firstengine_python_module>
            ${CMAKE_BINARY_DIR}/bin/$<CONFIG>/
        )
    endif()
endif()

# Demo程序
if(Python3_FOUND)
    add_executable(PythonDemo
        demo.cpp
    )
    
    target_link_libraries(PythonDemo PRIVATE
        FirstEngine_Python
        pybind11::embed
        Python3::Python
    )
    
    target_include_directories(PythonDemo PRIVATE
        ${CMAKE_SOURCE_DIR}/include
    )
    
    set_target_properties(PythonDemo PROPERTIES
        FOLDER "Tools"
    )
    
    # 复制DLL
    if(WIN32)
        add_custom_command(TARGET PythonDemo POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:FirstEngine_Python>
            $<TARGET_FILE_DIR:PythonDemo>
        )
    endif()
endif()
