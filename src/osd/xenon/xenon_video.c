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

#include <vector>

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
/*
#include "shaders/osd_ps.h"
#include "shaders/osd_vs.h"
 */

#include "ps.h"
#include "vs.h"

#include "shaders/xbr_2x_ps.h"
#include "shaders/xbr_2x_vs.h"

#include "shaders/xbr_5x_ps.h"
#include "shaders/xbr_5x_vs.h"

#include "shaders/xbr_5x_crt_ps.h"
#include "shaders/xbr_5x_crt_vs.h"

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

static DrawVerticeFormats *vertices;

static int nb_vertices = 0;

class shader_effect
{
public:
    char * name;
    void * vs_program;
    void * ps_program;
    XenosShader * ps;
    XenosShader * vs;

    void Set(char * _name, void * _vs_program, void * _ps_program) {
        name = _name;
        vs_program = _vs_program;
        ps_program = _ps_program;
        ps = NULL;
        vs = NULL;
    }

    void Init() {
        static const struct XenosVBFFormat vbf = {
            3,
            {
                {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
                {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
                {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
            }
        };

        ps = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) ps_program);
        Xe_InstantiateShader(g_pVideoDevice, ps, 0);

        vs = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) vs_program);
        Xe_InstantiateShader(g_pVideoDevice, vs, 0);
        Xe_ShaderApplyVFetchPatches(g_pVideoDevice, vs, 0, &vbf);
    }

    virtual void Render(int width, int height) {
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, ps, 0);
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, vs, 0);
    };
};

class xbr_effect : public shader_effect
{

public:
    void Render(int width, int height) {
        float settings_texture_size[2] = {width, height};

        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, ps, 0);
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, vs, 0);

        Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, settings_texture_size, 1);
        Xe_SetPixelShaderConstantF(g_pVideoDevice, 0, settings_texture_size, 1);
    };
};

std::vector <shader_effect> ShaderEffects;

static void LoadShaderEffects() {
    shader_effect normal;
    xbr_effect xbr2x;
    xbr_effect xbr5x;
    xbr_effect xbr5x_crt;

    normal.Set("Normal", (void*) g_xvs_VS, (void*) g_xps_PS);
    xbr2x.Set("Xbr 2x", (void*) g_xvs_xbr2x_vs_main, (void*) g_xps_xbr2x_ps_main);
    xbr5x.Set("Xbr 5x", (void*) g_xvs_xbr5x_vs_main, (void*) g_xps_xbr5x_ps_main);
    xbr5x_crt.Set("Xbr 5x Crt", (void*) g_xvs_xbr5xcrt_vs_main, (void*) g_xps_xbr5xcrt_ps_main);

    ShaderEffects.push_back(normal);
    ShaderEffects.push_back(xbr2x);
    ShaderEffects.push_back(xbr5x);
    ShaderEffects.push_back(xbr5x_crt);

    std::vector<shader_effect>::iterator it;

    for (it = ShaderEffects.begin(); it < ShaderEffects.end(); it++) {
        it->Init();
    }
}

static DrawVerticeFormats * CreateRect(float width, float height, DrawVerticeFormats *Rect) {
    // bottom left
    Rect[0].x = -width;
    Rect[0].y = -height;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = 0;

    // bottom right
    Rect[1].x = width;
    Rect[1].y = -height;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = 0;

    // top right
    Rect[2].x = width;
    Rect[2].y = height;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = 0;

    // Top left
    Rect[3].x = -width;
    Rect[3].y = height;
    Rect[3].u = 0;
    Rect[3].v = 1;
    Rect[3].color = 0;

    int i = 0;
    for (i = 0; i < 3; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }

    Rect += 3;

    return Rect;
}

void osd_xenon_video_init() {
    g_pVideoDevice = &_xe;
    Xe_Init(g_pVideoDevice);

    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    Xe_SetRenderTarget(g_pVideoDevice, fb);

    LoadShaderEffects();
    
    g_pTexture = Xe_CreateTexture(g_pVideoDevice, XE_W, XE_H, 1, XE_FMT_8888 | XE_FMT_ARGB, 0);
    screen = (unsigned int*) Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    g_pTexture->use_filtering = 0;

    pitch = g_pTexture->wpitch;

    screen_width = fb->width;
    screen_height = fb->height;

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (DrawVerticeFormats));
    soft_vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (DrawVerticeFormats));


    float w = fb->width;
    float h = fb->height;

    //CreateRect(w, h);
    // CreateVertices(-1, -1, w, h);

    Xe_SetClearColor(g_pVideoDevice, 0);

    edram_init(g_pVideoDevice);
}

static void pre_render() {
    // sync before drawing
    Xe_Sync(g_pVideoDevice);
    
    //while (!Xe_IsVBlank(g_pVideoDevice));

    Xe_SetAlphaTestEnable(g_pVideoDevice, 1);

    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);

    Xe_InvalidateState(g_pVideoDevice);

    Xe_SetStreamSource(g_pVideoDevice, 0, soft_vb, 0, sizeof (DrawVerticeFormats));

    vertices = (DrawVerticeFormats *) Xe_VB_Lock(g_pVideoDevice, soft_vb, 0, 3 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);

    nb_vertices = 0;
}

static void render() {
    Xe_VB_Unlock(g_pVideoDevice, soft_vb);
    Xe_Resolve(g_pVideoDevice);
    //while (!Xe_IsVBlank(g_pVideoDevice));

    // Xe_Sync(g_pVideoDevice); // done in pre_render
    /* begin rendering in background */
    Xe_Execute(g_pVideoDevice);
}

void osd_xenon_update_video(render_primitive_list &primlist) {
    // lock them, and then render them
    primlist.acquire_lock();

    pre_render();

    int minwidth, minheight;
    int newwidth, newheight;

    // get the minimum width/height for the current layout
    xenos_target->compute_minimum_size(minwidth, minheight);

    // make that the size of our target
    xenos_target->set_bounds(minwidth, minheight);

    xenos_target->compute_visible_area(screen_width, screen_height, screen_width / screen_height, xenos_target->orientation(), newwidth, newheight);

    ShaderEffects.at(2).Render(minwidth,minheight);

    CreateRect(((float) newwidth / (float) screen_width), -((float) newheight / (float) screen_height), vertices);

    // update texture
    draw32_draw_primitives(primlist, screen, minwidth, minheight, g_pTexture->wpitch / 4);
    Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
    Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

    // correct texture size
    g_pTexture->width = minwidth;
    g_pTexture->height = minheight;

    // draw
    Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);

    render();

    primlist.release_lock();
}

