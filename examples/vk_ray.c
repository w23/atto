#include "atto/app.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define ATTO_VK_INSTANCE_EXTENSIONS \
		VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, \

#define ATTO_VK_DEVICE_EXTENSIONS \
		VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, \
		VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, \
		VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, \

#define AVK_VK_VERSION VK_MAKE_VERSION(1, 2, 0)

#define ATTO_VK_IMPLEMENT
#include "atto/worobushek.h"

//struct Vertex {
//	float x, y;
//	uint8_t r, g, b;
//};

struct Image {
	VkDeviceMemory devmem;
	VkImage image;
	VkImageView view;
};

static struct Image createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage) {
	VkImageCreateInfo ici = {.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	ici.imageType = VK_IMAGE_TYPE_2D;
	ici.extent.width = width;
	ici.extent.height = height;
	ici.extent.depth = 1;
	ici.mipLevels = 1;
	ici.arrayLayers = 1;
	ici.format = format;
	ici.tiling = tiling;
	ici.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	ici.usage = usage;
	ici.samples = VK_SAMPLE_COUNT_1_BIT;
	ici.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	struct Image image;
	AVK_CHECK_RESULT(vkCreateImage(a_vk.dev, &ici, NULL, &image.image));

	VkMemoryRequirements memreq;
	vkGetImageMemoryRequirements(a_vk.dev, image.image, &memreq);

	VkMemoryAllocateInfo mai={.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	mai.allocationSize = memreq.size;
	mai.memoryTypeIndex = aVkFindMemoryWithType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	AVK_CHECK_RESULT(vkAllocateMemory(a_vk.dev, &mai, NULL, &image.devmem));
	AVK_CHECK_RESULT(vkBindImageMemory(a_vk.dev, image.image, image.devmem, 0));

	VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
	ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
	ivci.format = ici.format;
	ivci.image = image.image;
	ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	ivci.subresourceRange.levelCount = 1;
	ivci.subresourceRange.layerCount = 1;
	AVK_CHECK_RESULT(vkCreateImageView(a_vk.dev, &ivci, NULL, &image.view));

	return image;
}

static struct {
	struct {
		VkPhysicalDeviceProperties2 prop2;
		VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt;
	} prop;
	uint32_t shader_group_size;

	VkFence fence;
	VkSemaphore done;
	VkRenderPass render_pass;

	VkPipelineLayout pipeline_layout;
	struct {
		VkShaderModule raygen;
	} modules;
	VkDescriptorSetLayout desc_layout;
	VkDescriptorPool desc_pool;
	VkDescriptorSet desc_set;

	VkImageView *image_views;
	VkFramebuffer *framebuffers;
	VkPipeline pipeline;
	struct Image rt_image;

	VkDeviceMemory devmem;
	VkBuffer sbt_buf;
	//VkBuffer vertex_buf;

	VkCommandPool cmdpool;
	VkCommandBuffer cmdbuf;
} g;

static void createRenderPass() {
	VkAttachmentDescription attachment = {0};
	attachment.format = VK_FORMAT_B8G8R8A8_SRGB;// FIXME too early a_vk.swapchain.info.imageFormat;
	attachment.samples = VK_SAMPLE_COUNT_1_BIT;
	//attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	//attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference color_attachment = {0};
	color_attachment.attachment = 0;
	color_attachment.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subdesc = {0};
	subdesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subdesc.colorAttachmentCount = 1;
	subdesc.pColorAttachments = &color_attachment;

	VkRenderPassCreateInfo rpci = {0};
	rpci.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	rpci.attachmentCount = 1;
	rpci.pAttachments = &attachment;
	rpci.subpassCount = 1;
	rpci.pSubpasses = &subdesc;
	AVK_CHECK_RESULT(vkCreateRenderPass(a_vk.dev, &rpci, NULL, &g.render_pass));
}

static void createLayouts() {

  VkDescriptorSetLayoutBinding bindings[] = {
		{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},
		/*{.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		},*/
	};

	VkDescriptorSetLayoutCreateInfo dslci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = COUNTOF(bindings), .pBindings = bindings, };

	AVK_CHECK_RESULT(vkCreateDescriptorSetLayout(a_vk.dev, &dslci, NULL, &g.desc_layout));

	/*
	VkPushConstantRange push_const = {0};
	push_const.offset = 0;
	push_const.size = sizeof(float);
	push_const.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	*/

	VkPipelineLayoutCreateInfo plci = {0};
	plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plci.setLayoutCount = 1;
	plci.pSetLayouts = &g.desc_layout;
	// plci.pushConstantRangeCount = 1;
	// plci.pPushConstantRanges = &push_const;
	AVK_CHECK_RESULT(vkCreatePipelineLayout(a_vk.dev, &plci, NULL, &g.pipeline_layout));

	VkDescriptorPoolSize pools[] = { {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},};

	VkDescriptorPoolCreateInfo dpci = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.maxSets = 1, .poolSizeCount = COUNTOF(pools), .pPoolSizes = pools,
	};
	AVK_CHECK_RESULT(vkCreateDescriptorPool(a_vk.dev, &dpci, NULL, &g.desc_pool));

