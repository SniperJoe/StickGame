#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <map>
#include <optional>
#include <set>
#include <cstdint> // Necessary for UINT32_MAX
#include <algorithm> // Necessary for std::min/std::max
#include "Families.h"
#include "CreateCommandPool.h"
#include "PipelineManager.h"
#include "CircleVertexInput.h"

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 800;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SHADER_NON_SEMANTIC_INFO_EXTENSION_NAME
};

#ifdef NDEBUG
    const bool enableValidationLayers = false;
#else
    const bool enableValidationLayers = true;
#endif

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    }
    else {
        return VkResult::VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
class HelloTriangleApplication {
public:
    void run() {
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
    }

private:
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkQueue presentQueue;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    VkRenderPass renderPass;
    //VkPipeline graphicsPipeline;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkCommandPool graphicsCommandPool;
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    size_t currentFrame = 0;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> imagesInFlight;
    bool framebufferResized = false;
    uint32_t linesInCircle = 53;
    PipelineManager* pipelineManager;

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        //glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        this->window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
        glfwSetWindowUserPointer(this->window, this);
        glfwSetFramebufferSizeCallback(this->window, this->framebufferResizeCallback);

    }
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }
    void initVulkan() {
        this->createInstance();
        this->setupDebugMessenger();
        this->createSurface();
        this->pickPhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createGraphicsPipeline();
        this->createFramebuffers();
        this->createCommandPools();
        this->createCommandBuffers();
        this->createSyncObjects();
    }
    
    void createSyncObjects() {
        this->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        this->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        this->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        this->imagesInFlight.resize(this->swapChainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->imageAvailableSemaphores[i]) != VkResult::VK_SUCCESS ||
                vkCreateSemaphore(this->device, &semaphoreInfo, nullptr, &this->renderFinishedSemaphores[i]) != VkResult::VK_SUCCESS ||
                vkCreateFence(this->device, &fenceInfo, nullptr, &this->inFlightFences[i]) != VkResult::VK_SUCCESS) {

                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    void createCommandBuffers() {
        this->commandBuffers.resize(this->swapChainFramebuffers.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = this->graphicsCommandPool;
        allocInfo.level = VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

        if (vkAllocateCommandBuffers(this->device, &allocInfo, this->commandBuffers.data()) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }

        for (size_t i = 0; i < this->commandBuffers.size(); i++) {
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional
            beginInfo.pInheritanceInfo = nullptr; // Optional

            if (vkBeginCommandBuffer(this->commandBuffers[i], &beginInfo) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to begin recording command buffer!");
            }

            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = this->renderPass;
            renderPassInfo.framebuffer = this->swapChainFramebuffers[i];
            renderPassInfo.renderArea.offset = { 0, 0 };
            renderPassInfo.renderArea.extent = this->swapChainExtent;
            VkClearValue clearColor = { {{1.0f, 1.0f, 1.0f, 1.0f}} };
            renderPassInfo.clearValueCount = 1;
            renderPassInfo.pClearValues = &clearColor;

            vkCmdBeginRenderPass(this->commandBuffers[i], &renderPassInfo, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
            this->pipelineManager->writeCommands(this->commandBuffers[i]);


            vkCmdDraw(this->commandBuffers[i], this->linesInCircle+1, 1, 0, 0);
            vkCmdEndRenderPass(this->commandBuffers[i]);
            if (vkEndCommandBuffer(this->commandBuffers[i]) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to record command buffer!");
            }
        }
    }
    void createCommandPools() {
        auto graphicsFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isGraphicsFamily);

        createCommandPool(this->device, graphicsFamilyIndex.value(), &this->graphicsCommandPool);
    }
    void createFramebuffers() {
        this->swapChainFramebuffers.resize(this->swapChainImageViews.size());

        for (size_t i = 0; i < this->swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                this->swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = this->renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = this->swapChainExtent.width;
            framebufferInfo.height = this->swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(this->device, &framebufferInfo, nullptr, &this->swapChainFramebuffers[i]) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }
    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = this->swapChainImageFormat;
        colorAttachment.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VkAttachmentLoadOp::VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VkAttachmentStoreOp::VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.initialLayout = VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VkImageLayout::VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VkImageLayout::VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VkAccessFlagBits::VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(this->device, &renderPassInfo, nullptr, &this->renderPass) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }
    void createGraphicsPipeline() {
        auto graphicsFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isGraphicsFamily);
        auto transferFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isTransferFamily);

        this->pipelineManager = new PipelineManager(this->physicalDevice, this->device, this->renderPass, transferFamilyIndex.value(), graphicsFamilyIndex.value());

        CircleVertexInput circleVertextInput;
        

        PipelineCreateInfo createInfo{};
        createInfo.extent = this->swapChainExtent;
        createInfo.fragmentShaderModule = "compiled_shaders/shader.frag.spv";
        createInfo.input = &circleVertextInput;
        createInfo.name = "circle";
        createInfo.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        createInfo.vertexShaderModule = "compiled_shaders/shader.vert.spv";


        this->pipelineManager->createPipelines(1, &createInfo);
        this->pipelineManager->writeVertexData(&this->linesInCircle, "circle");
    }
    void createImageViews() {
        this->swapChainImageViews.resize(this->swapChainImages.size());
        for (size_t i = 0; i < this->swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = this->swapChainImages[i];
            createInfo.viewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = this->swapChainImageFormat;
            createInfo.components.r = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VkComponentSwizzle::VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(this->device, &createInfo, nullptr, &this->swapChainImageViews[i]) != VkResult::VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
    }
    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
         
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }
        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = this->surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VkImageUsageFlagBits::VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        auto graphicsFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isGraphicsFamily);
        auto presentFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isPresentFamily);

        uint32_t queueFamilyIndices[] = { graphicsFamilyIndex.value(), presentFamilyIndex.value() };

        if (graphicsFamilyIndex != presentFamilyIndex) {
            createInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            createInfo.imageSharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0; // Optional
            createInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VkCompositeAlphaFlagBitsKHR::VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(this->device, &createInfo, nullptr, &this->swapChain) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, nullptr);
        this->swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(this->device, this->swapChain, &imageCount, this->swapChainImages.data());

        this->swapChainImageFormat = surfaceFormat.format;
        this->swapChainExtent = extent;
    }

    void createSurface() {
        if (glfwCreateWindowSurface(this->instance, this->window, nullptr, &this->surface) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    void createLogicalDevice() {
        auto graphicsFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isGraphicsFamily);
        auto presentFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isPresentFamily);
        auto transferFamilyIndex = this->getFamilyIndex(this->physicalDevice, &isTransferFamily);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { graphicsFamilyIndex.value(), presentFamilyIndex.value(), transferFamilyIndex.value() };
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            float queuePriority = 1.0f;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }
        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo vkDeviceCreateInfo{};
        vkDeviceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        vkDeviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
        vkDeviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());;

        vkDeviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        vkDeviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        vkDeviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            vkDeviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            vkDeviceCreateInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            vkDeviceCreateInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(this->physicalDevice, &vkDeviceCreateInfo, nullptr, &this->device) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }
        vkGetDeviceQueue(this->device, graphicsFamilyIndex.value(), 0, &this->graphicsQueue);
        vkGetDeviceQueue(this->device, presentFamilyIndex.value(), 0, &this->presentQueue);
    }

    void pickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(this->instance, &deviceCount, devices.data());
        std::cout << "\n\n\nPhysical devices:\n";

        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;

        for (const auto& device : devices) {
            vkGetPhysicalDeviceProperties(device, &deviceProperties);
            vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

            std::cout << '\t' << deviceProperties.deviceName << '\n';
            //std::cout << "\t\tWide lines are ";
            //if (deviceFeatures.wideLines) {
            //    std::cout << "supported\n";
            //}
            //else {
            //    std::cout << "not supported\n";
            //}
            if (this->isDeviceSuitable(device)) {
                this->physicalDevice = device;
                break;
            }
        }

        if (this->physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(this->window)) {
            glfwPollEvents();
            this->drawFrame();
        }
        vkDeviceWaitIdle(this->device);
    }

    void drawFrame() {
        vkWaitForFences(this->device, 1, &this->inFlightFences[this->currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(this->device, this->swapChain, UINT64_MAX, this->imageAvailableSemaphores[this->currentFrame], VK_NULL_HANDLE, &imageIndex);
        if (result == VkResult::VK_ERROR_OUT_OF_DATE_KHR || result == VkResult::VK_SUBOPTIMAL_KHR) {
            this->recreateSwapChain();
            return;
        }
        else if (result != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        if (this->imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
            vkWaitForFences(device, 1, &this->imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        }
        this->imagesInFlight[imageIndex] = this->inFlightFences[this->currentFrame];

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = { this->imageAvailableSemaphores[this->currentFrame] };
        VkPipelineStageFlags waitStages[] = { VkPipelineStageFlagBits::VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &this->commandBuffers[imageIndex];

        VkSemaphore signalSemaphores[] = { this->renderFinishedSemaphores[this->currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        vkResetFences(this->device, 1, &this->inFlightFences[this->currentFrame]);

        if (vkQueueSubmit(this->graphicsQueue, 1, &submitInfo, this->inFlightFences[this->currentFrame]) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        VkSwapchainKHR swapChains[] = { this->swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr; // Optional
        result = vkQueuePresentKHR(this->presentQueue, &presentInfo);
        if (result == VkResult::VK_ERROR_OUT_OF_DATE_KHR || result == VkResult::VK_SUBOPTIMAL_KHR || this->framebufferResized) {
            this->framebufferResized = false;
            this->recreateSwapChain();
        }
        else if (result != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }
        this->currentFrame = (this->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanup() {
        this->cleanupSwapChain();



        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            vkDestroySemaphore(this->device, this->renderFinishedSemaphores[i], nullptr);
            vkDestroySemaphore(this->device, this->imageAvailableSemaphores[i], nullptr);
            vkDestroyFence(this->device, this->inFlightFences[i], nullptr);
        }

        vkDestroyCommandPool(this->device, this->graphicsCommandPool, nullptr);

        vkDestroyDevice(this->device, nullptr);

        if (enableValidationLayers) {
            DestroyDebugUtilsMessengerEXT(this->instance, this->debugMessenger, nullptr);
        }

        vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
        vkDestroyInstance(this->instance, nullptr);

        glfwDestroyWindow(this->window);

        glfwTerminate();
    }
    void createInstance() {

        if (enableValidationLayers && !this->checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        VkInstanceCreateInfo vkInstanceCreateInfo{};
        vkInstanceCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        vkInstanceCreateInfo.pApplicationInfo = &appInfo;
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            vkInstanceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            vkInstanceCreateInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            vkInstanceCreateInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
        }
        else {
            vkInstanceCreateInfo.enabledLayerCount = 0;
            vkInstanceCreateInfo.pNext = nullptr;
        }
        auto extensions = this->getRequiredExtensions();
        vkInstanceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        vkInstanceCreateInfo.ppEnabledExtensionNames = extensions.data();

        uint32_t availableExtensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, nullptr);
        std::vector<VkExtensionProperties> availableExtensions(availableExtensionCount);
        vkEnumerateInstanceExtensionProperties(nullptr, &availableExtensionCount, availableExtensions.data());
        std::cout << "available extensions:\n";

        std::string availableExtensionNames = "";

        for (const auto& extension : availableExtensions) {
            std::cout << extension.extensionName << '\n';
            availableExtensionNames.append(extension.extensionName);
        }
        std::cout << "\nrequired extensions:\n";

        for (const auto& extension : extensions) {
            std::cout << extension << '\n';
            if (availableExtensionNames.find(extension) == std::string::npos) {
                std::string exceptionString = "extension ";
                exceptionString.append(extension);
                exceptionString.append(" is not available");
                throw std::runtime_error(exceptionString);
            }
        }

        VkResult result = vkCreateInstance(&vkInstanceCreateInfo, nullptr, &this->instance);
        if (result == VkResult::VK_SUCCESS) {
            return;
        }
        throw std::runtime_error("failed to create instance!");
    }

    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        return this->isNeededLayersAvailable(availableLayers);
    }
    bool isNeededLayersAvailable(std::vector<VkLayerProperties> availableLayers) {
        std::cout << "Available layers:\n";
        for (const auto& layerProperties : availableLayers) {
            std::cout << layerProperties.layerName << '\n';
            std::cout << '\t' << layerProperties.description << '\n';
        }
        for (const char* layerName : validationLayers) {
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
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData) {

        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        this->populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(this->instance, &createInfo, nullptr, &debugMessenger) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT 
            | VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT 
            | VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT 
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT 
            | VkDebugUtilsMessageTypeFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    bool isDeviceSuitable(VkPhysicalDevice device) {
        
        auto graphicsFamilyIndex = this->getFamilyIndex(device, &isGraphicsFamily);

        auto presentFamilyIndex = this->getFamilyIndex(device, &isPresentFamily);

        auto transferFamilyIndex = this->getFamilyIndex(device, &isTransferFamily);

        auto extensionsSupported = this->checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return graphicsFamilyIndex.has_value() && presentFamilyIndex.has_value() && transferFamilyIndex.has_value() && swapChainAdequate;
    }

    std::optional<uint32_t> getFamilyIndex(VkPhysicalDevice device, bool(*checker)(VkQueueFamilyProperties, uint32_t, VkPhysicalDevice, VkSurfaceKHR)) {
        std::optional<uint32_t> familyIndex;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        VkQueueFlags implementedFamilies = 0;

        for (int queueFamilyindex = 0; queueFamilyindex < queueFamilies.size(); queueFamilyindex++) {
            if ((*checker)(queueFamilies[queueFamilyindex], queueFamilyindex, device, this->surface)) {
                familyIndex = queueFamilyindex;
                break;
            }
        }
        return familyIndex;
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
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, this->surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, this->surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VkFormat::VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VkColorSpaceKHR::VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        }
        else {
            int width, height;
            glfwGetFramebufferSize(this->window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(this->window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(this->window, &width, &height);
            glfwWaitEvents();
        }
        vkDeviceWaitIdle(this->device);
        this->cleanupSwapChain();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createGraphicsPipeline();
        this->createFramebuffers();
        this->createCommandBuffers();
    }
    void cleanupSwapChain() {
        for (auto framebuffer : this->swapChainFramebuffers) {
            vkDestroyFramebuffer(this->device, framebuffer, nullptr);
        }
        vkFreeCommandBuffers(this->device, this->graphicsCommandPool, static_cast<uint32_t>(this->commandBuffers.size()), this->commandBuffers.data());
        delete this->pipelineManager;
        vkDestroyRenderPass(this->device, this->renderPass, nullptr);

        for (auto imageView : this->swapChainImageViews) {
            vkDestroyImageView(this->device, imageView, nullptr);
        }
        vkDestroySwapchainKHR(this->device, this->swapChain, nullptr);
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}