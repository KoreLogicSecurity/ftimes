/*-
 ***********************************************************************
 *
 * $Id: sys-includes.h,v 1.12 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#include <stdlib.h>
#include <ctype.h>
#include <direct.h>
#include <time.h>
#include <process.h>
#include <fcntl.h>
#include <io.h>
#ifdef WINNT
#include "native.h"
#endif
#endif

#ifdef UNIX
#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/utsname.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifdef FTimes_AIX
#include <strings.h>
#include <sys/statfs.h>
#include <sys/statvfs.h>
#include <sys/sysmacros.h>
#include <sys/vmount.h>
#endif
#ifdef FTimes_HPUX
#include <sys/statvfs.h>
#endif
#ifdef FTimes_LINUX
#include <sys/sysmacros.h>
#include <sys/vfs.h>
#endif
#ifdef FTimes_SOLARIS
#include <sys/sysmacros.h>
#include <sys/statvfs.h>
#endif
#if defined(FTimes_BSD) || defined(FTimes_MACOS)
#include <sys/param.h>
#include <sys/mount.h>
#endif
#endif
