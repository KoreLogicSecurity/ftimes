/*-
 ***********************************************************************
 *
 * $Id: all-includes.h,v 1.3 2013/02/14 16:55:22 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _ALL_INCLUDES_H_INCLUDED
#define _ALL_INCLUDES_H_INCLUDED

#if defined(UNIX) || defined(MINGW32)
#include "config.h"
#endif

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#ifdef WIN32
#include <io.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef UNIX
#include <netinet/in.h>
#endif
#ifdef WIN32
#include <tchar.h>
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
#include "options.h"
#include "sha1.h"
#include "ftimes-xpa.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE (1024 * sizeof(char))
#endif

#define PROPERTIES_COMMENT_S "#"
#define PROPERTIES_DELIMITER_S "|"
#define PROPERTIES_MAX_LINE 65536

#endif /* !_ALL_INCLUDES_H_INCLUDED */
