#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

class Window 
{
public:
    void init();
    void pollEvents();
    void swapBuffers();
    bool isShouldClose();
    GLFWwindow* getWindow();
    ~Window();
private:
    GLFWwindow* window;
};