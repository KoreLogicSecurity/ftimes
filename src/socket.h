/*-
 ***********************************************************************
 *
 * $Id: socket.h,v 1.7 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2007 Klayton Monroe, All Rights Reserved.
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
