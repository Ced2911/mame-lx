#include <stdio.h>
#include <stdlib.h>
#include "gui_romlist.h"

#include "emu.h"
#include "osdnet.h"
#include "clifront.h"
#include "emuopts.h"
#include "driver.h"

#include "../xenon.h"

ROMLISTINFO rominfo;
ROMENTRY * romList = NULL;

static int found_rom(const char * romname){
    int ret = 0;
    char filename[256];
    sprintf(filename,"uda:/roms/%s.zip",romname);
    FILE * fd = fopen(filename,"rb");
    if(fd)
    {
        ret = 1;
        fclose(fd);
    }
    return ret;
}

void CreateRomList(){
    if(romList)
        free(romList);
    
    int found = 0;
    int count = driver_list::total();
    
    
    romList = (ROMENTRY *) malloc(count * sizeof (ROMENTRY));
    
    ROMENTRY * current_entry = romList;
    
    for(int i=0;i<count;i++){
        
        const game_driver & driver = driver_list::driver(i);
       
        if(found_rom(driver.name)){
            // add a new game
            strncpy( current_entry[0].displayname, driver.description,MAXDISPLAY);
            strncpy( current_entry[0].romname, driver.name,MAXJOLIET);
            strncpy( current_entry[0].systemname, driver.source_file,MAXJOLIET);
            current_entry[0].is_clone = (driver.parent==NULL)?0:1;

            //printf("driver.name = %s\r\n",driver.name);

            current_entry++;
            found++;
        }
    }
    
    rominfo.numEntries = found;
}



