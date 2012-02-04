#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <debug.h>
#include <stdarg.h>

#include <usb/usbmain.h>
#include <console/console.h>
#include <usb/usbmain.h>
#include <xenon_soc/xenon_power.h>
#include <time/time.h>
#include <ppc/atomic.h>
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

render_target *xenos_target;

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

#if 0 // need port
#include "ps.h"
#include "vs.h"

#include "shaders/xbr_2x_ps.h"
#include "shaders/xbr_2x_vs.h"

#include "shaders/xbr_5x_ps.h"
#include "shaders/xbr_5x_vs.h"

#include "shaders/xbr_5x_crt_ps.h"
#include "shaders/xbr_5x_crt_vs.h"
#endif

#include "hlsl/primary.ps.h"
#include "hlsl/primary.vs.h"

static struct XenosDevice _xe;
static struct XenosVertexBuffer *vb = NULL;
static struct XenosVertexBuffer *soft_vb = NULL;
// static struct XenosDevice * g_pVideoDevice = NULL;
extern struct XenosDevice * g_pVideoDevice;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;
static struct XenosSurface * g_pTexture = NULL;
static unsigned int * screen = NULL;

static int screen_width;
static int screen_height;
static int video_thread_running = 0;
static render_primitive_list * currList;
static unsigned int thread_lock __attribute__((aligned(128))) = 0;

static unsigned char thread_stack[0x10000];

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

typedef struct {
    float x, y, z; // 12
    unsigned int color; // 16
    float u, v; // 24
    /** padding **/
    float p1, p2; // 32
} __attribute__((packed, aligned(32))) MameVerticeFormats;

static MameVerticeFormats *vertices;

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
            4,
            {
                {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT3},
                {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
                {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
                {XE_USAGE_TEXCOORD, 1, XE_TYPE_FLOAT2}, //padding
            }
        };

        ps = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) ps_program);
        Xe_InstantiateShader(g_pVideoDevice, ps, 0);

        vs = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) vs_program);
        Xe_InstantiateShader(g_pVideoDevice, vs, 0);
        Xe_ShaderApplyVFetchPatches(g_pVideoDevice, vs, 0, &vbf);
    }

    virtual void Render(int width, int height) {
        // primary.fx
        // Registers:
        //
        //   Name         Reg   Size
        //   ------------ ----- ----
        //   TargetWidth  c0       1
        //   TargetHeight c1       1
        //   PostPass     c2       1

        XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
        float TargetWidth = fb->width;
        float TargetHeight = fb->height;

        float PostPass = 0.f;
        
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, ps, 0);
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, vs, 0);
        
        Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, (float*) &TargetWidth, 1);
        Xe_SetVertexShaderConstantF(g_pVideoDevice, 1, (float*) &TargetHeight, 1);
        Xe_SetVertexShaderConstantF(g_pVideoDevice, 2, (float*) &PostPass, 1);
    };
};

class xbr_effect : public shader_effect
{

public:
    void Render(int width, int height) {
        float settings_texture_size[2] = {width, height};

        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, ps, 0);
        Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, vs, 0);

//        Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, settings_texture_size, 1);
//        Xe_SetPixelShaderConstantF(g_pVideoDevice, 0, settings_texture_size, 1);
    };
};

std::vector <shader_effect> ShaderEffects;

static void LoadShaderEffects() {
    shader_effect normal;
//    xbr_effect xbr2x;
//    xbr_effect xbr5x;
//    xbr_effect xbr5x_crt;

    normal.Set("Normal", (void*) g_xvs_vs_main, (void*) g_xps_ps_main);
//    xbr2x.Set("Xbr 2x", (void*) g_xvs_xbr2x_vs_main, (void*) g_xps_xbr2x_ps_main);
//    xbr5x.Set("Xbr 5x", (void*) g_xvs_xbr5x_vs_main, (void*) g_xps_xbr5x_ps_main);
//    xbr5x_crt.Set("Xbr 5x Crt", (void*) g_xvs_xbr5xcrt_vs_main, (void*) g_xps_xbr5xcrt_ps_main);

    ShaderEffects.push_back(normal);
//    ShaderEffects.push_back(xbr2x);
//    ShaderEffects.push_back(xbr5x);
//    ShaderEffects.push_back(xbr5x_crt);

    std::vector<shader_effect>::iterator it;

    for (it = ShaderEffects.begin(); it < ShaderEffects.end(); it++) {
        it->Init();
    }
}

