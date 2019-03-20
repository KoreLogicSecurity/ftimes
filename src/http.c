/*
 ***********************************************************************
 *
 * $Id: http.c,v 1.1.1.1 2002/01/18 03:17:37 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

#include "all-includes.h"

static unsigned char ucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*-
 ***********************************************************************
 *
 * HTTPBuildRequest
 *
 ***********************************************************************
 */
char *
HTTPBuildRequest(HTTP_URL *ptURL, char *pcError)
{
  const char          cRoutine[] = "HTTPBuildRequest()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcBasicEncoding,
                     *pcRequest;
  int                 iRequestLength;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Make sure the URL and its members aren't NULL.
   *
   *********************************************************************
   */
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return NULL;
  }

  if (ptURL->pcUser == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Username.", cRoutine);
    return NULL;
  }

  if (ptURL->pcPass == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Password.", cRoutine);
    return NULL;
  }

  if (ptURL->pcHost == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Host.", cRoutine);
    return NULL;
  }

  if (ptURL->pcPort == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Port.", cRoutine);
    return NULL;
  }

  if (ptURL->pcPath == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Path.", cRoutine);
    return NULL;
  }

  if (ptURL->pcQuery == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Query.", cRoutine);
    return NULL;
  }

  if (ptURL->pcMeth == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL Method.", cRoutine);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Figure out how much memory we would need with everything enabled.
   * Then, tack on an extra Kbyte.
   *
   *********************************************************************
   */
  iRequestLength = 0;
  iRequestLength += strlen(ptURL->pcMeth);
  iRequestLength += strlen(ptURL->pcPath);
  iRequestLength += strlen(ptURL->pcQuery);
  iRequestLength += strlen("HTTP/1.1\r\n");
  iRequestLength += strlen("Host: \r\n");
  iRequestLength += strlen(ptURL->pcHost);
  iRequestLength += strlen("Content-type: text/plain\r\n");
  iRequestLength += strlen("Content-Length: 4294967295\r\n");
  iRequestLength += strlen("Authorization: Basic\r\n");
  iRequestLength += 4 * (strlen(ptURL->pcUser) + strlen(ptURL->pcPass));
  iRequestLength += strlen("\r\n");
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
  switch (ptURL->iAuthType)
  {
  case HTTP_AUTH_TYPE_BASIC:
    pcBasicEncoding = HTTPEncodeBasic(ptURL->pcUser, ptURL->pcPass, cLocalError);
    if (pcBasicEncoding == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      HTTPFreeData(pcRequest);
      return NULL;
    }
    sprintf(pcRequest, "%s %s%s%s HTTP/1.1\r\n"
                       "Host: %s:%d\r\n"
                       "Content-Type: application/octet-stream\r\n"
                       "Content-Length: %lu\r\n"
                       "Authorization: Basic %s\r\n"
                       "\r\n",
                       ptURL->pcMeth,
                       ptURL->pcPath,
                       (ptURL->pcQuery[0]) ? "?" : "",
                       (ptURL->pcQuery[0]) ? ptURL->pcQuery : "",
                       ptURL->pcHost,
                       ptURL->ui16Port,
                       ptURL->ui32ContentLength,
                       pcBasicEncoding
           );
    HTTPFreeData(pcBasicEncoding);
    break;
  default:
    sprintf(pcRequest, "%s %s%s%s HTTP/1.1\r\n"
                       "Host: %s:%d\r\n"
                       "Content-Type: application/octet-stream\r\n"
                       "Content-Length: %lu\r\n"
                       "\r\n",
                       ptURL->pcMeth,
                       ptURL->pcPath,
                       (ptURL->pcQuery[0]) ? "?" : "",
                       (ptURL->pcQuery[0]) ? ptURL->pcQuery : "",
                       ptURL->pcHost,
                       ptURL->ui16Port,
                       ptURL->ui32ContentLength
           );
    break;
  }

  return pcRequest;
}


/*-
 ***********************************************************************
 *
 * HTTPEncodeBasic
 *
 ***********************************************************************
 */
