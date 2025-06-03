#include <vulkan/vulkan.h>
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <stb/stb_image.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "sdl.h"
#include "pipeline.h"
#include "game.h"
#include "util.h"
#include "garbage.h"
#include "config.h"
#include "mathext.h"
#include "dynArray.h"

game_globals_t gameglobals = {};

#define GARBAGE_CMD_BUFFER_NUM 1
#define GARBAGE_BUFFER_NUM 2
#define GARBAGE_BUFFER_MEM_OFFSET_NUM 1
#define GARBAGE_MEM_SIZE_NUM 1
#define GARBAGE_BUFFER_MEMORY_NUM 1
#define GARBAGE_FENCE_NUM 1

#define CHARACTER_PIXEL_SIZE 80

void gameInit() {
    VkCommandBuffer garbageCmdBuffers[GARBAGE_CMD_BUFFER_NUM];
    VkBuffer garbageBuffers[GARBAGE_BUFFER_NUM];
    VkDeviceSize garbageBufferMemOffsets[GARBAGE_BUFFER_MEM_OFFSET_NUM];
    VkDeviceSize garbageMemSizes[GARBAGE_MEM_SIZE_NUM];
    VkDeviceMemory garbageBuffersMem[GARBAGE_BUFFER_MEMORY_NUM];
    VkFence garbageFences[GARBAGE_FENCE_NUM];

    garbageCreate(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_FENCE_NUM, garbageFences);

    FT_Library ftlib;
    FT_Face face;
    {
        FT_ASSERT(FT_Init_FreeType(&ftlib));

        FT_ASSERT(FT_New_Face(ftlib, "assets/fonts/Comic Sans MS.ttf", 0, &face));

        FT_ASSERT(FT_Set_Pixel_Sizes(face, CHARACTER_PIXEL_SIZE, CHARACTER_PIXEL_SIZE));

        u32 w = 0;
        u32 h = 0;

        for (u8 i = 0; i < 10; i++) {
            FT_ASSERT(FT_Load_Glyph(face, FT_Get_Char_Index(face, '0' + i), 0));
            if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) FT_ASSERT(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));

            w += face->glyph->bitmap.width;
            if (face->glyph->bitmap.rows > h) h = face->glyph->bitmap.rows;
        }

        gameglobals.textW = w;
        gameglobals.textH = h;
    }

    {
        i32 starW, starH, starC;
        stbi_uc* imageData = stbi_load("assets/textures/star.png", &starW, &starH, &starC, STBI_rgb_alpha);
        if (!imageData) {
            printf("failed to load an image\n");
            exit(1);
        }

        {
            createBuffer(&garbageBuffers[0], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, starW * starH * starC);
            createBuffer(&garbageBuffers[1], VK_BUFFER_USAGE_TRANSFER_SRC_BIT, gameglobals.textW * gameglobals.textH);

            VkMemoryRequirements memReq0 = {};
            vkGetBufferMemoryRequirements(vkglobals.device, garbageBuffers[0], &memReq0);
            VkMemoryRequirements memReq1 = {};
            vkGetBufferMemoryRequirements(vkglobals.device, garbageBuffers[1], &memReq1);

            VkDeviceSize notAlignedSize = memReq0.size + memReq1.size;
            VkDeviceSize alignCooficient = getAlignCooficientByTwo(memReq0.size, vkglobals.deviceProperties.limits.nonCoherentAtomSize, memReq1.alignment);
            garbageBufferMemOffsets[0] = memReq0.size + alignCooficient;
            garbageMemSizes[0] = notAlignedSize + alignCooficient;

            allocateMemory(&garbageBuffersMem[0], notAlignedSize + alignCooficient, getMemoryTypeIndex(memReq0.memoryTypeBits & memReq1.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

            VK_ASSERT(vkBindBufferMemory(vkglobals.device, garbageBuffers[0], garbageBuffersMem[0], 0), "failed to bind buffer memory\n");
            VK_ASSERT(vkBindBufferMemory(vkglobals.device, garbageBuffers[1], garbageBuffersMem[0], garbageBufferMemOffsets[0]), "failed to bind buffer memory\n");
        }

        {
            beginCreateTexture(&gameglobals.text, gameglobals.textW, gameglobals.textH, VK_FORMAT_R8_UNORM);
            beginCreateTexture(&gameglobals.star, starW, starH, VK_FORMAT_R8G8B8A8_UNORM);

            VkMemoryRequirements textMemReq = {};
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.text.image, &textMemReq);
            VkMemoryRequirements starMemReq = {};
            vkGetImageMemoryRequirements(vkglobals.device, gameglobals.star.image, &starMemReq);

            VkDeviceSize notAlignedSize = textMemReq.size + starMemReq.size;
            VkDeviceSize alignCooficient = getAlignCooficient(textMemReq.size, starMemReq.alignment);
            gameglobals.starMemOffset = textMemReq.size + alignCooficient;

            allocateMemory(&gameglobals.texturesMem, notAlignedSize + alignCooficient, getMemoryTypeIndex(textMemReq.memoryTypeBits & starMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.text.image, gameglobals.texturesMem, 0), "failed to bind image memory\n");
            VK_ASSERT(vkBindImageMemory(vkglobals.device, gameglobals.star.image, gameglobals.texturesMem, gameglobals.starMemOffset), "failed to bind image memory\n");

            endCreateTexture(&gameglobals.text, VK_FORMAT_R8_UNORM);
            endCreateTexture(&gameglobals.star, VK_FORMAT_R8G8B8A8_UNORM);
        }

        {
            VkDeviceSize notAlignedSize = starW * starH * starC;
            VkDeviceSize alignedSize = notAlignedSize + getAlignCooficient(notAlignedSize, vkglobals.deviceProperties.limits.nonCoherentAtomSize);
            if (alignedSize >= garbageMemSizes[0]) alignedSize = VK_WHOLE_SIZE;

            void* garbageBufferRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, garbageBuffersMem[0], 0, alignedSize, 0, &garbageBufferRaw), "failed to map device memory\n");

            memcpy(garbageBufferRaw, imageData, starW * starH * starC);

            VkMappedMemoryRange memoryRange = {};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.size = alignedSize;
            memoryRange.offset = 0;
            memoryRange.memory = garbageBuffersMem[0];

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");

            vkUnmapMemory(vkglobals.device, garbageBuffersMem[0]);

            copyBufferToImage(garbageCmdBuffers[0], garbageBuffers[0], gameglobals.star.image, starW, starH, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        stbi_image_free(imageData);
    }

    {
        u32 offset = 0;
        u8* textTextureBuffer = malloc(sizeof(u8) * gameglobals.textW * gameglobals.textH);
        memset(textTextureBuffer, 0, sizeof(u8) * gameglobals.textW * gameglobals.textH);

        for (u8 i = 0; i < 10; i++) {
            FT_ASSERT(FT_Load_Glyph(face, FT_Get_Char_Index(face, '0' + i), 0));
            if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP) FT_ASSERT(FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL));

            for (u32 y = 0; y < face->glyph->bitmap.rows; y++) {
                for (u32 x = offset; x < (offset + face->glyph->bitmap.width); x++) {
                    u8 gv = face->glyph->bitmap.buffer[y * face->glyph->bitmap.width + x - offset];
                    textTextureBuffer[y * gameglobals.textW + x] = gv;
                }
            }
            gameglobals.cinfos[i].wWindowRelative = (f32)face->glyph->bitmap.width / WINDOW_WIDTH;
            gameglobals.cinfos[i].hWindowRelative = (f32)face->glyph->bitmap.rows / WINDOW_HEIGHT;
            gameglobals.cinfos[i].wTextureRelative = (f32)face->glyph->bitmap.width / gameglobals.textW;
            gameglobals.cinfos[i].hTextureRelative = (f32)face->glyph->bitmap.rows / gameglobals.textH;
            gameglobals.cinfos[i].offset = (f32)offset / gameglobals.textW;
            offset += face->glyph->bitmap.width;
        }

        {
            void* garbageBufferRaw;
            VK_ASSERT(vkMapMemory(vkglobals.device, garbageBuffersMem[0], garbageBufferMemOffsets[0], VK_WHOLE_SIZE, 0, &garbageBufferRaw), "failed to map device memory\n");

            memcpy(garbageBufferRaw, textTextureBuffer, gameglobals.textW * gameglobals.textH);

            VkMappedMemoryRange memoryRange = {};
            memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRange.size = VK_WHOLE_SIZE;
            memoryRange.offset = garbageBufferMemOffsets[0];
            memoryRange.memory = garbageBuffersMem[0];

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &memoryRange), "failed to flush device memory\n");

            vkUnmapMemory(vkglobals.device, garbageBuffersMem[0]);

            copyBufferToImage(garbageCmdBuffers[0], garbageBuffers[1], gameglobals.text.image, gameglobals.textW, gameglobals.textH, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }

        free(textTextureBuffer);

        FT_ASSERT(FT_Done_Face(face));

        FT_ASSERT(FT_Done_FreeType(ftlib));
    }

    {
        VK_ASSERT(vkEndCommandBuffer(garbageCmdBuffers[0]), "failed to end command buffer\n");

        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &garbageCmdBuffers[0];

        VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, garbageFences[0]), "failed to submit command buffer\n");
    }

    {
        createBuffer(&gameglobals.textVertexBuffer, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, sizeof(textVertexData) * 10 * 4);
        createBuffer(&gameglobals.textIndexBuffer, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, sizeof(u16) * 10 * 6);

        VkMemoryRequirements vertexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.textVertexBuffer, &vertexBufferMemReq);
        VkMemoryRequirements indexBufferMemReq;
        vkGetBufferMemoryRequirements(vkglobals.device, gameglobals.textIndexBuffer, &indexBufferMemReq);

        u32 alignCoefficient = getAlignCooficientByTwo(vertexBufferMemReq.size, vkglobals.deviceProperties.limits.nonCoherentAtomSize, indexBufferMemReq.alignment);
        gameglobals.textIndexBufferOffset = vertexBufferMemReq.size + alignCoefficient;
        gameglobals.textBuffersMemorySize = vertexBufferMemReq.size + indexBufferMemReq.size + alignCoefficient;

        allocateMemory(&gameglobals.textBuffersMemory, gameglobals.textBuffersMemorySize, getMemoryTypeIndex(vertexBufferMemReq.memoryTypeBits & indexBufferMemReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.textVertexBuffer, gameglobals.textBuffersMemory, 0), "failed to bind buffer memory\n");
        VK_ASSERT(vkBindBufferMemory(vkglobals.device, gameglobals.textIndexBuffer, gameglobals.textBuffersMemory, vertexBufferMemReq.size + alignCoefficient), "failed to bind buffer memory\n");

        VK_ASSERT(vkMapMemory(vkglobals.device, gameglobals.textBuffersMemory, 0, VK_WHOLE_SIZE, 0, &gameglobals.textBuffersMemoryRaw), "failed to map device memory\n");
    }

    {
        VkAttachmentDescription colorAttachmentDesc = {};
        colorAttachmentDesc.format = vkglobals.surfaceFormat.format;
        colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef = {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass = {};
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.inputAttachmentCount = 0;
        subpass.preserveAttachmentCount = 0;
        subpass.pDepthStencilAttachment = NULL;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkRenderPassCreateInfo renderpassInfo = {};
        renderpassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderpassInfo.dependencyCount = 0;
        renderpassInfo.pDependencies = NULL;
        renderpassInfo.attachmentCount = 1;
        renderpassInfo.pAttachments = &colorAttachmentDesc;
        renderpassInfo.subpassCount = 1;
        renderpassInfo.pSubpasses = &subpass;

        VK_ASSERT(vkCreateRenderPass(vkglobals.device, &renderpassInfo, NULL, &gameglobals.renderpass), "failed to create renderpass\n");
    }

    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = gameglobals.renderpass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &vkglobals.swapchainImageViews[i];
        framebufferInfo.width = vkglobals.swapchainExtent.width;
        framebufferInfo.height = vkglobals.swapchainExtent.height;
        framebufferInfo.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(vkglobals.device, &framebufferInfo, NULL, &vkglobals.swapchainFramebuffers[i]), "failed to create framebuffer\n");
    }

    {
        VkDescriptorPoolSize texturePoolSize = {};
        texturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        texturePoolSize.descriptorCount = 2;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &texturePoolSize;
        poolInfo.maxSets = 2;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, NULL, &gameglobals.descriptorPool), "failed to create descriptor pool\n");
    }

    {
        VkDescriptorSetLayoutBinding textureBinding = {};
        textureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        textureBinding.binding = 0;
        textureBinding.descriptorCount = 1;
        textureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = 1;
        descriptorSetLayoutInfo.pBindings = &textureBinding;

        VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, NULL, &gameglobals.textureDescriptorSetLayout), "failed to create descriptor set layout\n");
    }

    {
        VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
        descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetsInfo.descriptorPool = gameglobals.descriptorPool;
        descriptorSetsInfo.descriptorSetCount = 2;
        descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.textureDescriptorSetLayout, gameglobals.textureDescriptorSetLayout};

        {
            VkDescriptorSet sets[2];

            VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, sets), "failed to allocate descriptor sets\n");

            gameglobals.starDescriptorSet = sets[0];
            gameglobals.textDescriptorSet = sets[1];
        }

        VkDescriptorImageInfo starImageDescriptorInfo = {};
        starImageDescriptorInfo.imageView = gameglobals.star.view;
        starImageDescriptorInfo.sampler = gameglobals.star.sampler;
        starImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkDescriptorImageInfo textImageDescriptorInfo = {};
        textImageDescriptorInfo.imageView = gameglobals.text.view;
        textImageDescriptorInfo.sampler = gameglobals.text.sampler;
        textImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet starTextureDescriptorWrite = {};
        starTextureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        starTextureDescriptorWrite.descriptorCount = 1;
        starTextureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        starTextureDescriptorWrite.dstBinding = 0;
        starTextureDescriptorWrite.dstArrayElement = 0;
        starTextureDescriptorWrite.dstSet = gameglobals.starDescriptorSet;
        starTextureDescriptorWrite.pImageInfo = &starImageDescriptorInfo;

        VkWriteDescriptorSet textTextureDescriptorWrite = {};
        textTextureDescriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        textTextureDescriptorWrite.descriptorCount = 1;
        textTextureDescriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        textTextureDescriptorWrite.dstBinding = 0;
        textTextureDescriptorWrite.dstArrayElement = 0;
        textTextureDescriptorWrite.dstSet = gameglobals.textDescriptorSet;
        textTextureDescriptorWrite.pImageInfo = &textImageDescriptorInfo;

        vkUpdateDescriptorSets(vkglobals.device, 2, (VkWriteDescriptorSet[]){starTextureDescriptorWrite, textTextureDescriptorWrite}, 0, NULL);
    }

    {
        VkPushConstantRange pcRange = {};
        pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcRange.size = sizeof(f32);
        pcRange.offset = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &gameglobals.textureDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pcRange;

        VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, NULL, &gameglobals.starPipelineLayout), "failed to create pipeline layout\n");

        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pSetLayouts = &gameglobals.textureDescriptorSetLayout;

        VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, NULL, &gameglobals.textPipelineLayout), "failed to create pipeline layout\n");


        graphics_pipeline_info_t pipelineInfos[2] = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[0]);
        pipelineInfos[0].stageCount = 2;
        pipelineInfos[0].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[0].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[0].stages[0].module = createShaderModuleFromFile("assets/shaders/star.vert.spv");
        pipelineInfos[0].stages[1].module = createShaderModuleFromFile("assets/shaders/star.frag.spv");
        pipelineInfos[0].layout = gameglobals.starPipelineLayout;
        pipelineInfos[0].renderpass = gameglobals.renderpass;
        pipelineInfos[0].subpass = 0;

        VkVertexInputBindingDescription bindingDesc = {};
        bindingDesc.binding = 0;
        bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        bindingDesc.stride = sizeof(textVertexData);

        VkVertexInputAttributeDescription attributeDesc = {};
        attributeDesc.binding = 0;
        attributeDesc.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDesc.location = 0;
        attributeDesc.offset = offsetof(textVertexData, posuv);

        pipelineFillDefaultGraphicsPipeline(&pipelineInfos[1]);
        pipelineInfos[1].vertexInputState.vertexAttributeDescriptionCount = 1;
        pipelineInfos[1].vertexInputState.pVertexAttributeDescriptions = &attributeDesc;
        pipelineInfos[1].vertexInputState.vertexBindingDescriptionCount = 1;
        pipelineInfos[1].vertexInputState.pVertexBindingDescriptions = &bindingDesc;

        pipelineInfos[1].stageCount = 2;
        pipelineInfos[1].stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfos[1].stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfos[1].stages[0].module = createShaderModuleFromFile("assets/shaders/scoretext.vert.spv");
        pipelineInfos[1].stages[1].module = createShaderModuleFromFile("assets/shaders/scoretext.frag.spv");
        pipelineInfos[1].layout = gameglobals.textPipelineLayout;
        pipelineInfos[1].renderpass = gameglobals.renderpass;
        pipelineInfos[1].subpass = 0;

        {
            VkPipeline pipelines[2];
            pipelineCreateGraphicsPipelines(NULL, 2, pipelineInfos, pipelines);

            gameglobals.starPipeline = pipelines[0];
            gameglobals.textPipeline = pipelines[1];
        }

        vkDestroyShaderModule(vkglobals.device, pipelineInfos[0].stages[0].module, NULL);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[0].stages[1].module, NULL);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[1].stages[0].module, NULL);
        vkDestroyShaderModule(vkglobals.device, pipelineInfos[1].stages[1].module, NULL);
    }

    {
        VkSemaphoreCreateInfo semaphoreInfo = {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, NULL, &gameglobals.swapchainReadySemaphore), "failed to create semaphore\n");
        VK_ASSERT(vkCreateSemaphore(vkglobals.device, &semaphoreInfo, NULL, &gameglobals.renderingDoneSemaphore), "failed to create semaphore\n");

        VkFenceCreateInfo fenceInfo = {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        VK_ASSERT(vkCreateFence(vkglobals.device, &fenceInfo, NULL, &gameglobals.frameFence), "failed to create fence\n");
    }

    gameglobals.curStarScale = 1.0f;
    gameglobals.clickCounter = 0;
    gameglobals.n = 0;

    garbageWaitAndDestroy(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_BUFFER_NUM, garbageBuffers, GARBAGE_BUFFER_MEMORY_NUM, garbageBuffersMem, GARBAGE_FENCE_NUM, garbageFences);
}

