#include "CreateCommandPool.h"
#include <stdexcept>

void createCommandPool(VkDevice device, uint32_t familyIndex, VkCommandPool* commandPool)
{
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = familyIndex;
    poolInfo.flags = 0; // Optional

    if (vkCreateCommandPool(device, &poolInfo, nullptr, commandPool) != VkResult::VK_SUCCESS) {
        throw std::runtime_error("failed to create command pool!");
    }
}