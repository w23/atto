#ifndef ATTO_APP_H__DECLARED
#define ATTO_APP_H__DECLARED

/* common platform detection */
#ifndef ATTO_PLATFORM
#ifdef __linux__
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_LINUX
#define ATTO_PLATFORM_X11
/* FIXME this Linux-only definition should be before ANY header inclusion; this requirement also transcends to the user */
/* required for Linux clock_gettime */
#if __STDC_VERSION__ >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif /* __STDC_VERSION__ */
#elif defined(_WIN32)
#define ATTO_PLATFORM_WINDOWS
#elif defined(__MACH__) && defined(__APPLE__)
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_MACH
#define ATTO_PLATFORM_OSX
#define ATTO_PLATFORM_GLUT
#else
#error Not ported
#endif
#define ATTO_PLATFORM
#endif /* ifndef ATTO_PLATFORM */
/* end common platform detection */

unsigned int aTime();

void aAppExit(int code);

/* hide ponter, pin and set it to relative mode
void aAppGrabFocus(int grab);
*/

#ifdef ATTO_APP_KEY_FUNC
typedef enum {
	AK_Unknown = 0,
	AK_Backspace = 8,
	AK_Tab = 9,
	AK_Enter = 13,
	AK_Space  = ' ',
	AK_Esc = 27,
	AK_PageUp = 33,
	AK_PageDown = 34,
	AK_Left = 37,
	AK_Up = 38,
	AK_Right = 39,
	AK_Down = 40,

	AK_Asterisk = '*',
	AK_Plus = '+',
	AK_Comma = ',',
	AK_Minus = '-',
	AK_Dot = '.',
	AK_Slash = '/',
	AK_0 = '0',
	AK_1 = '1',
	AK_2 = '2',
	AK_3 = '3',
	AK_4 = '4',
	AK_5 = '5',
	AK_6 = '6',
	AK_7 = '7',
	AK_8 = '8',
	AK_9 = '9',
	AK_Equal = '=',
	AK_A = 'a',
	AK_B = 'b',
	AK_C = 'c',
	AK_D = 'd',
	AK_E = 'e',
	AK_F = 'f',
	AK_G = 'g',
	AK_H = 'h',
	AK_I = 'i',
	AK_J = 'j',
	AK_K = 'k',
	AK_L = 'l',
	AK_M = 'm',
	AK_N = 'n',
	AK_O = 'o',
	AK_P = 'p',
	AK_Q = 'q',
	AK_R = 'r',
	AK_S = 's',
	AK_T = 't',
	AK_U = 'u',
	AK_V = 'v',
	AK_W = 'w',
	AK_X = 'x',
	AK_Y = 'y',
	AK_Z = 'z',
	AK_Tilda = '~',

	AK_Del = 127,
	AK_Ins,
	AK_LeftAlt,
	AK_LeftCtrl,
	AK_LeftMeta,
	AK_LeftSuper,
	AK_LeftShift,
	AK_RightAlt,
	AK_RightCtrl,
	AK_RightMeta,
	AK_RightSuper,
	AK_RightShift,
	AK_F1, AK_F2, AK_F3, AK_F4, AK_F5, AK_F6, AK_F7, AK_F8, AK_F9, AK_F10,
	AK_F11, AK_F12,
	AK_KeypadAsterisk,
	AK_KeypadPlus, AK_KeypadMinus,
	AK_Home, AK_End,
	AK_Capslock,
	AK_Max = 256
} aKey;

void ATTO_APP_KEY_FUNC(int key, int down);
#endif /* ifdef ATTO_APP_KEY_FUNC */

#ifdef ATTO_APP_POINTER_FUNC
typedef enum {
	AB_Left = 0x01,
	AB_Right = 0x02,
	AB_Middle = 0x04,
	AB_WheelUp = 0x08,
	AB_WheelDown = 0x10
} aButton;

void ATTO_APP_POINTER_FUNC(
	int x, int y,
	unsigned int buttonbits, unsigned int buttonnbitsdiff);
#endif /* ifdef ATTO_APP_POINTER_FUNC */

#endif /* ifndef ATTO_APP_H__DECLARED */


/*****************************************************************************
 * IMPLEMENTATION
 ****************************************************************************/


#ifdef ATTO_APP_H_IMPLEMENT