void getDigits(int n, u32 idx, u32* buf) {
    if (n == 0) {
        return;
    }

    u32 r = n % 10;
    getDigits(n / 10, idx-1, buf);
    buf[idx] = r;
}

void gameRender() {
    f32 mouseX, mouseY;
    u32 mouse = sdlQueryMouseState(&mouseX, &mouseY);
    if (mouse & SDL_BUTTON_LEFT && !gameglobals.blockInput) {
        gameglobals.clickCounter++;
        gameglobals.blockInput = 1;

        u32 digits[10] = {};
        getDigits(gameglobals.clickCounter, 9, digits);

        gameglobals.n = 0;
        while (digits[gameglobals.n] == 0) gameglobals.n++;

        f32 sx = 0.0f;
        f32 sy = -0.775f;
        if ((10 - gameglobals.n) % 2 == 1) {
            u32 middle = (10 - gameglobals.n - 1) / 2.0f;
            sx -= gameglobals.cinfos[digits[middle]].wWindowRelative / 2.0f;
            for (u32 i = gameglobals.n; i < gameglobals.n + middle; i++) {
                sx -= gameglobals.cinfos[digits[i]].wWindowRelative;
            }
        } else {
            u32 middle = (10 - gameglobals.n) / 2.0f;
            for (u32 i = gameglobals.n; i < gameglobals.n + middle; i++) {
                sx -= gameglobals.cinfos[digits[i]].wWindowRelative;
            }
        }

        f32 sxoffset = 0.0f;
        for (u32 i = gameglobals.n; i < 10; i++) {
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4].posuv[0] = sx + sxoffset;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4].posuv[1] = sy;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4].posuv[2] = gameglobals.cinfos[digits[i]].offset;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4].posuv[3] = 0.0f;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 1].posuv[0] = sx + sxoffset;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 1].posuv[1] = sy + gameglobals.cinfos[digits[i]].hWindowRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 1].posuv[2] = gameglobals.cinfos[digits[i]].offset;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 1].posuv[3] = gameglobals.cinfos[digits[i]].hTextureRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 2].posuv[0] = sx + sxoffset + gameglobals.cinfos[digits[i]].wWindowRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 2].posuv[1] = sy;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 2].posuv[2] = gameglobals.cinfos[digits[i]].offset + gameglobals.cinfos[digits[i]].wTextureRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 2].posuv[3] = 0.0f;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 3].posuv[0] = sx + sxoffset + gameglobals.cinfos[digits[i]].wWindowRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 3].posuv[1] = sy + gameglobals.cinfos[digits[i]].hWindowRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 3].posuv[2] = gameglobals.cinfos[digits[i]].offset + gameglobals.cinfos[digits[i]].wTextureRelative;
            ((textVertexData*)gameglobals.textBuffersMemoryRaw)[(i-gameglobals.n) * 4 + 3].posuv[3] = gameglobals.cinfos[digits[i]].hTextureRelative;

            sxoffset += gameglobals.cinfos[digits[i]].wWindowRelative;

            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6] = (i-gameglobals.n) * 4;
            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6 + 1] = (i-gameglobals.n) * 4 + 2;
            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6 + 2] = (i-gameglobals.n) * 4 + 1;
            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6 + 3] = (i-gameglobals.n) * 4 + 2;
            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6 + 4] = (i-gameglobals.n) * 4 + 3;
            ((u16*)(gameglobals.textBuffersMemoryRaw + gameglobals.textIndexBufferOffset))[(i-gameglobals.n) * 6 + 5] = (i-gameglobals.n) * 4 + 1;
        }

        VkDeviceSize vertexBufferNotAlignedSize = sizeof(textVertexData) * 4 * (10 - gameglobals.n);
        VkDeviceSize vertexBufferAlignCooeficient = getAlignCooficient(vertexBufferNotAlignedSize, vkglobals.deviceProperties.limits.nonCoherentAtomSize);
        VkDeviceSize vertexBufferAlignedSize = vertexBufferNotAlignedSize + vertexBufferAlignCooeficient;
        VkDeviceSize indexBufferNotAlignedSize = sizeof(u16) * 6 * (10 - gameglobals.n);
        if (vertexBufferAlignedSize >= gameglobals.textBuffersMemorySize) {
            VkMappedMemoryRange bufferRange = {};
            bufferRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            bufferRange.size = VK_WHOLE_SIZE;
            bufferRange.offset = 0;
            bufferRange.memory = gameglobals.textBuffersMemory;

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &bufferRange), "failed to flush device memory\n");
        } else if (vertexBufferAlignedSize > indexBufferNotAlignedSize + gameglobals.textIndexBufferOffset) {
            VkMappedMemoryRange bufferRange = {};
            bufferRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            bufferRange.size = vertexBufferAlignedSize;
            bufferRange.offset = 0;
            bufferRange.memory = gameglobals.textBuffersMemory;

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 1, &bufferRange), "failed to flush device memory\n");
        } else {
            VkMappedMemoryRange vertexBufferRange = {};
            vertexBufferRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            vertexBufferRange.size = vertexBufferAlignedSize;
            vertexBufferRange.offset = 0;
            vertexBufferRange.memory = gameglobals.textBuffersMemory;

            VkMappedMemoryRange indexBufferRange = {};
            indexBufferRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            indexBufferRange.offset = gameglobals.textIndexBufferOffset;
            indexBufferRange.memory = gameglobals.textBuffersMemory;

            VkDeviceSize indexBufferAlignCooeficient = getAlignCooficient(indexBufferNotAlignedSize, vkglobals.deviceProperties.limits.nonCoherentAtomSize);
            VkDeviceSize indexBufferAlignedSize = indexBufferNotAlignedSize + indexBufferAlignCooeficient;
            if (indexBufferAlignedSize + gameglobals.textIndexBufferOffset > gameglobals.textBuffersMemorySize) indexBufferRange.size = VK_WHOLE_SIZE;
            else indexBufferRange.size = indexBufferAlignedSize;

            VK_ASSERT(vkFlushMappedMemoryRanges(vkglobals.device, 2, (VkMappedMemoryRange[]){vertexBufferRange, indexBufferRange}), "failed to flush device memory\n");
        }
    } else if (!(mouse & SDL_BUTTON_LEFT)) {
        gameglobals.blockInput = 0;
    }

    mouseX = mouseX / (WINDOW_WIDTH - 1) * 2 - 1;
    mouseY = mouseY / (WINDOW_HEIGHT - 1) * 2 - 1;
    if (mouseX > -0.5f && mouseX < 0.5f && mouseY > -0.5f && mouseY < 0.5f && !(mouse & SDL_BUTTON_LEFT)) {
        if (!gameglobals.lerpUp) {
            gameglobals.lerpUp = 1;
            gameglobals.lerpStarTimer = 0.0f;
            gameglobals.lerpStartStarScale = gameglobals.curStarScale;
        }
        gameglobals.curStarScale = lerpf(gameglobals.lerpStartStarScale, MAX_STAR_SCALE, gameglobals.lerpStarTimer);
        gameglobals.lerpStarTimer += deltaTime / LERP_STAR_INVERSE_SPEED;
        clampf(&gameglobals.lerpStarTimer, 0.0f, 1.0f);
    } else if (gameglobals.curStarScale != 1.0f) {
        if (gameglobals.lerpUp) {
            gameglobals.lerpUp = 0;
            gameglobals.lerpStarTimer = 0.0f;
            gameglobals.lerpStartStarScale = gameglobals.curStarScale;
        }
        gameglobals.curStarScale = lerpf(gameglobals.lerpStartStarScale, 1.0f, gameglobals.lerpStarTimer);
        gameglobals.lerpStarTimer += deltaTime / LERP_STAR_INVERSE_SPEED;
        clampf(&gameglobals.lerpStarTimer, 0.0f, 1.0f);
    }



    VK_ASSERT(vkWaitForFences(vkglobals.device, 1, &gameglobals.frameFence, VK_FALSE, 0xFFFFFFFFFFFFFFFF), "failed to wait for fences\n");
    VK_ASSERT(vkResetFences(vkglobals.device, 1, &gameglobals.frameFence), "failed to reset fences\n");

    u32 imageIndex;
    VK_ASSERT(vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, gameglobals.swapchainReadySemaphore, NULL, &imageIndex), "failed tp acquire swapchain image\n");

    {
        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        VK_ASSERT(vkBeginCommandBuffer(vkglobals.cmdBuffer, &cmdBeginInfo), "failed to begin command buffer\n");

        VkClearValue clearValue = {};
        clearValue.color = (VkClearColorValue){{0.0f, 0.0f, 0.0f, 0.0f}};

        VkRenderPassBeginInfo renderpassBeginInfo = {};
        renderpassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderpassBeginInfo.renderPass = gameglobals.renderpass;
        renderpassBeginInfo.renderArea = (VkRect2D){(VkOffset2D){0, 0}, vkglobals.swapchainExtent};
        renderpassBeginInfo.framebuffer = vkglobals.swapchainFramebuffers[imageIndex];
        renderpassBeginInfo.clearValueCount = 1;
        renderpassBeginInfo.pClearValues = &clearValue;

        vkCmdBeginRenderPass(vkglobals.cmdBuffer, &renderpassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.starPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.starPipelineLayout, 0, 1, &gameglobals.starDescriptorSet, 0, NULL);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.starPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0.0f, sizeof(f32), &gameglobals.curStarScale);
        vkCmdDraw(vkglobals.cmdBuffer, 6, 1, 0, 0);

        VkDeviceSize vertexBufferOffsets[1] = {0};

        vkCmdBindPipeline(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.textPipeline);
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.textPipelineLayout, 0, 1, &gameglobals.textDescriptorSet, 0, NULL);
        vkCmdBindVertexBuffers(vkglobals.cmdBuffer, 0, 1, &gameglobals.textVertexBuffer, vertexBufferOffsets);
        vkCmdBindIndexBuffer(vkglobals.cmdBuffer, gameglobals.textIndexBuffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(vkglobals.cmdBuffer, (10 - gameglobals.n) * 6, 1, 0, 0, 0);

        vkCmdEndRenderPass(vkglobals.cmdBuffer);

        VK_ASSERT(vkEndCommandBuffer(vkglobals.cmdBuffer), "failed to end command buffer\n");
    }

    VkPipelineStageFlags semaphoreSignalStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &gameglobals.swapchainReadySemaphore;
    submitInfo.pWaitDstStageMask = &semaphoreSignalStage;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &gameglobals.renderingDoneSemaphore;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &vkglobals.cmdBuffer;

    VK_ASSERT(vkQueueSubmit(vkglobals.queue, 1, &submitInfo, gameglobals.frameFence), "failed to submit command buffer\n");

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &vkglobals.swapchain;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &gameglobals.renderingDoneSemaphore;
    presentInfo.pImageIndices = &imageIndex;

    VK_ASSERT(vkQueuePresentKHR(vkglobals.queue, &presentInfo), "failed to present swapchain image\n");
}

