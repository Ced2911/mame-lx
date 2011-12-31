//============================================================
//
//  minidir.c - Minimal core directory access functions
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
#include <malloc.h>
#include <diskio/diskio.h>
#include <dirent.h>

struct _osd_directory {
    osd_directory_entry ent;
    struct dirent *data;
    DIR *fd;
    char *path;
};

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
//  osd_opendir
//============================================================

osd_directory *osd_opendir(const char *dirname) {
    char path[1024];

    // add uda
    if (strncmp("uda:", dirname, 4)) {
        sprintf(path, "uda:/%s", dirname);
    } else {
        strcpy(path, dirname);
    }

    CleanupPath(path);

    osd_directory *dir = NULL;
    dir = (osd_directory *) malloc(sizeof (osd_directory));
    memset(dir, 0, sizeof (osd_directory));

    dir->path = (char*) malloc(strlen(path));
    strcpy(dir->path, path);

    dir->fd = opendir(dir->path);

    if (dir->fd == NULL) {
        free(dir->path);
        free(dir);
        dir = NULL;
    }

    return dir;
}


//============================================================
//  osd_readdir
//============================================================

const osd_directory_entry *osd_readdir(osd_directory *dir) {
    char *temp;
    dir->data = readdir(dir->fd);

    if (dir->data == NULL)
        return NULL;

    dir->ent.name = dir->data->d_name;
    dir->ent.type = (dir->data->d_type & DT_DIR) ? ENTTYPE_DIR : ENTTYPE_FILE;

    // fstat ...
    //dir->ent.size = dir->data->;
    return &dir->ent;
}


//============================================================
//  osd_closedir
//============================================================

void osd_closedir(osd_directory *dir) {
    // since there are no standard C library routines for walking directories,
    // we do nothing
    if (dir->fd != NULL)
        closedir(dir->fd);
    free(dir->path);
    free(dir);
}


//============================================================
//  osd_is_absolute_path
//============================================================

int osd_is_absolute_path(const char *path) {
    // assume no for everything
    return FALSE;
}
