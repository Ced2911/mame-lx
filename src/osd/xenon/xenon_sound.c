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

void osd_xenon_sound_init(){
    xenon_sound_init();
}

void osd_xenon_update_sound(const INT16 *buffer, int samples_this_frame){
    xenon_sound_submit((void*)buffer,samples_this_frame);
}