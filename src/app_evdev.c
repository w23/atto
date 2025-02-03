#include "atto/app.h"

#include <linux/input.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/syscall.h> /* SYS_ */
#include <unistd.h> /* syscall() */
#include <string.h> /* strcmp() */
#include <stddef.h> /* offsetof() */

#define ATTO_EVDEV_MAX_DEVICES 16
#define ATTO_EVDEV_DEVICE_MAX_NAME 16

#ifndef ATTO_PRINT
	#include <stdio.h> /* printf */
	#define STR_(a) #a
	#define STR(a) STR_(a)
	#define ATTO_PRINT(fmt, ...) fprintf(stderr, __FILE__ ":" STR(__LINE__) ": " fmt "\n", __VA_ARGS__)
#endif
#ifndef ATTO_ASSERT
	#include <stdlib.h> /* abort() */
	#define ATTO_ASSERT(cond) \
		if (!(cond)) { \
			ATTO_PRINT("%s", "ASSERT(" #cond ") failed"); \
			abort(); \
		}
#endif

struct A__EvdevDevice {
	char name[ATTO_EVDEV_DEVICE_MAX_NAME];
	int fd;
};

static struct {
	struct AAppState *state;
	struct AAppProctable *proc;
	struct A__EvdevDevice devices[ATTO_EVDEV_MAX_DEVICES];
} a__evdev;

struct linux_dirent {
	unsigned long d_ino;
	unsigned long d_off;
	unsigned short d_reclen;
	char d_name[];
	/*
	char zero;
	char d_type;
	*/
};

static struct A__EvdevDevice *findSlotForDevice(const char *name) {
	struct A__EvdevDevice *slot = 0;
	for (int i = 0; i < ATTO_EVDEV_MAX_DEVICES; ++i) {
		struct A__EvdevDevice *const dev = a__evdev.devices + i;

		// If the device is already in the table, return it
		if (strcmp(dev->name, name) == 0)
			return dev;

		// If slot is not empty, continue
		if (dev->fd > 0)
			continue;

		// Only set the first empty slot
		if (!slot)
			slot = dev;

		// Continue looking for slot with the same device name
	} /* for max devices */
	return slot;
}

static void a__EvdevScan(void) {
	char buffer[8192];
	const int evdir = open("/dev/input", O_RDONLY);
	ATTO_ASSERT(evdir > 0);
	const long bytes = syscall(SYS_getdents, evdir, buffer, sizeof buffer);
	close(evdir);

	for (long i = 0; i + (long)sizeof(struct linux_dirent) < bytes;) {
		const struct linux_dirent *dent = (void *)(buffer + i);
		const long length = dent->d_reclen - 2 - offsetof(struct linux_dirent, d_name);
		if (i + length > bytes)
			break;
		const char d_type = dent->d_name[length + 1];
		/*
		ATTO_PRINT("@%ld d_ino=%lu d_off=%lu d_reclen=%d d_name=%s d_type=%d",
				i, dent->d_ino, dent->d_off, dent->d_reclen, dent->d_name, d_type);
		*/
		i += dent->d_reclen;

		if (strncmp(dent->d_name, "event", 5))
			continue;

		/* ATTO_PRINT("entry=%s type=%d", dent->d_name, d_type); */

		if (length > ATTO_EVDEV_DEVICE_MAX_NAME - 1) {
			ATTO_PRINT("Warning: device name is too long (%ld >= %d), skipping", length, ATTO_EVDEV_DEVICE_MAX_NAME);
			continue;
		}

		// Skip non-char devices
		if (d_type != DT_CHR)
			continue;

		struct A__EvdevDevice *const slot = findSlotForDevice(dent->d_name);

		if (!slot) {
			ATTO_PRINT("Unable to add new evdev device %s: Exceeded max devices %d",
				dent->d_name, ATTO_EVDEV_MAX_DEVICES);
			continue;
		}

		// Skip already opened devices
		if (slot->fd > 0)
			continue;

		char device_name[12 + ATTO_EVDEV_DEVICE_MAX_NAME] = "/dev/input/";
		strcpy(device_name + 11, dent->d_name);
		slot->fd = open(device_name, O_RDONLY | O_NONBLOCK);

		if (slot->fd < 0) {
			ATTO_PRINT("Failed to open device \"%s\"", device_name);
		} else {
			ioctl(slot->fd, EVIOCGRAB, 1);
			strcpy(slot->name, dent->d_name);
			ATTO_PRINT("Device %s opened as fd=%d", slot->name, slot->fd);
		}
	} /* for all dirents */
}

void a__EvdevInit(struct AAppState *state, struct AAppProctable *proc) {
	a__evdev.state = state;
	a__evdev.proc = proc;
	for (int i = 0; i < ATTO_EVDEV_MAX_DEVICES; ++i) {
		a__evdev.devices[i].name[0] = '\0';
		a__evdev.devices[i].fd = -1;
	}
	a__EvdevScan();
}

static AKey a__evdevKey(int code) {
	switch (code) {
	case KEY_BACKSPACE: return AK_Backspace;
	case KEY_TAB: return AK_Tab;
	case KEY_ENTER: return AK_Enter;
	case KEY_SPACE: return AK_Space;
	case KEY_ESC: return AK_Esc;
	case KEY_PAGEUP: return AK_PageUp;
	case KEY_PAGEDOWN: return AK_PageDown;
	case KEY_LEFT: return AK_Left;
	case KEY_UP: return AK_Up;
	case KEY_RIGHT: return AK_Right;
	case KEY_DOWN: return AK_Down;
	case KEY_COMMA: return AK_Comma;
	case KEY_MINUS: return AK_Minus;
	case KEY_DOT: return AK_Dot;
	case KEY_SLASH: return AK_Slash;
	case KEY_0: return AK_0;
	case KEY_1: return AK_1;
	case KEY_2: return AK_2;
	case KEY_3: return AK_3;
	case KEY_4: return AK_4;
	case KEY_5: return AK_5;
	case KEY_6: return AK_6;
	case KEY_7: return AK_7;
	case KEY_8: return AK_8;
	case KEY_9: return AK_9;
	case KEY_EQUAL: return AK_Equal;
	case KEY_A: return AK_A;
	case KEY_B: return AK_B;
	case KEY_C: return AK_C;
	case KEY_D: return AK_D;
	case KEY_E: return AK_E;
	case KEY_F: return AK_F;
	case KEY_G: return AK_G;
	case KEY_H: return AK_H;
	case KEY_I: return AK_I;
	case KEY_J: return AK_J;
	case KEY_K: return AK_K;
	case KEY_L: return AK_L;
	case KEY_M: return AK_M;
	case KEY_N: return AK_N;
	case KEY_O: return AK_O;
	case KEY_P: return AK_P;
	case KEY_Q: return AK_Q;
	case KEY_R: return AK_R;
	case KEY_S: return AK_S;
	case KEY_T: return AK_T;
	case KEY_U: return AK_U;
	case KEY_V: return AK_V;
	case KEY_W: return AK_W;
	case KEY_X: return AK_X;
	case KEY_Y: return AK_Y;
	case KEY_Z: return AK_Z;
	case KEY_APOSTROPHE: return AK_Tilda;
	case KEY_DELETE: return AK_Del;
	case KEY_INSERT: return AK_Ins;
	case KEY_LEFTALT: return AK_LeftAlt;
	case KEY_LEFTCTRL: return AK_LeftCtrl;
	case KEY_LEFTMETA: return AK_LeftMeta;
	case KEY_LEFTSHIFT: return AK_LeftShift;
	case KEY_RIGHTALT: return AK_RightAlt;
	case KEY_RIGHTCTRL: return AK_RightCtrl;
	case KEY_RIGHTMETA: return AK_RightMeta;
	case KEY_RIGHTSHIFT: return AK_RightShift;
	case KEY_F1: return AK_F1;
	case KEY_F2: return AK_F2;
	case KEY_F3: return AK_F3;
	case KEY_F4: return AK_F4;
	case KEY_F5: return AK_F5;
	case KEY_F6: return AK_F6;
	case KEY_F7: return AK_F7;
	case KEY_F8: return AK_F8;
	case KEY_F9: return AK_F9;
	case KEY_F10: return AK_F10;
	case KEY_F11: return AK_F11;
	case KEY_F12: return AK_F12;
	case KEY_KPASTERISK: return AK_KeypadAsterisk;
	case KEY_KPPLUS: return AK_KeypadPlus;
	case KEY_KPMINUS: return AK_KeypadMinus;
	case KEY_HOME: return AK_Home;
	case KEY_END: return AK_End;
	case KEY_CAPSLOCK: return AK_Capslock;
	}
	return AK_Unknown;
}

static void a__EvdevRead(int fd) {
	for (;;) {
		ATimeUs ts = 0;
		struct input_event event;
		const ssize_t rd = read(fd, &event, sizeof event);
		if (rd < (ssize_t)sizeof event)
			break;

		/*
		ATTO_PRINT("%ld.%ld %d %d %d", event.time.tv_sec, event.time.tv_usec,
				event.type, event.code, event.value);
		*/

		ts = event.time.tv_usec + event.time.tv_sec * 1000000ull;

		// FIXME care for input device type

		// FIXME not correct, this can be something else, not gamepad
		if (event.type == EV_ABS) {
			switch (event.code) {
				case ABS_X:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Stick0X, event.value);
					break;
				case ABS_Y:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Stick0Y, event.value);
					break;
				case ABS_Z:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Stick1X, event.value);
					break;
				case ABS_RZ:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Stick1Y, event.value);
					break;
				case ABS_HAT0X:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Pad0X, event.value);
					break;
				case ABS_HAT0Y:
					if (a__evdev.proc->gamepad)
						a__evdev.proc->gamepad(ts, AG_Pad0Y, event.value);
					break;
			}
		}

		if (event.type == EV_KEY) {
			int button = 0;
			if (event.code == BTN_LEFT)
				button = AB_Left;
			else if (event.code == BTN_RIGHT)
				button = AB_Right;
			else if (event.code == BTN_MIDDLE)
				button = AB_Middle;

			switch (event.code) {
				case BTN_A:
					if (a__evdev.proc->gamepad) a__evdev.proc->gamepad(ts, AG_ButtonA, event.value);
					break;
				case BTN_B:
					if (a__evdev.proc->gamepad) a__evdev.proc->gamepad(ts, AG_ButtonB, event.value);
					break;
				case BTN_X:
					if (a__evdev.proc->gamepad) a__evdev.proc->gamepad(ts, AG_ButtonX, event.value);
					break;
				case BTN_Y:
					if (a__evdev.proc->gamepad) a__evdev.proc->gamepad(ts, AG_ButtonY, event.value);
					break;
			}

			if (button) {
				if (event.value)
					a__evdev.state->pointer.buttons |= button;
				else
					a__evdev.state->pointer.buttons &= ~button;
				a__evdev.proc->pointer(ts, 0, 0, button);
				button = 0;
			} else {
				const AKey key = a__evdevKey(event.code);

				if (key != AK_Unknown) {
					const int down = !!event.value;
					if (a__evdev.state->keys[key] != down) {
						a__evdev.state->keys[key] = down;
						a__evdev.proc->key(ts, key, down);
					}
				}
			}
		} else if (event.type == EV_REL) {
			if (event.code == REL_X)
				a__evdev.proc->pointer(ts, event.value, 0, 0);
			else if (event.code == REL_Y)
				a__evdev.proc->pointer(ts, 0, event.value, 0);
		}
	} /* for all events */
}

