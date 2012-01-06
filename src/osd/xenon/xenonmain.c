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
#include <debug.h>
#include <usb/usbmain.h>
#include <input/input.h>
#include <ppc/timebase.h>
#include <time/time.h>
//============================================================
//  CONSTANTS
//============================================================

//============================================================
//  GLOBALS
//============================================================

// a single rendering target
render_target *xenos_target;

//============================================================
//  main
//============================================================
int main() {
    osd_xenon_init();
    int argc = 2;
    char * argv[]
    {
        "mame.elf", "mslug"
        //"mame.elf"
        //		"uda:/xenon.elf"
        //"uda:/xenon.elf","-lx"
        //"mame.elf", "sfiiin"
        //"mame.elf", "sf2ce"
        //"mame.elf", "sfa3"
        //"mame.elf", "mk"
        //"mame.elf"
        //"mame.elf", "umk3"
    };
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

    // init input
    osd_xenon_input_init(machine);
}



//============================================================
//  osd_update
//============================================================
static void ShowFPS() {
    static unsigned long lastTick = 0;
    static int frames = 0;
    unsigned long nowTick;
    frames++;
    nowTick = mftb() / (PPC_TIMEBASE_FREQ / 1000);
    if (lastTick + 1000 <= nowTick) {

        printf("Mame %d fps\r\n", frames);

        frames = 0;
        lastTick = nowTick;
    }
}

void mini_osd_interface::update(bool skip_redraw) {
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
    // This function is called on startup, before reading the
    // configuration from disk. Scan the list, and change the
    // default control mappings you want. It is quite possible
    // you won't need to change a thing.


    osd_xenon_customize_input_type_list(typelist);
}


