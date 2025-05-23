#ifndef GARBAGE_H
#define GARBAGE_H

#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "game.h"

void garbageCreate(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 fenceCount, VkFence* fences);
void garbageWaitAndDestroy(u32 cmdBufferCount, VkCommandBuffer* cmdBuffers, u32 bufferCount, buffer_t* buffers, u32 fenceCount, VkFence* fences);

#endif