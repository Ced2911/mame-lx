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

// we fake a keyboard with the following keys

enum {
    KEY_ESCAPE,
    KEY_P1_START,
    KEY_P1_COIN,
    KEY_BUTTON_1,
    KEY_BUTTON_2,
    KEY_BUTTON_3,
    KEY_JOYSTICK_U,
    KEY_JOYSTICK_D,
    KEY_JOYSTICK_L,
    KEY_JOYSTICK_R,
    KEY_TOTAL
};

static const char * x360DeviceNames[] = {
    "Joystick 1", "Joystick 2",
    "Joystick 3", "Joystick 4",
};

static const char * x360BtnNames[] = {
    "Big X", "Start", "Back",
    "Dpad Up", "Dpad Down", "Dpad Left", "Dpad Right",
    "A", "B", "X", "Y", "LB", "RB",
};
static const char * x360AnalogNames[] = {
    "RStick X", "RStick Y",
    "LStick X", "LStick Y",
    "LT", "RT",
};

enum XINPUT_AXIS {
    XINPUT_RX,
    XINPUT_RY,
    XINPUT_LX,
    XINPUT_LY,
    XINPUT_LT,
    XINPUT_RT,
};

enum XINPUTBTN {
    XINPUT_BIGX,
    XINPUT_START,
    XINPUT_BACK,
    XINPUT_UP,
    XINPUT_DOWN,
    XINPUT_LEFT,
    XINPUT_RIGHT,
    XINPUT_A,
    XINPUT_B,
    XINPUT_X,
    XINPUT_Y,
    XINPUT_LB,
    XINPUT_RB,
    XINPUT_MAX
};

static UINT8 joystick_state[4][XINPUT_MAX];
static signed int joystick_axis[4][XINPUT_MAX];

// a single input device
static input_device *joystick_device[4];

//============================================================
//  GLOBALS
//============================================================

// a single rendering target
static render_target *our_target;

//============================================================
//  FUNCTION PROTOTYPES
//============================================================
static INT32 keyboard_get_state(void *device_internal, void *item_internal);
static INT32 generic_axis_get_state(void *device_internal, void *item_internal);

//============================================================
//  main
//============================================================

int main() {
    xenon_init();
    int argc = 2;
    char * argv[]
    {
        "mame.elf", "mslug"
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
    our_target = machine.render().target_alloc();

    for (int i = 0; i < 4; i++) {

        joystick_device[i] = machine.input().device_class(DEVICE_CLASS_JOYSTICK).add_device(x360DeviceNames[i]);
        if (joystick_device[i] == NULL)
            fatalerror("Error creating joystick device");
        int dir_pos = ITEM_ID_BUTTON10;
        int btn_pos = ITEM_ID_BUTTON1; // (input_item_id)

        // add key
        joystick_device[i]->add_item(x360BtnNames[XINPUT_START], ITEM_ID_START, keyboard_get_state, &joystick_state[i][XINPUT_START]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BACK], ITEM_ID_SELECT, keyboard_get_state, &joystick_state[i][XINPUT_BACK]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BIGX], ITEM_ID_BUTTON16, keyboard_get_state, &joystick_state[i][XINPUT_BIGX]);
        // dir
/*
        joystick_device[i]->add_item(x360BtnNames[XINPUT_UP], (input_item_id) (dir_pos), keyboard_get_state, &joystick_state[i][XINPUT_UP]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LEFT], (input_item_id) (dir_pos + 1), keyboard_get_state, &joystick_state[i][XINPUT_LEFT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RIGHT], (input_item_id) (dir_pos + 2), keyboard_get_state, &joystick_state[i][XINPUT_RIGHT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_DOWN], (input_item_id) (dir_pos + 3), keyboard_get_state, &joystick_state[i][XINPUT_DOWN]);
*/
        
        joystick_device[i]->add_item(x360BtnNames[XINPUT_UP], (input_item_id)(ITEM_ID_HAT1UP + (i*4)), keyboard_get_state, &joystick_state[i][XINPUT_UP]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LEFT], (input_item_id)(ITEM_ID_HAT1LEFT+ (i*4)), keyboard_get_state, &joystick_state[i][XINPUT_LEFT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RIGHT], (input_item_id)(ITEM_ID_HAT1RIGHT+ (i*4)), keyboard_get_state, &joystick_state[i][XINPUT_RIGHT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_DOWN], (input_item_id)(ITEM_ID_HAT1DOWN+ (i*4)), keyboard_get_state, &joystick_state[i][XINPUT_DOWN]);
        
        // btn 
        joystick_device[i]->add_item(x360BtnNames[XINPUT_A], (input_item_id) (btn_pos), keyboard_get_state, &joystick_state[i][XINPUT_A]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_B], (input_item_id) (btn_pos + 1), keyboard_get_state, &joystick_state[i][XINPUT_B]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_X], (input_item_id) (btn_pos + 2), keyboard_get_state, &joystick_state[i][XINPUT_X]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_Y], (input_item_id) (btn_pos + 3), keyboard_get_state, &joystick_state[i][XINPUT_Y]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RB], (input_item_id) (btn_pos + 4), keyboard_get_state, &joystick_state[i][XINPUT_RB]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LB], (input_item_id) (btn_pos + 5), keyboard_get_state, &joystick_state[i][XINPUT_LB]);
        // axis
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_LX], ITEM_ID_XAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_LX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_LY], ITEM_ID_YAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_LY]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_RX], ITEM_ID_RXAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_RX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_RY], ITEM_ID_RYAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_RY]);
    }

    // nothing yet to do to initialize sound, since we don't have any
    // sound updates are handled by update_audio_stream() below

    // initialize the input system by adding devices
    // let's pretend like we have a keyboard device


    // our faux keyboard only has a couple of keys (corresponding to the
    // common defaults)
    /*
        keyboard_device->add_item("Esc", ITEM_ID_ESC, keyboard_get_state, &keyboard_state[KEY_ESCAPE]);
        keyboard_device->add_item("P1", ITEM_ID_1, keyboard_get_state, &keyboard_state[KEY_P1_START]);
        keyboard_device->add_item("B1", ITEM_ID_LCONTROL, keyboard_get_state, &keyboard_state[KEY_BUTTON_1]);
        keyboard_device->add_item("B2", ITEM_ID_LALT, keyboard_get_state, &keyboard_state[KEY_BUTTON_2]);
        keyboard_device->add_item("B3", ITEM_ID_SPACE, keyboard_get_state, &keyboard_state[KEY_BUTTON_3]);
        keyboard_device->add_item("JoyU", ITEM_ID_UP, keyboard_get_state, &keyboard_state[KEY_JOYSTICK_U]);
        keyboard_device->add_item("JoyD", ITEM_ID_DOWN, keyboard_get_state, &keyboard_state[KEY_JOYSTICK_D]);
        keyboard_device->add_item("JoyL", ITEM_ID_LEFT, keyboard_get_state, &keyboard_state[KEY_JOYSTICK_L]);
        keyboard_device->add_item("JoyR", ITEM_ID_RIGHT, keyboard_get_state, &keyboard_state[KEY_JOYSTICK_R]);
     */

    // hook up the debugger log
    //  add_logerror_callback(machine, output_oslog);
}

