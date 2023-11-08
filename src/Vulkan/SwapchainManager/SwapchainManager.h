#pragma once

#include "Logger/Logger.h"
#include "Vulkan/CommandPool/CommandPoolManager.h"
#include "Vulkan/Utils/Structs/QueueFamilyIndices.h"
#include "Vulkan/Utils/Structs/SwapChainSupportDetails.h"
#include "Vulkan/VulkanObject/VulkanObject.h"
#include "Vulkan/Window/Window.h"
#include "Image.h"
#include "TexCube.h"

class SwapchainManager {

private:
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	std::vector<VkImageView> swapChainImageViews;

	std::vector<VkFramebuffer> swapChainFramebuffers;
	std::vector<VkFramebuffer> swapChainFramebuffers2;
	std::vector<VkFramebuffer> quadSwapChainFramebuffers;
	std::vector<VkFramebuffer> blurSwapChainFramebuffers;

	Image depthImage;
	Image depthImage2;
	Image quadDepthImage;
	std::vector<Image> blurImage;
	std::vector<Image> blurImage2;

	TexCube texCube;

	Window* window;
	VkSurfaceKHR* surface;
	VkPhysicalDevice* physicalDevice;
	VkDevice* device;
	SwapChainSupportDetails swapChainSupportDetails;
	QueueFamilyIndices* indices;
	VulkanObject* vulkanObject;

	Logger& logger;

	uint32_t minImageCount;

	void createSwapChain();

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice* phDevice,
		VkSurfaceKHR& surface);

	void createImageViews();

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(
		const std::vector<VkSurfaceFormatKHR>& availableFormats);

	VkPresentModeKHR chooseSwapPresentMode(
		const std::vector<VkPresentModeKHR>& availablePresentModes);

	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);

	VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates,
		VkImageTiling tiling,
		VkFormatFeatureFlags features);

public:
	SwapchainManager() : logger(Logger::getInstance()) {}

	void init(Window& window, VkSurfaceKHR& surface,
		VkPhysicalDevice& physicalDevice, VkDevice& device,
		QueueFamilyIndices& indices);

	void createDepthResources(CommandPoolManager& commandPoolManager,
		VkQueue& graphicsQueue);

	void createFramebuffers(VulkanObject& vulkanObject);

	void recreateSwapChain(GLFWwindow* window, VkDevice& device,
		CommandPoolManager& commandPoolManager,
		VkQueue& graphicsQueue);

	void cleanupSwapChain();

	VkFormat findDepthFormat();

	VkFormat& getSwapChainImageFormat() { return swapChainImageFormat; }
	VkExtent2D& getSwapChainExtent() { return swapChainExtent; }
	VkSwapchainKHR& getSwapChain() { return swapChain; }
	std::vector<VkFramebuffer>& getSwapChainFramebuffers(int i) {
		return i < 1 ? swapChainFramebuffers : swapChainFramebuffers2;
	}

	std::vector<VkFramebuffer>& getQuadSwapChainFramebuffers() {
		return quadSwapChainFramebuffers;
	}

	std::vector<VkFramebuffer>& getBlurSwapChainFramebuffers() {
		return blurSwapChainFramebuffers;
	}

	uint32_t getSwapchainImageCount() { return swapChainImages.size(); }

	uint32_t getMinImageCount() { return minImageCount; }

	VkImage& getDepthImage() { return depthImage.getImage(); }
	VkDeviceMemory& getDepthImageMemory() { return depthImage.getImageMemory(); }
	VkImageView& getDepthImageView() { return depthImage.getImageView(); }

	VkImage& getDepthImage2() { return depthImage2.getImage(); }
	VkDeviceMemory& getDepthImageMemory2() { return depthImage2.getImageMemory(); }
	VkImageView& getDepthImageView2() { return depthImage2.getImageView(); }

	VkImage& getQuadDepthImage() { return quadDepthImage.getImage(); }
	VkDeviceMemory& getQuadDepthImageMemory() { return quadDepthImage.getImageMemory(); }
	VkImageView& getQuadDepthImageView() { return quadDepthImage.getImageView(); }

	VkImage& getTexCubeImage() { return texCube.getImage(); }
	VkDeviceMemory& getTexCubeImageMemory() { return texCube.getImageMemory(); }
	VkImageView& getTexCubeImageView() { return texCube.getImageView(); }

	VkImage& getBlurImage(size_t i) { return blurImage[i].getImage(); }
	VkDeviceMemory& getBlurImageMemory(size_t i) { return blurImage[i].getImageMemory(); }
	VkImageView& getBlurImageView(size_t i) { return blurImage[i].getImageView(); }

	VkImage& getBlurImage2(size_t i) { return blurImage2[i].getImage(); }
	VkDeviceMemory& getBlurImageMemory2(size_t i) { return blurImage2[i].getImageMemory(); }
	VkImageView& getBlurImageView2(size_t i) { return blurImage2[i].getImageView(); }
};
