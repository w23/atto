#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <xf86drm.h>
#include <xf86drmMode.h>
#include <drm_fourcc.h>
#include <gbm.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>

#include "atto/app.h"

#define ALOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

#ifndef COUNTOF
#define COUNTOF(a) (sizeof(a)/sizeof(*(a)))
#endif

static int openFirstKmsDevice(void) {
	int fd = -1;
	const int devices_count = drmGetDevices2(0, NULL, 0);
	ALOG("Found %d devices", devices_count);
	if (devices_count < 1)
		return -1;

	drmDevicePtr *const devices = malloc(sizeof(drmDevicePtr) * devices_count);
	const int devices_count2 = drmGetDevices2(0, devices, devices_count);
	ATTO_ASSERT(devices_count2 == devices_count);

	for (int i = 0; i < devices_count2; ++i) {
		const drmDevicePtr dev = devices[i];
		if (!(dev->available_nodes & (1 << DRM_NODE_PRIMARY)))
			continue;

		const char *node = dev->nodes[DRM_NODE_PRIMARY];
		if (!node || node[0] == '\0') {
			ALOG("Device %d, avaliable_nodes report DRM_NODE_PRIMARY, but no node name is available", i);
			continue;
		}

		fd = open(node, O_RDWR);
		if (fd < 0) {
			ALOG("Device %d, unable to open node \"%s\"", i, node);
			continue;
		}

		if (!drmIsKMS(fd)) {
			ALOG("Device %d, node \"%s\" is not suitable for KMS", i, node);
			close(fd);
			continue;
		}

		ALOG("Device %d, node \"%s\" selected as fd=%d", i, node, fd);
		break;
	}

	drmFreeDevices(devices, devices_count2);
	free(devices);
	return fd;
}

static drmModeConnectorPtr findFirstConnectedConnector(int fd, const drmModeResPtr res) {
	for (int i = 0; i < res->count_connectors; ++i) {
		drmModeConnectorPtr c = drmModeGetConnector(fd, res->connectors[i]);

		if (c->connection == DRM_MODE_CONNECTED) {
			ALOG("Found first connected connector index=%d", i);
			return c;
		}

		drmModeFreeConnector(c);
	}

	return NULL;
}

static drmModeModeInfoPtr findPreferredMode(const drmModeConnectorPtr conn) {
	for (int i = 0; i < conn->count_modes; ++i) {
		drmModeModeInfoPtr mode = &conn->modes[i];
		if ((mode->type & DRM_MODE_TYPE_PREFERRED) == DRM_MODE_TYPE_PREFERRED) {
			ALOG("Found preferred mode %ux%u@%u with index=%d", mode->vdisplay, mode->hdisplay, mode->vrefresh, i);
			return mode;
		}
	}

	return NULL;
}

static uint32_t findEncoderCompatibleCrtcId(int fd, const drmModeResPtr res, uint32_t encoder_id) {
	drmModeEncoderPtr enc = drmModeGetEncoder(fd, encoder_id);

	if (enc->crtc_id)
		return enc->crtc_id;

	for (int i = 0; i < res->count_crtcs; ++i) {
		if (enc->possible_crtcs & (1 << i))
			return res->crtcs[i];
	}

	drmModeFreeEncoder(enc);

	return 0;
}

static uint32_t findCrtcIdForConnector(int fd, const drmModeResPtr res, const drmModeConnectorPtr conn) {
	if (conn->encoder_id) {
		const uint32_t crtc_id = findEncoderCompatibleCrtcId(fd, res, conn->encoder_id);
		if (crtc_id)
			return crtc_id;
	}

	// If no current encoder, find a new one
	for (int i = 0; i < conn->count_encoders; ++i) {
		const uint32_t crtc_id = findEncoderCompatibleCrtcId(fd, res, conn->encoders[i]);
		if (crtc_id)
			return crtc_id;
	}

	return 0;
}

static struct {
	struct {
		int fd;
		drmModeModeInfo mode;
		uint32_t crtc_id, connector_id;
	} drm;
	struct {
		struct gbm_device *device;
		struct gbm_surface *surface;
		uint32_t format;

		struct gbm_bo *bo_currently_displayed;
		struct gbm_bo *bo_enqueued_to_flip;
	} gbm;
	struct {
		EGLDisplay display;
		EGLConfig config;
		EGLSurface surface;
		EGLContext context;
	} egl;
} a__kms;

