/*-
 ***********************************************************************
 *
 * $Id: app-includes.h,v 1.10 2003/02/24 19:35:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "ktypes.h"

#ifdef USE_SSL
#include "ssl.h"
#include "ssl-pool.h"
#endif

#ifdef USE_AP_SNPRINTF
#include "ap_snprintf.h"
#define snprintf ap_snprintf
#endif

#include "error.h"
#include "md5.h"
#include "message.h"

#include "compare.h"
#include "decode.h"
#include "dig.h"
#include "fsinfo.h"
#ifdef USE_XMAGIC
#include "xmagic.h"
#endif
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
