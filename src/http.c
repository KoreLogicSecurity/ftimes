/*-
 ***********************************************************************
 *
 * $Id: http.c,v 1.25 2012/05/04 20:19:47 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static unsigned char gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char gaucBase16[] = "0123456789abcdef";

/*-
 ***********************************************************************
 *
 * HttpBuildProxyConnectRequest
 *
 ***********************************************************************
 */
char *
HttpBuildProxyConnectRequest(HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpBuildProxyConnectRequest()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcBasicEncoding = NULL;
  char               *pcRequest = NULL;
  int                 iIndex = 0;
  int                 iRequestLength = 0;

  /*-
   *********************************************************************
   *
   * Make sure the URL and its members aren't NULL.
   *
   *********************************************************************
   */
  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return NULL;
  }

  if (psUrl->pcUser == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Username.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPass == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Password.", acRoutine);
    return NULL;
  }

  if (psUrl->pcHost == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Host.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Port.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Path.", acRoutine);
    return NULL;
  }

  if (psUrl->pcQuery == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Query.", acRoutine);
    return NULL;
  }

  if (psUrl->pcMeth == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Method.", acRoutine);
    return NULL;
  }

  if (psUrl->pcJobId == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Job-Id.", acRoutine);
    return NULL;
  }

  if (psUrl->iUseProxy)
  {
    if (psUrl->pcProxyUser == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Username.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyPass == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Password.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyHost == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Host.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyPort == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Port.", acRoutine);
      return NULL;
    }
  }

  /*-
   *********************************************************************
   *
   * Calculate the amount of memory that would required with everything
   * enabled.
   *
   *********************************************************************
   */
  iRequestLength = 0;
  iRequestLength += strlen("CONNECT") + strlen(" ") + strlen(psUrl->pcHost) + strlen(":") + strlen(psUrl->pcPort) + strlen(" HTTP/1.1\r\n");
  iRequestLength += strlen("Host: ") + strlen(psUrl->pcHost) + strlen(":") + strlen(psUrl->pcPort) + strlen("\r\n");
  iRequestLength += strlen("Proxy-Authorization: Basic ") + (4 * (strlen(psUrl->pcProxyUser) + strlen(psUrl->pcProxyPass))) + strlen("\r\n");
  iRequestLength += strlen("Connection: close\r\n");
  iRequestLength += strlen("\r\n");
  iRequestLength += 1;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcRequest = malloc(iRequestLength);
  if (pcRequest == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcRequest[0] = 0;

  /*-
   *********************************************************************
   *
   * Prepare the Request-Header.
   *
   *********************************************************************
   */
  iIndex = 0;
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "CONNECT %s:%d HTTP/%s\r\n",
    psUrl->pcHost,
    psUrl->ui16Port,
    ((psUrl->iFlags & HTTP_FLAG_USE_HTTP_1_0) == HTTP_FLAG_USE_HTTP_1_0) ? "1.0" : "1.1"
    );
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Host: %s:%d\r\n", psUrl->pcHost, psUrl->ui16Port);
  if (psUrl->iProxyAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    pcBasicEncoding = HttpEncodeBasic(psUrl->pcProxyUser, psUrl->pcProxyPass, acLocalError);
    if (pcBasicEncoding == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeData(pcRequest);
      return NULL;
    }
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Proxy-Authorization: Basic %s\r\n", pcBasicEncoding);
    HttpFreeData(pcBasicEncoding);
  }
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Connection: close\r\n");
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "\r\n");

  return pcRequest;
}


/*-
 ***********************************************************************
 *
 * HttpBuildRequest
 *
 ***********************************************************************
 */
char *
HttpBuildRequest(HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpBuildRequest()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcBasicEncoding = NULL;
  char               *pcRequest = NULL;
  int                 iIndex = 0;
  int                 iRequestLength = 0;

  /*-
   *********************************************************************
   *
   * Make sure the URL and its members aren't NULL.
   *
   *********************************************************************
   */
  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return NULL;
  }

  if (psUrl->pcUser == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Username.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPass == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Password.", acRoutine);
    return NULL;
  }

  if (psUrl->pcHost == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Host.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Port.", acRoutine);
    return NULL;
  }

  if (psUrl->pcPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Path.", acRoutine);
    return NULL;
  }

  if (psUrl->pcQuery == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Query.", acRoutine);
    return NULL;
  }

  if (psUrl->pcMeth == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Method.", acRoutine);
    return NULL;
  }

  if (psUrl->pcJobId == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Job-Id.", acRoutine);
    return NULL;
  }

  if (psUrl->iUseProxy)
  {
    if (psUrl->pcProxyUser == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Username.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyPass == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Password.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyHost == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Host.", acRoutine);
      return NULL;
    }

    if (psUrl->pcProxyPort == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Proxy Port.", acRoutine);
      return NULL;
    }
  }

  /*-
   *********************************************************************
   *
   * Calculate the amount of memory that would required with everything
   * enabled. Then, tack on an extra 1024 bytes.
   *
   *********************************************************************
   */
  iRequestLength = 0;
  iRequestLength += strlen(psUrl->pcMeth) + strlen(" ") + strlen(psUrl->pcPath) + strlen("?") + strlen(psUrl->pcQuery) + strlen(" HTTP/1.1\r\n");
  iRequestLength += strlen("Host: ") + strlen(psUrl->pcHost) + strlen(":65535\r\n");
  iRequestLength += strlen("Content-Type: application/octet-stream\r\n");
  iRequestLength += strlen("Content-Length: 4294967295\r\n");
  iRequestLength += strlen("Job-Id: ") + strlen(psUrl->pcJobId) + strlen("\r\n");
  iRequestLength += strlen("Authorization: Basic ") + (4 * (strlen(psUrl->pcUser) + strlen(psUrl->pcPass))) + strlen("\r\n");
  iRequestLength += strlen("Connection: close\r\n");
  iRequestLength += strlen("\r\n");
  if (psUrl->iUseProxy)
  {
    iRequestLength += strlen("http://") + strlen(psUrl->pcHost) + strlen(":") + strlen(psUrl->pcPort);
    iRequestLength += strlen("Proxy-Authorization: Basic ") + (4 * (strlen(psUrl->pcProxyUser) + strlen(psUrl->pcProxyPass))) + strlen("\r\n");
  }
  iRequestLength += 1024;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcRequest = malloc(iRequestLength);
  if (pcRequest == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcRequest[0] = 0;

  /*-
   *********************************************************************
   *
   * Prepare the Request-Header. If a proxy is enabled and the scheme
   * is HTTP, the request must contain the full URL. If the scheme is
   * not HTTP (implying that it's HTTPS), no changes are made here.
   * Instead, a separate CONNECT request is constructed and used during
   * connection setup.
   *
   *********************************************************************
   */
  iIndex = 0;
  if (psUrl->iUseProxy && psUrl->iScheme == HTTP_SCHEME_HTTP)
  {
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "%s http://%s:%d%s%s%s HTTP/%s\r\n",
      psUrl->pcMeth,
      psUrl->pcHost,
      psUrl->ui16Port,
      psUrl->pcPath,
      (psUrl->pcQuery[0]) ? "?" : "",
      (psUrl->pcQuery[0]) ? psUrl->pcQuery : "",
      ((psUrl->iFlags & HTTP_FLAG_USE_HTTP_1_0) == HTTP_FLAG_USE_HTTP_1_0) ? "1.0" : "1.1"
      );
  }
  else
  {
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "%s %s%s%s HTTP/%s\r\n",
      psUrl->pcMeth,
      psUrl->pcPath,
      (psUrl->pcQuery[0]) ? "?" : "",
      (psUrl->pcQuery[0]) ? psUrl->pcQuery : "",
      ((psUrl->iFlags & HTTP_FLAG_USE_HTTP_1_0) == HTTP_FLAG_USE_HTTP_1_0) ? "1.0" : "1.1"
      );
  }
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Host: %s:%d\r\n", psUrl->pcHost, psUrl->ui16Port);
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Content-Type: application/octet-stream\r\n");
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Content-Length: %u\r\n", psUrl->ui32ContentLength);
  if (psUrl->iAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    pcBasicEncoding = HttpEncodeBasic(psUrl->pcUser, psUrl->pcPass, acLocalError);
    if (pcBasicEncoding == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeData(pcRequest);
      return NULL;
    }
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Authorization: Basic %s\r\n", pcBasicEncoding);
    HttpFreeData(pcBasicEncoding);
  }
  if (psUrl->iUseProxy && psUrl->iScheme == HTTP_SCHEME_HTTP && psUrl->iProxyAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    pcBasicEncoding = HttpEncodeBasic(psUrl->pcProxyUser, psUrl->pcProxyPass, acLocalError);
    if (pcBasicEncoding == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeData(pcRequest);
      return NULL;
    }
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Proxy-Authorization: Basic %s\r\n", pcBasicEncoding);
    HttpFreeData(pcBasicEncoding);
  }
  if (psUrl->pcJobId[0])
  {
    iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Job-Id: %s\r\n", psUrl->pcJobId);
  }
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "Connection: close\r\n");
  iIndex += snprintf(&pcRequest[iIndex], iRequestLength - iIndex, "\r\n");

  return pcRequest;
}


