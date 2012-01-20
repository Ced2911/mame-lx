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
#include <time/time.h>
#include <ppc/atomic.h>
#include <newlib/malloc_lock.h>

#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdxenon.h"

#include "xenon_video_hw.h"
#include "gui/video.h"
#include "gui/video_mame.h"

render_target *xenos_target;

static unsigned int thread_lock __attribute__((aligned(128))) = 0;
extern struct XenosDevice * g_pVideoDevice;

static int screen_width;
static int screen_height;

static int nb_vertices = 0;
int n = 0;

static void pre_render() {

}

static void render() {
    Menu_Render();
}

extern void MameFrame();


static int video_thread_running = 1;
static render_primitive_list * currList;

static void osd_xenon_video_thread() {
    video_thread_running = 1;
    while (video_thread_running) {

        if (currList == NULL) {
            udelay(10);
            continue;
        } else {
            pre_render();

            int minwidth, minheight;
            int newwidth, newheight;

            XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

            // make that the size of our target
            xenos_target->set_bounds(fb->width, fb->height);

            xenos_target->compute_visible_area(fb->width, fb->height, (float) fb->width / (float) fb->height, xenos_target->orientation(), newwidth, newheight);

            n = 0;
            // begin ...
            currList->acquire_lock();
            // tmp
            lock(&thread_lock);
            
            render_primitive *prim;
            for (prim = currList->first(); prim != NULL; prim = prim->next()) {
                switch (prim->type) {
                    case render_primitive::LINE:
                        DrawLine(prim);
                        break;

                    case render_primitive::QUAD:
                        //draw_quad(prim);
                        DrawQuad(prim);
                        break;

                    default:
                        throw emu_fatalerror("Unexpected render_primitive type");
                }

                n++;
            }
            currList->release_lock();
            // tmp
            unlock(&thread_lock);

            //printf("Number of primitives :%d\r\n", n);

            render();


            /** Clean some buffers **/
            MameFrame();
        }
    }
}

#if 0

void osd_xenon_update_video(render_primitive_list &primlist) {
    primlist.acquire_lock();

    pre_render();

    int minwidth, minheight;
    int newwidth, newheight;

    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    // make that the size of our target
    xenos_target->set_bounds(fb->width, fb->height);

    xenos_target->compute_visible_area(fb->width, fb->height, (float) fb->width / (float) fb->height, xenos_target->orientation(), newwidth, newheight);

    n = 0;
    // begin ...
    render_primitive *prim;
    for (prim = primlist.first(); prim != NULL; prim = prim->next()) {
        switch (prim->type) {
            case render_primitive::LINE:
                DrawLine(prim);
                break;

            case render_primitive::QUAD:
                //draw_quad(prim);
                DrawQuad(prim);
                break;

            default:
                throw emu_fatalerror("Unexpected render_primitive type");
        }

        n++;
    }

    //printf("Number of primitives :%d\r\n", n);

    render();


    /** Clean some buffers **/
    MameFrame();

    primlist.release_lock();
}
#else

void osd_xenon_update_video(render_primitive_list &primlist) {
    currList = &primlist;
}
#endif

static void osd_xenon_video_cleanup(running_machine &machine) {
    video_thread_running = 0;
    render_primitive_list *primlist = currList;
    if (primlist) {
        // tmp
        lock(&thread_lock);
        
        primlist->acquire_lock();
        currList = NULL;
        primlist->release_lock();
        
        // tmp 
        unlock(&thread_lock);
    }
    
    // wait a few
    udelay(10);
}

static unsigned char thread_stack[0x10000];

void osd_xenon_video_hw_init(running_machine &machine) {
    TR;
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
    Xe_SetClearColor(g_pVideoDevice, 0);

    // run the video on a new thread
    xenon_run_thread_task(4, &thread_stack[sizeof (thread_stack) - 0x100], (void*) osd_xenon_video_thread);

    // on mame exit
    machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(osd_xenon_video_cleanup), &machine));
}