#pragma once

typedef union {

    struct {
        unsigned char a;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
    unsigned int lcol;
} XeColor;

typedef struct {
    u8 r, g, b, a;
}
GXColor;

void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, XeColor color, u8 filled);
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, XenosSurface * data,f32 degrees, f32 scaleX, f32 scaleY, u8 alpha);
void Menu_T(XenosSurface * surf, f32 texWidth, f32 texHeight, int16_t screenX, int16_t screenY, XeColor color);

void Menu_Render();
void InitVideo();
void InitMameShaders();

extern int screenheight;
extern int screenwidth;

extern XenosDevice * g_pVideoDevice;
extern u32 FrameTimer;