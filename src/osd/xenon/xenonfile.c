//============================================================
//
//  minifile.c - Minimal core file access functions
//
//============================================================
//
//  Copyright Aaron Giles
//  All rights reserved.
//
//  Redistribution and use in source and binary forms, with or
//  without modification, are permitted provided that the
//  following conditions are met:
//
//    * Redistributions of source code must retain the above
//      copyright notice, this list of conditions and the
//      following disclaimer.
//    * Redistributions in binary form must reproduce the
//      above copyright notice, this list of conditions and
//      the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//    * Neither the name 'MAME' nor the names of its
//      contributors may be used to endorse or promote
//      products derived from this software without specific
//      prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY AARON GILES ''AS IS'' AND
//  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
//  FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
//  EVENT SHALL AARON GILES BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
//  DAMAGE (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
//  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
//  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
//  ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
//  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
//  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
//  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//============================================================

#include "osdcore.h"
#include <stdlib.h>
#include <debug.h>

extern "C" int mkdir(const char *pathname, mode_t mode);

#define PATHSEPCH '/'
#define NO_ERROR	(0)

static UINT32 create_path_recursive(char *path) {
    printf("create_path_recursive : %s\r\n",path);
    char *sep = strrchr(path, PATHSEPCH);
    UINT32 filerr;

    // if there's still a separator, and it's not the root, nuke it and recurse
    if (sep != NULL && sep > path && sep[0] != ':' && sep[-1] != PATHSEPCH) {
        *sep = 0;
        filerr = create_path_recursive(path);
        *sep = PATHSEPCH;
        if (filerr != NO_ERROR)
            return filerr;
    }
/*
    // create the path
    if (mkdir(path, 0777) != 0)
        return -1;
 */ 
    return NO_ERROR;
}

static void CleanupPath(char * path) {
    if (!path || path[0] == 0)
        return;

    int pathlen = strlen(path);
    int j = 0;
    for (int i = 0; i < pathlen && i < 1024; i++) {
        if (path[i] == '\\')
            path[i] = '/';

        if (j == 0 || !(path[j - 1] == '/' && path[i] == '/'))
            path[j++] = path[i];
    }
    path[j] = 0;
}

//============================================================
//  osd_open
//============================================================

file_error osd_open(const char *_path, UINT32 openflags, osd_file **file, UINT64 *filesize) {

    //printf("osd_open : %s\r\n", _path);

    const char *mode;
    FILE *fileptr;
    char path[1024];

    if (strncmp("uda:", _path, 4)) {
        sprintf(path, "uda:/%s", _path);
    } else {
        strcpy(path, _path);
    }

    CleanupPath(path);

    // based on the flags, choose a mode
    if (openflags & OPEN_FLAG_WRITE) {
        if (openflags & OPEN_FLAG_READ)
            mode = (openflags & OPEN_FLAG_CREATE) ? "w+b" : "r+b";
        else
            mode = "wb";
    } else if (openflags & OPEN_FLAG_READ)
        mode = "rb";
    else {
        return FILERR_INVALID_ACCESS;
    }
    
    if ((openflags & OPEN_FLAG_CREATE) && (openflags & OPEN_FLAG_CREATE_PATHS)) {
        char *pathsep = strrchr(path, PATHSEPCH);
        if (pathsep != NULL) {
            int error;

            // create the path up to the file
            *pathsep = 0;
            error = create_path_recursive(path);
            *pathsep = PATHSEPCH;
        }
    } 

    // open the file
    fileptr = fopen(path, mode);
   
    if(fileptr == NULL){
         //printf("osd_open : %s not found\r\n", path);
        return FILERR_NOT_FOUND;
    }

    // store the file pointer directly as an osd_file
    *file = (osd_file *) fileptr;

    // get the size -- note that most fseek/ftell implementations are limited to 32 bits
    fseek(fileptr, 0, SEEK_END);
    *filesize = ftell(fileptr);
    fseek(fileptr, 0, SEEK_SET);

    //printf("osd_open : %s\r\n", path);

    return FILERR_NONE;
}


//============================================================
//  osd_close
//============================================================

file_error osd_close(osd_file *file) {
    // close the file handle
    fclose((FILE *) file);
    return FILERR_NONE;
}


//============================================================
//  osd_read
//============================================================

file_error osd_read(osd_file *file, void *buffer, UINT64 offset, UINT32 length, UINT32 *actual) {
    size_t count;

    // seek to the new location; note that most fseek implementations are limited to 32 bits
    fseek((FILE *) file, offset, SEEK_SET);

    // perform the read
    count = fread(buffer, 1, length, (FILE *) file);
    if (actual != NULL)
        *actual = count;

    return FILERR_NONE;
}


//============================================================
//  osd_write
//============================================================

file_error osd_write(osd_file *file, const void *buffer, UINT64 offset, UINT32 length, UINT32 *actual) {    
    size_t count;

    // seek to the new location; note that most fseek implementations are limited to 32 bits
    fseek((FILE *) file, offset, SEEK_SET);

    // perform the write
    count = fwrite(buffer, 1, length, (FILE *) file);
    if (actual != NULL)
        *actual = count;
    return FILERR_NONE;
}


//============================================================
//  osd_rmfile
//============================================================

file_error osd_rmfile(const char *filename) {
    return remove(filename) ? FILERR_FAILURE : FILERR_NONE;
}


//============================================================
//  osd_get_physical_drive_geometry
//============================================================

int osd_get_physical_drive_geometry(const char *filename, UINT32 *cylinders, UINT32 *heads, UINT32 *sectors, UINT32 *bps) {
    // there is no standard way of doing this, so we always return FALSE, indicating
    // that a given path is not a physical drive
    return FALSE;
}


//============================================================
//  osd_uchar_from_osdchar
//============================================================

int osd_uchar_from_osdchar(UINT32 /* unicode_char */ *uchar, const char *osdchar, size_t count) {
    // we assume a standard 1:1 mapping of characters to the first 256 unicode characters
    *uchar = (UINT8) * osdchar;
    return 1;
}


//============================================================
//  osd_stat
//============================================================

osd_directory_entry *osd_stat(const char *path) {
    osd_directory_entry *result = NULL;

    // create an osd_directory_entry; be sure to make sure that the caller can
    // free all resources by just freeing the resulting osd_directory_entry
    result = (osd_directory_entry *) malloc(sizeof (*result) + strlen(path) + 1);
    strcpy((char *) (result + 1), path);
    result->name = (char *) (result + 1);
    result->type = ENTTYPE_NONE;
    result->size = 0;

    FILE *f = fopen(path, "rb");
    if (f != NULL) {
        fseek(f, 0, SEEK_END);
        result->type = ENTTYPE_FILE;
        result->size = ftell(f);
        fclose(f);
    }
    return result;
}


//============================================================
//  osd_get_full_path
//============================================================

file_error osd_get_full_path(char **dst, const char *path) {
    // derive the full path of the file in an allocated string
    // for now just fake it since we don't presume any underlying file system
    *dst = strdup(path);
    return FILERR_NONE;
}


//============================================================
//  osd_get_volume_name
//============================================================

const char *osd_get_volume_name(int idx) {
    // we don't expose volumes
    return NULL;
}
