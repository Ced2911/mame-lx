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


void ShowProgress (const char *msg, int done, int total);
void CreateRomList(){
    if(romList)
        free(romList);
    
    int found = 0;
    int count = driver_list::total();
    
    char progress_str[200];
    
    romList = (ROMENTRY *) malloc(count * sizeof (ROMENTRY));
    
    ROMENTRY * current_entry = romList;
    
    for(int i=0;i<count;i++){
        
        //ShowProgress("Scanning dir",i,count);
        
        const game_driver & driver = driver_list::driver(i);
        
        snprintf(progress_str,200,"Scaning %s",driver.name);
        
        ShowProgress(progress_str,i,count);
       
        if(found_rom(driver.name) && (driver.flags & GAME_IS_BIOS_ROOT)==0){
            // add a new game
            strncpy( current_entry[0].displayname, driver.description,MAXDISPLAY);
            strncpy( current_entry[0].romname, driver.name,MAXJOLIET);
            
            // unix path
            const char * source_file = strrchr(driver.source_file,'/');
            if(source_file==NULL)
            {
                //windows path
                source_file = strrchr(driver.source_file,'\\');
            }
            
            strncpy( current_entry[0].systemname, source_file+1,MAXJOLIET);
            
            strncpy( current_entry[0].year, driver.year,MAXDISPLAY);
            strncpy( current_entry[0].manufacturer, driver.manufacturer,MAXDISPLAY);
            strncpy( current_entry[0].parent, driver.parent,MAXDISPLAY);
            
            if(driver.parent){
                current_entry[0].is_clone = (driver.parent[0]=='0')?0:1;
            }
            else
            {
                current_entry[0].is_clone=0;
            }
            if(current_entry[0].is_clone)
            {
                strncpy(current_entry[0].artname,driver.parent,10);
            }
            else
            {
                strncpy(current_entry[0].artname,driver.name,10);
            }

            //printf("driver.name = %s\r\n",driver.name);

            current_entry++;
            found++;
        }
    }
    
    rominfo.numEntries = found;
}



