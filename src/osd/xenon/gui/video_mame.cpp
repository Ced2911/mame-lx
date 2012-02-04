#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <xetypes.h>

#include <input/input.h>
#include <console/console.h>

#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>

#include <debug.h>


// MAME headers
#include "emu.h"
#include "render.h"
#include "ui.h"
#include "rendutil.h"
#include "options.h"
#include "emuopts.h"
#include "aviio.h"
#include "png.h"

// wiigui stuff
#include "video.h"

// blitter func
#include "blit.inl.h"

// tmp
void SetRS();

typedef unsigned int DWORD;

extern int nb_vertices;
extern XenosDevice * g_pVideoDevice;
extern XenosVertexBuffer *vb;

static XenosShader * g_pVertexShader = NULL;
static XenosShader * g_pPixelShader = NULL;

static XenosSurface * default_surface = NULL;

#include "../hlsl/primary.ps.h"
#include "../hlsl/primary.vs.h"

typedef struct {
    float x, y, z; // 12
    unsigned int color; // 16
    float u, v; // 24
    /** padding **/
    float p1, p2; // 32
} __attribute__((packed, aligned(32))) MameVerticeFormats;

struct xe_tex {
    xe_tex *next;
    void *addr;
    struct XenosSurface *surface;
};

/* line_aa_step is used for drawing antialiased lines */
typedef struct _line_aa_step line_aa_step;

struct _line_aa_step {
    float xoffs, yoffs; // X/Y deltas
    float weight; // weight contribution
};
static const line_aa_step line_aa_1step[] = {
    { 0.00f, 0.00f, 1.00f},
    { 0}
};

static const line_aa_step line_aa_4step[] = {
    { -0.25f, 0.00f, 0.25f},
    { 0.25f, 0.00f, 0.25f},
    { 0.00f, -0.25f, 0.25f},
    { 0.00f, 0.25f, 0.25f},
    { 0}
};

static xe_tex *firstTex = NULL;
static xe_tex *lastTex = NULL;

static xe_tex *screenFirstTex = NULL;
static xe_tex *screenLastTex = NULL;

struct XenosSurface * screen_texture = NULL;

static struct XenosSurface * CreateSurface(int w, int h, int fmt) {

    struct XenosSurface * s = Xe_CreateTexture(g_pVideoDevice, w, h, 1, fmt, 0);
    memset(s->base, 0, s->hpitch * s->wpitch);
    return s;
}

static struct XenosSurface * GetScreenSurface(int w, int h, int fmt) {
    if (screen_texture == NULL) {
        screen_texture = CreateSurface(1024, 1024, fmt);
    }

    screen_texture->width = w;
    screen_texture->height = h;

    return screen_texture;
}

/**
 * When we change the number of prim we clean all the allocated textures exepct the screen one
 **/
static int bNeedClean = 0;
static int nbPrim = 0;

