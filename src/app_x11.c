#ifndef ATTO_APP_WIDTH
#define ATTO_APP_WIDTH 1280
#endif

#ifndef ATTO_APP_HEIGHT
#define ATTO_APP_HEIGHT 720
#endif

#ifndef ATTO_APP_NAME
#define ATTO_APP_NAME "atto app"
#endif

#include <string.h>
#include <stdlib.h> /* exit() */

#include <atto/platform.h>
#include <atto/app.h>

#define GL_GLEXT_PROTOTYPES 1
#include <X11/Xlib.h>
#include <GL/glx.h>

static AGlobalState a__global_state;
const AGlobalState *a_global_state = &a__global_state;

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

static void a__appCleanup(void);
static void a__appProcessXKeyEvent(XEvent *e);

void aAppTerminate(int code) {
	a__appCleanup();
	exit(code);
}

static void a__appProcessXKeyEvent(XEvent *e) {
	int key = AK_Unknown, down = KeyPress == e->type;
	switch(XLookupKeysym(&e->xkey, 0)) {
#define ATTOMAPK__(x,a) case XK_##x: key = AK_##a; break;
		ATTOMAPK__(BackSpace,Backspace) ATTOMAPK__(Tab,Tab)
		ATTOMAPK__(Return,Enter) ATTOMAPK__(space,Space) ATTOMAPK__(Escape,Esc)
		ATTOMAPK__(Page_Up,PageUp) ATTOMAPK__(Page_Down,PageDown)
		ATTOMAPK__(Left,Left) ATTOMAPK__(Up,Up) ATTOMAPK__(Right,Right)
		ATTOMAPK__(Down,Down) ATTOMAPK__(comma,Comma) ATTOMAPK__(minus,Minus)
		ATTOMAPK__(period,Dot) ATTOMAPK__(slash,Slash) ATTOMAPK__(equal,Equal)
		ATTOMAPK__(Delete,Del) ATTOMAPK__(Insert,Ins) ATTOMAPK__(Home,Home)
		ATTOMAPK__(End,End) ATTOMAPK__(asterisk,KeypadAsterisk) ATTOMAPK__(0,0)
		ATTOMAPK__(1,1) ATTOMAPK__(2,2) ATTOMAPK__(3,3) ATTOMAPK__(4,4)
		ATTOMAPK__(5,5) ATTOMAPK__(6,6) ATTOMAPK__(7,7) ATTOMAPK__(8,8)
		ATTOMAPK__(9,9) ATTOMAPK__(a,A) ATTOMAPK__(b,B) ATTOMAPK__(c,C)
		ATTOMAPK__(d,D) ATTOMAPK__(e,E) ATTOMAPK__(f,F) ATTOMAPK__(g,G)
		ATTOMAPK__(h,H) ATTOMAPK__(i,I) ATTOMAPK__(j,J) ATTOMAPK__(k,K)
		ATTOMAPK__(l,L) ATTOMAPK__(m,M) ATTOMAPK__(n,N) ATTOMAPK__(o,O)
		ATTOMAPK__(p,P) ATTOMAPK__(q,Q) ATTOMAPK__(r,R) ATTOMAPK__(s,S)
		ATTOMAPK__(t,T) ATTOMAPK__(u,U) ATTOMAPK__(v,V) ATTOMAPK__(w,W)
		ATTOMAPK__(x,X) ATTOMAPK__(y,Y) ATTOMAPK__(z,Z) ATTOMAPK__(KP_Add,KeypadPlus)
		ATTOMAPK__(KP_Subtract,KeypadMinus) ATTOMAPK__(F1,F1) ATTOMAPK__(F2,F2)
		ATTOMAPK__(F3,F3) ATTOMAPK__(F4,F4) ATTOMAPK__(F5,F5) ATTOMAPK__(F6,F6)
		ATTOMAPK__(F7,F7) ATTOMAPK__(F8,F8) ATTOMAPK__(F9,F9) ATTOMAPK__(F10,F10)
		ATTOMAPK__(F11,F11) ATTOMAPK__(F12,F12) ATTOMAPK__(Alt_L,LeftAlt)
		ATTOMAPK__(Control_L,LeftCtrl) ATTOMAPK__(Meta_L,LeftMeta)
		ATTOMAPK__(Super_L,LeftSuper) ATTOMAPK__(Shift_L,LeftShift)
		ATTOMAPK__(Alt_R,RightAlt) ATTOMAPK__(Control_R,RightCtrl)
		ATTOMAPK__(Meta_R,RightMeta) ATTOMAPK__(Super_R,RightSuper)
		ATTOMAPK__(Shift_R,RightShift) ATTOMAPK__(Caps_Lock,Capslock);
#undef ATTOMAPK_
		default: return;
	}

	a__global_state.keys[key] = down;

	{
		AEvent event;
		event.timestamp = aAppTime();
		event.type = AET_Key;
		event.data.key.key = key;
		event.data.key.down = down;
		atto_app_event(&event);
	}
}

static unsigned int a__appX11ToButton(unsigned int x11btn) {
	return 0
		| ((x11btn & Button1Mask) ? AB_Left : 0)
		| ((x11btn & Button2Mask) ? AB_Middle : 0)
		| ((x11btn & Button3Mask) ? AB_Right : 0)
		| ((x11btn & Button4Mask) ? AB_WheelUp : 0)
		| ((x11btn & Button5Mask) ? AB_WheelDown : 0);
}

