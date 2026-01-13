#include "FirstEngine/Python/Bindings.h"
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <pybind11/operators.h>

namespace py = pybind11;

// Python模块绑定
PYBIND11_MODULE(firstengine, m) {
    m.doc() = "FirstEngine Python bindings - C++ functions and classes exposed to Python";
    
    // 绑定基本类型函数
    m.def("add", &FirstEngine::Python::Add, "Add two integers", py::arg("a"), py::arg("b"));
    m.def("multiply", &FirstEngine::Python::Multiply, "Multiply two floats", py::arg("a"), py::arg("b"));
    m.def("concatenate", &FirstEngine::Python::Concatenate, "Concatenate two strings", py::arg("a"), py::arg("b"));
    
    // 绑定容器类型函数
    m.def("process_int_vector", &FirstEngine::Python::ProcessIntVector, 
          "Process a vector of integers (multiply each by 2)", py::arg("input"));
    m.def("process_float_vector", &FirstEngine::Python::ProcessFloatVector, 
          "Process a vector of floats (square each)", py::arg("input"));
    m.def("process_map", &FirstEngine::Python::ProcessMap, 
          "Process a map (multiply values by 10)", py::arg("input"));
    
    // 绑定自定义类型函数
    m.def("add_vectors", &FirstEngine::Python::AddVectors, "Add two Vector3", 
          py::arg("a"), py::arg("b"));
    m.def("calculate_distance", &FirstEngine::Python::CalculateDistance, 
          "Calculate distance between two Vector3", py::arg("a"), py::arg("b"));
    
    // 绑定混合类型函数
    m.def("process_data", &FirstEngine::Python::ProcessData, 
          "Process mixed data types", 
          py::arg("id"), py::arg("value"), py::arg("name"), 
          py::arg("numbers"), py::arg("position"));
    
    // 绑定返回多个值的函数
    m.def("get_multiple_values", &FirstEngine::Python::GetMultipleValues, 
          "Return multiple values as a tuple");
    
    // 绑定可选参数函数
    m.def("sum_with_default", &FirstEngine::Python::SumWithDefault, 
          "Sum with default values", 
          py::arg("a"), py::arg("b") = 10, py::arg("c") = 20);
    
    // 绑定回调函数
    m.def("call_python_function", &FirstEngine::Python::CallPythonFunction, 
          "Call a Python function from C++", 
          py::arg("value"), py::arg("callback"));
    
    // 绑定Vector3类
    py::class_<FirstEngine::Python::Vector3>(m, "Vector3")
        .def(py::init<>())
        .def(py::init<float, float, float>(), py::arg("x") = 0.0f, py::arg("y") = 0.0f, py::arg("z") = 0.0f)
        .def("get_x", &FirstEngine::Python::Vector3::GetX, "Get X component")
        .def("get_y", &FirstEngine::Python::Vector3::GetY, "Get Y component")
        .def("get_z", &FirstEngine::Python::Vector3::GetZ, "Get Z component")
        .def("set_x", &FirstEngine::Python::Vector3::SetX, "Set X component", py::arg("x"))
        .def("set_y", &FirstEngine::Python::Vector3::SetY, "Set Y component", py::arg("y"))
        .def("set_z", &FirstEngine::Python::Vector3::SetZ, "Set Z component", py::arg("z"))
        .def("dot", &FirstEngine::Python::Vector3::Dot, "Dot product", py::arg("other"))
        .def("length", &FirstEngine::Python::Vector3::Length, "Calculate length")
        .def("normalize", &FirstEngine::Python::Vector3::Normalize, "Normalize vector")
        .def("__add__", &FirstEngine::Python::Vector3::operator+, py::is_operator())
        .def("__sub__", &FirstEngine::Python::Vector3::operator-, py::is_operator())
        .def("__str__", &FirstEngine::Python::Vector3::ToString)
        .def("__repr__", &FirstEngine::Python::Vector3::ToString);
}