static xe_tex *create_texture(render_primitive *prim) {
    int j, k, l, x, y, tx, ty, bpp;
    int flag = PRIMFLAG_GET_TEXFORMAT(prim->flags);
    int rawwidth = prim->texture.width;
    int rawheight = prim->texture.height;
    //    int width = ((rawwidth + 3) & (~3));
    //    int height = ((rawheight + 3) & (~3));

    int width = ((rawwidth));
    int height = ((rawheight));
    // first the width
    if (width & (width - 1)) {
        width |= width >> 1;
        width |= width >> 2;
        width |= width >> 4;
        width |= width >> 8;
        width++;
    }

    // then the height
    if (height & (height - 1)) {
        height |= height >> 1;
        height |= height >> 2;
        height |= height >> 4;
        height |= height >> 8;
        height++;
    }

    u8 *data = (u8 *) prim->texture.base;

    xe_tex *newTex = (xe_tex*) malloc(sizeof (xe_tex));

    memset(newTex, 0, sizeof (xe_tex));

    j = 0;
    switch (flag) {
            // screen tex
       
        case TEXFORMAT_PALETTEA16:
        {
            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            }
            //            newTex->surface = GetScreenSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);

            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_palettea16((UINT32 *) dst, (UINT16 *) prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
        case TEXFORMAT_PALETTE16:
        {
            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            }
            //            newTex->surface = GetScreenSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_palette16((UINT32 *) dst, (UINT16 *) prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
            // neogeo system use this one
        case TEXFORMAT_RGB32:
        {
            TR;
            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            }
            //            printf("%d - %d\r\n",width,height);
            //            newTex->surface = GetScreenSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_rgb32((UINT32 *) dst, (UINT32 *) prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }


            // screen tex
        case TEXFORMAT_YUY16:
        {
            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            }
            //            newTex->surface = GetScreenSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_yuy16_to_argb((UINT32 *) dst, (UINT16 *) prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 1);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }

            // Small bitmap
        case TEXFORMAT_ARGB32:
        {
            

            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, XE_FMT_8888 | XE_FMT_ARGB);
            }
            break;
#if 1            
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u32 * src32;
            u32 * dst_limit = xe_dest + ((newTex->surface->wpitch / 4) * newTex->surface->hpitch);
            int hpitch, wpitch;

            for (hpitch = 0; hpitch < newTex->surface->hpitch; hpitch += newTex->surface->height) {
                for (wpitch = 0; wpitch < newTex->surface->wpitch; wpitch += newTex->surface->width) {
                    for (int y = 0; y < newTex->surface->height; y++) {
                        for (int x = 0; x < newTex->surface->width; x++) {

                            if (x >= newTex->surface->wpitch)
                                break;

                            dst = xe_dest + ((x + ((y + hpitch) * (newTex->surface->wpitch / 4))) + wpitch);

                            int nx = x;
                            int ny = y;

                            if (x > rawwidth) {
                                nx = 0;
                            }
                            if (y > rawheight) {
                                ny = 0;
                            }

                            src32 = (u32 *) data + ((nx)+(ny * prim->texture.rowpixels));
                            if (dst < dst_limit)
                                dst[0] = src32[0];
                        }
                        if (y >= newTex->surface->hpitch)
                            break;
                    }
                }
            }
            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
#endif
            break;
        }

        default:
            return NULL;
    }
    newTex->addr = &(*data);

//    printf("prim->flags = 0x%08x\r\n", prim->flags);

    if (PRIMFLAG_GET_SCREENTEX(prim->flags)) {
        TR;
        if (screenFirstTex == NULL)
            screenFirstTex = newTex;
        else
            screenLastTex->next = newTex;

        screenLastTex = newTex;
    } else {
        if (firstTex == NULL)
            firstTex = newTex;
        else
            lastTex->next = newTex;

        lastTex = newTex;
    }

    return newTex;
}

static xe_tex *get_texture(render_primitive *prim) {
    xe_tex *t = firstTex;

    if (PRIMFLAG_GET_SCREENTEX(prim->flags)) {
        return create_texture(prim);
    }

    while (t != NULL)
        if (t->addr == prim->texture.base)
            return t;
        else
            t = t->next;

    return create_texture(prim);
}

static void prep_texture(render_primitive *prim) {
    Xe_SetTexture(g_pVideoDevice, 0, default_surface);

    xe_tex *newTex = get_texture(prim);

    if (newTex == NULL) {
        if (prim->texture.base)
            printf("Error\r\n");
        return;
    }


//    newTex->surface->width = prim->texture.width;
//    newTex->surface->height = prim->texture.height;



    if (newTex->surface == screen_texture) {
        //        printf("Use screen surf\r\n");
        //        printf("%d - %d\r\n",newTex->surface->width,newTex->surface->height);
    }

    if (prim->texture.base)
        Xe_SetTexture(g_pVideoDevice, 0, newTex->surface);
}

static void clearTexs() {
    xe_tex *t = firstTex;
    xe_tex *n;

    TR;
    while (t != NULL) {
        n = t->next;
        //free(t->data);
        if (t->surface) {
            TR;
            if (t->surface->base)
                Xe_DestroyTexture(g_pVideoDevice, t->surface);
            TR;
        }
        free(t);
        TR;
        t = n;
    }
    TR;

    firstTex = NULL;
    lastTex = NULL;
}

