#pragma once

#ifdef _WIN32
    #ifdef TEST_GLSLANG_EXPORTS
        #define TEST_GLSLANG_API __declspec(dllexport)
    #else
        #define TEST_GLSLANG_API __declspec(dllimport)
    #endif
#else
    #define TEST_GLSLANG_API
#endif

extern "C" {
    TEST_GLSLANG_API int test_glslang_compile();
    TEST_GLSLANG_API const char* test_glslang_get_version();
}
