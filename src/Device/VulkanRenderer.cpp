#include "FirstEngine/Device/VulkanRenderer.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <iostream>
#include <stdexcept>
#include <set>
#include <string>
#include <cstring>
#include <vector>
#include <cstdlib>  // for getenv

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // Exclude rarely-used stuff from Windows headers
#define VK_USE_PLATFORM_WIN32_KHR  // Enable Win32 platform extensions
#include <Windows.h>
#undef CreateSemaphore  // Undefine Windows API macro to avoid conflict with our method
#endif

namespace FirstEngine {
    namespace Device {
        VulkanRenderer::VulkanRenderer(Core::Window* window)
            : m_Window(window), m_Instance(VK_NULL_HANDLE), m_DebugMessenger(VK_NULL_HANDLE),
              m_PhysicalDevice(VK_NULL_HANDLE), m_Device(VK_NULL_HANDLE),
              m_GraphicsQueue(VK_NULL_HANDLE), m_PresentQueue(VK_NULL_HANDLE),
              m_CommandPool(VK_NULL_HANDLE), m_GraphicsQueueFamily(0), m_PresentQueueFamily(0),
              m_Surface(VK_NULL_HANDLE), m_ImageAvailableSemaphore(VK_NULL_HANDLE),
              m_RenderFinishedSemaphore(VK_NULL_HANDLE), m_InFlightFence(VK_NULL_HANDLE) {
            
            CreateInstance();
            SetupDebugMessenger();
            CreateSurface();
            PickPhysicalDevice();
            CreateLogicalDevice();
            CreateCommandPool();
            
            m_DeviceContext = std::make_unique<DeviceContext>(
                m_Instance, m_Device, m_PhysicalDevice,
                m_GraphicsQueue, m_PresentQueue, m_CommandPool,
                m_GraphicsQueueFamily, m_PresentQueueFamily
            );
            
            // Swapchain will be created via VulkanDevice::CreateSwapchain() when needed
            
            CreateSyncObjects();
        }

        VulkanRenderer::~VulkanRenderer() {
            vkDeviceWaitIdle(m_Device);

            if (m_InFlightFence != VK_NULL_HANDLE) {
                vkDestroyFence(m_Device, m_InFlightFence, nullptr);
            }
            if (m_RenderFinishedSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_RenderFinishedSemaphore, nullptr);
            }
            if (m_ImageAvailableSemaphore != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_Device, m_ImageAvailableSemaphore, nullptr);
            }

            m_DeviceContext.reset();

            if (m_CommandPool != VK_NULL_HANDLE) {
                vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            }

            if (m_Device != VK_NULL_HANDLE) {
                vkDestroyDevice(m_Device, nullptr);
            }

            if (m_EnableValidationLayers && m_DebugMessenger != VK_NULL_HANDLE) {
                auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");
                if (func != nullptr) {
                    func(m_Instance, m_DebugMessenger, nullptr);
                }
            }

            if (m_Surface != VK_NULL_HANDLE) {
                vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            }

