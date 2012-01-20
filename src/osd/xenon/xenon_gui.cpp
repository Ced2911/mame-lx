//#include <ogcsys.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

//xenon stuff
#include <xetypes.h>
#include <xenon_smc/xenon_smc.h>
#include <xenon_soc/xenon_power.h>
#include <debug.h>
#include <usb/usbmain.h>
#include <input/input.h>
#include <ppc/timebase.h>
#include <time/time.h>

//gui stuff
#include "gui/libwiigui/gui.h"
#include "gui/input.h"
#include "gui/filelist.h"
#include "gui/filebrowser.h"
#include "gui/w_input.h"
#include "gui/gui_debug.h"
#include "gui/gui_romlist.h"

// maem stuff
#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdxenon.h"
#include "xenon.h"

// mame define thoses
#undef delete
#undef new

enum
{
    MENU_EXIT = -1,
    MENU_NONE,
    MENU_MAIN,
    MENU_OPTIONS,
    MENU_CHEATS,
    MENU_SAVE,
    MENU_SETTINGS,
    MENU_SETTINGS_FILE,
    MENU_IN_GAME,
    MENU_BROWSE_DEVICE,
    MENU_GAME_SAVE,
    MENU_GAME_LOAD,
    MENU_EMULATION
};

#define THREAD_SLEEP 100

static GuiImageData * pointer[4];
static GuiImage * bgImg = NULL;
//static GuiSound * bgMusic = NULL;
static GuiWindow * mainWindow = NULL;
//static lwp_t guithread = LWP_THREAD_NULL;
static bool guiHalt = true;

//PTHREAD guithread = NULL;

static void UGUI();

GXColor ColorGrey = {104, 104, 104, 255};
GXColor ColorGrey2 = {49, 49, 49, 255};

// emulator option
OptionList options;

/****************************************************************************
 * ResumeGui
 *
 * Signals the GUI thread to start, and resumes the thread. This is called
 * after finishing the removal/insertion of new elements, and after initial
 * GUI setup.
 ***************************************************************************/
static void
_ResumeGui() {
    guiHalt = false;
    //	LWP_ResumeThread (guithread);
    //    thread_resume(guithread);
    //    printf("thread_resume %d \r\n",thread_resume(guithread));
}

/****************************************************************************
 * HaltGui
 *
 * Signals the GUI thread to stop, and waits for GUI thread to stop
 * This is necessary whenever removing/inserting new elements into the GUI.
 * This eliminates the possibility that the GUI is in the middle of accessing
 * an element that is being changed.
 ***************************************************************************/
static void
_HaltGui() {
    guiHalt = true;
    // wait for thread to finish
    //while (!thread_suspend(guithread))
    //    while(guithread->SuspendCount==0){
    //        udelay(50);
    //    }
}
//
#define ResumeGui(){TR;_ResumeGui();}
#define HaltGui(){TR;_HaltGui();}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label) {
    int choice = -1;

    //    GuiWindow promptWindow(448, 288);
    GuiWindow promptWindow(640, 360);
    promptWindow.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    promptWindow.SetPosition(0, -10);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiTrigger trigA;

    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    //    GuiImageData dialogBox(dialogue_box_png);
    GuiImageData dialogBox(xenon_popup_png);
    GuiImage dialogBoxImg(&dialogBox);

    GuiText titleTxt(title, 26, ColorGrey);
    titleTxt.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    titleTxt.SetPosition(0, 40);

    GuiText msgTxt(msg, 22, ColorGrey2);
    msgTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    msgTxt.SetPosition(0, -20);
    msgTxt.SetWrap(true, 600);

    GuiText btn1Txt(btn1Label, 22, ColorGrey);
    GuiImage btn1Img(&btnOutline);
    GuiImage btn1ImgOver(&btnOutlineOver);
    GuiButton btn1(btnOutline.GetWidth(), btnOutline.GetHeight());

    if (btn2Label) {
        btn1.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
        btn1.SetPosition(20, -25);
    } else {
        btn1.SetAlignment(ALIGN_CENTRE, ALIGN_BOTTOM);
        btn1.SetPosition(0, -25);
    }

    btn1.SetLabel(&btn1Txt);
    btn1.SetImage(&btn1Img);
    btn1.SetImageOver(&btn1ImgOver);
    btn1.SetSoundOver(&btnSoundOver);
    btn1.SetTrigger(&trigA);
    btn1.SetState(STATE_SELECTED);
    btn1.SetEffectGrow();

    GuiText btn2Txt(btn2Label, 22, ColorGrey);
    GuiImage btn2Img(&btnOutline);
    GuiImage btn2ImgOver(&btnOutlineOver);
    GuiButton btn2(btnOutline.GetWidth(), btnOutline.GetHeight());
    btn2.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    btn2.SetPosition(-20, -25);
    btn2.SetLabel(&btn2Txt);
    btn2.SetImage(&btn2Img);
    btn2.SetImageOver(&btn2ImgOver);
    btn2.SetSoundOver(&btnSoundOver);
    btn2.SetTrigger(&trigA);
    btn2.SetEffectGrow();

    promptWindow.Append(&dialogBoxImg);
    promptWindow.Append(&titleTxt);
    promptWindow.Append(&msgTxt);
    promptWindow.Append(&btn1);

    if (btn2Label)
        promptWindow.Append(&btn2);

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_IN, 50);
    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&promptWindow);
    mainWindow->ChangeFocus(&promptWindow);
    ResumeGui();

    while (choice == -1) {

        UGUI();
        udelay(THREAD_SLEEP);

        if (btn1.GetState() == STATE_CLICKED)
            choice = 1;
        else if (btn2.GetState() == STATE_CLICKED)
            choice = 0;
    }

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
    while (promptWindow.GetEffect() > 0) {
        UGUI();
        udelay(THREAD_SLEEP);
    }
    HaltGui();
    mainWindow->Remove(&promptWindow);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
    return choice;
}

