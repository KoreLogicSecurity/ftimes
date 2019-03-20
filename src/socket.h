/*
 ***********************************************************************
 *
 * $Id: socket.h,v 1.1.1.1 2002/01/18 03:17:47 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

/* #include "ssl.h" */

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#ifndef ERRBUF_SIZE
#define ERRBUF_SIZE 1024
#endif

#define SOCKET_TYPE_REGULAR 0
#define SOCKET_TYPE_SSL 1

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _SOCKET_CONTEXT
{
  int                 iSocket,
                      iType;
#ifdef USE_SSL
  SSL_CTX            *psslCTX;
  SSL                *pssl;
#endif
} SOCKET_CONTEXT;

/*-
 ***********************************************************************
 *
 * Public Prototypes
 *
 ***********************************************************************
 */
void                  SocketCleanup(SOCKET_CONTEXT *psockCTX);
SOCKET_CONTEXT       *SocketConnect(unsigned long ulIP, unsigned short usPort, int iType, void *psslCTX, char *pcError);
int                   SocketRead(SOCKET_CONTEXT *psockCTX, char *pcData, int iToRead, char *pcError);
int                   SocketWrite(SOCKET_CONTEXT *psockCTX, char *pcData, int iToSend, char *pcError);
