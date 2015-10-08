#ifndef ATTO_PLATFORM_H__DECLARED
#define ATTO_PLATFORM_H__DECLARED

#ifdef __linux__
#define ATTO_PLATFORM_POSIX
#define ATTO_PLATFORM_LINUX
#ifndef ATTO_PLATFORM_RPI
#define ATTO_PLATFORM_X11
#endif /* ifndef ATTO_PLATFORM_RPI */
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

#endif /* ifndef ATTO_PLATFORM_H__DECLARED */