void ErrorPrompt(const char *msg) {
    WindowPrompt("Error", msg, "OK", NULL);
}

void InfoPrompt(const char *msg) {
    WindowPrompt("Information", msg, "OK", NULL);
}

/****************************************************************************
 * UpdateGUI
 *
 * Primary thread to allow GUI to respond to state changes, and draws GUI
 ***************************************************************************/

void UGUI() {
    int i;
    UpdatePads();
    mainWindow->Draw();

#ifdef HW_RVL
    for (i = 3; i >= 0; i--) // so that player 1's cursor appears on top!
    {
        if (userInput[i].wpad->ir.valid)
            Menu_DrawImg(userInput[i].wpad->ir.x - 48, userInput[i].wpad->ir.y - 48,
                96, 96, pointer[i]->GetImage(), userInput[i].wpad->ir.angle, 1, 1, 255);
        DoRumble(i);
    }
#endif
    Menu_Render();

    for (i = 0; i < 4; i++)
        mainWindow->Update(&userInput[i]);
#if 0
    if (ExitRequested) {
        for (i = 0; i <= 255; i += 15) {
            mainWindow->Draw();

            Menu_DrawRectangle(0, 0, screenwidth, screenheight, (XeColor) {
                i, 0, 0, 0
            }, 1);
            Menu_Render();
        }
        ExitApp();
    }
#endif
}

