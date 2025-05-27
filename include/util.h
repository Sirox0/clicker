#ifndef UTIL_H
#define UTIL_H

#include <vulkan/vulkan.h>
#include <stb/stb_image.h>

#include "numtypes.h"
#include "game.h"

void copyBufferToImage(VkCommandBuffer cmdBuffer, VkBuffer buffer, VkImage image, u32 w, u32 h, VkImageLayout oldLayout, VkImageLayout newLayout);
void allocateMemory(VkDeviceMemory* pMem, VkDeviceSize size, u32 memoryTypeIndex);
void createBuffer(VkBuffer* pBuffer, VkBufferUsageFlags usage, VkDeviceSize size);
void beginCreateTexture(texture_t* pTexture, i32 w, i32 h, VkFormat textureFormat);
void endCreateTexture(texture_t* pTexture, VkFormat textureFormat);

#endif