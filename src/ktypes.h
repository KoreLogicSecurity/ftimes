/*
 ***********************************************************************
 *
 * $Id: ktypes.h,v 1.1.1.1 2002/01/18 03:17:43 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#ifndef ALREADY_INCLUDED_K_INT08
typedef char               K_INT08;
#ifndef ALREADY_INCLUDED_K_INT16
typedef short              K_INT16;
#endif
#ifndef ALREADY_INCLUDED_K_INT32
typedef long               K_INT32;
#endif
#ifndef ALREADY_INCLUDED_K_UINT08
typedef unsigned char      K_UINT08;
#endif
#ifndef ALREADY_INCLUDED_K_UINT16
typedef unsigned short     K_UINT16;
#endif
#endif
#ifndef ALREADY_INCLUDED_K_UINT32
typedef unsigned long      K_UINT32;
#endif

#ifdef WIN32
#ifndef ALREADY_INCLUDED_K_INT64
typedef __int64            K_INT64;
#endif
#ifndef ALREADY_INCLUDED_K_UINT64
typedef unsigned __int64   K_UINT64;
#endif
#endif

#ifdef UNIX
#ifndef ALREADY_INCLUDED_K_INT64
typedef long long          K_INT64;
#endif
#ifndef ALREADY_INCLUDED_K_UINT64
typedef unsigned long long K_UINT64;
#endif
#endif

#ifndef ALREADY_INCLUDED_K_INT08
#define ALREADY_INCLUDED_K_INT08 1
#endif
#ifndef ALREADY_INCLUDED_K_INT16
#define ALREADY_INCLUDED_K_INT16 1
#endif
#ifndef ALREADY_INCLUDED_K_INT32
#define ALREADY_INCLUDED_K_INT32 1
#endif
#ifndef ALREADY_INCLUDED_K_INT64
#define ALREADY_INCLUDED_K_INT64 1
#endif
#ifndef ALREADY_INCLUDED_K_UINT08
#define ALREADY_INCLUDED_K_UINT08 1
#endif
#ifndef ALREADY_INCLUDED_K_UINT16
#define ALREADY_INCLUDED_K_UINT16 1
#endif
#ifndef ALREADY_INCLUDED_K_UINT32
#define ALREADY_INCLUDED_K_UINT32 1
#endif
#ifndef ALREADY_INCLUDED_K_UINT64
#define ALREADY_INCLUDED_K_UINT64 1
#endif
