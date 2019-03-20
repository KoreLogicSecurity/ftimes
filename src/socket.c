/*-
 ***********************************************************************
 *
 * $Id: socket.c,v 1.9 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2007 Klayton Monroe, All Rights Reserved.
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
SocketCleanup(SOCKET_CONTEXT *psSocketCTX)
{
  if (psSocketCTX != NULL)
  {
    if (psSocketCTX->iSocket != -1)
    {
#ifdef WIN32
      closesocket(psSocketCTX->iSocket);
      WSACleanup();
#endif
#ifdef UNIX
      close(psSocketCTX->iSocket);
#endif
    }
#ifdef USE_SSL
    if (psSocketCTX->iType == SOCKET_TYPE_SSL)
    {
      SSLSessionCleanup(psSocketCTX->pssl);
    }
#endif
    free(psSocketCTX);
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
  const char          acRoutine[] = "SocketConnect()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif
  struct sockaddr_in  sServerAddr;
  SOCKET_CONTEXT     *psSocketCTX;

#ifdef WIN32
  DWORD               dwStatus;
  WORD                wVersion;
  WSADATA             wsaData;
#endif

  /*-
   *********************************************************************
   *
   * Initialize a socket context.
   *
   *********************************************************************
   */
  psSocketCTX = malloc(sizeof(SOCKET_CONTEXT));
  if (psSocketCTX == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(psSocketCTX, 0, sizeof(SOCKET_CONTEXT));
  psSocketCTX->iSocket = -1;

  /*-
   ***********************************************************************
   *
   * Create a socket, and open a TCP connection to the server.
   *
   ***********************************************************************
   */
  sServerAddr.sin_family = AF_INET;
  sServerAddr.sin_addr.s_addr = ulIP;
  sServerAddr.sin_port = htons(usPort);

#ifdef WIN32
  wVersion = (WORD)(1) | ((WORD)(1) << 8); /* MAKEWORD(1, 1) */
  if ((dwStatus = WSAStartup(wVersion, &wsaData)) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: WSAStartup(): %u", acRoutine, dwStatus);
    SocketCleanup(psSocketCTX);
    return NULL;
  }

  psSocketCTX->iSocket = socket(PF_INET, SOCK_STREAM, 0);
  if (psSocketCTX->iSocket == INVALID_SOCKET)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: socket(): %u", acRoutine, WSAGetLastError());
    SocketCleanup(psSocketCTX);
    return NULL;
  }

  if (connect(psSocketCTX->iSocket, (struct sockaddr *) & sServerAddr, sizeof(sServerAddr)) == SOCKET_ERROR)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: connect(): %u", acRoutine, WSAGetLastError());
    SocketCleanup(psSocketCTX);
    return NULL;
  }
#endif

#ifdef UNIX
  psSocketCTX->iSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (psSocketCTX->iSocket == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: socket(): %s", acRoutine, strerror(errno));
    SocketCleanup(psSocketCTX);
    return NULL;
  }

  if (connect(psSocketCTX->iSocket, (struct sockaddr *) & sServerAddr, sizeof(sServerAddr)) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: connect(): %s", acRoutine, strerror(errno));
    SocketCleanup(psSocketCTX);
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
      psSocketCTX->psslCTX = (SSL_CTX *) psslCTX;
      psSocketCTX->pssl = SSLConnect(psSocketCTX->iSocket, psSocketCTX->psslCTX, acLocalError);
      if (psSocketCTX->pssl == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        SocketCleanup(psSocketCTX);
        return NULL;
      }
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSL_CTX.", acRoutine);
      SocketCleanup(psSocketCTX);
      return NULL;
    }
  }
#endif

  psSocketCTX->iType = iType;

  return psSocketCTX;
}


/*-
 ***********************************************************************
 *
 * SocketRead
 *
 ***********************************************************************
 */
int
SocketRead(SOCKET_CONTEXT *psSocketCTX, char *pcData, int iToRead, char *pcError)
{
  const char          acRoutine[] = "SocketRead()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif
  int                 iNRead;

  switch (psSocketCTX->iType)
  {
#ifdef USE_SSL
  case SOCKET_TYPE_SSL:
    iNRead = SSLRead(psSocketCTX->pssl, pcData, iToRead, acLocalError);
    if (iNRead == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    }
    break;
#endif
  default:
    iNRead = recv(psSocketCTX->iSocket, pcData, iToRead, 0);
    if (iNRead == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: recv(): %s", acRoutine, strerror(errno));
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
SocketWrite(SOCKET_CONTEXT *psSocketCTX, char *pcData, int iToSend, char *pcError)
{
  const char          acRoutine[] = "SocketWrite()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif
  int                 iNSent;

  if (iToSend == 0)
  {
    return 0;
  }

  switch (psSocketCTX->iType)
  {
#ifdef USE_SSL
  case SOCKET_TYPE_SSL:
    iNSent = SSLWrite(psSocketCTX->pssl, pcData, iToSend, acLocalError);
    if (iNSent == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    }
    break;
#endif
  default:
    iNSent = send(psSocketCTX->iSocket, pcData, iToSend, 0);
    if (iNSent == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: send(): %s", acRoutine, strerror(errno));
    }
    break;
  }

  return iNSent;
}
