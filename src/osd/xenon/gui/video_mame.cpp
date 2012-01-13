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


// mame stuff
#include "render.h"

// wiigui stuff
#include "video.h"

// tmp
void SetRS();

typedef unsigned int DWORD;

extern int nb_vertices;
extern XenosDevice * g_pVideoDevice;
extern XenosVertexBuffer *vb;

static XenosShader * g_pVertexShader = NULL;
static XenosShader * g_pPixelShader = NULL;

static XenosSurface * default_surface = NULL;

#define USE_MAME_HLSL

#ifdef USE_MAME_HLSL
#include "../hlsl/primary.ps.h"
#include "../hlsl/primary.vs.h"

typedef struct {
    float x, y, z; // 12
    unsigned int color; // 16
    float u, v; // 24
    /** padding **/
    float p1, p2; // 32
} __attribute__((packed, aligned(32))) MameVerticeFormats;
#else
#include "../shaders_hw/ps.tc.h"
#include "../shaders_hw/vs.h"

typedef struct {
    float x, y, z, w; // 16
    unsigned int color; // 20
    unsigned int padding; // 24
    float u, v; // 32
} __attribute__((packed, aligned(32))) MameVerticeFormats;
#endif

/**
 *
 * Mame stuff 
 * 
 * 
 */
typedef struct _gx_tex gx_tex;

struct _gx_tex {
    u32 size;
    u8 format;
    gx_tex *next;
    void *addr;
    void *data;
    struct XenosSurface *surface;
};

static gx_tex *firstTex = NULL;
static gx_tex *lastTex = NULL;
static gx_tex *firstScreenTex = NULL;
static gx_tex *lastScreenTex = NULL;

/* adapted from rendersw.c, might not work because as far as I can tell, only 
   laserdisc uses YCbCr textures, and we don't support that be default */

static u32 yuy_rgb = 0;

inline u8 clamp16_shift8(u32 x) {
    return (((s32) x < 0) ? 0 : (x > 65535 ? 255 : x >> 8));
}

inline u16 GXGetRGBA5551_YUY16(u32 *src, u32 x, u8 i) {
    if (!(i & 1)) {
        u32 ycc = src[x];
        u8 y = ycc;
        u8 cb = ycc >> 8;
        u8 cr = ycc >> 16;
        u32 r, g, b, common;

        common = 298 * y - 56992;
        r = (common + 409 * cr);
        g = (common - 100 * cb - 208 * cr + 91776);
        b = (common + 516 * cb - 13696);

        yuy_rgb = MAKE_RGB(clamp16_shift8(r), clamp16_shift8(g), clamp16_shift8(b)) | 0xFF;

        return (yuy_rgb >> 16) & 0x0000FFFF;
    } else {
        return yuy_rgb & 0x0000FFFF;
    }
}

/* heavily adapted from Wii64 */

inline u16 GXGetRGBA5551_RGB5A3(u16 *src, u32 x) {
    u16 c = src[x];
    if ((c & 1) != 0) c = 0x8000 | (((c >> 11)&0x1F) << 10) | (((c >> 6)&0x1F) << 5) | ((c >> 1)&0x1F); //opaque texel
    else c = 0x0000 | (((c >> 12)&0xF) << 8) | (((c >> 7)&0xF) << 4) | ((c >> 2)&0xF); //transparent texel
    return (u32) c;
}

inline u16 GXGetRGBA8888_RGBA8(u32 *src, u32 x, u8 i) {
    u32 c = src[x];
    u32 color = (i & 1) ? /* GGBB */ c & 0x0000FFFF : /* AARR */ (c >> 16) & 0x0000FFFF;
    return (u16) color;
}

inline u16 GXGetRGBA5551_PALETTE16(u16 *src, u32 x, int i, const rgb_t *palette) {
    u16 c = src[x];
    u32 rgb = palette[c];
    if (i == TEXFORMAT_PALETTE16) return rgb_to_rgb15(rgb) | (1 << 15);
    else return (u32) (((RGB_RED(rgb) >> 4) << 8) | ((RGB_GREEN(rgb) >> 4) << 4) | ((RGB_BLUE(rgb) >> 4) << 0) | ((RGB_ALPHA(rgb) >> 5) << 12));
}

