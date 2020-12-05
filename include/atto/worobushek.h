#pragma once

#include "atto/platform.h"

#ifdef ATTO_PLATFORM_WINDOWS
#define LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#define VK_USE_PLATFORM_WIN32_KHR
#else
// FIXME atto platform
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include <vulkan/vulkan.h>

#ifndef COUNTOF
#define COUNTOF(a) (sizeof(a)/sizeof(*(a)))
#endif

#define AVK_VERSION(v) \
	VK_VERSION_MAJOR(v), \
	VK_VERSION_MINOR(v), \
	VK_VERSION_PATCH(v)

#define AVK_CHECK_RESULT(res) do { \
	const VkResult result = res; \
	if (result != VK_SUCCESS) { \
		aAppDebugPrintf("VK ERROR @ %s:%d: %s failed with result %d", __FILE__, __LINE__, #res, result); \
		aAppTerminate(-1); \
	} \
} while(0)

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

// FIXME declare this somewhere properly
#include "atto/app.h"
#if defined(ATTO_PLATFORM_WINDOWS)
void a_vkInitWithWindows(HINSTANCE hInst, HWND hWnd);
#else
void a_vkInitWithWayland(struct wl_display *disp, struct wl_surface *surf);
#endif
void a_vkCreateSwapchain(int w, int h);
void a_vkPaint(struct AAppProctable *ptbl, ATimeUs t, float dt);
void a_vkDestroy();
void a_vkDestroySwapchain();
