#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "game.h"

void createTexture(const char* path, i32* pW, i32* pH, texture_t* pTexture, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer, VkFence garbageFence);

#endif