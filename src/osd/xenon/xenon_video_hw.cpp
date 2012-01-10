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

static void draw_quad(render_primitive *prim) {
    void * texture = (prim->texture.base);
/*
    switch (PRIMFLAG_GET_BLENDMODE(prim->flags)) {
        case BLENDMODE_NONE:
            Xe_SetAlphaTestEnable(g_pVideoDevice,0);
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_INVSRCALPHA);
            break;
        case BLENDMODE_ALPHA:
            Xe_SetAlphaTestEnable(g_pVideoDevice,1);
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_INVSRCALPHA);
            break;
        case BLENDMODE_RGB_MULTIPLY:
            Xe_SetAlphaTestEnable(g_pVideoDevice,1);
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_DESTCOLOR);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_ZERO);
            break;
        case BLENDMODE_ADD:
            Xe_SetAlphaTestEnable(g_pVideoDevice,1);
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_ONE);
            break;
    }
*/
    
    float x=prim->bounds.x0;
    float y=prim->bounds.y0;
    
    float w=prim->bounds.x1-x;
    float h=prim->bounds.y1-y;
    
    XeColor color;
    color.r = (prim->color.r * 255.0f);
    color.g = (prim->color.g * 255.0f);
    color.b = (prim->color.b * 255.0f);
    color.a = (prim->color.a * 255.0f);
    
    Menu_DrawRectangle(x,y,w,h,color,1);
/*   
    DrawVerticeFormats* vertices = (DrawVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        vertices[0].x = prim->bounds.x0 - 0.5f;
        vertices[0].y = prim->bounds.y0 - 0.5f;
        vertices[1].x = prim->bounds.x1 - 0.5f;
        vertices[1].y = prim->bounds.y0 - 0.5f;
        vertices[2].x = prim->bounds.x0 - 0.5f;
        vertices[2].y = prim->bounds.y1 - 0.5f;
        vertices[3].x = prim->bounds.x1 - 0.5f;
        vertices[3].y = prim->bounds.y1 - 0.5f;

        // set the texture coordinates
        if (texture != NULL) {
            vertices[0].u = prim->texcoords.tl.u;
            vertices[0].v = prim->texcoords.tl.v;
            vertices[1].u = prim->texcoords.tr.u;
            vertices[1].v = prim->texcoords.tr.v;
            vertices[2].u = prim->texcoords.bl.u;
            vertices[2].v = prim->texcoords.bl.v;
            vertices[3].u = prim->texcoords.br.u;
            vertices[3].v = prim->texcoords.br.v;
            n++;
        }

        XeColor color;
        color.r = (prim->color.r * 255.0f);
        color.g = (prim->color.g * 255.0f);
        color.b = (prim->color.b * 255.0f);
        color.a = (prim->color.a * 255.0f);
        
        color.a = 0xFF;
        color.b = 0x7F;

        int i = 0;
        for (i = 0; i < 3; i++) {
            vertices[i].x =  (vertices[i].x)/1280.f;
            vertices[i].y =  (vertices[i].y)/720.f;
            vertices[i].z = 0.0;
            vertices[i].w = 1.0;
            vertices[i].color = color.lcol;
        }
    }
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

void osd_xenon_update_video(render_primitive_list &primlist) {
    primlist.acquire_lock();
    
    pre_render();
    
    int minwidth, minheight;
    
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    // make that the size of our target
    xenos_target->set_bounds(fb->width, fb->height);

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
    
    primlist.release_lock();
}
