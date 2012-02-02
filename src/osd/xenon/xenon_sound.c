/**
 * Based on muppen for libxenon
 */
#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdxenon.h"
#include "xenon.h"
#include <debug.h>
#include <usb/usbmain.h>
#include <input/input.h>
#include <ppc/timebase.h>
#include <time/time.h>
#include <xenon_sound/sound.h>
#include <byteswap.h>
#include <xetypes.h>
#include <ppc/atomic.h>
#include <xenon_soc/xenon_power.h>

#undef realloc
extern "C" int xenos_is_hdmi;

static UINT32 stream_buffer_size;
static UINT32 stream_buffer_in;
static INT16 audio_buffer[48000];

#define BUFFER_SIZE 65536
static char buffer[BUFFER_SIZE];
static unsigned int freq;
static unsigned int real_freq;
static double freq_ratio=1;
static int is_60Hz;
// NOTE: 32khz actually uses ~2136 bytes/frame @ 60hz
/*
static enum {
    BUFFER_SIZE_32_60 = 2112, BUFFER_SIZE_48_60 = 3200,
    BUFFER_SIZE_32_50 = 2560, BUFFER_SIZE_48_50 = 3840
} buffer_size = 1024;
 */

unsigned int buffer_size = 1024;


static unsigned char thread_stack[0x10000];

static unsigned int thread_lock __attribute__((aligned(128))) = 0;
static volatile void * thread_buffer = NULL;
static volatile int thread_bufsize = 0;
static int thread_bufmaxsize = 0;
static volatile int thread_terminate = 0;

static void inline play_buffer(void) {
    int i;
    for (i = 0; i < buffer_size / 4; ++i)
        ((int*) buffer)[i] = bswap_32(((int*) buffer)[i]);

    xenon_sound_submit(buffer, buffer_size);
}

static void inline copy_to_buffer(int* buffer, int* stream, unsigned int length, unsigned int stream_length) {
    //	printf("c2b %p %p %d %d\n",buffer,stream,length,stream_length);
    // NOTE: length is in samples (stereo (2) shorts)
    int di;
    double si;
    for (di = 0, si = 0.0f; di < length; ++di, si += freq_ratio) {
#if 1
        // Linear interpolation between current and next sample
        double t = si - floor(si);
        short* osample = (short*) (buffer + di);
        short* isample1 = (short*) (stream + (int) si);
        short* isample2 = (short*) (stream + (int) ceil(si));

        // Left and right
        osample[0] = (1.0f - t) * isample1[0] + t * isample2[0];
        osample[1] = (1.0f - t) * isample1[1] + t * isample2[1];
#else
        // Quick and dirty resampling: skip over or repeat samples
        buffer[di] = stream[(int) si];
#endif
    }
}

static s16 prevLastSample[2] = {0, 0};
// resamples pStereoSamples (taken from http://pcsx2.googlecode.com/svn/trunk/plugins/zerospu2/zerospu2.cpp)

void ResampleLinear(s16* pStereoSamples, s32 oldsamples, s16* pNewSamples, s32 newsamples) {
    s32 newsampL, newsampR;
    s32 i;

    for (i = 0; i < newsamples; ++i) {
        s32 io = i * oldsamples;
        s32 old = io / newsamples;
        s32 rem = io - old * newsamples;

        old *= 2;
        //printf("%d %d\n",old,oldsamples);
        if (old == 0) {
            newsampL = prevLastSample[0] * (newsamples - rem) + pStereoSamples[0] * rem;
            newsampR = prevLastSample[1] * (newsamples - rem) + pStereoSamples[1] * rem;
        } else {
            newsampL = pStereoSamples[old - 2] * (newsamples - rem) + pStereoSamples[old] * rem;
            newsampR = pStereoSamples[old - 1] * (newsamples - rem) + pStereoSamples[old + 1] * rem;
        }
        pNewSamples[2 * i] = newsampL / newsamples;
        pNewSamples[2 * i + 1] = newsampR / newsamples;
    }

    prevLastSample[0] = pStereoSamples[oldsamples * 2 - 2];
    prevLastSample[1] = pStereoSamples[oldsamples * 2 - 1];
}

static inline void add_to_buffer(void* stream, unsigned int length) {
    unsigned int lengthLeft = length >> 2;
    unsigned int rlengthLeft = ceil(lengthLeft / freq_ratio);

    //copy_to_buffer((int *)buffer, stream , rlengthLeft, lengthLeft);
    ResampleLinear((s16 *) stream, lengthLeft, (s16 *) buffer, rlengthLeft);
    buffer_size = rlengthLeft << 2;
    play_buffer();
}

static void thread_enqueue(void * buffer, int size) {
    while (thread_bufsize);

    lock(&thread_lock);

    if (thread_bufmaxsize < size) {
        thread_bufmaxsize = size;
        thread_buffer = realloc((void*) thread_buffer, thread_bufmaxsize);
    }

    thread_bufsize = size;
    memcpy((void*) thread_buffer, buffer, thread_bufsize);

    unlock(&thread_lock);
}

static void thread_loop() {
    static char * local_buffer[0x10000];
    int local_bufsize = 0;
    int k;

    while (!thread_terminate) {
        lock(&thread_lock);

        if (thread_bufsize) {
            local_bufsize = thread_bufsize;
            if (local_bufsize>sizeof (local_buffer)) local_bufsize = sizeof (local_buffer);
            memcpy(local_buffer, (void*) thread_buffer, local_bufsize);
            thread_bufsize -= local_bufsize;
        }

        unlock(&thread_lock);

        if (local_bufsize) {
            //			printf("add_to_buffer %d\n",local_bufsize);
            add_to_buffer(local_buffer, local_bufsize);
            local_bufsize = 0;
        }

        for (k = 0; k < 100; ++k)
            asm volatile("nop");
    }
}

#define MIN_RATIO 2
#define MAX_RATIO 4

static void run_xenon_sound_thread() {
    xenon_run_thread_task(2, &thread_stack[sizeof (thread_stack) - 0x100], (void*)thread_loop);
}

void osd_xenon_sound_init() {

    printf("xenos_is_hdmi = %d\r\n", xenos_is_hdmi);

    xenon_sound_init();
    static int first_run = 1;
    // start sound thread
    if(first_run)
        run_xenon_sound_thread();
    first_run=0;
}

void osd_xenon_update_sound(const INT16 *buffer, int samples_this_frame) {
    for (int i = 0; i < samples_this_frame; i++) {
        audio_buffer[i] = bswap_16(buffer[i]);
    }
    xenon_sound_submit(audio_buffer, samples_this_frame * 4);
/*
    thread_enqueue((void*)buffer, samples_this_frame * 4);
    add_to_buffer((void*)buffer, samples_this_frame * 4);
*/
}