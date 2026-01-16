#pragma once

#ifdef _WIN32
#ifdef FirstEngine_Device_EXPORTS
                #define FE_DEVICE_API __declspec(dllexport)
            #else
                #define FE_DEVICE_API __declspec(dllimport)
            #endif
#else
    #define FE_DEVICE_API __attribute__((visibility("default")))
#endif