            if (m_Instance != VK_NULL_HANDLE) {
                vkDestroyInstance(m_Instance, nullptr);
            }
        }

        void VulkanRenderer::CreateInstance() {
            if (m_EnableValidationLayers && !CheckValidationLayerSupport()) {
                throw std::runtime_error("Validation layers requested, but not available!");
            }

            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "FirstEngine";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "FirstEngine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            auto extensions = GetRequiredExtensions();
            createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
            createInfo.ppEnabledExtensionNames = extensions.data();

            // Log enabled extensions
            std::cout << "Creating Vulkan instance with " << extensions.size() << " extensions:" << std::endl;
            for (const auto& ext : extensions) {
                std::cout << "  - " << ext << std::endl;
            }

            VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
            if (m_EnableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
                createInfo.ppEnabledLayerNames = m_ValidationLayers.data();

                debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
                debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                  VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
                debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
                debugCreateInfo.pfnUserCallback = DebugCallback;

                createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
            } else {
                createInfo.enabledLayerCount = 0;
                createInfo.pNext = nullptr;
            }

            VkResult instanceResult = vkCreateInstance(&createInfo, nullptr, &m_Instance);
            if (instanceResult != VK_SUCCESS) {
                std::string errorMsg = "Failed to create Vulkan instance! Error code: ";
                errorMsg += std::to_string(instanceResult);
                
                switch (instanceResult) {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_HOST_MEMORY)";
                        break;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_DEVICE_MEMORY)";
                        break;
                    case VK_ERROR_INITIALIZATION_FAILED:
                        errorMsg += " (VK_ERROR_INITIALIZATION_FAILED)";
                        break;
                    case VK_ERROR_LAYER_NOT_PRESENT:
                        errorMsg += " (VK_ERROR_LAYER_NOT_PRESENT)";
                        break;
                    case VK_ERROR_EXTENSION_NOT_PRESENT:
                        errorMsg += " (VK_ERROR_EXTENSION_NOT_PRESENT - One or more requested extensions are not available)";
                        break;
                    case VK_ERROR_INCOMPATIBLE_DRIVER:
                        errorMsg += " (VK_ERROR_INCOMPATIBLE_DRIVER - Driver is incompatible)";
                        break;
                    default:
                        errorMsg += " (Unknown error)";
                        break;
                }
                
                std::cerr << errorMsg << std::endl;
                throw std::runtime_error(errorMsg);
            }
            
            std::cout << "Vulkan instance created successfully!" << std::endl;
            
            // Verify enabled extensions
            uint32_t enabledExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &enabledExtensionCount, nullptr);
            std::vector<VkExtensionProperties> enabledExtensions(enabledExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &enabledExtensionCount, enabledExtensions.data());
            
            std::cout << "Instance has " << enabledExtensionCount << " available extensions" << std::endl;
            for (const auto& ext : extensions) {
                bool found = false;
                for (const auto& enabled : enabledExtensions) {
                    if (strcmp(ext, enabled.extensionName) == 0) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    std::cerr << "Warning: Extension " << ext << " was requested but not found in available extensions!" << std::endl;
                }
            }
        }

        void VulkanRenderer::CreateSurface() {
            // Check prerequisites
            if (!m_Window) {
                throw std::runtime_error("Failed to create window surface: Window is null!");
            }

            GLFWwindow* glfwWindow = m_Window->GetHandle();
            if (!glfwWindow) {
                throw std::runtime_error("Failed to create window surface: GLFW window handle is null!");
            }

            if (m_Instance == VK_NULL_HANDLE) {
                throw std::runtime_error("Failed to create window surface: Vulkan instance is null!");
            }

            // Check if GLFW supports Vulkan (this should have been checked earlier in GetRequiredExtensions)
            // But we check again here for safety and better error context
            if (!glfwVulkanSupported()) {
                std::cerr << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << "ERROR: Cannot create surface - Vulkan not supported!" << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << std::endl;
                std::cerr << "This error should not occur if GetRequiredExtensions() was called correctly." << std::endl;
                std::cerr << "Please ensure:" << std::endl;
                std::cerr << "  1. Vulkan Runtime Libraries are installed" << std::endl;
                std::cerr << "  2. Graphics drivers support Vulkan and are up-to-date" << std::endl;
                std::cerr << "  3. GLFW was initialized before checking Vulkan support" << std::endl;
                std::cerr << std::endl;
                
                // Try to get GLFW error for more details
                const char* errorDescription = nullptr;
                int errorCode = glfwGetError(&errorDescription);
                if (errorCode != GLFW_NO_ERROR && errorDescription) {
                    std::cerr << "GLFW Error Code: " << errorCode << std::endl;
                    std::cerr << "GLFW Error Description: " << errorDescription << std::endl;
                }
                std::cerr << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << std::endl;
                
                throw std::runtime_error("GLFW does not support Vulkan on this system. Cannot create surface.");
            }
            
            std::cout << "GLFW Vulkan support: OK" << std::endl;

            // Check if required extensions are enabled in the instance
            uint32_t enabledExtensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &enabledExtensionCount, nullptr);
            std::vector<VkExtensionProperties> enabledExtensions(enabledExtensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &enabledExtensionCount, enabledExtensions.data());

            bool hasSurfaceExtension = false;
            bool hasPlatformSurfaceExtension = false;
            for (const auto& ext : enabledExtensions) {
                if (strcmp(ext.extensionName, VK_KHR_SURFACE_EXTENSION_NAME) == 0) {
                    hasSurfaceExtension = true;
                }
#ifdef _WIN32
                if (strcmp(ext.extensionName, "VK_KHR_win32_surface") == 0) {
                    hasPlatformSurfaceExtension = true;
                }
#endif
            }

            std::cout << "Checking instance extensions:" << std::endl;
            std::cout << "  VK_KHR_surface: " << (hasSurfaceExtension ? "Enabled" : "NOT ENABLED!") << std::endl;
