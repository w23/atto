#ifndef ATTO_APP_WIDTH
	#define ATTO_APP_WIDTH 1280
#endif

#ifndef ATTO_APP_HEIGHT
	#define ATTO_APP_HEIGHT 720
#endif

#ifndef ATTO_APP_NAME
	#define ATTO_APP_NAME "atto app"
#endif

#include "atto/platform.h"
#include "atto/app.h"
#include "wl-xdg-proto.h"
#include <wayland-client.h>
#include <string.h>
#include <stdlib.h> /* exit() */

// FIXME declare this somewhere properly
void a_vkInitWithWayland(struct wl_display *disp, struct wl_surface *surf);
void a_vkCreateSwapchain(int w, int h);
void a_vkPaint(struct AAppProctable *ptbl, ATimeUs t, float dt);
void a_vkDestroy();
void a_vkDestroySwapchain();

static struct AAppState a__app_state;
const struct AAppState *a_app_state = &a__app_state;
static struct AAppProctable a__app_proctable;

void aAppTerminate(int code) {
	exit(code);
}

void aAppGrabInput(int grab) {
	(void)(grab);
}

static int running = 1;
void aAppClose() {
	running = 0;
}

static struct {
	struct wl_display *disp;
	struct wl_compositor *comp;
	struct xdg_wm_base *xdg_wm;
	struct wl_seat *seat;
} a__wl = {0};

static void noop() {}

static struct wl_pointer_listener pointer_listener = {
	.enter = noop,
	.leave = noop,
	.motion = noop,
	.button = noop,
	.axis = noop,
};

static void key(void *data,
			struct wl_keyboard *wl_keyboard,
			uint32_t serial,
			uint32_t time,
			uint32_t key,
			uint32_t state) {
	aAppDebugPrintf("key: %u state: %u", key, state);

	// KEK
	int akey = 0;
	switch (key) {
		case 17: akey = AK_W; break;
		case 30: akey = AK_A; break;
		case 31: akey = AK_S; break;
		case 32: akey = AK_D; break;
		case 42: akey = AK_LeftShift; break;
		default: return;
	}
	if (a__app_proctable.key)
		a__app_proctable.key(aAppTime(), akey, state);
}

static struct wl_keyboard_listener keyboard_listener = {
 	.keymap = noop,
	.enter = noop,
	.leave = noop,
	.modifiers = noop,
	.repeat_info = noop,
	.key = key,
};

static void seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard *const keyboard = wl_seat_get_keyboard(seat);
		wl_keyboard_add_listener(keyboard, &keyboard_listener, NULL);
	}
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *const pointer = wl_seat_get_pointer(seat);
		wl_pointer_add_listener(pointer, &pointer_listener, NULL);
	}
}

static struct wl_seat_listener seat_listener = {
	.capabilities = seat_capabilities,
};

static void reg_global(void *data, struct wl_registry *reg, uint32_t name, const char *interface, uint32_t version) {
	//printf("%s: %s name=%u ver=%u\n", __FUNCTION__, interface, name, version);
	if (strcmp(interface, wl_compositor_interface.name) == 0) {
		a__wl.comp = wl_registry_bind(reg, name, &wl_compositor_interface, 1);
	} else
	if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		a__wl.xdg_wm = wl_registry_bind(reg, name, &xdg_wm_base_interface, 1);
	} else
	if (strcmp(interface, wl_seat_interface.name) == 0) {
		a__wl.seat = wl_registry_bind(reg, name, &wl_seat_interface, 1);
		wl_seat_add_listener(a__wl.seat, &seat_listener, NULL);
	}
}

static void reg_global_remove(void *data, struct wl_registry *reg, uint32_t name) {
	//printf("%s: name=%u\n", __FUNCTION__, name);
}

static struct wl_registry_listener reg_listen = {
	.global = reg_global,
	.global_remove = reg_global_remove,
};

static void xsurf_configure(void *data,
			  struct xdg_surface *xdg_surface,
			  uint32_t serial) {
	xdg_surface_ack_configure(xdg_surface, serial);
	wl_surface_commit(data);
}

static struct xdg_surface_listener xsurf_listener = {
	.configure = xsurf_configure
};

static void xtop_close(void *data,
				struct xdg_toplevel *xdg_toplevel) {
	//printf("%s\n", __FUNCTION__);
	*(int*)data = 0;
}

static struct xdg_toplevel_listener xtop_listener = {
	.configure = noop,
	.close = xtop_close
};

int main(int argc, char *argv[]) {
	ATimeUs timestamp = aAppTime();

	ATTO_ASSERT(a__wl.disp = wl_display_connect(NULL));

	struct wl_registry *reg = wl_display_get_registry(a__wl.disp);
	wl_registry_add_listener(reg, &reg_listen, NULL);
	wl_display_roundtrip(a__wl.disp);

	struct wl_surface *surf = wl_compositor_create_surface(a__wl.comp);
	struct xdg_surface *xsurf = xdg_wm_base_get_xdg_surface(a__wl.xdg_wm, surf);
	struct xdg_toplevel *xtop = xdg_surface_get_toplevel(xsurf);
	xdg_surface_add_listener(xsurf, &xsurf_listener, surf);
	xdg_toplevel_add_listener(xtop, &xtop_listener, &running);

	wl_surface_commit(surf);
	wl_display_roundtrip(a__wl.disp);

	a_vkInitWithWayland(a__wl.disp, surf);

	a__app_state.argc = argc;
	a__app_state.argv = (const char *const *)argv;
	a__app_state.graphics_version = AOVK10;
	a__app_state.width = 1280;
	a__app_state.height = 720;
	ATTO_APP_INIT_FUNC(&a__app_proctable);

	a_vkCreateSwapchain(a__app_state.width, a__app_state.height);
	a__app_proctable.swapchain_created();

	ATimeUs last_paint = 0;
	while (running) {
		ATimeUs now = aAppTime();
		float dt;
		dt = (now - last_paint) * 1e-6f;
		last_paint = now;

		a_vkPaint(&a__app_proctable, now, dt);

		ATTO_ASSERT(wl_display_dispatch(a__wl.disp) != -1);
	}

	a__app_proctable.swapchain_will_destroy();
	if (a__app_proctable.close)
		a__app_proctable.close();
	a_vkDestroySwapchain();
	a_vkDestroy();
	xdg_toplevel_destroy(xtop);
	xdg_surface_destroy(xsurf);
	wl_surface_destroy(surf);
	wl_display_disconnect(a__wl.disp);
}
