#include <vulkan/vulkan.h>

#pragma once
bool isGraphicsFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
bool isPresentFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
bool isTransferFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


