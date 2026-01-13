#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_Renderer_EXPORTS
        #define FE_RENDERER_API __declspec(dllexport)
    #else
        #define FE_RENDERER_API __declspec(dllimport)
    #endif
#else
    #define FE_RENDERER_API __attribute__((visibility("default")))
#endif
