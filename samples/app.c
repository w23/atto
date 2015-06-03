/* cc samples/app.c -I. -O3 -Wall -Werror -Wpedantic -lX11 -lGL -o sample_app */

#define ATTO_APP_INIT_FUNC sample_init
#define ATTO_APP_RESIZE_FUNC sample_resize
#define ATTO_APP_PAINT_FUNC sample_paint
#define ATTO_APP_CLOSE_FUNC sample_close
#define ATTO_APP_KEY_FUNC sample_key
#define ATTO_APP_POINTER_FUNC sample_pointer
#define ATTO_APP_H_IMPLEMENT
#include "atto_app.h"

#include <stdio.h>

#define ATTO_GL_H_IMPLEMENT
#include "atto_gl.h"

void sample_init(int argc, char *argv[]) {
	printf("%s: %d %p\n", __FUNCTION__, argc, (void*)argv);
}

void sample_resize(int w, int h) {
	printf("%s: %d %d\n", __FUNCTION__, w, h);
}

void sample_paint(unsigned int time_ms, float dt) {
	printf("%s: %u %f\n", __FUNCTION__, time_ms, dt);
}

void sample_close() {
	printf("%s\n", __FUNCTION__);
}

void sample_key(int key, int down) {
	printf("%s: %d %d\n", __FUNCTION__, key, down);
	if (key == AK_Esc)
		aAppExit(0);
}

void sample_pointer(int x, int y, unsigned int btns, unsigned int btnsdiff) {
	printf("%s: %d %d %#x %#x\n", __FUNCTION__, x, y, btns, btnsdiff);
}