static MameVerticeFormats * CreateRect(float width, float height, MameVerticeFormats *Rect) {
    // bottom left
    Rect[0].x = -width;
    Rect[0].y = -height;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = 0xFFFFFFFF;

    // bottom right
    Rect[1].x = width;
    Rect[1].y = -height;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = 0xFFFFFFFF;

    // top right
    Rect[2].x = width;
    Rect[2].y = height;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = 0xFFFFFFFF;

    int i = 0;
    for (i = 0; i < 3; i++) {
        Rect[i].z = 0.0;
//        Rect[i].w = 1.0;
    }

    Rect += 3;

    return Rect;
}

static MameVerticeFormats * CreateRectHlsl(float width, float height, MameVerticeFormats *Rect) {
    // bottom left
    Rect[0].x = 0 - 0.5f;
    Rect[0].y = 0 - 0.5f;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = 0xFFFFFFFF;

    // bottom right
    Rect[1].x = width - 0.5f;
    Rect[1].y = 0 - 0.5f;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = 0xFFFFFFFF;

    // top right
    Rect[2].x = width - 0.5f;
    Rect[2].y = height - 0.5f;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = 0xFFFFFFFF;

    int i = 0;
    for (i = 0; i < 3; i++) {
        Rect[i].z = 0.0;
//        Rect[i].w = 1.0;
    }

    Rect += 3;

    return Rect;
}

static void pre_render() {
    while (!Xe_IsVBlank(g_pVideoDevice));

    // sync before drawing
    Xe_Sync(g_pVideoDevice);

    Xe_InvalidateState(g_pVideoDevice);

    Xe_SetAlphaTestEnable(g_pVideoDevice, 1);

    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);

    nb_vertices = 0;
}

static void render() {

    Xe_Resolve(g_pVideoDevice);
    //

    // Xe_Sync(g_pVideoDevice); // done in pre_render
    /* begin rendering in background */
    Xe_Execute(g_pVideoDevice);
}

static void osd_xenon_video_thread() {
    video_thread_running = 1;
    while (video_thread_running) {

        if (currList == NULL) {
            udelay(10);
        } else {
            // tmp
            lock(&thread_lock);
            // lock them, and then render them
            //currList->acquire_lock();

            pre_render();

            int minwidth, minheight;
            int newwidth, newheight;

            // get the minimum width/height for the current layout
            xenos_target->compute_minimum_size(minwidth, minheight);

//            minwidth = screen_width;
//            minheight = screen_height;

            // make that the size of our target
            xenos_target->set_bounds(minwidth, minheight);

            xenos_target->compute_visible_area(screen_width, screen_height, screen_width / screen_height, xenos_target->orientation(), newwidth, newheight);

            //ShaderEffects.at(2).Render(minwidth, minheight);
            ShaderEffects.at(0).Render(minwidth, minheight);

            Xe_SetStreamSource(g_pVideoDevice, 0, soft_vb, nb_vertices, sizeof (MameVerticeFormats));

            vertices = (MameVerticeFormats *) Xe_VB_Lock(g_pVideoDevice, soft_vb, 0, 3 * sizeof (MameVerticeFormats), XE_LOCK_WRITE);
            //CreateRect(((float) newwidth / (float) screen_width), -((float) newheight / (float) screen_height), vertices);
            CreateRectHlsl(screen_width, screen_height, vertices);
            Xe_VB_Unlock(g_pVideoDevice, soft_vb);

            // update texture
            //draw32_draw_primitives(primlist, screen, minwidth, minheight, g_pTexture->wpitch / 4);

            /* loop over the list and render each element */
            const render_primitive *prim;
            void *dstdata = (void *) screen;
            UINT32 width = minwidth;
            UINT32 height = minheight;
            UINT32 pitch = g_pTexture->wpitch / 4;
            for (prim = currList->first(); prim != NULL; prim = prim->next()) {
                switch (prim->type) {
                    case render_primitive::LINE:
                        draw32_draw_line(prim, dstdata, width, height, pitch);
                        break;

                    case render_primitive::QUAD:
                        if (!prim->texture.base)
                            draw32_draw_rect(prim, dstdata, width, height, pitch);
                        else
                            draw32_setup_and_draw_textured_quad(prim, dstdata, width, height, pitch);
                        break;

                    default:
                        throw emu_fatalerror("Unexpected render_primitive type");
                }
            }

            Xe_Surface_LockRect(g_pVideoDevice, g_pTexture, 0, 0, 0, 0, XE_LOCK_WRITE);
            Xe_Surface_Unlock(g_pVideoDevice, g_pTexture);

            // correct texture size
            g_pTexture->width = minwidth;
            g_pTexture->height = minheight;

            // draw
            Xe_SetTexture(g_pVideoDevice, 0, g_pTexture);
            Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);

            render();

            // tmp
            unlock(&thread_lock);
           // currList->release_lock();
            currList=NULL;
        }
    }
}

