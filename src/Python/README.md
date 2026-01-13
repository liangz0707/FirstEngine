# FirstEngine_Python Module

Python scripting support for FirstEngine using pybind11.

## Requirements

- **Python 3.x** (Python 2.7 is not supported)
- **pybind11** (included as submodule)

## Features

- Embed Python interpreter in C++ application
- Execute Python scripts and code strings
- Error handling and error message retrieval

## Usage

```cpp
#include "FirstEngine/Python/PythonEngine.h"

using namespace FirstEngine::Python;

// Initialize Python engine
PythonEngine engine;
if (!engine.Initialize()) {
    std::cerr << "Failed to initialize Python: " << engine.GetLastError() << std::endl;
    return;
}

// Execute Python code
if (!engine.ExecuteString("print('Hello from Python!')")) {
    std::cerr << "Error: " << engine.GetLastError() << std::endl;
}

// Execute Python script file
if (!engine.ExecuteFile("script.py")) {
    std::cerr << "Error: " << engine.GetLastError() << std::endl;
}

// Shutdown (automatically called in destructor)
engine.Shutdown();
```

## Building

The module requires Python 3.x to be installed on your system. CMake will automatically find Python 3 if it's in your PATH.

If Python 3 is not found, the module will be skipped during configuration with a warning message.

## Future Enhancements

- Expose FirstEngine C++ classes to Python
- Two-way data exchange between C++ and Python
- Python callback support
- Python module loading and management
