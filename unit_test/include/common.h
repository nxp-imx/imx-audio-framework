//*****************************************************************
// Copyright 2018 NXP
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
//*****************************************************************

#ifndef _COMMON_H_
#define _COMMON_H_

#include "xf-debug.h"

typedef signed char             WORD8   ;
typedef signed char         *   pWORD8  ;
typedef unsigned char           UWORD8  ;
typedef unsigned char       *   pUWORD8 ;

typedef signed short            WORD16  ;
typedef signed short        *   pWORD16 ;
typedef unsigned short          UWORD16 ;
typedef unsigned short      *   pUWORD16;

typedef signed int              WORD24  ;
typedef signed int          *   pWORD24 ;
typedef unsigned int            UWORD24 ;
typedef unsigned int        *   pUWORD24;

typedef signed int              WORD32  ;
typedef signed int          *   pWORD32 ;
typedef unsigned int            UWORD32 ;
typedef unsigned int        *   pUWORD32;

typedef void                    VOID    ;
typedef void                *   pVOID   ;

typedef signed int              BOOL    ;
typedef unsigned int            UBOOL   ;
typedef signed int              FLAG    ;
typedef unsigned int            UFLAG   ;
typedef signed int              LOOPIDX ;
typedef unsigned int            ULOOPIDX;
typedef signed int              WORD    ;
typedef unsigned int            UWORD   ;

typedef LOOPIDX                 LOOPINDEX;
typedef ULOOPIDX                ULOOPINDEX;

typedef struct
{
	int scalOutObjectType;
	int scalOutNumChannels;
	int sampleRate;
	int SamplingFreqIndex;
	int framelengthflag;
}BSAC_Block_Params;


int App_get_mp4forbsac_header(BSAC_Block_Params * params, char *buf, int buf_size);
void xa_shift_input_buffer (char *buf, int buf_size, int bytes_consumed);


#endif
