#include "CircleVertexInput.h"
#include "PipelineManager.h"
#include <stdexcept>
#include <iostream>
#include <fstream>
#include "VertexInput.h"
#include "CreateCommandPool.h"
#include "Families.h"
#ifdef __linux__
    #include <cstring>
#endif
VkShaderModule PipelineManager::createShaderModule(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(this->device, &createInfo, nullptr, &shaderModule) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

std::vector<char> PipelineManager::readFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

PipelineManager::PipelineManager(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, uint32_t transferFamilyIndex, uint32_t graphicsFamilyIndex)
{
	this->device = device;
    this->renderPass = renderPass;
    this->physicalDevice = physicalDevice;
    this->transferFamilyIndex = transferFamilyIndex;
    this->graphicsFamilyIndex = graphicsFamilyIndex;

    createCommandPool(this->device, transferFamilyIndex, &this->transferCommandPool);
    vkGetDeviceQueue(this->device, transferFamilyIndex, 0, &this->transferQueue);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0; // Optional
    pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

}

PipelineManager::~PipelineManager()
{
    for (const auto& pipeline : this->pipelines) {
        vkDestroyPipeline(this->device, pipeline.second, nullptr);
        if (this->vertexBuffers[pipeline.first]) {
            vkDestroyBuffer(this->device, this->vertexBuffers[pipeline.first], nullptr);
            vkFreeMemory(this->device, this->vertexBufferMemories[pipeline.first], nullptr);
            vkDestroyBuffer(this->device, this->stagingBuffers[pipeline.first], nullptr);
            vkFreeMemory(this->device, this->stagingBufferMemories[pipeline.first], nullptr);
        }
    }
    vkDestroyPipelineLayout(this->device, this->pipelineLayout, nullptr);
    vkDestroyCommandPool(this->device, this->transferCommandPool, nullptr);
}

void PipelineManager::createPipelines(size_t infosCount, PipelineCreateInfo* createInfos)
{
    std::vector<VkGraphicsPipelineCreateInfo> pipelineInfos;
    pipelineInfos.resize(infosCount);
    std::vector<VkShaderModule> shaderModules;
    shaderModules.resize(infosCount * 2);

    for (size_t infoIndex = 0; infoIndex < infosCount; infoIndex++) {

        auto vertShaderCode = this->readFile(createInfos[infoIndex].vertexShaderModule);
        auto fragShaderCode = this->readFile(createInfos[infoIndex].fragmentShaderModule);

        shaderModules[infoIndex * 2] = createShaderModule(vertShaderCode);
        shaderModules[infoIndex * 2 + 1] = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = shaderModules[infoIndex * 2];
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = shaderModules[infoIndex * 2 + 1];
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        VkVertexInputBindingDescription vertexInputBindingDesc;
        if (createInfos[infoIndex].input) {
            vertexInputBindingDesc = createInfos[infoIndex].input->getBindingDescription();
            auto vertexInputAttributeDescs = createInfos[infoIndex].input->getAttributeDescriptions();
            VkVertexInputAttributeDescription* vertexInputAttributeDescsMem = (VkVertexInputAttributeDescription*)malloc(sizeof(VkVertexInputAttributeDescription) * vertexInputAttributeDescs.size());
            if (vertexInputAttributeDescsMem != 0) {
                memcpy(vertexInputAttributeDescsMem, vertexInputAttributeDescs.data(), sizeof(VkVertexInputAttributeDescription) * vertexInputAttributeDescs.size());
            }
            vertexInputInfo.vertexBindingDescriptionCount = 1;
            vertexInputInfo.pVertexBindingDescriptions = &vertexInputBindingDesc; // Optional
            vertexInputInfo.vertexAttributeDescriptionCount = vertexInputAttributeDescs.size();
            vertexInputInfo.pVertexAttributeDescriptions = vertexInputAttributeDescsMem;
        }
        else {
            vertexInputInfo.vertexBindingDescriptionCount = 0;
            vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
            vertexInputInfo.vertexAttributeDescriptionCount = 0;
            vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional
        }

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = createInfos[infoIndex].topology;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)createInfos[infoIndex].extent.width;
        viewport.height = (float)createInfos[infoIndex].extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = createInfos[infoIndex].extent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;


        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;

        if (createInfos[infoIndex].topology == VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) {
            rasterizer.lineWidth = 5.0f;
            rasterizer.polygonMode = VkPolygonMode::VK_POLYGON_MODE_LINE;
        }
        else {
            rasterizer.lineWidth = 1.0f;
            rasterizer.polygonMode = VkPolygonMode::VK_POLYGON_MODE_FILL;
        }
        rasterizer.cullMode = VkCullModeFlagBits::VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VkFrontFace::VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;
        rasterizer.depthBiasConstantFactor = 0.0f; // Optional
        rasterizer.depthBiasClamp = 0.0f; // Optional
        rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;
        multisampling.minSampleShading = 1.0f; // Optional
        multisampling.pSampleMask = nullptr; // Optional
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
        multisampling.alphaToOneEnable = VK_FALSE; // Optional

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VkColorComponentFlagBits::VK_COLOR_COMPONENT_R_BIT
            | VkColorComponentFlagBits::VK_COLOR_COMPONENT_G_BIT
            | VkColorComponentFlagBits::VK_COLOR_COMPONENT_B_BIT
            | VkColorComponentFlagBits::VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstColorBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VkBlendOp::VK_BLEND_OP_ADD; // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ONE; // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VkBlendFactor::VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VkBlendOp::VK_BLEND_OP_ADD; // Optional

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VkLogicOp::VK_LOGIC_OP_COPY; // Optional
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f; // Optional
        colorBlending.blendConstants[1] = 0.0f; // Optional
        colorBlending.blendConstants[2] = 0.0f; // Optional
        colorBlending.blendConstants[3] = 0.0f; // Optional

        pipelineInfos[infoIndex].sType = VkStructureType::VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfos[infoIndex].stageCount = 2;
        pipelineInfos[infoIndex].pStages = shaderStages;
        pipelineInfos[infoIndex].pVertexInputState = &vertexInputInfo;
        pipelineInfos[infoIndex].pInputAssemblyState = &inputAssembly;
        pipelineInfos[infoIndex].pViewportState = &viewportState;
        pipelineInfos[infoIndex].pRasterizationState = &rasterizer;
        pipelineInfos[infoIndex].pMultisampleState = &multisampling;
        pipelineInfos[infoIndex].pDepthStencilState = nullptr; // Optional
        pipelineInfos[infoIndex].pColorBlendState = &colorBlending;
        pipelineInfos[infoIndex].layout = this->pipelineLayout;
        pipelineInfos[infoIndex].renderPass = renderPass;
        pipelineInfos[infoIndex].subpass = 0;
        pipelineInfos[infoIndex].basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfos[infoIndex].basePipelineIndex = -1; // Optional
        if (createInfos[infoIndex].topology == VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) {
            VkDynamicState dynamicStates[1] = { VkDynamicState::VK_DYNAMIC_STATE_LINE_WIDTH };
            VkPipelineDynamicStateCreateInfo dynamicStateInfo;
            dynamicStateInfo.pDynamicStates = dynamicStates;
            dynamicStateInfo.dynamicStateCount = 1;
            dynamicStateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicStateInfo.pNext = nullptr;
            dynamicStateInfo.flags = 0;
            pipelineInfos[infoIndex].pDynamicState = &dynamicStateInfo; // Optional
        }
        else
        {
            pipelineInfos[infoIndex].pDynamicState = nullptr;
        }
        this->createInfos[createInfos[infoIndex].name] = createInfos[infoIndex];
        VkPipeline pipeline;
        std::cout << pipelineInfos[infoIndex].pRasterizationState->sType;
        if (vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, 1, &pipelineInfos[infoIndex], nullptr, &pipeline) != VkResult::VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }
        this->pipelines[createInfos[infoIndex].name] = pipeline;
        if (createInfos[infoIndex].input) {
            this->createVertexBuffer(createInfos[infoIndex].name, createInfos[infoIndex].input);
        }
    }
    /*std::vector<VkPipeline> pipelines;
    pipelines.resize(infosCount);
    if (vkCreateGraphicsPipelines(this->device, VK_NULL_HANDLE, (uint32_t)infosCount, pipelineInfos.data(), nullptr, pipelines.data()) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }
    for (size_t infoIndex = 0; infoIndex < infosCount; infoIndex++) {
        this->pipelines[createInfos[infoIndex].name] = pipelines[infoIndex];
        if (createInfos[infoIndex].input) {
            this->createVertexBuffer(createInfos[infoIndex].name, createInfos[infoIndex].input);
        }
    }*/

    for (const auto& shaderModule : shaderModules) {
        vkDestroyShaderModule(this->device, shaderModule, nullptr);
    }
}
void PipelineManager::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = this->transferCommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(this->device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.srcOffset = 0; // Optional
    copyRegion.dstOffset = 0; // Optional
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(this->transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(this->transferQueue);
    vkFreeCommandBuffers(this->device, this->transferCommandPool, 1, &commandBuffer);
}
void PipelineManager::createVertexBuffer(const std::string name, VertexInput* bufferContent) {
    uint32_t vertexBufferUsingFamilyIndices[] = { this->graphicsFamilyIndex, this->transferFamilyIndex };
    uint32_t stagingBufferUsingFamilyIndices[] = { this->transferFamilyIndex };

    VkDeviceSize bufferSize = sizeof(bufferContent->getDataSize());

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    this->createBuffer(bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory,
        1,
        stagingBufferUsingFamilyIndices
    );

    VkBuffer vertexBuffer;
    VkDeviceMemory vertextBufferMemory;

    this->createBuffer(bufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        vertexBuffer,
        vertextBufferMemory,
        2,
        vertexBufferUsingFamilyIndices
    );

    this->vertexBuffers[name] = vertexBuffer;
    this->vertexBufferMemories[name] = vertextBufferMemory;

    this->stagingBuffers[name] = stagingBuffer;
    this->stagingBufferMemories[name] = stagingBufferMemory;
}
void PipelineManager::writeVertexData(void* vertexData, std::string name) {
    size_t dataSize = this->createInfos[name].input->getDataSize();
    
    void* data;
    vkMapMemory(this->device, this->stagingBufferMemories[name], 0, dataSize, 0, &data);
    memcpy(data, vertexData, dataSize);
    vkUnmapMemory(this->device, this->stagingBufferMemories[name]);

    this->copyBuffer(this->stagingBuffers[name], this->vertexBuffers[name], dataSize);
}
void PipelineManager::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t usingFamiliesCount, uint32_t* usingFamilies) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    if (usingFamiliesCount < 2) {
        bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        bufferInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_CONCURRENT;
    }
    if (usingFamiliesCount) {
        bufferInfo.queueFamilyIndexCount = usingFamiliesCount;
        bufferInfo.pQueueFamilyIndices = usingFamilies;
    }

    if (vkCreateBuffer(this->device, &bufferInfo, nullptr, &buffer) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create vertex buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(this->device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = this->findMemoryType(memRequirements.memoryTypeBits, properties);

    if (vkAllocateMemory(this->device, &allocInfo, nullptr, &bufferMemory) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to allocate vertex buffer memory!");
    }

    vkBindBufferMemory(this->device, buffer, bufferMemory, 0);
}
uint32_t PipelineManager::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(this->physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void PipelineManager::writeCommands(VkCommandBuffer buffer)
{
    for (const auto& pipeline : this->pipelines) {
        vkCmdBindPipeline(buffer, VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS, this->pipelines[pipeline.first]);
        if (this->vertexBuffers[pipeline.first]) {
            VkBuffer vertexBuffers[] = { this->vertexBuffers[pipeline.first] };
            VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(buffer, 0, 1, vertexBuffers, offsets);
        }
        if (this->createInfos[pipeline.first].topology == VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP) {
            vkCmdSetLineWidth(buffer, 1.0);
        }
    }
}