#ifdef _WIN32
            std::cout << "  VK_KHR_win32_surface: " << (hasPlatformSurfaceExtension ? "Enabled" : "NOT ENABLED!") << std::endl;
#endif
            
            if (!hasSurfaceExtension) {
                std::cerr << "Error: VK_KHR_surface extension is not enabled in the Vulkan instance!" << std::endl;
                std::cerr << "This extension must be enabled when creating the instance." << std::endl;
                throw std::runtime_error("VK_KHR_surface extension not enabled in instance!");
            }
            
#ifdef _WIN32
            if (!hasPlatformSurfaceExtension) {
                std::cerr << "Error: VK_KHR_win32_surface extension is not enabled in the Vulkan instance!" << std::endl;
                std::cerr << "This extension must be enabled when creating the instance." << std::endl;
                throw std::runtime_error("VK_KHR_win32_surface extension not enabled in instance!");
            }
#endif

            // Try to create the surface
            std::cout << "Attempting to create window surface..." << std::endl;
            VkResult result = glfwCreateWindowSurface(m_Instance, glfwWindow, nullptr, &m_Surface);
            
            if (result != VK_SUCCESS) {
                std::string errorMsg = "Failed to create window surface! Error code: ";
                errorMsg += std::to_string(result);
                
                // Add specific error descriptions
                switch (result) {
                    case VK_ERROR_OUT_OF_HOST_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_HOST_MEMORY - Out of host memory)";
                        break;
                    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
                        errorMsg += " (VK_ERROR_OUT_OF_DEVICE_MEMORY - Out of device memory)";
                        break;
                    case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
                        errorMsg += " (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR - Native window already in use)";
                        break;
                    case VK_ERROR_INITIALIZATION_FAILED:
                        errorMsg += " (VK_ERROR_INITIALIZATION_FAILED - Initialization failed)";
                        break;
                    case VK_ERROR_EXTENSION_NOT_PRESENT:
                        errorMsg += " (VK_ERROR_EXTENSION_NOT_PRESENT - Required extension not present)";
                        break;
                    case VK_ERROR_SURFACE_LOST_KHR:
                        errorMsg += " (VK_ERROR_SURFACE_LOST_KHR - Surface lost)";
                        break;
                    default:
                        errorMsg += " (Unknown error)";
                        break;
                }
                
                std::cerr << errorMsg << std::endl;
                std::cerr << "Diagnostic information:" << std::endl;
                std::cerr << "  Window handle: " << glfwWindow << std::endl;
                std::cerr << "  Vulkan instance: " << m_Instance << std::endl;
                std::cerr << "  GLFW Vulkan supported: " << (glfwVulkanSupported() ? "Yes" : "No") << std::endl;
                
                // List enabled extensions
                std::cerr << "  Enabled instance extensions:" << std::endl;
                for (const auto& ext : enabledExtensions) {
                    std::cerr << "    - " << ext.extensionName << std::endl;
                }
                
                throw std::runtime_error(errorMsg);
            }
            
            std::cout << "Window surface created successfully!" << std::endl;
        }

        void VulkanRenderer::SetupDebugMessenger() {
            if (!m_EnableValidationLayers) return;

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = DebugCallback;
            createInfo.pUserData = nullptr;

            auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");
            if (func != nullptr) {
                func(m_Instance, &createInfo, nullptr, &m_DebugMessenger);
            } else {
                throw std::runtime_error("Failed to set up debug messenger!");
            }
        }

        void VulkanRenderer::PickPhysicalDevice() {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

            if (deviceCount == 0) {
                throw std::runtime_error("Failed to find GPUs with Vulkan support!");
            }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

            for (const auto& device : devices) {
                VkPhysicalDeviceProperties deviceProperties;
                vkGetPhysicalDeviceProperties(device, &deviceProperties);

                uint32_t extensionCount;
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
                std::vector<VkExtensionProperties> availableExtensions(extensionCount);
                vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

                std::set<std::string> requiredExtensions(m_DeviceExtensions.begin(), m_DeviceExtensions.end());
                for (const auto& extension : availableExtensions) {
                    requiredExtensions.erase(extension.extensionName);
                }

                if (requiredExtensions.empty()) {
                    m_PhysicalDevice = device;
                    std::cout << "Selected GPU: " << deviceProperties.deviceName << std::endl;
                    break;
                }
            }

            if (m_PhysicalDevice == VK_NULL_HANDLE) {
                throw std::runtime_error("Failed to find a suitable GPU!");
            }
        }

        void VulkanRenderer::CreateLogicalDevice() {
            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(m_PhysicalDevice, &queueFamilyCount, queueFamilies.data());

            uint32_t graphicsFamily = UINT32_MAX;
            uint32_t presentFamily = UINT32_MAX;

            for (uint32_t i = 0; i < queueFamilyCount; i++) {
                if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphicsFamily = i;
                }

                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(m_PhysicalDevice, i, m_Surface, &presentSupport);
                if (presentSupport) {
                    presentFamily = i;
                }

                if (graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX) {
                    break;
                }
            }

            if (graphicsFamily == UINT32_MAX || presentFamily == UINT32_MAX) {
                throw std::runtime_error("Failed to find suitable queue families!");
            }

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = {graphicsFamily, presentFamily};

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};

            VkDeviceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(m_DeviceExtensions.size());
            createInfo.ppEnabledExtensionNames = m_DeviceExtensions.data();

            if (m_EnableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(m_ValidationLayers.size());
                createInfo.ppEnabledLayerNames = m_ValidationLayers.data();
            } else {
                createInfo.enabledLayerCount = 0;
            }

            if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create logical device!");
            }

            m_GraphicsQueueFamily = graphicsFamily;
            m_PresentQueueFamily = presentFamily;
            
            vkGetDeviceQueue(m_Device, graphicsFamily, 0, &m_GraphicsQueue);
            vkGetDeviceQueue(m_Device, presentFamily, 0, &m_PresentQueue);
        }

        void VulkanRenderer::CreateCommandPool() {
            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
            poolInfo.queueFamilyIndex = m_GraphicsQueueFamily;

            if (vkCreateCommandPool(m_Device, &poolInfo, nullptr, &m_CommandPool) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create command pool!");
            }
        }

        void VulkanRenderer::CreateSyncObjects() {
            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            if (vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
                vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFence) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create synchronization objects!");
            }
        }

        void VulkanRenderer::OnWindowResize(int width, int height) {
            // Swapchain recreation is now handled via RHI::ISwapchain::Recreate()
            // This method is kept for compatibility but does nothing
            (void)width;
            (void)height;
        }

        void VulkanRenderer::Present() {
            // Present is now handled via RHI::ISwapchain::Present()
            // This method is kept for compatibility but does nothing
            // Application should use ISwapchain interface directly
        }

        void VulkanRenderer::WaitIdle() {
            vkDeviceWaitIdle(m_Device);
        }

        bool VulkanRenderer::CheckValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : m_ValidationLayers) {
                bool layerFound = false;

                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }

                if (!layerFound) {
                    return false;
                }
            }

            return true;
        }

        std::vector<const char*> VulkanRenderer::GetRequiredExtensions() {
            // Ensure GLFW is initialized before querying Vulkan support
            // This avoids "The GLFW library is not initialized" errors
            if (!glfwInit()) {
                const char* errorDescription = nullptr;
                int errorCode = glfwGetError(&errorDescription);
                std::cerr << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << "ERROR: Failed to initialize GLFW before querying Vulkan support!" << std::endl;
                std::cerr << "===========================================================" << std::endl;
                if (errorDescription) {
                    std::cerr << "GLFW Error Code: " << errorCode << std::endl;
                    std::cerr << "GLFW Error Description: " << errorDescription << std::endl;
                }
                else {
                    std::cerr << "No additional GLFW error information available." << std::endl;
                }
                std::cerr << std::endl;
                return {};
            }

            // Check if GLFW supports Vulkan (requires GLFW to be initialized)
            if (!glfwVulkanSupported()) {
                std::cerr << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << "ERROR: GLFW reports Vulkan is not supported on this system!" << std::endl;
                std::cerr << "===========================================================" << std::endl;
                std::cerr << std::endl;
                std::cerr << "This usually means one of the following:" << std::endl;
                std::cerr << "  1. Vulkan loader (vulkan-1.dll) is not installed or not found" << std::endl;
                std::cerr << "  2. Graphics driver does not support Vulkan or is outdated" << std::endl;
                std::cerr << "  3. Vulkan Runtime Libraries are missing" << std::endl;
                std::cerr << std::endl;
                std::cerr << "Solutions:" << std::endl;
                std::cerr << "  - Update your graphics drivers to the latest version" << std::endl;
                std::cerr << "  - Install Vulkan Runtime Libraries from:" << std::endl;
                std::cerr << "    https://vulkan.lunarg.com/sdk/home#windows" << std::endl;
                std::cerr << "  - Verify that vulkan-1.dll exists in your system PATH" << std::endl;
                std::cerr << std::endl;
                
                // Try to get GLFW error for more details
                const char* errorDescription = nullptr;
                int errorCode = glfwGetError(&errorDescription);
                if (errorCode != GLFW_NO_ERROR && errorDescription) {
                    std::cerr << "GLFW Error Code: " << errorCode << std::endl;
                    std::cerr << "GLFW Error Description: " << errorDescription << std::endl;
                    std::cerr << std::endl;
                }
                
#ifdef _WIN32
                // Check if vulkan-1.dll can be found and show its location
                HMODULE vulkanModule = LoadLibraryA("vulkan-1.dll");
                if (vulkanModule) {
                    char dllPath[MAX_PATH];
                    if (GetModuleFileNameA(vulkanModule, dllPath, MAX_PATH)) {
                        std::cerr << "Note: vulkan-1.dll was found and loaded from:" << std::endl;
                        std::cerr << "  " << dllPath << std::endl;
                        
                        // Check if the DLL exports vkGetInstanceProcAddr (required function)
                        typedef void* (__stdcall *PFN_vkGetInstanceProcAddr)(void*, const char*);
                        PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = 
                            (PFN_vkGetInstanceProcAddr)GetProcAddress(vulkanModule, "vkGetInstanceProcAddr");
                        
                        if (vkGetInstanceProcAddr) {
                            std::cerr << "  DLL exports vkGetInstanceProcAddr: Yes" << std::endl;
                            
                            // Try to call vkEnumerateInstanceExtensionProperties to verify DLL works
                            typedef void* (__stdcall *PFN_vkGetInstanceProcAddr)(void*, const char*);
                            PFN_vkGetInstanceProcAddr vkGetInstanceProcAddrFunc = 
                                (PFN_vkGetInstanceProcAddr)vkGetInstanceProcAddr(nullptr, "vkEnumerateInstanceExtensionProperties");
                            
                            if (vkGetInstanceProcAddrFunc) {
                                std::cerr << "  DLL can query Vulkan functions: Yes" << std::endl;
                                std::cerr << "  But GLFW cannot use it. Possible reasons:" << std::endl;
                                std::cerr << "    - GLFW checks at a different time or context" << std::endl;
                                std::cerr << "    - DLL architecture mismatch (32-bit vs 64-bit)" << std::endl;
                                std::cerr << "    - DLL version incompatibility" << std::endl;
                                std::cerr << "    - Missing dependencies (other DLLs required by vulkan-1.dll)" << std::endl;
                            } else {
                                std::cerr << "  DLL can query Vulkan functions: No (DLL may be corrupted or incomplete)" << std::endl;
                            }
                        } else {
                            std::cerr << "  DLL exports vkGetInstanceProcAddr: No (DLL is invalid or corrupted!)" << std::endl;
                            std::cerr << "  This DLL cannot be used as a Vulkan loader." << std::endl;
                        }
                        
                        // Check DLL architecture
                        HANDLE hFile = CreateFileA(dllPath, GENERIC_READ, FILE_SHARE_READ, nullptr, 
                                                   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
                        if (hFile != INVALID_HANDLE_VALUE) {
                            DWORD fileSize = GetFileSize(hFile, nullptr);
                            std::cerr << "  DLL file size: " << fileSize << " bytes" << std::endl;
                            CloseHandle(hFile);
                        }
                    } else {
                        std::cerr << "Note: vulkan-1.dll was found, but cannot get its path." << std::endl;
                    }
                    FreeLibrary(vulkanModule);
                } else {
                    DWORD error = GetLastError();
                    std::cerr << "Note: vulkan-1.dll was NOT found by LoadLibraryA()." << std::endl;
                    std::cerr << "  Error code: " << error << std::endl;
                    
                    // Translate error code
                    switch (error) {
                        case ERROR_MOD_NOT_FOUND:
                            std::cerr << "  Error: Module not found" << std::endl;
                            break;
                        case ERROR_BAD_EXE_FORMAT:
                            std::cerr << "  Error: Bad executable format (architecture mismatch?)" << std::endl;
                            break;
                        case ERROR_DLL_INIT_FAILED:
                            std::cerr << "  Error: DLL initialization failed (missing dependencies?)" << std::endl;
                            break;
                        default:
                            std::cerr << "  Error: Unknown error" << std::endl;
                            break;
                    }
                    
                    // Check common locations
                    std::cerr << "  Searching common locations:" << std::endl;
                    const char* commonPaths[] = {
                        "C:\\VulkanSDK\\1.3.275.0\\Bin\\vulkan-1.dll",
                        "C:\\VulkanSDK\\1.3.250.0\\Bin\\vulkan-1.dll",
                        "C:\\VulkanSDK\\1.3.224.1\\Bin\\vulkan-1.dll",
                        "C:\\Windows\\System32\\vulkan-1.dll",
                        nullptr
                    };
                    
                    bool foundAny = false;
                    for (int i = 0; commonPaths[i] != nullptr; i++) {
                        if (GetFileAttributesA(commonPaths[i]) != INVALID_FILE_ATTRIBUTES) {
                            std::cerr << "    Found: " << commonPaths[i] << std::endl;
                            
                            // Try to load this specific DLL
                            HMODULE testModule = LoadLibraryA(commonPaths[i]);
                            if (testModule) {
                                std::cerr << "      Can load: Yes" << std::endl;
                                FreeLibrary(testModule);
                            } else {
                                std::cerr << "      Can load: No (Error: " << GetLastError() << ")" << std::endl;
                            }
                            foundAny = true;
                        }
                    }
                    
                    // Check environment variable
                    const char* vulkanSdk = getenv("VULKAN_SDK");
                    if (vulkanSdk) {
                        std::string sdkPath = std::string(vulkanSdk) + "\\Bin\\vulkan-1.dll";
                        if (GetFileAttributesA(sdkPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                            std::cerr << "    Found in VULKAN_SDK: " << sdkPath << std::endl;
                            foundAny = true;
                        }
                    }
                    
                    if (!foundAny) {
                        std::cerr << "    No vulkan-1.dll found in common locations." << std::endl;
                        std::cerr << "    This is the most likely cause of the problem." << std::endl;
                    }
                }
                std::cerr << std::endl;
#endif
                
                std::cerr << "===========================================================" << std::endl;
                std::cerr << std::endl;
                
                throw std::runtime_error("GLFW does not support Vulkan on this system. Please install Vulkan Runtime Libraries and update your graphics drivers.");
            }

            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            
            if (!glfwExtensions || glfwExtensionCount == 0) {
                std::cerr << "Error: Failed to get required GLFW extensions!" << std::endl;
                throw std::runtime_error("Failed to get required GLFW Vulkan extensions!");
            }

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            // Log the extensions from GLFW
            std::cout << "GLFW required extensions (" << glfwExtensionCount << "):" << std::endl;
            for (uint32_t i = 0; i < glfwExtensionCount; i++) {
                std::cout << "  - " << glfwExtensions[i] << std::endl;
            }

            // Add debug utils extension if validation layers are enabled
            if (m_EnableValidationLayers) {
                // Check if already present
                bool hasDebugUtils = false;
                for (uint32_t i = 0; i < glfwExtensionCount; i++) {
                    if (strcmp(glfwExtensions[i], VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                        hasDebugUtils = true;
                        break;
                    }
                }
                if (!hasDebugUtils) {
                    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
                    std::cout << "  - " << VK_EXT_DEBUG_UTILS_EXTENSION_NAME << " (added for validation layers)" << std::endl;
                }
            }

            // Note: GLFW already includes VK_KHR_surface and platform-specific surface extension
            // So we don't need to add them manually. The manual additions were causing duplicates.
            
            return extensions;
        }

        VKAPI_ATTR VkBool32 VKAPI_CALL VulkanRenderer::DebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData) {

            std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;
            return VK_FALSE;
        }
    }
}