static int allowed = 0;

static struct XenosSurface * CreateSurface(int w, int h, int fmt) {
    printf("Create texture : %d %d\r\n", w, h);

    struct XenosSurface * s = Xe_CreateTexture(g_pVideoDevice, w, h, 1, fmt, 0);

    allowed += s->wpitch * s->height;

    printf("Allowed %d for texture\r\n", allowed);

    return s;
}



#include "blit.inl.h"

struct XenosSurface * screen_texture = NULL;

static struct XenosSurface * GetScreenSurface(int w, int h, int fmt){
    if(screen_texture==NULL)
    {
        screen_texture = CreateSurface(1024,1024,fmt);
        
    }
//    if((screen_texture->width != w)||(screen_texture->height != h)){
//        // destroy
//        Xe_DestroyTexture(g_pVideoDevice,screen_texture);
//        screen_texture=CreateSurface(w,h,fmt);
//        return screen_texture;
//    }
    
    screen_texture->width=w;
    screen_texture->height=h;
    
    return screen_texture;
}

static gx_tex *create_texture(render_primitive *prim) {
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

    //    int width = ((rawwidth + 3) & (~3));
    //    int height = ((rawheight + 3) & (~3));

    int hpitch, wpitch;
    u8 *data = (u8 *) prim->texture.base;
    u8 *src;
    u16 *fixed;
    gx_tex *newTex = (gx_tex*) malloc(sizeof (*newTex));

    memset(newTex, 0, sizeof (*newTex));

    j = 0;
    
   // printf("flag:%d\r\n",flag);

    switch (flag) {
        // screen tex
        case TEXFORMAT_RGB15:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;

            newTex->surface = GetScreenSurface(width, height, newTex->format);
            
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u16 * src16 = (u16 *)data;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_rgb15((UINT32 *)dst, (UINT16 *)prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
        case TEXFORMAT_PALETTEA16:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;
            
            
            newTex->surface = GetScreenSurface(width, height, newTex->format);
            
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u16 * src16 = (u16 *)data;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_palettea16((UINT32 *)dst, (UINT16 *)prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
         case TEXFORMAT_PALETTE16:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;
                        
            newTex->surface = GetScreenSurface(width, height, newTex->format);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u16 * src16 = (u16 *)data;

            for (int y = 0; y < rawheight; y++) {
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_palette16((UINT32 *)dst, (UINT16 *)prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
        case TEXFORMAT_RGB32:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;
                        
            newTex->surface = GetScreenSurface(width, height, newTex->format);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u32 * src32 = (u32 *)data;

            for (int y = 0; y < rawheight; y++) {
                /*
                for (int x = 0; x < rawwidth; x++) {
                    dst = xe_dest + (x + (y * newTex->surface->wpitch / 4));
                    dst[0] = 0xff000000 | *src32++;
                }
                 */ 
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_rgb32((UINT32 *)dst, (UINT32 *)prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 2);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
        
        
        // screen tex
        case TEXFORMAT_YUY16:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;
            
            newTex->surface = GetScreenSurface(width, height, newTex->format);
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u16 * src16 = (u16 *)data;

            for (int y = 0; y < rawheight; y++) {
                /*
                for (int x = 0; x < rawwidth; x+=2) {
                    dst = xe_dest + (x + (y * newTex->surface->wpitch / 4));
                    
                    UINT16 srcpix0 = *src16++;
                    UINT16 srcpix1 = *src16++;
                    UINT8 cb = srcpix0 & 0xff;
                    UINT8 cr = srcpix1 & 0xff;
                    dst[0] = ycc_to_rgb(srcpix0 >> 8, cb, cr);
                    dst[1] = ycc_to_rgb(srcpix1 >> 8, cb, cr);
                }
                **/
                
                dst = xe_dest + ((y * newTex->surface->wpitch / 4));
                copyline_yuy16_to_argb((UINT32 *)dst, (UINT16 *)prim->texture.base + y * prim->texture.rowpixels, prim->texture.width, prim->texture.palette, 1);
            }

            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);
            break;
        }
        
        // Small bitmap
        case TEXFORMAT_ARGB32:
        {
            newTex->format = XE_FMT_8888 | XE_FMT_ARGB;
            bpp = 4;
            //fixed = (newTex->data) ? newTex->data : memalign(32, height * width * bpp);

            if (newTex->surface == NULL) {
                newTex->surface = CreateSurface(width, height, newTex->format);
                memset(newTex->surface->base, 0, newTex->surface->hpitch * newTex->surface->wpitch);
            }
            u32 * xe_dest = (u32*) Xe_Surface_LockRect(g_pVideoDevice, newTex->surface, 0, 0, 0, 0, XE_LOCK_WRITE);
            u32 * dst;
            u32 * src32;

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
                            dst[0] = src32[0];
                        }
                        if (y >= newTex->surface->hpitch)
                            break;
                    }
                }
            }
            Xe_Surface_Unlock(g_pVideoDevice, newTex->surface);

            break;
        }

        default:
            return NULL;
    }

    newTex->size = height * width * bpp;
    newTex->data = fixed;
    newTex->addr = &(*data);

    if (PRIMFLAG_GET_SCREENTEX(prim->flags)) {
        if (firstScreenTex == NULL)
            firstScreenTex = newTex;
        else
            lastScreenTex->next = newTex;

        lastScreenTex = newTex;
    } else {
        if (firstTex == NULL)
            firstTex = newTex;
        else
            lastTex->next = newTex;

        lastTex = newTex;
    }

    return newTex;
}

