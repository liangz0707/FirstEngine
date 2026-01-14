#include "FirstEngine/Device/VulkanRenderer.h"
#include "FirstEngine/Device/Swapchain.h"
#include "FirstEngine/Device/DeviceContext.h"
#include <iostream>
#include <stdexcept>
#include <set>

#ifdef _WIN32
#include <Windows.h>
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
            
            // 创建设备上下文
            m_DeviceContext = std::make_unique<DeviceContext>(
                m_Instance, m_Device, m_PhysicalDevice,
                m_GraphicsQueue, m_PresentQueue, m_CommandPool,
                m_GraphicsQueueFamily, m_PresentQueueFamily
            );
            
            // 创建交换链
            m_Swapchain = std::make_unique<Swapchain>(m_DeviceContext.get(), m_Window, m_Surface);
            m_Swapchain->Create();
            
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

            m_Swapchain.reset();
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

            if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create Vulkan instance!");
            }
        }

        void VulkanRenderer::CreateSurface() {
            if (glfwCreateWindowSurface(m_Instance, m_Window->GetHandle(), nullptr, &m_Surface) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create window surface!");
            }
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
            if (width > 0 && height > 0) {
                m_Swapchain->Recreate();
            }
        }

        void VulkanRenderer::Present() {
            uint32_t imageIndex;
            if (m_Swapchain->AcquireNextImage(m_ImageAvailableSemaphore, VK_NULL_HANDLE, imageIndex)) {
                // TODO: Record command buffer and submit here
                // For now, just present
                m_Swapchain->Present(imageIndex, m_RenderFinishedSemaphore);
            }
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
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;
            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

            std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

            if (m_EnableValidationLayers) {
                extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
            }

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
