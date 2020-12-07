#pragma once

#ifdef ATTO_PLATFORM_WINDOWS
#define VK_USE_PLATFORM_WIN32_KHR
#else
// FIXME atto platform
#define VK_USE_PLATFORM_WAYLAND_KHR
#endif

#include "atto/app.h"

#include <vulkan/vulkan.h>

#include <stdio.h>
#include <stdlib.h>

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

void aVkCreateSurface();
void aVkPokePresentModes();
void aVkCreateSwapchain(int w, int h);
void aVkAcquireNextImage();
void aVkDestroy();
void aVkDestroySwapchain();

#if defined(ATTO_VK_IMPLEMENT)
static const char *instance_exts[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef ATTO_PLATFORM_WINDOWS
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#else
	VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME,
#endif
};

struct AVkState a_vk;

void aVkInitInstance() {
	uint32_t version = 0;
	AVK_CHECK_RESULT(vkEnumerateInstanceVersion(&version));
	aAppDebugPrintf("InstanceVersion: %u.%u.%u", AVK_VERSION(version));

	VkApplicationInfo ai;
	ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	ai.pNext = NULL;
	ai.apiVersion = VK_MAKE_VERSION(1, 0, 0);
	ai.applicationVersion = 0;
	ai.engineVersion = 0;
	ai.pApplicationName = "LOL";
	ai.pEngineName = "KEK";

	const char *layers[] = {
	#ifdef _DEBUG
		"VK_LAYER_KHRONOS_validation",
	#endif
	};

	VkInstanceCreateInfo vici;
	vici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vici.pNext = NULL;
	vici.enabledLayerCount = COUNTOF(layers);
	vici.flags = 0;
	vici.ppEnabledLayerNames = layers;
	vici.enabledExtensionCount = COUNTOF(instance_exts);
	vici.ppEnabledExtensionNames = instance_exts;
	vici.pApplicationInfo = &ai;

	AVK_CHECK_RESULT(vkCreateInstance(&vici, NULL, &a_vk.inst));
}

void aVkInitDevice() {
	// Get only the first device
	uint32_t num_devices = 1;
	vkEnumeratePhysicalDevices(a_vk.inst, &num_devices, &a_vk.phys_dev);

	uint32_t queue_index = UINT32_MAX;
	VkPhysicalDeviceProperties pd;
	vkGetPhysicalDeviceProperties(a_vk.phys_dev, &pd);

#define MAX_QUEUE_FAMILIES 8
	uint32_t num_queue_families = MAX_QUEUE_FAMILIES;
	VkQueueFamilyProperties queue_families[MAX_QUEUE_FAMILIES];
	vkGetPhysicalDeviceQueueFamilyProperties(a_vk.phys_dev, &num_queue_families, queue_families);

	for (uint32_t i = 0; i < num_queue_families; ++i) {
		if (!(queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
			continue;

		VkBool32 present = 0;
		vkGetPhysicalDeviceSurfaceSupportKHR(a_vk.phys_dev, i, a_vk.surf, &present);

		if (!present)
			continue;

		queue_index = i;
		break;
	}

	ATTO_ASSERT(queue_index < num_queue_families);

	vkGetPhysicalDeviceMemoryProperties(a_vk.phys_dev, &a_vk.mem_props);

	float prio = 1.f;
	VkDeviceQueueCreateInfo queue_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = queue_index,
		.queueCount = 1,
		.pQueuePriorities = &prio,
	};

	const char* devex[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};
	VkDeviceCreateInfo create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_info,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = COUNTOF(devex),
		.ppEnabledExtensionNames = devex,
		.pEnabledFeatures = NULL
	};

	AVK_CHECK_RESULT(vkCreateDevice(a_vk.phys_dev, &create_info, NULL, &a_vk.dev));

	vkGetDeviceQueue(a_vk.dev, 0, 0, &a_vk.main_queue);

	a_vk.swapchain.image_available = aVkCreateSemaphore();
}

void aVkCreateSwapchain(int w, int h) {
	struct AVkSwapchain* sw = &a_vk.swapchain;
	sw->info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	sw->info.pNext = NULL;
	sw->info.surface = a_vk.surf;
	sw->info.minImageCount = 5; // TODO get from caps
	sw->info.imageFormat = VK_FORMAT_B8G8R8A8_SRGB; // TODO get from caps
	sw->info.imageColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR; // TODO get from caps
	sw->info.imageExtent.width = w;
	sw->info.imageExtent.height = h;
	sw->info.imageArrayLayers = 1;
	sw->info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	sw->info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	sw->info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR; // TODO get from caps
	sw->info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	sw->info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO caps, MAILBOX is better
	//sw->info.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // TODO caps, MAILBOX is better
	sw->info.clipped = VK_TRUE;
	sw->info.oldSwapchain = sw->handle;

	AVK_CHECK_RESULT(vkCreateSwapchainKHR(a_vk.dev, &sw->info, NULL, &sw->handle));

	sw->num_images = 0;
	AVK_CHECK_RESULT(vkGetSwapchainImagesKHR(a_vk.dev, sw->handle, &sw->num_images, NULL));

	sw->num_images = sw->num_images > MAX_SWAPCHAIN_IMAGES ? MAX_SWAPCHAIN_IMAGES : sw->num_images;
	AVK_CHECK_RESULT(vkGetSwapchainImagesKHR(a_vk.dev, sw->handle, &sw->num_images, sw->images));

	aAppDebugPrintf("swapchain created: %d %d", w, h);
}

