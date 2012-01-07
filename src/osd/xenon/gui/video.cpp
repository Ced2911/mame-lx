/****************************************************************************
 * libwiigui Template
 * Tantric 2009
 *
 * video.cpp
 * Video routines
 ***************************************************************************/

#include <xetypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <input/input.h>
#include <console/console.h>

#include <xenos/xe.h>
#include <xenos/xenos.h>
#include <xenos/edram.h>
#include <xenos/xenos.h>

#include "Vec.h"

// mame stuff
#include "render.h"

#include <debug.h>

int LoadTextureFromFile(char * pSrcFile, XenosSurface **ppTexture);

typedef unsigned int DWORD;
//#include "ps.h"
//#include "vs.h"
#include "../shaders_hw/vs.h"
#include "../shaders_hw/ps.t.h"
#include "../shaders_hw/ps.c.h"

#include "video.h"

#define DEFAULT_FIFO_SIZE 256 * 1024

int screenheight;
int screenwidth;
u32 FrameTimer = 0;

#define MAX_VERTEX_COUNT 65536

static struct XenosDevice _xe;
static struct XenosVertexBuffer *vb = NULL;
extern struct XenosDevice * g_pVideoDevice;
static struct XenosShader * g_pVertexShader = NULL;
static struct XenosShader * g_pPixelTexturedShader = NULL;
//static struct XenosShader * g_pPixelTexturedColoredShader = NULL;

static struct XenosShader * g_pPixelColoredShader = NULL;

//XenosSurface * g_pTexture;

matrix4x4 modelView2D;
matrix4x4 projection;
//matrix4x4 WVP;

static int nb_vertices = 0;

typedef struct {
    float x, y, z, w; // 32
    unsigned int color; // 36
    unsigned int padding; // 40
    float u, v; // 48
} __attribute__((packed, aligned(32))) DrawVerticeFormats;

// Init Matrices

void InitMatrices() {
    matrix4x4 WVP;
    matrixLoadIdentity(&WVP);

    matrixLoadIdentity(&projection);
    matrixOrthoRH(&projection, 640, 480, 0, 300);

    matrixLoadIdentity(&modelView2D);
    matrixTranslation(&modelView2D, 0, 0, -50.f);
}

/****************************************************************************
 * ResetVideo_Menu
 *
 * Reset the video/rendering mode for the menu
 ****************************************************************************/
void
ResetVideo_Menu() {
    // Init Matrices
    InitMatrices();
}

void CreateVbText(float x, float y, float w, float h, uint32_t color, DrawVerticeFormats * Rect) {
    // bottom left
    Rect[0].x = x - w;
    Rect[0].y = y + h;
    Rect[0].u = 0;
    Rect[0].v = 1;
    Rect[0].color = color;

    // bottom right
    Rect[1].x = x + w;
    Rect[1].y = y + h;
    Rect[1].u = 1;
    Rect[1].v = 1;
    Rect[1].color = color;

    // top right
    Rect[2].x = x + w;
    Rect[2].y = y - h;
    Rect[2].u = 1;
    Rect[2].v = 0;
    Rect[2].color = color;

    // Top left
    Rect[3].x = x - w;
    Rect[3].y = y - h;
    Rect[3].u = 0;
    Rect[3].v = 0;
    Rect[3].color = color;

    int i = 0;
    for (i = 0; i < 4; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }
}

void CreateVb(float x, float y, float w, float h, uint32_t color, DrawVerticeFormats * Rect) {
    // bottom left
    Rect[0].x = x - w;
    Rect[0].y = y - h;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = color;

    // bottom right
    Rect[1].x = x + w;
    Rect[1].y = y - h;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = color;

    // top right
    Rect[2].x = x + w;
    Rect[2].y = y + h;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = color;

    // Top left
    Rect[3].x = x - w;
    Rect[3].y = y + h;
    Rect[3].u = 0;
    Rect[3].v = 1;
    Rect[3].color = color;

    int i = 0;
    for (i = 0; i < 4; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }
}

