#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "game.h"

void createTextureFromMemory(void* data, i32 w, i32 h, texture_t* pTexture, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer, VkFence garbageFence);
void createTexture(const char* path, i32* pW, i32* pH, texture_t* pTexture, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer, VkFence garbageFence);

#endif