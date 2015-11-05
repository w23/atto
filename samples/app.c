#include <atto/app.h>

static const char *event_name(AEventType type) {
	switch(type) {
#define CASE_TYPE(type) case type: return #type; break
		CASE_TYPE(AET_Init);
		CASE_TYPE(AET_Resize);
		CASE_TYPE(AET_Paint);
		CASE_TYPE(AET_Key);
		CASE_TYPE(AET_Pointer);
		CASE_TYPE(AET_Close);
#undef CASE_TYPE
	}
	return "Unknown";
}

static const char *opengl_version(AOpenGLVersion ver) {
	switch(ver) {
		case AOGLV_21: return "2.1"; break;
		case AOGLV_ES_20: return "ES 2.0"; break;
	}
	return "Unknown";
}

static ATimeMs fps_time = 0;
static float fps_dt = 0, fps_min_dt = 0, fps_max_dt = 0;
static int fps_frames = 0;

void atto_app_event(const AEvent *event) {
	int i;
	switch(event->type) {
		case AET_Init:
			aAppDebugPrintf("%s[%u]: argc = %d, argv = %p", event_name(event->type),
				event->timestamp, a_app_state->argc, a_app_state->argv);
			for (i = 0; i < a_app_state->argc; ++i)
				aAppDebugPrintf("\targv[%d] = '%s'", i, a_app_state->argv[i]);
			aAppDebugPrintf("\tOpenGL version %s", opengl_version(a_app_state->gl_version));
			break;

		case AET_Resize:
			aAppDebugPrintf("%s[%u]: width = %d, height = %d",
				event_name(event->type), event->timestamp,
				a_app_state->width, a_app_state->height);
			break;

		case AET_Paint:
			fps_dt += event->data.paint.dt;
			fps_min_dt = (fps_min_dt > event->data.paint.dt || !fps_min_dt) ? event->data.paint.dt : fps_min_dt;
			fps_max_dt = (fps_max_dt < event->data.paint.dt) ? event->data.paint.dt : fps_max_dt;
			++fps_frames;
			if (event->timestamp - fps_time > 3000) {
				float sec = (event->timestamp - fps_time) * .001f;
				aAppDebugPrintf("%s[%u]: elapsed: %.2fs, frames = %d, "
					"min_dt = %.2fms, avg_dt = %.2fms, max_dt = %.2fms, avg_fps = %.2f",
					event_name(event->type), event->timestamp, sec, fps_frames,
					fps_min_dt * 1000., fps_dt * 1000. / fps_frames, fps_max_dt * 1000., fps_frames / sec);
				fps_frames = 0;
				fps_min_dt = fps_dt = fps_max_dt = 0;
				fps_time = event->timestamp;
			}
			break;

		case AET_Key:
			aAppDebugPrintf("%s[%u]: key = %d, down = %d",
				event_name(event->type), event->timestamp,
				event->data.key.key, event->data.key.down);
			if (event->data.key.key == AK_Esc)
				aAppTerminate(0);
			break;

		case AET_Pointer:
			aAppDebugPrintf("%s[%u]: x: %d, y: %d, btn: 0x%x, dx: %d, dy: %d, dbtn: 0x%x",
				event_name(event->type), event->timestamp,
				a_app_state->pointer.x, a_app_state->pointer.y, a_app_state->pointer.buttons,
				event->data.pointer.dx, event->data.pointer.dy, event->data.pointer.buttons_diff);
			break;

		case AET_Close:
			aAppDebugPrintf("%s[%u]", event_name(event->type), event->timestamp);
			break;

		default:
			aAppDebugPrintf("%s(%d)[%u]", event_name(event->type), event->type, event->timestamp);
			break;
	}
}