void CreateVbQuad(float width, float height, uint32_t color, DrawVerticeFormats * Rect) {
    // bottom left
    Rect[0].x = -width;
    Rect[0].y = -height;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = color;

    // bottom right
    Rect[1].x = width;
    Rect[1].y = -height;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = color;

    // top right
    Rect[2].x = width;
    Rect[2].y = height;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = color;

    // Top left
    Rect[3].x = -width;
    Rect[3].y = height;
    Rect[3].u = 0;
    Rect[3].v = 1;
    Rect[3].color = color;

    int i = 0;
    for (i = 0; i < 4; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }
}

void oCreateVbQuad(float width, float height, uint32_t color, DrawVerticeFormats * Rect) {
    // bottom left
    Rect[0].x = -width;
    Rect[0].y = -height;
    Rect[0].u = 0;
    Rect[0].v = 0;
    Rect[0].color = color;

    // bottom right
    Rect[1].x = width;
    Rect[1].y = -height;
    Rect[1].u = 1;
    Rect[1].v = 0;
    Rect[1].color = color;

    // top right
    Rect[2].x = width;
    Rect[2].y = height;
    Rect[2].u = 1;
    Rect[2].v = 1;
    Rect[2].color = color;

    // Top left
    Rect[3].x = -width;
    Rect[3].y = height;
    Rect[3].u = 0;
    Rect[3].v = 1;
    Rect[3].color = color;

    int i = 0;
    for (i = 0; i < 4; i++) {
        Rect[i].z = 0.0;
        Rect[i].w = 1.0;
    }
}

extern "C" struct XenosDevice * GetVideoDevice() {
    return g_pVideoDevice;
}

/****************************************************************************
 * InitVideo
 *
 * This function MUST be called at startup.
 * - also sets up menu video mode
 ***************************************************************************/
void
InitVideo() {
    TR;
    //xenos_init(VIDEO_MODE_AUTO);
    //console_init();

    //g_pVideoDevice = &_xe;

    //Xe_Init(g_pVideoDevice);

    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);

    screenheight = fb->height;
    screenwidth = fb->width;

    //    screenheight = 480;
    //    screenwidth = 640;

    Xe_Init(g_pVideoDevice);

    Xe_SetRenderTarget(g_pVideoDevice, Xe_GetFramebufferSurface(g_pVideoDevice));

    static const struct XenosVBFFormat vbf = {
        4,
        {
            {XE_USAGE_POSITION, 0, XE_TYPE_FLOAT4},
            {XE_USAGE_COLOR, 0, XE_TYPE_UBYTE4},
            {XE_USAGE_COLOR, 1, XE_TYPE_UBYTE4}, //padding
            {XE_USAGE_TEXCOORD, 0, XE_TYPE_FLOAT2},
        }
    };

    g_pPixelTexturedShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_psT);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelTexturedShader, 0);

    g_pPixelColoredShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_psC);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelColoredShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xvs_VSmain);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);

    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);

    edram_init(g_pVideoDevice);

    vb = Xe_CreateVertexBuffer(g_pVideoDevice, MAX_VERTEX_COUNT * sizeof (DrawVerticeFormats));

    Xe_SetClearColor(g_pVideoDevice, 0xFF888888);

    //    LoadTextureFromFile("uda:/1.png", &g_pTexture);

    Xe_InvalidateState(g_pVideoDevice);

    ResetVideo_Menu();
}

/****************************************************************************
 * StopGX
 *
 * Stops GX (when exiting)
 ***************************************************************************/
void StopGX() {
    //    GX_AbortFrame();
    //    GX_Flush();
    //
    //    VIDEO_SetBlack(TRUE);
    //    VIDEO_Flush();
}

extern "C" void doScreenCapture();

/****************************************************************************
 * Menu_Render
 *
 * Renders everything current sent to GX, and flushes video
 ***************************************************************************/
void Menu_Render() {

    FrameTimer++;

    Xe_Resolve(g_pVideoDevice);

    while (!Xe_IsVBlank(g_pVideoDevice));

    Xe_Sync(g_pVideoDevice);

    //doScreenCapture();

    Xe_InvalidateState(g_pVideoDevice);

    nb_vertices = 0;


}

