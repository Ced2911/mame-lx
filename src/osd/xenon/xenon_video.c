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
#include "ps.h"
#include "vs.h"

static struct XenosDevice _xe;
static struct XenosVertexBuffer *vb = NULL;
static struct XenosVertexBuffer *soft_vb = NULL;
static struct XenosDevice * g_pVideoDevice = NULL;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;
static struct XenosSurface * g_pTexture = NULL;
static unsigned int * screen = NULL;

static int screen_width;
static int screen_height;
static int hofs;
static int vofs;

typedef union {

    struct {
        unsigned char a;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
    unsigned int lcol;
} XeColor;

static uint32_t pitch = 0;

typedef struct DrawVerticeFormats {
    float x, y, z, w;
    unsigned int color;
    float u, v;
} DrawVerticeFormats;


struct mame_surface
{
    XenosSurface * surf;
    mame_surface * next;
};

mame_surface * first_surf;

static int nb_vertices = 0;

int n = 0;

enum {
    UvBottom = 0,
    UvTop,
    UvLeft,
    UvRight
};
float ScreenUv[4] = {0.f, 1.0f, 1.0f, 0.f};

#if 0

static void draw_line(render_primitive *prim) {
    /*
        nb_vertices += 256;
     */
}

static void draw_quad(render_primitive *prim) {
    void * texture = (prim->texture.base);
    
    //void Xe_SetBlendControl(struct XenosDevice *xe, int col_src, int col_op, int col_dst, int alpha_src, int alpha_op, int alpha_dst);
    switch (PRIMFLAG_GET_BLENDMODE(prim->flags)) {
        case BLENDMODE_NONE:
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_MIN);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_ONE);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_ZERO);
            //GX_SetBlendMode(GX_BM_NONE, GX_BL_ONE, GX_BL_ZERO, GX_LO_CLEAR);
            break;
        case BLENDMODE_ALPHA:
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_INVSRCALPHA);
            //GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
            break;
        case BLENDMODE_RGB_MULTIPLY:
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_SUBTRACT);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCCOLOR);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_ZERO);
            //GX_SetBlendMode(GX_BM_SUBTRACT, GX_BL_SRCCLR, GX_BL_ZERO, GX_LO_CLEAR);
            break;
        case BLENDMODE_ADD:
            Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
            Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
            Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_ONE);
            //GX_SetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_ONE, GX_LO_CLEAR);
            break;
    }

    DrawVerticeFormats *vertex = (DrawVerticeFormats *) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    memset(vertex, 0, 3 * sizeof (DrawVerticeFormats));
    {
        vertex[0].x = prim->bounds.x0 - 0.5f;
        vertex[0].y = prim->bounds.y0 - 0.5f;
        vertex[1].x = prim->bounds.x1 - 0.5f;
        vertex[1].y = prim->bounds.y0 - 0.5f;
        vertex[2].x = prim->bounds.x0 - 0.5f;
        vertex[2].y = prim->bounds.y1 - 0.5f;
        vertex[3].x = prim->bounds.x1 - 0.5f;
        vertex[3].y = prim->bounds.y1 - 0.5f;

        // set the texture coordinates
        if (texture != NULL) {
            vertex[0].u = prim->texcoords.tl.u;
            vertex[0].v = prim->texcoords.tl.v;
            vertex[1].u = prim->texcoords.tr.u;
            vertex[1].v = prim->texcoords.tr.v;
            vertex[2].u = prim->texcoords.bl.u;
            vertex[2].v = prim->texcoords.bl.v;
            vertex[3].u = prim->texcoords.br.u;
            vertex[3].v = prim->texcoords.br.v;
            n++;
        }

        XeColor color;
        color.r = (prim->color.r * 255.0f);
        color.g = (prim->color.g * 255.0f);
        color.b = (prim->color.b * 255.0f);
        color.a = (prim->color.a * 255.0f);

        int i = 0;
        for (i = 0; i < 3; i++) {
            vertex[i].z = 0.0;
            vertex[i].w = 1.0;
        }
    }


    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetStreamSource(g_pVideoDevice, 0, vb, nb_vertices, sizeof (DrawVerticeFormats));
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);

    nb_vertices += 256;
}