#ifdef __ATTO_APP_H__IMPLEMENTED
#error atto_app.h can be implemented only once
#endif
#define __ATTO_APP_H__IMPLEMENTED

#ifndef ATTO_APP_WIDTH
#define ATTO_APP_WIDTH 1280
#endif

#ifndef ATTO_APP_HEIGHT
#define ATTO_APP_HEIGHT 720
#endif

#ifndef ATTO_APP_NAME
#define ATTO_APP_NAME "atto app"
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

extern void ATTO_APP_INIT_FUNC(int argc, char *argv[]);
extern void ATTO_APP_RESIZE_FUNC(int width, int height);
extern void ATTO_APP_PAINT_FUNC(unsigned int time_ms, float dt);

#ifdef ATTO_APP_CLOSE_FUNC
extern void ATTO_APP_CLOSE_FUNC();
#endif

#include <string.h>

#ifdef ATTO_PLATFORM_LINUX
#include <time.h>

#ifdef ATTO_DEBUG
static void a__print_and_exit(const char *message, const char *file, int line) {
	fprintf(stderr, "ERROR in %s:%d: %s\n", file, line, message);
	exit(-1);
}
#define ATTO__CHECK(cond, msg) if (!(cond)) a__print_and_exit(msg, __FILE__, __LINE__)
#else
#define ATTO__CHECK(cond, msg) {}
#endif

static struct timespec a__time_start = {0, 0};
unsigned int aTime() {
	struct timespec ts;
	int res = clock_gettime(CLOCK_MONOTONIC, &ts);
	(void)(res);
	ATTO__CHECK(0 == res, "clock_gettime(CLOCK_MONOTONIC)");

	if (a__time_start.tv_sec == 0 && a__time_start.tv_nsec == 0) a__time_start = ts;

	return
		(ts.tv_sec - a__time_start.tv_sec) * 1000 +
		(ts.tv_nsec - a__time_start.tv_nsec) / 1000000;
}

#endif /* ATTO_PLATFORM_LINUX */

#ifdef ATTO_PLATFORM_X11
#define GL_GLEXT_PROTOTYPES 1
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

void aAppExit(int code) {
	a__exit_code = code;
	a__should_exit = 1;
}

static void a__processXKeyEvent(XEvent *e) {
	int key = AK_Unknown, down = (e->type == KeyPress) ? 1 : 0;
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
	ATTO_APP_KEY_FUNC(key, down);
}

static unsigned int a__X11ToButton(unsigned int x11btn) {
	return 0
		| ((x11btn & Button1Mask) ? AB_Left : 0)
		| ((x11btn & Button2Mask) ? AB_Middle : 0)
		| ((x11btn & Button3Mask) ? AB_Right : 0)
		| ((x11btn & Button4Mask) ? AB_WheelUp : 0)
		| ((x11btn & Button5Mask) ? AB_WheelDown : 0);
}

static void a__processXButton(const XEvent *e) {
	unsigned int button = 0;
	switch (e->xbutton.button) {
		case Button1: button = AB_Left; break;
		case Button2: button = AB_Middle; break;
		case Button3: button = AB_Right; break;
		case Button4: button = AB_WheelUp; break;
		case Button5: button = AB_WheelDown; break;
	}
	ATTO_APP_POINTER_FUNC(
		e->xbutton.x, e->xbutton.y,
		a__X11ToButton(e->xbutton.state) ^ button, button
	);
}

