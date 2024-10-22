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
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <windowsx.h> /* GET_X/Y_LPARAM */
#include <GL/gl.h>
/* #include "wglext.h"
#include <atto/platform.h> */
#include <atto/app.h>

#include <ShellScalingApi.h> // GetDpiForMonitor() and friends
#include <Setupapi.h> // SetupDiGetClassDevsEx()
#include <initguid.h> // Must be before ntddvdeo.h to avoid linking errors (?! wtf)
#include <ntddvdeo.h> // GUID_DEVINTERFACE_MONITOR

/* static WCHAR *utf8_to_wchar(const char *string, int length, int *out_length); */
static char *wchar_to_utf8(const WCHAR *string, int length, int *out_length);
static void a__AppOpenConsole(void);
static void a__AppCleanup(void);
static AKey a__AppMapKey(WPARAM key);
static LRESULT CALLBACK a__AppWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

static struct AAppState a__app_state;
const struct AAppState *a_app_state = &a__app_state;
static struct AAppProctable a__app_proctable;

static const PIXELFORMATDESCRIPTOR a__pfd = {sizeof(a__pfd), 0,
	PFD_DOUBLEBUFFER | PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL, PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0};

static LARGE_INTEGER a__time_freq;
static LARGE_INTEGER a__time_start;

#define A__MAX_DISPLAYS 16

static struct {
	HWND hwnd;
	HDC hdc;
	HGLRC hglrc;

	struct {
		int resetAbsolute;
		long x, y;
	} rawMouse;

	int displays_count;
	AAppDisplay displays[A__MAX_DISPLAYS];
} g;

ATimeUs aAppTime(void) {
	LARGE_INTEGER now;
	if (a__time_start.QuadPart == 0) {
		QueryPerformanceFrequency(&a__time_freq);
		QueryPerformanceCounter(&a__time_start);
	}
	QueryPerformanceCounter(&now);
	return (ATimeUs)((now.QuadPart - a__time_start.QuadPart) * 1000000ul / a__time_freq.QuadPart);
}

void aAppDebugPrintf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "DBG: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	fflush(stderr);
	va_end(args);
}

void aAppTerminate(int code) {
	a__AppCleanup();
	ExitProcess(code);
}

static void printError(const char *msg, DWORD error) {
	LPSTR pbuf = NULL;
	const DWORD size = FormatMessageA(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, error,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&pbuf, 0, NULL
	);
	aAppDebugPrintf("ERROR: %s: code=%#lx(%lu): %.*s", msg, error, error, (int)size, pbuf);
	LocalFree(pbuf);
}

#define printLastError(msg) printError(msg, GetLastError())

static void printDisplayDevice(const char *prefix, const DISPLAY_DEVICE *dd) {
	aAppDebugPrintf("%sDeviceName: %s", prefix, dd->DeviceName);
	aAppDebugPrintf("%sDeviceString: %s", prefix, dd->DeviceString);
	aAppDebugPrintf("%sStateFlags: %#lx", prefix, dd->StateFlags);
	aAppDebugPrintf("%sDeviceID: %s", prefix, dd->DeviceID);
	aAppDebugPrintf("%sDeviceKey: %s", prefix, dd->DeviceKey);
}

typedef struct {
	BYTE data[128];
} EDID;

