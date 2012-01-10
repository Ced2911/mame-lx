typedef union {

    struct {
        unsigned char a;
        unsigned char r;
        unsigned char g;
        unsigned char b;
    };
    unsigned int lcol;
} XeColor;


void Menu_DrawRectangle(f32 x, f32 y, f32 width, f32 height, XeColor color, u8 filled);
void Menu_DrawImg(f32 xpos, f32 ypos, u16 width, u16 height, XenosSurface * data,f32 degrees, f32 scaleX, f32 scaleY, u8 alpha);

/** draw used by mame **/
void Menu_DrawMame(render_primitive *prim);

void Menu_Render();
void InitVideo();
void InitMameShaders();