static gx_tex *get_texture(render_primitive *prim) {
    gx_tex *t = firstTex;

    // must handle this correctly
    /*
    if (PRIMFLAG_GET_SCREENTEX(prim->flags))
        return create_texture(prim);
     */ 
    while (t != NULL)
        if (t->addr == prim->texture.base)
            return t;
        else
            t = t->next;

    return create_texture(prim);
}

static void prep_texture(render_primitive *prim) {
    Xe_SetTexture(g_pVideoDevice, 0, default_surface);

    gx_tex *newTex = get_texture(prim);

    if (newTex == NULL)
        return;

    newTex->surface->width = prim->texture.width;
    newTex->surface->height = prim->texture.height;
    
    //DCFlushRange(newTex->data, newTex->size);
    //GX_InitTexObj(&texObj, newTex->data, prim->texture.width, prim->texture.height, newTex->format, GX_CLAMP, GX_CLAMP, GX_FALSE);

    //if (PRIMFLAG_GET_SCREENTEX(prim->flags))
    //	GX_InitTexObjLOD(&texObj, GX_NEAR, GX_NEAR, 0.0f, 0.0f, 0.0f, GX_DISABLE, GX_DISABLE, GX_ANISO_1);

    //GX_LoadTexObj(&texObj, GX_TEXMAP0);

    if (prim->texture.base)
        Xe_SetTexture(g_pVideoDevice, 0, newTex->surface);
}

static void clearTexs() {
    gx_tex *t = firstTex;
    gx_tex *n;

    while (t != NULL) {
        n = t->next;
        //free(t->data);
        if (t->surface)
            Xe_DestroyTexture(g_pVideoDevice, t->surface);
        free(t);
        t = n;
    }

    firstTex = NULL;
    lastTex = NULL;
}

