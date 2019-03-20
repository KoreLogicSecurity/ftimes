/*-
 ***********************************************************************
 *
 * $Id: app-includes.h,v 1.19 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
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
#include "sha256.h"

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
