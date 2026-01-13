#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_Shader_EXPORTS
        #define FE_SHADER_API __declspec(dllexport)
    #else
        #define FE_SHADER_API __declspec(dllimport)
    #endif
#else
    #define FE_SHADER_API __attribute__((visibility("default")))
#endif
