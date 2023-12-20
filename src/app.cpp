#include "app.h"
#include "loadBinFile.h"

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

VkShaderModule createShaderModule(VkDevice& device, std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Не удалось создать шейдерный модуль!");

    return shaderModule;
}

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

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    // 0 - специальное число, означающее, что максимального количества изображений в swap chain нету
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.minImageCount;

    VkSwapchainCreateInfoKHR swapchainCreateInfo {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = _surface;
    swapchainCreateInfo.imageExtent = extent;
    swapchainCreateInfo.imageArrayLayers = 1; //количество слоев каждого изображения (1 если без стереоскопии)
    swapchainCreateInfo.imageFormat = surfaceFormat.format;
    swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = findQueueFamilies(_physicalDevice, _surface);
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if(indices.graphicsFamily != indices.presentFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; //опционально
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; //опционально
    }

    swapchainCreateInfo.preTransform = swapChainSupport.capabilities.currentTransform; //если не собираемся производить трансформации
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; //не используем альфа канал для смешивания с другими окнами

    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = true; //нас не волнует цвет затемненных пикселей другим окном
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; //ссылка на предыдущий общект цепочки обмена

    if(vkCreateSwapchainKHR(_device, &swapchainCreateInfo, nullptr, &_swapchain) != VK_SUCCESS)
        throw std::runtime_error("Не удалось создать swapchain");

    //получаем дескрипторы изображений цепочки обмена
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, nullptr);
    _swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(_device, _swapchain, &imageCount, _swapchainImages.data());

    _swapchainImageFormat = surfaceFormat.format;
    _swapchainExtent = extent;
}

void Application::imageViewsInit() {
    _swapchainImageViews.resize(_swapchainImages.size());
    for(size_t i = 0; i < _swapchainImages.size(); i++) {

        VkImageViewCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapchainImages[i];

        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapchainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.levelCount = 1;

        if(vkCreateImageView(_device, &createInfo, nullptr, &_swapchainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("не удалось создать представление изображения цепочки обмена");
    }
}

void Application::graphicsPipelineInit() {
    auto vertShaderCode = utils::readFile("../shaders/shader.vert");
    auto fragShaderCode = utils::readFile("../shaders/shader.frag");

    std::cout << vertShaderCode.size() << " байт (вершинный шейдер)" << std::endl;
    std::cout << fragShaderCode.size() << " байт (фрагментный шейдер)" << std::endl;

    VkShaderModule vertShaderModule = createShaderModule(_device, vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(_device, fragShaderCode);

    //вершинный шейдер
    VkPipelineShaderStageCreateInfo vertShaderCreateInfo {};
    vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderCreateInfo.module = vertShaderModule;
    vertShaderCreateInfo.pName = "main"; //определяем точку входа


    //фрагментный шейдер
    VkPipelineShaderStageCreateInfo fragShaderCreateInfo {};
    vertShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    vertShaderCreateInfo.module = fragShaderModule;
    vertShaderCreateInfo.pName = "main"; //определяем точку входа

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderCreateInfo, fragShaderCreateInfo };

    //описываем входные вершины с учётом того, что передавать их не будем
    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0; //вершины
    vertexInputInfo.pVertexBindingDescriptions = nullptr;
    vertexInputInfo.vertexAttributeDescriptionCount = 0; //аттрибуты вершин
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo {};
    inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST; //без переиспользования вершин (EBO)
    inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;


    VkViewport viewport {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(_swapchainExtent.width);
    viewport.height = static_cast<float>(_swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    //обрезает часть фреймбуфера (настроен по размеру на весь холст, без обрезки)
    VkRect2D scissor {}; //ножницы
    scissor.offset = {0, 0};
    scissor.extent = _swapchainExtent;

    //настраиваем параметры, которые не будут статичными
    std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT, //динамическое изменение вьюпорта
            VK_DYNAMIC_STATE_SCISSOR   //динамическое изменение обрезки вьюпорта
    };
    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {};
    dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicStateCreateInfo.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    //viewportState.pViewports = &viewport; /* ----- раскомментировать для не динамического использования ----- */
    viewportState.scissorCount = 1;
    //viewportState.pScissors = &scissor;   /* ----- раскомментировать для не динамического использования ----- */

    VkPipelineRasterizationStateCreateInfo rasterizer {}; //преобразовывает данные из вершинного шейдера во фрагменты
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = false; //прикрепление фрагмента к плоскости обрезания, не отбрасываение
    rasterizer.rasterizerDiscardEnable = false; //геометрия не передается через растеризатор (отключает вывод в фреймбуфер)
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    //rasterizer.lineWidth = 1.0f; /* для любого режима, кроме заполнения (fill) */
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; //отбраковываем задние грани
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; //лицевая сторона по часовой стрелке

    rasterizer.depthBiasEnable = false;
    rasterizer.depthBiasConstantFactor = 0.0f; //опционально (2 поля ниже тоже)
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 0.0f;

    VkPipelineMultisampleStateCreateInfo multisampling {}; //мультисемплинг отключён
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = false;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0; //опционально (3 поля ниже тоже)
    multisampling.pSampleMask = nullptr;
    multisampling.alphaToCoverageEnable = false;
    multisampling.alphaToOneEnable = false;

    VkPipelineColorBlendAttachmentState colorBlendAttachment {}; //смешивание цветов (что есть в фреймбуфере + цвет фрагмента)
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; //опционально если смешивание выключено (5 полей ниже тоже)
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; //коэффициент смешивания равен 1 - альфа канал
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; //альфу не меняем В)
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo {};
    colorBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendCreateInfo.logicOpEnable = VK_FALSE;
    colorBlendCreateInfo.logicOp = VK_LOGIC_OP_COPY;
    colorBlendCreateInfo.attachmentCount = 1;
    colorBlendCreateInfo.pAttachments = &colorBlendAttachment;
    colorBlendCreateInfo.blendConstants[0] = 0.0f; //опционально (3 поля ниже тоже)
    colorBlendCreateInfo.blendConstants[1] = 0.0f;
    colorBlendCreateInfo.blendConstants[2] = 0.0f;
    colorBlendCreateInfo.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 0; //опционально (3 поля ниже тоже)
    pipelineLayoutCreateInfo.pSetLayouts = nullptr;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

    if(vkCreatePipelineLayout(_device, &pipelineLayoutCreateInfo, nullptr, &_pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Не удалось создать VkPipelineLayout!");



    vkDestroyShaderModule(_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(_device, fragShaderModule, nullptr);
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
    imageViewsInit();
    graphicsPipelineInit();
}

Application::~Application()
{
    vkDestroyPipelineLayout(_device, _pipelineLayout, nullptr);

    for(auto& imageView : _swapchainImageViews)
        vkDestroyImageView(_device, imageView, nullptr); //удаление явно созданных image view

    vkDestroySwapchainKHR(_device, _swapchain, nullptr);
    vkDestroyDevice(_device, nullptr);
    vkDestroySurfaceKHR(_instance, _surface, nullptr);
    vkDestroyInstance(_instance, nullptr);
    std::cout << "Приложение уничтожено\n";
}