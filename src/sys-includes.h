/*-
 ***********************************************************************
 *
 * $Id: sys-includes.h,v 1.31 2019/03/14 16:07:43 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _SYS_INCLUDES_H_INCLUDED
#define _SYS_INCLUDES_H_INCLUDED

#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

#ifdef WIN32
  #if defined(MINGW32) && !defined(MINGW64)
    #ifndef WINVER
    #define WINVER 0x0500
    #endif
    #include <windows.h>
    #define SDDL_REVISION_1 1
    WINADVAPI BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorA(PSECURITY_DESCRIPTOR, DWORD, SECURITY_INFORMATION, LPSTR *, PULONG);
    WINADVAPI BOOL WINAPI ConvertSecurityDescriptorToStringSecurityDescriptorW(PSECURITY_DESCRIPTOR, DWORD, SECURITY_INFORMATION, LPWSTR *, PULONG);
    #ifdef UNICODE
      #define ConvertSecurityDescriptorToStringSecurityDescriptor ConvertSecurityDescriptorToStringSecurityDescriptorW
    #else
      #define ConvertSecurityDescriptorToStringSecurityDescriptor ConvertSecurityDescriptorToStringSecurityDescriptorA
    #endif
  #else
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
    #endif
    #include <windows.h>
  #endif
  #include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#include <tchar.h>
#ifdef MINGW32
#include <math.h>
#endif
#include <winsock.h>
#ifdef WINNT
#include "native.h"
#endif
#endif

#ifdef UNIX
#include <dirent.h>
#include <limits.h>
#include <math.h>
#include <netdb.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
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
#ifdef USE_FILE_HOOKS
#include <sys/wait.h>
#endif
#endif

#endif /* !_SYS_INCLUDES_H_INCLUDED */
