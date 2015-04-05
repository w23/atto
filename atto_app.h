#ifndef ATTO_APP_H_
#define ATTO_APP_H_

void aTimeReset();

void aExit(int code);

#ifdef ATTO_APP_H_IMPLEMENT

#ifdef ATTO_APP_H_IMPLEMENTED
#error This header can be implemented only once
#endif

#ifndef ATTO_APP_WIDTH
#define ATTO_APP_WIDTH 1280
#endif

#ifndef ATTO_APP_HEIGHT
#define ATTO_APP_HEIGHT 720
#endif

#ifndef ATTO_APP_INIT_FUNC
#error define ATTO_APP_INIT_FUNC void aAppInit(int argc, const char **argv);
#endif

#ifndef ATTO_APP_RESIZE_FUNC
#error define ATTO_APP_RESIZE_FUNC void aAppResize(int width, int height);
#endif

#ifndef ATTO_APP_PAINT_FUNC
#error define ATTO_APP_PAINT_FUNC void aAppPaint(unsigned int time_ms, float dt);
#endif

/*
#ifndef ATTO_APP_CLOSE_FUNC
#error define ATTO_APP_CLOSE_FUNC void aAppClose();
#endif
*/

#ifdef __linux__
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_LINUX
#define ATTO_PLATFORM_X11
/*
#elif defined(__MACH__) && defined(__APPLE__)
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_OSX
#elif defined(_WIN32)
#define ATTO_PLATFORM_WINDOWS
*/
#else
#error Not ported
#endif

#include <string.h>

#ifdef ATTO_PLATFORM_LINUX
#include <time.h>

#ifdef ATTO_DEBUG
static void a__print_and_exit(const char *message, const char *file, int line) {
  fprintf(stderr, "ERROR in %s:%d: %s\n", file, line, message);
  exit(-1);
}
#define ATTO__CHECK(cond, msg) a__print_and_exit(msg, __FILE__, __LINE__)
#else
#define ATTO__CHECK(cond, msg) {}
#endif

static struct timespec a__time_start = {0, 0};
static unsigned int a__time() {
  struct timespec ts;
  ATTO__CHECK(0 == clock_gettime(CLOCK_MONOTONIC, &ts), "clock_gettime(CLOCK_MONOTONIC)");

  if (a__time_start.tv_sec == 0 && a__time_start.tv_nsec == 0) a__time_start = ts;

  return
    (ts.tv_sec - a__time_start.tv_sec) * 1000 +
    (ts.tv_nsec - a__time_start.tv_nsec) / 1000000;
}

void aTimeReset() {
  a__time_start.tv_sec = a__time_start.tv_nsec = 0;
}
#endif /* ATTO_PLATFORM_LINUX */

#ifdef ATTO_PLATFORM_X11
#include <X11/Xlib.h>
#include <GL/glx.h>

static const int a__glxattribs[] = {
    GLX_X_RENDERABLE, True,
    GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
    GLX_RENDER_TYPE, GLX_RGBA_BIT,
    GLX_CONFIG_CAVEAT, GLX_NONE,
    GLX_RED_SIZE, 8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE, 8,
    GLX_ALPHA_SIZE, 8,
    GLX_DOUBLEBUFFER, True,
    0
  };

static int a__exit_code = 0;
static int a__should_exit = 0;

void aExit(int code) {
  a__exit_code = code;
  a__should_exit = 1;
}

int main(int argc, char *argv[]) {
  Display *display;
  Window window;
  GLXDrawable drawable;
  GLXContext context;
  XSetWindowAttributes winattrs;
  Atom delete_message;
  int nglxconfigs = 0;
  GLXFBConfig *glxconfigs = NULL;
  XVisualInfo *vinfo = NULL;
  unsigned int time_ms = 0;

  display = XOpenDisplay(NULL);
  ATTO__CHECK(display, "XOpenDisplay");

  glxconfigs = glXChooseFBConfig(display, 0, a__glxattribs, &nglxconfigs);
  ATTO__CHECK(glxconfigs && nglxconfigs, "glXChooseFBConfig");

  vinfo = glXGetVisualFromFBConfig(display, glxconfigs[0]);
  ATTO__CHECK(vinfo, "glXGetVisualFromFBConfig");

  memset(&winattrs, 0, sizeof(winattrs));
  winattrs.event_mask =
    ExposureMask | VisibilityChangeMask | StructureNotifyMask |
    KeyPressMask | PointerMotionMask;
  winattrs.border_pixel = 0;
  winattrs.bit_gravity = StaticGravity;
  winattrs.colormap = XCreateColormap(display,
    RootWindow(display, vinfo->screen),
    vinfo->visual, AllocNone);
  winattrs.override_redirect = False;

  window = XCreateWindow(display, RootWindow(display, vinfo->screen),
    0, 0, ATTO_APP_WIDTH, ATTO_APP_HEIGHT,
    0, vinfo->depth, InputOutput, vinfo->visual,
    CWBorderPixel | CWBitGravity | CWEventMask | CWColormap,
    &winattrs);
  ATTO__CHECK(window, "XCreateWindow");

  XStoreName(display, window, "shapa");

  delete_message = XInternAtom(display, "WM_DELETE_WINDOW", True);
  XSetWMProtocols(display, window, &delete_message, 1);

  XMapWindow(display, window);

  context = glXCreateNewContext(display, glxconfigs[0], GLX_RGBA_TYPE, 0, True);
  ATTO__CHECK(context, "glXCreateNewContext");

  drawable = glXCreateWindow(display, glxconfigs[0], window, 0);
  ATTO__CHECK(drawable, "glXCreateWindow");

  glXMakeContextCurrent(display, drawable, drawable, context);

  XSelectInput(display, window, StructureNotifyMask | KeyReleaseMask);

  ATTO_APP_INIT_FUNC(argc, argv);
  ATTO_APP_RESIZE_FUNC(ATTO_APP_WIDTH, ATTO_APP_HEIGHT);

  for (; !a__should_exit;) {
    while (XPending(display)) {
      XEvent e;
      XNextEvent(display, &e);
      switch (e.type) {
        case ConfigureNotify:
          ATTO_APP_RESIZE_FUNC(e.xconfigure.width, e.xconfigure.height);
          break;

        case KeyRelease:
          if (XLookupKeysym(&e.xkey, 0) != XK_Escape)
            break;
        case ClientMessage:
        case DestroyNotify:
        case UnmapNotify:
          a__should_exit = 1;
          break;
      }
    }

    {
      unsigned int now = a__time();
      ATTO_APP_PAINT_FUNC(time_ms, (now - time_ms) * 1e-3f);
      glXSwapBuffers(display, drawable);
      time_ms = now;
    }
  }

#ifdef ATTO_APP_CLOSE_FUNC
  ATTO_APP_CLOSE_FUNC();
#endif

  glXMakeContextCurrent(display, 0, 0, 0);
  glXDestroyWindow(display, drawable);
  glXDestroyContext(display, context);
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  return a__exit_code;
}
#endif /* ATTO_PLATFORM_X11 */

#endif /* ATTO_APP_H_IMPLEMENT */
#endif /* ATTO_APP_H_ */
