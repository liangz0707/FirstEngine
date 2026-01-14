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

            // Initialize Python interpreter
            bool Initialize();
            
            // Shutdown Python interpreter
            void Shutdown();

            // Execute Python script file
            bool ExecuteFile(const std::string& filepath);

            // Execute Python code string
            bool ExecuteString(const std::string& code);

            // Check if Python is initialized
            bool IsInitialized() const;

            // Get error message
            std::string GetLastError() const;

        private:
            class Impl;
            std::unique_ptr<Impl> m_Impl;
        };
    }
}