static BOOL readEdidForDisplayPath(const char *monitor_id, EDID *out_edid) {
	BOOL result = FALSE;

	// Start a query for all monitors device interfaces
	const HDEVINFO devs =
			SetupDiGetClassDevs(&GUID_DEVINTERFACE_MONITOR, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (devs == INVALID_HANDLE_VALUE) {
		printLastError("SetupDiGetClassDevs");
		return FALSE;
	}

	// Iterate through all monitor device interfaces
	for (DWORD iface_index = 0;; iface_index++) {
		// Get device interface data. It's needed in order to get interface detail
		SP_DEVICE_INTERFACE_DATA iface_data = {
				.cbSize = sizeof(iface_data),
		};
		if (!SetupDiEnumDeviceInterfaces(devs, NULL, &GUID_DEVINTERFACE_MONITOR, iface_index, &iface_data))
			break;
		aAppDebugPrintf("    iface[%lu]:", iface_index);

		// Get interface detail. It contains device path string, which is the same
		// as DeviceID for DISPLAY_DEVICE, see EDD_GET_DEVICE_INTERFACE_NAME flag
		// comment below.

		// Interface detail is obtained in two steps:
		// 1. First, read the needed size for the string
		DWORD detail_size = 0;
		if (!SetupDiGetDeviceInterfaceDetail(devs, &iface_data, NULL, 0, &detail_size, NULL)) {
			const DWORD err = GetLastError();
			if (err != ERROR_INSUFFICIENT_BUFFER) {
				printError("SetupDiGetDeviceInterfaceDetail -- get size", err);
				continue;
			}
		}

		// Allocate the structure with the required size.
		aAppDebugPrintf("      detail_size: %lu", detail_size);
		SP_INTERFACE_DEVICE_DETAIL_DATA *detail = malloc(detail_size);
		// Note how cbSize contains the static structure part, not the full payload.
		detail->cbSize = sizeof(*detail);

		// 2. Obtain the detail value fully. Also, obatin SP_DEVINFO_DATA, it is
		// needed to get the registry key
		SP_DEVINFO_DATA dev_data = {
				.cbSize = sizeof(dev_data),
		};
		if (!SetupDiGetDeviceInterfaceDetail(devs, &iface_data, detail, detail_size, NULL, &dev_data)) {
			printLastError("SetupDiGetDeviceInterfaceDetail -- get detail");
			free(detail);
			continue;
		}
		aAppDebugPrintf("      detail: %s", detail->DevicePath);

		const BOOL found = (_stricmp(detail->DevicePath, monitor_id) == 0);

		// Not needed anymore
		free(detail);

		if (!found)
			continue;

		aAppDebugPrintf("      DevicePath matches monitor");

		const HKEY key = SetupDiOpenDevRegKey(devs, &dev_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
		if (key == INVALID_HANDLE_VALUE) {
			printLastError("SetupDiOpenDevRegKey");
			continue;
		}

		DWORD edid_size = sizeof(*out_edid);
		DWORD reg_type;

		const DWORD read_result = RegQueryValueEx(key, "EDID", NULL, &reg_type, out_edid->data, &edid_size);

		// Not needed anymore
		RegCloseKey(key);

		if (read_result != ERROR_SUCCESS) {
			printError("SetupDiOpenDevRegKey", read_result);
			continue;
		}

		// TODO check edid_size
		// TODO check reg_type

		// Reached here = found edid, may exit
		aAppDebugPrintf("      Read %lu EDID bytes", edid_size);
		result = TRUE;
		break;
	}

	SetupDiDestroyDeviceInfoList(devs);
	return result;
}

static BOOL readEdidFromDisplayName(LPSTR device_name, EDID *out_edid) {
	// EDD_GET_DEVICE_INTERFACE_NAME specifies that DISPLAY_DEVICE.DeviceID field
	// will contain interface name that is congruent to GUID_DEVINTERFACE_MONITOR
	// detail DevicePath. This is used in readEdidForDisplayPath().
	DISPLAY_DEVICE monitor = {
			.cb = sizeof(monitor),
	};
	if (0 == EnumDisplayDevices(device_name, 0, &monitor, EDD_GET_DEVICE_INTERFACE_NAME)) {
		printLastError("EnumDisplayDevices");
		return FALSE;
	}

	aAppDebugPrintf("  DISPLAY_DEVICE(%s):", device_name);
	printDisplayDevice("    ", &monitor);

	return readEdidForDisplayPath(monitor.DeviceID, out_edid);
}

typedef struct {
	int x, y, w, h;
	int raw_dpi;
	EDID edid;
} DisplayInfo;

/*
static void printDisplayInfo(const DisplayInfo *info) {
	aAppDebugPrintf("#####################################################################");
	aAppDebugPrintf("> FOUND DISPLAY %d,%d %dx%d dpi=%d", info->x, info->y, info->w, info->h, info->raw_dpi);
	const int block_size = 16;
	for (int i = 0; i < COUNTOF(info->edid.data); i += block_size) {
		const BYTE *const b = info->edid.data + i;
		fprintf(stderr, ">  ");
		for (int j = 0; j < block_size; ++j)
			fprintf(stderr, "%02x ", b[j]);
		fprintf(stderr, "| ");
		for (int j = 0; j < block_size; ++j)
			fprintf(stderr, "%c", isprint(b[j]) ? b[j] : '.');
		fprintf(stderr, "\n");
	}
	aAppDebugPrintf("#####################################################################");
}
*/

static BOOL CALLBACK monitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
	aAppDebugPrintf("monitorEnumProc(hMonitor=%p):", hMonitor);
	(void)hdcMonitor;
	(void)lprcMonitor;
	(void)dwData;

	MONITORINFOEX minfo = {
			.cbSize = sizeof(minfo),
	};
	if (!GetMonitorInfo(hMonitor, (LPMONITORINFO)&minfo)) {
		printLastError("GetMonitorInfo");
		return TRUE;
	}

	aAppDebugPrintf("  szDevice: %s", minfo.szDevice);
	aAppDebugPrintf(
			"  rcMonitor: left=%ld top=%ld right=%ld bottom=%ld", minfo.rcMonitor.left, minfo.rcMonitor.top,
			minfo.rcMonitor.right, minfo.rcMonitor.bottom
	);
	aAppDebugPrintf(
			"  rcWork: left=%ld top=%ld right=%ld bottom=%ld", minfo.rcWork.left, minfo.rcWork.top, minfo.rcWork.right,
			minfo.rcWork.bottom
	);
	aAppDebugPrintf("  dwFlags: %#lx%s", minfo.dwFlags, minfo.dwFlags & MONITORINFOF_PRIMARY ? ": PRIMARY" : "");

	DEVMODE dev_mode = {
			.dmSize = sizeof(dev_mode),
	};
	if (!EnumDisplaySettings(minfo.szDevice, ENUM_CURRENT_SETTINGS, &dev_mode)) {
		printLastError("EnumDisplaySettings");
		return TRUE;
	}

	aAppDebugPrintf("  DEVMODE:");
	aAppDebugPrintf("    dmDeviceName: %s", dev_mode.dmDeviceName);
	aAppDebugPrintf("    dmFields: %#lx", dev_mode.dmFields);
	aAppDebugPrintf("    dmPelsWH: %lux%lu", dev_mode.dmPelsWidth, dev_mode.dmPelsHeight);
	aAppDebugPrintf("    dmBitsPerPel: %lu", dev_mode.dmBitsPerPel);
	aAppDebugPrintf("    dmDisplayFrequency: %lu", dev_mode.dmDisplayFrequency);

	UINT raw_dpi_x, raw_dpi_y, eff_dpi_x, eff_dpi_y;
	GetDpiForMonitor(hMonitor, MDT_RAW_DPI, &raw_dpi_x, &raw_dpi_y);
	GetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &eff_dpi_x, &eff_dpi_y);
	aAppDebugPrintf("  DPI: effective=(%u, %u), raw=(%u, %u)", eff_dpi_x, eff_dpi_y, raw_dpi_y, raw_dpi_y);

	DisplayInfo info = {
			.x = minfo.rcMonitor.left,
			.y = minfo.rcMonitor.top,
			.w = dev_mode.dmPelsWidth,
			.h = dev_mode.dmPelsHeight,
			.raw_dpi = raw_dpi_x,
	};
	if (!readEdidFromDisplayName(minfo.szDevice, &info.edid)) {
		aAppDebugPrintf("  Couldn't read EDID for this monitor, oh well");
		return TRUE;
	}

	//printDisplayInfo(&info);

	if (g.displays_count < A__MAX_DISPLAYS) {
		AAppDisplay *const disp = &g.displays[g.displays_count++];
		*disp = (AAppDisplay) {
			.name = NULL,
			.flags = AAPP_DISPLAY_HAS_EDID,
			.width = info.w,
			.height = info.h,
			._.x = info.x,
			._.y = info.y,
		};
		memcpy(disp->edid, info.edid.data, sizeof(disp->edid));
	}

	return TRUE;
}

