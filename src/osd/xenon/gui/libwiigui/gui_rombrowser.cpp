/****************************************************************************
 * libwiigui
 *
 * Tantric 2009
 *
 * gui_filerominfo.cpp
 *
 * GUI class definitions
 ***************************************************************************/

#include "gui.h"
#include "../gui_romlist.h"
#include "../w_input.h"
#include "../gui_debug.h"

//#define FILE_PAGESIZE 20

/**
 * Constructor for the GuiRomBrowser class.
 */
GuiRomBrowser::GuiRomBrowser(int w, int h) {
    width = w;
    height = h;
    numEntries = 0;
    selectedItem = 0;
    selectable = true;
    listChanged = true; // trigger an initial list update
    focus = 0; // allow focus

    trigA = new GuiTrigger;
    //	trigA->SetSimpleTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigA->SetSimpleTrigger(-1, 0, PAD_BUTTON_A);
    trig2 = new GuiTrigger;
    //	trig2->SetSimpleTrigger(-1, WPAD_BUTTON_2, 0);
    trig2->SetSimpleTrigger(-1, 0, 0);

    trigHeldA = new GuiTrigger;
    //	trigHeldA->SetHeldTrigger(-1, WPAD_BUTTON_A | WPAD_CLASSIC_BUTTON_A, PAD_BUTTON_A);
    trigHeldA->SetHeldTrigger(-1, 0, PAD_BUTTON_A);

    btnSoundOver = new GuiSound(button_over_pcm, button_over_pcm_size, SOUND_PCM);
    btnSoundClick = new GuiSound(button_click_pcm, button_click_pcm_size, SOUND_PCM);

    bgFileSelection = new GuiImageData(xenon_filebrowser_png);
    bgFileSelectionImg = new GuiImage(bgFileSelection);
    bgFileSelectionImg->SetParent(this);
    bgFileSelectionImg->SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);

    bgFileSelectionEntry = new GuiImageData(xenon_bg_file_selection_entry_png);
    fileFolder = new GuiImageData(folder_png);

    scrollbar = new GuiImageData(xenon_scrollbar_png);
    scrollbarImg = new GuiImage(scrollbar);
    scrollbarImg->SetParent(this);
    scrollbarImg->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    scrollbarImg->SetPosition(0, 30);

    arrowDown = new GuiImageData(xenon_scrollbar_arrowdown_png);
    arrowDownImg = new GuiImage(arrowDown);
    arrowDownOver = new GuiImageData(scrollbar_arrowdown_over_png);
    arrowDownOverImg = new GuiImage(arrowDownOver);
    arrowUp = new GuiImageData(xenon_scrollbar_arrowup_png);
    arrowUpImg = new GuiImage(arrowUp);
    arrowUpOver = new GuiImageData(scrollbar_arrowup_over_png);
    arrowUpOverImg = new GuiImage(arrowUpOver);
    scrollbarBox = new GuiImageData(xenon_scrollbar_box_png);
    scrollbarBoxImg = new GuiImage(scrollbarBox);
    scrollbarBoxOver = new GuiImageData(scrollbar_box_over_png);
    scrollbarBoxOverImg = new GuiImage(scrollbarBoxOver);

    arrowUpBtn = new GuiButton(arrowUpImg->GetWidth(), arrowUpImg->GetHeight());
    arrowUpBtn->SetParent(this);
    arrowUpBtn->SetImage(arrowUpImg);
    arrowUpBtn->SetImageOver(arrowUpOverImg);
    arrowUpBtn->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    arrowUpBtn->SetPosition(0, -2);
    arrowUpBtn->SetSelectable(false);
    arrowUpBtn->SetClickable(false);
    arrowUpBtn->SetHoldable(true);
    arrowUpBtn->SetTrigger(trigHeldA);
    arrowUpBtn->SetSoundOver(btnSoundOver);
    arrowUpBtn->SetSoundClick(btnSoundClick);

    arrowDownBtn = new GuiButton(arrowDownImg->GetWidth(), arrowDownImg->GetHeight());
    arrowDownBtn->SetParent(this);
    arrowDownBtn->SetImage(arrowDownImg);
    arrowDownBtn->SetImageOver(arrowDownOverImg);
    arrowDownBtn->SetAlignment(ALIGN_RIGHT, ALIGN_BOTTOM);
    arrowDownBtn->SetSelectable(false);
    arrowDownBtn->SetClickable(false);
    arrowDownBtn->SetHoldable(true);
    arrowDownBtn->SetTrigger(trigHeldA);
    arrowDownBtn->SetSoundOver(btnSoundOver);
    arrowDownBtn->SetSoundClick(btnSoundClick);

    scrollbarBoxBtn = new GuiButton(scrollbarBoxImg->GetWidth(), scrollbarBoxImg->GetHeight());
    scrollbarBoxBtn->SetParent(this);
    scrollbarBoxBtn->SetImage(scrollbarBoxImg);
    scrollbarBoxBtn->SetImageOver(scrollbarBoxOverImg);
    scrollbarBoxBtn->SetAlignment(ALIGN_RIGHT, ALIGN_TOP);
    scrollbarBoxBtn->SetMinY(0);
