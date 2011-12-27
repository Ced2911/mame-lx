#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <debug.h>
#include <stdarg.h>

#include <usb/usbmain.h>
#include <console/console.h>
#include <usb/usbmain.h>
#include <xenon_soc/xenon_power.h>
#include <xenos/xenos.h>
#include <debug.h>

#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdmini.h"

//minimain.c

int xenon_main(){
	TR;
	// init xenon stuff	
	xenos_init(VIDEO_MODE_AUTO);
	//console_init();
	usb_init();
	usb_do_poll();
	xenon_make_it_faster(XENON_SPEED_FULL);
	TR;
	return 0;
}

int getrusage(int who, struct rusage *r_usage) {
    return 0;
}
/*
void mame_printf_verbose(const char *text, ...){
    TR;
    char buffer[256];
    va_list args;
    va_start (args, text);
    vsprintf (buffer,text, args);
    printf(buffer);
    va_end (args);
}
*/