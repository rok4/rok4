/*
 * Copyright (c) 2005, Herve Drolon, FreeImage Team
 * Copyright (c) 2008;2011-2012, Centre National d'Etudes Spatiales (CNES), France 
 * Copyright (c) 2012, CS Systemes d'Information, France
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef OPJ_INCLUDES_H
#define OPJ_INCLUDES_H

/*
 * This must be included before any system headers,
 * since they can react to macro defined there
 */
//#include "opj_config.h"

/*
 ==========================================================
   Standard includes used by the library
 ==========================================================
*/
#include <memory.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>

/*
  Use fseeko() and ftello() if they are available since they use
  'off_t' rather than 'long'.  It is wrong to use fseeko() and
  ftello() only on systems with special LFS support since some systems
  (e.g. FreeBSD) support a 64-bit off_t by default.
*/
#  define OPJ_FSEEK(stream,offset,whence) fseek(stream,offset,whence)
#  define OPJ_FSTAT(fildes,stat_buff) fstat(fildes,stat_buff)
#  define OPJ_FTELL(stream) ftell(stream)
#  define OPJ_STAT_STRUCT_T struct stat
#  define OPJ_STAT(path,stat_buff) stat(path,stat_buff)



/*
 ==========================================================
   OpenJPEG interface
 ==========================================================
 */

#define restrict __restrict__


#include "openjpeg.h"
#include "opj_inttypes.h"
#include "opj_clock.h"
#include "opj_malloc.h"
#include "function_list.h"
#include "event.h"
#include "bio.h"
#include "cio.h"

#include "image.h"
#include "invert.h"
#include "j2k.h"
#include "jp2.h"

#include "mqc.h"
#include "raw.h"
#include "bio.h"

#include "pi.h"
#include "tgt.h"
#include "tcd.h"
#include "t1.h"
#include "dwt.h"
#include "t2.h"
#include "mct.h"
#include "opj_intmath.h"

#include "cidx_manager.h"
#include "indexbox_manager.h"


#endif /* OPJ_INCLUDES_H */
