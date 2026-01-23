#pragma once

#ifdef _WIN32
    // FirstEngine_Renderer is a STATIC library, so we don't need __declspec(dllexport/dllimport)
    // Static libraries are linked directly into the executable/DLL, so symbols don't need to be exported
    #define FE_RENDERER_API
#else
    #define FE_RENDERER_API
#endif
