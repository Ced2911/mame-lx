//============================================================
//
//  minisync.c - Minimal core synchronization functions
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
#include <ppc/atomic.h>

#include <debug.h>

struct _osd_lock{
    unsigned int __attribute__ ((aligned (128))) _lock;
};

//============================================================
//  osd_lock_alloc
//============================================================

osd_lock *osd_lock_alloc(void)
{
	// the minimal implementation does not support threading
	// just return a dummy value here
	//return (osd_lock *)1;
    _osd_lock * _lock = (_osd_lock *)osd_malloc(sizeof(_osd_lock));
    _lock->_lock = 0;
    return _lock;
        //TR;
}


//============================================================
//  osd_lock_acquire
//============================================================

void osd_lock_acquire(osd_lock *_lock)
{
	// the minimal implementation does not support threading
	// the acquire always "succeeds"
    //lock(&_lock->_lock);
    //TR;
}


//============================================================
//  osd_lock_try
//============================================================

int osd_lock_try(osd_lock *_lock)
{
    //TR;
	// the minimal implementation does not support threading
	// the acquire always "succeeds"
	return TRUE;
}


//============================================================
//  osd_lock_release
//============================================================

void osd_lock_release(osd_lock *_lock)
{
    //TR;
	// the minimal implementation does not support threading
	// do nothing here
    //unlock(&_lock->_lock);
}


//============================================================
//  osd_lock_free
//============================================================

void osd_lock_free(osd_lock *_lock)
{
    //TR;
	// the minimal implementation does not support threading
	// do nothing here

    _lock->_lock=0;
    osd_free(_lock);
    _lock = NULL;
}
