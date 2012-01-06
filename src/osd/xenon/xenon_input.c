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
//  FUNCTION PROTOTYPES
//============================================================
static INT32 generic_axis_get_state(void *device_internal, void *item_internal);
static INT32 generic_btn_get_state(void *device_internal, void *item_internal);

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

// input devices
static input_device *joystick_device[4];
struct controller_data_s ctrl[4];

void osd_xenon_input_init(running_machine &machine) {
    for (int i = 0; i < 4; i++) {

        joystick_device[i] = machine.input().device_class(DEVICE_CLASS_JOYSTICK).add_device(x360DeviceNames[i]);
        
        if (joystick_device[i] == NULL)
            fatalerror("Error creating joystick device");
        
        int dir_pos = ITEM_ID_BUTTON10;
        int btn_pos = ITEM_ID_BUTTON1; // (input_item_id)

        // add key
        joystick_device[i]->add_item(x360BtnNames[XINPUT_START], ITEM_ID_START, generic_btn_get_state, &joystick_state[i][XINPUT_START]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BACK], ITEM_ID_SELECT, generic_btn_get_state, &joystick_state[i][XINPUT_BACK]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BIGX], ITEM_ID_BUTTON16, generic_btn_get_state, &joystick_state[i][XINPUT_BIGX]);
        // dir
        /*
                joystick_device[i]->add_item(x360BtnNames[XINPUT_UP], (input_item_id) (dir_pos), keyboard_get_state, &joystick_state[i][XINPUT_UP]);
                joystick_device[i]->add_item(x360BtnNames[XINPUT_LEFT], (input_item_id) (dir_pos + 1), keyboard_get_state, &joystick_state[i][XINPUT_LEFT]);
                joystick_device[i]->add_item(x360BtnNames[XINPUT_RIGHT], (input_item_id) (dir_pos + 2), keyboard_get_state, &joystick_state[i][XINPUT_RIGHT]);
                joystick_device[i]->add_item(x360BtnNames[XINPUT_DOWN], (input_item_id) (dir_pos + 3), keyboard_get_state, &joystick_state[i][XINPUT_DOWN]);
         */

        joystick_device[i]->add_item(x360BtnNames[XINPUT_UP], (input_item_id) (ITEM_ID_HAT1UP + (i * 4)), generic_btn_get_state, &joystick_state[i][XINPUT_UP]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LEFT], (input_item_id) (ITEM_ID_HAT1LEFT + (i * 4)), generic_btn_get_state, &joystick_state[i][XINPUT_LEFT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RIGHT], (input_item_id) (ITEM_ID_HAT1RIGHT + (i * 4)), generic_btn_get_state, &joystick_state[i][XINPUT_RIGHT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_DOWN], (input_item_id) (ITEM_ID_HAT1DOWN + (i * 4)), generic_btn_get_state, &joystick_state[i][XINPUT_DOWN]);

        // btn 
        joystick_device[i]->add_item(x360BtnNames[XINPUT_A], (input_item_id) (btn_pos), generic_btn_get_state, &joystick_state[i][XINPUT_A]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_B], (input_item_id) (btn_pos + 1), generic_btn_get_state, &joystick_state[i][XINPUT_B]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_X], (input_item_id) (btn_pos + 2), generic_btn_get_state, &joystick_state[i][XINPUT_X]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_Y], (input_item_id) (btn_pos + 3), generic_btn_get_state, &joystick_state[i][XINPUT_Y]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RB], (input_item_id) (btn_pos + 4), generic_btn_get_state, &joystick_state[i][XINPUT_RB]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LB], (input_item_id) (btn_pos + 5), generic_btn_get_state, &joystick_state[i][XINPUT_LB]);
        
        // axis
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_LX], ITEM_ID_XAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_LX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_LY], ITEM_ID_YAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_LY]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_RX], ITEM_ID_RXAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_RX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_RY], ITEM_ID_RYAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_RY]);
    }
}

void osd_xenon_update_input() {
    usb_do_poll();

    for (int i = 0; i < 4; i++) {
        //memset(keyboard_state,0,KEY_TOTAL);
        
        get_controller_data(&ctrl[i], i);
       
        if(ctrl[i].logo)
            exit(0);
        
        //btn
        joystick_state[i][XINPUT_START] = (ctrl[i].start) ? 0x80 : 0;
        joystick_state[i][XINPUT_BACK] = (ctrl[i].select) ? 0x80 : 0;
        joystick_state[i][XINPUT_BIGX] = (ctrl[i].logo) ? 0x80 : 0;

        joystick_state[i][XINPUT_UP] = (ctrl[i].up) ? 0x80 : 0;
        joystick_state[i][XINPUT_LEFT] = (ctrl[i].left) ? 0x80 : 0;
        joystick_state[i][XINPUT_RIGHT] = (ctrl[i].right) ? 0x80 : 0;
        joystick_state[i][XINPUT_DOWN] = (ctrl[i].down) ? 0x80 : 0;

        joystick_state[i][XINPUT_A] = (ctrl[i].a) ? 0x80 : 0;
        joystick_state[i][XINPUT_B] = (ctrl[i].b) ? 0x80 : 0;
        joystick_state[i][XINPUT_X] = (ctrl[i].x) ? 0x80 : 0;
        joystick_state[i][XINPUT_Y] = (ctrl[i].y) ? 0x80 : 0;
        joystick_state[i][XINPUT_RB] = (ctrl[i].rb) ? 0x80 : 0;
        joystick_state[i][XINPUT_LB] = (ctrl[i].lb) ? 0x80 : 0;

        //axis
        joystick_axis[i][XINPUT_LX] = ctrl[i].s1_x;
        joystick_axis[i][XINPUT_LY] = ctrl[i].s1_y;
        joystick_axis[i][XINPUT_RX] = ctrl[i].s2_x;
        joystick_axis[i][XINPUT_RY] = ctrl[i].s2_y;
    }
}


// default ...

void osd_xenon_customize_input_type_list(simple_list<input_type_entry> &typelist) {
    input_type_entry *entry;

    // loop over the defaults
    for (entry = typelist.first(); entry != NULL; entry = entry->next()) {
        switch (entry->type) {
            case IPT_UI_CONFIGURE:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON16);
                break;
        }
    }
}



//============================================================
//  
//============================================================

static INT32 generic_btn_get_state(void *device_internal, void *item_internal) {
    UINT8 *keystate = (UINT8 *) item_internal;
    return *keystate;
}

static INT32 generic_axis_get_state(void *device_internal, void *item_internal) {
    INT16 *axisdata = (INT16 *) item_internal;
    return *axisdata;
}