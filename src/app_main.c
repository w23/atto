#include "atto/app.h"

static struct AAppState a__global_state;
const struct AAppState *a_app_state = &a__global_state;
static struct AAppProctable a__app_proctable;

#ifdef ATTO_KMS
// FIXME evdev
static void a__inputInit(void) {}
static void a__inputPoll(void) {}
static void a__inputDestroy(void) {}

// FIXME ...
#include "app_kms.c"
#define a__videoInit a__kmsInit
#define a__videoSwap a__kmsSwap
#define a__videoDestroy a__kmsDestroy
#endif

static void deinit(void) {
	a__inputDestroy();
	a__videoDestroy();
}

int main(int argc, char *argv[]) {
	a__global_state.argc = argc;
	a__global_state.argv = (const char **)argv;

	a__videoInit(&a__global_state);
	a__inputInit();

	a__global_state.gl_version = AOGLV_ES_20;

	ATimeUs timestamp = aAppTime();
	ATTO_APP_INIT_FUNC(&a__app_proctable);

	if (a__app_proctable.resize)
		a__app_proctable.resize(timestamp, 0, 0);

	ATimeUs last_paint = 0;
	for (;;) {
		ATimeUs now = aAppTime();
		float dt;
		if (!last_paint)
			last_paint = now;
		dt = (now - last_paint) * 1e-6f;

		a__inputPoll();

		if (a__app_proctable.paint)
			a__app_proctable.paint(now, dt);

		a__videoSwap();
		last_paint = now;
	}

	if (a__app_proctable.close)
		a__app_proctable.close();

	deinit();
	return 0;
}

void aAppTerminate(int code) {
	deinit();
	exit(code);
}

void aAppGrabInput(int grab) {
	(void)grab;
	ATTO_ASSERT(!"Not implemented");
	/* No-op. Input is always 'grabbed' in evdev */
	a__global_state.grabbed = grab;
}
