#ifndef ATTO_IO_H__DECLARED
#define ATTO_IO_H__DECLARED

int aIoFileRead(const char *filename, void *buffer, int size);
int aIoFileWrite(const char *filename, void *buffer, int size);

struct Aio_monitor_t;

struct Aio_monitor_t *aIoMonitorOpen(const char *filename);
int aIoMonitorCheck(struct Aio_monitor_t *monitor);
void aIoMonitorClose(struct Aio_monitor_t *monitor);

#endif /* ifndef ATTO_IO_H__DECLARED */

#ifdef ATTO_IO_H_IMPLEMENT

#ifdef ATTO_IO_H__IMPLEMENTED
#error atto_io.h can be implemented only once
#endif /* ifdef ATTO_IO_H__IMPLEMENTED */
#define ATTO_IO_H__IMPLEMENTED

#ifndef ATTO_PLATFORM
#error ATTO_PLATFORM is not defined
#endif

#ifndef ATTO_IO_MONITORS_MAX
#define ATTO_IO_MONITORS_MAX 16
#endif

#ifdef ATTO_PLATFORM_POSIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int aIoFileRead(const char *filename, void *buffer, int size) {
	int fd;
	ssize_t rd;

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		return 0;

	rd = read(fd, buffer, size);
	close(fd);
	return rd;
}

int aIoFileWrite(const char *filename, void *buffer, int size) {
	int fd;
	ssize_t rd;

	fd = open(filename, O_WRONLY | O_CREAT);
	if (fd < 0)
		return 0;

	rd = write(fd, buffer, size);
	close(fd);
	return rd;
}

#endif

#ifdef ATTO_PLATFORM_LINUX
#include <sys/inotify.h>
#include <limits.h>
static int a__io_inotifyfd = -1;

struct a__io_monitor_t {
	const char *filename;
	int watch;
	int event;
};

struct a__io_monitor_t a__io_monitors[ATTO_IO_MONITORS_MAX];

int aIoMonitorOpen(const char *filename) {
	int i;
	if (a__io_inotifyfd < 0) {
		memset(a__io_monitors, 0, sizeof(a__io_monitors));
		a__io_inotifyfd = inotify_init1(IN_NONBLOCK);
		if (a__io_inotifyfd < 0)
			return -1;
	}

	for (i = 0; i < ATTO_IO_MONITORS_MAX; ++i)
		if (a__io_monitors[i].filename == 0) {
			a__io_monitors[i].watch = -1;
			a__io_monitors[i].event = 0;
			a__io_monitors[i].filename = filename;
			return i;
		}
	return -1;
}

int aIoMonitorCheck(int monitor) {
	char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
	const struct inotify_event *e = (const struct inotify_event*)buffer;
	struct a__io_monitor_t *m = a__io_monitors + monitor;
	int retval;
	int i;
	if (m->watch == -1) {
		m->watch = inotify_add_watch(
			a__io_inotifyfd, m->filename,
			IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF);
		m->event = 0;
		return (m->watch == -1) ? 0 : 1;
	}

	for (;;) {
		ssize_t rd = read(a__io_inotifyfd, buffer, sizeof(buffer));
		if (rd == -1) {
			/* CHECK(errno == EAGAIN, "read inotify fd, errno != EAGAIN"); */
			break;
		}

		for (i = 0; i < ATTO_IO_MONITORS_MAX; ++i) {
			struct a__io_monitor_t *mm = a__io_monitors + i;
			if (mm->watch != e->wd)
				continue;

			mm->event = 1;

			if (e->mask & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF)) {
				if (e->mask & IN_MOVE_SELF)
					inotify_rm_watch(a__io_inotifyfd, mm->watch);
				mm->watch = -1;
				mm->event = 0;
			}
		} /* for all monitors */
	}

	retval = m->event;
	m->event = 0;
	return retval;
}

void aIoMonitorClose(int monitor) {
	struct a__io_monitor_t *m = a__io_monitors + monitor;

	if (m->watch != -1) inotify_rm_watch(a__io_inotifyfd, m->watch);
	m->filename = 0;
}
#elif defined(ATTO_PLATFORM_WINDOWS)
#include <shellapi.h>

