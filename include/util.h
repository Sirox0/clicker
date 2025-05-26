#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>
#include <stb/stb_image.h>

#include "numtypes.h"
#include "game.h"

void createTextureFromMemory(void* data, i32 w, i32 h, i32 c, texture_t* pTexture, VkFormat textureFormat, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer);
void createTexture(const char* path, i32* pW, i32* pH, i32* pC, texture_t* pTexture, VkFormat textureFormat, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer);
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProps, buffer_t* pBuffer);

#endif