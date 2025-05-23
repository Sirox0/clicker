#ifndef PIPELINE_H
#define PIPELINE_H

#include <vulkan/vulkan.h>

#include "numtypes.h"

typedef struct {
    void* pNext;
    VkPipelineCreateFlags flags;
    u32 stageCount;
    VkPipelineShaderStageCreateInfo stages[5];
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineTessellationStateCreateInfo tessellationState;
    VkViewport viewport;
    VkRect2D scissor;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterizationState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    VkPipelineColorBlendAttachmentState colorBlendAttachment;
    VkPipelineColorBlendStateCreateInfo colorBlendState;
    VkPipelineDynamicStateCreateInfo dynamicState;
    VkPipelineLayout layout;
    VkRenderPass renderpass;
    u32 subpass;
    VkPipeline basePipelineHandle;
    i32 basePipelineIndex;
} graphics_pipeline_info_t;

typedef struct {
    void* pNext;
    VkPipelineCreateFlags flags;
    VkPipelineShaderStageCreateInfo stage;
    VkPipelineLayout layout;
    VkPipeline basePipelineHandle;
    i32 basePipelineIndex;
} compute_pipeline_info_t;

void pipelineFillDefaultGraphicsPipeline(graphics_pipeline_info_t* pInfo);
void pipelineFillDefaultComputePipeline(compute_pipeline_info_t* pInfo);
void pipelineCreateComputePipelines(VkPipelineCache cache, u32 infoCount, compute_pipeline_info_t* pInfos, VkPipeline* pPipelines);
void pipelineCreateGraphicsPipelines(VkPipelineCache cache, u32 infoCount, graphics_pipeline_info_t* pInfos, VkPipeline* pPipelines);

#endif