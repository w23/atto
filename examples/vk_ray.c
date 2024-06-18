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

void destroyImage(struct Image *img) {
	vkDestroyImageView(a_vk.dev, img->view, NULL);
	vkDestroyImage(a_vk.dev, img->image, NULL);
	vkFreeMemory(a_vk.dev, img->devmem, NULL);
	*img = (struct Image){0};
}

struct Buffer {
	VkBuffer buffer;
	VkDeviceMemory devmem;
	void *data;
	size_t size;
};

static struct Buffer createBuffer(size_t size, VkBufferUsageFlags usage) {
	struct Buffer ret = {.size = size};
	VkBufferCreateInfo bci = {0};
	bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bci.size = size;
	bci.usage = usage;
	bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	AVK_CHECK_RESULT(vkCreateBuffer(a_vk.dev, &bci, NULL, &ret.buffer));

	VkMemoryRequirements memreq;
	vkGetBufferMemoryRequirements(a_vk.dev, ret.buffer, &memreq);
	aAppDebugPrintf("memreq: memoryTypeBits=0x%x alignment=%zu size=%zu", memreq.memoryTypeBits, memreq.alignment, memreq.size);

	VkMemoryAllocateFlagsInfo mafi = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
		.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	};
	VkMemoryAllocateInfo mai = {
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = &mafi,
		.allocationSize = memreq.size,
		.memoryTypeIndex = aVkFindMemoryWithType(memreq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
	};
	AVK_CHECK_RESULT(vkAllocateMemory(a_vk.dev, &mai, NULL, &ret.devmem));
	AVK_CHECK_RESULT(vkBindBufferMemory(a_vk.dev, ret.buffer, ret.devmem, 0));

	AVK_CHECK_RESULT(vkMapMemory(a_vk.dev, ret.devmem, 0, bci.size, 0, &ret.data));
	//vkUnmapMemory(a_vk.dev, g.devmem);
	return ret;
}

static void destroyBuffer(struct Buffer *buf) {
	vkUnmapMemory(a_vk.dev, buf->devmem);
	vkDestroyBuffer(a_vk.dev, buf->buffer, NULL);
	vkFreeMemory(a_vk.dev, buf->devmem, NULL);
	*buf = (struct Buffer){0};
}

VkDeviceAddress getBufferDeviceAddress(VkBuffer buffer) {
	const VkBufferDeviceAddressInfo bdai = {.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO, .buffer = buffer};
	return vkGetBufferDeviceAddress(a_vk.dev, &bdai);
}

struct Accel {
	struct Buffer buffer;
	VkAccelerationStructureKHR handle;
};

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
		VkShaderModule rayintersect;
		VkShaderModule raymiss;
		VkShaderModule rayclosesthit;
		VkShaderModule rayclosesthit_tri;
	} modules;
	VkDescriptorSetLayout desc_layout;
	VkDescriptorPool desc_pool;
	VkDescriptorSet desc_set;

	VkImageView *image_views;
	VkPipeline pipeline;
	struct Image rt_image;

	struct Buffer sbt_buf, aabb_buf, tri_buf, tl_geom_buffer;
	struct Accel blas, blas_tri, tlas;

	VkCommandPool cmdpool;
	VkCommandBuffer cmdbuf;
} g;

