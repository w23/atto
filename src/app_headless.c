#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "atto/app.h"

#ifndef COUNTOF
#define COUNTOF(a) (sizeof(a)/sizeof(*(a)))
#endif

static struct {
	EGLDisplay display;
	EGLContext context;
	EGLSurface surface;
} a__headless;

void a__headlessInit(struct AAppState *state) {
	PFNEGLGETPLATFORMDISPLAYEXTPROC getPlatformDisplay =
		(PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");

	// EGL_MESA_platform_surfaceless is the only truly headless EGL platform
	// (no display server, no /dev/dri needed). No KHR equivalent exists.
	// GBM/Wayland/X11 platforms all require hardware or a display server.
	a__headless.display = getPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, NULL, NULL);
	ATTO_ASSERT(a__headless.display != EGL_NO_DISPLAY);

	EGLint vmaj, vmin;
	ATTO_ASSERT(eglInitialize(a__headless.display, &vmaj, &vmin));
	aAppDebugPrintf("EGL surfaceless %d.%d", vmaj, vmin);
	aAppDebugPrintf("EGL_VENDOR: %s", eglQueryString(a__headless.display, EGL_VENDOR));
	aAppDebugPrintf("EGL_VERSION: %s", eglQueryString(a__headless.display, EGL_VERSION));

	// eglChooseConfig doesn't work on surfaceless Mesa, use eglGetConfigs
	EGLConfig configs[256];
	EGLint count = 0;
	eglGetConfigs(a__headless.display, NULL, 0, &count);
	ATTO_ASSERT(count > 0);
	if ((unsigned)count > COUNTOF(configs))
		count = (EGLint)COUNTOF(configs);
	eglGetConfigs(a__headless.display, configs, count, &count);

	// Pick first config (all are equivalent on surfaceless)
	EGLConfig config = configs[0];

	// Create a pbuffer surface — required as a dummy placeholder for
	// eglMakeCurrent. All actual rendering goes to FBOs never to this surface.
	static const EGLint pbuffer_attrs[] = {
		EGL_WIDTH, 64,
		EGL_HEIGHT, 64,
		EGL_NONE
	};
	a__headless.surface = eglCreatePbufferSurface(
		a__headless.display, config, pbuffer_attrs);
	ATTO_ASSERT(a__headless.surface != EGL_NO_SURFACE);

	// Create context (GLES 3)
	static const EGLint ctx_attrs[] = {
		EGL_CONTEXT_MAJOR_VERSION, 3,
		EGL_CONTEXT_MINOR_VERSION, 0,
		EGL_NONE
	};
	a__headless.context = eglCreateContext(
		a__headless.display, config, EGL_NO_CONTEXT, ctx_attrs);
	ATTO_ASSERT(a__headless.context != EGL_NO_CONTEXT);

	ATTO_ASSERT(eglMakeCurrent(a__headless.display,
		a__headless.surface, a__headless.surface, a__headless.context));

	// Default to 1080p virtual resolution.
	// The pbuffer size above is irrelevant — it's never drawn into.
	state->width = 1920;
	state->height = 1080;

	// aoGLVersion enum only has ES20/21; KMS also sets ES20 despite GLES3.
	// The real GL version comes from context creation attrs above.
	state->gl_version = AOGLV_ES_20;
}

void a__headlessSwap(void) {
	// No surface to swap — all rendering is FBO-bound
}

void a__headlessDestroy(void) {
	eglMakeCurrent(a__headless.display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroyContext(a__headless.display, a__headless.context);
	eglDestroySurface(a__headless.display, a__headless.surface);
	eglTerminate(a__headless.display);
}