char *
HTTPEncodeBasic(char *pcUsername, char *pcPassword, char *pcError)
{
  const char          cRoutine[] = "HTTPEncodeBasic()";
  char               *pcBasicEncoding,
                     *pcCredentials;
  int                 i,
                      iLength,
                      iPassLength,
                      iUserLength,
                      n;
  unsigned long       x,
                      ulLeft;


  iUserLength = strlen(pcUsername);
  if (iUserLength == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Username.", cRoutine);
    return NULL;
  }

  iPassLength = strlen(pcPassword);
  if (iPassLength == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Password.", cRoutine);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    HTTPFreeData(pcBasicEncoding);
    return NULL;
  }
  snprintf(pcCredentials, iLength + 1, "%s:%s", pcUsername, pcPassword);

  for (i = 0, n = 0, x = 0, ulLeft = 0; i < iLength; i++)
  {
    x = (x << 8) | pcCredentials[i];
    ulLeft += 8;
    while (ulLeft > 6)
    {
      pcBasicEncoding[n++] = ucBase64[(x >> (ulLeft - 6)) & 0x3f];
      ulLeft -= 6;
    }
  }
  if (ulLeft != 0)
  {
    pcBasicEncoding[n++] = ucBase64[(x << (6 - ulLeft)) & 0x3f];
    pcBasicEncoding[n++] = '=';
  }
  if (ulLeft == 2)
  {
    pcBasicEncoding[n++] = '=';
  }
  pcBasicEncoding[n] = 0;

  HTTPFreeData(pcCredentials);

  return pcBasicEncoding;
}


/*-
 ***********************************************************************
 *
 * HTTPEscape
 *
 ***********************************************************************
 */
char *
HTTPEscape(char *pcUnEscaped, char *pcError)
{
  const char          cRoutine[] = "HTTPEscape()";
  char               *pcEscaped,
                     *pc;
  int                 i;

  /*-
   *********************************************************************
   *
   * Escape everything but [a-zA-Z0-9_.-]. Convert spaces to '+'.
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
  pcEscaped = malloc((3 * strlen(pcUnEscaped)) + 1);
  if (pcEscaped == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
      i += sprintf(&pcEscaped[i], "%%%02x", (unsigned char) *pc);
    }
  }
  pcEscaped[i] = 0;

  return pcEscaped;
}


/*-
 ***********************************************************************
 *
 * HTTPFreeData
 *
 ***********************************************************************
 */
void
HTTPFreeData(char *pcData)
{
  if (pcData != NULL)
  {
    free(pcData);
  }
}


/*-
 ***********************************************************************
 *
 * HTTPFreeURL
 *
 ***********************************************************************
 */
void
HTTPFreeURL(HTTP_URL *ptURL)
{
  if (ptURL != NULL)
  {
    if (ptURL->pcUser != NULL)
    {
      free(ptURL->pcUser);
    }
    if (ptURL->pcPass != NULL)
    {
      free(ptURL->pcPass);
    }
    if (ptURL->pcHost != NULL)
    {
      free(ptURL->pcHost);
    }
    if (ptURL->pcPort != NULL)
    {
      free(ptURL->pcPort);
    }
    if (ptURL->pcPath != NULL)
    {
      free(ptURL->pcPath);
    }
    if (ptURL->pcQuery != NULL)
    {
      free(ptURL->pcQuery);
    }
    if (ptURL->pcMeth != NULL)
    {
      free(ptURL->pcMeth);
    }
    free(ptURL);
  }
}


/*-
 ***********************************************************************
 *
 * HTTPHexToInt
 *
 ***********************************************************************
 */
int
HTTPHexToInt(int i)
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
 * HTTPNewURL
 *
 ***********************************************************************
 */
HTTP_URL *
HTTPNewURL(char *pcError)
{
  const char          cRoutine[] = "HTTPNewURL()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  HTTP_URL           *ptURL;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory with HTTPFreeURL().
   *
   *********************************************************************
   */
  ptURL = malloc(sizeof(HTTP_URL));
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  memset(ptURL, 0, sizeof(HTTP_URL));

  /*-
   *********************************************************************
   *
   * Set the default request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(ptURL, HTTP_DEFAULT_HTTP_METHOD, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    HTTPFreeURL(ptURL);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default download limit.
   *
   *********************************************************************
   */
  HTTPSetURLDownloadLimit(ptURL, HTTP_MAX_MEMORY_SIZE);

  return ptURL;
}


/*-
 ***********************************************************************
 *
 * HTTPParseAddress
 *
 ***********************************************************************
 */
int
HTTPParseAddress(char *pcUserPassHostPort, HTTP_URL *ptURL, char *pcError)
{
  const char          cRoutine[] = "HTTPParseAddress()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcHostPort,
                     *pcT,
                     *pcUserPass;

  if (pcUserPassHostPort == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined UserPassHostPort.", cRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  pcUserPass = malloc(strlen(pcUserPassHostPort) + 1);
  if (pcUserPass == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return -1;
  }
  pcUserPass[0] = 0;

  pcHostPort = malloc(strlen(pcUserPassHostPort) + 1);
  if (pcHostPort == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    HTTPFreeData(pcUserPass);
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
    strcpy(pcUserPass, pcUserPassHostPort);
    strcpy(pcHostPort, pcT);
  }
  else
  {
    strcpy(pcHostPort, pcUserPassHostPort);
  }

  if (HTTPParseUserPass(pcUserPass, ptURL, cLocalError) == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    HTTPFreeData(pcUserPass);
    HTTPFreeData(pcHostPort);
    return -1;
  }

  if (HTTPParseHostPort(pcHostPort, ptURL, cLocalError) == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    HTTPFreeData(pcUserPass);
    HTTPFreeData(pcHostPort);
    return -1;
  }

  HTTPFreeData(pcUserPass);
  HTTPFreeData(pcHostPort);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParseGRELine (General, Response, Entity)
 *
 ***********************************************************************
 */
int
HTTPParseGRELine(char *pcLine, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError)
{
  const char          cRoutine[] = "HTTPParseGRELine()";
  char               *pc,
                     *pcEnd,
                     *pcFieldName,
                     *pcFieldValue,
                     *pcTempLine;
  int                 iLength;

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
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
      return -1;
    }
    strcpy(pcTempLine, pcLine);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Message-Header = [%s]: Invalid header.", cRoutine, pcTempLine);
    HTTPFreeData(pcTempLine);
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
  while (isspace((int) *pcFieldValue))
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
  pcEnd = &pcFieldValue[iLength - 1];
  while (isspace((int) *pcEnd))
  {
    *pcEnd-- = 0;
    iLength--;
  }

  /*-
   *********************************************************************
   *
   * Initialize found variables.
   *
   *********************************************************************
   */
  ptResponseHeader->iContentLengthFound = 0;

  /*-
   *********************************************************************
   *
   * Identify the field, and parse its value.
   *
   *********************************************************************
   */
  if (strcasecmp(pcFieldName, FIELD_ContentLength) == 0)
  {
    ptResponseHeader->iContentLengthFound = 0;
    pc = pcFieldValue;
    while (*pc)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Content-Length = [%s]: Value must contain all digits.", cRoutine, pcFieldValue);
        HTTPFreeData(pcTempLine);
        return -1;
      }
      pc++;
    }
    if (strlen(pcFieldValue) > strlen("4294967295"))
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Content-Length = [%s]: Value too large to be converted to a 32 bit integer.", cRoutine, pcFieldValue);
      HTTPFreeData(pcTempLine);
      return -1;
    }
    ptResponseHeader->ui32ContentLength = strtol(pcFieldValue, NULL, 10);
    ptResponseHeader->iContentLengthFound = 1;
  }

  else if (strcasecmp(pcFieldName, FIELD_TransferEncoding) == 0)
  {
/* FIXME Add support for chunked transfers. */
  }

  /* ADD aditional fields here */

  HTTPFreeData(pcTempLine);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParseHeader
 *
 ***********************************************************************
 */