struct Accel createAccelerationStructure(const VkAccelerationStructureGeometryKHR *geoms, const uint32_t *max_prim_counts, const VkAccelerationStructureBuildRangeInfoKHR **build_ranges, uint32_t n_geoms, VkAccelerationStructureTypeKHR type) {
	struct Accel accel;
	VkAccelerationStructureBuildGeometryInfoKHR build_info = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
		.type = type,
		.flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
		.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
		.geometryCount = n_geoms,
		.pGeometries = geoms,
	};

	VkAccelerationStructureBuildSizesInfoKHR build_size = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR};
  AVK_DEV_FUNC(vkGetAccelerationStructureBuildSizesKHR)(
		a_vk.dev, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &build_info, max_prim_counts, &build_size);

	aAppDebugPrintf(
		"AS build size: %d, scratch size: %d", build_size.accelerationStructureSize, build_size.buildScratchSize);

	accel.buffer = createBuffer(build_size.accelerationStructureSize,
		VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
	struct Buffer scratch_buffer = createBuffer(build_size.buildScratchSize,
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	const VkAccelerationStructureCreateInfoKHR asci = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
		.buffer = accel.buffer.buffer,
		.size = accel.buffer.size,
		.type = type,
	};
	AVK_CHECK_RESULT(AVK_DEV_FUNC(vkCreateAccelerationStructureKHR)(a_vk.dev, &asci, NULL, &accel.handle));

	build_info.dstAccelerationStructure = accel.handle;
	build_info.scratchData.deviceAddress = getBufferDeviceAddress(scratch_buffer.buffer);

	VkCommandBufferBeginInfo beginfo = {.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	beginfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	AVK_CHECK_RESULT(vkBeginCommandBuffer(g.cmdbuf, &beginfo));
	AVK_DEV_FUNC(vkCmdBuildAccelerationStructuresKHR)(g.cmdbuf, 1, &build_info, build_ranges);
	AVK_CHECK_RESULT(vkEndCommandBuffer(g.cmdbuf));

	VkSubmitInfo subinfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.commandBufferCount = 1,
		.pCommandBuffers = &g.cmdbuf,
	};
	AVK_CHECK_RESULT(vkQueueSubmit(a_vk.main_queue, 1, &subinfo, NULL));
	AVK_CHECK_RESULT(vkQueueWaitIdle(a_vk.main_queue));

	destroyBuffer(&scratch_buffer);

	return accel;
}

void destroyAccelerationStructure(struct Accel *a) {
	AVK_DEV_FUNC(vkDestroyAccelerationStructureKHR)(a_vk.dev, a->handle, NULL);
	destroyBuffer(&a->buffer);
}

static void createLayouts() {
  VkDescriptorSetLayoutBinding bindings[] = {{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		}, {
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
		}, {
			.binding = 2,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		},
	};

	VkDescriptorSetLayoutCreateInfo dslci = {.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO, .bindingCount = COUNTOF(bindings), .pBindings = bindings, };

	AVK_CHECK_RESULT(vkCreateDescriptorSetLayout(a_vk.dev, &dslci, NULL, &g.desc_layout));

	VkPushConstantRange push_const = {0};
	push_const.offset = 0;
	push_const.size = sizeof(float);
	push_const.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR;

	VkPipelineLayoutCreateInfo plci = {0};
	plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	plci.setLayoutCount = 1;
	plci.pSetLayouts = &g.desc_layout;
	plci.pushConstantRangeCount = 1;
	plci.pPushConstantRanges = &push_const;
	AVK_CHECK_RESULT(vkCreatePipelineLayout(a_vk.dev, &plci, NULL, &g.pipeline_layout));

	VkDescriptorPoolSize pools[] = {
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1},
			{.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 1},
			{.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, .descriptorCount = 1},
	};

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
	g.modules.raymiss = loadShaderFromFile("ray.rmiss.spv");
	g.modules.rayintersect = loadShaderFromFile("ray.rint.spv");
	g.modules.rayclosesthit = loadShaderFromFile("ray.rchit.spv");
	g.modules.rayclosesthit_tri = loadShaderFromFile("ray_tri.rchit.spv");
}

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
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_MISS_BIT_KHR,
		.module = g.modules.raymiss,
		.pName = "main",
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		.module = g.modules.rayclosesthit,
		.pName = "main",
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
		.module = g.modules.rayintersect,
		.pName = "main",
	}, {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
		.module = g.modules.rayclosesthit_tri,
		.pName = "main",
	},
	};

	VkRayTracingShaderGroupCreateInfoKHR groups[] = {{
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = 0, // Raygen
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	}, {
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
		.generalShader = 1, // Miss
		.closestHitShader = VK_SHADER_UNUSED_KHR,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	}, { // procedural
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR,
		.generalShader = VK_SHADER_UNUSED_KHR,
		.closestHitShader = 2,
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = 3,
	}, { // tri
		.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
		.type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
		.generalShader = VK_SHADER_UNUSED_KHR,
		.closestHitShader = 4, // index into stages
		.anyHitShader = VK_SHADER_UNUSED_KHR,
		.intersectionShader = VK_SHADER_UNUSED_KHR,
	},
	};

	VkRayTracingPipelineCreateInfoKHR rtci = {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR};
	//rtci.flags = VK_PIPELINE_CREATE_RAY_TRACING_SKIP_TRIANGLES_BIT_KHR;
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
	g.sbt_buf =
		createBuffer(sbt_size, VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);

	const uint32_t handles_size = rtci.groupCount * g.prop.rt.shaderGroupHandleSize;
	uint8_t *sbt_tmp_buf = malloc(handles_size);
	AVK_CHECK_RESULT(AVK_DEV_FUNC(vkGetRayTracingShaderGroupHandlesKHR)(
		a_vk.dev, g.pipeline, 0, rtci.groupCount, handles_size, sbt_tmp_buf));
	for (int i = 0; i < (int)rtci.groupCount; ++i) {
		memcpy(((uint8_t *)g.sbt_buf.data) + i * g.shader_group_size, sbt_tmp_buf + i * g.prop.rt.shaderGroupHandleSize,
			g.prop.rt.shaderGroupHandleSize);
	}
	free(sbt_tmp_buf);
}

