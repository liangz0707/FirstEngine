#include <iostream>
#include "test_glslang.h"

int main() {
    std::cout << "Testing glslang DLL..." << std::endl;
    std::cout << "Version: " << test_glslang_get_version() << std::endl;
    
    int result = test_glslang_compile();
    
    if (result == 0) {
        std::cout << "All tests passed!" << std::endl;
    } else {
        std::cout << "Tests failed with code: " << result << std::endl;
    }
    
    return result;
}
