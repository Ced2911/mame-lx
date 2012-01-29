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

#define XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689
#define XINPUT_GAMEPAD_TRIGGER_THRESHOLD    30

static const char * x360DeviceNames[] = {
    "Joystick 1", "Joystick 2",
    "Joystick 3", "Joystick 4",
};

static const char * x360BtnNames[] = {
    "Big X", "Start", "Back",
    "Dpad Up", "Dpad Down", "Dpad Left", "Dpad Right",
    "A", "B", "X", "Y",
    "LB", "RB",
    "LT", "RT",
    "Stick 1","Stick 2"
};
static const char * x360AnalogNames[] = {
    "RStick X", "RStick Y",
    "LStick X", "LStick Y",
};

enum XINPUT_AXIS {
    XINPUT_AXIS_RX,
    XINPUT_AXIS_RY,
    XINPUT_AXIS_LX,
    XINPUT_AXIS_LY,
    
    XINPUT_AXIS_MAX
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
    XINPUT_LT,
    XINPUT_RT,
    
    XINPUT_S1,
    XINPUT_S2,
    
    XINPUT_MAX
};

static UINT8 joystick_state[4][XINPUT_MAX];
static INT32 joystick_axis[4][XINPUT_AXIS_MAX];

// input devices
static input_device *joystick_device[4];
struct controller_data_s ctrl[4];

void osd_xenon_input_init(running_machine &machine) {
    for (int i = 0; i < 4; i++) {

        joystick_device[i] = machine.input().device_class(DEVICE_CLASS_JOYSTICK).add_device(x360DeviceNames[i]);

        if (joystick_device[i] == NULL)
            fatalerror("Error creating joystick device");
        
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
        joystick_device[i]->add_item(x360BtnNames[XINPUT_RT], ITEM_ID_BUTTON7, generic_btn_get_state, &joystick_state[i][XINPUT_RT]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_LT], ITEM_ID_BUTTON8, generic_btn_get_state, &joystick_state[i][XINPUT_LT]);
                
        joystick_device[i]->add_item(x360BtnNames[XINPUT_S1], ITEM_ID_BUTTON14, generic_btn_get_state, &joystick_state[i][XINPUT_S1]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_S2], ITEM_ID_BUTTON15, generic_btn_get_state, &joystick_state[i][XINPUT_S2]);

        joystick_device[i]->add_item(x360BtnNames[XINPUT_START], ITEM_ID_START, generic_btn_get_state, &joystick_state[i][XINPUT_START]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BACK], ITEM_ID_SELECT, generic_btn_get_state, &joystick_state[i][XINPUT_BACK]);
        joystick_device[i]->add_item(x360BtnNames[XINPUT_BIGX], ITEM_ID_BUTTON16, generic_btn_get_state, &joystick_state[i][XINPUT_BIGX]);
        
        
        // axis
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_AXIS_LX], ITEM_ID_XAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_AXIS_LX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_AXIS_LY], ITEM_ID_YAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_AXIS_LY]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_AXIS_RX], ITEM_ID_RXAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_AXIS_RX]);
        joystick_device[i]->add_item(x360AnalogNames[XINPUT_AXIS_RY], ITEM_ID_RYAXIS, generic_axis_get_state, &joystick_axis[i][XINPUT_AXIS_RY]);
        
    }
}

static int osd_exit = 0;
void osd_xenon_update_input() {
    usb_do_poll();

    for (int i = 0; i < 4; i++) {
        //memset(keyboard_state,0,KEY_TOTAL);
        memset(&ctrl[i], 0, sizeof (controller_data_s));
        memset(&joystick_state[i], 0, sizeof (XINPUT_MAX));
        memset(&joystick_axis[i], 0, sizeof (XINPUT_AXIS_MAX));
        
        get_controller_data(&ctrl[i], i);
         
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
                
        joystick_state[i][XINPUT_S1] = (ctrl[i].s1_z) ? 0x80 : 0;
        joystick_state[i][XINPUT_S2] = (ctrl[i].s2_z) ? 0x80 : 0;
        
        joystick_state[i][XINPUT_RT] = (ctrl[i].rt>XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? 0x80 : 0;
        joystick_state[i][XINPUT_LT] = (ctrl[i].lt>XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ? 0x80 : 0;
       
/*
        joystick_axis[i][XINPUT_AXIS_LX] = -(ctrl[i].s1_x)<<16;
        joystick_axis[i][XINPUT_AXIS_LY] = -(ctrl[i].s1_y)<<16;
        joystick_axis[i][XINPUT_AXIS_RX] = (ctrl[i].s2_x)<<16;
        joystick_axis[i][XINPUT_AXIS_RY] = (ctrl[i].s2_y)<<16;
*/
        joystick_axis[i][XINPUT_AXIS_LX] = -(ctrl[i].s1_x)<<1;
        joystick_axis[i][XINPUT_AXIS_LY] = -(ctrl[i].s1_y)<<1;
        joystick_axis[i][XINPUT_AXIS_RX] = (ctrl[i].s2_x)<<1;
        joystick_axis[i][XINPUT_AXIS_RY] = (ctrl[i].s2_y)<<1;
    }
    
    if(ctrl[0].logo)
    {
        osd_exit++;
        if(osd_exit>10){
            void ask_exit();
            ask_exit();
            osd_exit=0;
        }
    }
    else{
        osd_exit = 0;
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
                entry->defseq[SEQ_TYPE_STANDARD].set(JOYCODE_BUTTON14, JOYCODE_BUTTON15); // s1 + s2
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