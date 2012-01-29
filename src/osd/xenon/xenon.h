#include "clifront.h"
#include "render.h"

// init libxenon
int osd_xenon_init();
// update video
void osd_xenon_update_video(render_primitive_list &primlist);
void osd_xenon_set_dim(int w,int h);
void osd_xenon_video_init();
void osd_xenon_video_hw_init(running_machine &machine);

// input
void osd_xenon_input_init(running_machine &machine);
void osd_xenon_update_input();

// sound
void osd_xenon_update_sound(const INT16 *buffer, int samples_this_frame);
void osd_xenon_sound_init();

int WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label) ;
void osd_xenon_video_pause();
void osd_xenon_video_resume();
struct XenosSurface * osd_xenon_get_surface();

class xe_options : public cli_options
{
public:
    // construction/destruction
    xe_options();
};