/*-
 ***********************************************************************
 *
 * HttpConnect
 *
 ***********************************************************************
 */
SOCKET_CONTEXT *
HttpConnect(HTTP_URL *psUrl, int iSocketType, char *pcError)
{
  const char          acRoutine[] = "HttpConnect()";
  char                acLocalError[MESSAGE_SIZE] = "";
#ifdef USE_SSL
  char                acSockData[HTTP_IO_BUFSIZE];
  char               *pcRequest = NULL;
  HTTP_RESPONSE_HDR   sResponseHeader;
  int                 iError = 0;
  int                 iNRead = 0;
  int                 iNSent = 0;
  int                 iRequestLength = 0;
#endif
  SOCKET_CONTEXT     *psSocketCTX = NULL;

  /*-
   *********************************************************************
   *
   * Conditionally connect to the server through a proxy.
   *
   *********************************************************************
   */
  if (psUrl->iUseProxy)
  {
    /*-
     *******************************************************************
     *
     * Establish a regular TCP connection to the proxy server.
     *
     *******************************************************************
     */
    psSocketCTX = SocketConnect(psUrl->ui32ProxyIp, psUrl->ui16ProxyPort, SOCKET_TYPE_REGULAR, NULL, acLocalError);
    if (psSocketCTX == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }

#ifdef USE_SSL
    /*-
     *******************************************************************
     *
     * Issue a CONNECT request for HTTPS connections.
     *
     *******************************************************************
     */
    if (iSocketType == SOCKET_TYPE_SSL)
    {
      /*-
       *****************************************************************
       *
       * Build the Request Header.
       *
       *****************************************************************
       */
      pcRequest = HttpBuildProxyConnectRequest(psUrl, acLocalError);
      if (pcRequest == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        SocketCleanup(psSocketCTX);
        return NULL;
      }
      iRequestLength = strlen(pcRequest);

      /*-
       *****************************************************************
       *
       * Transmit the Request Header.
       *
       *****************************************************************
       */
      iNSent = SocketWrite(psSocketCTX, pcRequest, iRequestLength, acLocalError);
      if (iNSent != iRequestLength)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", acRoutine, iNSent, iRequestLength, acLocalError);
        HttpFreeData(pcRequest);
        SocketCleanup(psSocketCTX);
        return NULL;
      }
      HttpFreeData(pcRequest);

      /*-
       *****************************************************************
       *
       * Read the Response Header.
       *
       *****************************************************************
       */
      memset(acSockData, 0, HTTP_IO_BUFSIZE);

      iNRead = HttpReadHeader(psSocketCTX, acSockData, HTTP_IO_BUFSIZE - 1, acLocalError);
      if (iNRead == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        SocketCleanup(psSocketCTX);
        return NULL;
      }
      acSockData[iNRead] = 0;

      /*-
       *****************************************************************
       *
       * Parse the Response Header. If successful, HTTP status will be set.
       *
       *****************************************************************
       */
      iError = HttpParseHeader(acSockData, iNRead, &sResponseHeader, acLocalError);
      if (iError != 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        SocketCleanup(psSocketCTX);
        return NULL;
      }

      /*-
       ***************************************************************
       *
       * Check the Status Code in the Response Header.
       *
       ***************************************************************
       */
      if (sResponseHeader.iStatusCode < 200 || sResponseHeader.iStatusCode > 299)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Proxy = [%s:%d], Status = [%d], Reason = [%s]",
          acRoutine,
          psUrl->pcProxyHost,
          psUrl->ui16ProxyPort,
          sResponseHeader.iStatusCode,
          sResponseHeader.acReasonPhrase
          );
        SocketCleanup(psSocketCTX);
        return NULL;
      }

      /*-
       *****************************************************************
       *
       * We have a proxied TCP conncetion... begin SSL negotiations.
       *
       *****************************************************************
       */
      psSocketCTX->iType = iSocketType;
      if (psUrl->psSslProperties->psslCTX != NULL)
      {
        psSocketCTX->psslCTX = psUrl->psSslProperties->psslCTX;
        psSocketCTX->pssl = SslConnect(psSocketCTX->iSocket, psSocketCTX->psslCTX, acLocalError);
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
  }
  else
  {
#ifdef USE_SSL
    psSocketCTX = SocketConnect(psUrl->ui32Ip, psUrl->ui16Port, iSocketType, (iSocketType == SOCKET_TYPE_SSL) ? psUrl->psSslProperties->psslCTX : NULL, acLocalError);
#else
    psSocketCTX = SocketConnect(psUrl->ui32Ip, psUrl->ui16Port, iSocketType, NULL, acLocalError);
#endif
    if (psSocketCTX == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }
  }

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Verify the Peer's Common Name.
   *
   *********************************************************************
   */
  if (iSocketType == SOCKET_TYPE_SSL && psUrl->psSslProperties->iVerifyPeerCert)
  {
    iError = SslVerifyCN(psSocketCTX->pssl, psUrl->psSslProperties->pcExpectedPeerCN, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      SocketCleanup(psSocketCTX);
      return NULL;
    }
  }
#endif

  return psSocketCTX;
}


/*-
 ***********************************************************************
 *
 * HttpEncodeBasic
 *
 ***********************************************************************
 */
