#include <vulkan/vulkan.h>
#include <stb/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "game.h"

void createTextureFromMemory(void* data, i32 w, i32 h, i32 c, texture_t* pTexture, VkFormat textureFormat, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.extent = (VkExtent3D){w, h, 1};
    imageInfo.format = textureFormat;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_ASSERT(vkCreateImage(vkglobals.device, &imageInfo, NULL, &pTexture->image), "failed to create image\n");

    VkMemoryRequirements imageMemReq;
    vkGetImageMemoryRequirements(vkglobals.device, pTexture->image, &imageMemReq);

    VkMemoryAllocateInfo starMemAllocInfo = {};
    starMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    starMemAllocInfo.allocationSize = imageMemReq.size;
    starMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(imageMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VK_ASSERT(vkAllocateMemory(vkglobals.device, &starMemAllocInfo, NULL, &pTexture->mem), "failed to allocate memory\n");

    VK_ASSERT(vkBindImageMemory(vkglobals.device, pTexture->image, pTexture->mem, 0), "failed to binf image memory");

    {
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.size = imageMemReq.size;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VK_ASSERT(vkCreateBuffer(vkglobals.device, &bufferInfo, NULL, &garbageBuffer->buffer), "failed to create buffer\n");

        VkMemoryRequirements bufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, garbageBuffer->buffer, &bufferMemReq);

        VkMemoryAllocateInfo bufferMemAllocInfo = {};
        bufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        bufferMemAllocInfo.allocationSize = bufferMemReq.size;
        bufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(bufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

        VK_ASSERT(vkAllocateMemory(vkglobals.device, &bufferMemAllocInfo, NULL, &garbageBuffer->mem), "failed to allocate memory\n");

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, garbageBuffer->buffer, garbageBuffer->mem, 0), "failed to bind buffer memory\n");

        {
            void* garbageBufferRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, garbageBuffer->mem, 0, bufferMemReq.size, 0, &garbageBufferRaw), "failed to map device memory\n");

            memcpy(garbageBufferRaw, data, w * h * c);
            
            VkMappedMemoryRange flushMemRange = {};
            flushMemRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            flushMemRange.memory = garbageBuffer->mem;
            flushMemRange.size = bufferMemReq.size;
            flushMemRange.offset = 0;

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &flushMemRange), "failed to flush device memory\n");

            vkUnmapMemory(vkglobals.device, garbageBuffer->mem);
        }

        {
            VkImageMemoryBarrier imageBarrier = {};
            imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            imageBarrier.image = pTexture->image;
            imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            imageBarrier.subresourceRange.baseArrayLayer = 0;
            imageBarrier.subresourceRange.layerCount = 1;
            imageBarrier.subresourceRange.baseMipLevel = 0;
            imageBarrier.subresourceRange.levelCount = 1;
            imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            imageBarrier.srcAccessMask = 0;
            imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

            vkCmdPipelineBarrier(garbageCmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageBarrier);

            VkBufferImageCopy copyInfo = {};
            copyInfo.imageExtent = (VkExtent3D){w, h, 1};
            copyInfo.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            copyInfo.imageSubresource.baseArrayLayer = 0;
            copyInfo.imageSubresource.layerCount = 1;
            copyInfo.imageSubresource.mipLevel = 0;

            vkCmdCopyBufferToImage(garbageCmdBuffer, garbageBuffer->buffer, pTexture->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);

            imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            vkCmdPipelineBarrier(garbageCmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_DEPENDENCY_BY_REGION_BIT, 0, NULL, 0, NULL, 1, &imageBarrier);
        }
    }

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.image = pTexture->image;
    viewInfo.format = textureFormat;
    viewInfo.components = (VkComponentMapping){VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;

    VK_ASSERT(vkCreateImageView(vkglobals.device, &viewInfo, NULL, &pTexture->view), "failed to create image view\n");

    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;

    VK_ASSERT(vkCreateSampler(vkglobals.device, &samplerInfo, NULL, &pTexture->sampler), "failed to create sampler\n");
}

void createTexture(const char* path, i32* pW, i32* pH, i32* pC, texture_t* pTexture, VkFormat textureFormat, buffer_t* garbageBuffer, VkCommandBuffer garbageCmdBuffer) {
    stbi_uc* imageData = stbi_load(path, pW, pH, pC, STBI_rgb_alpha);
    if (!imageData) {
        printf("failed to load an image\n");
        exit(1);
    }

    createTextureFromMemory(imageData, *pW, *pH, *pC, pTexture, textureFormat, garbageBuffer, garbageCmdBuffer);

    stbi_image_free(imageData);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags memoryProps, buffer_t* pBuffer) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.usage = usage;
    bufferInfo.size = size;

    VK_ASSERT(vkCreateBuffer(vkglobals.device, &bufferInfo, NULL, &pBuffer->buffer), "failed to create buffer\n");

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(vkglobals.device, pBuffer->buffer, &memReq);

    VkMemoryAllocateInfo bufferMemAllocInfo = {};
    bufferMemAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    bufferMemAllocInfo.allocationSize = memReq.size;
    bufferMemAllocInfo.memoryTypeIndex = getMemoryTypeIndex(memReq.memoryTypeBits, memoryProps);

    VK_ASSERT(vkAllocateMemory(vkglobals.device, &bufferMemAllocInfo, NULL, &pBuffer->mem), "failed to allocate memory\n");

    VK_ASSERT(vkBindBufferMemory(vkglobals.device, pBuffer->buffer, pBuffer->mem, 0), "failed to bind buffer memory\n");
}