void aVkDestroySwapchain() {
	vkDestroySwapchainKHR(a_vk.dev, a_vk.swapchain.handle, NULL);
	a_vk.swapchain.handle = NULL;
}

void aVkCreateSurface() {
#if defined(ATTO_PLATFORM_WINDOWS)
	VkWin32SurfaceCreateInfoKHR wsci = {0};
	wsci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	wsci.hinstance = a_app_state->hInstance;
	wsci.hwnd = a_app_state->hWnd;
	AVK_CHECK_RESULT(vkCreateWin32SurfaceKHR(a_vk.inst, &wsci, NULL, &a_vk.surf));
#else
	VkWaylandSurfaceCreateInfoKHR wsci = {0};
	wsci.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	wsci.surface = surf;
	wsci.display = disp;
	AVK_CHECK_RESULT(vkCreateWaylandSurfaceKHR(a_vk.inst, &wsci, NULL, &a_vk.surf));
#endif
}

void aVkPokePresentModes() {
#define MAX_PRESENT_MODES 8
	uint32_t num_present_modes = MAX_PRESENT_MODES;
	VkPresentModeKHR present_modes[MAX_PRESENT_MODES];
	vkGetPhysicalDeviceSurfacePresentModesKHR(a_vk.phys_dev, a_vk.surf, &num_present_modes, present_modes);
	// TODO use the best mode
}

void aVkDestroy() {
	vkDestroySurfaceKHR(a_vk.inst, a_vk.surf, NULL);
	aVkDestroySemaphore(a_vk.swapchain.image_available);
	vkDestroyDevice(a_vk.dev, NULL);
	vkDestroyInstance(a_vk.inst, NULL);
}

void aVkAcquireNextImage() {
	AVK_CHECK_RESULT(vkAcquireNextImageKHR(a_vk.dev, a_vk.swapchain.handle, UINT64_MAX, a_vk.swapchain.image_available,
		NULL, &a_vk.swapchain.current_frame_image_index));
}

VkSemaphore aVkCreateSemaphore() {
	VkSemaphore sema;
	VkSemaphoreCreateInfo sci = {0};
	sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	sci.flags = 0;
	AVK_CHECK_RESULT(vkCreateSemaphore(a_vk.dev, &sci, NULL, &sema));
	return sema;
}

void aVkDestroySemaphore(VkSemaphore sema) {
	vkDestroySemaphore(a_vk.dev, sema, NULL);
}

VkShaderModule loadShaderFromFile(const char *filename) {
	FILE *f = fopen(filename, "rb");
	if (!f) {
		perror(filename);
		exit(1);
	}
	fseek(f, 0, SEEK_END);
	const size_t size = ftell(f);
	if (size % 4 != 0) {
		aAppDebugPrintf("size %zu is not aligned to 4 bytes as required by SPIR-V/Vulkan spec", size);
	}

	fseek(f, 0, SEEK_SET);
	uint32_t *buf = malloc(4 * ((size+3)/4));
	if (size != fread(buf, 1, size, f)) {
		fprintf(stderr, "Cannot read the entire file %s\n", filename);
		exit(1);
	}

	VkShaderModuleCreateInfo smci = {0};
	smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	smci.codeSize = size;
	smci.pCode = buf;
	VkShaderModule shader;
	const VkResult res = vkCreateShaderModule(a_vk.dev, &smci, NULL, &shader);
	if (VK_SUCCESS != res) { \
		aAppDebugPrintf("%s:%d: failed w/ %d\n", __FILE__, __LINE__, res); \
		exit(1);
	}
	fclose(f);
	free(buf);
	return shader;
}

uint32_t aVkFindMemoryWithType(uint32_t type_index_bits, VkMemoryPropertyFlags flags) {
	for (uint32_t i = 0; i < a_vk.mem_props.memoryTypeCount; ++i) {
		if (!(type_index_bits & (1 << i)))
			continue;

		if ((a_vk.mem_props.memoryTypes[i].propertyFlags & flags) == flags)
			return i;
	}

	return UINT32_MAX;
}
#endif