void hw_xenon_update_video(render_primitive_list &primlist) {
    pre_render();

    render_primitive *prim;

    // texture ...
    /*
        for (prim = primlist.first(); prim != NULL; prim = prim->next()) {
            if (prim->texture.base != NULL) {
                // update texture ...
                texture_update(prim);
            }
        }
     */

    n = 0;
    // begin ...
    for (prim = primlist.first(); prim != NULL; prim = prim->next()) {
        
        switch (prim->type) {
            case render_primitive::LINE:
                draw_line(prim);
                break;

            case render_primitive::QUAD:
                draw_quad(prim);
                break;

            default:
                throw emu_fatalerror("Unexpected render_primitive type");
        }
    }


    printf("N:%d\r\n",n);
    
    render();
}
#endif

static DrawVerticeFormats * CreateVertices(float x,float y,float w,float h){
    DrawVerticeFormats *Rect = (DrawVerticeFormats *)Xe_VB_Lock(g_pVideoDevice, soft_vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // top left
        Rect[0].x = x - w;
        Rect[0].y = y + h;
        Rect[0].u = ScreenUv[UvBottom];
        Rect[0].v = ScreenUv[UvRight];
        Rect[0].color = 0;

        // bottom left
        Rect[1].x = x -w ;
        Rect[1].y = y - h;
        Rect[1].u = ScreenUv[UvBottom];
        Rect[1].v = ScreenUv[UvLeft];
        Rect[1].color = 0;

        // top right
        Rect[2].x = x + w;
        Rect[2].y = y + h;
        Rect[2].u = ScreenUv[UvTop];
        Rect[2].v = ScreenUv[UvRight];
        Rect[2].color = 0;

        int i = 0;
        for (i = 0; i < 3; i++) {
            Rect[i].z = 0.0;
            Rect[i].w = 1.0;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, soft_vb);
    
    return Rect;
}

void osd_xenon_video_init() {
    g_pVideoDevice = &_xe;
    Xe_Init(g_pVideoDevice);
    
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    Xe_SetRenderTarget(g_pVideoDevice, fb);

    static const struct XenosVBFFormat vbf ={
        3,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
        }
    };

    g_pPixelTexturedShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_PS);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelTexturedShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xvs_VS);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);
    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);

    g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);
    screen = (unsigned int*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    g_pTexture->use_filtering = 0;
    
    pitch = g_pTexture->wpitch;    

    screen_width = fb->width;
    screen_height = fb->height;

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (DrawVerticeFormats));
    soft_vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (DrawVerticeFormats));
        
    CreateVertices(0,0,1,1);

    Xe_SetClearColor(g_pVideoDevice, 0);

    edram_init(g_pVideoDevice);
}

static void pre_render() {
    Xe_SetAlphaTestEnable(g_pVideoDevice, 1);

    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);

    Xe_InvalidateState(g_pVideoDevice);
    
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);
    
    nb_vertices = 0;
}

static void render() {
    Xe_Resolve(g_pVideoDevice);
    //while (!Xe_IsVBlank(g_pVideoDevice));
    Xe_Sync(g_pVideoDevice);
}


void osd_xenon_update_video(render_primitive_list &primlist) {
     // lock them, and then render them
    primlist.acquire_lock();
    
    int minwidth, minheight;
    int newwidth, newheight;
    
    // get the minimum width/height for the current layout
    xenos_target->compute_minimum_size(minwidth, minheight);

    // make that the size of our target
    xenos_target->set_bounds(minwidth, minheight);

    xenos_target->compute_visible_area(screen_width, screen_height, screen_width/screen_height, xenos_target->orientation(), newwidth, newheight);
    
    // update vertices buffer
    CreateVertices(0,0,(float)((float)newwidth/(float)screen_width),1);
    //printf("%d - %d - %d - %d\r\n",newwidth, newheight, minwidth, minheight);
        
    // update texture
    draw32_draw_primitives(primlist, screen, minwidth, minheight, g_pTexture->wpitch/4);
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);
    
    // correct texture size
    g_pTexture->width = minwidth;
    g_pTexture->height = minheight;
    
    pre_render();
    
    // draw
    Xe_SetStreamSource(g_pVideoDevice, 0, soft_vb, 0, sizeof (DrawVerticeFormats));
    Xe_SetTexture(g_pVideoDevice,0,g_pTexture);
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);
    
    render();
    
    primlist.release_lock();
}

