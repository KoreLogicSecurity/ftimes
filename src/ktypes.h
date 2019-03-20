/*-
 ***********************************************************************
 *
 * $Id: ktypes.h,v 1.8 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2003-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#if defined(UNIX)
  #if defined(K_CPU_ALPHA) || defined(K_CPU_IA64) || defined(K_CPU_X86_64) || defined(K_CPU_AMD64)
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
    typedef int                K_INT32;
    typedef unsigned int       K_UINT32;
    typedef long long          K_INT64;
    typedef unsigned long long K_UINT64;
  #endif
#elif defined(WIN32)
    typedef char               K_INT08;
    typedef unsigned char      K_UINT08;
    typedef short              K_INT16;
    typedef unsigned short     K_UINT16;
    typedef int                K_INT32;
    typedef unsigned int       K_UINT32;
    typedef __int64            K_INT64;
    typedef unsigned __int64   K_UINT64;
#endif