char *
HttpEncodeBasic(char *pcUsername, char *pcPassword, char *pcError)
{
  const char          acRoutine[] = "HttpEncodeBasic()";
  char               *pcBasicEncoding = NULL;
  char               *pcCredentials = NULL;
  int                 i = 0;
  int                 iLength = 0;
  int                 iPassLength = 0;
  int                 iUserLength = 0;
  int                 n = 0;
  unsigned long       x = 0;
  unsigned long       ulLeft = 0;


  iUserLength = strlen(pcUsername);
  if (iUserLength == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Username.", acRoutine);
    return NULL;
  }

  iPassLength = strlen(pcPassword);
  if (iPassLength == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Password.", acRoutine);
    return NULL;
  }

  iLength = iUserLength + 1 + iPassLength;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcBasicEncoding = malloc((iLength * 4) + 1);
  if (pcBasicEncoding == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(pcBasicEncoding, 0, (iLength * 4) + 1);

  if (iLength <= 1)
  {
    return pcBasicEncoding;
  }

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  pcCredentials = malloc(iLength + 1);
  if (pcCredentials == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    HttpFreeData(pcBasicEncoding);
    return NULL;
  }
  snprintf(pcCredentials, iLength + 1, "%s:%s", pcUsername, pcPassword);

  for (i = 0, n = 0, x = 0, ulLeft = 0; i < iLength; i++)
  {
    x = (x << 8) | pcCredentials[i];
    ulLeft += 8;
    while (ulLeft > 6)
    {
      pcBasicEncoding[n++] = gaucBase64[(x >> (ulLeft - 6)) & 0x3f];
      ulLeft -= 6;
    }
  }
  if (ulLeft != 0)
  {
    pcBasicEncoding[n++] = gaucBase64[(x << (6 - ulLeft)) & 0x3f];
    pcBasicEncoding[n++] = '=';
  }
  if (ulLeft == 2)
  {
    pcBasicEncoding[n++] = '=';
  }
  pcBasicEncoding[n] = 0;

  HttpFreeData(pcCredentials);

  return pcBasicEncoding;
}


/*-
 ***********************************************************************
 *
 * HttpEscape
 *
 ***********************************************************************
 */
char *
HttpEscape(char *pcUnEscaped, char *pcError)
{
  const char          acRoutine[] = "HttpEscape()";
  char               *pcEscaped = NULL;
  char               *pc = NULL;
  int                 i = 0;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Escape everything but [a-zA-Z0-9_.-/\]. Convert spaces to '+'.
   *
   *********************************************************************
   */

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  iLength = strlen(pcUnEscaped) * 3;
  pcEscaped = malloc(iLength + 1);
  if (pcEscaped == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcEscaped[0] = 0;

  for (i = 0, pc = pcUnEscaped; *pc; pc++)
  {
    if (isalnum((int) *pc) || *pc == '_' || *pc == '.' || *pc == '-' || *pc == '/' || *pc == '\\')
    {
      pcEscaped[i++] = *pc;
    }
    else if (*pc == ' ')
    {
      pcEscaped[i++] = '+';
    }
    else
    {
      pcEscaped[i++] = '%';
      pcEscaped[i++] = gaucBase16[((*pc >> 4) & 0x0f)];
      pcEscaped[i++] = gaucBase16[((*pc >> 0) & 0x0f)];
    }
  }
  pcEscaped[i] = 0;

  return pcEscaped;
}


/*-
 ***********************************************************************
 *
 * HttpFreeData
 *
 ***********************************************************************
 */
void
HttpFreeData(char *pcData)
{
  if (pcData != NULL)
  {
    free(pcData);
  }
}


/*-
 ***********************************************************************
 *
 * HttpFreeUrl
 *
 ***********************************************************************
 */
void
HttpFreeUrl(HTTP_URL *psUrl)
{
  if (psUrl != NULL)
  {
    if (psUrl->pcUser != NULL)
    {
      free(psUrl->pcUser);
    }
    if (psUrl->pcPass != NULL)
    {
      free(psUrl->pcPass);
    }
    if (psUrl->pcHost != NULL)
    {
      free(psUrl->pcHost);
    }
    if (psUrl->pcPort != NULL)
    {
      free(psUrl->pcPort);
    }
    if (psUrl->pcPath != NULL)
    {
      free(psUrl->pcPath);
    }
    if (psUrl->pcQuery != NULL)
    {
      free(psUrl->pcQuery);
    }
    if (psUrl->pcMeth != NULL)
    {
      free(psUrl->pcMeth);
    }
    if (psUrl->pcJobId != NULL)
    {
      free(psUrl->pcJobId);
    }
    if (psUrl->pcProxyUser != NULL)
    {
      free(psUrl->pcProxyUser);
    }
    if (psUrl->pcProxyPass != NULL)
    {
      free(psUrl->pcProxyPass);
    }
    if (psUrl->pcProxyHost != NULL)
    {
      free(psUrl->pcProxyHost);
    }
    if (psUrl->pcProxyPort != NULL)
    {
      free(psUrl->pcProxyPort);
    }
    free(psUrl);
  }
}


/*-
 ***********************************************************************
 *
 * HttpHexToInt
 *
 ***********************************************************************
 */
int
HttpHexToInt(int i)
{
  if (isdigit(i))
  {
    return i - '0';
  }
  else if ((i >= 'a') && (i <= 'f'))
  {
    return i + 10 - 'a';
  }
  else if ((i >= 'A') && (i <= 'F'))
  {
    return i + 10 - 'A';
  }
  else
  {
    return -1;
  }
}


/*-
 ***********************************************************************
 *
 * HttpNewUrl
 *
 ***********************************************************************
 */
HTTP_URL *
HttpNewUrl(char *pcError)
{
  const char          acRoutine[] = "HttpNewUrl()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
  HTTP_URL           *psUrl = NULL;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory with HttpFreeUrl().
   *
   *********************************************************************
   */
  psUrl = malloc(sizeof(HTTP_URL));
  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(psUrl, 0, sizeof(HTTP_URL));

  /*-
   *********************************************************************
   *
   * Set the default request method.
   *
   *********************************************************************
   */
  iError = HttpSetUrlMeth(psUrl, HTTP_DEFAULT_HTTP_METHOD, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default job id.
   *
   *********************************************************************
   */
  iError = HttpSetUrlJobId(psUrl, HTTP_DEFAULT_JOB_ID, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default download limit.
   *
   *********************************************************************
   */
  HttpSetUrlDownloadLimit(psUrl, HTTP_MAX_MEMORY_SIZE);

  /*-
   *********************************************************************
   *
   * Allocate at least one byte for each of the proxy pointers.
   *
   *********************************************************************
   */
  iError = HttpSetDynamicString(&psUrl->pcProxyHost, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyPort, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyUser, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyPass, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    return NULL;
  }

  return psUrl;
}


/*-
 ***********************************************************************
 *
 * HttpParseAddress
 *
 ***********************************************************************
 */
int
HttpParseAddress(char *pcUserPassHostPort, HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpParseAddress()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcHostPort = NULL;
  char               *pcT = NULL;
  char               *pcUserPass = NULL;
  int                 iLength = 0;

  if (pcUserPassHostPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined UserPassHostPort.", acRoutine);
    return -1;
  }
  iLength = strlen(pcUserPassHostPort);

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  pcUserPass = malloc(iLength + 1);
  if (pcUserPass == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return -1;
  }
  pcUserPass[0] = 0;

  pcHostPort = malloc(iLength + 1);
  if (pcHostPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    HttpFreeData(pcUserPass);
    return -1;
  }
  pcHostPort[0] = 0;

  /*-
   *********************************************************************
   *
   * Separate User:Pass from Host:Port. Look for the first at-sign,
   * and mark it. If no at-sign was found, assume that User:Pass was
   * not specified.
   *
   *********************************************************************
   */
  pcT = strstr(pcUserPassHostPort, "@");
  if (pcT != NULL)
  {
    *pcT++ = 0;
    strncpy(pcUserPass, pcUserPassHostPort, iLength + 1);
    strncpy(pcHostPort, pcT, iLength + 1);
  }
  else
  {
    strncpy(pcHostPort, pcUserPassHostPort, iLength + 1);
  }

  if (HttpParseUserPass(pcUserPass, psUrl, acLocalError) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeData(pcUserPass);
    HttpFreeData(pcHostPort);
    return -1;
  }

  if (HttpParseHostPort(pcHostPort, psUrl, acLocalError) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeData(pcUserPass);
    HttpFreeData(pcHostPort);
    return -1;
  }

  HttpFreeData(pcUserPass);
  HttpFreeData(pcHostPort);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParseGreLine (General, Response, Entity)
 *
 ***********************************************************************
 */
int
HttpParseGreLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HttpParseGreLine()";
  char               *pc = NULL;
  char               *pcEnd = NULL;
  char               *pcFieldName = NULL;
  char               *pcFieldValue = NULL;
  char               *pcTempLine = NULL;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  iLength = strlen(pcLine);
  if (iLength > 0)
  {
    pcTempLine = malloc(iLength + 1);
    if (pcTempLine == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
      return -1;
    }
    strncpy(pcTempLine, pcLine, iLength + 1);
  }
  else
  {
    return 0;
  }

  /*-
   *********************************************************************
   *
   * Look for the first colon, and mark it to isolate the field-name.
   *
   *********************************************************************
   */
  if ((pcFieldValue = strstr(pcTempLine, ":")) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Message-Header = [%s]: Invalid header.", acRoutine, pcTempLine);
    HttpFreeData(pcTempLine);
    return -1;
  }
  *pcFieldValue++ = 0;
  pcFieldName = pcTempLine;

  /*-
   *********************************************************************
   *
   * Burn any leading white space off field-value.
   *
   *********************************************************************
   */
  while (*pcFieldValue != 0 && isspace((int) *pcFieldValue))
  {
    pcFieldValue++;
  }

  /*-
   *********************************************************************
   *
   * Burn any trailing white space off field-value.
   *
   *********************************************************************
   */
  iLength = strlen(pcFieldValue);
  if (iLength > 0)
  {
    pcEnd = &pcFieldValue[iLength - 1];
    while (iLength > 0 && isspace((int) *pcEnd))
    {
      *pcEnd-- = 0;
      iLength--;
    }
  }

  /*-
   *********************************************************************
   *
   * Identify the field, and parse its value.
   *
   *********************************************************************
   */
  if (strcasecmp(pcFieldName, FIELD_ContentLength) == 0)
  {
    if (psResponseHeader->iContentLengthFound)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%s]: Field already defined.", acRoutine, pcFieldValue);
      HttpFreeData(pcTempLine);
      return -1;
    }
    for (pc = pcFieldValue; *pc; pc++)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%s]: Value must contain all digits.", acRoutine, pcFieldValue);
        HttpFreeData(pcTempLine);
        return -1;
      }
    }
    psResponseHeader->ui32ContentLength = strtoul(pcFieldValue, NULL, 10);
    if (errno == ERANGE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: strtoul(): Content-Length = [%s]: Value too large.", acRoutine, pcFieldValue);
      HttpFreeData(pcTempLine);
      return -1;
    }
    psResponseHeader->iContentLengthFound = 1;
  }

  else if (strcasecmp(pcFieldName, FIELD_JobId) == 0)
  {
    if (psResponseHeader->iJobIdFound)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Job-Id = [%s]: Field already defined.", acRoutine, pcFieldValue);
      HttpFreeData(pcTempLine);
      return -1;
    }
    if (iLength < 1 || iLength > HTTP_JOB_ID_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Job-Id = [%s]: Value has invalid length (%d).", acRoutine, pcFieldValue, iLength);
      HttpFreeData(pcTempLine);
      return -1;
    }
    strncpy(psResponseHeader->acJobId, pcFieldValue, HTTP_JOB_ID_SIZE);
    psResponseHeader->iJobIdFound = 1;
  }

  else if (strcasecmp(pcFieldName, FIELD_TransferEncoding) == 0)
  {
/* FIXME Add support for chunked transfers. */
  }

