#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <map>
#include "VertexInput.h"

#pragma once
struct PipelineCreateInfo {
	const char* name;
	VkPrimitiveTopology topology;
	const char* vertexShaderModule;
	const char* fragmentShaderModule;
	VertexInput* input;
	VkExtent2D extent;
};
class PipelineManager
{
private:
	VkDevice device;
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	std::map<const std::string, VkPipeline> pipelines;
	VkShaderModule createShaderModule(const std::vector<char>& code);
	static std::vector<char> readFile(const std::string& filename);
	std::map<const std::string, VkBuffer> vertexBuffers;
	std::map<const std::string, VkDeviceMemory> vertexBufferMemories;
	std::map<const std::string, VkBuffer> stagingBuffers;
	std::map<const std::string, VkDeviceMemory> stagingBufferMemories;
	VkCommandPool transferCommandPool;
	VkPhysicalDevice physicalDevice;
	VkQueue transferQueue;
	uint32_t transferFamilyIndex;
	uint32_t graphicsFamilyIndex;
	std::map<const std::string, PipelineCreateInfo> createInfos;

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void createVertexBuffer(const std::string name, VertexInput* bufferContent);
	void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, uint32_t usingFamiliesCount, uint32_t* usingFamilies);
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
public: 
	PipelineManager(VkPhysicalDevice physicalDevice, VkDevice device, VkRenderPass renderPass, uint32_t transferFamilyIndex, uint32_t graphicsFamilyIndex);
	~PipelineManager();
	void createPipelines(size_t infosCount, PipelineCreateInfo* createInfos);
	void writeCommands(VkCommandBuffer buffer);
	void writeVertexData(void* vertexData, std::string name);
};