static void a__processXMotion(const XEvent *e) {
	ATTO_APP_POINTER_FUNC(
		e->xmotion.x, e->xmotion.y,
		a__X11ToButton(e->xmotion.state), 0
	);
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
	winattrs.event_mask = KeyPressMask | KeyReleaseMask |
#ifdef ATTO_APP_POINTER_FUNC
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
#endif
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
	ATTO__CHECK(window, "XCreateWindow");

	XStoreName(display, window, ATTO_APP_NAME);

	delete_message = XInternAtom(display, "WM_DELETE_WINDOW", True);
	XSetWMProtocols(display, window, &delete_message, 1);

	XMapWindow(display, window);

	context = glXCreateNewContext(display, glxconfigs[0], GLX_RGBA_TYPE, 0, True);
	ATTO__CHECK(context, "glXCreateNewContext");

	drawable = glXCreateWindow(display, glxconfigs[0], window, 0);
	ATTO__CHECK(drawable, "glXCreateWindow");

	glXMakeContextCurrent(display, drawable, drawable, context);

	XSelectInput(display, window,
		StructureNotifyMask | KeyPressMask | KeyReleaseMask |
#ifdef ATTO_APP_POINTER_FUNC
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask
#endif
	);

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

#ifdef ATTO_APP_POINTER_FUNC
				case ButtonPress:
				case ButtonRelease:
					a__processXButton(&e);
					break;
				case MotionNotify:
					a__processXMotion(&e);
#endif

				case KeyPress:
				case KeyRelease:
#ifdef ATTO_APP_KEY_FUNC
					a__processXKeyEvent(&e);
					break;
#else
					if (XLookupKeysym(&e.xkey, 0) != XK_Escape)
						break;
#endif

				case ClientMessage:
				case DestroyNotify:
				case UnmapNotify:
					a__should_exit = 1;
					break;
			}
		}

		{
			unsigned int now = aTime();
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

/*****************************************************************************
 * WINDOWS IMPLEMENTATION
 ****************************************************************************/

#ifdef ATTO_PLATFORM_WINDOWS
#include <windows.h>
#include <gl/gl.h>
#include "wglext.h"

static WCHAR *utf8_to_wchar(const char *string, int length, int *out_length) {
	WCHAR *buffer;
	int buf_length = MultiByteToWideChar(CP_UTF8, 0, string, length, NULL, 0);
	buffer = malloc(buf_length * sizeof(WCHAR)); /* TODO alloc function */
	MultiByteToWideChar(CP_UTF8, 0, string, length, buffer, buf_length);
	if (out_length)
		*out_length = buf_length;
	return buffer;
}

static char *wchar_to_utf8(const WCHAR *string, int length, int *out_length) {
	char *buffer;
	int buf_length = WideCharToMultiByte(CP_UTF8, 0, string, length, NULL, 0, 0, NULL);
	buffer = malloc(buf_length);
	WideCharToMultiByte(CP_UTF8, 0, string, length, buffer, buf_length, 0, NULL);
	if (out_length)
		*out_length = buf_length;
	return buffer;
}

#ifdef ATTO_DEBUG
static void a__message_and_exit(const char *message, const char *file, int line) {
	int err = GetLastError();
	MessageBox(0, utf8_to_wchar(message, -1, NULL), utf8_to_wchar(file, -1, NULL), MB_ICONSTOP | MB_OK);
	ExitProcess(-1);
}
#define ATTO__CHECK(cond, msg) if (!(cond)) a__message_and_exit(msg, __FILE__, __LINE__)
#else
#define ATTO__CHECK(cond, msg) {}
#endif

static LARGE_INTEGER a__time_freq = {0};
static LARGE_INTEGER a__time_start = {0};

unsigned int aTime() {
	LARGE_INTEGER now;
	if (a__time_start.QuadPart == 0) {
		QueryPerformanceFrequency(&a__time_freq);
		QueryPerformanceCounter(&a__time_start);
	}
	QueryPerformanceCounter(&now);
	return (unsigned int)((now.QuadPart - a__time_start.QuadPart) * 1000ull / a__time_freq.QuadPart);
}

void aAppExit(int code) {
	ExitProcess(code);
}

static LRESULT CALLBACK a__window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	switch (msg) {
	case WM_SIZE:
		ATTO_APP_RESIZE_FUNC(lparam & 0xffff, lparam >> 16);
		break;

	case WM_KEYDOWN:
	case WM_KEYUP:
		break;

	case WM_CLOSE:
		ExitProcess(0);
		break;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	return 0;
}

static const PIXELFORMATDESCRIPTOR a__pfd = {
	sizeof(a__pfd), 0, PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
	PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	(void)hPrevInstance;
	(void)lpCmdLine;

	WNDCLASSEX wndclass;
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;
	unsigned int prev_ms = 0;
	int argc;
	LPWSTR* wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	char **argv = malloc(sizeof(char*) * (argc + 1));
	for (int i = 0; i < argc; ++i)
		argv[i] = wchar_to_utf8(wargv[i], -1, NULL);
	
	memset(&wndclass, 0, sizeof(wndclass));
	wndclass.cbSize = sizeof(wndclass);
	wndclass.lpszClassName = TEXT("atto");
	wndclass.lpfnWndProc = a__window_proc;
	wndclass.hInstance = hInstance;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ATTO__CHECK(RegisterClassEx(&wndclass), "RegisterClassEx");

	hwnd = CreateWindow(
		TEXT("atto"), TEXT(ATTO_APP_NAME),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		0, 0, ATTO_APP_WIDTH, ATTO_APP_HEIGHT, NULL, NULL, hInstance, NULL);
	ATTO__CHECK(0 != hwnd, "CreateWindow failed");
	hdc = GetDC(hwnd);
	ATTO__CHECK(0 != hdc, "GetDC failed");

	SetPixelFormat(hdc, ChoosePixelFormat(hdc, &a__pfd), &a__pfd);
	hglrc = wglCreateContext(hdc);
	ATTO__CHECK(0 != hglrc, "wglCreateContext failed");
	wglMakeCurrent(hdc, hglrc);

	ATTO_APP_INIT_FUNC(argc, argv);

	ShowWindow(hwnd, nCmdShow);

	for (;;) {
		MSG msg;
		while (0 != PeekMessage(&msg, hwnd, 0, 0, PM_NOREMOVE)) {
			if (0 == GetMessage(&msg, NULL, 0, 0)) goto exit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		unsigned int now = aTime();
		ATTO_APP_PAINT_FUNC(now, (now - prev_ms) * 1e-3f);
		SwapBuffers(hdc);
		prev_ms = now;
	}

exit:
#ifdef ATTO_APP_CLOSE_FUNC
	ATTO_APP_CLOSE_FUNC();
#endif
	wglMakeCurrent(hdc, NULL);
	wglDeleteContext(hglrc);
	ReleaseDC(hwnd, hdc);
	DestroyWindow(hwnd);
	return 0;
}
#endif /* ATTO_PLATFORM_WINDOWS */

#ifdef ATTO_PLATFORM_GLUT
#include <stdlib.h>
#include <GLUT/glut.h>

static unsigned int a__prev_ms = 0;

unsigned int aTime() {
	return glutGet(GLUT_ELAPSED_TIME);
}

static void a__display_cb(void) {
	unsigned int now = aTime();
	ATTO_APP_PAINT_FUNC(now, (now - a__prev_ms) * 1e-3f);
	a__prev_ms = now;

	glutSwapBuffers();
	glutPostRedisplay();
}

static void a__reshape_cb(int w, int h) {
	ATTO_APP_RESIZE_FUNC(w, h);
}

#ifdef ATTO_APP_KEY_FUNC
static void a__keyb_cb(unsigned char key, int x, int y) {
}
#endif /* ATTO_APP_KEY_FUNC */

#ifdef ATTO_APP_POINTER_FUNC
static void  a__motion_cb(int x, int y) {
}

static void a__mouse_cb(int a, int b, int c, int d) {
}
#endif /* ATTO_APP_POINTER_FUNC */

#ifdef ATTO_APP_CLOSE_FUNC
static void a__close_cb(void) {
	ATTO_APP_CLOSE_FUNC();
}
#endif

void aAppExit(int code) {
	exit(code);
}

int main(int argc, char* argv[]) {
	glutInit(&argc, (char**)argv);
	glutInitWindowSize(ATTO_APP_WIDTH, ATTO_APP_HEIGHT);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutCreateWindow(ATTO_APP_NAME);
	
	ATTO_APP_INIT_FUNC(argc, argv);
	ATTO_APP_RESIZE_FUNC(ATTO_APP_WIDTH, ATTO_APP_HEIGHT);
	
	glutDisplayFunc(a__display_cb);
	glutReshapeFunc(a__reshape_cb);
#ifdef ATTO_APP_KEY_FUNC
	glutKeyboardFunc(a__keyb_cb);
#endif
#ifdef ATTO_APP_POINTER_FUNC
	glutMotionFunc(a__motion_cb);
	glutMouseFunc(a__mouse_cb);
#endif
#ifdef ATTO_APP_CLOSE_FUNC
	glutWMCloseFunc(a__close_cb);
#endif

	a__prev_ms = aTime();
	glutMainLoop();
	return 0;
}
#endif /* ATTO_PLATFORM_GLUT */

#endif /* ATTO_APP_H_IMPLEMENT */
