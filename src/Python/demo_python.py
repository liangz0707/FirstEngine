#!/usr/bin/env python3
"""
FirstEngine Python绑定示例
演示如何从Python调用C++函数和类
"""

print("=== FirstEngine Python绑定示例 ===\n")

try:
    import firstengine as fe
    print("[1] 基本类型函数调用")
    
    # 调用C++函数 - 整数
    result = fe.add(10, 20)
    print(f"  fe.add(10, 20) = {result}")
    
    # 调用C++函数 - 浮点数
    result = fe.multiply(3.5, 2.0)
    print(f"  fe.multiply(3.5, 2.0) = {result}")
    
    # 调用C++函数 - 字符串
    result = fe.concatenate("Hello", " World")
    print(f"  fe.concatenate('Hello', ' World') = '{result}'")
    
    print("\n[2] 容器类型函数调用")
    
    # 调用C++函数 - 整数向量
    input_list = [1, 2, 3, 4, 5]
    result = fe.process_int_vector(input_list)
    print(f"  Input: {input_list}")
    print(f"  Output: {result}")
    
    # 调用C++函数 - 浮点数向量
    input_float_list = [1.0, 2.0, 3.0, 4.0, 5.0]
    result = fe.process_float_vector(input_float_list)
    print(f"  Input: {input_float_list}")
    print(f"  Output: {result}")
    
    # 调用C++函数 - 字典
    input_dict = {"a": 1, "b": 2, "c": 3}
    result = fe.process_map(input_dict)
    print(f"  Input: {input_dict}")
    print(f"  Output: {result}")
    
    print("\n[3] 自定义类型（Vector3）")
    
    # 创建Vector3对象
    v1 = fe.Vector3(1.0, 2.0, 3.0)
    v2 = fe.Vector3(4.0, 5.0, 6.0)
    
    print(f"  v1 = {v1}")
    print(f"  v2 = {v2}")
    print(f"  v1.get_x() = {v1.get_x()}")
    print(f"  v1.length() = {v1.length()}")
    
    # Vector3运算
    v3 = v1 + v2
    print(f"  v1 + v2 = {v3}")
    
    v4 = v1 - v2
    print(f"  v1 - v2 = {v4}")
    
    dot_product = v1.dot(v2)
    print(f"  v1.dot(v2) = {dot_product}")
    
    normalized = v1.normalize()
    print(f"  v1.normalize() = {normalized}")
    
    print("\n[4] 调用C++函数处理Vector3")
    
    v_sum = fe.add_vectors(v1, v2)
    print(f"  fe.add_vectors(v1, v2) = {v_sum}")
    
    distance = fe.calculate_distance(v1, v2)
    print(f"  fe.calculate_distance(v1, v2) = {distance}")
    
    print("\n[5] 混合类型函数调用")
    
    numbers = [10, 20, 30]
    position = fe.Vector3(1.5, 2.5, 3.5)
    result = fe.process_data(42, 3.14, "Test", numbers, position)
    print(f"  fe.process_data(...) = {result}")
    
    print("\n[6] 返回多个值")
    
    result = fe.get_multiple_values()
    print(f"  fe.get_multiple_values() = {result}")
    print(f"  Type: {type(result)}")
    
    print("\n[7] 可选参数")
    
    result = fe.sum_with_default(5)
    print(f"  fe.sum_with_default(5) = {result} (使用默认值b=10, c=20)")
    
    result = fe.sum_with_default(5, 15)
    print(f"  fe.sum_with_default(5, 15) = {result} (使用默认值c=20)")
    
    result = fe.sum_with_default(5, 15, 25)
    print(f"  fe.sum_with_default(5, 15, 25) = {result}")
    
    print("\n[8] 回调函数（从C++调用Python函数）")
    
    def python_square(x):
        return x * x
    
    # 将Python函数传递给C++
    result = fe.call_python_function(5, python_square)
    print(f"  fe.call_python_function(5, square) = {result}")
    
    print("\n=== 示例完成 ===")
    
except ImportError as e:
    print(f"错误: 无法导入firstengine模块")
    print(f"请确保已编译firstengine_python_module并安装到Python路径中")
    print(f"详细错误: {e}")
except Exception as e:
    print(f"错误: {e}")
    import traceback
    traceback.print_exc()