void a__EvdevProcess(void) {
	// Scan evdev devices every 5 seconds
	{
		static ATimeUs last_scan = 0;
		const ATimeUs now = aAppTime();
		const ATimeUs delta = now - last_scan;
		if (last_scan == 0 || delta > 5000000) {
			ATTO_PRINT("evdev scan %ums", delta/1000);
			a__EvdevScan();
			last_scan = now;
		}
	}

	struct pollfd fds[ATTO_EVDEV_MAX_DEVICES];
	int nfds = 0;
	for (int i = 0; i < ATTO_EVDEV_MAX_DEVICES; ++i)
		if (a__evdev.devices[i].fd >= 0) {
			fds[nfds].fd = a__evdev.devices[i].fd;
			fds[nfds].events = POLLIN;
			++nfds;
		}

	const int events = poll(fds, nfds, 0);
	if (events == 0)
		return;
	if (events < 0) {
		if (errno != EINTR)
			ATTO_PRINT("poll error: %d", errno);
		return;
	}

	for (int i = 0; i < nfds; ++i) {
		if (!fds[i].revents)
			continue;
		if (fds[i].revents & POLLIN) {
			a__EvdevRead(fds[i].fd);
		} else { /* if fd was readable */
			ATTO_PRINT("fd %d got revents=%x, closing", fds[i].fd, fds[i].revents);

			for (int idev = 0; idev < ATTO_EVDEV_MAX_DEVICES; ++idev)
				if (a__evdev.devices[idev].fd == fds[i].fd) {
					close(a__evdev.devices[idev].fd);
					a__evdev.devices[idev].fd = -1;
					break;
				}
		}
	} /* for all fds */
}

void a__EvdevClose(void) {
	for (int i = 0; i < ATTO_EVDEV_MAX_DEVICES; ++i)
		if (a__evdev.devices[i].fd >= 0) {
			close(a__evdev.devices[i].fd);
			a__evdev.devices[i].fd = -1;
		}
}