static void enumerateDisplays(void) {
	EnumDisplayMonitors(NULL, NULL, monitorEnumProc, 0);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	ATimeUs last_paint = 0;
	int x = 0, y = 0, w = ATTO_APP_WIDTH, h = ATTO_APP_HEIGHT, fullscreen = 0;

	(void)hPrevInstance;
	(void)lpCmdLine;

	//a__AppOpenConsole();

	{
		int i;
		char **argv;
		LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &a__app_state.argc);
		argv = malloc(sizeof(char *) * (a__app_state.argc + 1));
		for (i = 0; i < a__app_state.argc; ++i) argv[i] = wchar_to_utf8(wargv[i], -1, NULL);
		a__app_state.argv = (const char **)argv;
	}

#ifdef ATTO_APP_PREINIT_FUNC
	{
		enumerateDisplays();
		const AAppPreinitArgs args = {
			.argc = a__app_state.argc,
			.argv = (char *const *const)a__app_state.argv,
			.displays = g.displays,
			.displays_count = g.displays_count,
		};
		const AAppPreinitResult result = ATTO_APP_PREINIT_FUNC(&args);

		if (result.fullscreen_display_index >= 0) {
			const AAppDisplay *const disp = args.displays + result.fullscreen_display_index;
			x = disp->_.x;
			y = disp->_.y;
			w = disp->width;
			h = disp->height;
			fullscreen = 1;
			aAppDebugPrintf("Making a fullscreen window at display[%d] %s %d,%d %dx%d",
				result.fullscreen_display_index, disp->name,
				disp->_.x, disp->_.y, disp->width, disp->height);
		}
	}
