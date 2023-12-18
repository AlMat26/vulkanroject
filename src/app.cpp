#include "app.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <optional>
#include <set>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};


struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities; //Базовые возможности surface (минимальное / максимальное количество изображений в цепочке подкачки, минимальная / максимальная ширина и высота изображений)
    std::vector<VkSurfaceFormatKHR> formats; //Форматы поверхностей (формат пикселей, цветовое пространство)
    std::vector<VkPresentModeKHR> presentModes; //Доступные режимы презентации
};

SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice &device, VkSurfaceKHR &surface) {
    SwapChainSupportDetails details;

    //базовые возможности
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    std::cout << "Максимальное количество изображений в цепочке: " << details.capabilities.maxImageCount << std::endl;
    std::cout << "Минимальное количество изображений в цепочке: " << details.capabilities.minImageCount << std::endl;

    //формат поверхности
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());

    //режим представления
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());

    return details;
}

bool checkDeviceExtensionSupport(VkPhysicalDevice &device, std::vector<const char*> requiredExtensions) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::cout << "Доступные расширения:\n";
    for(const auto& extension : availableExtensions)
        std::cout << extension.extensionName << " " << extension.specVersion << std::endl;

    unsigned int requiredExtensionCount = requiredExtensions.size();

    for(const auto& extension : availableExtensions)
        for(unsigned int i = 0; i < requiredExtensions.size(); i++) {
            std::string exName = extension.extensionName;
            if (exName == requiredExtensions[i])
                requiredExtensionCount--;
        }

    return requiredExtensionCount == 0;
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    for(const auto& availableFormat : availableFormats) {
        if(availableFormat.format == VK_FORMAT_R8G8B8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) { //предпочтительный формат
            return availableFormat;
        }
    }
    return availableFormats[0]; //первый попавшийся формат
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    for(const auto& availablePresentMode : availablePresentModes) {
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) //тройная буферизация
            return availablePresentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR; //двойная буферизация
}

VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilitiesKhr, int &width, int &height) { //размеры фреймбуфера окна

    VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
    };

    actualExtent.width = std::clamp(actualExtent.width, capabilitiesKhr.minImageExtent.width, capabilitiesKhr.maxImageExtent.width);
    actualExtent.height = std::clamp(actualExtent.height, capabilitiesKhr.minImageExtent.height, capabilitiesKhr.maxImageExtent.height);

    return actualExtent;
}

bool isDeviceSuitable (VkPhysicalDevice &device) {
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);
    std::cout << "Имя устройства: " << props.deviceName << std::endl;
    std::cout << "Производитель: " << props.vendorID << std::endl;

    return props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader;
}

QueueFamilyIndices findQueueFamilies(VkPhysicalDevice &device, VkSurfaceKHR &surface) {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector <VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyProperties.data());

    int i = 0;
    for (const auto& queueFamilyProperty : queueFamilyProperties) {
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

        if(presentSupport)
            indices.presentFamily = i;

        if(queueFamilyProperty.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        if(indices.isComplete())
            break;

        i++;
    }

    return indices;
}

void Application::baseInit()
{
    uint32_t glfwExCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExCount);
    std::cout << glfwExCount << " glfwExCount\n";
    for(int i = 0; i < glfwExCount; i++)
        std::cout << glfwExtensions[i] << std::endl;

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.enabledExtensionCount = glfwExCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

    if(vkCreateInstance(&instanceInfo, nullptr, &_instance) != VK_SUCCESS)
        throw std::runtime_error("Невозможно создать экземпляр");

    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::cout << extensionCount << " расширения\n";

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    for(auto& ext : extensions)
        std::cout << ext.extensionName << std::endl;
}

void Application::physicalDeviceInit()
{
    VkPhysicalDevice phDevice = nullptr;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
    std::cout << deviceCount << " устройств\n";

    std::vector<VkPhysicalDevice> phDevices(deviceCount);
    vkEnumeratePhysicalDevices(_instance, &deviceCount, phDevices.data());

    for(auto& dev : phDevices)
    {
        if(isDeviceSuitable(dev))
        {
            phDevice = dev;
            break;
        }
    }
    if(phDevice == nullptr)
        throw std::runtime_error("Физическое устройство непригодно для использования!");

    _physicalDevice = phDevice;
}
void Application::logicalDeviceInit() {
    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set <uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    float queuePriority = 1.0f;
    for(uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures physicalDeviceFeatures {};

    const std::vector<const char*> deviceExtensions = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    VkDeviceCreateInfo deviceCreateInfo {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data(); //сообщаем логическому устройству об очередях
    deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

    deviceCreateInfo.enabledLayerCount = 0; //мы не используем слои проверки

    if(!checkDeviceExtensionSupport(_physicalDevice, deviceExtensions))
        throw std::runtime_error("Не найдено расширение swap chain");

    bool swapChainAdequate = false;
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice, _surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    if(!swapChainAdequate)
        throw std::runtime_error("Нету поддержки цепочки обмена");

    deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if(vkCreateDevice(_physicalDevice, &deviceCreateInfo, nullptr, &_device) != VK_SUCCESS)
        throw std::runtime_error("Не удалось создать логическое устройство!");

    vkGetDeviceQueue(_device, indices.graphicsFamily.value(), 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, indices.presentFamily.value(), 0, &_presentQueue);
}

void Application::swapChainInit() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(_physicalDevice, _surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, extentWidth, extentHeight);
}

void Application::init(Window& window)
{
    baseInit();
    physicalDeviceInit();

    if(!glfwVulkanSupported())
    throw std::runtime_error("glfw не поддерживает Vulkan");

    if(glfwCreateWindowSurface(_instance, window.getWindow(), nullptr, &_surface) != VK_SUCCESS)
        throw std::runtime_error("Невозможно получить поверхность окна");

    glfwGetFramebufferSize(window.getWindow(), &extentWidth, &extentHeight);

    logicalDeviceInit();
    swapChainInit();
}

Application::~Application()
{
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
    std::cout << "Приложение уничтожено\n";
}