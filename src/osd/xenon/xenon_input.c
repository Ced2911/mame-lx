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
static INT32 joystick_axis[4][XINPUT_MAX];

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

        joystick_device[i]->add_item(x360BtnNames[XINPUT_UP], ITEM_ID_BUTTON10, generic_btn_get_state, &joystick_state[i][XINPUT_UP]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LEFT], ITEM_ID_BUTTON11, generic_btn_get_state, &joystick_state[i][XINPUT_LEFT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RIGHT], ITEM_ID_BUTTON12, generic_btn_get_state, &joystick_state[i][XINPUT_RIGHT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_DOWN], ITEM_ID_BUTTON13, generic_btn_get_state, &joystick_state[i][XINPUT_DOWN]);

        // btn 
        joystick_device[i]->add_item(x360BtnNames[XINPUT_A], ITEM_ID_BUTTON1, generic_btn_get_state, &joystick_state[i][XINPUT_A]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_B], ITEM_ID_BUTTON2, generic_btn_get_state, &joystick_state[i][XINPUT_B]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_X], ITEM_ID_BUTTON3, generic_btn_get_state, &joystick_state[i][XINPUT_X]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_Y], ITEM_ID_BUTTON4, generic_btn_get_state, &joystick_state[i][XINPUT_Y]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RB], ITEM_ID_BUTTON5, generic_btn_get_state, &joystick_state[i][XINPUT_RB]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LB], ITEM_ID_BUTTON6, generic_btn_get_state, &joystick_state[i][XINPUT_LB]);

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
        memset(&ctrl[i], 0, sizeof (controller_data_s));

        get_controller_data(&ctrl[i], i);
        /*
         if(ctrl[i].logo)
             exit(0);
         */
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
#define	STICK_DEAD_ZONE (32768*0.4)
#define HANDLE_STICK_DEAD_ZONE(x) ((((x)>-STICK_DEAD_ZONE) && (x)<STICK_DEAD_ZONE)?0:(x-x/abs(x)*STICK_DEAD_ZONE))

        joystick_axis[i][XINPUT_LX] = HANDLE_STICK_DEAD_ZONE(ctrl[i].s1_x) * 256;
        joystick_axis[i][XINPUT_LY] = HANDLE_STICK_DEAD_ZONE(ctrl[i].s1_y) * 256;
        joystick_axis[i][XINPUT_RX] = HANDLE_STICK_DEAD_ZONE(ctrl[i].s2_x) * 256;
        joystick_axis[i][XINPUT_RY] = HANDLE_STICK_DEAD_ZONE(ctrl[i].s2_y) * 256;

        /*
                printf("ctrl[i].s1_x = %d\r\n",joystick_axis[i][XINPUT_LX]);
                printf("ctrl[i].s1_y = %d\r\n",joystick_axis[i][XINPUT_LY]);
         */
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

            case IPT_UI_UP:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON10);
                break;
            case IPT_UI_DOWN:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON13);
                break;
            case IPT_UI_LEFT:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON11);
                break;
            case IPT_UI_RIGHT:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON12);
                break;

            case IPT_JOYSTICK_UP:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON10);
                break;
            case IPT_JOYSTICK_DOWN:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON13);
                break;
            case IPT_JOYSTICK_LEFT:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON11);
                break;
            case IPT_JOYSTICK_RIGHT:
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON12);
                break;

            case IPT_UI_CANCEL:
                //entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_START,JOYCODE_SELECT);
                break;
                
            case IPT_OSD_1:
                entry->token = "MAME_UI_EXIT";
                entry->name = "Exit mame";
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON1, JOYCODE_BUTTON16);
                break;
                
            case IPT_JOYSTICKRIGHT_UP:
            case IPT_JOYSTICKRIGHT_DOWN:
            case IPT_JOYSTICKRIGHT_LEFT:
            case IPT_JOYSTICKRIGHT_RIGHT:
            case IPT_JOYSTICKLEFT_UP:
            case IPT_JOYSTICKLEFT_DOWN:
            case IPT_JOYSTICKLEFT_LEFT:
            case IPT_JOYSTICKLEFT_RIGHT:
                entry->defseq[SEQ_TYPE_STANDARD].set();
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
    INT32 *axisdata = (INT32 *) item_internal;
    return *axisdata;
}