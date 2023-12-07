#pragma once

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

    VkInstance instance;
    VkDevice device;
    VkQueue queue;

    VkSurfaceKHR surface;
};