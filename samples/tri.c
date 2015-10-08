#include <math.h>
#include <atto/app.h>

#define ATTO_GL_H_IMPLEMENT
#include <atto/gl.h>

void atto_app_event(const AEvent *event) {
	switch(event->type) {
		case AET_Init:
			aGLInit();
			break;

		case AET_Resize:
			glViewport(0, 0, a_global_state->width, a_global_state->height);
			break;

		case AET_Paint:
			glClearColor(
				sinf(event->timestamp*.001f),
				sinf(event->timestamp*.002f),
				sinf(event->timestamp*.003f),
				1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			break;

		case AET_Key:
			if (event->data.key.key == AK_Esc)
				aAppTerminate(0);
			break;

		default: break;
	}
}
