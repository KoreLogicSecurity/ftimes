/*
 ***********************************************************************
 *
 * $Id: acconfig.h,v 1.1.1.1 2002/01/18 03:16:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

/* Enable xmagic */
#undef USE_XMAGIC

/* Enable SSL */
#undef USE_SSL

/* SSL directory. */
#undef ssldir

/* Define if headers are in the openssl directory. */
#undef HAVE_OPENSSL

/* OS specific defines */
#undef FTimes_UNIX
#undef FTimes_AIX
#undef FTimes_BSD
#undef FTimes_LINUX
#undef FTimes_SOLARIS

/* ap_snprintf work around */
#undef USE_AP_SNPRINTF
