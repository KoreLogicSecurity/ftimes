/*-
 ***********************************************************************
 *
 * $Id: app-includes.h,v 1.39 2014/07/30 07:27:59 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _APP_INCLUDES_H_INCLUDED
#define _APP_INCLUDES_H_INCLUDED

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

#ifdef USE_EMBEDDED_PERL
#include <EXTERN.h>
#include <perl.h>
#ifdef USE_EMBEDDED_PERL_XSUB
#include <XSUB.h>
#endif
#endif

#ifdef USE_EMBEDDED_PYTHON
#ifdef _GNU_SOURCE
#undef _GNU_SOURCE /* NOTE: This macro is undefined to suppress compiler warnings. It's defined again as a result of including Python.h. This may also be a Gentoo-specific problem. */
#endif
#include <Python.h>
#endif

#ifdef USE_KLEL
#include <klel.h>
#endif

#ifdef USE_FILE_HOOKS
#include "hook.h"
#endif

#ifdef USE_PCRE
#include <pcre.h>
#endif

#ifdef USE_SSL
#include "ssl.h"
#include "ssl-pool.h"
#endif

#ifdef USE_XMAGIC
#include "xmagic.h"
#endif

#ifdef USE_AP_SNPRINTF
#include "ap_snprintf.h"
#define snprintf ap_snprintf
#endif

#include "error.h"
#include "mask.h"
#include "md5.h"
#include "message.h"
#include "options.h"
#include "sha1.h"
#include "sha256.h"

#include "compare.h"
#include "decode.h"
#include "dig.h"
#include "fsinfo.h"
#include "socket.h"
#include "http.h"
#include "version.h"

#include "ftimes.h"

#ifdef WIN32
#define chdir _chdir
#define execlp _execlp
#define fdopen _fdopen
#ifndef fstat
#define fstat _fstat
#endif
#define getcwd _getcwd
#define getpid _getpid
#define snprintf _snprintf
#ifndef stat
#define stat _stat
#endif
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define unlink _unlink
#endif

#endif /* !_APP_INCLUDES_H_INCLUDED */