static void screenClearTexs() {
    xe_tex *t = screenFirstTex;
    xe_tex *n;

    int i = 0;

    //TR;
    while (t != NULL) {
        n = t->next;
        //free(t->data);
        if (t->surface) {
            //TR;
            if (t->surface->base)
                Xe_DestroyTexture(g_pVideoDevice, t->surface);
            //TR;
        }
        free(t);
        //TR;
        t = n;
        i++;
    }
    //TR;
//    if (i)
//        printf("screenClearTexs %d\r\n", i);


    screenFirstTex = NULL;
    screenLastTex = NULL;
}

void SetRS(render_primitive * prim) {
    int blendmode = PRIMFLAG_GET_BLENDMODE(prim->flags);

    int blendenable;
    int blendop;
    int blendsrc;
    int blenddst;

    switch (blendmode) {
        default:
        case BLENDMODE_NONE: blendenable = FALSE;
            blendop = XE_BLENDOP_ADD;
            blendsrc = XE_BLEND_SRCALPHA;
            blenddst = XE_BLEND_INVSRCALPHA;
            break;
        case BLENDMODE_ALPHA: blendenable = TRUE;
            blendop = XE_BLENDOP_ADD;
            blendsrc = XE_BLEND_SRCALPHA;
            blenddst = XE_BLEND_INVSRCALPHA;
            break;
        case BLENDMODE_RGB_MULTIPLY: blendenable = TRUE;
            blendop = XE_BLENDOP_ADD;
            blendsrc = XE_BLEND_DESTCOLOR;
            blenddst = XE_BLEND_ZERO;
            break;
        case BLENDMODE_ADD: blendenable = TRUE;
            blendop = XE_BLENDOP_ADD;
            blendsrc = XE_BLEND_SRCALPHA;
            blenddst = XE_BLEND_ONE;
            break;
    }

    Xe_SetBlendOp(g_pVideoDevice, blendop);
    Xe_SetSrcBlend(g_pVideoDevice, blendsrc);
    Xe_SetDestBlend(g_pVideoDevice, blenddst);
    Xe_SetAlphaTestEnable(g_pVideoDevice, blendenable);

    //    Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
    //    Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
    //    Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_INVSRCALPHA);
    //    Xe_SetAlphaTestEnable(g_pVideoDevice, 1);

    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, nb_vertices, 12);
}

/**
 *
 * @param prim
 */
void DrawQuad(render_primitive * prim) {
    /* from shader */
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
    float TargetWidth = fb->width;
    float TargetHeight = fb->height;

    float RawWidth = prim->texture.width;
    float RawHeight = prim->texture.height;

    float PostPass = 0.f;

    float WidthRatio = 1;
    float HeightRatio = 1;

    XeColor color;

    // R => B
    color.a = prim->color.a * 255.f;
    color.b = prim->color.r * 255.f;
    color.g = prim->color.g * 255.f;
    color.r = prim->color.b * 255.f;

    prep_texture(prim);

    MameVerticeFormats* Rect = (MameVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (MameVerticeFormats), XE_LOCK_WRITE);
    {
        Rect[0].x = prim->bounds.x0 - 0.5f;
        Rect[0].y = prim->bounds.y0 - 0.5f;
        Rect[1].x = prim->bounds.x1 - 0.5f;
        Rect[1].y = prim->bounds.y0 - 0.5f;
        Rect[2].x = prim->bounds.x0 - 0.5f;
        Rect[2].y = prim->bounds.y1 - 0.5f;
        Rect[3].x = prim->bounds.x1 - 0.5f;
        Rect[3].y = prim->bounds.y1 - 0.5f;

        Rect[0].u = prim->texcoords.tl.u;
        Rect[0].v = prim->texcoords.tl.v;
        Rect[1].u = prim->texcoords.tr.u;
        Rect[1].v = prim->texcoords.tr.v;
        Rect[2].u = prim->texcoords.bl.u;
        Rect[2].v = prim->texcoords.bl.v;
        Rect[3].u = prim->texcoords.br.u;
        Rect[3].v = prim->texcoords.br.v;
        int i = 0;
        for (i = 0; i < 4; i++) {
            Rect[i].z = 0.0;
            Rect[i].color = color.lcol;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    // primary.fx
    // Registers:
    //
    //   Name         Reg   Size
    //   ------------ ----- ----
    //   TargetWidth  c0       1
    //   TargetHeight c1       1
    //   PostPass     c2       1
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, (float*) &TargetWidth, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 1, (float*) &TargetHeight, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 2, (float*) &PostPass, 1);

    SetRS(prim);

    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);
    //nb_vertices += 4 * sizeof (DrawVerticeFormats);
    nb_vertices += 256; // fixe aligement

    nbPrim++;
}