//    scrollbarBoxBtn->SetMaxY(130);
//    scrollbarBoxBtn->SetMaxY(360);
    scrollbarBoxBtn->SetSelectable(false);
    scrollbarBoxBtn->SetClickable(false);
    scrollbarBoxBtn->SetHoldable(true);
    scrollbarBoxBtn->SetTrigger(trigHeldA);

    for (int i = 0; i < FILE_PAGESIZE; ++i) {

        fileListText[i] = new GuiText(NULL, 20, (GXColor) {
            0, 0, 0, 0xff
        });
        fileListText[i]->SetAlignment(ALIGN_LEFT, ALIGN_MIDDLE);
        fileListText[i]->SetPosition(5, 0);
        fileListText[i]->SetMaxWidth(1050);

        fileListBg[i] = new GuiImage(bgFileSelectionEntry);
        fileListFolder[i] = new GuiImage(fileFolder);

        fileList[i] = new GuiButton(1050, 30);
        fileList[i]->SetParent(this);
        fileList[i]->SetLabel(fileListText[i]);
        fileList[i]->SetImageOver(fileListBg[i]);
        fileList[i]->SetPosition(2, 30 * i + 3);
        fileList[i]->SetTrigger(trigA);
        fileList[i]->SetTrigger(trig2);
        fileList[i]->SetSoundClick(btnSoundClick);
    }
}

/**
 * Destructor for the GuiFileBrowser class.
 */
GuiRomBrowser::~GuiRomBrowser() {
    delete arrowUpBtn;
    delete arrowDownBtn;
    delete scrollbarBoxBtn;

    delete bgFileSelectionImg;
    delete scrollbarImg;
    delete arrowDownImg;
    delete arrowDownOverImg;
    delete arrowUpImg;
    delete arrowUpOverImg;
    delete scrollbarBoxImg;
    delete scrollbarBoxOverImg;

    delete bgFileSelection;
    delete bgFileSelectionEntry;
    delete fileFolder;
    delete scrollbar;
    delete arrowDown;
    delete arrowDownOver;
    delete arrowUp;
    delete arrowUpOver;
    delete scrollbarBox;
    delete scrollbarBoxOver;

    delete btnSoundOver;
    delete btnSoundClick;
    delete trigHeldA;
    delete trigA;
    delete trig2;

    for (int i = 0; i < FILE_PAGESIZE; i++) {
        delete fileListText[i];
        delete fileList[i];
        delete fileListBg[i];
        delete fileListFolder[i];
    }
}

void GuiRomBrowser::SetFocus(int f) {
    focus = f;

    for (int i = 0; i < FILE_PAGESIZE; i++)
        fileList[i]->ResetState();

    if (f == 1)
        fileList[selectedItem]->SetState(STATE_SELECTED);
}

void GuiRomBrowser::ResetState() {
    state = STATE_DEFAULT;
    stateChan = -1;
    selectedItem = 0;

    for (int i = 0; i < FILE_PAGESIZE; i++) {
        fileList[i]->ResetState();
    }
}

void GuiRomBrowser::TriggerUpdate() {
    listChanged = true;
}

/**
 * Draw the button on screen
 */
void GuiRomBrowser::Draw() {
    if (!this->IsVisible())
        return;

    bgFileSelectionImg->Draw();

    for (u32 i = 0; i < FILE_PAGESIZE; ++i) {
        fileList[i]->Draw();
    }

    scrollbarImg->Draw();
    arrowUpBtn->Draw();
    arrowDownBtn->Draw();
    scrollbarBoxBtn->Draw();

    this->UpdateEffects();
}

void GuiRomBrowser::DrawTooltip() {
}

