# FirstEngine Python绑定说明

本文档说明如何使用FirstEngine的Python绑定功能，实现C++和Python之间的双向调用。

## 功能特性

### 1. 从Python调用C++代码

FirstEngine提供了完整的pybind11绑定，允许从Python调用C++函数和类。

#### 支持的数据类型

- **基本类型**: `int`, `float`, `double`, `bool`, `std::string`
- **容器类型**: `std::vector<T>`, `std::map<K, V>`
- **自定义类型**: `Vector3`类
- **多返回值**: 使用`std::tuple`
- **可选参数**: 支持默认参数
- **回调函数**: 从C++调用Python函数

### 2. 从C++调用Python代码

`PythonEngine`类提供了调用Python函数的能力，支持各种参数类型。

## 编译

### 编译Python扩展模块

```bash
cd build
cmake --build . --config Release --target firstengine_python_module
```

编译后的模块将位于 `build/bin/Release/` 目录。

### 编译Demo程序

```bash
cmake --build . --config Release --target PythonDemo
```

## 使用方法

### 1. 从Python调用C++

#### 基本使用

```python
import firstengine as fe

# 调用C++函数
result = fe.add(10, 20)
print(result)  # 输出: 30

# 使用Vector3类
v1 = fe.Vector3(1.0, 2.0, 3.0)
v2 = fe.Vector3(4.0, 5.0, 6.0)
v3 = v1 + v2
print(v3)  # 输出: Vector3(5.0, 7.0, 9.0)
```

#### 容器类型

```python
# 处理整数列表
numbers = [1, 2, 3, 4, 5]
result = fe.process_int_vector(numbers)
print(result)  # 输出: [2, 4, 6, 8, 10]

# 处理字典
data = {"a": 1, "b": 2, "c": 3}
result = fe.process_map(data)
print(result)  # 输出: {'a': 10, 'b': 20, 'c': 30}
```

#### 回调函数

```python
def square(x):
    return x * x

# 将Python函数传递给C++
result = fe.call_python_function(5, square)
print(result)  # 输出: 25
```

### 2. 从C++调用Python

```cpp
#include "FirstEngine/Python/PythonEngine.h"
#include <pybind11/embed.h>

using namespace FirstEngine::Python;

PythonEngine engine;
engine.Initialize();

// 执行Python代码定义函数
engine.ExecuteString(R"(
def python_add(a, b):
    return a + b
)");

// 调用Python函数
pybind11::object result;
if (engine.CallFunction("__main__", "python_add", result, 10, 20)) {
    int sum = result.cast<int>();
    std::cout << "Result: " << sum << std::endl;
}
```

## 示例代码

### Python示例

运行 `demo_python.py`:

```bash
# 确保firstengine模块在Python路径中
cd build/bin/Release
python ../../../src/Python/demo_python.py
```

### C++示例

运行编译后的 `PythonDemo.exe`:

```bash
cd build/bin/Release
./PythonDemo.exe
```

## 可用的C++函数和类

### 函数

- `add(a, b)` - 两个整数相加
- `multiply(a, b)` - 两个浮点数相乘
- `concatenate(a, b)` - 连接两个字符串
- `process_int_vector(input)` - 处理整数向量
- `process_float_vector(input)` - 处理浮点数向量
- `process_map(input)` - 处理字典
- `add_vectors(a, b)` - 两个Vector3相加
- `calculate_distance(a, b)` - 计算两个Vector3之间的距离
- `process_data(id, value, name, numbers, position)` - 处理混合数据类型
- `get_multiple_values()` - 返回多个值（tuple）
- `sum_with_default(a, b=10, c=20)` - 带默认参数的求和
- `call_python_function(value, callback)` - 调用Python回调函数

### Vector3类

```python
v = fe.Vector3(1.0, 2.0, 3.0)

# 属性访问
v.get_x()
v.get_y()
v.get_z()
v.set_x(10.0)

# 方法
v.length()      # 计算长度
v.normalize()   # 归一化
v.dot(other)    # 点积

# 运算符
v1 + v2  # 加法
v1 - v2  # 减法
```

## 注意事项

1. **模块路径**: 确保`firstengine`模块在Python的搜索路径中
2. **Python版本**: 需要Python 3.x（不支持Python 2.7）
3. **线程安全**: 在多线程环境中使用需要注意GIL（Global Interpreter Lock）
4. **内存管理**: pybind11自动处理大部分内存管理，但需要注意循环引用

## 扩展绑定

要添加新的C++函数或类到Python绑定：

1. 在 `Bindings.h` 中声明函数或类
2. 在 `Bindings.cpp` 中实现
3. 在 `PythonModule.cpp` 中添加绑定代码
4. 重新编译模块

示例：

```cpp
// Bindings.h
FE_PYTHON_API int MyNewFunction(int x);

// Bindings.cpp
int MyNewFunction(int x) {
    return x * 2;
}

// PythonModule.cpp
m.def("my_new_function", &FirstEngine::Python::MyNewFunction, "My new function");
```
