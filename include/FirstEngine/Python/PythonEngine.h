#pragma once

#include "FirstEngine/Python/Export.h"
#include <string>
#include <memory>

namespace FirstEngine {
    namespace Python {
        class FE_PYTHON_API PythonEngine {
        public:
            PythonEngine();
            ~PythonEngine();

            // 初始化Python解释器
            bool Initialize();
            
            // 关闭Python解释器
            void Shutdown();

            // 执行Python脚本文件
            bool ExecuteFile(const std::string& filepath);

            // 执行Python代码字符串
            bool ExecuteString(const std::string& code);

            // 检查Python是否已初始化
            bool IsInitialized() const;

            // 获取错误信息
            std::string GetLastError() const;

        private:
            class Impl;
            std::unique_ptr<Impl> m_Impl;
        };
    }
}
