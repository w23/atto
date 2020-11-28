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
#include <unistd.h>

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
	// FIXME:
	// https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/master/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
	// https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/master/unstable/relative-pointer/relative-pointer-unstable-v1.xml
	a__app_state.grabbed = grab;
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

static void motion(void *data,
			 struct wl_pointer *wl_pointer,
			 uint32_t time,
			 wl_fixed_t surface_x,
			 wl_fixed_t surface_y) {
	const int x = wl_fixed_to_int(surface_x);
	const int y = wl_fixed_to_int(surface_y);

	// TODO track enter/leave
	const int dx = x - a__app_state.pointer.x;
	const int dy = y - a__app_state.pointer.y;

	a__app_state.pointer.x = x;
	a__app_state.pointer.y = y;

	if (!a__app_proctable.pointer)
		return;

	const ATimeUs now = aAppTime(); // FIXME convert wl time
	a__app_proctable.pointer(now, dx, dy, 0);
}

static void button(void *data,
			 struct wl_pointer *wl_pointer,
			 uint32_t serial,
			 uint32_t time,
			 uint32_t button,
			 uint32_t state) {
	const uint32_t button_bit = 1 << (button-0x110);
	switch (state) {
		case WL_POINTER_BUTTON_STATE_PRESSED:
			a__app_state.pointer.buttons |= button_bit;
			break;
		case WL_POINTER_BUTTON_STATE_RELEASED:
			a__app_state.pointer.buttons &= ~button_bit;
			break;
	}

	if (!a__app_proctable.pointer)
		return;

	const ATimeUs now = aAppTime(); // FIXME convert wl time
	a__app_proctable.pointer(now, 0, 0, button_bit);
}

static struct wl_pointer_listener pointer_listener = {
	.enter = noop,
	.leave = noop,
	.motion = motion,
	.button = button,
	.axis = noop,
};

static void key(void *data,
			struct wl_keyboard *wl_keyboard,
			uint32_t serial,
			uint32_t time,
			uint32_t key,
			uint32_t state) {
	aAppDebugPrintf("key: %u state: %u", key, state);

	// FIXME do this properly w/ xkb and all that jazz
	int akey = 0;
	switch (key) {
		case 1: akey = AK_Esc; break;
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

static void keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) {
	/* char *keymap_string = mmap (NULL, size, PROT_READ, MAP_SHARED, fd, 0); */
	/* xkb_keymap_unref (keymap); */
	/* keymap = xkb_keymap_new_from_string (xkb_context, keymap_string, XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS); */
	/* munmap (keymap_string, size); */
	close (fd);
	/* xkb_state_unref (xkb_state); */
	/* xkb_state = xkb_state_new (keymap); */
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
	aAppDebugPrintf("%s", __FUNCTION__);
}

static struct xdg_surface_listener xsurf_listener = {
	.configure = xsurf_configure
};

static void xtop_close(void *data,
				struct xdg_toplevel *xdg_toplevel) {
	*(int*)data = 0;
}

static void xtop_configure(void *data,
			struct xdg_toplevel *xdg_toplevel,
			int32_t width,
			int32_t height,
			struct wl_array *states) {
	aAppDebugPrintf("%s %d %d", __FUNCTION__, width, height);
	if (width == 0 || height == 0)
		return;

	if (width == a__app_state.width && height == a__app_state.height)
		return;

	if (a__app_proctable.swapchain_will_destroy)
		a__app_proctable.swapchain_will_destroy();

	a__app_state.width = width;
	a__app_state.height = height;

	a_vkCreateSwapchain(width, height);
	if (a__app_proctable.swapchain_created)
		a__app_proctable.swapchain_created();
}

static struct xdg_toplevel_listener xtop_listener = {
	.configure = xtop_configure,
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

	aAppDebugPrintf("post-init");

	a_vkCreateSwapchain(a__app_state.width, a__app_state.height);
	a__app_proctable.swapchain_created();

	aAppDebugPrintf("post-swapchain");

	ATimeUs last_paint = 0;
	while (running) {
		ATimeUs now = aAppTime();
		float dt;
		dt = (now - last_paint) * 1e-6f;
		last_paint = now;

		//aAppDebugPrintf("DRAW");
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
