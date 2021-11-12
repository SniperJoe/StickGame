#include "CircleVertexInput.h"
#include <iostream>

std::vector<VkVertexInputAttributeDescription> CircleVertexInput::getAttributeDescriptions()
{
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions{};
    attributeDescriptions.resize(1);
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VkFormat::VK_FORMAT_R32_SINT;
    attributeDescriptions[0].offset = 0;
    return attributeDescriptions;
}

VkVertexInputBindingDescription CircleVertexInput::getBindingDescription()
{
    VkVertexInputBindingDescription vertexInputBindingDesc;
    vertexInputBindingDesc.binding = 0;
    vertexInputBindingDesc.stride = 0;
    vertexInputBindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return vertexInputBindingDesc;
}
size_t CircleVertexInput::getDataSize()
{
    return sizeof(uint32_t);
}