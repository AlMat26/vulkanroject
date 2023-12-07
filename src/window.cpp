#include "window.h"
#include <GLFW/glfw3.h>
#include <iostream>

void Window::init()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(1280, 720, "vulkanproj", nullptr, nullptr);
}

void Window::pollEvents()
{
    glfwPollEvents();
}

void Window::swapBuffers()
{
    glfwSwapBuffers(window);
}

bool Window::isShouldClose()
{
    return glfwWindowShouldClose(window);
}

Window::~Window()
{
    glfwDestroyWindow(window);
    glfwTerminate();
}

GLFWwindow* Window::getWindow() {
    return window;
}