void update_input() {
    usb_do_poll();


    for (int i = 0; i < 4; i++) {
        //memset(keyboard_state,0,KEY_TOTAL);
        struct controller_data_s ctrl;
        get_controller_data(&ctrl, i);

        //btn
        joystick_state[i][XINPUT_START] = (ctrl.start)?0x80:0;
        joystick_state[i][XINPUT_BACK] = (ctrl.select)?0x80:0;
        joystick_state[i][XINPUT_BIGX] = (ctrl.logo)?0x80:0;

        joystick_state[i][XINPUT_UP] = (ctrl.up)?0x80:0;
        joystick_state[i][XINPUT_LEFT] = (ctrl.left)?0x80:0;
        joystick_state[i][XINPUT_RIGHT] = (ctrl.right)?0x80:0;
        joystick_state[i][XINPUT_DOWN] = (ctrl.down)?0x80:0;

        joystick_state[i][XINPUT_A] = (ctrl.a)?0x80:0;
        joystick_state[i][XINPUT_B] = (ctrl.b)?0x80:0;
        joystick_state[i][XINPUT_X] = (ctrl.x)?0x80:0;
        joystick_state[i][XINPUT_Y] = (ctrl.y)?0x80:0;
        joystick_state[i][XINPUT_RB] = (ctrl.rb)?0x80:0;
        joystick_state[i][XINPUT_LB] = (ctrl.lb)?0x80:0;

        //axis
        joystick_axis[i][XINPUT_LX] = ctrl.s1_x * 512;
        joystick_axis[i][XINPUT_LY] = ctrl.s1_y * -512;
        joystick_axis[i][XINPUT_RX] = ctrl.s2_x * 512;
        joystick_axis[i][XINPUT_RY] = ctrl.s2_y * -512;
    }

}

static void ShowFPS() {
    static unsigned long lastTick = 0;
    static int frames = 0;
    unsigned long nowTick;
    frames++;
    nowTick = mftb() / (PPC_TIMEBASE_FREQ / 1000);
    if (lastTick + 1000 <= nowTick) {

        printf("mini_osd_interface::update %d fps\r\n", frames);

        frames = 0;
        lastTick = nowTick;
    }
}


//============================================================
//  osd_update
//============================================================

void mini_osd_interface::update(bool skip_redraw) {
    // get the minimum width/height for the current layout
    int minwidth, minheight;
    our_target->compute_minimum_size(minwidth, minheight);

    // make that the size of our target
    our_target->set_bounds(minwidth, minheight);

    xenon_set_dim(minwidth, minheight);

    // get the list of primitives for the target at the current size
    render_primitive_list &primlist = our_target->get_primitives();

    // lock them, and then render them
    primlist.acquire_lock();

    if (skip_redraw == false)
        xenon_update_video(primlist);

    // do the drawing here
    primlist.release_lock();

    // after 5 seconds, exit
    if (machine().time() > attotime::from_seconds(5)) {
        //machine().schedule_exit();
    }

    ShowFPS();

    update_input();
}


//============================================================
//  update_audio_stream
//============================================================

void mini_osd_interface::update_audio_stream(const INT16 *buffer, int samples_this_frame) {
    // if we had actual sound output, we would copy the
    // interleaved stereo samples to our sound stream
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

void mini_osd_interface::customize_input_type_list(simple_list<input_type_entry> &typelist) {
    // This function is called on startup, before reading the
    // configuration from disk. Scan the list, and change the
    // default control mappings you want. It is quite possible
    // you won't need to change a thing.
}


//============================================================
//  keyboard_get_state
//============================================================

static INT32 keyboard_get_state(void *device_internal, void *item_internal) {
    UINT8 *keystate = (UINT8 *) item_internal;
    return *keystate;
}

static INT32 generic_axis_get_state(void *device_internal, void *item_internal) {
    INT32 *axisdata = (INT32 *) item_internal;
    return *axisdata;
}