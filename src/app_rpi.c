#include <bcm_host.h>

#include <atto/app.h>

#include "app_egl.c"

static AAppState a__global_state;
const AAppState *a_app_state = &a__global_state;
static EGL_DISPMANX_WINDOW_T a__app_window;

static void a__appCleanup(void);
static void a__app_vc_init(void);

int main(int argc, char *argv[]) {
	AEvent event;
	ATimeMs last_paint = 0;

	a__app_vc_init();
	a__appEglInit(EGL_DEFAULT_DISPLAY, &a__app_window);

	a__global_state.argc = argc;
	a__global_state.argv = (const char**)argv;
	a__global_state.gl_version = AOGLV_ES_20;

	event.timestamp = aAppTime();
	event.type = AET_Init;
	atto_app_event(&event);
	event.type = AET_Resize;
	atto_app_event(&event);

	for (;;) {
		ATimeMs now = aAppTime();
		if (!last_paint) last_paint = now;
		event.timestamp = now;
		event.type = AET_Paint;
		event.data.paint.dt = (now - last_paint) * 1e-3f;
		atto_app_event(&event);
		a__appEglSwap();
		last_paint = now;
	}

	event.timestamp = aAppTime();
	event.type = AET_Close;
	atto_app_event(&event);
	a__appCleanup();
	return 0;
}

void aAppTerminate(int code) {
	if (code >= 0)
		a__appCleanup();
	exit(code);
}

static void a__appCleanup(void) {
	a__appEglDeinit();
	bcm_host_deinit();
}

static void a__app_vc_init(void) {
	DISPMANX_ELEMENT_HANDLE_T dispman_element;
	DISPMANX_DISPLAY_HANDLE_T dispman_display;
	DISPMANX_UPDATE_HANDLE_T dispman_update;
	VC_DISPMANX_ALPHA_T alpha;
	VC_RECT_T dst_rect;
	VC_RECT_T src_rect;
	uint32_t w, h;

	bcm_host_init();

	ATTO_ASSERT(!graphics_get_display_size(0, &w, &h));
	ATTO_ASSERT(w > 0);
	ATTO_ASSERT(h > 0);
	a__global_state.width = w;
	a__global_state.height = h;

	dst_rect.x = dst_rect.y = 0;
	dst_rect.width = w;
	dst_rect.height = h;

	src_rect.x = src_rect.y = 0;
	src_rect.width = w << 16;
	src_rect.height = h << 16;

	alpha.flags = DISPMANX_FLAGS_ALPHA_FIXED_ALL_PIXELS;
	alpha.opacity = 255;
	alpha.mask = 0;

	dispman_display = vc_dispmanx_display_open(0);
	dispman_update = vc_dispmanx_update_start(0);
	dispman_element = vc_dispmanx_element_add(
		dispman_update, dispman_display, 0, &dst_rect, 0, &src_rect,
		DISPMANX_PROTECTION_NONE, &alpha, 0, DISPMANX_NO_ROTATE);

	a__app_window.element = dispman_element;
	a__app_window.width = w;
	a__app_window.height = h;
	vc_dispmanx_update_submit_sync(dispman_update);
}