#endif
	
	{
		const WNDCLASSEX wndclass = {
			.cbSize = sizeof(wndclass),
			.lpszClassName = TEXT("atto"),
			.lpfnWndProc = a__AppWndProc,
			.hInstance = hInstance,
			.hCursor = LoadCursor(NULL, IDC_ARROW),
			.hIcon = LoadIcon(NULL, IDI_APPLICATION),
			.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		};
		ATTO_ASSERT(RegisterClassEx(&wndclass));
	}

	g.hwnd = CreateWindow(TEXT("atto"), TEXT(ATTO_APP_NAME),
		fullscreen
		? WS_POPUP | WS_VISIBLE
		: WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		x, y, w, h,
		NULL, NULL, hInstance, NULL);
	ATTO_ASSERT(0 != g.hwnd);
	g.hdc = GetDC(g.hwnd);
	ATTO_ASSERT(0 != g.hdc);

	SetPixelFormat(g.hdc, ChoosePixelFormat(g.hdc, &a__pfd), &a__pfd);
	g.hglrc = wglCreateContext(g.hdc);
	ATTO_ASSERT(0 != g.hglrc);
	wglMakeCurrent(g.hdc, g.hglrc);

	a__app_state.gl_version = AOGLV_21;
	a__app_state.width = w;
	a__app_state.height = h;

	ATTO_APP_INIT_FUNC(&a__app_proctable);
	if (a__app_proctable.resize)
		a__app_proctable.resize(aAppTime(), 0, 0);

	ShowWindow(g.hwnd, nCmdShow);

	for (;;) {
		MSG msg;
		while (0 != PeekMessage(&msg, g.hwnd, 0, 0, PM_NOREMOVE)) {
			if (0 == GetMessage(&msg, NULL, 0, 0))
				goto exit;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		{
			ATimeUs now = aAppTime();
			if (!last_paint)
				last_paint = now;
			float dt = (now - last_paint) * 1e-6f;
			if (a__app_proctable.paint)
				a__app_proctable.paint(now, dt);
			SwapBuffers(g.hdc);
			last_paint = now;
		}
	}

exit:
	if (a__app_proctable.close)
		a__app_proctable.close();
	a__AppCleanup();
	return 0;
}

static void a__AppCleanup(void) {
	wglMakeCurrent(g.hdc, NULL);
	wglDeleteContext(g.hglrc);
	ReleaseDC(g.hwnd, g.hdc);
	DestroyWindow(g.hwnd);
}

static void a__AppOpenConsole(void) {
	AllocConsole();
	FILE *dummy_file;
	freopen_s(&dummy_file, "CONIN$", "r", stdin);
	freopen_s(&dummy_file, "CONOUT$", "w", stdout);
	freopen_s(&dummy_file, "CONOUT$", "w", stderr);
}

#if 0
static WCHAR *utf8_to_wchar(const char *string, int length, int *out_length) {
	WCHAR *buffer;
	int buf_length = MultiByteToWideChar(CP_UTF8, 0, string, length, NULL, 0);
	buffer = malloc(buf_length * sizeof(WCHAR)); /* TODO alloc function */
	MultiByteToWideChar(CP_UTF8, 0, string, length, buffer, buf_length);
	if (out_length)
		*out_length = buf_length;
	return buffer;
}
#endif

