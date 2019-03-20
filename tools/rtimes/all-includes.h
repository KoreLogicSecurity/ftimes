/*-
 ***********************************************************************
 *
 * $Id: all-includes.h,v 1.13 2019/03/14 16:07:44 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2008-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#if defined(MINGW32)
#include "config.h"
#endif

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
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <tchar.h>

typedef char APP_SI8;
typedef unsigned char APP_UI8;
typedef short APP_SI16;
typedef unsigned short APP_UI16;
typedef int APP_SI32;
typedef unsigned int APP_UI32;
typedef __int64 APP_SI64;
typedef unsigned __int64 APP_UI64;

#include "md5.h"
#include "options.h"

#include "rtimes.h"

#ifdef WIN32
#define snprintf _snprintf
#endif

#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 1024
#endif

