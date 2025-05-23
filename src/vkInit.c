#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "sdl.h"
#include "mathext.h"
#include "pipeline.h"
#include "game.h"

vulkan_globals_t vkglobals = {};

u32 getMemoryTypeIndex(u32 filter, VkMemoryPropertyFlags props) {
    for (u32 i = 0; i < vkglobals.deviceMemoryProperties.memoryTypeCount; i++) {
        if (filter & (1 << i) && ((vkglobals.deviceMemoryProperties.memoryTypes[i].propertyFlags & props) == props)) {
            return i;
        }
    }
    printf("failed to find required memory type\n");
    exit(1);
}

VkShaderModule createShaderModuleFromFile(char* path) {
    FILE* f = fopen(path, "rb");
    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* shaderCode = (char*)malloc(sizeof(char) * fsize);
    fread(shaderCode, fsize, 1, f);
    fclose(f);

    VkShaderModule module;
    VkShaderModuleCreateInfo moduleInfo = {};
    moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleInfo.codeSize = fsize;
    moduleInfo.pCode = (u32*)shaderCode;

    VK_ASSERT(vkCreateShaderModule(vkglobals.device, &moduleInfo, NULL, &module), "failed to create shader module\n");
    free(shaderCode);
    return module;
}

void vkInit() {
    loadVulkanLoaderFunctions();

    #define DEVICE_EXTENSION_COUNT 1
    const char* deviceExtensions[DEVICE_EXTENSION_COUNT] = {"VK_KHR_swapchain"};

    #ifdef VALIDATION
        #define vkLayerCount 1
        const char* vkLayers[] = {"VK_LAYER_KHRONOS_validation"};
    #else
        #define vkLayerCount 0
        const char* vkLayers[] = NULL;
    #endif

    {
        const char** instanceExtensions = NULL;
        u32 instanceExtensionCount = 0;
        sdlQueryInstanceExtensions(&instanceExtensionCount, &instanceExtensions);

        VkInstanceCreateInfo instanceInfo = {};
        instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        instanceInfo.enabledLayerCount = vkLayerCount;
        instanceInfo.ppEnabledLayerNames = vkLayers;
        instanceInfo.enabledExtensionCount = instanceExtensionCount;
        instanceInfo.ppEnabledExtensionNames = instanceExtensions;

        VK_ASSERT(vkCreateInstance(&instanceInfo, NULL, &vkglobals.instance), "failed to create vulkan instance\n");
    }

    loadVulkanInstanceFunctions(vkglobals.instance);

    sdlCreateSurface(vkglobals.instance, &vkglobals.surface);

    {
        u32 physicalDeviceCount;
        vkEnumeratePhysicalDevices(vkglobals.instance, &physicalDeviceCount, NULL);
        VkPhysicalDevice physicalDevices[physicalDeviceCount];
        vkEnumeratePhysicalDevices(vkglobals.instance, &physicalDeviceCount, physicalDevices);

        u8 foundDevice = 0;
        for (u32 i = 0; i < physicalDeviceCount; i++) {
            u32 extensionPropertyCount;
            vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCount, NULL);
            VkExtensionProperties extensionProperties[extensionPropertyCount];
            vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCount, extensionProperties);

            u8 foundExts;
            for (u32 requiredExt = 0; requiredExt < DEVICE_EXTENSION_COUNT; requiredExt++) {
                foundExts = 0;

                for (u32 availableExt = 0; availableExt < extensionPropertyCount; availableExt++) {
                    if (strcmp(deviceExtensions[requiredExt], extensionProperties[availableExt].extensionName) == 0) {
                        foundExts = 1;
                    }
                }

                if (!foundExts) {
                    break;
                }
            }
            if (!foundExts) continue;

            u32 queueFamilyPropertyCount;
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, NULL);
            VkQueueFamilyProperties queueFamilyProperties[queueFamilyPropertyCount];
            vkGetPhysicalDeviceQueueFamilyProperties(physicalDevices[i], &queueFamilyPropertyCount, queueFamilyProperties);

            u8 foundQueueFamily = 0;
            for (u32 queueFamilyIndex = 0; queueFamilyIndex < queueFamilyPropertyCount; queueFamilyIndex++) {
                if (queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT && queueFamilyProperties[queueFamilyIndex].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                    VkBool32 canPresent;
                    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevices[i], queueFamilyIndex, vkglobals.surface, &canPresent);

                    if (canPresent == VK_TRUE) {
                        vkglobals.queueFamilyIndex = queueFamilyIndex;
                        foundQueueFamily = 1;
                        break;
                    }
                }
            }
            if (!foundQueueFamily) continue;

            vkglobals.surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;

            u32 surfaceFormatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkglobals.surface, &surfaceFormatCount, NULL);
            if (surfaceFormatCount == 0) continue;
            VkSurfaceFormatKHR surfaceFormats[surfaceFormatCount];
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevices[i], vkglobals.surface, &surfaceFormatCount, surfaceFormats);

            vkglobals.surfaceFormat = surfaceFormats[0];

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevices[i], vkglobals.surface, &vkglobals.surfaceCapabilities);
            vkGetPhysicalDeviceMemoryProperties(physicalDevices[i], &vkglobals.deviceMemoryProperties);
            vkGetPhysicalDeviceProperties(physicalDevices[i], &vkglobals.deviceProperties);

            vkglobals.physicalDevice = physicalDevices[i];
            foundDevice = 1;
        }

        if (!foundDevice) {
            printf("failed to find a suitable vulkan device\n");
            exit(1);
        }
    }

    {
        f32 priorities[] = {1.0f};

        VkDeviceQueueCreateInfo deviceQueueInfo = {};
        deviceQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueInfo.pQueuePriorities = priorities;
        deviceQueueInfo.queueCount = 1;
        deviceQueueInfo.queueFamilyIndex = vkglobals.queueFamilyIndex;

        VkDeviceCreateInfo deviceInfo = {};
        deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceInfo.enabledLayerCount = vkLayerCount;
        deviceInfo.ppEnabledLayerNames = vkLayers;
        deviceInfo.enabledExtensionCount = DEVICE_EXTENSION_COUNT;
        deviceInfo.ppEnabledExtensionNames = deviceExtensions;
        deviceInfo.pEnabledFeatures = NULL;
        deviceInfo.queueCreateInfoCount = 1;
        deviceInfo.pQueueCreateInfos = &deviceQueueInfo;

        VK_ASSERT(vkCreateDevice(vkglobals.physicalDevice, &deviceInfo, NULL, &vkglobals.device), "failed to create vulkan logical device\n");
    }

    loadVulkanDeviceFunctions(vkglobals.device);

    vkGetDeviceQueue(vkglobals.device, vkglobals.queueFamilyIndex, 0, &vkglobals.queue);

    {
        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        commandPoolInfo.queueFamilyIndex = vkglobals.queueFamilyIndex;

        VK_ASSERT(vkCreateCommandPool(vkglobals.device, &commandPoolInfo, NULL, &vkglobals.commandPool), "failed to create command pool\n");

        commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VK_ASSERT(vkCreateCommandPool(vkglobals.device, &commandPoolInfo, NULL, &vkglobals.shortCommandPool), "failed to create command pool\n");
    }

    {
        VkCommandBufferAllocateInfo cmdBufferInfo = {};
        cmdBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdBufferInfo.commandBufferCount = 1;
        cmdBufferInfo.commandPool = vkglobals.commandPool;

        VK_ASSERT(vkAllocateCommandBuffers(vkglobals.device, &cmdBufferInfo, &vkglobals.cmdBuffer), "failed to allocate command buffer\n");
    }

    // 0xFFFFFFFF means that the extent is defined by the swapchain
    if (vkglobals.surfaceCapabilities.currentExtent.width != 0xFFFFFFFF) {
        vkglobals.swapchainExtent = vkglobals.surfaceCapabilities.currentExtent;
    } else {
        u32 w;
        u32 h;
        sdlQueryWindowResolution(&w, &h);

        vkglobals.swapchainExtent.width = w;
        vkglobals.swapchainExtent.height = h;
        clamp(&vkglobals.swapchainExtent.width, vkglobals.surfaceCapabilities.minImageExtent.width, vkglobals.surfaceCapabilities.maxImageExtent.width);
        clamp(&vkglobals.swapchainExtent.height, vkglobals.surfaceCapabilities.minImageExtent.height, vkglobals.surfaceCapabilities.maxImageExtent.height);
    }

    {
        u32 swapchainImageCount;
        if (vkglobals.surfaceCapabilities.minImageCount + 1 <= vkglobals.surfaceCapabilities.maxImageCount) {
            swapchainImageCount = vkglobals.surfaceCapabilities.minImageCount + 1;
        } else {
            swapchainImageCount = vkglobals.surfaceCapabilities.minImageCount;
        }

        VkCompositeAlphaFlagsKHR swapchainCompositeAlpha;
        if (vkglobals.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR) {
            swapchainCompositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
        } else {
            swapchainCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        VkSwapchainCreateInfoKHR swapchainInfo = {};
        swapchainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        swapchainInfo.clipped = VK_TRUE;
        swapchainInfo.compositeAlpha = swapchainCompositeAlpha;
        swapchainInfo.imageArrayLayers = 1;
        swapchainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        swapchainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainInfo.imageFormat = vkglobals.surfaceFormat.format;
        swapchainInfo.imageColorSpace = vkglobals.surfaceFormat.colorSpace;
        swapchainInfo.imageExtent = vkglobals.swapchainExtent;
        swapchainInfo.minImageCount = swapchainImageCount;
        swapchainInfo.presentMode = vkglobals.surfacePresentMode;
        swapchainInfo.preTransform = vkglobals.surfaceCapabilities.currentTransform;
        swapchainInfo.surface = vkglobals.surface;

        VK_ASSERT(vkCreateSwapchainKHR(vkglobals.device, &swapchainInfo, NULL, &vkglobals.swapchain), "failed to create swapchain\n");
    }

    vkGetSwapchainImagesKHR(vkglobals.device, vkglobals.swapchain, &vkglobals.swapchainImageCount, NULL);

    {
        VkImage swapchainImages[vkglobals.swapchainImageCount];
        vkGetSwapchainImagesKHR(vkglobals.device, vkglobals.swapchain, &vkglobals.swapchainImageCount, swapchainImages);

        // the buffer stores both swapchain image views and swapchain framebuffers
        void* buffer = malloc(sizeof(VkImageView) * vkglobals.swapchainImageCount + sizeof(VkFramebuffer) * vkglobals.swapchainImageCount);
        vkglobals.swapchainImageViews = (VkImageView*)buffer;
        vkglobals.swapchainFramebuffers = (VkFramebuffer*)(buffer + vkglobals.swapchainImageCount * sizeof(VkImageView));

        for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
            VkImageViewCreateInfo viewInfo = {};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.image = swapchainImages[i];
            viewInfo.format = vkglobals.surfaceFormat.format;
            viewInfo.components = (VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.layerCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;

            VK_ASSERT(vkCreateImageView(vkglobals.device, &viewInfo, NULL, &vkglobals.swapchainImageViews[i]), "failed to create image view\n");
        }
    }
}

void vkQuit() {
    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        vkDestroyImageView(vkglobals.device, vkglobals.swapchainImageViews[i], NULL);
    }
    free(vkglobals.swapchainImageViews);
    vkDestroySwapchainKHR(vkglobals.device, vkglobals.swapchain, NULL);
    vkDestroyCommandPool(vkglobals.device, vkglobals.shortCommandPool, NULL);
    vkDestroyCommandPool(vkglobals.device, vkglobals.commandPool, NULL);
    vkDestroyDevice(vkglobals.device, NULL);
    sdlDestroySurface(vkglobals.instance, vkglobals.surface);
    vkDestroyInstance(vkglobals.instance, NULL);
}