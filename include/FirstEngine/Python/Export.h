#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_Python_EXPORTS
        #define FE_PYTHON_API __declspec(dllexport)
    #else
        #define FE_PYTHON_API __declspec(dllimport)
    #endif
#else
    #define FE_PYTHON_API __attribute__((visibility("default")))
#endif
