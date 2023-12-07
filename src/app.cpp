#include "app.h"

#include <iostream>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

void Application::baseInit()
{
    uint32_t glfwExCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExCount);
    std::cout << glfwExCount << " glfwExCount\n";

    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.enabledExtensionCount = glfwExCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;

    if(vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS)
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
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    std::cout << deviceCount << " устройств\n";

    std::vector<VkPhysicalDevice> phDevices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, phDevices.data());

    for(auto& dev : phDevices)
    {
        VkPhysicalDeviceProperties props;
        VkPhysicalDeviceFeatures features;

        vkGetPhysicalDeviceProperties(dev, &props);
        vkGetPhysicalDeviceFeatures(dev, &features);
        std::cout << props.deviceName << std::endl;

        if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && features.geometryShader)
        {
            phDevice = dev;
            break;
        }
    }

    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(phDevice, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phDevice, &queueFamilyCount, queueFamilies.data());

    uint32_t graphicsFamilyIndex;
    for(auto& queueFamily : queueFamilies)
    {
        static int i = 0;

        if(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            std::cout << "семейство очередей для графики найдено!" << std::endl;
            graphicsFamilyIndex = i;
        }
        i++;
    }

    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = graphicsFamilyIndex;
    queueCreateInfo.queueCount = 1;

    float queuePriority = 1.0;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    VkDeviceCreateInfo deviceCreateInfo{};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.queueCreateInfoCount = 1;

    VkPhysicalDeviceFeatures features;

    deviceCreateInfo.pEnabledFeatures = &features;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.enabledLayerCount = 0;

    if(vkCreateDevice(phDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Невозможно создать устройство");

    vkGetDeviceQueue(device, graphicsFamilyIndex, 0, &queue);
}

void Application::init(Window& window)
{
    baseInit();
    physicalDeviceInit();

    if(!glfwVulkanSupported())
        throw std::runtime_error("glfw не поддерживает Vulkan");

    if(glfwCreateWindowSurface(instance, window.getWindow(), nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Невозможно получить поверхность окна");
}

Application::~Application()
{
    vkDestroyDevice(device, nullptr);
    vkDestroyInstance(instance, nullptr);
    std::cout << "Приложение уничтожено\n";
}