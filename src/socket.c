/*
 ***********************************************************************
 *
 * $Id: socket.c,v 1.1.1.1 2002/01/18 03:17:47 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * SocketCleanup
 *
 ***********************************************************************
 */
void
SocketCleanup(SOCKET_CONTEXT *psockCTX)
{
  if (psockCTX != NULL)
  {
    if (psockCTX->iSocket != -1)
    {
#ifdef WIN32
      closesocket(psockCTX->iSocket);
      WSACleanup();
#endif
#ifdef UNIX
      close(psockCTX->iSocket);
#endif
    }
#ifdef USE_SSL
    if (psockCTX->iType == SOCKET_TYPE_SSL)
    {
      SSLSessionCleanup(psockCTX->pssl);
    }
#endif
    free(psockCTX);
  }
}


/*-
 ***********************************************************************
 *
 * SocketConnect
 *
 ***********************************************************************
 */
SOCKET_CONTEXT
*SocketConnect(unsigned long ulIP, unsigned short usPort, int iType, void *psslCTX, char *pcError)
{
  const char          cRoutine[] = "SocketConnect()";
  char                cLocalError[ERRBUF_SIZE];
  struct sockaddr_in  saServer;
  SOCKET_CONTEXT     *psockCTX;

#ifdef WIN32
  DWORD               dwStatus;
  WORD                wVersion;
  WSADATA             wsaData;
#endif

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Initialize a socket context.
   *
   *********************************************************************
   */
  psockCTX = malloc(sizeof(SOCKET_CONTEXT));
  if (psockCTX == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  memset(psockCTX, 0, sizeof(SOCKET_CONTEXT));
  psockCTX->iSocket = -1;

  /*-
   ***********************************************************************
   *
   * Create a socket, and open a TCP connection to the server.
   *
   ***********************************************************************
   */
  saServer.sin_family = AF_INET;
  saServer.sin_addr.s_addr = ulIP;
  saServer.sin_port = htons(usPort);

#ifdef WIN32
  wVersion = (WORD)(1) | ((WORD)(1) << 8); /* MAKEWORD(1, 1) */
  if ((dwStatus = WSAStartup(wVersion, &wsaData)) != 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: WSAStartup(): %u", cRoutine, dwStatus);
    SocketCleanup(psockCTX);
    return NULL;
  }

  psockCTX->iSocket = socket(PF_INET, SOCK_STREAM, 0);
  if (psockCTX->iSocket == INVALID_SOCKET)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: socket(): %u", cRoutine, WSAGetLastError());
    SocketCleanup(psockCTX);
    return NULL;
  }

  if (connect(psockCTX->iSocket, (struct sockaddr *) & saServer, sizeof(saServer)) == SOCKET_ERROR)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: connect(): %u", cRoutine, WSAGetLastError());
    SocketCleanup(psockCTX);
    return NULL;
  }
#endif

#ifdef UNIX
  psockCTX->iSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (psockCTX->iSocket == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: socket(): %s", cRoutine, strerror(errno));
    SocketCleanup(psockCTX);
    return NULL;
  }

  if (connect(psockCTX->iSocket, (struct sockaddr *) & saServer, sizeof(saServer)) == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: connect(): %s", cRoutine, strerror(errno));
    SocketCleanup(psockCTX);
    return NULL;
  }
#endif

#ifdef USE_SSL
  /*-
   ***********************************************************************
   *
   * We have a TCP conncetion... begin SSL negotiations.
   *
   ***********************************************************************
   */
  if (iType == SOCKET_TYPE_SSL)
  {
    if (psslCTX != NULL)
    {
      psockCTX->psslCTX = (SSL_CTX *) psslCTX;
      psockCTX->pssl = SSLConnect(psockCTX->iSocket, psockCTX->psslCTX, cLocalError);
      if (psockCTX->pssl == NULL)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        SocketCleanup(psockCTX);
        return NULL;
      }
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSL_CTX.", cRoutine);
      SocketCleanup(psockCTX);
      return NULL;
    }
  }
#endif

  psockCTX->iType = iType;

  return psockCTX;
}


/*-
 ***********************************************************************
 *
 * SocketRead
 *
 ***********************************************************************
 */
int
SocketRead(SOCKET_CONTEXT *psockCTX, char *pcData, int iToRead, char *pcError)
{
  const char          cRoutine[] = "SocketRead()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iNRead;

  cLocalError[0] = 0;

  switch (psockCTX->iType)
  {
#ifdef USE_SSL
  case SOCKET_TYPE_SSL:
    iNRead = SSLRead(psockCTX->pssl, pcData, iToRead, cLocalError);
    if (iNRead == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    }
    break;
#endif
  default:
    iNRead = recv(psockCTX->iSocket, pcData, iToRead, 0);
    if (iNRead == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    }
    break;
  }

  return iNRead;
}


/*-
 ***********************************************************************
 *
 * SocketWrite
 *
 ***********************************************************************
 */
int
SocketWrite(SOCKET_CONTEXT *psockCTX, char *pcData, int iToSend, char *pcError)
{
  const char          cRoutine[] = "SocketWrite()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iNSent;

  cLocalError[0] = 0;

  if (iToSend == 0)
  {
    return 0;
  }

  switch (psockCTX->iType)
  {
#ifdef USE_SSL
  case SOCKET_TYPE_SSL:
    iNSent = SSLWrite(psockCTX->pssl, pcData, iToSend, cLocalError);
    if (iNSent == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    }
    break;
#endif
  default:
    iNSent = send(psockCTX->iSocket, pcData, iToSend, 0);
    if (iNSent == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    }
    break;
  }

  return iNSent;
}