static void openKmsAndGbm(void) {
	a__kms.drm.fd = openFirstKmsDevice();
	ATTO_ASSERT(a__kms.drm.fd > 0);

	const int set_drm_master_result = drmSetMaster(a__kms.drm.fd);
	ATTO_ASSERT(set_drm_master_result == 0);

	a__kms.gbm.device = gbm_create_device(a__kms.drm.fd);
	ATTO_ASSERT(a__kms.gbm.device);
}

static void findBestMode(void) {
	drmModeResPtr res = NULL;
	drmModeConnectorPtr conn = NULL;

	res = drmModeGetResources(a__kms.drm.fd);
	ATTO_ASSERT(res);

	conn = findFirstConnectedConnector(a__kms.drm.fd, res);
	ATTO_ASSERT(conn);

	const uint32_t crtc_id = findCrtcIdForConnector(a__kms.drm.fd, res, conn);
	ATTO_ASSERT(crtc_id);

	drmModeModeInfoPtr mode = findPreferredMode(conn);
	ATTO_ASSERT(mode);
	a__kms.drm.mode = *mode;

	a__kms.drm.crtc_id = crtc_id;
	a__kms.drm.connector_id = conn->connector_id;

	drmModeFreeConnector(conn);
	drmModeFreeResources(res);
}

static const EGLint egl_config_attrs[] = {
	EGL_BUFFER_SIZE, 16,
	EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER,
	EGL_CONFIG_CAVEAT, EGL_NONE,
	EGL_RED_SIZE, 5,
	EGL_GREEN_SIZE, 6,
	EGL_BLUE_SIZE, 5,
	EGL_DEPTH_SIZE, 16,
	EGL_STENCIL_SIZE, 0,
	EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
	EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
	EGL_CONFORMANT, EGL_OPENGL_ES2_BIT,
	EGL_NONE
};

static const EGLint egl_context_attrs[] = {
	EGL_CONTEXT_CLIENT_VERSION, 2,
	EGL_NONE
};

static void initEgl(void) {
	const char *client_extensions = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);
	ALOG("EGL_EXTENSIONS(client): %s", client_extensions);

	a__kms.egl.display = eglGetPlatformDisplay(EGL_PLATFORM_GBM_MESA, a__kms.gbm.device, NULL);
	ATTO_ASSERT(a__kms.egl.display != EGL_NO_DISPLAY);

	{
		EGLint vmaj, vmin;
		ATTO_ASSERT(eglInitialize(a__kms.egl.display, &vmaj, &vmin));
		ALOG("EGL %d.%d", vmaj, vmin);

		if (vmaj > 1 || vmin >= 2) {
			ALOG("EGL_CLIENT_APIS: %s", eglQueryString(a__kms.egl.display, EGL_CLIENT_APIS));
		}

		ALOG("EGL_VENDOR: %s", eglQueryString(a__kms.egl.display, EGL_VENDOR));
		ALOG("EGL_VERSION: %s", eglQueryString(a__kms.egl.display, EGL_VERSION));
		ALOG("EGL_EXTENSIONS(display): %s", eglQueryString(a__kms.egl.display, EGL_EXTENSIONS));
	}

	{
		//ATTO_ASSERT(eglGetConfigs(a__kms.egl.display, NULL, 0, egl_config_attrs));
		EGLConfig configs[256];
		EGLint config_count = 0;
		ATTO_ASSERT(eglChooseConfig(a__kms.egl.display, egl_config_attrs, configs, COUNTOF(configs), &config_count));

		for (int i = 0; i < config_count; ++i) {
			EGLint gbm_format;
			ATTO_ASSERT(eglGetConfigAttrib(a__kms.egl.display, configs[i], EGL_NATIVE_VISUAL_ID, &gbm_format));

#define GET_ATTR(attr, name) \
			EGLint name; \
			ATTO_ASSERT(eglGetConfigAttrib(a__kms.egl.display, configs[i], attr, &name))

			GET_ATTR(EGL_BUFFER_SIZE, buffer_size);
			GET_ATTR(EGL_CONFIG_CAVEAT, caveat);
			GET_ATTR(EGL_NATIVE_RENDERABLE, native_renderable);
			GET_ATTR(EGL_CONFIG_ID, config_id);
			GET_ATTR(EGL_RED_SIZE, blue_bits);
			GET_ATTR(EGL_GREEN_SIZE, green_bits);
			GET_ATTR(EGL_BLUE_SIZE, red_bits);
			GET_ATTR(EGL_DEPTH_SIZE, depth_bits);
			GET_ATTR(EGL_STENCIL_SIZE, stencil_bits);

			ALOG("configs[%d]: id=0x%x rgb%d=%d%d%d d=%d stencil=%d gbm_fmt=%.4s caveat=0x%x native=%d", i, config_id,
				buffer_size, red_bits, green_bits, blue_bits, depth_bits, stencil_bits,
				(const char*)&gbm_format,
				caveat, native_renderable);

			// Lol Mesa prefers higher bits, but we actually want the smallest possible
			if (red_bits == 5 && green_bits == 6 && blue_bits == 5 && depth_bits == 16 && stencil_bits == 0) {
				a__kms.gbm.format = gbm_format;
				a__kms.egl.config = configs[i];
				return;
			}
		}

		ATTO_ASSERT(!"Suitable EGL config not found");
	}
}

