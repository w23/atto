/* cc samples/app.c -I. -O3 -Wall -Werror -Wpedantic -lX11 -lGL -o sample_app */

void sample_init(int argc, char *argv[]) {
}

void sample_resize(int w, int h) {
}

void sample_paint(unsigned int time_ms, float dt) {
}

void sample_close() {
}

#define ATTO_APP_INIT_FUNC sample_init
#define ATTO_APP_RESIZE_FUNC sample_resize
#define ATTO_APP_PAINT_FUNC sample_paint
#define ATTO_APP_CLOSE_FUNC sample_close
#define ATTO_APP_H_IMPLEMENT
#include "atto_app.h"
