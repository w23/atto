#ifndef ATTO_IO_H__DECLARED
#define ATTO_IO_H__DECLARED

int aIoFileRead(const char *filename, void *buffer, int size);

int aIoMonitorOpen(const char *filename);
int aIoMonitorCheck(int monitor);
void aIoMonitorClose(int monitor);

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
	}
	if (a__io_inotifyfd < 0)
		return -1;

	for (i = 0; i < ATTO_IO_MONITORS_MAX; ++i)
		if (a__io_monitors[i].filename == 0) {
			a__io_monitors[i].watch = inotify_add_watch(
					a__io_inotifyfd, filename,
					IN_MODIFY | IN_DELETE_SELF | IN_MOVE_SELF);
			a__io_monitors[i].event = (a__io_monitors[i].watch < 0) ? 0 : 1;
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

			if (e->mask & (IN_IGNORED | IN_DELETE_SELF | IN_MOVE_SELF)) {
				if (e->mask & IN_MOVE_SELF) inotify_rm_watch(a__io_inotifyfd, mm->watch);
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
#endif

#endif /* ifdef ATTO_IO_H_IMPLEMENT */