void SetRS() {
    Xe_SetBlendOp(g_pVideoDevice, XE_BLENDOP_ADD);
    Xe_SetSrcBlend(g_pVideoDevice, XE_BLEND_SRCALPHA);
    Xe_SetDestBlend(g_pVideoDevice, XE_BLEND_INVSRCALPHA);
    Xe_SetAlphaTestEnable(g_pVideoDevice, 1);

    Xe_SetCullMode(g_pVideoDevice, XE_CULL_NONE);
    Xe_SetStreamSource(g_pVideoDevice, 0, vb, nb_vertices, sizeof (DrawVerticeFormats));
}

void Draw() {
    SetRS();

    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_RECTLIST, 0, 1);
    //nb_vertices += 4 * sizeof (DrawVerticeFormats);
    nb_vertices += 256; // fixe aligement
}

void UpdatesMatrices(f32 xpos, f32 ypos, f32 width, f32 height, f32 degrees, f32 scaleX, f32 scaleY) {
#define DegToRad(a)   ( (a) *  0.01745329252f )

    matrix4x4 m;
    matrix4x4 rotation;
    matrix4x4 scale;
    matrix4x4 translation;
    matrix4x4 WVP;

    matrixLoadIdentity(&WVP);
    matrixLoadIdentity(&m);
    matrixLoadIdentity(&rotation);
    matrixLoadIdentity(&scale);
    matrixLoadIdentity(&translation);

    matrixRotationZ(&rotation, DegToRad(degrees));
    matrixTranslation(&translation, xpos + width, ypos + height, 0);
    matrixScaling(&scale, scaleX, scaleY, 1.0f);

    //    // scale => rotate => translate
    matrixMultiply(&m, &scale, &rotation);
    matrixMultiply(&WVP, &m, &translation);

    Xe_SetVertexShaderConstantF(g_pVideoDevice, 0, (float*) &WVP, 4);
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, XenosSurface * data,
        f32 degrees, f32 scaleX, f32 scaleY, u8 alpha) {

    if (scaleX != 1)
        printf("ScaleX = %f\r\n", scaleX);


    if (scaleY != 1)
        printf("scaleY = %f\r\n", scaleY);

    if (data == NULL)
        return;

    //    width >>= 1;
    //    height >>= 1;

    float x, y, w, h;

    x = (float) xpos;
    y = (float) ypos;
    w = (float) width;
    h = (float) height;

    x = (x / ((float) screenwidth / 2.f)) - 1.f; // 1280/2
    y = (y / ((float) screenheight / 2.f)) - 1.f; // 720/2

    //y = -y;

    w = (float) w / ((float) screenwidth);
    h = (float) h / ((float) screenheight);


    //    printf("x = %f\r\n", x);
    //    printf("y = %f\r\n", y);
    //    printf("w = %f\r\n", w);
    //    printf("h = %f\r\n", h);
    //    printf("\r\n");

    XeColor color;

    color.a = alpha;
    color.r = 0xFF;
    color.g = 0xFF;
    color.b = 0xFF;

    DrawVerticeFormats* Rect = (DrawVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // CreateVb(x,y,w*scaleX,h*scaleY,color.lcol,Rect);
        CreateVbQuad(w, h, color.lcol, Rect);
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetTexture(g_pVideoDevice, 0, data);

    UpdatesMatrices(x, y, w, h, degrees, scaleX, scaleY);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    Draw();
}

/****************************************************************************
 * Menu_DrawRectangle
 *
 * Draws a rectangle at the specified coordinates using GX
 ***************************************************************************/
void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, XeColor color, u8 filled) {

    float w, h;

    x = (float) x;
    y = (float) y;
    w = (float) width;
    h = (float) height;

    x = (x / ((float) screenwidth / 2.f)) - 1.f; // 1280/2
    y = (y / ((float) screenheight / 2.f)) - 1.f; // 720/2

    w = (float) w / ((float) screenwidth);
    h = (float) h / ((float) screenheight);


    DrawVerticeFormats* Rect = (DrawVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        CreateVbQuad(w, h, color.lcol, Rect);
        //CreateVb(x,y,w,h,_color.lcol,Rect);
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetTexture(g_pVideoDevice, 0, NULL);

    UpdatesMatrices(x, y, w, h, 0, 1, 1);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelColoredShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    Draw();
}

void Menu_T(XenosSurface * surf, f32 texWidth, f32 texHeight, int16_t screenX, int16_t screenY, XeColor color) {
    //return;
    float x, y, w, h;
    if (surf == NULL)
        return;

    x = (float) screenX;
    y = (float) screenY;
    w = (float) texWidth;
    h = (float) texHeight;

    x = (x / ((float) screenwidth / 2.f)) - 1.f; // 1280/2
    y = (y / ((float) screenheight / 2.f)) - 1.f; // 720/2

    w = (float) w / ((float) screenwidth);
    h = (float) h / ((float) screenheight);

    // Correct aspect ratio
    //    h = h * ((float) screenwidth/(float) screenheight);

    //    w = w/2;
    //    w = h/2;

    DrawVerticeFormats* Rect = (DrawVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        CreateVbText(0, 0, w, h, color.lcol, Rect);
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    Xe_SetTexture(g_pVideoDevice, 0, surf);

    UpdatesMatrices(x, y, w, h, 0, 1, 1);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    Draw();
}

/**
 *
 * Mame stuff 
 * 
 * 
 */
/* d3d_texture_info holds information about a texture */
typedef struct _d3d_texture_info d3d_texture_info;

struct _d3d_texture_info {
    d3d_texture_info * next; // next texture in the list
    d3d_texture_info * prev; // prev texture in the list
    UINT32 hash; // hash value for the texture
    UINT32 flags; // rendering flags
    render_texinfo texinfo; // copy of the texture info
    float ustart, ustop; // beginning/ending U coordinates
    float vstart, vstop; // beginning/ending V coordinates
    int rawwidth, rawheight; // raw width/height of the texture
    int type; // what type of texture are we?
    int xborderpix; // number of border pixels in X
    int yborderpix; // number of border pixels in Y
    int xprescale; // what is our X prescale factor?
    int yprescale; // what is our Y prescale factor?
    int cur_frame; // what is our current frame?
    int prev_frame; // what was our last frame? (used to determine pause state)
    struct XenosSurface * d3dtex; // Direct3D texture pointer
    struct XenosSurface * d3dsurface; // Direct3D offscreen plain surface pointer
    struct XenosSurface * d3dfinaltex; // Direct3D final (post-scaled) texture
    int target_index; // Direct3D target index
};

d3d_texture_info * texlist;

// textures
static void texture_compute_size(int texwidth, int texheight, d3d_texture_info *texture);
static void texture_set_data(d3d_texture_info *texture, const render_texinfo *texsource, UINT32 flags);
static void texture_prescale(d3d_texture_info *texture);
static d3d_texture_info *texture_find(const render_primitive *prim);
static void texture_update(const render_primitive *prim);
static inline UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags);

static inline XenosSurface * xe_create_tex(int w, int h, int fmt) {
    printf("CreateTexture(%d,%d)\r\n",w,h);
    return Xe_CreateTexture(g_pVideoDevice, w, h, 0, fmt, 0);
}

//============================================================
//  texture_create
//============================================================

d3d_texture_info *texture_create(const render_texinfo *texsource, UINT32 flags) {
    d3d_texture_info *texture;

    // allocate a new texture
    texture = global_alloc_clear(d3d_texture_info);

    // fill in the core data
    texture->hash = texture_compute_hash(texsource, flags);
    texture->flags = flags;
    texture->texinfo = *texsource;
    texture->xprescale = 0;
    texture->yprescale = 0;

    // compute the size
    texture_compute_size(texsource->width, texsource->height, texture);

    // non-screen textures are easy
    if (!PRIMFLAG_GET_SCREENTEX(flags)) {
        assert(PRIMFLAG_TEXFORMAT(flags) != TEXFORMAT_YUY16);
        texture->d3dtex = xe_create_tex(texture->rawwidth, texture->rawheight, XE_FMT_8888 | XE_FMT_ARGB);
        TR;
        texture->d3dfinaltex = texture->d3dtex;
        //texture->type = TEXTURE_TYPE_PLAIN;
    }// screen textures are allocated differently
    else {
        unsigned int format;
        //DWORD usage = d3d->dynamic_supported ? D3DUSAGE_DYNAMIC : 0;
        //D3DPOOL pool = d3d->dynamic_supported ? D3DPOOL_DEFAULT : D3DPOOL_MANAGED;
        //int maxdim = MAX(d3d->presentation.BackBufferWidth, d3d->presentation.BackBufferHeight);
        int attempt;

        // pick the format
        if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_YUY16) {
            //format = d3d->yuv_format;
            TR;
        } else if (PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_ARGB32 || PRIMFLAG_GET_TEXFORMAT(flags) == TEXFORMAT_PALETTEA16) {
            format = XE_FMT_8888;
        } else {
            TR;
            //format = d3d->screen_format;
            format = XE_FMT_8888;
        }

        // loop until we allocate something or error
        for (attempt = 0; attempt < 2; attempt++) {
            // second attempt is always 1:1
            if (attempt == 1)
                texture->xprescale = texture->yprescale = 1;

            // screen textures with no prescaling are pretty easy
            if (texture->xprescale == 1 && texture->yprescale == 1) {
                TR;
                //result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, usage, format, pool, &texture->d3dtex);
                texture->d3dtex = xe_create_tex(texture->rawwidth, texture->rawheight, format | XE_FMT_ARGB);
                //if (result == D3D_OK)
                if (texture->d3dtex) {
                    texture->d3dfinaltex = texture->d3dtex;
                    //texture->type = d3d->dynamic_supported ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;
                    break;
                }
            }
#if 0
                // screen textures with prescaling require two allocations
            else {
                int scwidth, scheight;
                D3DFORMAT finalfmt;

                // use an offscreen plain surface for stretching if supported
                // (won't work for YUY textures)
                if (d3d->stretch_supported && PRIMFLAG_GET_TEXFORMAT(flags) != TEXFORMAT_YUY16) {
                    result = (*d3dintf->device.create_offscreen_plain_surface)(d3d->device, texture->rawwidth, texture->rawheight, format, D3DPOOL_DEFAULT, &texture->d3dsurface);
                    if (result != D3D_OK)
                        continue;
                    texture->type = TEXTURE_TYPE_SURFACE;
                }// otherwise, we allocate a dynamic texture for the source
                else {
                    result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, usage, format, pool, &texture->d3dtex);
                    if (result != D3D_OK)
                        continue;
                    texture->type = d3d->dynamic_supported ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;
                }

                // for the target surface, we allocate a render target texture
                scwidth = texture->rawwidth * texture->xprescale;
                scheight = texture->rawheight * texture->yprescale;

                // target surfaces typically cannot be YCbCr, so we always pick RGB in that case
                finalfmt = (format != d3d->yuv_format) ? format : D3DFMT_A8R8G8B8;
                result = (*d3dintf->device.create_texture)(d3d->device, scwidth, scheight, 1, D3DUSAGE_RENDERTARGET, finalfmt, D3DPOOL_DEFAULT, &texture->d3dfinaltex);
                if (result == D3D_OK) {
                    int ret = d3d->hlsl->register_prescaled_texture(texture, scwidth, scheight);
                    if (ret != 0)
                        goto error;

                    break;
                }
                (*d3dintf->texture.release)(texture->d3dtex);
                texture->d3dtex = NULL;
            }
#endif                        
        }
    }

    // copy the data to the texture
    texture_set_data(texture, texsource, flags);

    // add us to the texture list
    if (texlist != NULL)
        texlist->prev = texture;
    texture->prev = NULL;
    texture->next = texlist;
    texlist = texture;
    return texture;