static void clearScreenTexs() {
    gx_tex *t = firstScreenTex;
    gx_tex *n;

    while (t != NULL) {
        n = t->next;
        //free(t->data);
//        if (t->surface)
//            Xe_DestroyTexture(g_pVideoDevice, t->surface);
        free(t);
        t = n;
    }

    firstScreenTex = NULL;
    lastScreenTex = NULL;
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawMame(render_primitive * prim) {
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
#if 0
        // bottom left
        Rect[0].x = -width;
        Rect[0].y = -height;
        //        Rect[0].u = prim->texcoords.bl.u;
        //        Rect[0].v = prim->texcoords.bl.v;
        Rect[0].color = color.lcol;

        // bottom right
        Rect[1].x = width;
        Rect[1].y = -height;
        //        Rect[1].u = prim->texcoords.br.u;
        //        Rect[1].v = prim->texcoords.br.v;
        Rect[1].color = color.lcol;

        // top right
        Rect[2].x = width;
        Rect[2].y = height;
        //        Rect[2].u = prim->texcoords.tr.u;
        //        Rect[2].v = prim->texcoords.tr.v;
        Rect[2].color = color.lcol;

        // Top left
        Rect[3].x = -width;
        Rect[3].y = height;
        //        Rect[3].u = prim->texcoords.tl.u;
        //        Rect[3].v = prim->texcoords.tl.u;
        Rect[3].color = color.lcol;

        /**
         * 
         * @param prim
         */

        Rect[0].u = 0;
        Rect[0].v = 0;

        // bottom right
        Rect[1].u = 1;
        Rect[1].v = 0;

        // top right
        Rect[2].u = 1;
        Rect[2].v = 1;

        // Top left
        Rect[3].u = 0;
        Rect[3].v = 1;
#else
        Rect[0].x = prim->bounds.x0 - 0.5f;
        Rect[0].y = prim->bounds.y0 - 0.5f;
        Rect[1].x = prim->bounds.x1 - 0.5f;
        Rect[1].y = prim->bounds.y0 - 0.5f;
        Rect[2].x = prim->bounds.x0 - 0.5f;
        Rect[2].y = prim->bounds.y1 - 0.5f;
        Rect[3].x = prim->bounds.x1 - 0.5f;
        Rect[3].y = prim->bounds.y1 - 0.5f;
        //        Rect[0].x = -width;
        //        Rect[0].y = -height;
        //        Rect[1].x = width;
        //        Rect[1].y = -height;
        //        Rect[2].x = width;
        //        Rect[2].y = height;
        //        Rect[3].x = -width;
        //        Rect[3].y = height;
        // set the texture coordinates
        //if (texture != NULL) {
        if (1) {
            Rect[0].u = prim->texcoords.tl.u;
            Rect[0].v = prim->texcoords.tl.v;
            Rect[1].u = prim->texcoords.tr.u;
            Rect[1].v = prim->texcoords.tr.v;
            Rect[2].u = prim->texcoords.bl.u;
            Rect[2].v = prim->texcoords.bl.v;
            Rect[3].u = prim->texcoords.br.u;
            Rect[3].v = prim->texcoords.br.v;
        }
#endif

        int i = 0;
        for (i = 0; i < 4; i++) {
            Rect[i].z = 0.0;
#ifndef USE_MAME_HLSL
            Rect[i].w = 1.0;
            // offset size ...
            Rect[i].x /= TargetWidth;
            Rect[i].y /= TargetHeight;
            Rect[i].y = 1.0f - Rect[i].y;
            Rect[i].x -= 0.5f;
            Rect[i].y -= 0.5f;

            Rect[i].x *= 2.f;
            Rect[i].y *= 2.f;
#endif
            Rect[i].color = color.lcol;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    //if (texture && texture->d3dtex)
    //    Xe_SetTexture(g_pVideoDevice, 0, texture->d3dtex);
    //else
    //    Xe_SetTexture(g_pVideoDevice, 0, 0);

    //    UpdatesMatrices(x, y, width, height, 0, 1, 1);

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

#ifdef USE_MAME_HLSL
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, (float*) &TargetWidth, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 1, (float*) &TargetHeight, 1);
    Xe_SetVertexShaderConstantF(g_pVideoDevice, 2, (float*) &PostPass, 1);
#endif

    SetRS();

    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);
    //nb_vertices += 4 * sizeof (DrawVerticeFormats);
    nb_vertices += 256; // fixe aligement
}

void MameFrame() {
    clearScreenTexs();
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
#ifdef USE_MAME_HLSL
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
#else

    static const struct XenosVBFFormat vbf = {
        4,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT3},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_COLOR, 1, XE_TYPE_UBYTE4}, //padding
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
        }
    };
    vs_program = (void*) g_xvs_VSmain;
    ps_program = (void*) g_xps_psTC;
#endif

    g_pPixelShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) ps_program);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) vs_program);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);

    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);
}