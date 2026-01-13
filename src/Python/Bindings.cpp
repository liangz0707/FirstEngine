#include "FirstEngine/Python/Bindings.h"
#include <cmath>
#include <sstream>
#include <algorithm>
#include <functional>

namespace FirstEngine {
    namespace Python {

        // Vector3 实现
        Vector3::Vector3() : m_X(0.0f), m_Y(0.0f), m_Z(0.0f) {}
        
        Vector3::Vector3(float x, float y, float z) : m_X(x), m_Y(y), m_Z(z) {}
        
        float Vector3::GetX() const { return m_X; }
        float Vector3::GetY() const { return m_Y; }
        float Vector3::GetZ() const { return m_Z; }
        
        void Vector3::SetX(float x) { m_X = x; }
        void Vector3::SetY(float y) { m_Y = y; }
        void Vector3::SetZ(float z) { m_Z = z; }
        
        Vector3 Vector3::operator+(const Vector3& other) const {
            return Vector3(m_X + other.m_X, m_Y + other.m_Y, m_Z + other.m_Z);
        }
        
        Vector3 Vector3::operator-(const Vector3& other) const {
            return Vector3(m_X - other.m_X, m_Y - other.m_Y, m_Z - other.m_Z);
        }
        
        float Vector3::Dot(const Vector3& other) const {
            return m_X * other.m_X + m_Y * other.m_Y + m_Z * other.m_Z;
        }
        
        float Vector3::Length() const {
            return std::sqrt(m_X * m_X + m_Y * m_Y + m_Z * m_Z);
        }
        
        Vector3 Vector3::Normalize() const {
            float len = Length();
            if (len > 0.0001f) {
                return Vector3(m_X / len, m_Y / len, m_Z / len);
            }
            return Vector3(0, 0, 0);
        }
        
        std::string Vector3::ToString() const {
            std::ostringstream oss;
            oss << "Vector3(" << m_X << ", " << m_Y << ", " << m_Z << ")";
            return oss.str();
        }

        // 基本类型函数
        int Add(int a, int b) {
            return a + b;
        }
        
        float Multiply(float a, float b) {
            return a * b;
        }
        
        std::string Concatenate(const std::string& a, const std::string& b) {
            return a + b;
        }
        
        // 容器类型函数
        std::vector<int> ProcessIntVector(const std::vector<int>& input) {
            std::vector<int> result;
            result.reserve(input.size());
            for (int val : input) {
                result.push_back(val * 2); // 每个元素乘以2
            }
            return result;
        }
        
        std::vector<float> ProcessFloatVector(const std::vector<float>& input) {
            std::vector<float> result = input;
            std::transform(result.begin(), result.end(), result.begin(), 
                         [](float x) { return x * x; }); // 平方
            return result;
        }
        
        std::map<std::string, int> ProcessMap(const std::map<std::string, int>& input) {
            std::map<std::string, int> result;
            for (const auto& pair : input) {
                result[pair.first] = pair.second * 10; // 值乘以10
            }
            return result;
        }
        
        // 自定义类型函数
        Vector3 AddVectors(const Vector3& a, const Vector3& b) {
            return a + b;
        }
        
        float CalculateDistance(const Vector3& a, const Vector3& b) {
            Vector3 diff = a - b;
            return diff.Length();
        }
        
        // 混合类型函数
        std::string ProcessData(
            int id,
            float value,
            const std::string& name,
            const std::vector<int>& numbers,
            const Vector3& position
        ) {
            std::ostringstream oss;
            oss << "ID: " << id << ", Value: " << value << ", Name: " << name
                << ", Numbers: [";
            for (size_t i = 0; i < numbers.size(); ++i) {
                if (i > 0) oss << ", ";
                oss << numbers[i];
            }
            oss << "], Position: " << position.ToString();
            return oss.str();
        }
        
        // 返回多个值
        std::tuple<int, float, std::string> GetMultipleValues() {
            return std::make_tuple(42, 3.14f, "Hello from C++");
        }
        
        // 可选参数
        int SumWithDefault(int a, int b, int c) {
            return a + b + c;
        }
        
        // 回调函数
        int CallPythonFunction(int value, std::function<int(int)> callback) {
            if (callback) {
                return callback(value);
            }
            return value;
        }

    } // namespace Python
} // namespace FirstEngine