void gameQuit() {
    vkDestroyFence(vkglobals.device, gameglobals.frameFence, NULL);
    vkDestroySemaphore(vkglobals.device, gameglobals.renderingDoneSemaphore, NULL);
    vkDestroySemaphore(vkglobals.device, gameglobals.swapchainReadySemaphore, NULL);
    vkDestroyPipeline(vkglobals.device, gameglobals.textPipeline, NULL);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.textPipelineLayout, NULL);
    vkDestroyPipeline(vkglobals.device, gameglobals.starPipeline, NULL);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.starPipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.textureDescriptorSetLayout, NULL);
    vkDestroyBuffer(vkglobals.device, gameglobals.textIndexBuffer, NULL);
    vkDestroyBuffer(vkglobals.device, gameglobals.textVertexBuffer, NULL);
    vkUnmapMemory(vkglobals.device, gameglobals.textBuffersMemory);
    vkFreeMemory(vkglobals.device, gameglobals.textBuffersMemory, NULL);
    vkDestroyDescriptorPool(vkglobals.device, gameglobals.descriptorPool, NULL);
    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        vkDestroyFramebuffer(vkglobals.device, vkglobals.swapchainFramebuffers[i], NULL);
    }
    vkDestroyRenderPass(vkglobals.device, gameglobals.renderpass, NULL);
    vkDestroySampler(vkglobals.device, gameglobals.text.sampler, NULL);
    vkDestroyImageView(vkglobals.device, gameglobals.text.view, NULL);
    vkDestroyImage(vkglobals.device, gameglobals.text.image, NULL);
    vkDestroySampler(vkglobals.device, gameglobals.star.sampler, NULL);
    vkDestroyImageView(vkglobals.device, gameglobals.star.view, NULL);
    vkDestroyImage(vkglobals.device, gameglobals.star.image, NULL);
    vkFreeMemory(vkglobals.device, gameglobals.texturesMem, NULL);
}