void osd_xenon_update_video(render_primitive_list &primlist) {

    lock(&thread_lock);

    //primlist->acquire_lock();
    currList = &primlist;
    //primlist->release_lock();

    // tmp 
    unlock(&thread_lock);
}

void osd_xenon_video_pause(){
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

/**
 * @todo copy the fb
 * @return 
 */
struct XenosSurface * osd_xenon_get_surface(){
//    udelay(1000);
//    TR;
//    lock(&thread_lock);
//    TR;
//    struct XenosSurface * copy = Xe_CreateTexture(g_pVideoDevice,g_pTexture->width,g_pTexture->height,0,g_pTexture->format,0);
//    TR;
//    uint32_t * src =(uint32_t *) Xe_Surface_LockRect(g_pVideoDevice,g_pTexture,0,0,0,0,XE_LOCK_WRITE);
//    uint32_t * dst =(uint32_t *) Xe_Surface_LockRect(g_pVideoDevice,copy,0,0,0,0,XE_LOCK_WRITE);
//    
//    // copy data
//    for(int y=0;y<g_pTexture->height;y++){
//        for(int x=0;x<g_pTexture->width;x++){
//            //dst[((y*copy->wpitch)+x)]=src[((y*g_pTexture->wpitch)+x)];
//        }
//    }
//    TR;
//    Xe_Surface_Unlock(g_pVideoDevice,g_pTexture);
//    Xe_Surface_Unlock(g_pVideoDevice,copy);
//    TR;
//    unlock(&thread_lock);
//    TR;
//    return copy;
    return g_pTexture;
}

void osd_xenon_video_resume(){
    if(video_thread_running==0){
        // run the video on a new thread
         xenon_run_thread_task(4, &thread_stack[sizeof (thread_stack) - 0x100], (void*) osd_xenon_video_thread);
    }
}

void osd_xenon_video_cleanup(running_machine &machine) {
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



void osd_xenon_video_hw_init(running_machine &machine) {
    //g_pVideoDevice = &_xe;
    //Xe_Init(g_pVideoDevice);

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

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (MameVerticeFormats));
    soft_vb = Xe_CreateVertexBuffer(g_pVideoDevice, 65536 * sizeof (MameVerticeFormats));


    float w = fb->width;
    float h = fb->height;

    Xe_SetClearColor(g_pVideoDevice, 0);

    osd_xenon_video_resume();

    // on mame exit
    machine.add_notifier(MACHINE_NOTIFY_EXIT, machine_notify_delegate(FUNC(osd_xenon_video_cleanup), &machine));
}