#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_EXPORTS
        #define FE_API __declspec(dllexport)
    #else
        #define FE_API __declspec(dllimport)
    #endif
#else
    #define FE_API __attribute__((visibility("default")))
#endif