VkDeviceAddress getASAddress(VkAccelerationStructureKHR as) {
	VkAccelerationStructureDeviceAddressInfoKHR asdai = {
		.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
		.accelerationStructure = as,
	};
	return AVK_DEV_FUNC(vkGetAccelerationStructureDeviceAddressKHR)(a_vk.dev, &asdai);
}

void createAccelerationStructures() {
	const VkAabbPositionsKHR aabbox = {
		-1, -1, -1,
		1, 1, 1,
	};

	g.aabb_buf = createBuffer(sizeof(aabbox),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
	memcpy(g.aabb_buf.data, &aabbox, sizeof(aabbox));

	#define TRIS 256
	float triangles[TRIS * 12] = {
			3.f, 0.f, 3.f, 0.f,
			3.f, 0.f, -3.f, 0.f,
			3.f, 3.f, 0.f, 0.f,

			0.f, 3.f, 3.f, 0.f,
			0.f, -3.f, 3.f, 0.f,
			3.f, 0.f, 3.f, 0.f,

			0.f, 3.f, 3.f, 0.f,
			0.f, 3.f, -3.f, 0.f,
			3.f, 3.f, 0.f, 0.f,
	};
	for (int i = 3; i < TRIS; ++i) {
			float x = (float)rand() / RAND_MAX;
			float y = (float)rand() / RAND_MAX;
			float z = (float)rand() / RAND_MAX;

			y *= 10.f;
			x = (x - .5f) * 20.f;
			z = (z - .5f) * 20.f;

			triangles[i * 12 + 0] = x;
			triangles[i * 12 + 1] = y;
			triangles[i * 12 + 2] = z;

			triangles[i * 12 + 4] = x + 2.f * (float)rand() / RAND_MAX;
			triangles[i * 12 + 5] = y + 2.f * (float)rand() / RAND_MAX;
			triangles[i * 12 + 6] = z + 2.f * (float)rand() / RAND_MAX;

			triangles[i * 12 + 8] = x + 2.f * (float)rand() / RAND_MAX;
			triangles[i * 12 + 9] = y + 2.f * (float)rand() / RAND_MAX;
			triangles[i * 12 + 10] = z + 2.f * (float)rand() / RAND_MAX;
	}

	
	g.tri_buf = createBuffer(sizeof(triangles),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
	memcpy(g.tri_buf.data, triangles, sizeof(triangles));

	{
		const VkAccelerationStructureGeometryKHR geom[] = {
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
				.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
				.geometryType = VK_GEOMETRY_TYPE_AABBS_KHR,
				.geometry.aabbs =
					(VkAccelerationStructureGeometryAabbsDataKHR){
						.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR,
						.data.deviceAddress = getBufferDeviceAddress(g.aabb_buf.buffer),
						.stride = sizeof(VkAabbPositionsKHR),
					},
			},
			};

		const uint32_t max_prim_counts[COUNTOF(geom)] = {1};
		const VkAccelerationStructureBuildRangeInfoKHR build_range_aabb = {
			.primitiveCount = 1,
		};
		const VkAccelerationStructureBuildRangeInfoKHR *build_ranges[] = {
			&build_range_aabb,
		};
		g.blas = createAccelerationStructure(
			geom, max_prim_counts, build_ranges, COUNTOF(geom), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
	}

	{
		const VkAccelerationStructureGeometryKHR geom[] = {
			{
				.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
				.flags = VK_GEOMETRY_OPAQUE_BIT_KHR,
				.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
				.geometry.triangles =
					(VkAccelerationStructureGeometryTrianglesDataKHR){
						.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
						.indexType = VK_INDEX_TYPE_NONE_KHR,
						.maxVertex = TRIS * 3,
						.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
						.vertexStride = sizeof(float) * 4,
						.vertexData.deviceAddress = getBufferDeviceAddress(g.tri_buf.buffer),
					},
			}};

		const uint32_t max_prim_counts[COUNTOF(geom)] = {TRIS};
		const VkAccelerationStructureBuildRangeInfoKHR build_range_tri = {
			.primitiveCount = TRIS,
		};
		const VkAccelerationStructureBuildRangeInfoKHR *build_ranges[COUNTOF(geom)] = {&build_range_tri};
		g.blas_tri = createAccelerationStructure(
			geom, max_prim_counts, build_ranges, COUNTOF(geom), VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR);
	}

	const VkAccelerationStructureInstanceKHR inst[] = {{
		.transform = (VkTransformMatrixKHR){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
		.instanceCustomIndex = 0,
		.mask = 0xff,
		.instanceShaderBindingTableRecordOffset = 0,
		.flags = 0,
		.accelerationStructureReference = getASAddress(g.blas.handle),
	}, {
		.transform = (VkTransformMatrixKHR){1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0},
		.instanceCustomIndex = 0,
		.mask = 0xff,
		.instanceShaderBindingTableRecordOffset = 1, // index into sbt_hit group
		.flags = 0,
		.accelerationStructureReference = getASAddress(g.blas_tri.handle),
	},
	};

	g.tl_geom_buffer = createBuffer(sizeof(inst),
		VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
			VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR);
	memcpy(g.tl_geom_buffer.data, &inst, sizeof(inst));

	const VkAccelerationStructureGeometryKHR tl_geom[] = {
		{
			.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
			//.flags = VK_GEOMETRY_OPAQUE_BIT,
			.geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
			.geometry.instances =
				(VkAccelerationStructureGeometryInstancesDataKHR){
					.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
					.data.deviceAddress = getBufferDeviceAddress(g.tl_geom_buffer.buffer),
					.arrayOfPointers = VK_FALSE,
				},
		},
	};

	const uint32_t tl_max_prim_counts[COUNTOF(tl_geom)] = {COUNTOF(inst)};
	const VkAccelerationStructureBuildRangeInfoKHR tl_build_range = {
			.primitiveCount = COUNTOF(inst),
	};
	const VkAccelerationStructureBuildRangeInfoKHR *tl_build_ranges[] = {&tl_build_range};
	g.tlas = createAccelerationStructure(
		tl_geom, tl_max_prim_counts, tl_build_ranges, COUNTOF(tl_geom), VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR);
}

static void swapchainWillDestroy() {
	//vkDestroyPipeline(a_vk.dev, g.pipeline, NULL);
	for (uint32_t i = 0; i < a_vk.swapchain.num_images; ++i) {
		//vkDestroyFramebuffer(a_vk.dev, g.framebuffers[i], NULL);
		vkDestroyImageView(a_vk.dev, g.image_views[i], NULL);
	}
}

static void swapchainCreated() {
	const uint32_t num_images = a_vk.swapchain.num_images;
	g.image_views = malloc(num_images * sizeof(VkImageView));
	//g.framebuffers = malloc(num_images * sizeof(VkFramebuffer));
	for (uint32_t i = 0; i < num_images; ++i) {
		VkImageViewCreateInfo ivci = {.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
		ivci.format = a_vk.swapchain.info.imageFormat;
		ivci.image = a_vk.swapchain.images[i];
		ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		ivci.subresourceRange.levelCount = 1;
		ivci.subresourceRange.layerCount = 1;
		AVK_CHECK_RESULT(vkCreateImageView(a_vk.dev, &ivci, NULL, g.image_views + i));

		//VkFramebufferCreateInfo fbci = {.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		//fbci.renderPass = g.render_pass;
		//fbci.attachmentCount = 1;
		//fbci.pAttachments = g.image_views + i;
		//fbci.width = a_vk.surface_width;
		//fbci.height = a_vk.surface_height;
		//fbci.layers = 1;
		//AVK_CHECK_RESULT(vkCreateFramebuffer(a_vk.dev, &fbci, NULL, g.framebuffers + i));
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

	const VkDescriptorImageInfo dii = {
		.sampler = VK_NULL_HANDLE,
		.imageView = g.rt_image.view,
		.imageLayout = VK_IMAGE_LAYOUT_GENERAL,
	};
	const VkDescriptorBufferInfo dbi = {
		.buffer = g.tri_buf.buffer,
		.offset = 0,
		.range = VK_WHOLE_SIZE,
	};
	const VkWriteDescriptorSetAccelerationStructureKHR wdsas = {
		.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
		.accelerationStructureCount = 1,
		.pAccelerationStructures = &g.tlas.handle,
	};
	const VkWriteDescriptorSet wds[] = {
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
			.dstSet = g.desc_set,
			.dstBinding = 0,
			.dstArrayElement = 0,
			.pImageInfo = &dii,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.dstSet = g.desc_set,
			.dstBinding = 2,
			.dstArrayElement = 0,
			.pBufferInfo = &dbi,
		},
		{
			.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.descriptorCount = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
			.dstSet = g.desc_set,
			.dstBinding = 1,
			.dstArrayElement = 0,
			.pNext = &wdsas,
		},
	};

	vkUpdateDescriptorSets(a_vk.dev, COUNTOF(wds), wds, 0, NULL);
	vkCmdPushConstants(g.cmdbuf, g.pipeline_layout, VK_SHADER_STAGE_RAYGEN_BIT_KHR, 0, sizeof(float), &t);
	vkCmdBindDescriptorSets(g.cmdbuf, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, g.pipeline_layout, 0, 1, &g.desc_set, 0, NULL);

	const VkStridedDeviceAddressRegionKHR sbt_null = {0};
	const VkDeviceAddress sbt_addr = getBufferDeviceAddress(g.sbt_buf.buffer);
	const VkStridedDeviceAddressRegionKHR sbt_raygen = {.deviceAddress = sbt_addr, .size = g.shader_group_size, .stride = g.shader_group_size};
	const VkStridedDeviceAddressRegionKHR sbt_raymiss = {.deviceAddress = sbt_addr + g.shader_group_size, .size = g.shader_group_size, .stride = g.shader_group_size};
	const VkStridedDeviceAddressRegionKHR sbt_rayhit = {.deviceAddress = sbt_addr + g.shader_group_size * 2, .size = g.shader_group_size * 2, .stride = g.shader_group_size};
	vkCmdTraceRaysKHR_(g.cmdbuf, &sbt_raygen, &sbt_raymiss, &sbt_rayhit, &sbt_null, 1280, 720, 1);

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
	vkDeviceWaitIdle(a_vk.dev);

	swapchainWillDestroy();
	destroyImage(&g.rt_image);

	vkFreeCommandBuffers(a_vk.dev, g.cmdpool, 1, &g.cmdbuf);
	vkDestroyCommandPool(a_vk.dev, g.cmdpool, NULL);

	vkDestroyPipeline(a_vk.dev, g.pipeline, NULL);
	vkDestroyDescriptorSetLayout(a_vk.dev, g.desc_layout, NULL);
	vkDestroyDescriptorPool(a_vk.dev, g.desc_pool, NULL);
	vkDestroyPipelineLayout(a_vk.dev, g.pipeline_layout, NULL);
	vkDestroyShaderModule(a_vk.dev, g.modules.raygen, NULL);
	vkDestroyShaderModule(a_vk.dev, g.modules.raymiss, NULL);
	vkDestroyShaderModule(a_vk.dev, g.modules.rayclosesthit, NULL);
	vkDestroyShaderModule(a_vk.dev, g.modules.rayclosesthit_tri, NULL);
	vkDestroyShaderModule(a_vk.dev, g.modules.rayintersect, NULL);

	destroyAccelerationStructure(&g.tlas);
	destroyAccelerationStructure(&g.blas);
	destroyAccelerationStructure(&g.blas_tri);
	destroyBuffer(&g.sbt_buf);
	destroyBuffer(&g.tri_buf);
	destroyBuffer(&g.aabb_buf);
	destroyBuffer(&g.tl_geom_buffer);

	vkDestroyRenderPass(a_vk.dev, g.render_pass, NULL);
	aVkDestroySemaphore(g.done);
	vkDestroyFence(a_vk.dev, g.fence, NULL);

	aVkDestroySwapchain();

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
	// TODO FIGURE OUT WHAT I MEANT BY THAT ^^^
	aVkInitInstance();
	aVkCreateSurface();

	VkPhysicalDeviceBufferDeviceAddressFeatures pdbdaf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
		.bufferDeviceAddress = VK_TRUE,
	};
	VkPhysicalDeviceAccelerationStructureFeaturesKHR pdasf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
		.accelerationStructure = VK_TRUE,
		// not supported by nv .accelerationStructureHostCommands = VK_TRUE,
		.pNext = &pdbdaf,
	};
	VkPhysicalDeviceRayTracingPipelineFeaturesKHR pdrtf = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
		.rayTracingPipeline = VK_TRUE,
		.pNext = &pdasf,
	};

	aVkInitDevice(&pdrtf, NULL, NULL);

	g.prop.rt = (VkPhysicalDeviceRayTracingPipelinePropertiesKHR){.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
	g.prop.prop2 = (VkPhysicalDeviceProperties2){.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2, .pNext = &g.prop.rt};
	vkGetPhysicalDeviceProperties2(a_vk.phys_dev, &g.prop.prop2);
	g.shader_group_size = g.prop.rt.shaderGroupHandleSize < g.prop.rt.shaderGroupBaseAlignment
		? g.prop.rt.shaderGroupBaseAlignment
		: g.prop.rt.shaderGroupHandleSize;

	g.done = aVkCreateSemaphore();
	//createRenderPass();
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
	createAccelerationStructures();

	VkFenceCreateInfo fci = {0};
	fci.flags = 0;
	fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	AVK_CHECK_RESULT(vkCreateFence(a_vk.dev, &fci, NULL, &g.fence));
}
