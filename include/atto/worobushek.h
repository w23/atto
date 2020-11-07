#pragma once

// FIXME atto platform
#define VK_USE_PLATFORM_WAYLAND_KHR
#include <vulkan/vulkan.h>

#define COUNTOF(a) (sizeof(a)/sizeof(*(a)))

#define AVK_VERSION(v) \
	VK_VERSION_MAJOR(v), \
	VK_VERSION_MINOR(v), \
	VK_VERSION_PATCH(v)

#define AVK_CHECK_RESULT(res) { \
	const VkResult result = res; \
	if (result != VK_SUCCESS) { \
		aAppDebugPrintf("VK ERROR @ %s:%d: %s failed with result %d", __FILE__, __LINE__, #res, result); \
		aAppTerminate(-1); \
	} \
}

#define MAX_SWAPCHAIN_IMAGES 8

struct AVkSwapchain {
	VkSwapchainCreateInfoKHR info;
	VkSwapchainKHR handle;
	uint32_t num_images;
	VkImage images[MAX_SWAPCHAIN_IMAGES];
	uint32_t current_frame_image_index;
	VkSemaphore image_available;
};

struct AVkState {
	VkInstance inst;
	VkSurfaceKHR surf;

	VkPhysicalDevice phys_dev;
	VkPhysicalDeviceMemoryProperties mem_props;
	//VkSurfaceCapabilitiesKHR surf_caps;
	VkDevice dev;

	VkQueue main_queue;

	struct AVkSwapchain swapchain;
};

extern struct AVkState a_vk;

VkSemaphore aVkCreateSemaphore();
void aVkDestroySemaphore(VkSemaphore sema);

VkShaderModule loadShaderFromFile(const char *filename);
uint32_t aVkFindMemoryWithType(uint32_t type_index_bits, VkMemoryPropertyFlags flags);