error:
    return NULL;
}


//============================================================
//  texture_compute_size
//============================================================
#define ENABLE_BORDER_PIX	(1)

static void texture_compute_size(int texwidth, int texheight, d3d_texture_info *texture) {
    int finalheight = texheight;
    int finalwidth = texwidth;

    // if we're not wrapping, add a 1-2 pixel border on all sides
    texture->xborderpix = 0;
    texture->yborderpix = 0;
    if (ENABLE_BORDER_PIX && !(texture->flags & PRIMFLAG_TEXWRAP_MASK)) {
        // note we need 2 pixels in X for YUY textures
        texture->xborderpix = (PRIMFLAG_GET_TEXFORMAT(texture->flags) == TEXFORMAT_YUY16) ? 2 : 1;
        texture->yborderpix = 1;
    }

    // compute final texture size
    finalwidth += 2 * texture->xborderpix;
    finalheight += 2 * texture->yborderpix;
#if 0
    // round width/height up to nearest power of 2 if we need to
    //if (!(d3d->texture_caps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
    if (0) {
        // first the width
        if (finalwidth & (finalwidth - 1)) {
            finalwidth |= finalwidth >> 1;
            finalwidth |= finalwidth >> 2;
            finalwidth |= finalwidth >> 4;
            finalwidth |= finalwidth >> 8;
            finalwidth++;
        }

        // then the height
        if (finalheight & (finalheight - 1)) {
            finalheight |= finalheight >> 1;
            finalheight |= finalheight >> 2;
            finalheight |= finalheight >> 4;
            finalheight |= finalheight >> 8;
            finalheight++;
        }
    }

    // round up to square if we need to
    //if (d3d->texture_caps & D3DPTEXTURECAPS_SQUAREONLY)
    if (0) {
        if (finalwidth < finalheight)
            finalwidth = finalheight;
        else
            finalheight = finalwidth;
    }

    // adjust the aspect ratio if we need to
    while (finalwidth < finalheight && finalheight / finalwidth > d3d->texture_max_aspect)
        finalwidth *= 2;
    while (finalheight < finalwidth && finalwidth / finalheight > d3d->texture_max_aspect)
        finalheight *= 2;

    // if we added pixels for the border, and that just barely pushed us over, take it back
    if ((finalwidth > d3d->texture_max_width && finalwidth - 2 * texture->xborderpix <= d3d->texture_max_width) ||
            (finalheight > d3d->texture_max_height && finalheight - 2 * texture->yborderpix <= d3d->texture_max_height)) {
        finalwidth -= 2 * texture->xborderpix;
        finalheight -= 2 * texture->yborderpix;
        texture->xborderpix = 0;
        texture->yborderpix = 0;
    }

    // if we're above the max width/height, do what?
    if (finalwidth > d3d->texture_max_width || finalheight > d3d->texture_max_height) {
        static int printed = FALSE;
        if (!printed) mame_printf_warning("Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth, finalheight, (int) d3d->texture_max_width, (int) d3d->texture_max_height);
        printed = TRUE;
    }
#endif
    // compute the U/V scale factors
    texture->ustart = (float) texture->xborderpix / (float) finalwidth;
    texture->ustop = (float) (texwidth + texture->xborderpix) / (float) finalwidth;
    texture->vstart = (float) texture->yborderpix / (float) finalheight;
    texture->vstop = (float) (texheight + texture->yborderpix) / (float) finalheight;

    // set the final values
    texture->rawwidth = finalwidth;
    texture->rawheight = finalheight;
}

//============================================================
//  texture_set_data
//============================================================
#include "blit.inl.h"

static void _texture_set_data(d3d_texture_info *texture, const render_texinfo *texsource, UINT32 flags) {
    int miny, maxy;
    int dsty;
    unsigned char * pBits = NULL;
    unsigned int pitch = texture->d3dtex->wpitch;

    // lock the texture
    pBits = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, texture->d3dtex, 0, 0, 0, 0, XE_LOCK_WRITE);


    // loop over Y
    miny = 0 - texture->yborderpix;
    maxy = texsource->height + texture->yborderpix;
    for (dsty = miny; dsty < maxy; dsty++) {
        int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;
        void *dst = (unsigned char *) pBits + (dsty + texture->yborderpix) * pitch;

        // switch off of the format and
        switch (PRIMFLAG_GET_TEXFORMAT(flags)) {
            case TEXFORMAT_PALETTE16:
                TR;
                copyline_palette16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_PALETTEA16:
                TR;
                copyline_palettea16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_RGB15:
                TR;
                copyline_rgb15((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_RGB32:
                TR;
                copyline_rgb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_ARGB32:
                TR;
                copyline_argb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_YUY16:
                TR;
#if 0
                if (d3d->yuv_format == D3DFMT_YUY2)
                    copyline_yuy16_to_yuy2((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                else if (d3d->yuv_format == D3DFMT_UYVY)
                    copyline_yuy16_to_uyvy((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                else
                    copyline_yuy16_to_argb((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;
#endif
            default:
                TR;
                //mame_printf_error("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
                break;
        }
    }

    // unlock
    Xe_Surface_Unlock(g_pVideoDevice, texture->d3dtex);
    // prescale
    //texture_prescale(d3d, texture);
}

static void texture_set_data(d3d_texture_info *texture, const render_texinfo *texsource, UINT32 flags) {
    int miny, maxy;
    int dsty;
    unsigned char * pBits = NULL;
    unsigned int pitch = texture->d3dtex->wpitch;
    unsigned int wpitch = texture->d3dtex->wpitch;

    unsigned char * src = (unsigned char *) texsource->base;
    unsigned char * dst = (unsigned char *) pBits;

    // lock the texture
    pBits = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, texture->d3dtex, 0, 0, 0, 0, XE_LOCK_WRITE);


    // loop over Y
    miny = 0 - texture->yborderpix;
    maxy = texsource->height + texture->yborderpix;
#if 0
    for (dsty = miny; dsty < maxy; dsty++) {
        int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;
        void *dst = (unsigned char *) pBits + (dsty + texture->yborderpix) * pitch;

        // switch off of the format and
        switch (PRIMFLAG_GET_TEXFORMAT(flags)) {
            case TEXFORMAT_PALETTE16:
                //TR;
                copyline_palette16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_PALETTEA16:
                //TR;
                copyline_palettea16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_RGB15:
               // TR;
                copyline_rgb15((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_RGB32:
                //TR;
                copyline_rgb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_ARGB32:
                //TR;
                copyline_argb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            case TEXFORMAT_YUY16:
                //TR;
#if 0
                if (d3d->yuv_format == D3DFMT_YUY2)
                    copyline_yuy16_to_yuy2((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                else if (d3d->yuv_format == D3DFMT_UYVY)
                    copyline_yuy16_to_uyvy((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                else
#endif
                    copyline_yuy16_to_argb((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                break;

            default:
                TR;
                //mame_printf_error("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
                break;
        }
    }
#elif 0
    uint8_t * dst_limit = (uint8_t *) pBits + ((texture->d3dtex->hpitch) * (texture->d3dtex->wpitch));

    for (int hpitch = 0; hpitch < texture->d3dtex->hpitch; hpitch += texture->d3dtex->height) {
        //        for (int y = 0; y < bmp->rows; y++)
        int y, dsty = 0;
        //        int y_offset = charData->glyphDataTexture->height;
        int y_offset = 0;

        for (y = 0, dsty = y_offset; y < (texsource->height); y++, dsty++) {
            //        for (y = 0, dsty = y_offset; y < (bmp->rows); y++, dsty--) {
            for (wpitch = 0; wpitch < texture->d3dtex->wpitch; wpitch += texture->d3dtex->width) {
                src = (uint8_t *) texsource->base + ((y) * texsource->rowpixels);
                dst = (uint8_t *) pBits + ((dsty + hpitch) * (texture->d3dtex->wpitch)) + wpitch;
                for (int x = 0; x < texsource->width; x++) {
                    if (dst < dst_limit)
                        *dst++ = *src++;
                }
            }
        }
    }
#else
    for(int hpitch = 0; hpitch< texture->d3dtex->hpitch; hpitch += texture->d3dtex->height )
    {
        for (dsty = miny; dsty < maxy; dsty++) {
            int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;
            void *dst = (unsigned char *) pBits + (hpitch + dsty + texture->yborderpix) * pitch;

            // switch off of the format and
            switch (PRIMFLAG_GET_TEXFORMAT(flags)) {
                case TEXFORMAT_PALETTE16:
                    //TR;
                    copyline_palette16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                case TEXFORMAT_PALETTEA16:
                    //TR;
                    copyline_palettea16((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                case TEXFORMAT_RGB15:
                   // TR;
                    copyline_rgb15((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                case TEXFORMAT_RGB32:
                    //TR;
                    copyline_rgb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                case TEXFORMAT_ARGB32:
                    //TR;
                    copyline_argb32((UINT32 *) dst, (UINT32 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                case TEXFORMAT_YUY16:
                    //TR;
#if 0
                    if (d3d->yuv_format == D3DFMT_YUY2)
                        copyline_yuy16_to_yuy2((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    else if (d3d->yuv_format == D3DFMT_UYVY)
                        copyline_yuy16_to_uyvy((UINT16 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    else
#endif
                        copyline_yuy16_to_argb((UINT32 *) dst, (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);
                    break;

                default:
                    TR;
                    //mame_printf_error("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
                    break;
            }
        }
    }
#endif
    // unlock
    Xe_Surface_Unlock(g_pVideoDevice, texture->d3dtex);
    // prescale
    //texture_prescale(d3d, texture);
}

UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags) {
    return (FPTR) texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}

//============================================================
//  texture_find
//============================================================

static d3d_texture_info *texture_find(const render_primitive *prim) {
    UINT32 texhash = texture_compute_hash(&prim->texture, prim->flags);
    d3d_texture_info *texture;

    // find a match
    for (texture = texlist; texture != NULL; texture = texture->next)
        if (texture->hash == texhash &&
                texture->texinfo.base == prim->texture.base &&
                texture->texinfo.width == prim->texture.width &&
                texture->texinfo.height == prim->texture.height &&
                ((texture->flags ^ prim->flags) & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK)) == 0)
            return texture;

    // nothing found
    return NULL;
}

//============================================================
//  texture_update
//============================================================

static void texture_update(const render_primitive *prim) {
    d3d_texture_info *texture = texture_find(prim);

    // if we didn't find one, create a new texture
    if (texture == NULL) {
        TR;
        texture = texture_create(&prim->texture, prim->flags);
    }

    // if we found it, but with a different seqid, copy the data
    if (texture->texinfo.seqid != prim->texture.seqid) {
        texture_set_data(texture, &prim->texture, prim->flags);
        texture->texinfo.seqid = prim->texture.seqid;
    }
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawMame(render_primitive *prim) {
    float x = prim->bounds.x0;
    float y = prim->bounds.y0;

    float width = prim->bounds.x1 - x;
    float height = prim->bounds.y1 - y;

    x = (x / ((float) screenwidth / 2.f)) - 1.f; // 1280/2
    y = (y / ((float) screenheight / 2.f)) - 1.f; // 720/2

    width = (float) width / ((float) screenwidth);
    height = (float) height / ((float) screenheight);

    XeColor color;

    color.a = prim->color.a * 255.f;
    color.r = prim->color.r * 255.f;
    color.g = prim->color.g * 255.f;
    color.b = prim->color.b * 255.f;

    texture_update(prim);
    d3d_texture_info *texture = texture_find(prim);

    DrawVerticeFormats* Rect = (DrawVerticeFormats*) Xe_VB_Lock(g_pVideoDevice, vb, nb_vertices, 4 * sizeof (DrawVerticeFormats), XE_LOCK_WRITE);
    {
        // bottom left
        Rect[0].x = -width;
        Rect[0].y = -height;
        Rect[0].u = prim->texcoords.bl.u;
        Rect[0].v = prim->texcoords.bl.v;
        Rect[0].color = color.lcol;

        // bottom right
        Rect[1].x = width;
        Rect[1].y = -height;
        Rect[1].u = prim->texcoords.br.u;
        Rect[1].v = prim->texcoords.br.v;
        Rect[1].color = color.lcol;

        // top right
        Rect[2].x = width;
        Rect[2].y = height;
        Rect[2].u = prim->texcoords.tr.u;
        Rect[2].v = prim->texcoords.tr.v;
        Rect[2].color = color.lcol;

        // Top left
        Rect[3].x = -width;
        Rect[3].y = height;
        Rect[3].u = prim->texcoords.tl.u;
        Rect[3].v = prim->texcoords.tl.u;
        Rect[3].color = color.lcol;

        int i = 0;
        for (i = 0; i < 4; i++) {
            Rect[i].z = 0.0;
            Rect[i].w = 1.0;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    if (texture->d3dtex == NULL)
        TR;
    Xe_SetTexture(g_pVideoDevice, 0, texture->d3dtex);

    UpdatesMatrices(x, y, width, height, 0, 1, 1);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelTexturedShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    Draw();
}