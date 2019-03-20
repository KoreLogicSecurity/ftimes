/*-
 ***********************************************************************
 *
 * $Id: all-includes.h,v 1.12 2012/01/04 03:12:40 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#if defined(UNIX) || defined(MINGW32)
#include "config.h"
#endif

#ifdef UNIX
#include <dirent.h>
#endif
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#ifdef UNIX
#include <netinet/in.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

#ifdef USE_AP_SNPRINTF
#ifdef FTimes_SOLARIS
#include <ctype.h>
#include <stdarg.h>
#endif
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include "ap_snprintf.h"
#define snprintf ap_snprintf
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
#include "tarmap.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 1024
#endif

