/*-
 ***********************************************************************
 *
 * $Id: ktypes.h,v 1.3 2003/08/13 16:19:05 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2003-2003 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */

#if defined(UNIX)
  #if defined(K_CPU_ALPHA) || defined(K_CPU_IA64)
    typedef char               K_INT08;
    typedef unsigned char      K_UINT08;
    typedef short              K_INT16;
    typedef unsigned short     K_UINT16;
    typedef int                K_INT32;
    typedef unsigned int       K_UINT32;
    typedef long               K_INT64;
    typedef unsigned long      K_UINT64;
  #else
    typedef char               K_INT08;
    typedef unsigned char      K_UINT08;
    typedef short              K_INT16;
    typedef unsigned short     K_UINT16;
    typedef long               K_INT32;
    typedef unsigned long      K_UINT32;
    typedef long long          K_INT64;
    typedef unsigned long long K_UINT64;
  #endif
#elif defined(WIN32)
    typedef char               K_INT08;
    typedef unsigned char      K_UINT08;
    typedef short              K_INT16;
    typedef unsigned short     K_UINT16;
    typedef long               K_INT32;
    typedef unsigned long      K_UINT32;
    typedef __int64            K_INT64;
    typedef unsigned __int64   K_UINT64;
#endif
