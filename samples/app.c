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

void atto_app_event(const AEvent *event) {
	int i;
	switch(event->type) {
		case AET_Init:
			aAppDebugPrintf("%s[%u]: argc = %d, argv = %p", event_name(event->type),
				event->timestamp, a_global_state->argc, a_global_state->argv);
			for (i = 0; i < a_global_state->argc; ++i)
				aAppDebugPrintf("\targv[%d] = '%s'", i, a_global_state->argv[i]);
			aAppDebugPrintf("\tOpenGL version %s", opengl_version(a_global_state->gl_version));
			break;

		case AET_Resize:
			aAppDebugPrintf("%s[%u]: width = %d, height = %d",
				event_name(event->type), event->timestamp,
				a_global_state->width, a_global_state->height);
			break;

		case AET_Paint:
			aAppDebugPrintf("%s[%u]: dt = %f", event_name(event->type),
				event->timestamp, event->data.paint.dt);
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
				a_global_state->pointer.x, a_global_state->pointer.y, a_global_state->pointer.buttons,
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
