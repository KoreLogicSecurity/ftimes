/*-
 ***********************************************************************
 *
 * $Id: all-includes.h,v 1.5 2012/01/04 03:12:39 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#if defined(UNIX) || defined(MINGW32)
#include "config.h"
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef UNIX
#include <netinet/in.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

typedef char APP_SI8;
typedef unsigned char APP_UI8;
typedef short APP_SI16;
typedef unsigned short APP_UI16;
typedef int APP_SI32;
typedef unsigned int APP_UI32;
#if defined(UNIX)
  #if defined(APP_CPU_ALPHA) || defined(APP_CPU_IA64) || defined(APP_CPU_X86_64) || defined(APP_CPU_AMD64)
    typedef long APP_SI64;
    typedef unsigned long APP_UI64;
  #else
    typedef long long APP_SI64;
    typedef unsigned long long APP_UI64;
  #endif
#elif defined(WIN32)
    typedef __int64 APP_SI64;
    typedef unsigned __int64 APP_UI64;
#endif

#include "md5.h"
#include "sha1.h"

#include "hashcp.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