void DrawLine(render_primitive * prim) {
    render_bounds b0, b1;
    const line_aa_step *step = line_aa_4step;

    /* from shader */
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
    float TargetWidth = fb->width;
    float TargetHeight = fb->height;

    float PostPass = 0.f;

    XeColor color;

    MameVerticeFormats* Rect = (MameVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (MameVerticeFormats), XE_LOCK_WRITE);
    {
        Rect[0].x = prim->bounds.x0 - 0.5f;
        Rect[0].y = prim->bounds.y0 - 0.5f;
        Rect[1].x = prim->bounds.x1 - 0.5f;
        Rect[1].y = prim->bounds.y0 - 0.5f;

        Rect[0].u = 0;
        Rect[0].v = 0;
        Rect[1].u = 0;
        Rect[1].v = 1;

        // R => B
        color.a = prim->color.a * 255.f;
        color.b = prim->color.r * 255.f;
        color.g = prim->color.g * 255.f;
        color.r = prim->color.b * 255.f;

        int i = 0;
        for (i = 0; i < 2; i++) {
            Rect[i].z = 0.0;
            Rect[i].color = color.lcol;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);


    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    // primary.fx
    // Registers:
    //
    //   Name         Reg   Size
    //   ------------ ----- ----
    //   TargetWidth  c0       1
    //   TargetHeight c1       1
    //   PostPass     c2       1

    Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, (float*) &TargetWidth, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 1, (float*) &TargetHeight, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 2, (float*) &PostPass, 1);

    SetRS(prim);

    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_LINELIST, 0, 1);
    nb_vertices += 256; // fixe aligement

    nbPrim++;
    //}

}

void MameFrame() {
    static int oldNbPrim = 0;
    if (oldNbPrim != nbPrim) {
        // if number of prim changed clear small textures caches
        // clearTexs();
    }

    screenClearTexs();
    oldNbPrim = nbPrim;
    nbPrim = 0;
}

void InitMameShaders() {
    // fake texture
    default_surface = Xe_CreateTexture(g_pVideoDevice, 256, 256, 1, XE_FMT_ARGB | XE_FMT_8888, 0);

    u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, default_surface, 0, 0, 0, 0, XE_LOCK_WRITE);

    for (int y = 0; y < default_surface->height; y++)
        for (int x = 0; x < default_surface->width; x++)
            *xe_dest++ = 0xFFFFFFFF;

    Xe_Surface_Unlock(g_pVideoDevice, default_surface);

    void* vs_program = NULL;
    void* ps_program = NULL;

    /*
     struct VS_INPUT
    {
            float3 Position : POSITION;
            float4 Color : COLOR0;
            float2 TexCoord : TEXCOORD0;
    };
     */

    static const struct XenosVBFFormat vbf = {
        4,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT3},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
            {XE_USAGE_TEXCOORD, 1, XE_TYPE_FLOAT2}, //padding
        }
    };
    vs_program = (void*) g_xvs_vs_main;
    ps_program = (void*) g_xps_ps_main;

    g_pPixelShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) ps_program);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) vs_program);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);

    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);
}