void GuiRomBrowser::Update(GuiTrigger * t) {
    if (state == STATE_DISABLED || !t)
        return;

    int position = 0;
    int positionWiimote = 0;

    arrowUpBtn->Update(t);
    arrowDownBtn->Update(t);
    scrollbarBoxBtn->Update(t);

    // move the file listing to respond to wiimote cursor movement
    if (scrollbarBoxBtn->GetState() == STATE_HELD &&
            scrollbarBoxBtn->GetStateChan() == t->chan &&
            t->wpad->ir.valid &&
            rominfo.numEntries > FILE_PAGESIZE
            ) {
        scrollbarBoxBtn->SetPosition(0, 0);
        positionWiimote = t->wpad->ir.y - 60 - scrollbarBoxBtn->GetTop();

        if (positionWiimote < scrollbarBoxBtn->GetMinY())
            positionWiimote = scrollbarBoxBtn->GetMinY();
        else if (positionWiimote > scrollbarBoxBtn->GetMaxY())
            positionWiimote = scrollbarBoxBtn->GetMaxY();

        rominfo.pageIndex = (positionWiimote * rominfo.numEntries) / 130.0 - selectedItem;

        if (rominfo.pageIndex <= 0) {
            rominfo.pageIndex = 0;
        } else if (rominfo.pageIndex + FILE_PAGESIZE >= rominfo.numEntries) {
            rominfo.pageIndex = rominfo.numEntries - FILE_PAGESIZE;
        }
        listChanged = true;
        focus = false;
    }

    if (arrowDownBtn->GetState() == STATE_HELD && arrowDownBtn->GetStateChan() == t->chan) {
        t->wpad->btns_d |= WPAD_BUTTON_DOWN;
        if (!this->IsFocused())
            ((GuiWindow *)this->GetParent())->ChangeFocus(this);
    } else if (arrowUpBtn->GetState() == STATE_HELD && arrowUpBtn->GetStateChan() == t->chan) {
        t->wpad->btns_d |= WPAD_BUTTON_UP;
        if (!this->IsFocused())
            ((GuiWindow *)this->GetParent())->ChangeFocus(this);
    }

    // pad/joystick navigation
    if (!focus) {
        goto endNavigation; // skip navigation
        listChanged = false;
    }

    if (t->Right()) {
        if (rominfo.pageIndex < rominfo.numEntries && rominfo.numEntries > FILE_PAGESIZE) {
            rominfo.pageIndex += FILE_PAGESIZE;
            if (rominfo.pageIndex + FILE_PAGESIZE >= rominfo.numEntries)
                rominfo.pageIndex = rominfo.numEntries - FILE_PAGESIZE;
            listChanged = true;
        }
    } else if (t->Left()) {
        if (rominfo.pageIndex > 0) {
            rominfo.pageIndex -= FILE_PAGESIZE;
            if (rominfo.pageIndex < 0)
                rominfo.pageIndex = 0;
            listChanged = true;
        }
    } else if (t->Down()) {
        if (rominfo.pageIndex + selectedItem + 1 < rominfo.numEntries) {
            if (selectedItem == FILE_PAGESIZE - 1) {
                // move list down by 1
                ++rominfo.pageIndex;
                listChanged = true;
            } else if (fileList[selectedItem + 1]->IsVisible()) {
                fileList[selectedItem]->ResetState();
                fileList[++selectedItem]->SetState(STATE_SELECTED, t->chan);
            }
        }
    } else if (t->Up()) {
        if (selectedItem == 0 && rominfo.pageIndex + selectedItem > 0) {
            // move list up by 1
            --rominfo.pageIndex;
            listChanged = true;
        } else if (selectedItem > 0) {
            fileList[selectedItem]->ResetState();
            fileList[--selectedItem]->SetState(STATE_SELECTED, t->chan);
        }
    }

endNavigation:

    for (int i = 0; i < FILE_PAGESIZE; ++i) {
        if (listChanged || numEntries != rominfo.numEntries) {
            if (rominfo.pageIndex + i < rominfo.numEntries) {
                if (fileList[i]->GetState() == STATE_DISABLED)
                    fileList[i]->SetState(STATE_DEFAULT);

                fileList[i]->SetVisible(true);

                fileListText[i]->SetText(romList[rominfo.pageIndex + i].displayname);

                if (romList[rominfo.pageIndex + i].is_clone) // directory
                {
                    fileList[i]->SetIcon(fileListFolder[i]);
                    fileListText[i]->SetPosition(30, 0);
                } else {
                    fileList[i]->SetIcon(NULL);
                    fileListText[i]->SetPosition(10, 0);
                }
            } else {
                fileList[i]->SetVisible(false);
                fileList[i]->SetState(STATE_DISABLED);
            }
        }

        if (i != selectedItem && fileList[i]->GetState() == STATE_SELECTED)
            fileList[i]->ResetState();
        else if (focus && i == selectedItem && fileList[i]->GetState() == STATE_DEFAULT)
            fileList[selectedItem]->SetState(STATE_SELECTED, t->chan);

        int currChan = t->chan;

        if (t->wpad->ir.valid && !fileList[i]->IsInside(t->wpad->ir.x, t->wpad->ir.y))
            t->chan = -1;

        fileList[i]->Update(t);
        t->chan = currChan;

        if (fileList[i]->GetState() == STATE_SELECTED) {
            selectedItem = i;
            rominfo.selIndex = rominfo.pageIndex + i;
        }

        if (selectedItem == i)
            fileListText[i]->SetScroll(SCROLL_HORIZONTAL);
        else
            fileListText[i]->SetScroll(SCROLL_NONE);
    }

    // update the location of the scroll box based on the position in the file list
    if (positionWiimote > 0) {
        position = positionWiimote; // follow wiimote cursor
        scrollbarBoxBtn->SetPosition(0, position + 36);
    } else if (listChanged || numEntries != rominfo.numEntries) {
        
//        if (float((rominfo.pageIndex << 1)) / (float(FILE_PAGESIZE)) < 1.0) {
        if ((rominfo.pageIndex)==0){
            position = 0;
        } else if (rominfo.pageIndex + FILE_PAGESIZE >= rominfo.numEntries) {
            position = 380;
        } else {
            position = 380 * (rominfo.pageIndex + FILE_PAGESIZE / 2) / (float) rominfo.numEntries;
        } 
        scrollbarBoxBtn->SetPosition(0, position + 36);
        
        scrollbarBoxBtn->SetPosition(0, position + 36);
    }

    listChanged = false;
    numEntries = rominfo.numEntries;

    if (updateCB)
        updateCB(this);
}