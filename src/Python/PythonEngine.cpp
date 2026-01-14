#include "FirstEngine/Python/PythonEngine.h"
#include <pybind11/embed.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <stdexcept>
#include <iostream>

namespace py = pybind11;

namespace FirstEngine {
    namespace Python {
        class PythonEngine::Impl {
        public:
            bool m_Initialized = false;
            std::string m_LastError;

            bool Initialize() {
                if (m_Initialized) {
                    return true;
                }

                try {
                    // 初始化Python解释器
                    py::initialize_interpreter();
                    m_Initialized = true;
                    return true;
                } catch (const std::exception& e) {
                    m_LastError = e.what();
                    return false;
                }
            }

            void Shutdown() {
                if (m_Initialized) {
                    try {
                        py::finalize_interpreter();
                        m_Initialized = false;
                    } catch (const std::exception& e) {
                        m_LastError = e.what();
                    }
                }
            }

            bool ExecuteFile(const std::string& filepath) {
                if (!m_Initialized) {
                    m_LastError = "Python interpreter not initialized";
                    return false;
                }

                try {
                    // Use eval_file to execute Python file
                    py::eval_file(filepath, py::globals());
                    return true;
                } catch (const std::exception& e) {
                    m_LastError = e.what();
                    return false;
                }
            }

            bool ExecuteString(const std::string& code) {
                if (!m_Initialized) {
                    m_LastError = "Python interpreter not initialized";
                    return false;
                }

                try {
                    py::exec(code);
                    return true;
                } catch (const std::exception& e) {
                    m_LastError = e.what();
                    return false;
                }
            }
        };

        PythonEngine::PythonEngine() : m_Impl(std::make_unique<Impl>()) {
        }

        PythonEngine::~PythonEngine() {
            if (m_Impl) {
                m_Impl->Shutdown();
            }
        }

        bool PythonEngine::Initialize() {
            return m_Impl->Initialize();
        }

        void PythonEngine::Shutdown() {
            m_Impl->Shutdown();
        }

        bool PythonEngine::ExecuteFile(const std::string& filepath) {
            return m_Impl->ExecuteFile(filepath);
        }

        bool PythonEngine::ExecuteString(const std::string& code) {
            return m_Impl->ExecuteString(code);
        }

        bool PythonEngine::IsInitialized() const {
            return m_Impl->m_Initialized;
        }

        std::string PythonEngine::GetLastError() const {
            return m_Impl->m_LastError;
        }
    }
}
