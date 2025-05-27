#include <vulkan/vulkan.h>

#include "numtypes.h"
#include "vkFunctions.h"
#include "vkInit.h"
#include "pipeline.h"

void pipelineFillDefaultGraphicsPipeline(graphics_pipeline_info_t* pInfo) {
    pInfo->pNext = NULL;
    pInfo->flags = 0;
    pInfo->stageCount = 0; // must be set by user
    for (u32 i = 0; i < 5; i++) {
        pInfo->stages[i].pNext = NULL;
        pInfo->stages[i].flags = 0;
        pInfo->stages[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pInfo->stages[i].stage = VK_SHADER_STAGE_ALL; // must be set by user
        pInfo->stages[i].pName = "main";
        pInfo->stages[i].module = NULL; // must be set by user
        pInfo->stages[i].pSpecializationInfo = NULL;
    }

    pInfo->vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    pInfo->vertexInputState.pNext = NULL;
    pInfo->vertexInputState.flags = 0;
    pInfo->vertexInputState.vertexAttributeDescriptionCount = 0;
    pInfo->vertexInputState.pVertexAttributeDescriptions = NULL;
    pInfo->vertexInputState.vertexBindingDescriptionCount = 0;
    pInfo->vertexInputState.pVertexBindingDescriptions = NULL;

    pInfo->inputAssemblyState.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pInfo->inputAssemblyState.pNext = NULL;
    pInfo->inputAssemblyState.flags = 0;
    pInfo->inputAssemblyState.primitiveRestartEnable = VK_FALSE;
    pInfo->inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    pInfo->tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    pInfo->tessellationState.pNext = NULL;
    pInfo->tessellationState.flags = 0;
    pInfo->tessellationState.patchControlPoints = 4;

    pInfo->viewport.x = 0;
    pInfo->viewport.y = 0;
    pInfo->viewport.minDepth = 0.0f;
    pInfo->viewport.maxDepth = 1.0f;
    pInfo->viewport.width = vkglobals.swapchainExtent.width;
    pInfo->viewport.height = vkglobals.swapchainExtent.height;

    pInfo->scissor.extent = vkglobals.swapchainExtent;
    pInfo->scissor.offset = (VkOffset2D){0, 0};

    pInfo->viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pInfo->viewportState.pNext = NULL;
    pInfo->viewportState.flags = 0;
    pInfo->viewportState.scissorCount = 1;
    pInfo->viewportState.pScissors = &pInfo->scissor;
    pInfo->viewportState.viewportCount = 1;
    pInfo->viewportState.pViewports = &pInfo->viewport;

    pInfo->rasterizationState.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pInfo->rasterizationState.pNext = NULL;
    pInfo->rasterizationState.flags = 0;
    pInfo->rasterizationState.depthClampEnable = VK_FALSE;
    pInfo->rasterizationState.rasterizerDiscardEnable = VK_FALSE;
    pInfo->rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
    pInfo->rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
    pInfo->rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pInfo->rasterizationState.depthBiasEnable = VK_FALSE;
    pInfo->rasterizationState.lineWidth = 1.0f;

    pInfo->multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pInfo->multisampleState.pNext = NULL;
    pInfo->multisampleState.flags = 0;
    pInfo->multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pInfo->multisampleState.sampleShadingEnable = VK_FALSE;
    pInfo->multisampleState.minSampleShading = 1.0f;
    pInfo->multisampleState.pSampleMask = NULL;
    pInfo->multisampleState.alphaToCoverageEnable = VK_FALSE;
    pInfo->multisampleState.alphaToOneEnable = VK_FALSE;

    pInfo->depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    pInfo->depthStencilState.pNext = NULL;
    pInfo->depthStencilState.flags = 0;
    pInfo->depthStencilState.depthTestEnable = VK_FALSE;
    pInfo->depthStencilState.depthWriteEnable = VK_FALSE;
    pInfo->depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS;
    pInfo->depthStencilState.depthBoundsTestEnable = VK_FALSE;
    pInfo->depthStencilState.stencilTestEnable = VK_FALSE;
    pInfo->depthStencilState.minDepthBounds = 0.0f;
    pInfo->depthStencilState.maxDepthBounds = 1.0f;

    pInfo->colorBlendAttachment.blendEnable = VK_TRUE;
    pInfo->colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    pInfo->colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    pInfo->colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    pInfo->colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    pInfo->colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
    pInfo->colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_CONSTANT_ALPHA;
    pInfo->colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    pInfo->colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pInfo->colorBlendState.pNext = NULL;
    pInfo->colorBlendState.flags = 0;
    pInfo->colorBlendState.logicOpEnable = VK_FALSE;
    pInfo->colorBlendState.logicOp = VK_LOGIC_OP_COPY;
    pInfo->colorBlendState.attachmentCount = 1;
    pInfo->colorBlendState.pAttachments = &pInfo->colorBlendAttachment;
    pInfo->colorBlendState.blendConstants[0] = 0.0f;
    pInfo->colorBlendState.blendConstants[1] = 0.0f;
    pInfo->colorBlendState.blendConstants[2] = 0.0f;
    pInfo->colorBlendState.blendConstants[3] = 0.5f;

    pInfo->dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pInfo->dynamicState.pNext = NULL;
    pInfo->dynamicState.flags = 0;
    pInfo->dynamicState.dynamicStateCount = 0;
    pInfo->dynamicState.pDynamicStates = NULL;

    pInfo->layout = NULL; // must be set by user
    pInfo->renderpass = NULL; // must be set by user
    pInfo->subpass = 0; // must be set by user
    pInfo->basePipelineHandle = NULL;
    pInfo->basePipelineIndex = 0;
}

void pipelineFillDefaultComputePipeline(compute_pipeline_info_t* pInfo) {
    pInfo->pNext = NULL;
    pInfo->flags = 0;
    pInfo->stage.pNext = NULL;
    pInfo->stage.flags = 0;
    pInfo->stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    pInfo->stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    pInfo->stage.pName = "main";
    pInfo->stage.module = NULL; // must be set by user
    pInfo->stage.pSpecializationInfo = NULL;
    pInfo->layout = NULL; // must be set by user
    pInfo->basePipelineHandle = NULL;
    pInfo->basePipelineIndex = 0;
}

void pipelineCreateComputePipelines(VkPipelineCache cache, u32 infoCount, compute_pipeline_info_t* pInfos, VkPipeline* pPipelines) {
    VkComputePipelineCreateInfo createInfos[infoCount];

    for (u32 i = 0; i < infoCount; i++) {
        createInfos[i].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfos[i].pNext = pInfos[i].pNext;
        createInfos[i].flags = pInfos[i].flags;
        createInfos[i].stage = pInfos[i].stage;
        createInfos[i].layout = pInfos[i].layout;
        createInfos[i].basePipelineHandle = pInfos[i].basePipelineHandle;
        createInfos[i].basePipelineIndex = pInfos[i].basePipelineIndex;
    }

    VK_ASSERT(vkCreateComputePipelines(vkglobals.device, cache, infoCount, createInfos, NULL, pPipelines), "failed to create compute pipelines\n");
}

void pipelineCreateGraphicsPipelines(VkPipelineCache cache, u32 infoCount, graphics_pipeline_info_t* pInfos, VkPipeline* pPipelines) {
    VkGraphicsPipelineCreateInfo createInfos[infoCount];

    for (u32 i = 0; i < infoCount; i++) {
        createInfos[i].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        createInfos[i].pNext = pInfos[i].pNext;
        createInfos[i].flags = pInfos[i].flags;
        createInfos[i].stageCount = pInfos[i].stageCount;
        createInfos[i].pStages = pInfos[i].stages;
        createInfos[i].pVertexInputState = &pInfos[i].vertexInputState;
        createInfos[i].pInputAssemblyState = &pInfos[i].inputAssemblyState;
        createInfos[i].pTessellationState = &pInfos[i].tessellationState;
        createInfos[i].pViewportState = &pInfos[i].viewportState;
        createInfos[i].pRasterizationState = &pInfos[i].rasterizationState;
        createInfos[i].pMultisampleState = &pInfos[i].multisampleState;
        createInfos[i].pRasterizationState = &pInfos[i].rasterizationState;
        createInfos[i].pDepthStencilState = &pInfos[i].depthStencilState;
        createInfos[i].pColorBlendState = &pInfos[i].colorBlendState;
        createInfos[i].pDynamicState = &pInfos[i].dynamicState;
        createInfos[i].layout = pInfos[i].layout;
        createInfos[i].renderPass = pInfos[i].renderpass;
        createInfos[i].subpass = pInfos[i].subpass;
        createInfos[i].basePipelineHandle = pInfos[i].basePipelineHandle;
        createInfos[i].basePipelineIndex = pInfos[i].basePipelineIndex;
    }

    VK_ASSERT(vkCreateGraphicsPipelines(vkglobals.device, cache, infoCount, createInfos, NULL, pPipelines), "failed to create graphics pipelines\n");
}