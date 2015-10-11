#ifndef ATTO_APP_H__DECLARED
#define ATTO_APP_H__DECLARED

typedef unsigned int ATimeMs;

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
} AKey;

typedef enum {
	AB_Left = 0x01,
	AB_Right = 0x02,
	AB_Middle = 0x04,
	AB_WheelUp = 0x08,
	AB_WheelDown = 0x10
} AButton;

typedef enum {
	AET_Init,
	AET_Resize,
	AET_Paint,
	AET_Key,
	AET_Pointer,
	AET_Close
} AEventType;

typedef enum {
	AOGLV_21,
	AOGLV_ES_20
} AOpenGLVersion;

typedef struct {
	ATimeMs timestamp;
	AEventType type;
	union {
		struct {
			float dt;
		} paint;
		struct {
			AKey key;
			int down;
		} key;
		struct {
			int dx, dy;
			unsigned int buttons_diff;
		} pointer;
	} data;
} AEvent;

typedef struct {
	int argc;
	const char *const *argv;
	AOpenGLVersion gl_version;
	int width, height;
	AKey keys[AK_Max];
	struct {
		int x, y;
		unsigned int buttons;
	} pointer;
} AAppState;

extern const AAppState *a_app_state;

ATimeMs aAppTime(void);

void aAppDebugPrintf(const char *fmt, ...);

/* Immediately terminate the current process */
void aAppTerminate(int code);

/* A program that uses atto/app must implement just this one function.
 * atto/app guarantees that:
 *  - this function will be called from one thread only
 *  - that thread always has a valid OpenGL context installed
 *  - the first event is always AET_Init
 *  - the first AET_Resize will always precede the first AET_Paint
 */
void atto_app_event(const AEvent *event);

#define ATTO_ASSERT(cond) \
	if (!(cond)) { \
		aAppDebugPrintf("ERROR @ %s:%d: (%s) failed", __FILE__, __LINE__, #cond); \
		aAppTerminate(-1); \
	}

#endif /* ifndef ATTO_APP_H__DECLARED */
