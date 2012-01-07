#define INLINE static inline

INLINE UINT32 ycc_to_rgb(UINT8 y, UINT8 cb, UINT8 cr)
{
	/* original equations:

        C = Y - 16
        D = Cb - 128
        E = Cr - 128

        R = clip(( 298 * C           + 409 * E + 128) >> 8)
        G = clip(( 298 * C - 100 * D - 208 * E + 128) >> 8)
        B = clip(( 298 * C + 516 * D           + 128) >> 8)

        R = clip(( 298 * (Y - 16)                    + 409 * (Cr - 128) + 128) >> 8)
        G = clip(( 298 * (Y - 16) - 100 * (Cb - 128) - 208 * (Cr - 128) + 128) >> 8)
        B = clip(( 298 * (Y - 16) + 516 * (Cb - 128)                    + 128) >> 8)

        R = clip(( 298 * Y - 298 * 16                        + 409 * Cr - 409 * 128 + 128) >> 8)
        G = clip(( 298 * Y - 298 * 16 - 100 * Cb + 100 * 128 - 208 * Cr + 208 * 128 + 128) >> 8)
        B = clip(( 298 * Y - 298 * 16 + 516 * Cb - 516 * 128                        + 128) >> 8)

        R = clip(( 298 * Y - 298 * 16                        + 409 * Cr - 409 * 128 + 128) >> 8)
        G = clip(( 298 * Y - 298 * 16 - 100 * Cb + 100 * 128 - 208 * Cr + 208 * 128 + 128) >> 8)
        B = clip(( 298 * Y - 298 * 16 + 516 * Cb - 516 * 128                        + 128) >> 8)
    */
	int r, g, b, common;

	common = 298 * y - 298 * 16;
	r = (common +                        409 * cr - 409 * 128 + 128) >> 8;
	g = (common - 100 * cb + 100 * 128 - 208 * cr + 208 * 128 + 128) >> 8;
	b = (common + 516 * cb - 516 * 128                        + 128) >> 8;

	if (r < 0) r = 0;
	else if (r > 255) r = 255;
	if (g < 0) g = 0;
	else if (g > 255) g = 255;
	if (b < 0) b = 0;
	else if (b > 255) b = 255;

	return MAKE_ARGB(0xff, r, g, b);
}

//============================================================
//  copyline_palette16
//============================================================

INLINE void copyline_palette16(UINT32 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 1);
    if (xborderpix)
        *dst++ = 0xff000000 | palette[*src];
    for (x = 0; x < width; x++)
        *dst++ = 0xff000000 | palette[*src++];
    if (xborderpix)
        *dst++ = 0xff000000 | palette[*--src];
}



//============================================================
//  copyline_palettea16
//============================================================

INLINE void copyline_palettea16(UINT32 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 1);
    if (xborderpix)
        *dst++ = palette[*src];
    for (x = 0; x < width; x++)
        *dst++ = palette[*src++];
    if (xborderpix)
        *dst++ = palette[*--src];
}



//============================================================
//  copyline_rgb15
//============================================================

INLINE void copyline_rgb15(UINT32 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 1);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT16 pix = *src;
            *dst++ = 0xff000000 | palette[0x40 + ((pix >> 10) & 0x1f)] | palette[0x20 + ((pix >> 5) & 0x1f)] | palette[0x00 + ((pix >> 0) & 0x1f)];
        }
        for (x = 0; x < width; x++) {
            UINT16 pix = *src++;
            *dst++ = 0xff000000 | palette[0x40 + ((pix >> 10) & 0x1f)] | palette[0x20 + ((pix >> 5) & 0x1f)] | palette[0x00 + ((pix >> 0) & 0x1f)];
        }
        if (xborderpix) {
            UINT16 pix = *--src;
            *dst++ = 0xff000000 | palette[0x40 + ((pix >> 10) & 0x1f)] | palette[0x20 + ((pix >> 5) & 0x1f)] | palette[0x00 + ((pix >> 0) & 0x1f)];
        }
    }
        // direct case
    else {
        if (xborderpix) {
            UINT16 pix = *src;
            UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
            *dst++ = 0xff000000 | color | ((color >> 5) & 0x070707);
        }
        for (x = 0; x < width; x++) {
            UINT16 pix = *src++;
            UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
            *dst++ = 0xff000000 | color | ((color >> 5) & 0x070707);
        }
        if (xborderpix) {
            UINT16 pix = *--src;
            UINT32 color = ((pix & 0x7c00) << 9) | ((pix & 0x03e0) << 6) | ((pix & 0x001f) << 3);
            *dst++ = 0xff000000 | color | ((color >> 5) & 0x070707);
        }
    }
}



//============================================================
//  copyline_rgb32
//============================================================

INLINE void copyline_rgb32(UINT32 *dst, const UINT32 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 1);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT32 srcpix = *src;
            *dst++ = 0xff000000 | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
        for (x = 0; x < width; x++) {
            UINT32 srcpix = *src++;
            *dst++ = 0xff000000 | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
        if (xborderpix) {
            UINT32 srcpix = *--src;
            *dst++ = 0xff000000 | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
    }
        // direct case
    else {
        if (xborderpix)
            *dst++ = 0xff000000 | *src;
        for (x = 0; x < width; x++)
            *dst++ = 0xff000000 | *src++;
        if (xborderpix)
            *dst++ = 0xff000000 | *--src;
    }
}



//============================================================
//  copyline_argb32
//============================================================

