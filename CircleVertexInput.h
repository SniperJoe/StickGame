#pragma once
#include "VertexInput.h"
struct CircleVertexInput:
    public VertexInput
{
    VkVertexInputBindingDescription getBindingDescription();
    std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
    size_t getDataSize();
};

