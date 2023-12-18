#pragma once

#include <optional>
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

    int extentWidth, extentHeight;
};