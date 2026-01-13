#include "FirstEngine/Python/PythonEngine.h"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <iostream>
#include <vector>
#include <map>

namespace py = pybind11;
using namespace FirstEngine::Python;

int main() {
    std::cout << "=== FirstEngine Python双向调用Demo ===" << std::endl;
    
    // 初始化Python引擎
    PythonEngine engine;
    if (!engine.Initialize()) {
        std::cerr << "Failed to initialize Python: " << engine.GetLastError() << std::endl;
        return 1;
    }
    
    std::cout << "\n[1] 从C++调用Python函数（基本类型）" << std::endl;
    try {
        // 执行Python代码定义函数
        engine.ExecuteString(R"(
def python_add(a, b):
    return a + b

def python_multiply(a, b):
    return a * b

def python_hello(name):
    return f"Hello, {name}!"
)");
        
        // 调用Python函数 - 使用pybind11直接调用
        py::module_ main = py::module_::import("__main__");
        py::object add_func = main.attr("python_add");
        py::object result = add_func(10, 20);
        int sum = result.cast<int>();
        std::cout << "  Python add(10, 20) = " << sum << std::endl;
        
        py::object multiply_func = main.attr("python_multiply");
        result = multiply_func(3.5f, 2.0f);
        float product = result.cast<float>();
        std::cout << "  Python multiply(3.5, 2.0) = " << product << std::endl;
        
        py::object hello_func = main.attr("python_hello");
        result = hello_func(std::string("FirstEngine"));
        std::string greeting = result.cast<std::string>();
        std::cout << "  Python hello('FirstEngine') = " << greeting << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n[2] 从C++调用Python函数（容器类型）" << std::endl;
    try {
        engine.ExecuteString(R"(
def python_process_list(numbers):
    return [x * 2 for x in numbers]

def python_process_dict(data):
    result = {}
    for key, value in data.items():
        result[key] = value * 10
    return result
)");
        
        // 调用处理列表的函数
        std::vector<int> input_list = {1, 2, 3, 4, 5};
        py::module_ main = py::module_::import("__main__");
        py::object process_list_func = main.attr("python_process_list");
        py::object result = process_list_func(input_list);
        std::vector<int> output = result.cast<std::vector<int>>();
        std::cout << "  Input: [1, 2, 3, 4, 5]" << std::endl;
        std::cout << "  Output: [";
        for (size_t i = 0; i < output.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << output[i];
        }
        std::cout << "]" << std::endl;
        
        // 调用处理字典的函数
        std::map<std::string, int> input_map = {{"a", 1}, {"b", 2}, {"c", 3}};
        py::object process_dict_func = main.attr("python_process_dict");
        result = process_dict_func(input_map);
        std::map<std::string, int> output_map = result.cast<std::map<std::string, int>>();
        std::cout << "  Input: {'a': 1, 'b': 2, 'c': 3}" << std::endl;
        std::cout << "  Output: {";
        bool first = true;
        for (const auto& pair : output_map) {
            if (!first) std::cout << ", ";
            std::cout << "'" << pair.first << "': " << pair.second;
            first = false;
        }
        std::cout << "}" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "  Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n[3] 从Python调用C++函数（需要先导入模块）" << std::endl;
    try {
        // 注意：在实际使用中，firstengine模块需要先编译并安装
        // 这里演示如何从Python调用C++函数
        engine.ExecuteString(R"(
# 如果firstengine模块已编译，可以这样使用：
# import firstengine
# result = firstengine.add(10, 20)
# print(f"C++ add(10, 20) = {result}")
# 
# v1 = firstengine.Vector3(1, 2, 3)
# v2 = firstengine.Vector3(4, 5, 6)
# v3 = firstengine.add_vectors(v1, v2)
# print(f"Vector3 addition: {v3}")

print("  (需要先编译firstengine模块才能从Python调用C++函数)")
)");
    } catch (const std::exception& e) {
        std::cerr << "  Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n[4] Python回调函数示例" << std::endl;
    try {
        engine.ExecuteString(R"(
def python_callback(x):
    return x * x + 1

# 这个回调函数可以被C++调用
)");
        
        // 从C++获取Python函数并调用
        py::module_ main = py::module_::import("__main__");
        py::object callback = main.attr("python_callback");
        
        for (int i = 1; i <= 5; ++i) {
            py::object result = callback(i);
            int value = result.cast<int>();
            std::cout << "  Callback(" << i << ") = " << value << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "  Error: " << e.what() << std::endl;
    }
    
    std::cout << "\n=== Demo完成 ===" << std::endl;
    
    return 0;
}
