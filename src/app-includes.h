/*-
 ***********************************************************************
 *
 * $Id: app-includes.h,v 1.17 2006/06/20 05:20:01 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2006 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "ktypes.h"

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
#include "sha1.h"

#include "compare.h"
#include "decode.h"
#include "dig.h"
#include "fsinfo.h"
#include "socket.h"
#include "http.h"

#include "ftimes.h"

#ifdef WIN32
#define execlp _execlp
#define fstat _fstat
#define getcwd _getcwd
#define snprintf _snprintf
#define stat _stat
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#define unlink _unlink
#endif