int aIoFileRead(const char *filename, void *buffer, int size) {
	WCHAR *wfilename = utf8_to_wchar(filename, -1, NULL);
	HANDLE h = CreateFile(wfilename, GENERIC_READ, FILE_SHARE_READ,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (h == INVALID_HANDLE_VALUE) return 0;
	DWORD read;
	BOOL rr = ReadFile(h, buffer, size, &read, NULL);
	CloseHandle(h);
	free(wfilename);
	if (rr == FALSE || read < 1) return 0;
	return read;
}

//int aIoFileWrite(const char *filename, void *buffer, int size) {}

struct Aio_monitor_t {
	LONG sentinel;
	HANDLE dir;
	HANDLE thread;
	DWORD filename_size;
	WCHAR *filename;
	WCHAR dirname[1];
};

static DWORD WINAPI filemon_thread_main(struct Aio_monitor_t *mon) {
	DWORD buffer[16384];
	for (;;) {
		DWORD returned = 0;
		BOOL result = ReadDirectoryChangesW(mon->dir, &buffer, sizeof(buffer), FALSE,
			FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE,
			&returned, NULL, NULL);
		if (!result) break;
		const char *ptr = (const char*)buffer;
		while (returned >= sizeof(FILE_NOTIFY_INFORMATION)) {
			PFILE_NOTIFY_INFORMATION notify = (PFILE_NOTIFY_INFORMATION)ptr;
			if ((notify->FileNameLength == mon->filename_size)
				&& (0 == memcmp(notify->FileName, mon->filename, notify->FileNameLength)))
				InterlockedIncrement(&mon->sentinel);
			ptr += notify->NextEntryOffset;
			returned -= notify->NextEntryOffset;
			if (notify->NextEntryOffset == 0)
				break;
		}
	}
	free(mon);
	return 0;
}

struct Aio_monitor_t *aIoMonitorOpen(const char *filename) {
	struct Aio_monitor_t *mon = 0;
	WCHAR *wfilename = 0;
	WCHAR buffer[MAX_PATH + 1];
	WCHAR *filepart;
	wfilename = utf8_to_wchar(filename, -1, NULL);
	DWORD length = GetFullPathName(wfilename, sizeof(buffer) / sizeof(*buffer), buffer, &filepart);
	free(wfilename);
	if (length == 0 || length >= sizeof(buffer))
		return NULL;

	int filename_length = filepart - buffer;
	WCHAR filetmp = filepart[0];
	filepart[0] = 0;
	HANDLE dir = CreateFile(buffer,
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, 0);
	if (dir == INVALID_HANDLE_VALUE)
		return NULL;
	filepart[0] = filetmp;

	mon = (struct Aio_monitor_t*)malloc(sizeof(*mon) + sizeof(WCHAR) * length);
	mon->sentinel = 1;
	mon->dir = dir;
	memcpy(mon->dirname, buffer, sizeof(WCHAR) * (length + 1));
	mon->filename_size = sizeof(WCHAR) * (length - filename_length);
	mon->filename = mon->dirname + filename_length;

	mon->thread = CreateThread(NULL, 0, filemon_thread_main, mon, 0, NULL);
	ATTO__CHECK(mon->thread != NULL, "CreateThread");

	return mon;
}

int aIoMonitorCheck(struct Aio_monitor_t *monitor) {
	return 0 != InterlockedExchange(&monitor->sentinel, 0);
}

void aIoMonitorClose(struct Aio_monitor_t *monitor) {
	CloseHandle(monitor->thread);
	CloseHandle(monitor->dir);
}

#endif

#ifdef ATTO_PLATFORM_MACH
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>

struct a__io_monitor_t {
	const char *filename;
	int fd;
};

static int a__io_kq = -1;
struct a__io_monitor_t a__io_monitors[ATTO_IO_MONITORS_MAX];

int aIoMonitorOpen(const char *filename) {
	int i;
	if (a__io_kq < 0) {
		a__io_kq = kqueue();
		if (a__io_kq < 0)
			return -1;
	}
	
	for (i = 0; i < ATTO_IO_MONITORS_MAX; ++i)
		if (a__io_monitors[i].filename == 0) {
			a__io_monitors[i].fd = open(filename, O_EVTONLY);
			a__io_monitors[i].filename = filename;
			return i;
		}
	return -1;
}

//static int a__io_

int aIoMonitorCheck(int monitor) {
	int i;
	if (a__io_kq < 0) return -1;
	
	for (i = 0; i < ATTO_IO_MONITORS_MAX; ++i)
		if (a__io_monitors[i].filename == 0) {
		}
	return 0;
}

void aIoMonitorClose(int monitor) {
	if (monitor < 0 || monitor >= ATTO_IO_MONITORS_MAX) return;

	struct a__io_monitor_t *m = a__io_monitors + monitor;
	if (m->filename == 0) return;
	m->filename = 0;
	if (m->fd > 0)
		close(m->fd);
}

#endif /* ATTO_PLATFORM_MACH */

#endif /* ifdef ATTO_IO_H_IMPLEMENT */