static char *wchar_to_utf8(const WCHAR *string, int length, int *out_length) {
	char *buffer;
	int buf_length = WideCharToMultiByte(CP_UTF8, 0, string, length, NULL, 0, 0, NULL);
	buffer = malloc(buf_length);
	WideCharToMultiByte(CP_UTF8, 0, string, length, buffer, buf_length, 0, NULL);
	if (out_length)
		*out_length = buf_length;
	return buffer;
}

/*
#ifdef ATTO_DEBUG
static void a__message_and_exit(const char *message, const char *file, int line) {
	int err = GetLastError();
	MessageBox(0, utf8_to_wchar(message, -1, NULL), utf8_to_wchar(file, -1, NULL), MB_ICONSTOP | MB_OK);
	ExitProcess(-1);
}
#define ATTO_ASSERT(cond, msg) if (!(cond)) a__message_and_exit(msg, __FILE__, __LINE__)
#else
#define ATTO_ASSERT(cond, msg) {}
#endif
*/

static AKey a__AppMapKey(WPARAM key) {
	if (key >= 'A' && key <= 'Z')
		return AK_A + key - 'A';
	if (key >= '0' && key <= '9')
		return AK_A + key - '0';

	switch (key) {
	case VK_BACK: return AK_Backspace;
	case VK_TAB: return AK_Tab;
	case VK_RETURN: return AK_Enter;
	case VK_LCONTROL: return AK_LeftCtrl;
	case VK_RCONTROL: return AK_RightCtrl;
	case VK_LMENU: return AK_LeftAlt;
	case VK_RMENU: return AK_RightAlt;
	case VK_CAPITAL: return AK_Capslock;
	case VK_ESCAPE: return AK_Esc;
	case VK_SPACE: return AK_Space;
	case VK_HOME: return AK_Home;
	case VK_LEFT: return AK_Left;
	case VK_UP: return AK_Up;
	case VK_RIGHT: return AK_Right;
	case VK_DOWN: return AK_Down;
	/* case VK_SNAPSHOT: return AK_Printscreen; */
	case VK_INSERT: return AK_Ins;
	case VK_DELETE: return AK_Del;
	case VK_SHIFT: return AK_LeftShift;
	case VK_LSHIFT: return AK_LeftShift;
	case VK_RSHIFT: return AK_RightShift;
	case VK_F1: return AK_F1;
	case VK_F2: return AK_F2;
	case VK_F3: return AK_F3;
	case VK_F4: return AK_F4;
	case VK_F5: return AK_F5;
	case VK_F6: return AK_F6;
	case VK_F7: return AK_F7;
	case VK_F8: return AK_F8;
	case VK_F9: return AK_F9;
	case VK_F10: return AK_F10;
	case VK_F11: return AK_F11;
	case VK_F12: return AK_F12;
	case VK_OEM_MINUS: return AK_Minus;
	case VK_OEM_PLUS: return AK_Plus;
	}
	return AK_Unknown;
}

