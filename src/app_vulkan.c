#include "atto/worobushek.h"
#include "atto/app.h"

#include <stdio.h>
#include <stdlib.h>

static const char *instance_exts[] = {
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME
};

struct AVkState a_vk;

void a_vkInitInstance() {
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

	VkInstanceCreateInfo vici;
	vici.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	vici.pNext = NULL;
	vici.enabledLayerCount = 0;
	vici.flags = 0;
	vici.ppEnabledLayerNames = NULL;
	vici.enabledExtensionCount = COUNTOF(instance_exts);
	vici.ppEnabledExtensionNames = instance_exts;
	vici.pApplicationInfo = &ai;

	AVK_CHECK_RESULT(vkCreateInstance(&vici, NULL, &a_vk.inst));
}

void a_vkInitDevice() {
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

void a_vkCreateSwapchain(int w, int h) {
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

	AVK_CHECK_RESULT(vkCreateSwapchainKHR(a_vk.dev, &sw->info, NULL, &sw->handle));

	sw->num_images = 0;
	AVK_CHECK_RESULT(vkGetSwapchainImagesKHR(a_vk.dev, sw->handle, &sw->num_images, NULL));

	sw->num_images = sw->num_images > MAX_SWAPCHAIN_IMAGES ? MAX_SWAPCHAIN_IMAGES : sw->num_images;
	AVK_CHECK_RESULT(vkGetSwapchainImagesKHR(a_vk.dev, sw->handle, &sw->num_images, sw->images));

	aAppDebugPrintf("swapchain created: %d %d", w, h);
}

void a_vkDestroySwapchain() {
	vkDestroySwapchainKHR(a_vk.dev, a_vk.swapchain.handle, NULL);
	a_vk.swapchain.handle = NULL;
}

void a_vkInitWithWayland(struct wl_display *disp, struct wl_surface *surf) {
	a_vkInitInstance();

	VkWaylandSurfaceCreateInfoKHR wsci = {0};
	wsci.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
	wsci.surface = surf;
	wsci.display = disp;
	AVK_CHECK_RESULT(vkCreateWaylandSurfaceKHR(a_vk.inst, &wsci, NULL, &a_vk.surf));

	a_vkInitDevice();

#define MAX_PRESENT_MODES 8
	uint32_t num_present_modes = MAX_PRESENT_MODES;
	VkPresentModeKHR present_modes[MAX_PRESENT_MODES];
	vkGetPhysicalDeviceSurfacePresentModesKHR(a_vk.phys_dev, a_vk.surf, &num_present_modes, present_modes);
	// TODO use the best mode
}

void a_vkDestroy() {
	vkDestroySurfaceKHR(a_vk.inst, a_vk.surf, NULL);
	aVkDestroySemaphore(a_vk.swapchain.image_available);
	vkDestroyDevice(a_vk.dev, NULL);
	vkDestroyInstance(a_vk.inst, NULL);
}

void a_vkPaint(struct AAppProctable *ptbl, ATimeUs t, float dt) {
		AVK_CHECK_RESULT(vkAcquireNextImageKHR(a_vk.dev, a_vk.swapchain.handle, UINT64_MAX, a_vk.swapchain.image_available, NULL, &a_vk.swapchain.current_frame_image_index));

		ptbl->paint(t, dt);

		/* VkPresentInfoKHR presinfo = {0}; */
		/* presinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR; */
		/* presinfo.pSwapchains = &a_vk.swapchain.handle; */
		/* presinfo.pImageIndices = &a_vk.swapchain.current_frame_image_index; */
		/* presinfo.swapchainCount = 1; */
		/* presinfo.pWaitSemaphores = &semaDone; */
		/* presinfo.waitSemaphoreCount = 1; */
		/* AVK_CHECK_RESULT(vkQueuePresentKHR(a_vk.main_queue, &presinfo)); */
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
