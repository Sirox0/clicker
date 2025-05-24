#ifndef GAME_H
#define GAME_H

#include <vulkan/vulkan.h>
#include <freetype/freetype.h>
#include <cglm/cglm.h>

#include "numtypes.h"

#define FT_ASSERT(expression) \
    { \
        FT_Error error = expression; \
        if (error != 0) { \
            printf("freetype error: %s\n", FT_Error_String(error)); \
            exit(1); \
        } \
    }

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
    f32 wWindowRelative;
    f32 hWindowRelative;
    f32 wTextureRelative;
    f32 hTextureRelative;
    f32 offset;
} char_info_t;

typedef struct {
    vec4 posuv;
} textVertexData;

typedef struct {
    VkDescriptorPool descriptorPool;
    VkDescriptorSetLayout textureDescriptorSetLayout;
    
    texture_t star;
    VkDescriptorSet starDescriptorSet;
    VkPipelineLayout starPipelineLayout;
    VkPipeline starPipeline;
    f32 curStarScale;
    f32 lerpStartStarScale;
    f32 lerpStarTimer;
    u8 lerpUp;

    texture_t text;
    VkBuffer textVertexBuffer;
    VkBuffer textIndexBuffer;
    VkDeviceSize textIndexBufferOffset;
    VkDeviceMemory textBuffersMemory;
    VkDeviceSize textBuffersMemorySize;
    void* textBuffersMemoryRaw;
    VkDescriptorSet textDescriptorSet;
    VkPipelineLayout textPipelineLayout;
    VkPipeline textPipeline;
    u32 textW;
    u32 textH;
    char_info_t cinfos[10];
    u32 clickCounter;
    u8 blockInput;
    u32 n;

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