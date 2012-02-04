###########################################################################
#
#   osdmini.mak
#
#   Minimal OSD makefile
#
###########################################################################
#
#   Copyright Aaron Giles
#   All rights reserved.
#
#   Redistribution and use in source and binary forms, with or without
#   modification, are permitted provided that the following conditions are
#   met:
#
#       * Redistributions of source code must retain the above copyright
#         notice, this list of conditions and the following disclaimer.
#       * Redistributions in binary form must reproduce the above copyright
#         notice, this list of conditions and the following disclaimer in
#         the documentation and/or other materials provided with the
#         distribution.
#       * Neither the name 'MAME' nor the names of its contributors may be
#         used to endorse or promote products derived from this software
#         without specific prior written permission.
#
#   THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND ANY EXPRESS OR
#   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
#   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#   DISCLAIMED. IN NO EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
#   INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
#   (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
#   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
#   STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
#   IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
#   POSSIBILITY OF SUCH DAMAGE.
#
###########################################################################


#-------------------------------------------------
# object and source roots
#-------------------------------------------------

XENONSRC = $(SRC)/osd/$(OSD)
XENONOBJ = $(OBJ)/osd/$(OSD)

OBJDIRS += $(XENONOBJ)
OBJDIRS += $(XENONOBJ)/gui/
OBJDIRS += $(XENONOBJ)/gui/libwiigui/
OBJDIRS += $(XENONOBJ)/gui/images/
OBJDIRS += $(XENONOBJ)/gui/sounds/
OBJDIRS += $(XENONOBJ)/gui/lang/
OBJDIRS += $(XENONOBJ)/gui/fonts/

# VIDEO
VIDEOCOMMON = \
        $(XENONOBJ)/gui/video.o \
        $(XENONOBJ)/gui/video_mame.o \
        $(XENONOBJ)/gui/VecMatrix.o \
        $(XENONOBJ)/gui/VecVector.o \

define bin2o
	bin2s $< | xenon-as -a32 -o $(@)
endef

$(OBJ)/%.o: $(SRC)/%.cpp 
	@echo Compiling $<...
	$(CC) $(CDEFS) $(CFLAGS) -c $< -o $@
	
$(OBJ)/%.o : $(SRC)/%.png
	@echo $(notdir $<)
	$(bin2o)

$(OBJ)/%.o : $(SRC)/%.pcm
	@echo $(notdir $<)
	$(bin2o)
	
$(OBJ)/%.o : $(SRC)/%.lang
	@echo $(notdir $<)
	$(bin2o)
		
$(OBJ)/%.o : $(SRC)/%.ttf
	@echo $(notdir $<)
	$(bin2o)

