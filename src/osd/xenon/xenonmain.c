//============================================================
//
//  minimain.c - Main function for mini OSD
//
//============================================================
//
//  Copyright Aaron Giles
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the
//  following conditions are met:
//
//    * Redistributions of source code must retain the above
//      copyright notice, this list of conditions and the
//      following disclaimer.
//    * Redistributions in binary form must reproduce the
//      above copyright notice, this list of conditions and
//      the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//    * Neither the name 'MAME' nor the names of its
//      contributors may be used to endorse or promote
//      products derived from this software without specific
//      prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
//  EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGE (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
//  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//============================================================

#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdxenon.h"
#include "xenon.h"
#include "ui.h"
#include "emuopts.h"
#include "uiinput.h"
#include <debug.h>
#include <usb/usbmain.h>
#include <input/input.h>
#include <ppc/timebase.h>
#include <time/time.h>
extern "C"{
#include <xenos/xenos_edid.h>
}

// edid stuff ...
void edid_stuff(){
    struct edid * hdmi_edid = xenos_get_edid();
    if(hdmi_edid){
        for(int i=0;i<8;i++)
            printf("hsize = %02x vfreq_aspect = %02x\r\n",hdmi_edid->standard_timings[i].hsize ,hdmi_edid->standard_timings[i].vfreq_aspect);
    }
}

xe_options::xe_options(){
    
}

//============================================================
//  CONSTANTS
//============================================================

//============================================================
//  GLOBALS
//============================================================

// a single rendering target
extern render_target *xenos_target;

#if 0
//============================================================
//  main
//============================================================
int main() {
    osd_xenon_init();
    int argc =1;
    char * argv[]
    {
        "mame.elf"
        //"mame.elf","mslug"
        //		"uda:/xenon.elf"
        //"uda:/xenon.elf","-lx"
        //"mame.elf", "sfiiin"
        //"mame.elf", "sf2ce"
        //"mame.elf", "sfa3"
        //"mame.elf", "mk"
        //"mame.elf"
        //"mame.elf", "umk3"
    };
    edid_stuff();
    TR;
    // cli_frontend does the heavy lifting; if we have osd-specific options, we
    // create a derivative of cli_options and add our own
    cli_options options;
    //options.
    mini_osd_interface osd;
    TR;
    cli_frontend frontend(options, osd);
    TR;
    return frontend.execute(argc, argv);
}
#endif

int xenon_main(int argc, char * argv[]){
    osd_xenon_init();
    // cli_frontend does the heavy lifting; if we have osd-specific options, we
    // create a derivative of cli_options and add our own
    cli_options options;
    //options.
    mini_osd_interface osd;
    TR;
    cli_frontend frontend(options, osd);
    TR;
    return frontend.execute(argc, argv);
}

//============================================================
//  constructor
//============================================================

mini_osd_interface::mini_osd_interface() {
}


//============================================================
//  destructor
//============================================================
mini_osd_interface::~mini_osd_interface() {
}

//============================================================
//  init
//============================================================
void mini_osd_interface::init(running_machine &machine) {
    // call our parent
    osd_interface::init(machine);

    // initialize the video system by allocating a rendering target
    xenos_target = machine.render().target_alloc();
        
    osd_xenon_video_hw_init(machine);
    
    // init input
    osd_xenon_input_init(machine);
    
    // init sound
    osd_xenon_sound_init();
}



//============================================================
//  osd_update
//============================================================
static void ShowFPS() {
    static unsigned long lastTick = 0;
    static int frames = 0;
    unsigned long nowTick;
    frames++;
    nowTick = mftb() / (PPC_TIMEBASE_FREQ/1000);
    if (lastTick + 1000 <= nowTick) {

        printf("Mame %d fps\r\n", frames);

        frames = 0;
        lastTick = nowTick;
    }
}

static int exit_asked=0;

void ask_exit(){
    exit_asked=1;
}

static void check_osd_inputs(running_machine &machine)
{
    if (ui_input_pressed(machine, IPT_OSD_1)|exit_asked)
    {
        // pause video thread
        osd_xenon_video_pause();
        
         // ask for exit
        int mame_exit = WindowPrompt("Exit to main menu", "Are you sure you want to exit the game and go back to main menu", "Yes", "No");
        if(mame_exit)
            machine.schedule_exit();
        else
            osd_xenon_video_resume();
        
        exit_asked=0;
    }
}

void mini_osd_interface::update(bool skip_redraw) {
    check_osd_inputs(machine());
    // get the list of primitives for the target at the current size
    render_primitive_list &primlist = xenos_target->get_primitives();

    if (!skip_redraw)
        osd_xenon_update_video(primlist);

    ShowFPS();

    osd_xenon_update_input();
}


//============================================================
//  update_audio_stream
//============================================================
void mini_osd_interface::update_audio_stream(const INT16 *buffer, int samples_this_frame) {
    // if we had actual sound output, we would copy the
    // interleaved stereo samples to our sound stream
    osd_xenon_update_sound(buffer,samples_this_frame);
}


//============================================================
//  set_mastervolume
//============================================================
void mini_osd_interface::set_mastervolume(int attenuation) {
    // if we had actual sound output, we would adjust the global
    // volume in response to this function
}


//============================================================
//  customize_input_type_list
//============================================================
void osd_xenon_customize_input_type_list(simple_list<input_type_entry> &typelist); // xenon_input.c
void mini_osd_interface::customize_input_type_list(simple_list<input_type_entry> &typelist) {
    osd_xenon_customize_input_type_list(typelist);
}


