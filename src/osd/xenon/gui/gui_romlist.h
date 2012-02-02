#pragma once

#define MAXJOLIET 255
#define MAXDISPLAY 90

typedef struct {
    char is_clone; // 0 - file, 1 - directory
    char is_found;
    char systemname[MAXJOLIET + 1];
    char romname[MAXJOLIET + 1]; // full filename
    char displayname[MAXDISPLAY + 1]; // name for browser display
    char year[MAXDISPLAY];
    char manufacturer[MAXDISPLAY];
    char parent[MAXDISPLAY];
    char artname[10]; // name of the art (snap/arwork/flyers filename)
} ROMENTRY;

typedef struct {
    char dir[255]; // directory path of browserList
    int numEntries; // # of entries in browserList
    int selIndex; // currently selected index of browserList
    int pageIndex; // starting index of browserList page display
} ROMLISTINFO;

extern ROMLISTINFO rominfo;
extern ROMENTRY * romList;

void CreateRomList();