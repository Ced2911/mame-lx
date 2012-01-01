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
static UINT32 stream_buffer_size;
static UINT32 stream_buffer_in;
static INT16 audio_buffer[48000];

static void run_xenon_sound_thread() {
}

void osd_xenon_sound_init() {
    xenon_sound_init();
    // start sound thread
    run_xenon_sound_thread();
}

static int f = 0;

void osd_xenon_update_sound(const INT16 *buffer, int samples_this_frame) {
    int bytes_this_frame = samples_this_frame * sizeof (INT16) * 2;

    memcpy(audio_buffer, buffer, bytes_this_frame);
/*
    for (int i = 0; i < samples_this_frame; i++) {
        audio_buffer[i] = buffer[i];
    }
*/
    xenon_sound_submit(audio_buffer, samples_this_frame);
}