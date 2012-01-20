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
#include <xenos/xe.h>
#include <xenos/edram.h>
#include <debug.h>

#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdxenon.h"

#include "xenon.h"

#include <setjmp.h>

int osd_xenon_init() {
    static int firstime =1 ;
    if(firstime){
    TR;
    //
    // init xenon stuff	
    xenon_make_it_faster(XENON_SPEED_FULL);
    xenos_init(VIDEO_MODE_HDMI_720P);
    //console_init();
    usb_init();
    usb_do_poll();
    }
    // init sound
    osd_xenon_sound_init();
    firstime=0;
    TR;
    return 0;
}

int xmon_setjmp(jmp_buf buf) /* NOTE: assert(sizeof(buf) > 184) */ {
    /* XXX should save fp regs as well */
    asm volatile (
            "mflr 0; std 0,0(%0)\n\
         std    1,8(%0)\n\
         std    2,16(%0)\n\
         mfcr 0; std 0,24(%0)\n\
         std    13,32(%0)\n\
         std    14,40(%0)\n\
         std    15,48(%0)\n\
         std    16,56(%0)\n\
         std    17,64(%0)\n\
         std    18,72(%0)\n\
         std    19,80(%0)\n\
         std    20,88(%0)\n\
         std    21,96(%0)\n\
         std    22,104(%0)\n\
         std    23,112(%0)\n\
         std    24,120(%0)\n\
         std    25,128(%0)\n\
         std    26,136(%0)\n\
         std    27,144(%0)\n\
         std    28,152(%0)\n\
         std    29,160(%0)\n\
         std    30,168(%0)\n\
         std    31,176(%0)\n\
         " : : "r" (buf));
    return 0;
}

void xmon_longjmp(jmp_buf buf, int val) {
    if (val == 0)
        val = 1;
    asm volatile (
            "ld     13,32(%0)\n\
         ld     14,40(%0)\n\
         ld     15,48(%0)\n\
         ld     16,56(%0)\n\
         ld     17,64(%0)\n\
         ld     18,72(%0)\n\
         ld     19,80(%0)\n\
         ld     20,88(%0)\n\
         ld     21,96(%0)\n\
         ld     22,104(%0)\n\
         ld     23,112(%0)\n\
         ld     24,120(%0)\n\
         ld     25,128(%0)\n\
         ld     26,136(%0)\n\
         ld     27,144(%0)\n\
         ld     28,152(%0)\n\
         ld     29,160(%0)\n\
         ld     30,168(%0)\n\
         ld     31,176(%0)\n\
         ld     0,24(%0)\n\
         mtcrf  0x38,0\n\
         ld     0,0(%0)\n\
         ld     1,8(%0)\n\
         ld     2,16(%0)\n\
         mtlr   0\n\
         mr     3,%1\n\
         " : : "r" (buf), "r" (val));
}





