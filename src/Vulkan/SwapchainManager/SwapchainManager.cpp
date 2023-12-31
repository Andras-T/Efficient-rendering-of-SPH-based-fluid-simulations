#include "SwapchainManager.h"
#include "Vulkan/Utils/Utils.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

void SwapchainManager::init(Window& window, VkSurfaceKHR& surface,
	VkPhysicalDevice& physicalDevice, VkDevice& device,
	QueueFamilyIndices& indices) {
	this->window = &window;
	this->surface = &surface;
	this->physicalDevice = &physicalDevice;
	this->device = &device;
	this->indices = &indices;
	blurImage1.reserve(Utils::MAX_FRAMES_IN_FLIGHT);
	blurImage2.reserve(Utils::MAX_FRAMES_IN_FLIGHT);
	blurVolumeImage1.reserve(Utils::MAX_FRAMES_IN_FLIGHT);
	blurVolumeImage2.reserve(Utils::MAX_FRAMES_IN_FLIGHT);
	blurImage1.push_back(Image());
	blurImage1.push_back(Image());
	blurImage2.push_back(Image());
	blurImage2.push_back(Image());
	blurVolumeImage1.push_back(Image());
	blurVolumeImage1.push_back(Image());
	blurVolumeImage2.push_back(Image());
	blurVolumeImage2.push_back(Image());
	createSwapChain();
	logger.LogInfo("Swap chain created");
	createImageViews();
	logger.LogInfo("Image views created");
}