INLINE void copyline_argb32(UINT32 *dst, const UINT32 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 1);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT32 srcpix = *src;
            *dst++ = (srcpix & 0xff000000) | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
        for (x = 0; x < width; x++) {
            UINT32 srcpix = *src++;
            *dst++ = (srcpix & 0xff000000) | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
        if (xborderpix) {
            UINT32 srcpix = *--src;
            *dst++ = (srcpix & 0xff000000) | palette[0x200 + RGB_RED(srcpix)] | palette[0x100 + RGB_GREEN(srcpix)] | palette[RGB_BLUE(srcpix)];
        }
    }
        // direct case
    else {
        if (xborderpix)
            *dst++ = *src;
        for (x = 0; x < width; x++)
            *dst++ = *src++;
        if (xborderpix)
            *dst++ = *--src;
    }
}



//============================================================
//  copyline_yuy16_to_yuy2
//============================================================

INLINE void copyline_yuy16_to_yuy2(UINT16 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 2);
    assert(width % 2 == 0);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src--;
            *dst++ = palette[0x000 + (srcpix0 >> 8)] | (srcpix0 << 8);
            *dst++ = palette[0x000 + (srcpix0 >> 8)] | (srcpix1 << 8);
        }
        for (x = 0; x < width; x += 2) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src++;
            *dst++ = palette[0x000 + (srcpix0 >> 8)] | (srcpix0 << 8);
            *dst++ = palette[0x000 + (srcpix1 >> 8)] | (srcpix1 << 8);
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            *dst++ = palette[0x000 + (srcpix1 >> 8)] | (srcpix0 << 8);
            *dst++ = palette[0x000 + (srcpix1 >> 8)] | (srcpix1 << 8);
        }
    }
        // direct case
    else {
        if (xborderpix) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src--;
            *dst++ = (srcpix0 >> 8) | (srcpix0 << 8);
            *dst++ = (srcpix0 >> 8) | (srcpix1 << 8);
        }
        for (x = 0; x < width; x += 2) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src++;
            *dst++ = (srcpix0 >> 8) | (srcpix0 << 8);
            *dst++ = (srcpix1 >> 8) | (srcpix1 << 8);
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            *dst++ = (srcpix1 >> 8) | (srcpix0 << 8);
            *dst++ = (srcpix1 >> 8) | (srcpix1 << 8);
        }
    }
}



//============================================================
//  copyline_yuy16_to_uyvy
//============================================================

INLINE void copyline_yuy16_to_uyvy(UINT16 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 2);
    assert(width % 2 == 0);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src--;
            *dst++ = palette[0x100 + (srcpix0 >> 8)] | (srcpix0 & 0xff);
            *dst++ = palette[0x100 + (srcpix0 >> 8)] | (srcpix1 & 0xff);
        }
        for (x = 0; x < width; x += 2) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src++;
            *dst++ = palette[0x100 + (srcpix0 >> 8)] | (srcpix0 & 0xff);
            *dst++ = palette[0x100 + (srcpix1 >> 8)] | (srcpix1 & 0xff);
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            *dst++ = palette[0x100 + (srcpix1 >> 8)] | (srcpix0 & 0xff);
            *dst++ = palette[0x100 + (srcpix1 >> 8)] | (srcpix1 & 0xff);
        }
    }
        // direct case
    else {
        if (xborderpix) {
            UINT16 srcpix0 = src[0];
            UINT16 srcpix1 = src[1];
            *dst++ = srcpix0;
            *dst++ = (srcpix0 & 0xff00) | (srcpix1 & 0x00ff);
        }
        for (x = 0; x < width; x += 2) {
            *dst++ = *src++;
            *dst++ = *src++;
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            *dst++ = (srcpix1 & 0xff00) | (srcpix0 & 0x00ff);
            *dst++ = srcpix1;
        }
    }
}



//============================================================
//  copyline_yuy16_to_argb
//============================================================

INLINE void copyline_yuy16_to_argb(UINT32 *dst, const UINT16 *src, int width, const rgb_t *palette, int xborderpix) {
    int x;

    assert(xborderpix == 0 || xborderpix == 2);
    assert(width % 2 == 0);

    // palette (really RGB map) case
    if (palette != NULL) {
        if (xborderpix) {
            UINT16 srcpix0 = src[0];
            UINT16 srcpix1 = src[1];
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix0 >> 8)], cb, cr);
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix0 >> 8)], cb, cr);
        }
        for (x = 0; x < width / 2; x++) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src++;
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix0 >> 8)], cb, cr);
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix1 >> 8)], cb, cr);
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix1 >> 8)], cb, cr);
            *dst++ = ycc_to_rgb(palette[0x000 + (srcpix1 >> 8)], cb, cr);
        }
    }
        // direct case
    else {
        if (xborderpix) {
            UINT16 srcpix0 = src[0];
            UINT16 srcpix1 = src[1];
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(srcpix0 >> 8, cb, cr);
            *dst++ = ycc_to_rgb(srcpix0 >> 8, cb, cr);
        }
        for (x = 0; x < width; x += 2) {
            UINT16 srcpix0 = *src++;
            UINT16 srcpix1 = *src++;
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(srcpix0 >> 8, cb, cr);
            *dst++ = ycc_to_rgb(srcpix1 >> 8, cb, cr);
        }
        if (xborderpix) {
            UINT16 srcpix1 = *--src;
            UINT16 srcpix0 = *--src;
            UINT8 cb = srcpix0 & 0xff;
            UINT8 cr = srcpix1 & 0xff;
            *dst++ = ycc_to_rgb(srcpix1 >> 8, cb, cr);
            *dst++ = ycc_to_rgb(srcpix1 >> 8, cb, cr);
        }
    }
}

