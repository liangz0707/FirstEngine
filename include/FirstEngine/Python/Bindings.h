#pragma once

#include "FirstEngine/Python/Export.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>
#include <tuple>

namespace FirstEngine {
    namespace Python {

        // 示例C++类 - 用于演示Python绑定
        class FE_PYTHON_API Vector3 {
        public:
            Vector3();
            Vector3(float x, float y, float z);
            
            float GetX() const;
            float GetY() const;
            float GetZ() const;
            void SetX(float x);
            void SetY(float y);
            void SetZ(float z);
            
            Vector3 operator+(const Vector3& other) const;
            Vector3 operator-(const Vector3& other) const;
            float Dot(const Vector3& other) const;
            float Length() const;
            Vector3 Normalize() const;
            
            std::string ToString() const;
            
        private:
            float m_X, m_Y, m_Z;
        };

        // 示例C++函数 - 基本类型
        FE_PYTHON_API int Add(int a, int b);
        FE_PYTHON_API float Multiply(float a, float b);
        FE_PYTHON_API std::string Concatenate(const std::string& a, const std::string& b);
        
        // 示例C++函数 - 容器类型
        FE_PYTHON_API std::vector<int> ProcessIntVector(const std::vector<int>& input);
        FE_PYTHON_API std::vector<float> ProcessFloatVector(const std::vector<float>& input);
        FE_PYTHON_API std::map<std::string, int> ProcessMap(const std::map<std::string, int>& input);
        
        // 示例C++函数 - 自定义类型
        FE_PYTHON_API Vector3 AddVectors(const Vector3& a, const Vector3& b);
        FE_PYTHON_API float CalculateDistance(const Vector3& a, const Vector3& b);
        
        // 示例C++函数 - 混合类型
        FE_PYTHON_API std::string ProcessData(
            int id,
            float value,
            const std::string& name,
            const std::vector<int>& numbers,
            const Vector3& position
        );
        
        // 示例C++函数 - 返回多个值（使用tuple）
        FE_PYTHON_API std::tuple<int, float, std::string> GetMultipleValues();
        
        // 示例C++函数 - 可选参数
        FE_PYTHON_API int SumWithDefault(int a, int b = 10, int c = 20);
        
        // 示例C++函数 - 回调函数（从Python接收函数）
        FE_PYTHON_API int CallPythonFunction(int value, std::function<int(int)> callback);

    } // namespace Python
} // namespace FirstEngine