static void a__appProcessXButton(const XEvent *e) {
	unsigned int button = 0;
	switch (e->xbutton.button) {
		case Button1: button = AB_Left; break;
		case Button2: button = AB_Middle; break;
		case Button3: button = AB_Right; break;
		case Button4: button = AB_WheelUp; break;
		case Button5: button = AB_WheelDown; break;
	}

	{
		AEvent event;
		event.timestamp = aAppTime();
		event.type = AET_Pointer;
		event.data.pointer.dx = e->xbutton.x - a__global_state.pointer.x;
		event.data.pointer.dy = e->xbutton.y - a__global_state.pointer.y;
		event.data.pointer.buttons_diff = a__appX11ToButton(e->xbutton.state) ^ button;
		a__global_state.pointer.x = e->xbutton.x;
		a__global_state.pointer.y = e->xbutton.y;
		a__global_state.pointer.buttons ^= event.data.pointer.buttons_diff;
		atto_app_event(&event);
	}
}

static void a__appProcessXMotion(const XEvent *e) {
	AEvent event;
	event.timestamp = aAppTime();
	event.type = AET_Pointer;
	event.data.pointer.dx = e->xmotion.x - a__global_state.pointer.x;
	event.data.pointer.dy = e->xmotion.y - a__global_state.pointer.y;
	event.data.pointer.buttons_diff = 0;
	a__global_state.pointer.x = e->xmotion.x;
	a__global_state.pointer.y = e->xmotion.y;
	a__global_state.pointer.buttons = a__appX11ToButton(e->xmotion.state);
	atto_app_event(&event);
}

static Display *display;
static Window window;
static GLXDrawable drawable;
static GLXContext context;

int main(int argc, char *argv[]) {
	AEvent event;
	XSetWindowAttributes winattrs;
	Atom delete_message;
	int nglxconfigs = 0;
	GLXFBConfig *glxconfigs = NULL;
	XVisualInfo *vinfo = NULL;
	ATimeMs last_paint = 0;

	ATTO_ASSERT(display = XOpenDisplay(NULL));

	ATTO_ASSERT(glxconfigs = glXChooseFBConfig(display, 0, a__glxattribs, &nglxconfigs));
	ATTO_ASSERT(nglxconfigs);

	ATTO_ASSERT(vinfo = glXGetVisualFromFBConfig(display, glxconfigs[0]));

	memset(&winattrs, 0, sizeof(winattrs));
	winattrs.event_mask = KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		ExposureMask | VisibilityChangeMask | StructureNotifyMask;
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
	ATTO_ASSERT(window);

	XStoreName(display, window, ATTO_APP_NAME);

	delete_message = XInternAtom(display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(display, window, &delete_message, 1);

	XMapWindow(display, window);

	ATTO_ASSERT(context =
		glXCreateNewContext(display, glxconfigs[0], GLX_RGBA_TYPE, 0, True));

	ATTO_ASSERT(drawable = glXCreateWindow(display, glxconfigs[0], window, 0));

	glXMakeContextCurrent(display, drawable, drawable, context);

	XSelectInput(display, window,
		StructureNotifyMask | KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask
	);

	a__global_state.argc = argc;
	a__global_state.argv = (const char * const *)argv;
	a__global_state.gl_version = AOGLV_21;
	a__global_state.width = ATTO_APP_WIDTH;
	a__global_state.height = ATTO_APP_HEIGHT;

	event.timestamp = aAppTime();
	event.type = AET_Init;
	atto_app_event(&event);
	event.type = AET_Resize;
	atto_app_event(&event);

	for (;;) {
		while (XPending(display)) {
			XEvent e;
			XNextEvent(display, &e);
			switch (e.type) {
				case ConfigureNotify:
					if (a__global_state.width == e.xconfigure.width
							&& a__global_state.height == e.xconfigure.height) break;
					a__global_state.width = e.xconfigure.width;
					a__global_state.height = e.xconfigure.height;
					event.timestamp = aAppTime();
					event.type = AET_Resize;
					atto_app_event(&event);
				break;

				case ButtonPress:
				case ButtonRelease:
					a__appProcessXButton(&e);
					break;
				case MotionNotify:
					a__appProcessXMotion(&e);

				case KeyPress:
				case KeyRelease:
					a__appProcessXKeyEvent(&e);
					break;

				case ClientMessage:
				case DestroyNotify:
				case UnmapNotify:
					goto exit;
					break;
			}
		}

		{
			ATimeMs now = aAppTime();
			if (!last_paint) last_paint = now;
			event.timestamp = now;
			event.type = AET_Paint;
			event.data.paint.dt = (now - last_paint) * 1e-3f;
			atto_app_event(&event);
			glXSwapBuffers(display, drawable);
			last_paint = now;
		}
	}

exit:
	event.timestamp = aAppTime();
	event.type = AET_Close;
	atto_app_event(&event);
	a__appCleanup();
	return 0;
}

static void a__appCleanup() {
	aAppDebugPrintf("cleaning up");
	glXMakeContextCurrent(display, 0, 0, 0);
	glXDestroyWindow(display, drawable);
	glXDestroyContext(display, context);
	XDestroyWindow(display, window);
	XCloseDisplay(display);
}
