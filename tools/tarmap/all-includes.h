/*-
 ***********************************************************************
 *
 * $Id: all-includes.h,v 1.5 2007/02/23 00:22:42 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2005-2007 The FTimes Project, All Rights Reserved.
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

#include "ktypes.h"
#include "md5.h"
#include "sha1.h"
#include "tarmap.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 1024
#endif

