#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_Resources_EXPORTS
        #define FE_RESOURCES_API __declspec(dllexport)
    #else
        #define FE_RESOURCES_API __declspec(dllimport)
    #endif
#else
    #define FE_RESOURCES_API __attribute__((visibility("default")))
#endif