GUI =\
	$(XENONOBJ)/gui/FreeTypeGX.o        $(XENONOBJ)/gui/gui_input.o        $(XENONOBJ)/gui/gui_wchar.o    $(XENONOBJ)/gui/video.o \
	$(XENONOBJ)/gui/gettext.o           $(XENONOBJ)/gui/gui_oggplayer.o      $(XENONOBJ)/gui/miss_.o        $(XENONOBJ)/gui/video_mame.o \
	$(XENONOBJ)/gui/gui_audio.o         $(XENONOBJ)/gui/gui_screencapture.o  $(XENONOBJ)/gui/pngu.o \
	$(XENONOBJ)/gui/gui_demo.o          $(XENONOBJ)/gui/gui_utils.o        $(XENONOBJ)/gui/VecMatrix.o \
	$(XENONOBJ)/gui/gui__filebrowser.o  $(XENONOBJ)/gui/VecVector.o         $(XENONOBJ)/gui/gui_romlist.o \
        $(XENONOBJ)/gui/libwiigui/gui_button.o       $(XENONOBJ)/gui/libwiigui/gui_optionbrowser.o \
        $(XENONOBJ)/gui/libwiigui/gui_element.o      $(XENONOBJ)/gui/libwiigui/gui_savebrowser.o \
        $(XENONOBJ)/gui/libwiigui/gui_filebrowser.o  $(XENONOBJ)/gui/libwiigui/gui_sound.o \
        $(XENONOBJ)/gui/libwiigui/gui_image.o        $(XENONOBJ)/gui/libwiigui/gui_text.o \
        $(XENONOBJ)/gui/libwiigui/gui_imagedata.o    $(XENONOBJ)/gui/libwiigui/gui_trigger.o \
        $(XENONOBJ)/gui/libwiigui/gui_keyboard.o     $(XENONOBJ)/gui/libwiigui/gui_window.o \
        $(XENONOBJ)/gui/libwiigui/gui_rombrowser.o \
        $(XENONOBJ)/gui/images/bg_file_selection_entry.o  $(XENONOBJ)/gui/images/player2_point.o \
        $(XENONOBJ)/gui/images/bg_file_selection.o        $(XENONOBJ)/gui/images/player3_grab.o \
        $(XENONOBJ)/gui/images/bg_options_entry.o         $(XENONOBJ)/gui/images/player3_point.o \
        $(XENONOBJ)/gui/images/bg_options.o               $(XENONOBJ)/gui/images/player4_grab.o \
        $(XENONOBJ)/gui/images/button_gamesave_blank.o    $(XENONOBJ)/gui/images/player4_point.o \
        $(XENONOBJ)/gui/images/button_gamesave_over.o     $(XENONOBJ)/gui/images/scrollbar_arrowdown_over.o \
        $(XENONOBJ)/gui/images/button_gamesave.o          $(XENONOBJ)/gui/images/scrollbar_arrowdown.o \
        $(XENONOBJ)/gui/images/button_large_over.o        $(XENONOBJ)/gui/images/scrollbar_arrowup_over.o \
        $(XENONOBJ)/gui/images/button_large.o             $(XENONOBJ)/gui/images/scrollbar_arrowup.o \
        $(XENONOBJ)/gui/images/button_over.o              $(XENONOBJ)/gui/images/scrollbar_box_over.o \
        $(XENONOBJ)/gui/images/button.o                   $(XENONOBJ)/gui/images/scrollbar_box.o \
        $(XENONOBJ)/gui/images/debug_xenon_bg.o           $(XENONOBJ)/gui/images/scrollbar.o \
        $(XENONOBJ)/gui/images/dialogue_box.o             $(XENONOBJ)/gui/images/xenon_bg_file_selection_entry.o \
        $(XENONOBJ)/gui/images/folder.o                   $(XENONOBJ)/gui/images/xenon_bg.o \
        $(XENONOBJ)/gui/images/keyboard_key_over.o        $(XENONOBJ)/gui/images/xenon_button_large_over.o \
        $(XENONOBJ)/gui/images/keyboard_key.o             $(XENONOBJ)/gui/images/xenon_button_large.o \
        $(XENONOBJ)/gui/images/keyboard_largekey_over.o   $(XENONOBJ)/gui/images/xenon_button_over.o \
        $(XENONOBJ)/gui/images/keyboard_largekey.o        $(XENONOBJ)/gui/images/xenon_button.o \
        $(XENONOBJ)/gui/images/keyboard_mediumkey_over.o  $(XENONOBJ)/gui/images/xenon_filebrowser.o \
        $(XENONOBJ)/gui/images/keyboard_mediumkey.o       $(XENONOBJ)/gui/images/xenon_popup.o \
        $(XENONOBJ)/gui/images/keyboard_textbox.o         $(XENONOBJ)/gui/images/xenon_scrollbar_arrowdown.o \
        $(XENONOBJ)/gui/images/player1_grab.o             $(XENONOBJ)/gui/images/xenon_scrollbar_arrowup.o \
        $(XENONOBJ)/gui/images/player1_point.o            $(XENONOBJ)/gui/images/xenon_scrollbar_box.o \
        $(XENONOBJ)/gui/images/player2_grab.o             $(XENONOBJ)/gui/images/xenon_scrollbar.o \
        $(XENONOBJ)/gui/sounds/button_click.o             $(XENONOBJ)/gui/sounds/button_over.o \
        $(XENONOBJ)/gui/lang/en.o $(XENONOBJ)/gui/fonts/font.o



VIDEOOBJ = \
        $(GUI) \
        $(XENONOBJ)/xenon_video.o \

VIDEOOBJHW = \
        $(GUI) \
        $(XENONOBJ)/xenon_video_hw.o \
		
#-------------------------------------------------
# OSD core library
#-------------------------------------------------

OSDCOREOBJS = \
	$(XENONOBJ)/xenondir.o \
	$(XENONOBJ)/xenonfile.o \
	$(XENONOBJ)/xenonmisc.o \
	$(XENONOBJ)/xenonsync.o \
	$(XENONOBJ)/xenontime.o \
	$(XENONOBJ)/xenonwork.o \
	$(XENONOBJ)/xenon_input.o \
	$(XENONOBJ)/xenon_sound.o \
	$(XENONOBJ)/xenon.o \
        $(VIDEOOBJ) \
	


#-------------------------------------------------
# OSD mini library
#-------------------------------------------------

OSDOBJS = \
        $(XENONOBJ)/xenonmain.o \
        $(XENONOBJ)/xenon_gui.o \



#-------------------------------------------------
# rules for building the libaries
#-------------------------------------------------

$(LIBOCORE): $(OSDCOREOBJS)

$(LIBOSD): $(OSDOBJS)