#ifdef USE_DSV
  else if (strcasecmp(pcFieldName, FIELD_WebJobPayloadSignature) == 0)
  {
    if (psResponseHeader->iWebJobPayloadSignatureFound)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: WebJob-Payload-Signature = [%s]: Field already defined.", acRoutine, pcFieldValue);
      HttpFreeData(pcTempLine);
      return -1;
    }
    if (iLength < 1 || iLength > HTTP_WEBJOB_PAYLOAD_SIGNATURE_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: WebJob-Payload-Signature = [%s]: Value has invalid length (%d).", acRoutine, pcFieldValue, iLength);
      HttpFreeData(pcTempLine);
      return -1;
    }
    strncpy(psResponseHeader->acWebJobPayloadSignature, pcFieldValue, HTTP_WEBJOB_PAYLOAD_SIGNATURE_SIZE);
    psResponseHeader->iWebJobPayloadSignatureFound = 1;
  }
#endif

  /* ADD aditional fields here */

  HttpFreeData(pcTempLine);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParseHeader
 *
 ***********************************************************************
 */
int
HttpParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HttpParseHeader()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcE = NULL;
  char               *pcS = NULL;
  int                 iError = 0;

  pcS = pcE = pcResponseHeader;

  memset(psResponseHeader, 0, sizeof(HTTP_RESPONSE_HDR));

  HTTP_TERMINATE_LINE(pcE);

  iError = HttpParseStatusLine(pcS, psResponseHeader, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  while (++pcE < &pcResponseHeader[iResponseHeaderLength])
  {
    pcS = pcE;
    HTTP_TERMINATE_LINE(pcE);
    iError = HttpParseGreLine(pcS, psResponseHeader, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParseHostPort
 *
 ***********************************************************************
 */
int
HttpParseHostPort(char *pcHostPort, HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpParseHostPort()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcT = NULL;
  int                 iError = 0;

  if (pcHostPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined HostPort.", acRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Separate Host from Port. Look for the first colon, and mark it.
   * If no colon was found, assume that a port was not specified.
   *
   *********************************************************************
   */
  pcT = strstr(pcHostPort, ":");
  if (pcT != NULL)
  {
    *pcT++ = 0;
    iError = HttpSetUrlPort(psUrl, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member, and set its default value. */
  {
    switch (psUrl->iScheme)
    {
    case HTTP_SCHEME_HTTPS:
      iError = HttpSetUrlPort(psUrl, "443", acLocalError);
      break;
    default:
      iError = HttpSetUrlPort(psUrl, "80", acLocalError);
      break;
    }
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }

  iError = HttpSetUrlHost(psUrl, pcHostPort, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParsePathQuery
 *
 ***********************************************************************
 */
int
HttpParsePathQuery(char *pcPathQuery, HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpParsePathQuery()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcT = NULL;
  int                 iError = 0;

  if (pcPathQuery == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined PathQuery.", acRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Separate Path from Query. Look for the first question mark, and
   * mark it. If no question mark was found, assume that a query was
   * not specified.
   *
   *********************************************************************
   */
  pcT = strstr(pcPathQuery, "?");
  if (pcT != NULL)
  {
    *pcT++ = 0;
    iError = HttpSetUrlQuery(psUrl, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HttpSetUrlQuery(psUrl, "", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  iError = HttpSetUrlPath(psUrl, pcPathQuery, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParseStatusLine
 *
 ***********************************************************************
 */
int
HttpParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HttpParseStatusLine()";
  char               *pcE = NULL;
  char               *pcS = NULL;
  char               *pcT = NULL;

  pcS = pcE = pcLine;

  pcT = NULL;

  HTTP_TERMINATE_FIELD(pcE);

  /*-
   *********************************************************************
   *
   * Check the HTTP-Version for compliance per section 3.1 of RFC2068.
   * Major and minor numbers must be treated as separate integers, and
   * leading zeros must be ignored. RE = HTTP/[0-9]{1,}\.[0-9]{1,}
   *
   *********************************************************************
   */
  if (strncmp(pcS, "HTTP/", 5) == 0)
  {
    pcS += 5;
    if ((pcT = strstr(pcS, ".")) != NULL)
    {
      *pcT = 0;
      pcT = pcS;
      while (*pcT)
      {
        if (!isdigit((int) *pcT))
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Major Version = [%s]: Invalid Version.", acRoutine, pcS);
          return -1;
        }
        pcT++;
      }
      psResponseHeader->iMajorVersion = atoi(pcS);
      pcT++;
      pcS = pcT;
      while (*pcT)
      {
        if (!isdigit((int) *pcT))
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Minor Version = [%s]: Invalid Version.", acRoutine, pcS);
          return -1;
        }
        pcT++;
      }
      psResponseHeader->iMinorVersion = atoi(pcS);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Major/Minor Version = [%s]: Invalid Version.", acRoutine, pcS);
      return -1;
    }
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Status Line = [%s]: Invalid Status.", acRoutine, pcS);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Skip "any amount of SP or HT" per section 19.3 of RFC2068.
   *
   *********************************************************************
   */
  HTTP_SKIP_SPACES_TABS(pcE);

  pcS = pcE;

  HTTP_TERMINATE_FIELD(pcE);

  /*-
   *********************************************************************
   *
   * Check the Status-Code for compliance per section 6.1.1 of RFC2068.
   * The Status-Code is a 3-digit integer. RE = [1-5][0-9]{2}
   *
   * 1XX = Informational
   * 2XX = Success
   * 3XX = Redirection
   * 4XX = Client Error
   * 5XX = Server Error
   *
   * HTTP status codes are extensible. RE = [0-9]{3}
   *
   *********************************************************************
   */
  if (strlen(pcS) == 3 && isdigit((int) (*(pcS))) && isdigit((int) (*(pcS+1))) && isdigit((int) (*(pcS+2))))
  {
    psResponseHeader->iStatusCode = atoi(pcS);
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Status-Code = [%s]: Invalid Code.", acRoutine, pcS);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Skip "any amount of SP or HT" per section 19.3 of RFC2068.
   *
   *********************************************************************
   */
  HTTP_SKIP_SPACES_TABS(pcE);

  pcS = pcE;

  strncpy(psResponseHeader->acReasonPhrase, pcS, HTTP_REASON_PHRASE_SIZE - 1);
  psResponseHeader->acReasonPhrase[HTTP_REASON_PHRASE_SIZE - 1] = 0;

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpParseUrl
 *
 ***********************************************************************
 */
HTTP_URL *
HttpParseUrl(char *pcUrl, char *pcError)
{
  const char          acRoutine[] = "HttpParseUrl()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcS = NULL;
  char               *pcT = NULL;
  char               *pcTempUrl = NULL;
  int                 iError = 0;
  int                 iLength = 0;
  HTTP_URL           *psUrl = NULL;

  /*-
   *********************************************************************
   *
   * REGEX revision 1.32 and below:
   *
   *   scheme://(user(:pass)?@)?host(:port)?/(path(\?query)?)?
   *
   * REGEX revision 1.33 and above:
   *
   *   scheme://(user(:pass)?@)?host(:port)?(/path(\?query)?)?
   *
   *********************************************************************
   */

  /*-
   *********************************************************************
   *
   * No URL, no go.
   *
   *********************************************************************
   */
  if (pcUrl == NULL || (iLength = strlen(pcUrl)) < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid/Undefined URL.", acRoutine);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate and initialize memory for the URL and its components.
   *
   *********************************************************************
   */
  psUrl = HttpNewUrl(acLocalError);
  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  pcTempUrl = malloc(iLength + 1);
  if (pcTempUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    HttpFreeUrl(psUrl);
    return NULL;
  }
  strncpy(pcTempUrl, pcUrl, iLength + 1);

  /*-
   *********************************************************************
   *
   * Determine the scheme. Supported schemes are file, http, and https.
   *
   *********************************************************************
   */
  pcS = pcTempUrl;
  if (strncasecmp("file://", pcS, 7) == 0)
  {
    psUrl->iScheme = HTTP_SCHEME_FILE;
    pcS += 7;
  }
  else if (strncasecmp("http://", pcS, 7) == 0)
  {
    psUrl->iScheme = HTTP_SCHEME_HTTP;
    pcS += 7;
  }
  else if (strncasecmp("https://", pcS, 8) == 0)
  {
    psUrl->iScheme = HTTP_SCHEME_HTTPS;
    pcS += 8;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unknown/Unsupported URL Scheme.", acRoutine);
    HttpFreeUrl(psUrl);
    HttpFreeData(pcTempUrl);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Separate User:Pass@Host:Port from Path?Query. Find the first '/',
   * and mark it. If there is no '/', be forgiving and assume that the
   * user omitted it.
   *
   *********************************************************************
   */
  pcT = strstr(pcS, "/");
  if (pcT != NULL)
  {
    /*-
     *******************************************************************
     *
     * Parse the Path?Query part.
     *
     *******************************************************************
     */
    if (HttpParsePathQuery(pcT, psUrl, acLocalError) == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeUrl(psUrl);
      HttpFreeData(pcTempUrl);
      return NULL;
    }
    *pcT = 0;
  }
  else
  {
    /*-
     *******************************************************************
     *
     * Add the implied Path (i.e., '/'), and force the Query to be "".
     *
     *******************************************************************
     */
    iError = HttpSetUrlPath(psUrl, "/", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeUrl(psUrl);
      HttpFreeData(pcTempUrl);
      return NULL;
    }

    iError = HttpSetUrlQuery(psUrl, "", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeUrl(psUrl);
      HttpFreeData(pcTempUrl);
      return NULL;
    }
  }

  /*-
   *********************************************************************
   *
   * Parse the User:Pass@Host:Port part.
   *
   *********************************************************************
   */
  if (HttpParseAddress(pcS, psUrl, acLocalError) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HttpFreeUrl(psUrl);
    HttpFreeData(pcTempUrl);
    return NULL;
  }

  HttpFreeData(pcTempUrl);
  return psUrl;
}


/*-
 ***********************************************************************
 *
 * HttpParseUserPass
 *
 ***********************************************************************
 */
int
HttpParseUserPass(char *pcUserPass, HTTP_URL *psUrl, char *pcError)
{
  const char          acRoutine[] = "HttpParseUserPass()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcT = NULL;
  int                 iError = 0;

  if (pcUserPass == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined UserPass.", acRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Separate User from Pass. Look for the first colon, and mark it.
   * If no colon was found, assume that a password was not specified.
   *
   *********************************************************************
   */
  pcT = strstr(pcUserPass, ":");
  if (pcT != NULL)
  {
    *pcT++ = 0;
    iError = HttpSetUrlPass(psUrl, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HttpSetUrlPass(psUrl, "", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  iError = HttpSetUrlUser(psUrl, pcUserPass, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Make sure that we have a username, if a password was specified.
   *
   *********************************************************************
   */
  if (psUrl->pcPass[0] != 0 && psUrl->pcUser[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Username.", acRoutine);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpReadDataIntoMemory
 *
 ***********************************************************************
 */
int
HttpReadDataIntoMemory(SOCKET_CONTEXT *psSocketCTX, void **ppvData, APP_UI32 ui32ContentLength, int iFlags, char *pcError)
{
  const char          acRoutine[] = "HttpReadDataIntoMemory()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char              **ppcData = NULL;
  int                 iEmptyRead = 0;
  int                 iNRead = 0;
  int                 iToRead = 0;
  APP_UI32            ui32Offset = 0;

  ppcData = (char **) ppvData;

  /*-
   *********************************************************************
   *
   * Check flags and modify default behavior as necessary.
   *
   *********************************************************************
   */
  /* Note: Flags are not used at this time. */

  /*-
   *********************************************************************
   *
   * Check/Limit Content-Length.
   *
   *********************************************************************
   */
  if (ui32ContentLength > (APP_UI32) HTTP_MAX_MEMORY_SIZE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%u] > [%u]: Length exceeds internally defined limit.",
      acRoutine,
      ui32ContentLength,
      (APP_UI32) HTTP_MAX_MEMORY_SIZE
      );
    return -1;
  }

  iToRead = (int) ui32ContentLength;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  *ppcData = malloc(iToRead);
  if (*ppcData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Read the data.
   *
   *********************************************************************
   */
  while (iToRead > 0 && !iEmptyRead)
  {
    iNRead = SocketRead(psSocketCTX, &(*ppcData)[ui32Offset], iToRead, acLocalError);
    switch (iNRead)
    {
    case 0:
      iEmptyRead = 1;
      break;
    case -1:
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HttpFreeData(*ppcData);
      return -1;
      break;
    default:
      ui32Offset += iNRead;
      iToRead -= iNRead;
      break;
    }
  }
  if (ui32Offset != ui32ContentLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Actual-Length = [%u] != [%u]: Actual-Length vs Content-Length Mismatch.",
      acRoutine,
      ui32Offset,
      ui32ContentLength
      );
    HttpFreeData(*ppcData);
    return -1;
  }
  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpReadDataIntoStream
 *
 ***********************************************************************
 */
int
HttpReadDataIntoStream(SOCKET_CONTEXT *psSocketCTX, FILE *pFile, APP_UI32 ui32ContentLength, int iFlags, char *pcError)
{
  const char          acRoutine[] = "HttpReadDataIntoStream()";
  char                acData[HTTP_IO_BUFSIZE];
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iContentLengthRequired = 1;
  int                 iEmptyRead = 0;
  int                 iNRead = 0;
  APP_UI32            ui32Offset = 0;
  APP_UI32            ui32ToRead = ui32ContentLength;

  /*-
   *********************************************************************
   *
   * Check flags and modify default behavior as necessary.
   *
   *********************************************************************
   */
  if ((iFlags & HTTP_FLAG_CONTENT_LENGTH_OPTIONAL))
  {
    iContentLengthRequired = 0;
  }

  /*-
   *********************************************************************
   *
   * Read the data.
   *
   *********************************************************************
   */
  while (!iEmptyRead)
  {
    iNRead = SocketRead(psSocketCTX, acData, HTTP_IO_BUFSIZE, acLocalError);
    switch (iNRead)
    {
    case 0:
      iEmptyRead = 1;
      break;
    case -1:
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
      break;
    default:
      ui32Offset += fwrite(acData, 1, iNRead, pFile);
      if (ferror(pFile))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): %s", acRoutine, strerror(errno));
        return -1;
      }
      ui32ToRead -= iNRead;
      break;
    }
    if (iContentLengthRequired && ui32ToRead < 1)
    {
      break;
    }
  }
  if (iContentLengthRequired && ui32Offset != ui32ContentLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Actual-Length = [%u] != [%u]: Actual-Length vs Content-Length Mismatch.",
      acRoutine,
      ui32Offset,
      ui32ContentLength
      );
    return -1;
  }
  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpReadHeader
 *
 ***********************************************************************
 */
int
HttpReadHeader(SOCKET_CONTEXT *psSocketCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError)
{
  const char          acRoutine[] = "HttpReadHeader()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iHdrOk = 0;
  int                 iIndex = 0;
  int                 iNRead = 0;

  for (iIndex = iHdrOk = 0; iIndex < iMaxResponseLength && !iHdrOk; iIndex++)
  {
    iNRead = SocketRead(psSocketCTX, &pcResponseHeader[iIndex], 1, acLocalError);
    switch (iNRead)
    {
    case 0:
      break;
    case 1:
      /*-
       *********************************************************************
       *
       * Recognize LFs as line terminators per section 19.3 of RFC2068.
       *
       *********************************************************************
       */
      if (iIndex > 1 && strncmp(&pcResponseHeader[iIndex - 1], "\n\n", 2) == 0)
      {
        iHdrOk = 1;
      }
      else if (iIndex > 2 && strncmp(&pcResponseHeader[iIndex - 2], "\n\r\n", 3) == 0)
      {
        iHdrOk = 1;
      }
      else if (iIndex > 3 && strncmp(&pcResponseHeader[iIndex - 3], "\r\n\r\n", 4) == 0)
      {
        iHdrOk = 1;
      }
      else
      {
        iHdrOk = 0;
      }
      break;
    case -1:
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: Read Count = [%d] != [1]: Invalid Count.", acRoutine, iNRead);
      break;
    }
  }
  if (!iHdrOk)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unterminated header or size exceeds %d bytes.", acRoutine, iMaxResponseLength);
    return -1;
  }
  return iIndex;
}


/*-
 ***********************************************************************
 *
 * HttpSetDynamicString
 *
 ***********************************************************************
 */
int
HttpSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          acRoutine[] = "HttpSetDynamicString()";
  char               *pcTempValue = NULL;
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  iLength = strlen(pcNewValue);
  pcTempValue = realloc(*ppcValue, iLength + 1);
  {
    if (pcTempValue == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
      return -1;
    }
    *ppcValue = pcTempValue;
  }
  strncpy(*ppcValue, pcNewValue, iLength + 1);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlDownloadLimit
 *
 ***********************************************************************
 */
void
HttpSetUrlDownloadLimit(HTTP_URL *psUrl, APP_UI32 ui32Limit)
{
  psUrl->ui32DownloadLimit = ui32Limit;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlHost
 *
 ***********************************************************************
 */
int
HttpSetUrlHost(HTTP_URL *psUrl, char *pcHost, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlHost()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
  struct hostent     *psHostEntry = NULL;
#ifdef WIN32
  DWORD               dwStatus = 0;
  WORD                wVersion = 0;
  WSADATA             wsaData;
#endif

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcHost, pcHost, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Lookup/Set the IP address.
   *
   *********************************************************************
   */
  if (psUrl->pcHost[0] == 0)
  {
    if (psUrl->iScheme != HTTP_SCHEME_FILE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Hostname or IP address.", acRoutine);
      return -1;
    }
  }
  else
  {
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
    psUrl->ui32Ip = inet_addr(psUrl->pcHost);
    if (psUrl->ui32Ip == INADDR_NONE)
    {
#ifdef WIN32
      wVersion = (WORD)(1) | ((WORD)(1) << 8); /* MAKEWORD(1, 1) */
      dwStatus = WSAStartup(wVersion, &wsaData);
      if (dwStatus != 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: WSAStartup(): %u", acRoutine, dwStatus);
        return -1;
      }
#endif
      psHostEntry = gethostbyname(psUrl->pcHost);
      if (psHostEntry == NULL)
      {
/* FIXME Add hstrerror() for UNIX and WSAGetLastError() for WINX. */
        snprintf(pcError, MESSAGE_SIZE, "%s: gethostbyname(): DNS Lookup Failed.", acRoutine);
        return -1;
      }
      else
      {
        psUrl->ui32Ip = ((struct in_addr *)(psHostEntry->h_addr))->s_addr;
      }
#ifdef WIN32
      WSACleanup();
#endif
    }
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlJobId
 *
 ***********************************************************************
 */
int
HttpSetUrlJobId(HTTP_URL *psUrl, char *pcJobId, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlJobId()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcJobId, pcJobId, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlMeth
 *
 ***********************************************************************
 */
int
HttpSetUrlMeth(HTTP_URL *psUrl, char *pcMeth, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlMeth()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcMeth, pcMeth, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlPass
 *
 ***********************************************************************
 */
int
HttpSetUrlPass(HTTP_URL *psUrl, char *pcPass, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlPass()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcPass, pcPass, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlPath
 *
 ***********************************************************************
 */
int
HttpSetUrlPath(HTTP_URL *psUrl, char *pcPath, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlPath()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcPath, pcPath, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlPort
 *
 ***********************************************************************
 */
int
HttpSetUrlPort(HTTP_URL *psUrl, char *pcPort, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlPort()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pc = NULL;
  int                 iError = 0;
  int                 iPort = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcPort, pcPort, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check/Set the numeric port value.
   *
   *********************************************************************
   */
  if (psUrl->pcPort[0] && strlen(psUrl->pcPort) <= 5)
  {
    pc = psUrl->pcPort;
    while (*pc)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must contain all digits.", acRoutine, psUrl->pcPort);
        return -1;
      }
      pc++;
    }
    iPort = atoi(psUrl->pcPort);
    if (iPort < 1 || iPort > 65535)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must be 1-65535.", acRoutine, psUrl->pcPort);
      return -1;
    }
    psUrl->ui16Port = (APP_UI16) iPort;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value could not be converted.", acRoutine, psUrl->pcPort);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlProxyHost
 *
 ***********************************************************************
 */
int
HttpSetUrlProxyHost(HTTP_URL *psUrl, char *pcProxyHost, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlHost()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
  struct hostent     *psHostEntry = NULL;
#ifdef WIN32
  DWORD               dwStatus = 0;
  WORD                wVersion = 0;
  WSADATA             wsaData;
#endif

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyHost, pcProxyHost, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Lookup/Set the IP address.
   *
   *********************************************************************
   */
  if (psUrl->pcProxyHost[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Hostname or IP address.", acRoutine);
    return -1;
  }
  else
  {
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
    psUrl->ui32ProxyIp = inet_addr(psUrl->pcProxyHost);
    if (psUrl->ui32ProxyIp == INADDR_NONE)
    {
#ifdef WIN32
      wVersion = (WORD)(1) | ((WORD)(1) << 8); /* MAKEWORD(1, 1) */
      dwStatus = WSAStartup(wVersion, &wsaData);
      if (dwStatus != 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: WSAStartup(): %u", acRoutine, dwStatus);
        return -1;
      }
#endif
      psHostEntry = gethostbyname(psUrl->pcProxyHost);
      if (psHostEntry == NULL)
      {
/* FIXME Add hstrerror() for UNIX and WSAGetLastError() for WINX. */
        snprintf(pcError, MESSAGE_SIZE, "%s: gethostbyname(): DNS Lookup Failed.", acRoutine);
        return -1;
      }
      else
      {
        psUrl->ui32ProxyIp = ((struct in_addr *)(psHostEntry->h_addr))->s_addr;
      }
#ifdef WIN32
      WSACleanup();
#endif
    }
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlProxyPass
 *
 ***********************************************************************
 */
int
HttpSetUrlProxyPass(HTTP_URL *psUrl, char *pcProxyPass, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlProxyPass()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyPass, pcProxyPass, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlProxyPort
 *
 ***********************************************************************
 */
int
HttpSetUrlProxyPort(HTTP_URL *psUrl, char *pcProxyPort, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlProxyPort()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pc = NULL;
  int                 iError = 0;
  int                 iProxyPort = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyPort, pcProxyPort, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check/Set the numeric port value.
   *
   *********************************************************************
   */
  if (psUrl->pcProxyPort[0] && strlen(psUrl->pcProxyPort) <= 5)
  {
    pc = psUrl->pcProxyPort;
    while (*pc)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must contain all digits.", acRoutine, psUrl->pcProxyPort);
        return -1;
      }
      pc++;
    }
    iProxyPort = atoi(psUrl->pcProxyPort);
    if (iProxyPort < 1 || iProxyPort > 65535)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must be 1-65535.", acRoutine, psUrl->pcProxyPort);
      return -1;
    }
    psUrl->ui16ProxyPort = (APP_UI16) iProxyPort;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value could not be converted.", acRoutine, psUrl->pcProxyPort);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlProxyUser
 *
 ***********************************************************************
 */
int
HttpSetUrlProxyUser(HTTP_URL *psUrl, char *pcProxyUser, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlProxyUser()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcProxyUser, pcProxyUser, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlQuery
 *
 ***********************************************************************
 */
int
HttpSetUrlQuery(HTTP_URL *psUrl, char *pcQuery, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlQuery()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcQuery, pcQuery, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSetUrlUser
 *
 ***********************************************************************
 */
int
HttpSetUrlUser(HTTP_URL *psUrl, char *pcUser, char *pcError)
{
  const char          acRoutine[] = "HttpSetUrlUser()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HttpSetDynamicString(&psUrl->pcUser, pcUser, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpSubmitRequest
 *
 ***********************************************************************
 */
int
HttpSubmitRequest(HTTP_URL *psUrl, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HttpSubmitRequest()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acFileData[HTTP_IO_BUFSIZE];
  char                acSockData[HTTP_IO_BUFSIZE];
  char               *pcRequest = NULL;
  int                 iContentLengthRequired = 1;
  int                 iError = 0;
  int                 iNRead = 0;
  int                 iNSent = 0;
  int                 iRequestLength = 0;
  int                 iSocketType = 0;
  APP_UI32            ui32Size = 0;
  APP_UI64            ui64ContentLength = 0;
  HTTP_MEMORY_LIST   *pMList = NULL;
  HTTP_STREAM_LIST   *pSList = NULL;
  SOCKET_CONTEXT     *psSocketCTX = NULL;

  /*-
   *********************************************************************
   *
   * If psUrl is not defined, abort.
   *
   *********************************************************************
   */
  if (psUrl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check flags and modify default behavior as necessary.
   *
   *********************************************************************
   */
  if ((psUrl->iFlags & HTTP_FLAG_CONTENT_LENGTH_OPTIONAL))
  {
    iContentLengthRequired = 0;
  }

  /*-
   *********************************************************************
   *
   * Set the socket type based on the URL scheme.
   *
   *********************************************************************
   */
  switch (psUrl->iScheme)
  {
  case HTTP_SCHEME_FILE:
    iSocketType = -1;
    snprintf(pcError, MESSAGE_SIZE, "%s: Scheme = [file]: Unsupported Scheme.", acRoutine);
    return -1;
    break;
  case HTTP_SCHEME_HTTP:
    iSocketType = SOCKET_TYPE_REGULAR;
    break;
  case HTTP_SCHEME_HTTPS:
#ifdef USE_SSL
    iSocketType = SOCKET_TYPE_SSL;
#else
    iSocketType = -1;
    snprintf(pcError, MESSAGE_SIZE, "%s: Scheme = [https]: Unsupported Scheme.", acRoutine);
    return -1;
#endif
    break;
  default:
    iSocketType = -1;
    snprintf(pcError, MESSAGE_SIZE, "%s: Scheme = [%d]: Unsupported Scheme.", acRoutine, psUrl->iScheme);
    return -1;
    break;
  }

  /*-
   *********************************************************************
   *
   * Set the Content-Length. If it exceeds 32 bits, abort.
   *
   *********************************************************************
   */
  ui64ContentLength = 0;
  switch (iInputType)
  {
  case HTTP_IGNORE_INPUT:
    break;
  case HTTP_MEMORY_INPUT:
    for (pMList = (HTTP_MEMORY_LIST *)pInput; pMList != NULL; pMList = pMList->psNext)
    {
      ui64ContentLength += (APP_UI64) pMList->ui32Size;
    }
    break;
  case HTTP_STREAM_INPUT:
    for (pSList = (HTTP_STREAM_LIST *)pInput; pSList != NULL; pSList = pSList->psNext)
    {
      ui64ContentLength += (APP_UI64) pSList->ui32Size;
    }
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Input Type = [%d]: Invalid Type.", acRoutine, iInputType);
    return -1;
    break;
  }

  if (((ui64ContentLength >> 32) & 0xffffff))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unsupported URL Scheme.", acRoutine);
    return -1;
  }
  else
  {
    psUrl->ui32ContentLength = (unsigned long) ui64ContentLength;
  }

  /*-
   *********************************************************************
   *
   * Build the Request Header.
   *
   *********************************************************************
   */
  pcRequest = HttpBuildRequest(psUrl, acLocalError);
  if (pcRequest == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }
  iRequestLength = strlen(pcRequest);

  /*-
   *********************************************************************
   *
   * Connect to the server.
   *
   *********************************************************************
   */
  psSocketCTX = HttpConnect(psUrl, iSocketType, acLocalError);
  if (psSocketCTX == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Transmit the Request Header.
   *
   *********************************************************************
   */
  iNSent = SocketWrite(psSocketCTX, pcRequest, iRequestLength, acLocalError);
  if (iNSent != iRequestLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", acRoutine, iNSent, iRequestLength, acLocalError);
    HttpFreeData(pcRequest);
    SocketCleanup(psSocketCTX);
    return -1;
  }
  HttpFreeData(pcRequest);

  /*-
   *********************************************************************
   *
   * Transmit the Request Content, if any.
   *
   *********************************************************************
   */
  switch (iInputType)
  {
  case HTTP_IGNORE_INPUT:
    break;
  case HTTP_MEMORY_INPUT:
    for (pMList = (HTTP_MEMORY_LIST *)pInput; pMList != NULL; pMList = pMList->psNext)
    {
      ui32Size = pMList->ui32Size;
      do
      {
        iNRead = (ui32Size > 0x7fffffff) ? 0x7fffffff : (int) ui32Size;
        iNSent = SocketWrite(psSocketCTX, pMList->pcData, iNRead, acLocalError);
        if (iNSent != iNRead)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", acRoutine, iNSent, iNRead, acLocalError);
          SocketCleanup(psSocketCTX);
          return -1;
        }
        ui32Size -= iNSent;
      } while (ui32Size > 0);
    }
    break;
  case HTTP_STREAM_INPUT:
    for (pSList = (HTTP_STREAM_LIST *)pInput; pSList != NULL; pSList = pSList->psNext)
    {
      do
      {
        iNRead = fread(acFileData, 1, HTTP_IO_BUFSIZE, pSList->pFile);
        if (ferror(pSList->pFile))
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: fread(): %s", acRoutine, strerror(errno));
          SocketCleanup(psSocketCTX);
          return -1;
        }
        iNSent = SocketWrite(psSocketCTX, acFileData, iNRead, acLocalError);
        if (iNSent != iNRead)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", acRoutine, iNSent, iNRead, acLocalError);
          SocketCleanup(psSocketCTX);
          return -1;
        }
      } while (!feof(pSList->pFile));
    }
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Input Type = [%d]: Invalid Type.", acRoutine, iInputType);
    SocketCleanup(psSocketCTX);
    return -1;
    break;
  }

  /*-
   *********************************************************************
   *
   * Read the Response Header.
   *
   *********************************************************************
   */
  memset(acSockData, 0, HTTP_IO_BUFSIZE);

  iNRead = HttpReadHeader(psSocketCTX, acSockData, HTTP_IO_BUFSIZE - 1, acLocalError);
  if (iNRead == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SocketCleanup(psSocketCTX);
    return -1;
  }
  acSockData[iNRead] = 0;

  /*-
   *********************************************************************
   *
   * Parse the Response Header. If successful, HTTP status will be set.
   *
   *********************************************************************
   */
  iError = HttpParseHeader(acSockData, iNRead, psResponseHeader, acLocalError);
  if (iError != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SocketCleanup(psSocketCTX);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check the Status Code in the Response Header.
   *
   *********************************************************************
   */
  if (psResponseHeader->iStatusCode < 200 || psResponseHeader->iStatusCode > 299)
  {
    SocketCleanup(psSocketCTX);
    return 0;
  }

/* FIXME Add support for chunked transfers. */

  /*-
   *********************************************************************
   *
   * Check that a Content-Length was specified in the Response Header.
   *
   *********************************************************************
   */
  if (iContentLengthRequired && !psResponseHeader->iContentLengthFound)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: The server's response did not include a Content-Length.", acRoutine);
    SocketCleanup(psSocketCTX);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check Content-Length against download limit (zero means no limit).
   *
   *********************************************************************
   */
  if (psUrl->ui32DownloadLimit != 0 && psResponseHeader->ui32ContentLength > psUrl->ui32DownloadLimit)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%u] > [%u]: Length exceeds user defined limit.",
      acRoutine,
      psResponseHeader->ui32ContentLength,
      psUrl->ui32DownloadLimit
      );
    SocketCleanup(psSocketCTX);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Read and write the Response Content, if any.
   *
   *********************************************************************
   */
  switch (iOutputType)
  {
  case HTTP_IGNORE_OUTPUT:
    iError = 0;
    break;
  case HTTP_MEMORY_OUTPUT:
    iError = HttpReadDataIntoMemory(psSocketCTX, (void **) &pOutput, psResponseHeader->ui32ContentLength, psUrl->iFlags, acLocalError);
    break;
  case HTTP_STREAM_OUTPUT:
    iError = HttpReadDataIntoStream(psSocketCTX, (FILE *) pOutput, psResponseHeader->ui32ContentLength, psUrl->iFlags, acLocalError);
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Output Type = [%d]: Invalid Type.", acRoutine, iOutputType);
    SocketCleanup(psSocketCTX);
    return -1;
    break;
  }
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SocketCleanup(psSocketCTX);
    return -1;
  }

  SocketCleanup(psSocketCTX);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HttpUnEscape
 *
 ***********************************************************************
 */
char *
HttpUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError)
{
  const char          acRoutine[] = "HttpUnEscape()";
  char               *pc = NULL;
  char               *pcUnEscaped = NULL;
  int                 i = 0;
  int                 iHexDigit1 = 0;
  int                 iHexDigit2 = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcUnEscaped = malloc(strlen(pcEscaped));
  if (pcUnEscaped == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcUnEscaped[0] = 0;

  for (i = 0, pc = pcEscaped; *pc; i++)
  {
    if (*pc == '%')
    {
      iHexDigit1 = HttpHexToInt(*(pc + 1));
      iHexDigit2 = HttpHexToInt(*(pc + 2));
      if (iHexDigit1 == -1 || iHexDigit2 == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Malformed Hex Value.", acRoutine);
        HttpFreeData(pcUnEscaped);
        return NULL;
      }
      pcUnEscaped[i] = iHexDigit1 * 16 + iHexDigit2;
      pc += 3;
    }
    else if (*pc == '+')
    {
      pcUnEscaped[i] = ' ';
      pc++;
    }
    else
    {
      pcUnEscaped[i] = *pc;
      pc++;
    }
  }
  *piUnEscapedLength = i;

  return pcUnEscaped;
}