typedef struct {
	struct gbm_bo *bo;
	uint32_t fb_id;
} Framebobuffer;

static void fboDestroy(struct gbm_bo* bo, void *user) {
	Framebobuffer *fbo = user;
	ATTO_ASSERT(bo == fbo->bo);

	const int drm_fd = gbm_device_get_fd(gbm_bo_get_device(bo));
	ATTO_ASSERT(fbo->fb_id);
	drmModeRmFB(drm_fd, fbo->fb_id);

	free(fbo);
}

static Framebobuffer *getFramebufferForGbmBo(struct gbm_bo* bo) {
	Framebobuffer *fbo = gbm_bo_get_user_data(bo);
	if (fbo)
		return fbo;

	ALOG("Creating new framebuffer for GBM bo=%p", (void*)bo);

	fbo = malloc(sizeof(*fbo));
	*fbo = (Framebobuffer) {
		.bo = bo,
	};

	const uint32_t width = gbm_bo_get_width(bo), height = gbm_bo_get_height(bo), format = gbm_bo_get_format(bo);
	const int planes_count = gbm_bo_get_plane_count(bo);
	ALOG("bo=%p %ux%u fmt=%.4s planes=%d", (void*)bo, width, height, (const char*)&format, planes_count);

	uint32_t handles[4] = {0}, pitches[4] = {0}, offsets[4] = {0};
	uint64_t modifiers[4] = {0};
	modifiers[0] = gbm_bo_get_modifier(bo);
	for (int i = 0; i < planes_count; ++i) {
		handles[i] = gbm_bo_get_handle_for_plane(bo, i).u32;
		pitches[i] = gbm_bo_get_stride_for_plane(bo, i);
		offsets[i] = gbm_bo_get_offset(bo, i);
		modifiers[i] = modifiers[0];
	}

	const uint32_t flags = (modifiers[0] && modifiers[0] != DRM_FORMAT_MOD_INVALID) ? DRM_MODE_FB_MODIFIERS : 0;
	const int fb_with_modifiers_result = drmModeAddFB2WithModifiers(a__kms.drm.fd, width, height, format, handles, pitches, offsets, modifiers, &fbo->fb_id, flags);
	if (fb_with_modifiers_result != 0) {
		ALOG("Trying libdrm framebuffer without modifiers");

		const uint32_t handles[4] = { gbm_bo_get_handle(bo).u32, 0, 0, 0 };
		const uint32_t pitches[4] = { gbm_bo_get_stride(bo), 0, 0, 0 };
		const uint32_t offsets[4] = { 0, 0, 0, 0 };
		const uint32_t flags = 0;
		const int fb_wo_modifiers_result = drmModeAddFB2(a__kms.drm.fd, width, height, format, handles, pitches, offsets, &fbo->fb_id, flags);
		ATTO_ASSERT(fb_wo_modifiers_result == 0);
	}

	// Track associated drm framebuffer with gbm buffer object
	gbm_bo_set_user_data(bo, fbo, fboDestroy);

	return fbo;
}

