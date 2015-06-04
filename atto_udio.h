#ifndef ATTO_UDIO_H__DECLARED
#define ATTO_UDIO_H__DECLARED

/*
 * 44100
 * 2 channels interleaved
 */

typedef void (*a_udio_callback_f)(void *opaque, unsigned int sample, float *samples, unsigned int nsamples);

int aUdioStart(a_udio_callback_f callback, void *opaque);
unsigned int aUdioPosition();
void aUdioStop();

#endif /* ifndef ATTO_UDIO_H__DECLARED */

#ifdef ATTO_UDIO_H_IMPLEMENT

#ifndef ATTO_PLATFORM
#error ATTO_PLATFORM must be already defined
#endif

#ifdef ATTO_UDIO_H__IMPLEMENTED
#error atto_udio.h can be implemented only once
#endif /* ifdef ATTO_UDIO_H__IMPLEMENTED */
#define ATTO_UDIO_H__IMPLEMENTED

#ifdef ATTO_PLATFORM_LINUX
/* Use pulseaudio simple api */
#include <pthread.h>
/* pulseaudio, y u no c89?! */
#define inline
#include <pulse/simple.h>
#undef inline

static pthread_mutex_t a__udio_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t a__udio_thread;
static void *a__udio_opaque = 0;
static a_udio_callback_f a__udio_callback = 0;
static unsigned int a__udio_sample = 0;

static void *a__udio_thread_main(void *arg) {
	pa_simple *s;
	pa_sample_spec ss;

	pthread_mutex_lock(&a__udio_mutex);
	ss.format = PA_SAMPLE_FLOAT32NE;
	ss.channels = 2;
	ss.rate = 44100;
	s = pa_simple_new(
		NULL, ATTO_APP_NAME, PA_STREAM_PLAYBACK,
		NULL, "m", &ss, NULL, NULL, NULL);

	for(;;) {
		float buffer[1024];

		if (a__udio_callback == 0) {
			pthread_mutex_unlock(&a__udio_mutex);
			break;
		}
		a__udio_callback(a__udio_opaque, a__udio_sample, buffer, 512);
		pthread_mutex_unlock(&a__udio_mutex);

		pa_simple_write(s, buffer, sizeof(buffer), NULL);

		pthread_mutex_lock(&a__udio_mutex);
		a__udio_sample += 512;
	}

	pa_simple_flush(s, NULL);
	pa_simple_free(s);

	return 0;
}

int aUdioStart(a_udio_callback_f callback, void *opaque) {
	int result;

	pthread_mutex_lock(&a__udio_mutex);
	if (a__udio_callback != 0)
		return -1;

	a__udio_sample = 0;
	result = pthread_create(&a__udio_thread, NULL, a__udio_thread_main, 0);
	if (result != 0)
		goto exit;

	a__udio_callback = callback;
	a__udio_opaque = opaque;

exit:
	pthread_mutex_unlock(&a__udio_mutex);
	return result;
}

unsigned int aUdioPosition() {
	unsigned int ret;
	pthread_mutex_lock(&a__udio_mutex);
	ret = a__udio_sample;
	pthread_mutex_unlock(&a__udio_mutex);
	return ret;
}

void aUdioStop() {
	pthread_mutex_lock(&a__udio_mutex);

	if (a__udio_callback == 0) {
		pthread_mutex_unlock(&a__udio_mutex);
		return;
	}

	a__udio_callback = 0;
	pthread_mutex_unlock(&a__udio_mutex);

	pthread_join(a__udio_thread, NULL);
}

#endif

#endif /* ifdef ATTO_UDIO_H_IMPLEMENT */
