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

enum {
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
static bool guiHalt = true;

static void UGUI();

GXColor ColorGrey = {104, 104, 104, 255};
GXColor ColorGrey2 = {49, 49, 49, 255};
GXColor ColorWhite = {255, 255, 255, 255};

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
    udelay(THREAD_SLEEP);
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
    udelay(THREAD_SLEEP);
}
//
#define ResumeGui(){TR;_ResumeGui();}
#define HaltGui(){TR;_HaltGui();}

static GuiImage * mameImg = NULL;

void AddMameSurf() {
    HaltGui();
    XenosSurface * mame_surf = osd_xenon_get_surface();

    if (mame_surf) {
        mameImg = new GuiImage(mame_surf, screenwidth, screenheight);
        mainWindow->Append(mameImg);
    }
    ResumeGui();
}

void RemoveMameSurf() {
    HaltGui();
    if (mameImg) {
        mainWindow->Remove(mameImg);
        delete mameImg;
    }
    ResumeGui();
}

/****************************************************************************
 * WindowPrompt
 *
 * Displays a prompt window to user, with information, an error message, or
 * presenting a user with a choice
 ***************************************************************************/
int
WindowPrompt(const char *title, const char *msg, const char *btn1Label, const char *btn2Label) {
    int choice = -1;

    AddMameSurf();

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
        udelay(THREAD_SLEEP);

        if (btn1.GetState() == STATE_CLICKED)
            choice = 1;
        else if (btn2.GetState() == STATE_CLICKED)
            choice = 0;
    }

    promptWindow.SetEffect(EFFECT_SLIDE_TOP | EFFECT_SLIDE_OUT, 50);
    while (promptWindow.GetEffect() > 0) {
        udelay(THREAD_SLEEP);
    }
    HaltGui();
    mainWindow->Remove(&promptWindow);
    mainWindow->SetState(STATE_DEFAULT);
    ResumeGui();

    RemoveMameSurf();

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
void TH_UGUI() {
    int i;
    
    UpdatePads();
    mainWindow->Draw();
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

static void * UpdateGUI() {
    while (1) {
        if (guiHalt) {
            udelay(THREAD_SLEEP);
        } else {
            TH_UGUI();
        }
    }
    return NULL;
}

/****************************************************************************
 * InitGUIThread
 *
 * Startup GUI threads
 ***************************************************************************/
static unsigned char gui_thread_stack[0x10000];

void InitGUIThreads() {
    xenon_run_thread_task(5, &gui_thread_stack[sizeof (gui_thread_stack) - 0x100], (void*) UpdateGUI);
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

static int progress_done = 0;
static int progress_total = 0;
static char progress_str[200];
int ShowProgress (const char *msg, int done, int total){
    progress_done = done;
    progress_total = total;
    snprintf(progress_str,200,"%s %d/%d",msg,done,total);
}

static void ProgressUpdateCallback(void * e){
    GuiText * _this = (GuiText *)e;
    _this->SetText(progress_str);
}

/****************************************************************************
 * MenuBrowseDevice
 ***************************************************************************/
static int MenuBrowseDevice() {
    static int first_run = 1;
    char title[100];
    int i;

    ShutoffRumble();

    int menu = MENU_NONE;
    extern const char build_version[];
    sprintf(title, "%s %s  - Load Game", emulator_info::get_applongname(), build_version);

    GuiText titleTxt(title, 28, ColorGrey);
    titleTxt.SetAlignment(ALIGN_LEFT, ALIGN_TOP);
    titleTxt.SetPosition(50, 50);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    GuiRomBrowser romBrowser(1080, 496);
    romBrowser.SetAlignment(ALIGN_CENTRE, ALIGN_TOP);
    romBrowser.SetPosition(0, 100);
    
    GuiText progressTxt("progress ...", 20, ColorWhite);
    progressTxt.SetAlignment(ALIGN_CENTRE, ALIGN_MIDDLE);
    progressTxt.SetPosition(0, 0);
    progressTxt.SetUpdateCallback(ProgressUpdateCallback);
    
    HaltGui();
    mainWindow->Append(&progressTxt);
    mainWindow->Append(&titleTxt);
    ResumeGui();
    
    if (first_run) {
        // populate initial directory listing
        CreateRomList();
        first_run = 0;
    }

    HaltGui();
    mainWindow->Remove(&progressTxt);
    mainWindow->Append(&romBrowser);
    ResumeGui();
    
    
    while (menu == MENU_NONE) {
        udelay(THREAD_SLEEP);

        for (i = 0; i < FILE_PAGESIZE; i++) {
            if (romBrowser.fileList[i]->GetState() == STATE_CLICKED) {
                romBrowser.fileList[i]->ResetState();

                ShutoffRumble();
                romBrowser.ResetState();
                menu = MENU_EMULATION;

                int argc = 2;
                char * argv[] = {
                    "mame.elf",
                    romList[rominfo.selIndex].romname
                };

                for (int ppp = 0; ppp < argc; ppp++)
                    printf("%s\r\n", argv[ppp]);

                extern int xenon_main(int argc, char * argv[]);
                HaltGui();
                xenon_main(argc, argv);
                ResumeGui();
            }
        }
    }
    HaltGui();
    mainWindow->Remove(&titleTxt);
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

    mainWindow->Append(bgImg);

    GuiTrigger trigA;
    trigA.SetSimpleTrigger(-1, 0, PAD_BUTTON_A);

    ResumeGui();

    //    bgMusic = new GuiSound(bg_music_ogg, bg_music_ogg_size, SOUND_OGG);
    //    bgMusic->SetVolume(50);
    //    bgMusic->Play(); // startup music

    while (currentMenu != MENU_EXIT) {
        currentMenu = MenuBrowseDevice();
    }
    ResumeGui();

    while (1) {
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

int main() {
    // init video / usb / thread etc ...
    osd_xenon_init();

    InitFreeType((u8*) font_ttf, font_ttf_size); // Initialize font system

    InitVideo();
    // run gui
    InitGUIThreads();

    MainMenu(MENU_BROWSE_DEVICE);

    return 0;
}