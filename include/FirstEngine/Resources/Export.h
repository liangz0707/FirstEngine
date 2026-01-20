#pragma once

#ifdef _WIN32
    // FirstEngine_Resources is a STATIC library, so we don't need __declspec(dllexport/dllimport)
    // Static libraries are linked directly into the executable/DLL, so symbols don't need to be exported
    #define FE_RESOURCES_API
#else
    #define FE_RESOURCES_API __attribute__((visibility("default")))
#endif
