#pragma once

#ifdef _WIN32
    #ifdef FirstEngine_RHI_EXPORTS
        #define FE_RHI_API __declspec(dllexport)
    #else
        #define FE_RHI_API __declspec(dllimport)
    #endif
#else
    #define FE_RHI_API
#endif
