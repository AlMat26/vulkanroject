#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include "window.h"


class Application 
{
public:
    void init(Window&);
    ~Application();
private:
    void baseInit();
    void physicalDeviceInit();
    void logicalDeviceInit();
    void swapChainInit();

    VkInstance _instance;
    VkPhysicalDevice _physicalDevice;
    VkDevice _device;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;

    VkSurfaceKHR _surface;

    VkSwapchainKHR _swapchain;
    std::vector<VkImage> _swapchainImages; //хранит дескрипторы изображений своп чейна
    VkFormat _swapchainImageFormat;
    VkExtent2D _swapchainExtent;

    int extentWidth, extentHeight;
};