static LRESULT CALLBACK a__AppWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	int down = 0;
	AKey key;

	enum {
		NoEvent,
		MouseClickEvent,
	} late_event = NoEvent;
	unsigned int new_button_state = a__app_state.pointer.buttons;

	switch (msg) {
	case WM_SIZE: {
		const unsigned int oldw = a__app_state.width, oldh = a__app_state.height;
		const unsigned int width = (unsigned int)(lparam & 0xffff), height = (unsigned int)(lparam >> 16);
		if (a__app_state.width == width && a__app_state.height == height)
			break;
		a__app_state.width = width;
		a__app_state.height = height;
		if (a__app_proctable.resize)
			a__app_proctable.resize(aAppTime(), oldw, oldh);
	} break;

	case WM_KEYDOWN: down = 1;
	case WM_KEYUP:
		key = a__AppMapKey(wparam);
		if (key == AK_Unknown)
			break;

		if (a__app_state.keys[key] == down)
			break;

		a__app_state.keys[key] = down;

		if (a__app_proctable.key)
			a__app_proctable.key(aAppTime(), key, down);
		break;

	case WM_CLOSE: ExitProcess(0); break;

	case WM_LBUTTONDOWN:
		late_event = MouseClickEvent;
		new_button_state |= AB_Left;
		break;
	case WM_LBUTTONUP:
		late_event = MouseClickEvent;
		new_button_state &= ~AB_Left;
		break;
	case WM_RBUTTONDOWN:
		late_event = MouseClickEvent;
		new_button_state |= AB_Right;
		break;
	case WM_RBUTTONUP:
		late_event = MouseClickEvent;
		new_button_state &= ~AB_Right;
		break;

	case WM_MOUSEMOVE:
		if (!a__app_state.grabbed) {
			const int x = GET_X_LPARAM(lparam), y = GET_Y_LPARAM(lparam);
			const int dx = x - a__app_state.pointer.x, dy = y - a__app_state.pointer.y;
			a__app_state.pointer.x = x;
			a__app_state.pointer.y = y;

			if (a__app_proctable.pointer)
				a__app_proctable.pointer(aAppTime(), dx, dy, 0);
		}
		break;

	case WM_INPUT: {
		RAWINPUT ri;
		UINT ri_size = sizeof(ri);
		const HRAWINPUT hrawinput = (HRAWINPUT)lparam;
		GetRawInputData(hrawinput, RID_INPUT, &ri, &ri_size, sizeof(RAWINPUTHEADER));

		switch (ri.header.dwType) {
		case RIM_TYPEMOUSE:
			if (!a__app_proctable.pointer)
				break;

			/*aAppDebugPrintf("%02x %u %04x %u %04x %d %d %u",
				ri.data.mouse.usFlags,
				ri.data.mouse.ulButtons,
				ri.data.mouse.usButtonFlags,
				ri.data.mouse.usButtonData,
				ri.data.mouse.ulRawButtons,
				ri.data.mouse.lLastX,
				ri.data.mouse.lLastY,
				ri.data.mouse.ulExtraInformation);*/

			if (ri.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) {
				if (g.rawMouse.resetAbsolute)
					g.rawMouse.resetAbsolute = 0;
				else {
					// TODO check on other absolute devices besides Synergy
					const int dx = (ri.data.mouse.lLastX - g.rawMouse.x) * GetSystemMetrics(SM_CXSCREEN) / 65536;
					const int dy = (ri.data.mouse.lLastY - g.rawMouse.y) * GetSystemMetrics(SM_CYSCREEN) / 65536;

					a__app_proctable.pointer(aAppTime(), dx, dy, 0);
				}

				g.rawMouse.x = ri.data.mouse.lLastX;
				g.rawMouse.y = ri.data.mouse.lLastY;
			} else if (ri.data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
				g.rawMouse.resetAbsolute = 1;
				a__app_proctable.pointer(aAppTime(), ri.data.mouse.lLastX, ri.data.mouse.lLastY, 0);
			}

			break;
		}
	} break;

	default: return DefWindowProc(hwnd, msg, wparam, lparam);
	}

	if (late_event == MouseClickEvent && new_button_state != a__app_state.pointer.buttons) {
		const unsigned int btn_diff = a__app_state.pointer.buttons ^ new_button_state;
		a__app_state.pointer.buttons = new_button_state;

		if (a__app_proctable.pointer)
			a__app_proctable.pointer(aAppTime(), 0, 0, btn_diff);
	}
	return 0;
}

void aAppGrabInput(int grab) {
	RAWINPUTDEVICE devices[] = {
		{1, 2, grab ? RIDEV_NOLEGACY /*| RIDEV_CAPTUREMOUSE*/ : RIDEV_REMOVE, grab ? g.hwnd : NULL}, /* mouse */
		/*{1, 6, RIDEV_NOLEGACY, NULL},*/ /* keyboard */
	};

	if (grab == a__app_state.grabbed)
		return;

	if (!RegisterRawInputDevices(devices, _countof(devices), sizeof(*devices))) {
		aAppDebugPrintf("Failed to register raw input devices");
		return;
	}

	g.rawMouse.resetAbsolute = 1;

	if (grab) {
		RECT r;
		GetClientRect(g.hwnd, &r);
		ClipCursor(&r);
		ShowCursor(FALSE);
	} else {
		ClipCursor(NULL);
		ShowCursor(TRUE);
	}

	a__app_state.grabbed = grab;
}
