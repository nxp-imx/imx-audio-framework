/*
***********************************************************************
* Copyright 2009-2010 by Freescale Semiconductor, Inc.
* All modifications are confidential and proprietary information
* of Freescale Semiconductor, Inc. ALL RIGHTS RESERVED.
***********************************************************************
*
*  History :
*  Date             Author              Version    Description
*
*  Aug,2009         B06543              0.1        Initial Version
*
*/

#ifndef _FSL_MMLAYER_TYPES_H
#define _FSL_MMLAYER_TYPES_H

#include <stdio.h>

#ifndef uint64
#ifdef WIN32
	typedef  unsigned __int64 uint64;	
#else
	typedef  unsigned long long uint64;	
#endif
#endif /*uint64*/


#ifndef int64
#ifdef WIN32	
	typedef  __int64 int64;
#else	
	typedef  long long int64;
#endif
#endif /*int64*/

typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef int int32;
typedef short int16;
typedef char int8;

#ifndef bool
    #define bool int
#endif

#ifndef TRUE
    #define TRUE 1    
#endif

#ifndef FALSE
    #define FALSE 0    
#endif

#ifndef NULL
    #define NULL (void *)0
#endif
#endif /* _FSL_MMLAYER_TYPES_H */

