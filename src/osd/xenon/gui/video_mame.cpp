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

typedef struct {
    float x, y, z; // 12
    unsigned int color; // 16
    float u, v; // 24
    /** padding **/
    float p1,p2; // 32
} __attribute__((packed, aligned(32))) MameVerticeFormats;

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
static d3d_texture_info * texture_update(const render_primitive *prim);
static inline UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags);

static inline XenosSurface * xe_create_tex(int w, int h, int fmt) {
    if(w<32)
        w=32;
    if(h<128)
        h=128;
    
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
                //TR;
                //result = (*d3dintf->device.create_texture)(d3d->device, texture->rawwidth, texture->rawheight, 1, usage, format, pool, &texture->d3dtex);
                texture->d3dtex = xe_create_tex(texture->rawwidth, texture->rawheight, format | XE_FMT_ARGB);
                //if (result == D3D_OK)
                if (texture->d3dtex) {
                    texture->d3dfinaltex = texture->d3dtex;
                    //texture->type = d3d->dynamic_supported ? TEXTURE_TYPE_DYNAMIC : TEXTURE_TYPE_PLAIN;
                    break;
                }
            }
            else{
                TR;
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

    // round width/height up to nearest power of 2 if we need to
    //if (!(d3d->texture_caps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
    if (1) {
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
    if (1) {
        if (finalwidth < finalheight)
            finalwidth = finalheight;
        else
            finalheight = finalwidth;
    }
    // if we added pixels for the border, and that just barely pushed us over, take it back
    int texture_max_width = 4096;
    int texture_max_height = 4096;
    if ((finalwidth > texture_max_width && finalwidth - 2 * texture->xborderpix <= texture_max_width) ||
            (finalheight > texture_max_height && finalheight - 2 * texture->yborderpix <= texture_max_height)) {
        finalwidth -= 2 * texture->xborderpix;
        finalheight -= 2 * texture->yborderpix;
        texture->xborderpix = 0;
        texture->yborderpix = 0;
    }

    // if we're above the max width/height, do what?
    if (finalwidth > texture_max_width || finalheight > texture_max_height) {
        static int printed = FALSE;
        if (!printed) printf("Texture too big! (wanted: %dx%d, max is %dx%d)\n", finalwidth, finalheight, (int) texture_max_width, (int) texture_max_height);
        printed = TRUE;
    }

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

static void texture_set_data(d3d_texture_info *texture, const render_texinfo *texsource, UINT32 flags) {
    int miny, maxy;
    int dsty;
    unsigned char * pBits = NULL;
    unsigned int pitch = texture->d3dtex->wpitch;
    unsigned int wpitch = texture->d3dtex->wpitch;

    unsigned char * src = (unsigned char *) texsource->base;
    unsigned char * dst = (unsigned char *) pBits;

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
    typedef void (*mame_32_blitter)(UINT32 *, const UINT32 *, int, const rgb_t *, int);
    mame_32_blitter func = NULL;

    // switch off of the format and
    switch (PRIMFLAG_GET_TEXFORMAT(flags)) {

        case TEXFORMAT_PALETTE16:
            TR;
            func = (mame_32_blitter) copyline_palette16;
            break;

        case TEXFORMAT_PALETTEA16:
            TR;
            func = (mame_32_blitter) copyline_palettea16;
            break;

        case TEXFORMAT_RGB15:
            TR;
            func = (mame_32_blitter) copyline_rgb15;
            break;

        case TEXFORMAT_RGB32:
            TR;
            func = (mame_32_blitter) copyline_rgb32;
            break;


        case TEXFORMAT_ARGB32:
        {
            //TR;
            //func = (mame_32_blitter) copyline_argb32;
            func = NULL;

            // XeTexSubImage(texture->d3dtex, 4, 4, 0, 0, texsource->width, texsource->height, texsource->base);

            unsigned char* surfbuf = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, texture->d3dtex, 0, 0, 0, 0, XE_LOCK_WRITE);

            int hpitch, wpitch;

            unsigned int* txdata = (unsigned int*) surfbuf;

            for (hpitch = 0; hpitch < texture->d3dtex->hpitch; hpitch += texture->d3dtex->height) {
                for (wpitch = 0; wpitch < texture->d3dtex->wpitch; wpitch += texture->d3dtex->width) {
                    txdata[0] = 0xFF1F7FFF;
                    *txdata++;
                }
            }
#if 0
            for (hpitch = 0; hpitch < texture->d3dtex->hpitch; hpitch += texture->d3dtex->height) {
                //        for (int y = 0; y < bmp->rows; y++)
                int y, dsty = 0;
                //        int y_offset = charData->glyphDataTexture->height;
                int y_offset = 0;

                for (y = 0, dsty = y_offset; y < (texsource->height); y++, dsty++) {
                    //        for (y = 0, dsty = y_offset; y < (bmp->rows); y++, dsty--) {
                    for (wpitch = 0; wpitch < texture->d3dtex->wpitch; wpitch += texture->d3dtex->width) {
                        src = (uint8_t *) texsource->base + ((y) * texsource->rowpixels);
                        dst = (uint8_t *) surfbuf + ((dsty + hpitch) * (texture->d3dtex->wpitch)) + wpitch;

                        for (int x = 0; x < texsource->width; x++) {
                            //if (dst < dst_limit)
                            //*dst++ = *src++;
                            // 32bits
                            /*
                            dst[0] = src[0];
                            dst[1] = src[1];
                            dst[2] = src[2];
                            dst[3] = src[3];
                             */


                            dst[0] = 0xFF;
                            dst[1] = 0x1F;
                            dst[2] = 0x7F;
                            dst[3] = 0xFF;

                            *dst += 4;
                            *src += 4;
                        }

                    }
                }

            }
#endif
            Xe_Surface_Unlock(g_pVideoDevice, texture->d3dtex);

            break;
        }

        case TEXFORMAT_YUY16:
            func = (mame_32_blitter) copyline_yuy16_to_argb;
            break;

        default:
            TR;
            printf("Unknown texture blendmode=%d format=%d\n", PRIMFLAG_GET_BLENDMODE(flags), PRIMFLAG_GET_TEXFORMAT(flags));
            //BP;
            func = NULL;
            break;
    }

    if (func) {
        // lock the texture
        pBits = (unsigned char*) Xe_Surface_LockRect(g_pVideoDevice, texture->d3dtex, 0, 0, 0, 0, XE_LOCK_WRITE);

        for (int hpitch = 0; hpitch < texture->d3dtex->hpitch; hpitch += texture->d3dtex->height) {
            for (dsty = miny; dsty < maxy; dsty++) {
                int srcy = (dsty < 0) ? 0 : (dsty >= texsource->height) ? texsource->height - 1 : dsty;
                void *dst = (unsigned char *) pBits + (hpitch + dsty + texture->yborderpix) * pitch;

                func((UINT32 *) dst, (const UINT32 *) (UINT16 *) texsource->base + srcy * texsource->rowpixels, texsource->width, texsource->palette, texture->xborderpix);

            }
        }
        // unlock
        Xe_Surface_Unlock(g_pVideoDevice, texture->d3dtex);
    }
#endif


    // prescale
    //texture_prescale(d3d, texture);
}

UINT32 texture_compute_hash(const render_texinfo *texture, UINT32 flags) {
    return (FPTR) texture->base ^ (flags & (PRIMFLAG_BLENDMODE_MASK | PRIMFLAG_TEXFORMAT_MASK));
}

//============================================================
//  texture_find
//============================================================

static d3d_texture_info * texture_find(const render_primitive * prim) {
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

static d3d_texture_info * texture_update(const render_primitive * prim) {
    d3d_texture_info *texture = texture_find(prim);

    // if we didn't find one, create a new texture
    if (texture == NULL) {
        TR;
        texture = texture_create(&prim->texture, prim->flags);
    } else
        if (texture->texinfo.seqid != prim->texture.seqid) {
        //        texture_set_data(texture, &prim->texture, prim->flags);
        texture->texinfo.seqid = prim->texture.seqid;
    }

    texture_set_data(texture, &prim->texture, prim->flags);

    return texture;
}

/****************************************************************************
 * Menu_DrawImg
 *
 * Draws the specified image on screen using GX
 ***************************************************************************/
void Menu_DrawMame(render_primitive * prim) {
    XeColor color;

    color.a = prim->color.a * 255.f;
    color.r = prim->color.r * 255.f;
    color.g = prim->color.g * 255.f;
    color.b = prim->color.b * 255.f;

    d3d_texture_info *texture = texture_update(prim);

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
        if (texture != NULL) {
            float du = texture->ustop - texture->ustart;
            float dv = texture->vstop - texture->vstart;
            Rect[0].u = texture->ustart + du * prim->texcoords.tl.u;
            Rect[0].v = texture->vstart + dv * prim->texcoords.tl.v;
            Rect[1].u = texture->ustart + du * prim->texcoords.tr.u;
            Rect[1].v = texture->vstart + dv * prim->texcoords.tr.v;
            Rect[2].u = texture->ustart + du * prim->texcoords.bl.u;
            Rect[2].v = texture->vstart + dv * prim->texcoords.bl.v;
            Rect[3].u = texture->ustart + du * prim->texcoords.br.u;
            Rect[3].v = texture->vstart + dv * prim->texcoords.br.v;
        }
#endif

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

        int i = 0;
        for (i = 0; i < 4; i++) {
            Rect[i].z = 0.0;
            //Rect[i].w = 1.0;
            Rect[i].color = color.lcol;
        }
    }
    Xe_VB_Unlock(g_pVideoDevice, vb);

    if (texture && texture->d3dtex)
        Xe_SetTexture(g_pVideoDevice, 0, texture->d3dtex);
    else
        Xe_SetTexture(g_pVideoDevice, 0, 0);

    //    UpdatesMatrices(x, y, width, height, 0, 1, 1);

    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_PIXEL, g_pPixelShader, 0);
    Xe_SetShader(g_pVideoDevice, SHADER_TYPE_VERTEX, g_pVertexShader, 0);

    /* from shader */
    XenosSurface * fb = Xe_GetFramebufferSurface(g_pVideoDevice);
    float TargetWidth = fb->width;
    float TargetHeight = fb->height;

    float RawWidth = texture->rawwidth;
    float RawHeight = texture->rawheight;

    float PostPass = 0.f;

    float WidthRatio = texture->ustop - texture->ustart;
    float HeightRatio = texture->vstop - texture->vstart;

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

    SetRS();

    Xe_DrawPrimitive(g_pVideoDevice, XE_PRIMTYPE_TRIANGLESTRIP, 0, 2);
    //nb_vertices += 4 * sizeof (DrawVerticeFormats);
    nb_vertices += 256; // fixe aligement
}


#include "../hlsl/primary.ps.h"
#include "../hlsl/primary.vs.h"

void InitMameShaders() {
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

    g_pPixelShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xps_ps_main);
    Xe_InstantiateShader(g_pVideoDevice, g_pPixelShader, 0);

    g_pVertexShader = Xe_LoadShaderFromMemory(g_pVideoDevice, (void*) g_xvs_vs_main);
    Xe_InstantiateShader(g_pVideoDevice, g_pVertexShader, 0);

    Xe_ShaderApplyVFetchPatches(g_pVideoDevice, g_pVertexShader, 0, &vbf);
}