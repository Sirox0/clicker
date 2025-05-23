#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

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

game_globals_t gameglobals = {};

#define GARBAGE_CMD_BUFFER_NUM 1
#define GARBAGE_BUFFER_NUM 1
#define GARBAGE_FENCE_NUM 1

void gameInit() {
    VkCommandBuffer garbageCmdBuffers[GARBAGE_CMD_BUFFER_NUM];
    buffer_t garbageBuffers[GARBAGE_BUFFER_NUM];
    VkFence garbageFences[GARBAGE_FENCE_NUM];

    garbageCreate(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_FENCE_NUM, garbageFences);

    i32 starW, starH;
    createTexture("assets/textures/star.png", &starW, &starH, &gameglobals.star, &garbageBuffers[0], garbageCmdBuffers[0], garbageFences[0]);

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
        VkDescriptorPoolSize starTexturePoolSize = {};
        starTexturePoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        starTexturePoolSize.descriptorCount = 1;

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &starTexturePoolSize;
        poolInfo.maxSets = 1;

        VK_ASSERT(vkCreateDescriptorPool(vkglobals.device, &poolInfo, NULL, &gameglobals.descriptorPool), "failed to create descriptor pool\n");
    }

    {
        VkDescriptorSetLayoutBinding starTextureBinding = {};
        starTextureBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        starTextureBinding.binding = 0;
        starTextureBinding.descriptorCount = 1;
        starTextureBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {};
        descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptorSetLayoutInfo.bindingCount = 1;
        descriptorSetLayoutInfo.pBindings = &starTextureBinding;

        VK_ASSERT(vkCreateDescriptorSetLayout(vkglobals.device, &descriptorSetLayoutInfo, NULL, &gameglobals.starDescriptorSetLayout), "failed to create descriptor set layout\n");
    }

    {
        VkDescriptorSetAllocateInfo descriptorSetsInfo = {};
        descriptorSetsInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        descriptorSetsInfo.descriptorPool = gameglobals.descriptorPool;
        descriptorSetsInfo.descriptorSetCount = 1;
        descriptorSetsInfo.pSetLayouts = (VkDescriptorSetLayout[]){gameglobals.starDescriptorSetLayout};

        VK_ASSERT(vkAllocateDescriptorSets(vkglobals.device, &descriptorSetsInfo, &gameglobals.starTextureDescriptorSet), "failed to allocate descriptor sets\n");

        VkDescriptorImageInfo starImageDescriptorInfo = {};
        starImageDescriptorInfo.imageView = gameglobals.star.view;
        starImageDescriptorInfo.sampler = gameglobals.star.sampler;
        starImageDescriptorInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.dstSet = gameglobals.starTextureDescriptorSet;
        descriptorWrite.pImageInfo = &starImageDescriptorInfo;

        vkUpdateDescriptorSets(vkglobals.device, 1, &descriptorWrite, 0, NULL);
    }

    {
        VkPushConstantRange pcRange = {};
        pcRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pcRange.size = sizeof(f32);
        pcRange.offset = 0.0f;

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &gameglobals.starDescriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pcRange;

        VK_ASSERT(vkCreatePipelineLayout(vkglobals.device, &pipelineLayoutInfo, NULL, &gameglobals.starPipelineLayout), "failed to create pipeline layout\n");


        graphics_pipeline_info_t pipelineInfo = {};
        pipelineFillDefaultGraphicsPipeline(&pipelineInfo);
        pipelineInfo.stageCount = 2;
        pipelineInfo.stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        pipelineInfo.stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        pipelineInfo.stages[0].module = createShaderModuleFromFile("assets/shaders/star.vert.spv");
        pipelineInfo.stages[1].module = createShaderModuleFromFile("assets/shaders/star.frag.spv");
        pipelineInfo.layout = gameglobals.starPipelineLayout;
        pipelineInfo.renderpass = gameglobals.renderpass;
        pipelineInfo.subpass = 0;

        pipelineCreateGraphicsPipelines(NULL, 1, &pipelineInfo, &gameglobals.starPipeline);

        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[0].module, NULL);
        vkDestroyShaderModule(vkglobals.device, pipelineInfo.stages[1].module, NULL);
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

    garbageWaitAndDestroy(GARBAGE_CMD_BUFFER_NUM, garbageCmdBuffers, GARBAGE_BUFFER_NUM, garbageBuffers, GARBAGE_FENCE_NUM, garbageFences);
}

void gameRender() {
    f32 mouseX, mouseY;
    sdlQueryMouseState(&mouseX, &mouseY);
    mouseX = mouseX / (WINDOW_WIDTH - 1) * 2 - 1;
    mouseY = mouseY / (WINDOW_HEIGHT - 1) * 2 - 1;
    if (mouseX > -0.5f && mouseX < 0.5f && mouseY > -0.5f && mouseY < 0.5f) {
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



    vkWaitForFences(vkglobals.device, 1, &gameglobals.frameFence, VK_FALSE, 0xFFFFFFFFFFFFFFFF);
    vkResetFences(vkglobals.device, 1, &gameglobals.frameFence);

    u32 imageIndex;
    vkAcquireNextImageKHR(vkglobals.device, vkglobals.swapchain, 0xFFFFFFFFFFFFFFFF, gameglobals.swapchainReadySemaphore, NULL, &imageIndex);

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
        vkCmdBindDescriptorSets(vkglobals.cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gameglobals.starPipelineLayout, 0, 1, &gameglobals.starTextureDescriptorSet, 0, NULL);
        vkCmdPushConstants(vkglobals.cmdBuffer, gameglobals.starPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0.0f, sizeof(f32), &gameglobals.curStarScale);
        vkCmdDraw(vkglobals.cmdBuffer, 6, 1, 0, 0);

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
    vkDestroyPipeline(vkglobals.device, gameglobals.starPipeline, NULL);
    vkDestroyPipelineLayout(vkglobals.device, gameglobals.starPipelineLayout, NULL);
    vkDestroyDescriptorSetLayout(vkglobals.device, gameglobals.starDescriptorSetLayout, NULL);
    vkDestroyDescriptorPool(vkglobals.device, gameglobals.descriptorPool, NULL);
    for (u32 i = 0; i < vkglobals.swapchainImageCount; i++) {
        vkDestroyFramebuffer(vkglobals.device, vkglobals.swapchainFramebuffers[i], NULL);
    }
    vkDestroyRenderPass(vkglobals.device, gameglobals.renderpass, NULL);
    vkDestroySampler(vkglobals.device, gameglobals.star.sampler, NULL);
    vkDestroyImageView(vkglobals.device, gameglobals.star.view, NULL);
    vkDestroyImage(vkglobals.device, gameglobals.star.image, NULL);
    vkFreeMemory(vkglobals.device, gameglobals.star.mem, NULL);
}