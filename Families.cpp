#include "Families.h"

bool isGraphicsFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    return family.queueFlags & VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT;
}

bool isPresentFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex, surface, &presentSupport);
    return presentSupport;
}

bool isTransferFamily(VkQueueFamilyProperties family, uint32_t familyIndex, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    return (family.queueFlags & (VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT | VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT)) == VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT;
}
