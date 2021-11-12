#include <vulkan/vulkan.h>
#include <vector>

#pragma once
struct VertexInput {
	virtual VkVertexInputBindingDescription getBindingDescription() = 0;
	virtual std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() = 0;
	virtual size_t getDataSize() = 0;
};