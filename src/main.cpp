#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fmt/core.h>
#include <fmt/color.h>
#include <fmt/ranges.h>

#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <unordered_set>
#include <set>

// Local includes
#include <queues.hpp>
#include <swapchain.hpp>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

// Debug stuff
const std::vector<const char*> validationLayer = {
    "VK_LAYER_KHRONOS_validation"
};

// Necessary extensions
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

#define DEBUG

VkResult CreateDebugUtilsMessengerEXT (
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
    const VkAllocationCallbacks* pAllocator,
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT debugMessenger,
    const VkAllocationCallbacks *pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");

    if (func != nullptr)
        func(instance, debugMessenger, pAllocator);
}

class HelloTriangleApplication {
public:
    void run(){
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapChain;

    void initWindow() {
        #if !defined(DEBUG) && defined(__linux__) || defined(__unix__)
            // Debug to run the window inside Visual Studio Code
            glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        #endif

        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        pickPhysicalDevice();
        createLogicalDevice();
        createSwapChain();
    }

    void mainLoop() {
        while(!glfwWindowShouldClose(this->window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
        vkDestroyDevice(this->device, nullptr);

        if (enableValidationLayers)
           DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);

        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        vkDestroyInstance(this->instance, nullptr);
        glfwDestroyWindow(this->window);
        glfwTerminate();
    }

    void createInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_API_VERSION(1, 0, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        if (enableValidationLayers && !checkValidationLayersSupport())
            throw std::runtime_error("validation layers requested, but not available!");

        if (!isGlfwSupported())
            throw std::runtime_error("Some Vulkan necessary extensions are missing on this machine");

        auto requiredExtensions = getRequiredExtensions();

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
            createInfo.ppEnabledLayerNames = validationLayer.data();
            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;
            createInfo.pNext = nullptr;
        }

        // (Opcional) listar extensões suportadas, apenas para log
        auto extensions = supportedVulkanExtensions();
        fmt::print("Available extensions:\n");
        for (const auto& e : extensions)
            fmt::print("\t{}\n", e.extensionName);

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
            throw std::runtime_error("Failed to create instance!");
}

    void createSurface() {
        if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VK_SUCCESS)
            throw std::runtime_error("failed to create window surface!");
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity =
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType =
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to setup debug messenger!");
        }
    }

    bool checkValidationLayersSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayer) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (std::strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound)
                return false;
        }

        return true;
    }

    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    // Check if all necessary GLFW vulkan extensions are available on the host machine
    bool isGlfwSupported() {
        // All GLFW extensions needed
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        // All local vulkan extensions available
        auto localExtensions = supportedVulkanExtensions();

        // Search if all glfw extensions are available
        for (int i = 0; i < glfwExtensionCount; i++) {
            bool found = false;

            for (const auto& localExt : localExtensions) {
                if (std::strcmp(glfwExtensions[i], localExt.extensionName) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found)
                return false;
        }

        return true;
    }

    std::vector<VkExtensionProperties> supportedVulkanExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> extensions(extensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        return extensions;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "validations layers: {}\n", pCallbackData->pMessage);
        }

        return VK_FALSE;
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);

        if (deviceCount == 0)
            throw std::runtime_error("failed to find GPUs with Vulkan support!");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());

        // Find the first suitable device
        bool isGpuSelected = false;
        for (const auto& device : devices) {
            if (isDeviceSuitable(device) && !isGpuSelected) {
                this->physicalDevice = device;
                isGpuSelected = true;
                fmt::print(fmt::fg(fmt::color::green), "Selected GPU: \n");

            } else if (isDeviceSuitable(device) && isGpuSelected) {
                fmt::print(fmt::fg(fmt::color::yellow), "Found another suitable GPU: \n");

            } else {
                fmt::print(fmt::fg(fmt::color::red), "Found an unsuitable GPU: \n");

            }

            displayDeviceFeatures(device);
        }

        if (this->physicalDevice == VK_NULL_HANDLE)
            throw std::runtime_error("failed to find a suitable GPU!");
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device, this->surface);

        bool extensionsSupported = checkDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device, this->surface);
            swapChainAdequate = !swapChainSupport.formats.empty() &&
                                !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void displayDeviceFeatures(VkPhysicalDevice device) {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

        fmt::print("Device Name: {}\n", deviceProperties.deviceName);
        fmt::print("API Version: {}.{}.{}\n",
            VK_API_VERSION_MAJOR(deviceProperties.apiVersion),
            VK_API_VERSION_MINOR(deviceProperties.apiVersion),
            VK_API_VERSION_PATCH(deviceProperties.apiVersion)
        );
        fmt::print("Driver Version: {}\n", deviceProperties.driverVersion);
        fmt::print("Vendor ID: {}\n", deviceProperties.vendorID);
        fmt::print("Device ID: {}\n", deviceProperties.deviceID);

        fmt::print("Geometry Shader: {}\n", deviceFeatures.geometryShader ? "Supported" : "Not Supported");
        fmt::print("Tessellation Shader: {}\n", deviceFeatures.tessellationShader ? "Supported" : "Not Supported");
        fmt::print("Wide Lines: {}\n", deviceFeatures.wideLines ? "Supported" : "Not Supported");
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice, this->surface);
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
        queueCreateInfo.queueCount = 1;

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
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
        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayer.size());
            createInfo.ppEnabledLayerNames = validationLayer.data();

        } else {
            createInfo.enabledLayerCount = 0;

        }

        if (vkCreateDevice(this->physicalDevice, &createInfo, nullptr, &this->device) != VK_SUCCESS)
        throw std::runtime_error("failed to create logical device!");

        vkGetDeviceQueue(this->device, indices.graphicsFamily.value(), 0, &this->graphicsQueue);
        vkGetDeviceQueue(this->device, indices.presentFamily.value(), 0, &this->presentQueue);
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(this->physicalDevice, this->surface);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, this->window);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount >
            swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(this->physicalDevice, this->surface);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                        indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;

        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices = nullptr;

        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
            throw std::runtime_error("failed to create swap chain!");
    }
};


int main() {
    HelloTriangleApplication app;

    fmt::print(fmt::fg(fmt::color::blue) | fmt::emphasis::bold, "Welcome to Vulkan test!\n\n");

    try {
        app.run();
    } catch (const std::exception& e) {
        fmt::print(stderr, fmt::fg(fmt::color::dark_red), "error starting app: {}\n", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
