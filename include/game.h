#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>

#include "numtypes.h"

typedef struct {
    VkImage image;
    VkDeviceMemory mem;
    VkImageView view;
    VkSampler sampler;
} texture_t;

typedef struct {
    VkBuffer buffer;
    VkDeviceMemory mem;
} buffer_t;

typedef struct {
    VkDescriptorPool descriptorPool;
    
    texture_t star;
    VkDescriptorSetLayout starDescriptorSetLayout;
    VkDescriptorSet starTextureDescriptorSet;
    VkPipelineLayout starPipelineLayout;
    VkPipeline starPipeline;
    f32 curStarScale;
    f32 lerpStartStarScale;
    f32 lerpStarTimer;
    u8 lerpUp;

    VkSemaphore swapchainReadySemaphore;
    VkSemaphore renderingDoneSemaphore;
    VkFence frameFence;

    VkRenderPass renderpass;
} game_globals_t;

#define MAX_STAR_SCALE 1.1f
#define LERP_STAR_INVERSE_SPEED 100.0f

void gameInit();
void gameRender();
void gameQuit();

extern game_globals_t gameglobals;

#endif