	VkDescriptorSetAllocateInfo dsai = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.descriptorPool = g.desc_pool,
		.descriptorSetCount = 1,
		.pSetLayouts = &g.desc_layout,
	};
	AVK_CHECK_RESULT(vkAllocateDescriptorSets(a_vk.dev, &dsai, &g.desc_set));

	g.modules.raygen = loadShaderFromFile("ray.rgen.spv");
}

//static const struct Vertex vertices[] = {
//	{1.f, 0.f, 255, 0, 0},
//	{-.5f, .866f, 255, 255, 0},
//	{-.5f, -.866f, 255, 0, 255},
//};
//
//static void createBuffer() {
//	VkBufferCreateInfo bci = {0};
//	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
//	bci.size = sizeof(vertices);
//	bci.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
//	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
//	AVK_CHECK_RESULT(vkCreateBuffer(a_vk.dev, &bci, NULL, &g.vertex_buf));
//
//	VkMemoryRequirements memreq;
//	vkGetBufferMemoryRequirements(a_vk.dev, g.vertex_buf, &memreq);
//	aAppDebugPrintf("memreq: memoryTypeBits=0x%x alignment=%zu size=%zu", memreq.memoryTypeBits, memreq.alignment, memreq.size);
//
//	VkMemoryAllocateInfo mai={0};
//	mai.allocationSize = memreq.size;
//	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
//	mai.memoryTypeIndex = aVkFindMemoryWithType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
//	AVK_CHECK_RESULT(vkAllocateMemory(a_vk.dev, &mai, NULL, &g.devmem));
//	AVK_CHECK_RESULT(vkBindBufferMemory(a_vk.dev, g.vertex_buf, g.devmem, 0));
//
//	void *ptr = NULL;
//	AVK_CHECK_RESULT(vkMapMemory(a_vk.dev, g.devmem, 0, bci.size, 0, &ptr));
//		memcpy(ptr, vertices, sizeof(vertices));
//	vkUnmapMemory(a_vk.dev, g.devmem);
//}

static void createCommandPool() {
	VkCommandPoolCreateInfo cpci = {0};
	cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cpci.queueFamilyIndex = 0;
	cpci.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	AVK_CHECK_RESULT(vkCreateCommandPool(a_vk.dev, &cpci, NULL, &g.cmdpool));

	VkCommandBufferAllocateInfo cbai = {0};
	cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cbai.commandBufferCount = 1;
	cbai.commandPool = g.cmdpool;
	cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	AVK_CHECK_RESULT(vkAllocateCommandBuffers(a_vk.dev, &cbai, &g.cmdbuf));
}

