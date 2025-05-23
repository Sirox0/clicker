#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "vkInit.h"
#include "vkFunctions.h"
#include "game.h"

void garbageCreate(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 fenceCount, VkFence* fences) {
    VkCommandBufferAllocateInfo garbageCmdBuffersInfo = {};
    garbageCmdBuffersInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    garbageCmdBuffersInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    garbageCmdBuffersInfo.commandPool = vkglobals.shortCommandPool;
    garbageCmdBuffersInfo.commandBufferCount = cmdBufferCount;

    VK_ASSERT(vkAllocateCommandBuffers(vkglobals.device, &garbageCmdBuffersInfo, cmdBuffers), "failed to allocate command buffers\n");

    VkFenceCreateInfo garbageFenceInfo = {};
    garbageFenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    for (u32 i = 0; i < fenceCount; i++) {
        VK_ASSERT(vkCreateFence(vkglobals.device, &garbageFenceInfo, NULL, &fences[i]), "failed to create fence\n");
    }
}

void garbageWaitAndDestroy(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 bufferCount, buffer_t* buffers, u32 fenceCount, VkFence* fences) {
    vkWaitForFences(vkglobals.device, fenceCount, fences, VK_TRUE, 0xFFFFFFFFFFFFFFFF);
    for (u32 i = 0; i < fenceCount; i++) {
        vkDestroyFence(vkglobals.device, fences[i], NULL);
    }
    vkFreeCommandBuffers(vkglobals.device, vkglobals.shortCommandPool, cmdBufferCount, cmdBuffers);
    for (u32 i = 0; i < bufferCount; i++) {
        vkDestroyBuffer(vkglobals.device, buffers[i].buffer, NULL);
        vkFreeMemory(vkglobals.device, buffers[i].mem, NULL);
    }
}