int
HTTPParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError)
{
  const char          cRoutine[] = "HTTPParseHeader()";
  char                cLocalError[ERRBUF_SIZE];
  char               *pcE,
                     *pcS;
  int                 iError,
                      iLength;

  iLength = iResponseHeaderLength;

  pcS = pcE = pcResponseHeader;

  memset(ptResponseHeader, 0, sizeof(HTTP_RESPONSE_HDR));

  HTTP_TERMINATE_LINE(pcE);

  iError = HTTPParseStatusLine(pcS, ptResponseHeader, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  while (++pcE < &pcResponseHeader[iResponseHeaderLength])
  {
    pcS = pcE;
    HTTP_TERMINATE_LINE(pcE);
    iError = HTTPParseGRELine(pcS, ptResponseHeader, cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParseHostPort
 *
 ***********************************************************************
 */
int
HTTPParseHostPort(char *pcHostPort, HTTP_URL *ptURL, char *pcError)
{
  const char          cRoutine[] = "HTTPParseHostPort()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcT;
  int                 iError;
  if (pcHostPort == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined HostPort.", cRoutine);
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
    iError = HTTPSetURLPort(ptURL, pcT, cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member, and set its default value. */
  {
    switch (ptURL->iScheme)
    {
    case HTTP_SCHEME_HTTPS:
      iError = HTTPSetURLPort(ptURL, "443", cLocalError);
      break;
    default:
      iError = HTTPSetURLPort(ptURL, "80", cLocalError);
      break;
    }
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }

  iError = HTTPSetURLHost(ptURL, pcHostPort, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParsePathQuery
 *
 ***********************************************************************
 */
int
HTTPParsePathQuery(char *pcPathQuery, HTTP_URL *ptURL, char *pcError)
{
  const char          cRoutine[] = "HTTPParsePathQuery()";
  char                cLocalError[ERRBUF_SIZE];
  char               *pcT;
  int                 iError;

  if (pcPathQuery == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined PathQuery.", cRoutine);
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
    iError = HTTPSetURLQuery(ptURL, pcT, cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HTTPSetURLQuery(ptURL, "", cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }
  iError = HTTPSetURLPath(ptURL, pcPathQuery, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParseStatusLine
 *
 ***********************************************************************
 */
int
HTTPParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError)
{
  const char          cRoutine[] = "HTTPParseStatusLine()";
  char               *pcE,
                     *pcS,
                     *pcT;

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
          snprintf(pcError, ERRBUF_SIZE, "%s: Major Version = [%s]: Invalid Version.", cRoutine, pcS);
          return -1;
        }
        pcT++;
      }
      ptResponseHeader->iMajorVersion = atoi(pcS);
      pcT++;
      pcS = pcT;
      while (*pcT)
      {
        if (!isdigit((int) *pcT))
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Minor Version = [%s]: Invalid Version.", cRoutine, pcS);
          return -1;
        }
        pcT++;
      }
      ptResponseHeader->iMinorVersion = atoi(pcS);
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Major/Minor Version = [%s]: Invalid Version.", cRoutine, pcS);
      return -1;
    }
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Status Line = [%s]: Invalid Status.", cRoutine, pcS);
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
    ptResponseHeader->iStatusCode = atoi(pcS);
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Status-Code = [%s]: Invalid Code.", cRoutine, pcS);
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

/* FIXME Bounds check this. */
  strncpy(ptResponseHeader->cReasonPhrase, pcS, sizeof(ptResponseHeader->cReasonPhrase));

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPParseURL
 *
 ***********************************************************************
 */
HTTP_URL *
HTTPParseURL(char *pcURL, char *pcError)
{
  const char          cRoutine[] = "HTTPParseURL()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcS,
                     *pcT,
                     *pcTempURL;
  int                 iLength;
  HTTP_URL           *ptURL;

  /*-
   *********************************************************************
   *
   * REGEX = scheme://(user(:pass)?@)?host(:port)?/(path(\?query)?)?
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
  if (pcURL == NULL || (iLength = strlen(pcURL)) < 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Invalid/Undefined URL.", cRoutine);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate and initialize memory for the URL and its components.
   *
   *********************************************************************
   */
  ptURL = HTTPNewURL(cLocalError);
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * This memory must be freed locally.
   *
   *********************************************************************
   */
  pcTempURL = malloc(iLength + 1);
  if (pcTempURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    HTTPFreeURL(ptURL);
    return NULL;
  }
  strcpy(pcTempURL, pcURL);

  /*-
   *********************************************************************
   *
   * Determine the scheme. Supported schemes are file, http, and https.
   *
   *********************************************************************
   */
  pcS = pcTempURL;
  if (strncasecmp("file://", pcS, 7) == 0)
  {
    ptURL->iScheme = HTTP_SCHEME_FILE;
    pcS += 7;
  }
  else if (strncasecmp("http://", pcS, 7) == 0)
  {
    ptURL->iScheme = HTTP_SCHEME_HTTP;
    pcS += 7;
  }
  else if (strncasecmp("https://", pcS, 8) == 0)
  {
    ptURL->iScheme = HTTP_SCHEME_HTTPS;
    pcS += 8;
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Unknown/Unsupported URL Scheme.", cRoutine);
    HTTPFreeURL(ptURL);
    HTTPFreeData(pcTempURL);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Separate User:Pass@Host:Port from Path?Query. Find the first '/',
   * and mark it.
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
    if (HTTPParsePathQuery(pcT, ptURL, cLocalError) == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      HTTPFreeURL(ptURL);
      HTTPFreeData(pcTempURL);
      return NULL;
    }
    *pcT = 0;

    /*-
     *******************************************************************
     *
     * Parse the User:Pass@Host:Port part.
     *
     *******************************************************************
     */
    if (HTTPParseAddress(pcS, ptURL, cLocalError) == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      HTTPFreeURL(ptURL);
      HTTPFreeData(pcTempURL);
      return NULL;
    }
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Invalid URL, Absolute URL Required.", cRoutine);
    HTTPFreeURL(ptURL);
    HTTPFreeData(pcTempURL);
    return NULL;
  }

  HTTPFreeData(pcTempURL);
  return ptURL;
}


/*-
 ***********************************************************************
 *
 * HTTPParseUserPass
 *
 ***********************************************************************
 */
int
HTTPParseUserPass(char *pcUserPass, HTTP_URL *ptURL, char *pcError)
{
  const char          cRoutine[] = "HTTPParseUserPass()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcT;
  int                 iError;

  if (pcUserPass == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined UserPass.", cRoutine);
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
    iError = HTTPSetURLPass(ptURL, pcT, cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HTTPSetURLPass(ptURL, "", cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
    }
  }
  iError = HTTPSetURLUser(ptURL, pcUserPass, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Make sure that we have a username, if a password was specified.
   *
   *********************************************************************
   */
  if (ptURL->pcPass[0] != 0 && ptURL->pcUser[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Username.", cRoutine);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPReadDataIntoMemory
 *
 ***********************************************************************
 */
int
HTTPReadDataIntoMemory(SOCKET_CONTEXT *psockCTX, char **ppcData, K_UINT32 ui32ContentLength, char *pcError)
{
  const char          cRoutine[] = "HTTPReadDataIntoMemory()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iNRead,
                      iZRead,
                      iToRead;
  K_UINT32            ui32Offset;

  ui32Offset = iZRead = 0;

  /*-
   *********************************************************************
   *
   * Check/Limit Content-Length.
   *
   *********************************************************************
   */
  if (ui32ContentLength > (K_UINT32) HTTP_MAX_MEMORY_SIZE)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Content-Length = [%lu] > [%d]: Length exceeds internally defined limit.", cRoutine, ui32ContentLength, HTTP_MAX_MEMORY_SIZE);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Read the data.
   *
   *********************************************************************
   */
  while (iToRead > 0 && !iZRead)
  {
    iNRead = SocketRead(psockCTX, &(*ppcData)[ui32Offset], iToRead, cLocalError);
    switch (iNRead)
    {
    case 0:
      iZRead = 1;
      break;
    case -1:
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      HTTPFreeData(*ppcData);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Actual-Length = [%lu] != [%lu]: Actual-Length vs Content-Length Mismatch.", cRoutine, ui32Offset, ui32ContentLength);
    HTTPFreeData(*ppcData);
    return -1;
  }
  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPReadDataIntoStream
 *
 ***********************************************************************
 */
int
HTTPReadDataIntoStream(SOCKET_CONTEXT *psockCTX, FILE *pFile, K_UINT32 ui32ContentLength, char *pcError)
{
  const char          cRoutine[] = "HTTPReadDataIntoStream()";
  char                cData[HTTP_IO_BUFSIZE],
                      cLocalError[ERRBUF_SIZE];
  int                 iNRead,
                      iZRead;
  K_UINT32            ui32Offset,
                      ui32ToRead;


  ui32Offset = iZRead = 0;

  ui32ToRead = ui32ContentLength;

  /*-
   *********************************************************************
   *
   * Read the data.
   *
   *********************************************************************
   */
  while (ui32ToRead > 0 && !iZRead)
  {
    iNRead = SocketRead(psockCTX, cData, HTTP_IO_BUFSIZE, cLocalError);
    switch (iNRead)
    {
    case 0:
      iZRead = 1;
      break;
    case -1:
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
      break;
    default:
      ui32Offset += fwrite(cData, 1, iNRead, pFile);
      if (ferror(pFile))
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: fwrite(): %s", cRoutine, strerror(errno));
        return -1;
      }
      ui32ToRead -= iNRead;
      break;
    }
  }
  if (ui32Offset != ui32ContentLength)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Actual-Length = [%lu] != [%lu]: Actual-Length vs Content-Length Mismatch.", cRoutine, ui32Offset, ui32ContentLength);
    return -1;
  }
  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPReadHeader
 *
 ***********************************************************************
 */
int
HTTPReadHeader(SOCKET_CONTEXT *psockCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError)
{
  const char          cRoutine[] = "HTTPReadHeader()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iHdrOk,
                      iIndex,
                      iNRead;

  for (iIndex = iHdrOk = 0; iIndex < iMaxResponseLength && !iHdrOk; iIndex++)
  {
    iNRead = SocketRead(psockCTX, &pcResponseHeader[iIndex], 1, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return -1;
      break;
    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: Read Count = [%d] != [1]: Invalid Count.", cRoutine, iNRead);
      break;
    }
  }
  if (!iHdrOk)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Unterminated header or size exceeds %d bytes.", cRoutine, iMaxResponseLength);
    return -1;
  }
  return iIndex;
}


/*-
 ***********************************************************************
 *
 * HTTPSetDynamicString
 *
 ***********************************************************************
 */
int
HTTPSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          cRoutine[] = "HTTPSetDynamicString()";
  char               *pcTempValue;

  if (*ppcValue == NULL || strlen(pcNewValue) > strlen(*ppcValue))
  {
    /*-
     *******************************************************************
     *
     * The caller is expected to free this memory.
     *
     *******************************************************************
     */
    pcTempValue = malloc(strlen(pcNewValue) + 1);
    if (pcTempValue == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
      return -1;
    }
    if (*ppcValue != NULL)
    {
      HTTPFreeData(*ppcValue);
    }
    *ppcValue = pcTempValue;
  }
  strcpy(*ppcValue, pcNewValue);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLDownloadLimit
 *
 ***********************************************************************
 */
void
HTTPSetURLDownloadLimit(HTTP_URL *ptURL, K_UINT32 ui32Limit)
{
  ptURL->ui32DownloadLimit = ui32Limit;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLHost
 *
 ***********************************************************************
 */
int
HTTPSetURLHost(HTTP_URL *ptURL, char *pcHost, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLHost()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  struct hostent     *pHostEntry;
#ifdef WIN32
  DWORD               dwStatus;
  WORD                wVersion;
  WSADATA             wsaData;
#endif


  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcHost, pcHost, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Lookup/Set the IP address.
   *
   *********************************************************************
   */
  if (ptURL->pcHost[0] == 0)
  {
    if (ptURL->iScheme != HTTP_SCHEME_FILE)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Hostname or IP address.", cRoutine);
      return -1;
    }
  }
  else
  {
#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif
    ptURL->ui32IP = inet_addr(ptURL->pcHost);
    if (ptURL->ui32IP == INADDR_NONE)
    {
#ifdef WIN32
      wVersion = (WORD)(1) | ((WORD)(1) << 8); /* MAKEWORD(1, 1) */
      dwStatus = WSAStartup(wVersion, &wsaData);
      if (dwStatus != 0)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: WSAStartup(): %u", cRoutine, dwStatus);
        return -1;
      }
#endif
      pHostEntry = gethostbyname(ptURL->pcHost);
      if (pHostEntry == NULL)
      {
/* FIXME Add hstrerror() for UNIX and WSAGetLastError() for Windows. */
        snprintf(pcError, ERRBUF_SIZE, "%s: DNS Lookup Failed.", cRoutine);
        return -1;
      }
      else
      {
        ptURL->ui32IP = ((struct in_addr *)(pHostEntry->h_addr))->s_addr;
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
 * HTTPSetURLMeth
 *
 ***********************************************************************
 */
int
HTTPSetURLMeth(HTTP_URL *ptURL, char *pcMeth, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLMeth()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcMeth, pcMeth, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLPass
 *
 ***********************************************************************
 */
int
HTTPSetURLPass(HTTP_URL *ptURL, char *pcPass, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLPass()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcPass, pcPass, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLPath
 *
 ***********************************************************************
 */
int
HTTPSetURLPath(HTTP_URL *ptURL, char *pcPath, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLPath()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcPath, pcPath, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLPort
 *
 ***********************************************************************
 */
int
HTTPSetURLPort(HTTP_URL *ptURL, char *pcPort, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLPort()";
  char                cLocalError[ERRBUF_SIZE],
                     *pc;
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcPort, pcPort, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check/Set the numeric port value.
   *
   *********************************************************************
   */
  if (ptURL->pcPort[0] && strlen(ptURL->pcPort) <= 5)
  {
    pc = ptURL->pcPort;
    while (*pc)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Port = [%s]: Value must contain all digits.", cRoutine, ptURL->pcPort);
        return -1;
      }
      pc++;
    }
    ptURL->ui16Port = atoi(ptURL->pcPort);
    if (ptURL->ui16Port < 1 || ptURL->ui16Port > 65535)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Port = [%s]: Value must be 1-65535.", cRoutine, ptURL->pcPort);
      return -1;
    }
  }
  else
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Port = [%s]: Value could not be converted.", cRoutine, ptURL->pcPort);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLQuery
 *
 ***********************************************************************
 */
int
HTTPSetURLQuery(HTTP_URL *ptURL, char *pcQuery, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLQuery()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcQuery, pcQuery, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLUser
 *
 ***********************************************************************
 */
int
HTTPSetURLUser(HTTP_URL *ptURL, char *pcUser, char *pcError)
{
  const char          cRoutine[] = "HTTPSetURLUser()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&ptURL->pcUser, pcUser, cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPSubmitRequest
 *
 ***********************************************************************
 */
int
HTTPSubmitRequest(HTTP_URL *ptURL, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          cRoutine[] = "HTTPSubmitRequest()";
  char                cLocalError[ERRBUF_SIZE],
                      cFileData[HTTP_IO_BUFSIZE],
                      cSockData[HTTP_IO_BUFSIZE],
                     *pcRequest;
  int                 iError,
                      iNRead,
                      iNSent,
                      iRequestLength,
                      iSocketType;
  K_UINT32            ui32Size;
  K_UINT64            ui64ContentLength;
  HTTP_MEMORY_LIST   *pMList;
  HTTP_STREAM_LIST   *pSList;
  SOCKET_CONTEXT     *psockCTX;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * If ptURL is not defined, abort.
   *
   *********************************************************************
   */
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Set the socket type based on the URL scheme.
   *
   *********************************************************************
   */
  switch (ptURL->iScheme)
  {
  case HTTP_SCHEME_HTTP:
    iSocketType = SOCKET_TYPE_REGULAR;
    break;
#ifdef USE_SSL
  case HTTP_SCHEME_HTTPS:
    iSocketType = SOCKET_TYPE_SSL;
    break;
#endif
  default:
    iSocketType = -1;
    snprintf(pcError, ERRBUF_SIZE, "%s: Scheme = [%d]: Unsupported Scheme.", cRoutine, ptURL->iScheme);
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
    for (pMList = (HTTP_MEMORY_LIST *)pInput; pMList != NULL; pMList = pMList->pNext)
    {
      ui64ContentLength += (K_UINT64) pMList->ui32Size;
    }
    break;
  case HTTP_STREAM_INPUT:
    for (pSList = (HTTP_STREAM_LIST *)pInput; pSList != NULL; pSList = pSList->pNext)
    {
      ui64ContentLength += (K_UINT64) pSList->ui32Size;
    }
    break;
  default:
    snprintf(pcError, ERRBUF_SIZE, "%s: Input Type = [%d]: Invalid Type.", cRoutine, iInputType);
    return -1;
    break;
  }

  if (((ui64ContentLength >> 32) & 0xffffff))
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Unsupported URL Scheme.", cRoutine);
    return -1;
  }
  else
  {
    ptURL->ui32ContentLength = (unsigned long) ui64ContentLength;
  }

  /*-
   *********************************************************************
   *
   * Build the Request Header.
   *
   *********************************************************************
   */
  pcRequest = HTTPBuildRequest(ptURL, cLocalError);
  if (pcRequest == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
#ifdef USE_SSL
  psockCTX = SocketConnect(ptURL->ui32IP, ptURL->ui16Port, iSocketType, (iSocketType == SOCKET_TYPE_SSL) ? ptURL->psSSLProperties->psslCTX : NULL, cLocalError);
#else
  psockCTX = SocketConnect(ptURL->ui32IP, ptURL->ui16Port, iSocketType, NULL, cLocalError);
#endif
  if (psockCTX == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return -1;
  }

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Verify the Peer's Common Name.
   *
   *********************************************************************
   */
  if (iSocketType == SOCKET_TYPE_SSL && ptURL->psSSLProperties->iVerifyPeerCert)
  {
    iError = SSLVerifyCN(psockCTX->pssl, ptURL->psSSLProperties->pcExpectedPeerCN, cLocalError);
    if (iError == -1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      SocketCleanup(psockCTX);
      return -1;
    }
  }
#endif

  /*-
   *********************************************************************
   *
   * Transmit the Request Header.
   *
   *********************************************************************
   */
  iNSent = SocketWrite(psockCTX, pcRequest, iRequestLength, cLocalError);
  if (iNSent != iRequestLength)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", cRoutine, iNSent, iRequestLength, cLocalError);
    HTTPFreeData(pcRequest);
    SocketCleanup(psockCTX);
    return -1;
  }
  HTTPFreeData(pcRequest);

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
    for (pMList = (HTTP_MEMORY_LIST *)pInput; pMList != NULL; pMList = pMList->pNext)
    {
      ui32Size = pMList->ui32Size;
      do
      {
        iNRead = (ui32Size > 0x7fffffff) ? 0x7fffffff : (int) ui32Size;
        iNSent = SocketWrite(psockCTX, pMList->pcData, iNRead, cLocalError);
        if (iNSent != iNRead)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", cRoutine, iNSent, iNRead, cLocalError);
          SocketCleanup(psockCTX);
          return -1;
        }
        ui32Size -= iNSent;
      } while (ui32Size > 0);
    }
    break;
  case HTTP_STREAM_INPUT:
    for (pSList = (HTTP_STREAM_LIST *)pInput; pSList != NULL; pSList = pSList->pNext)
    {
      do
      {
        iNRead = fread(cFileData, 1, HTTP_IO_BUFSIZE, pSList->pFile);
        if (ferror(pSList->pFile))
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: fread(): %s", cRoutine, strerror(errno));
          SocketCleanup(psockCTX);
          return -1;
        }
        iNSent = SocketWrite(psockCTX, cFileData, iNRead, cLocalError);
        if (iNSent != iNRead)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", cRoutine, iNSent, iNRead, cLocalError);
          SocketCleanup(psockCTX);
          return -1;
        }
      } while (!feof(pSList->pFile));
    }
    break;
  default:
    snprintf(pcError, ERRBUF_SIZE, "%s: Input Type = [%d]: Invalid Type.", cRoutine, iInputType);
    SocketCleanup(psockCTX);
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
  memset(cSockData, 0, HTTP_IO_BUFSIZE);

  iNRead = HTTPReadHeader(psockCTX, cSockData, HTTP_IO_BUFSIZE - 1, cLocalError);
  if (iNRead == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SocketCleanup(psockCTX);
    return -1;
  }
  cSockData[iNRead] = 0;

  /*-
   *********************************************************************
   *
   * Parse the Response Header. If successful, HTTP status will be set.
   *
   *********************************************************************
   */
  iError = HTTPParseHeader(cSockData, iNRead, psResponseHeader, cLocalError);
  if (iError != 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SocketCleanup(psockCTX);
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
    SocketCleanup(psockCTX);
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
  if (!psResponseHeader->iContentLengthFound)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: The server's response did not include a Content-Length.", cRoutine);
    SocketCleanup(psockCTX);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Check Content-Length against download limit (zero means no limit).
   *
   *********************************************************************
   */
  if (ptURL->ui32DownloadLimit != 0 && psResponseHeader->ui32ContentLength > ptURL->ui32DownloadLimit)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Content-Length = [%lu] > [%lu]: Length exceeds defined download limit.", cRoutine, psResponseHeader->ui32ContentLength, ptURL->ui32DownloadLimit);
    SocketCleanup(psockCTX);
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
    iError = HTTPReadDataIntoMemory(psockCTX, (char **) &pOutput, psResponseHeader->ui32ContentLength, cLocalError);
    break;
  case HTTP_STREAM_OUTPUT:
    iError = HTTPReadDataIntoStream(psockCTX, (FILE *) pOutput, psResponseHeader->ui32ContentLength, cLocalError);
    break;
  default:
    snprintf(pcError, ERRBUF_SIZE, "%s: Output Type = [%d]: Invalid Type.", cRoutine, iOutputType);
    SocketCleanup(psockCTX);
    return -1;
    break;
  }
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SocketCleanup(psockCTX);
    return -1;
  }

  SocketCleanup(psockCTX);

  return 0;
}


/*-
 ***********************************************************************
 *
 * HTTPUnEscape
 *
 ***********************************************************************
 */
char *
HTTPUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError)
{
  const char          cRoutine[] = "HTTPUnEscape()";
  char               *pc,
                     *pcUnEscaped;
  int                 i,
                      iHexDigit1,
                      iHexDigit2;

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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  pcUnEscaped[0] = 0;

  for (i = 0, pc = pcEscaped; *pc; i++)
  {
    if (*pc == '%')
    {
      iHexDigit1 = HTTPHexToInt(*(pc + 1));
      iHexDigit2 = HTTPHexToInt(*(pc + 2));
      if (iHexDigit1 == -1 || iHexDigit2 == -1)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Malformed Hex Value.", cRoutine);
        HTTPFreeData(pcUnEscaped);
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
