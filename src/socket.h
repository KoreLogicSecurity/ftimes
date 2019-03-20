/*-
 ***********************************************************************
 *
 * $Id: socket.h,v 1.19 2019/03/14 16:07:43 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _SOCKET_H_INCLUDED
#define _SOCKET_H_INCLUDED

/* #include "ssl.h" */

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 1024
#endif

#define SOCKET_TYPE_REGULAR 0
#define SOCKET_TYPE_SSL     1

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _SOCKET_CONTEXT
{
  int                 iSocket;
  int                 iType;
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
void                  SocketCleanup(SOCKET_CONTEXT *psSocketCTX);
SOCKET_CONTEXT       *SocketConnect(unsigned long ulIP, unsigned short usPort, int iType, void *psslCTX, char *pcError);
int                   SocketRead(SOCKET_CONTEXT *psSocketCTX, char *pcData, int iToRead, char *pcError);
int                   SocketWrite(SOCKET_CONTEXT *psSocketCTX, char *pcData, int iToSend, char *pcError);

#endif /* !_SOCKET_H_INCLUDED */