static void *
UpdateGUI(void *arg) {
    int i;

    TR;
    while (1) {
        //TR;
        if (guiHalt) {

            //            thread_sleep(THREAD_SLEEP);
            //            printf("thread_suspend %d \r\n",thread_suspend(guithread));
        } else {
            TR;
            UGUI();
        }
    }
    return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
void
InitGUIThreads() {
    //    threading_init();
    //    guithread = thread_create((void*) UpdateGUI, 0, NULL, THREAD_FLAG_CREATE_SUSPENDED);
    //    thread_set_processor(guithread,2);
    //    thread_resume(guithread);
}

/****************************************************************************
 * OnScreenKeyboard
 *
 * Opens an on-screen keyboard window, with the data entered being stored
 * into the specified variable.
 ***************************************************************************/
static void OnScreenKeyboard(char * var, u16 maxlen) {
    int save = -1;

    GuiKeyboard keyboard(var, maxlen);

    GuiSound btnSoundOver(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);
    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiText okBtnTxt("OK", 22, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage okBtnImg(&btnOutline);
    GuiImage okBtnImgOver(&btnOutlineOver);
    GuiButton okBtn(btnOutline.GetWidth(), btnOutline.GetHeight());

    okBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    okBtn.SetPosition(25, -25);

    okBtn.SetLabel(&okBtnTxt);
    okBtn.SetImage(&okBtnImg);
    okBtn.SetImageOver(&okBtnImgOver);
    okBtn.SetSoundOver(&btnSoundOver);
    okBtn.SetTrigger(&trigA);
    okBtn.SetEffectGrow();

    GuiText cancelBtnTxt("Cancel", 22, (GXColor) {
        0, 0, 0, 255
    });
    GuiImage cancelBtnImg(&btnOutline);
    GuiImage cancelBtnImgOver(&btnOutlineOver);
    GuiButton cancelBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    cancelBtn.SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    cancelBtn.SetPosition(-25, -25);
    cancelBtn.SetLabel(&cancelBtnTxt);
    cancelBtn.SetImage(&cancelBtnImg);
    cancelBtn.SetImageOver(&cancelBtnImgOver);
    cancelBtn.SetSoundOver(&btnSoundOver);
    cancelBtn.SetTrigger(&trigA);
    cancelBtn.SetEffectGrow();

    keyboard.Append(&okBtn);
    keyboard.Append(&cancelBtn);

    HaltGui();
    mainWindow->SetState(STATE_DISABLED);
    mainWindow->Append(&keyboard);
    mainWindow->ChangeFocus(&keyboard);
    ResumeGui();

    while (save == -1) {
        UGUI();
        udelay(THREAD_SLEEP);

        if (okBtn.GetState() == STATE_CLICKED)
            save = 1;
        else if (cancelBtn.GetState() == STATE_CLICKED)
            save = 0;
    }

    if (save) {
        snprintf(var, maxlen, "%s", keyboard.kbtextstr);
    }

    HaltGui();
    mainWindow->Remove(&keyboard);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();
}

/****************************************************************************
 * MenuBrowseDevice
 ***************************************************************************/
static int MenuBrowseDevice() {
    char title[100];
    int i;

    ShutoffRumble();

    // populate initial directory listing
    CreateRomList();

    int menu = MENU_NONE;

    sprintf(title, "%s - Load Game", "mame.version");

    GuiText titleTxt(title, 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiRomBrowser romBrowser(1080, 496);
    romBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    romBrowser.SetPosition(0, 100);

    GuiImageData btnOutline(xenon_button_png);
    GuiImageData btnOutlineOver(xenon_button_over_png);

    GuiText backBtnTxt("Go Back", 18, ColorGrey2);
    GuiImage backBtnImg(&btnOutline);
    GuiImage backBtnImgOver(&btnOutlineOver);
    GuiButton backBtn(btnOutline.GetWidth(), btnOutline.GetHeight());
    backBtn.SetAlignment(ALIGN_LEFT, ALIGN_BOTTOM);
    backBtn.SetPosition(100, -35);
    backBtn.SetLabel(&backBtnTxt);
    backBtn.SetImage(&backBtnImg);
    backBtn.SetImageOver(&backBtnImgOver);
    backBtn.SetTrigger(&trigA);
    backBtn.SetEffectGrow();

    GuiWindow xenon_buttonWindow(screenwidth, screenheight);
    xenon_buttonWindow.Append(&backBtn);

    HaltGui();
    mainWindow->Append(&titleTxt);
    mainWindow->Append(&romBrowser);
    mainWindow->Append(&xenon_buttonWindow);
    ResumeGui();

    while (menu == MENU_NONE) {
        UGUI();
        udelay(THREAD_SLEEP);

        // update file browser based on arrow xenon_buttons
        // set MENU_EXIT if A xenon_button pressed on a file
        for (i = 0; i < FILE_PAGESIZE; i++) {
            if (romBrowser.fileList[i]->GetState() == STATE_CLICKED) {
                romBrowser.fileList[i]->ResetState();
                // check corresponding browser entry
                if (romList[rominfo.selIndex].is_clone) {
                    /*
                    if (BrowserChangeFolder()) {
                        romBrowser.ResetState();
                        romBrowser.fileList[0]->SetState(STATE_SELECTED);
                        romBrowser.TriggerUpdate();
                    } else {
                        menu = MENU_BROWSE_DEVICE;
                        break;
                    }
                     */ 
                } else {
                    ShutoffRumble();
                    romBrowser.ResetState();
                    //mainWindow->SetState(STATE_DISABLED);

                    menu = MENU_EMULATION;

//                    emu.Initialise();
//                    emu.OpenRom(rootdir, browser.dir, browserList[browser.selIndex].filename);
                    //mainWindow->SetState(STATE_DEFAULT);
                }
                
                int argc = 2;
                char * argv[]=
                {
                    "mame.elf",
                    romList[rominfo.selIndex].romname
                };
                
                for(int ppp=0;ppp<argc;ppp++)
                    printf("%s\r\n",argv[ppp]);
                
                extern int xenon_main(int argc, char * argv[]);
                xenon_main(argc,argv);
            }
        }
        if (backBtn.GetState() == STATE_CLICKED)
            menu = MENU_SETTINGS;
    }
    HaltGui();
    mainWindow->Remove(&titleTxt);
    mainWindow->Remove(&xenon_buttonWindow);
    mainWindow->Remove(&romBrowser);
    return menu;
}

/****************************************************************************
 * MainMenu
 ***************************************************************************/
void MainMenu(int menu) {
    TR;
    int currentMenu = menu;

    GuiImageData * background = new GuiImageData(xenon_bg_png);

    mainWindow = new GuiWindow(screenwidth, screenheight);

    bgImg = new GuiImage(background);

    //    bgImg = new GuiImage(screenwidth, screenheight, (GXColor) {
    //        50, 50, 50, 255
    //    });
    //    bgImg->ColorStripe(30);
    mainWindow->Append(bgImg);

    GuiTrigger trigA;
    //	trigA.SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    ResumeGui();

    //    bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
    //    bgMusic->SetVolume(50);
    //    bgMusic->Play(); // startup music

    while (currentMenu != MENU_EXIT) {
       currentMenu = MenuBrowseDevice();
    }
    ResumeGui();
//    ExitRequested = 1;

    while (1) {
        UGUI();
        udelay(THREAD_SLEEP);
    }

    HaltGui();

    //    bgMusic->Stop();
    //    delete bgMusic;
    delete bgImg;
    delete mainWindow;

    delete pointer[0];
    delete pointer[1];
    delete pointer[2];
    delete pointer[3];

    mainWindow = NULL;
}



int main(){
    // init video / usb / thread etc ...
    osd_xenon_init();
    
    InitFreeType((u8*) font_ttf, font_ttf_size); // Initialize font system
    
    InitVideo();
    // run gui
    
    MainMenu(MENU_BROWSE_DEVICE);
    
    return 0;
}