static void createSurfaces(void) {
	const drmModeModeInfoPtr mode = &a__kms.drm.mode;
	ALOG("%ux%u fmt=%.4s", mode->hdisplay, mode->vdisplay, (const char*)&a__kms.gbm.format);
	a__kms.gbm.surface = gbm_surface_create(a__kms.gbm.device,
		mode->hdisplay, mode->vdisplay, a__kms.gbm.format,
		GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
	ATTO_ASSERT(a__kms.gbm.surface);

	a__kms.egl.surface = eglCreatePlatformWindowSurface(a__kms.egl.display, a__kms.egl.config, a__kms.gbm.surface, NULL);
	ATTO_ASSERT(a__kms.egl.surface != EGL_NO_SURFACE);
}

void a__kmsInit(struct AAppState *state) {
	// provides: kms.fd, gbm.device
	openKmsAndGbm();

	// needs: kms.fd
	// provides: kms.mode,crtc_id,connector_id
	findBestMode();

	// needs: gbm.device
	// provides: egl.display,config gbm.format
	initEgl();

	// needs: gbm.device,format drm.mode egl.display,config
	// provides: gbm.surface egl.surface
	createSurfaces();

	a__kms.egl.context = eglCreateContext(a__kms.egl.display, a__kms.egl.config, EGL_NO_CONTEXT, egl_context_attrs);
	ATTO_ASSERT(EGL_NO_CONTEXT != a__kms.egl.context);

	ATTO_ASSERT(eglMakeCurrent(a__kms.egl.display, a__kms.egl.surface, a__kms.egl.surface, a__kms.egl.context));

	// Set Mode
	eglSwapBuffers(a__kms.egl.display, a__kms.egl.surface); // why? copied from kmscube
	struct gbm_bo *buffer = gbm_surface_lock_front_buffer(a__kms.gbm.surface);
	const uint32_t framebuffer_id = getFramebufferForGbmBo(buffer)->fb_id;
	ATTO_ASSERT(0 == drmModeSetCrtc(a__kms.drm.fd, a__kms.drm.crtc_id, framebuffer_id, 0, 0,
		&a__kms.drm.connector_id, 1, &a__kms.drm.mode));
	a__kms.gbm.bo_currently_displayed = buffer;
	a__kms.gbm.bo_enqueued_to_flip = NULL;

	state->width = a__kms.drm.mode.hdisplay;
	state->height = a__kms.drm.mode.vdisplay;
	state->gl_version = AOGLV_ES_20;
}

static void page_flipped(int fd,
	unsigned int sequence,
	unsigned int tv_sec,
	unsigned int tv_usec,
	void *user_data)
{
	(void)fd;
	(void)sequence;
	(void)tv_sec;
	(void)tv_usec;

	// Make sure we're reacting to the right flip event
	ATTO_ASSERT(user_data == a__kms.gbm.bo_enqueued_to_flip);

	// Release previous buffer being displayed, and set the enqueued buffer as currently displayer one
	gbm_surface_release_buffer(a__kms.gbm.surface, a__kms.gbm.bo_currently_displayed);
	a__kms.gbm.bo_currently_displayed = a__kms.gbm.bo_enqueued_to_flip;

	// There's no buffer enqueued to be flipped
	a__kms.gbm.bo_enqueued_to_flip = NULL;
}

static void pageFlip(struct gbm_bo *bo) {
	// 1. Wait for previous flip, if any
	// React to page flip event
	drmEventContext event_context = {
		.version = DRM_EVENT_CONTEXT_VERSION,
		.page_flip_handler = page_flipped,
	};

	while (a__kms.gbm.bo_enqueued_to_flip) {
		struct pollfd pfd[1] = {{
			.fd = a__kms.drm.fd,
			.events = POLLIN,
		}};
		const int result = poll(pfd, COUNTOF(pfd), -1);
		if (result < 0) {
			if (errno == EINTR)
				continue;
		}
		ATTO_ASSERT(result == 1);
		ATTO_ASSERT(!(pfd[0].revents & POLLERR));

		if (pfd[0].revents & POLLIN)
			drmHandleEvent(a__kms.drm.fd, &event_context);
	}

	// 2. Enqueue the next one
	// drmModePageFlip() operates on fb_id, get one for bo
	const uint32_t framebuffer_id = getFramebufferForGbmBo(bo)->fb_id;

	// Enqueue the flip until the next vblank
	const int ret = drmModePageFlip(a__kms.drm.fd, a__kms.drm.crtc_id, framebuffer_id,
		// NO VSYNC DRM_MODE_PAGE_FLIP_ASYNC |
		DRM_MODE_PAGE_FLIP_EVENT, bo);
	if (ret != 0)
		ALOG("drmModePageFlip returned %d", ret);
	ATTO_ASSERT(ret == 0);

	// Mark this new bo as the one we're waiting to be flipped
	a__kms.gbm.bo_enqueued_to_flip = bo;
}

void a__kmsSwap(void) {
	// Finish rendering previous frame
	eglSwapBuffers(a__kms.egl.display, a__kms.egl.surface);

	// Lock the finished frame
	struct gbm_bo *bo = gbm_surface_lock_front_buffer(a__kms.gbm.surface);

	// Show the locked framebuffer
	// Waits for the previous flip to complete, and then schedules this new one
	pageFlip(bo);
}

void a__kmsDestroy(void) {
	// TODO lol
}