SwapChainSupportDetails
SwapchainManager::querySwapChainSupport(VkPhysicalDevice* phDevice,
	VkSurfaceKHR& surface) {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
		*phDevice, surface, &swapChainSupportDetails.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(*phDevice, surface, &formatCount,
		nullptr);

	if (formatCount != 0) {
		swapChainSupportDetails.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(
			*phDevice, surface, &formatCount,
			swapChainSupportDetails.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(*phDevice, surface,
		&presentModeCount, nullptr);

	if (presentModeCount != 0) {
		swapChainSupportDetails.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(
			*phDevice, surface, &presentModeCount,
			swapChainSupportDetails.presentModes.data());
	}

	return swapChainSupportDetails;
}

void SwapchainManager::createSwapChain() {
	SwapChainSupportDetails swapChainSupport =
		querySwapChainSupport(physicalDevice, *surface);

	VkSurfaceFormatKHR surfaceFormat =
		chooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode =
		chooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) {
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = *surface;
	minImageCount = imageCount;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32_t queueFamilyIndices[] = { indices->graphicsAndComputeFamily.value(),
																	 indices->presentFamily.value() };

	if (indices->graphicsAndComputeFamily != indices->presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	if (vkCreateSwapchainKHR(*device, &createInfo, nullptr, &swapChain) !=
		VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(*device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(*device, swapChain, &imageCount,
		swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;
}

void SwapchainManager::createDepthResources(
	CommandPoolManager& commandPoolManager, VkQueue& graphicsQueue) {
	// Depth Image
		{
			VkFormat depthFormat = findDepthFormat();
			depthImage.createImage(swapChainExtent.width, swapChainExtent.height,
				depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device,
				*physicalDevice);

			depthImage.createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, *device);

			volumeImage.createImage(swapChainExtent.width, swapChainExtent.height,
				depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device,
				*physicalDevice);

			volumeImage.createImageView(depthFormat, VK_IMAGE_ASPECT_STENCIL_BIT, *device);

			quadDepthImage.createImage(swapChainExtent.width, swapChainExtent.height,
				depthFormat, VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device,
				*physicalDevice);

			quadDepthImage.createImageView(depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, *device);

			logger.LogInfo("Depth Image and View created (" +
				std::to_string(swapChainExtent.width) + ", " +
				std::to_string(swapChainExtent.height) + ")");
		}
		// Blur Image
		{
			for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++)
			{
			VkFormat blurFormat = VK_FORMAT_R32_SFLOAT;
			blurImage1[i].createImage(
				swapChainExtent.width, swapChainExtent.height, blurFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device, *physicalDevice);
			blurImage2[i].createImage(
				swapChainExtent.width, swapChainExtent.height, blurFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device, *physicalDevice);

			blurImage1[i].createImageView(blurFormat,
				VK_IMAGE_ASPECT_COLOR_BIT, *device);
			blurImage2[i].createImageView(blurFormat,
				VK_IMAGE_ASPECT_COLOR_BIT, *device);

			commandPoolManager.transitionImageLayout(blurImage1[i].getImage(), blurFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);
			commandPoolManager.transitionImageLayout(blurImage2[i].getImage(), blurFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);
			
			VkFormat blurVolumeFormat = blurFormat;

			blurVolumeImage1[i].createImage(
				swapChainExtent.width, swapChainExtent.height, blurVolumeFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device, *physicalDevice);
			blurVolumeImage2[i].createImage(
				swapChainExtent.width, swapChainExtent.height, blurVolumeFormat,
				VK_IMAGE_TILING_OPTIMAL,
				VK_IMAGE_USAGE_STORAGE_BIT |
				VK_IMAGE_USAGE_SAMPLED_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, *device, *physicalDevice);

			blurVolumeImage1[i].createImageView(blurVolumeFormat,
				VK_IMAGE_ASPECT_COLOR_BIT, *device);
			blurVolumeImage2[i].createImageView(blurVolumeFormat,
				VK_IMAGE_ASPECT_COLOR_BIT, *device);

			commandPoolManager.transitionImageLayout(blurVolumeImage1[i].getImage(), blurVolumeFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);
			commandPoolManager.transitionImageLayout(blurVolumeImage2[i].getImage(), blurVolumeFormat,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_GENERAL);
			}
			logger.LogInfo("Blur Images & Views created (" +
				std::to_string(swapChainExtent.width) + ", " +
				std::to_string(swapChainExtent.height) + ")");
		}
		// TexCube Images
		{
		texCube.createTextureImages(*device, *physicalDevice, commandPoolManager,
			graphicsQueue);
		logger.LogInfo("TexCube Images & View created (" +
			std::to_string(swapChainExtent.width) + ", " +
			std::to_string(swapChainExtent.height) + ")");
		}
}

void SwapchainManager::createFramebuffers(VulkanObject& vulkanObject) {
	this->vulkanObject = &vulkanObject;
	// Simulation framebuffer
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		swapChainFramebuffers2.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 2> attachments = { swapChainImageViews[i],
																								depthImage.getImageView() };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vulkanObject.getSimulationRenderPass();
			framebufferInfo.attachmentCount =
				static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr,
				&swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
			attachments = { swapChainImageViews[i], volumeImage.getImageView() };
			framebufferInfo.pAttachments = attachments.data();
			if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr,
				&swapChainFramebuffers2[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	// Blur framebuffer
	{
		blurSwapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 1> attachments = { swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vulkanObject.getBlurRenderPass();
			framebufferInfo.attachmentCount =
				static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr,
				&blurSwapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create blur framebuffer!");
			}
		}
	}

	// Quad framebuffer
	{
		quadSwapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<VkImageView, 2> attachments = { swapChainImageViews[i], quadDepthImage.getImageView() };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = vulkanObject.getQuadRenderPass();
			framebufferInfo.attachmentCount =
				static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr,
				&quadSwapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create quad framebuffer!");
			}
		}
	}
	logger.LogInfo("Framebuffers created");
}

void SwapchainManager::recreateSwapChain(GLFWwindow* window, VkDevice& device,
	CommandPoolManager& commandPoolManager,
	VkQueue& graphicsQueue) {
	int width = 0, height = 0;
	glfwGetFramebufferSize(window, &width, &height);
	while (width == 0 || height == 0) {
		glfwGetFramebufferSize(window, &width, &height);
		glfwWaitEvents();
	}
	std::cout << "\n";
	logger.LogInfo("Recreating the swap chain");
	vkDeviceWaitIdle(device);

	cleanupSwapChain();

	createSwapChain();
	createImageViews();
	createDepthResources(commandPoolManager, graphicsQueue);
	createFramebuffers(*vulkanObject);
	logger.LogInfo("Swap chain recreated");
}

void SwapchainManager::cleanupSwapChain() {
	// Destroy Images
	{
		vkDestroyImage(*device, getDepthImage(), nullptr);
		vkDestroyImage(*device, getVolumeImage(), nullptr);
		vkDestroyImage(*device, getQuadDepthImage(), nullptr);

		vkDestroyImageView(*device, getDepthImageView(), nullptr);
		vkDestroyImageView(*device, getVolumeImageView(), nullptr);
		vkDestroyImageView(*device, getQuadDepthImageView(), nullptr);

		vkFreeMemory(*device, getDepthImageMemory(), nullptr);
		vkFreeMemory(*device, getVolumeImageMemory(), nullptr);
		vkFreeMemory(*device, getQuadDepthImageMemory(), nullptr);

		for (size_t i = 0; i < Utils::MAX_FRAMES_IN_FLIGHT; i++) {

			vkDestroyImage(*device, getBlurImage1(i), nullptr);
			vkDestroyImage(*device, getBlurImage2(i), nullptr);
			vkDestroyImage(*device, getBlurVolumeImage1(i), nullptr);
			vkDestroyImage(*device, getBlurVolumeImage2(i), nullptr);

			vkDestroyImageView(*device, getBlurImageView1(i), nullptr);
			vkDestroyImageView(*device, getBlurImageView2(i), nullptr);
			vkDestroyImageView(*device, getBlurVolumeImageView1(i), nullptr);
			vkDestroyImageView(*device, getBlurVolumeImageView2(i), nullptr);

			vkFreeMemory(*device, getBlurImageMemory1(i), nullptr);
			vkFreeMemory(*device, getBlurImageMemory2(i), nullptr);
			vkFreeMemory(*device, getBlurVolumeImageMemory1(i), nullptr);
			vkFreeMemory(*device, getBlurVolumeImageMemory2(i), nullptr);
		}

		texCube.cleanUp(*device);
	}
	// Destroy Framebuffers
	{
	for (auto& framebuffer : swapChainFramebuffers)
		vkDestroyFramebuffer(*device, framebuffer, nullptr);

	for (auto& framebuffer : swapChainFramebuffers2)
		vkDestroyFramebuffer(*device, framebuffer, nullptr);

	for (auto& framebuffer : blurSwapChainFramebuffers)
		vkDestroyFramebuffer(*device, framebuffer, nullptr);

	for (auto& framebuffer : quadSwapChainFramebuffers)
		vkDestroyFramebuffer(*device, framebuffer, nullptr);
	}
	// Destroy image view
	for (auto& imageView : swapChainImageViews) {
		vkDestroyImageView(*device, imageView, nullptr);
	}

	vkDestroySwapchainKHR(*device, swapChain, nullptr);
}

VkFormat SwapchainManager::findDepthFormat() {
	return findSupportedFormat(
		{VK_FORMAT_D32_SFLOAT_S8_UINT,
		 VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

void SwapchainManager::createImageViews() {
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = swapChainImages[i];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = swapChainImageFormat;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;

		if (vkCreateImageView(*device, &createInfo, nullptr,
			&swapChainImageViews[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create image views!");
		}
	}
}

VkSurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(
	const std::vector<VkSurfaceFormatKHR>& availableFormats) {
	for (const auto& availableFormat : availableFormats) {
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
			logger.LogInfo(
				"Chosen surface format: VK_COLOR_SPACE_SRGB_NONLINEAR_KHR");
			return availableFormat;
		}
	}
	logger.LogInfo("Chosen surface format: default {" +
		availableFormats[0].format + '}');
	return availableFormats[0];
}

VkPresentModeKHR SwapchainManager::chooseSwapPresentMode(
	const std::vector<VkPresentModeKHR>& availablePresentModes) {
	for (const auto& availablePresentMode : availablePresentModes) {
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
			logger.LogInfo("Chosen presentation mode: VK_PRESENT_MODE_MAILBOX_KHR");
			return availablePresentMode;
		}
	}
	logger.LogInfo("Chosen presentation mode: VK_PRESENT_MODE_FIFO_KHR");
	return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainManager::chooseSwapExtent(
	const VkSurfaceCapabilitiesKHR& capabilities) {
	if (capabilities.currentExtent.width !=
		std::numeric_limits<uint32_t>::max()) {
		return capabilities.currentExtent;
	}
	else {
		int width, height;

		glfwGetFramebufferSize(window->get_GLFW_Window(), &width, &height);

		VkExtent2D actualExtent = { static_cast<uint32_t>(width),
															 static_cast<uint32_t>(height) };

		actualExtent.width =
			std::clamp(actualExtent.width, capabilities.minImageExtent.width,
				capabilities.maxImageExtent.width);
		actualExtent.height =
			std::clamp(actualExtent.height, capabilities.minImageExtent.height,
				capabilities.maxImageExtent.height);

		return actualExtent;
	}
}

VkFormat
SwapchainManager::findSupportedFormat(const std::vector<VkFormat>& candidates,
	VkImageTiling tiling,
	VkFormatFeatureFlags features) {
	for (VkFormat format : candidates) {
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(*physicalDevice, format, &props);

		if (tiling == VK_IMAGE_TILING_LINEAR &&
			(props.linearTilingFeatures & features) == features) {
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
			(props.optimalTilingFeatures & features) == features) {
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}
