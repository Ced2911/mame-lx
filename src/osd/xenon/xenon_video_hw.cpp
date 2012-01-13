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

#include "xenon_video_hw.h"

#include "gui/video.h"

extern render_target *xenos_target;

#define XE_W 2048
#define XE_H 2048

//
//  SOFTWARE RENDER
//
#define FUNC_PREFIX(x)          draw32_##x
#define PIXEL_TYPE                      UINT32
#define SRCSHIFT_R                      0
#define SRCSHIFT_G                      0
#define SRCSHIFT_B                      0
#define DSTSHIFT_R                      16
#define DSTSHIFT_G                      8
#define DSTSHIFT_B                      0

#include "rendersw.c"

/**
 * Shaders
 **/
typedef unsigned int DWORD;
//#include "shaders/osd_ps.h"
//#include "shaders/osd_vs.h"

#include "shaders_hw/ps.c.h"
#include "shaders_hw/vs.h"

static struct XenosDevice _xe;
static struct XenosVertexBuffer *vb = NULL;
static struct XenosVertexBuffer *soft_vb = NULL;
struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;
static struct XenosSurface * g_pTexture = NULL;
static unsigned int * screen = NULL;

static int screen_width;
static int screen_height;
static int hofs;
static int vofs;

static uint32_t pitch = 0;

typedef struct DrawVerticeFormats {
    float x, y, z, w;
    unsigned int color;
    float u, v;
} DrawVerticeFormats;

DrawVerticeFormats *vertices;

struct mame_surface {
    XenosSurface * surf;
    mame_surface * next;
};

mame_surface * first_surf;

static int nb_vertices = 0;
int n = 0;

static void draw_line(render_primitive *prim) {
    /*
        nb_vertices += 256;
     */
}

void osd_xenon_video_init() {
    g_pVideoDevice = &_xe;
    Xe_Init(g_pVideoDevice);

    InitVideo();

    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    float w = fb->width;
    float h = fb->height;

    Xe_SetClearColor(g_pVideoDevice, 0);
}

static void pre_render() {

}

static void render() {
    Menu_Render();
}

extern void MameFrame();

void osd_xenon_update_video(render_primitive_list &primlist) {
    primlist.acquire_lock();

    pre_render();

    int minwidth, minheight;
    int newwidth, newheight;

    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    // make that the size of our target
    xenos_target->set_bounds(fb->width, fb->height);
    
    xenos_target->compute_visible_area(fb->width, fb->height, (float)fb->width / (float)fb->height, xenos_target->orientation(), newwidth, newheight);

    n = 0;
    // begin ...
    render_primitive *prim;
    for (prim = primlist.first(); prim != NULL; prim = prim->next()) {
        switch (prim->type) {
            case render_primitive::LINE:
                draw_line(prim);
                break;

            case render_primitive::QUAD:
                //draw_quad(prim);
                Menu_DrawMame(prim);
                break;

            default:
                throw emu_fatalerror("Unexpected render_primitive type");
        }

        n++;
    }

    //printf("Number of primitives :%d\r\n", n);

    render();


    MameFrame();

    primlist.release_lock();
}