static void createPipeline() {
	VkDynamicState dynstate[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

	VkPipelineDynamicStateCreateInfo pdsci = {.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO};
	pdsci.dynamicStateCount = COUNTOF(dynstate);
	pdsci.pDynamicStates = dynstate;

	VkPipelineShaderStageCreateInfo shader_stages[] = {{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		.module = g.modules.raygen,
		.pName = "main",
	}, };

	VkRayTracingShaderGroupCreateInfoKHR groups[] = {{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = 0,
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	},
	};

	VkRayTracingPipelineCreateInfoKHR rtci = {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
	rtci.flags = 0; //VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR
	rtci.stageCount = COUNTOF(shader_stages);
	rtci.pStages = shader_stages;
	rtci.groupCount = COUNTOF(groups);
	rtci.pGroups = groups;
	//rtci.pDynamicState = &pdsci;
	rtci.layout = g.pipeline_layout;
	rtci.maxPipelineRayRecursionDepth = 1;

	AVK_CHECK_RESULT(
		AVK_DEV_FUNC(vkCreateRayTracingPipelinesKHR)(a_vk.dev, VK_NULL_HANDLE, VK_NULL_HANDLE, 1, &rtci, NULL, &g.pipeline));
	ATTO_ASSERT(g.pipeline);

	//FIXME static_assert(COUNTOF(groups) == 1, "");
	//FIXME max(shaderGroupHandleSize, alignment)
	const uint32_t sbt_size = g.shader_group_size * rtci.groupCount;
	VkBufferCreateInfo bci = {0};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = sbt_size;
	bci.usage = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	AVK_CHECK_RESULT(vkCreateBuffer(a_vk.dev, &bci, NULL, &g.sbt_buf));

	VkMemoryRequirements memreq;
	vkGetBufferMemoryRequirements(a_vk.dev, g.sbt_buf, &memreq);
	aAppDebugPrintf("memreq: memoryTypeBits=0x%x alignment=%zu size=%zu", memreq.memoryTypeBits, memreq.alignment, memreq.size);

	VkMemoryAllocateInfo mai={0};
	mai.allocationSize = memreq.size;
	mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	mai.memoryTypeIndex = aVkFindMemoryWithType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	AVK_CHECK_RESULT(vkAllocateMemory(a_vk.dev, &mai, NULL, &g.devmem));
	AVK_CHECK_RESULT(vkBindBufferMemory(a_vk.dev, g.sbt_buf, g.devmem, 0));

	void *ptr = NULL;
	AVK_CHECK_RESULT(vkMapMemory(a_vk.dev, g.devmem, 0, bci.size, 0, &ptr));
	AVK_CHECK_RESULT(
		AVK_DEV_FUNC(vkGetRayTracingShaderGroupHandlesKHR)(a_vk.dev, g.pipeline, 0, rtci.groupCount, sbt_size, ptr));
	vkUnmapMemory(a_vk.dev, g.devmem);
}

static void swapchainWillDestroy() {
	//vkDestroyPipeline(a_vk.dev, g.pipeline, NULL);
	for (uint32_t i = 0; i < a_vk.swapchain.num_images; ++i) {
		vkDestroyFramebuffer(a_vk.dev, g.framebuffers[i], NULL);
		vkDestroyImageView(a_vk.dev, g.image_views[i], NULL);
	}
}

static void swapchainCreated() {
	const uint32_t num_images = a_vk.swapchain.num_images;
	g.image_views = malloc(num_images * sizeof(VkImageView));
	g.framebuffers = malloc(num_images * sizeof(VkFramebuffer));
	for (uint32_t i = 0; i < num_images; ++i) {
		VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = a_vk.swapchain.info.imageFormat;
		ivci.image = a_vk.swapchain.images[i];
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.layerCount = 1;
		AVK_CHECK_RESULT(vkCreateImageView(a_vk.dev, &ivci, NULL, g.image_views + i));

		VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		fbci.renderPass = g.render_pass;
		fbci.attachmentCount = 1;
		fbci.pAttachments = g.image_views + i;
		fbci.width = a_vk.surface_width;
		fbci.height = a_vk.surface_height;
		fbci.layers = 1;
		AVK_CHECK_RESULT(vkCreateFramebuffer(a_vk.dev, &fbci, NULL, g.framebuffers + i));
	}
}

static PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR_;
static void paint(ATimeUs timestamp, float dt) {
	const float t = timestamp / 1e6f;

	aVkAcquireNextImage();

					const VkImageSubresourceRange subrange = {
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					};

	VkCommandBufferBeginInfo beginfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	AVK_CHECK_RESULT(vkBeginCommandBuffer(g.cmdbuf, &beginfo));

	VkClearValue clear_value = {0};
	clear_value.color.float32[0] = .5f + .5f * sinf(t);
	clear_value.color.float32[1] = .5f + .5f * sinf(t*3);
	clear_value.color.float32[2] = .5f + .5f * sinf(t*5);
	clear_value.color.float32[3] = 1.f;
	/* clear_value.color.uint32[0] = 0xffffffff; */
	/* clear_value.color.uint32[3] = 0xffffffff; */
	VkRenderPassBeginInfo rpbi = {.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	rpbi.renderPass = g.render_pass;
	rpbi.framebuffer = g.framebuffers[a_vk.swapchain.current_frame_image_index];
	rpbi.renderArea.offset.x = rpbi.renderArea.offset.y = 0;
	rpbi.renderArea.extent.width = a_app_state->width;
	rpbi.renderArea.extent.height = a_app_state->height;
	rpbi.clearValueCount = 1;
	rpbi.pClearValues = &clear_value;
	//vkCmdBeginRenderPass(g.cmdbuf, &rpbi, VK_SUBPASS_CONTENTS_INLINE);

	#if 0
	vkCmdBindPipeline(g.cmdbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, g.pipeline);

	VkDeviceSize offset = {0};
	vkCmdBindVertexBuffers(g.cmdbuf, 0, 1, &g.vertex_buf, &offset);
		vkCmdPushConstants(g.cmdbuf, g.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float), &t);
		vkCmdDraw(g.cmdbuf, 3, 1, 0, 0);
	/* const int N = 32; */
	/* for (int i = 0; i < N; ++i) { */
	/* 	const float it = i / (float)N; */
	/* 	const float tt = t*(.8+it*.2) + it * ((1. + sin(t*.3))*.15 + .7) * 20.; */
	/* 	vkCmdPushConstants(cmdbuf, pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(float), &tt); */
	/* 	vkCmdDraw(cmdbuf, 3, 1, 0, 0); */
	/* } */
		#endif
	//vkCmdEndRenderPass(g.cmdbuf);

   {
		VkImageMemoryBarrier image_barriers[] = {
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.image = g.rt_image.image,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
				//.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT, // VK_ACCESS_SHADER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_GENERAL,
				.subresourceRange =
					(VkImageSubresourceRange){
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					},
			}};
		vkCmdPipelineBarrier(g.cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR, 0, 0,
		//vkCmdPipelineBarrier(g.cmdbuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0,
			NULL, 0, NULL, COUNTOF(image_barriers), image_barriers);
	}

	 //vkCmdClearColorImage(g.cmdbuf, g.rt_image.image, VK_IMAGE_LAYOUT_GENERAL, &clear_value.color, 1, &subrange);
	vkCmdBindPipeline(g.cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, g.pipeline);

	VkDescriptorImageInfo dii = {
		.sampler = VK_NULL_HANDLE,
		.imageView = g.rt_image.view,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	VkWriteDescriptorSet wds[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.dstSet = g.desc_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.pImageInfo = &dii,
		},
	};
	vkUpdateDescriptorSets(a_vk.dev, COUNTOF(wds), wds, 0, NULL);
	vkCmdBindDescriptorSets(g.cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, g.pipeline_layout, 0, 1, &g.desc_set, 0, NULL);

	const VkStridedDeviceAddressRegionKHR sbt_null = {0};
	const VkBufferDeviceAddressInfo bdai = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = g.sbt_buf};
	const VkDeviceAddress sbt_addr = vkGetBufferDeviceAddress(a_vk.dev, &bdai);
	const VkStridedDeviceAddressRegionKHR sbt_raygen = {.deviceAddress = sbt_addr, .size = g.shader_group_size, .stride = g.shader_group_size};
	vkCmdTraceRaysKHR_(g.cmdbuf, &sbt_raygen, &sbt_null, &sbt_null, &sbt_null, 1280, 720, 1);

		VkImageMemoryBarrier image_barriers[] = {
		{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.image = g.rt_image.image,
		.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
		.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_GENERAL,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		.subresourceRange =
			(VkImageSubresourceRange){
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
		},
				{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.image = a_vk.swapchain.images[a_vk.swapchain.current_frame_image_index],
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
		.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		.subresourceRange =
			(VkImageSubresourceRange){
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1,
			},
	}};
		vkCmdPipelineBarrier(g.cmdbuf,
				VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, NULL, 0, NULL, COUNTOF(image_barriers), image_barriers);

	VkImageBlit region = {0};
	region.srcOffsets[1].x = 1280;
	region.srcOffsets[1].y = 720;
	region.srcOffsets[1].z = 1;
	region.dstOffsets[1].x = 1280 < a_app_state->width ? 1280 : a_app_state->width;
	region.dstOffsets[1].y = 720 < a_app_state->height ? 720 : a_app_state->height;
	region.dstOffsets[1].z = 1;
	region.srcSubresource.aspectMask = region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.srcSubresource.layerCount = region.dstSubresource.layerCount = 1;
	vkCmdBlitImage(g.cmdbuf, g.rt_image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		a_vk.swapchain.images[a_vk.swapchain.current_frame_image_index], VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region,
		VK_FILTER_NEAREST);

   {
		VkImageMemoryBarrier image_barriers[] = {
			{
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.image = a_vk.swapchain.images[a_vk.swapchain.current_frame_image_index],
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
			  .dstAccessMask = VK_ACCESS_MEMORY_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
				.subresourceRange =
					(VkImageSubresourceRange){
						.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
						.baseMipLevel = 0,
						.levelCount = 1,
						.baseArrayLayer = 0,
						.layerCount = 1,
					},
			}};
		 vkCmdPipelineBarrier(g.cmdbuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0,
			NULL, 0, NULL, COUNTOF(image_barriers), image_barriers);
	}

	AVK_CHECK_RESULT(vkEndCommandBuffer(g.cmdbuf));

	VkPipelineStageFlags stageflags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	VkSubmitInfo subinfo = {0};
	subinfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	subinfo.pNext = NULL;
	subinfo.commandBufferCount = 1;
	subinfo.pCommandBuffers = &g.cmdbuf;
	subinfo.waitSemaphoreCount = 1;
	subinfo.pWaitSemaphores = &a_vk.swapchain.image_available;
	subinfo.signalSemaphoreCount = 1;
	subinfo.pSignalSemaphores = &g.done,
	subinfo.pWaitDstStageMask = &stageflags;
	AVK_CHECK_RESULT(vkQueueSubmit(a_vk.main_queue, 1, &subinfo, g.fence));

	VkPresentInfoKHR presinfo = {0};
	presinfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presinfo.pSwapchains = &a_vk.swapchain.handle;
	presinfo.pImageIndices = &a_vk.swapchain.current_frame_image_index;
	presinfo.swapchainCount = 1;
	presinfo.pWaitSemaphores = &g.done;
	presinfo.waitSemaphoreCount = 1;
	AVK_CHECK_RESULT(vkQueuePresentKHR(a_vk.main_queue, &presinfo));

	// FIXME bad sync
	AVK_CHECK_RESULT(vkWaitForFences(a_vk.dev, 1, &g.fence, VK_TRUE, INT64_MAX));
	AVK_CHECK_RESULT(vkResetFences(a_vk.dev, 1, &g.fence));
}

static void close() {
	vkFreeCommandBuffers(a_vk.dev, g.cmdpool, 1, &g.cmdbuf);
	vkDestroyCommandPool(a_vk.dev, g.cmdpool, NULL);

	vkDestroyPipelineLayout(a_vk.dev, g.pipeline_layout, NULL);

	vkDestroyShaderModule(a_vk.dev, g.modules.raygen, NULL);

	vkDestroyBuffer(a_vk.dev, g.sbt_buf, NULL);
	vkFreeMemory(a_vk.dev, g.devmem, NULL);

	vkDestroyRenderPass(a_vk.dev, g.render_pass, NULL);
	aVkDestroySemaphore(g.done);
	vkDestroyFence(a_vk.dev, g.fence, NULL);

	aVkDestroy();
}

static void resize(ATimeUs ts, unsigned int old_width, unsigned int old_height) {
	swapchainWillDestroy();
	aVkCreateSwapchain(a_app_state->width, a_app_state->height);
	swapchainCreated();
	paint(ts, 0);
}

void key(ATimeUs ts, AKey key, int down) {
	if (key == AK_Esc)
		aAppClose();
}

void attoAppInit(struct AAppProctable *proctable) {
	proctable->paint = paint;
	proctable->resize = resize;
	proctable->close = close;
	proctable->key = key;

	// TODO FIXME VK INIT
	aVkInitInstance();
	aVkCreateSurface();

	VkPhysicalDeviceBufferDeviceAddressFeaturesEXT pdbdaf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_EXT,
		.bufferDeviceAddress = VK_TRUE,
	};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR pdasf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
		.accelerationStructure = VK_TRUE,
		.pNext = &pdbdaf,
	};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR pdrtf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
		.rayTracingPipeline = VK_TRUE,
		.pNext = &pdasf,
	};

	aVkInitDevice(&pdrtf);

	g.prop.rt = (VkPhysicalDeviceRayTracingPipelinePropertiesKHR){.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
	g.prop.prop2 = (VkPhysicalDeviceProperties2){.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &g.prop.rt};
	vkGetPhysicalDeviceProperties2(a_vk.phys_dev, &g.prop.prop2);
	g.shader_group_size = g.prop.rt.shaderGroupHandleSize < g.prop.rt.shaderGroupHandleAlignment
		? g.prop.rt.shaderGroupHandleAlignment
		: g.prop.rt.shaderGroupHandleSize;

	g.done = aVkCreateSemaphore();
	createRenderPass();
	createLayouts();
	//createBuffer();
	createCommandPool();

	aVkPokePresentModes();
	aVkCreateSwapchain(a_app_state->width, a_app_state->height);
	swapchainCreated();

	vkCmdTraceRaysKHR_ = AVK_DEV_FUNC(vkCmdTraceRaysKHR);
	g.rt_image = createImage(1280, 720, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

	createPipeline();

	VkFenceCreateInfo fci = {0};
	fci.flags = 0;
	fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	AVK_CHECK_RESULT(vkCreateFence(a_vk.dev, &fci, NULL, &g.fence));
}
