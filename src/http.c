/*-
 ***********************************************************************
 *
 * $Id: http.c,v 1.12 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static unsigned char gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static unsigned char gaucBase16[] = "0123456789abcdef";

/*-
 ***********************************************************************
 *
 * HTTPBuildRequest
 *
 ***********************************************************************
 */
char *
HTTPBuildRequest(HTTP_URL *psURL, char *pcError)
{
  const char          acRoutine[] = "HTTPBuildRequest()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcBasicEncoding;
  char               *pcRequest;
  int                 iRequestLength;

  /*-
   *********************************************************************
   *
   * Make sure the URL and its members aren't NULL.
   *
   *********************************************************************
   */
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return NULL;
  }

  if (psURL->pcUser == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Username.", acRoutine);
    return NULL;
  }

  if (psURL->pcPass == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Password.", acRoutine);
    return NULL;
  }

  if (psURL->pcHost == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Host.", acRoutine);
    return NULL;
  }

  if (psURL->pcPort == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Port.", acRoutine);
    return NULL;
  }

  if (psURL->pcPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Path.", acRoutine);
    return NULL;
  }

  if (psURL->pcQuery == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Query.", acRoutine);
    return NULL;
  }

  if (psURL->pcMeth == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Method.", acRoutine);
    return NULL;
  }

  if (psURL->pcJobId == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL Job-Id.", acRoutine);
    return NULL;
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
  iRequestLength += strlen(psURL->pcMeth) + strlen(" ") + strlen(psURL->pcPath) + strlen("?") + strlen(psURL->pcQuery) + strlen(" HTTP/1.1\r\n");
  iRequestLength += strlen("Host: ") + strlen(psURL->pcHost) + strlen(":65535\r\n");
  iRequestLength += strlen("Content-Type: application/octet-stream\r\n");
  iRequestLength += strlen("Content-Length: 4294967295\r\n");
  iRequestLength += strlen("Job-Id: ") + strlen(psURL->pcJobId) + strlen("\r\n");
  iRequestLength += strlen("Authorization: Basic ") + (4 * (strlen(psURL->pcUser) + strlen(psURL->pcPass))) + strlen("\r\n");
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
  switch (psURL->iAuthType)
  {
  case HTTP_AUTH_TYPE_BASIC:
    pcBasicEncoding = HTTPEncodeBasic(psURL->pcUser, psURL->pcPass, acLocalError);
    if (pcBasicEncoding == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HTTPFreeData(pcRequest);
      return NULL;
    }
    snprintf(pcRequest, iRequestLength, "%s %s%s%s HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Content-Type: application/octet-stream\r\n"
      "Content-Length: %u\r\n"
      "Authorization: Basic %s\r\n"
      "%s%s%s"
      "\r\n",
      psURL->pcMeth,
      psURL->pcPath,
      (psURL->pcQuery[0]) ? "?" : "",
      (psURL->pcQuery[0]) ? psURL->pcQuery : "",
      psURL->pcHost,
      psURL->ui16Port,
      psURL->ui32ContentLength,
      pcBasicEncoding,
      (psURL->pcJobId[0]) ? "Job-Id: " : "",
      (psURL->pcJobId[0]) ? psURL->pcJobId : "",
      (psURL->pcJobId[0]) ? "\r\n" : ""
      );
    HTTPFreeData(pcBasicEncoding);
    break;
  default:
    snprintf(pcRequest, iRequestLength, "%s %s%s%s HTTP/1.1\r\n"
      "Host: %s:%d\r\n"
      "Content-Type: application/octet-stream\r\n"
      "Content-Length: %u\r\n"
      "%s%s%s"
      "\r\n",
      psURL->pcMeth,
      psURL->pcPath,
      (psURL->pcQuery[0]) ? "?" : "",
      (psURL->pcQuery[0]) ? psURL->pcQuery : "",
      psURL->pcHost,
      psURL->ui16Port,
      psURL->ui32ContentLength,
      (psURL->pcJobId[0]) ? "Job-Id: " : "",
      (psURL->pcJobId[0]) ? psURL->pcJobId : "",
      (psURL->pcJobId[0]) ? "\r\n" : ""
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
  const char          acRoutine[] = "HTTPEncodeBasic()";
  char               *pcBasicEncoding;
  char               *pcCredentials;
  int                 i;
  int                 iLength;
  int                 iPassLength;
  int                 iUserLength;
  int                 n;
  unsigned long       x;
  unsigned long       ulLeft;


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
  const char          acRoutine[] = "HTTPEscape()";
  char               *pcEscaped;
  char               *pc;
  int                 i;
  int                 iLength;

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
HTTPFreeURL(HTTP_URL *psURL)
{
  if (psURL != NULL)
  {
    if (psURL->pcUser != NULL)
    {
      free(psURL->pcUser);
    }
    if (psURL->pcPass != NULL)
    {
      free(psURL->pcPass);
    }
    if (psURL->pcHost != NULL)
    {
      free(psURL->pcHost);
    }
    if (psURL->pcPort != NULL)
    {
      free(psURL->pcPort);
    }
    if (psURL->pcPath != NULL)
    {
      free(psURL->pcPath);
    }
    if (psURL->pcQuery != NULL)
    {
      free(psURL->pcQuery);
    }
    if (psURL->pcMeth != NULL)
    {
      free(psURL->pcMeth);
    }
    if (psURL->pcJobId != NULL)
    {
      free(psURL->pcJobId);
    }
    free(psURL);
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
  const char          acRoutine[] = "HTTPNewURL()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  HTTP_URL           *psURL;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory with HTTPFreeURL().
   *
   *********************************************************************
   */
  psURL = malloc(sizeof(HTTP_URL));
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(psURL, 0, sizeof(HTTP_URL));

  /*-
   *********************************************************************
   *
   * Set the default request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(psURL, HTTP_DEFAULT_HTTP_METHOD, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HTTPFreeURL(psURL);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default job id.
   *
   *********************************************************************
   */
  iError = HTTPSetURLJobId(psURL, HTTP_DEFAULT_JOB_ID, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HTTPFreeURL(psURL);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default download limit.
   *
   *********************************************************************
   */
  HTTPSetURLDownloadLimit(psURL, HTTP_MAX_MEMORY_SIZE);

  return psURL;
}


/*-
 ***********************************************************************
 *
 * HTTPParseAddress
 *
 ***********************************************************************
 */
int
HTTPParseAddress(char *pcUserPassHostPort, HTTP_URL *psURL, char *pcError)
{
  const char          acRoutine[] = "HTTPParseAddress()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcHostPort;
  char               *pcT;
  char               *pcUserPass;
  int                 iLength;

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
    strncpy(pcUserPass, pcUserPassHostPort, iLength + 1);
    strncpy(pcHostPort, pcT, iLength + 1);
  }
  else
  {
    strncpy(pcHostPort, pcUserPassHostPort, iLength + 1);
  }

  if (HTTPParseUserPass(pcUserPass, psURL, acLocalError) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    HTTPFreeData(pcUserPass);
    HTTPFreeData(pcHostPort);
    return -1;
  }

  if (HTTPParseHostPort(pcHostPort, psURL, acLocalError) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
HTTPParseGRELine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HTTPParseGRELine()";
  char               *pc;
  char               *pcEnd;
  char               *pcFieldName;
  char               *pcFieldValue;
  char               *pcTempLine;
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
      HTTPFreeData(pcTempLine);
      return -1;
    }
    for (pc = pcFieldValue; *pc; pc++)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%s]: Value must contain all digits.", acRoutine, pcFieldValue);
        HTTPFreeData(pcTempLine);
        return -1;
      }
    }
    psResponseHeader->ui32ContentLength = strtoul(pcFieldValue, NULL, 10);
    if (errno == ERANGE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: strtoul(): Content-Length = [%s]: Value too large.", acRoutine, pcFieldValue);
      HTTPFreeData(pcTempLine);
      return -1;
    }
    psResponseHeader->iContentLengthFound = 1;
  }

  else if (strcasecmp(pcFieldName, FIELD_JobId) == 0)
  {
    if (psResponseHeader->iJobIdFound)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Job-Id = [%s]: Field already defined.", acRoutine, pcFieldValue);
      HTTPFreeData(pcTempLine);
      return -1;
    }
    if (iLength < 1 || iLength > HTTP_JOB_ID_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Job-Id = [%s]: Value has invalid length (%d).", acRoutine, pcFieldValue, iLength);
      HTTPFreeData(pcTempLine);
      return -1;
    }
    strncpy(psResponseHeader->acJobId, pcFieldValue, HTTP_JOB_ID_SIZE);
    psResponseHeader->iJobIdFound = 1;
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
HTTPParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HTTPParseHeader()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcE;
  char               *pcS;
  int                 iError;
  int                 iLength;

  iLength = iResponseHeaderLength;

  pcS = pcE = pcResponseHeader;

  memset(psResponseHeader, 0, sizeof(HTTP_RESPONSE_HDR));

  HTTP_TERMINATE_LINE(pcE);

  iError = HTTPParseStatusLine(pcS, psResponseHeader, acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return -1;
  }

  while (++pcE < &pcResponseHeader[iResponseHeaderLength])
  {
    pcS = pcE;
    HTTP_TERMINATE_LINE(pcE);
    iError = HTTPParseGRELine(pcS, psResponseHeader, acLocalError);
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
 * HTTPParseHostPort
 *
 ***********************************************************************
 */
int
HTTPParseHostPort(char *pcHostPort, HTTP_URL *psURL, char *pcError)
{
  const char          acRoutine[] = "HTTPParseHostPort()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcT;
  int                 iError;

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
    iError = HTTPSetURLPort(psURL, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member, and set its default value. */
  {
    switch (psURL->iScheme)
    {
    case HTTP_SCHEME_HTTPS:
      iError = HTTPSetURLPort(psURL, "443", acLocalError);
      break;
    default:
      iError = HTTPSetURLPort(psURL, "80", acLocalError);
      break;
    }
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }

  iError = HTTPSetURLHost(psURL, pcHostPort, acLocalError);
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
 * HTTPParsePathQuery
 *
 ***********************************************************************
 */
int
HTTPParsePathQuery(char *pcPathQuery, HTTP_URL *psURL, char *pcError)
{
  const char          acRoutine[] = "HTTPParsePathQuery()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcT;
  int                 iError;

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
    iError = HTTPSetURLQuery(psURL, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HTTPSetURLQuery(psURL, "", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  iError = HTTPSetURLPath(psURL, pcPathQuery, acLocalError);
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
 * HTTPParseStatusLine
 *
 ***********************************************************************
 */
int
HTTPParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HTTPParseStatusLine()";
  char               *pcE;
  char               *pcS;
  char               *pcT;

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

  strncpy(psResponseHeader->acReasonPhrase, pcS, HTTP_REASON_PHRASE_SIZE);
  psResponseHeader->acReasonPhrase[HTTP_REASON_PHRASE_SIZE] = 0;

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
  const char          acRoutine[] = "HTTPParseURL()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcS;
  char               *pcT;
  char               *pcTempURL;
  int                 iLength;
  HTTP_URL           *psURL;

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
  psURL = HTTPNewURL(acLocalError);
  if (psURL == NULL)
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
  pcTempURL = malloc(iLength + 1);
  if (pcTempURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    HTTPFreeURL(psURL);
    return NULL;
  }
  strncpy(pcTempURL, pcURL, iLength + 1);

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
    psURL->iScheme = HTTP_SCHEME_FILE;
    pcS += 7;
  }
  else if (strncasecmp("http://", pcS, 7) == 0)
  {
    psURL->iScheme = HTTP_SCHEME_HTTP;
    pcS += 7;
  }
  else if (strncasecmp("https://", pcS, 8) == 0)
  {
    psURL->iScheme = HTTP_SCHEME_HTTPS;
    pcS += 8;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Unknown/Unsupported URL Scheme.", acRoutine);
    HTTPFreeURL(psURL);
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
    if (HTTPParsePathQuery(pcT, psURL, acLocalError) == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HTTPFreeURL(psURL);
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
    if (HTTPParseAddress(pcS, psURL, acLocalError) == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      HTTPFreeURL(psURL);
      HTTPFreeData(pcTempURL);
      return NULL;
    }
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid URL, Absolute URL Required.", acRoutine);
    HTTPFreeURL(psURL);
    HTTPFreeData(pcTempURL);
    return NULL;
  }

  HTTPFreeData(pcTempURL);
  return psURL;
}


/*-
 ***********************************************************************
 *
 * HTTPParseUserPass
 *
 ***********************************************************************
 */
int
HTTPParseUserPass(char *pcUserPass, HTTP_URL *psURL, char *pcError)
{
  const char          acRoutine[] = "HTTPParseUserPass()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcT;
  int                 iError;

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
    iError = HTTPSetURLPass(psURL, pcT, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  else /* We still need to allocate some memory for this member. */
  {
    iError = HTTPSetURLPass(psURL, "", acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return -1;
    }
  }
  iError = HTTPSetURLUser(psURL, pcUserPass, acLocalError);
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
  if (psURL->pcPass[0] != 0 && psURL->pcUser[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Username.", acRoutine);
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
HTTPReadDataIntoMemory(SOCKET_CONTEXT *psSocketCTX, void **ppvData, K_UINT32 ui32ContentLength, char *pcError)
{
  const char          acRoutine[] = "HTTPReadDataIntoMemory()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char              **ppcData;
  int                 iNRead;
  int                 iZRead;
  int                 iToRead;
  K_UINT32            ui32Offset;

  ui32Offset = iZRead = 0;

  ppcData = (char **) ppvData;

  /*-
   *********************************************************************
   *
   * Check/Limit Content-Length.
   *
   *********************************************************************
   */
  if (ui32ContentLength > (K_UINT32) HTTP_MAX_MEMORY_SIZE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%u] > [%u]: Length exceeds internally defined limit.",
      acRoutine,
      ui32ContentLength,
      (K_UINT32) HTTP_MAX_MEMORY_SIZE
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
  while (iToRead > 0 && !iZRead)
  {
    iNRead = SocketRead(psSocketCTX, &(*ppcData)[ui32Offset], iToRead, acLocalError);
    switch (iNRead)
    {
    case 0:
      iZRead = 1;
      break;
    case -1:
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    snprintf(pcError, MESSAGE_SIZE, "%s: Actual-Length = [%u] != [%u]: Actual-Length vs Content-Length Mismatch.",
      acRoutine,
      ui32Offset,
      ui32ContentLength
      );
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
HTTPReadDataIntoStream(SOCKET_CONTEXT *psSocketCTX, FILE *pFile, K_UINT32 ui32ContentLength, char *pcError)
{
  const char          acRoutine[] = "HTTPReadDataIntoStream()";
  char                acData[HTTP_IO_BUFSIZE];
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iNRead;
  int                 iZRead;
  K_UINT32            ui32Offset;
  K_UINT32            ui32ToRead;

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
    iNRead = SocketRead(psSocketCTX, acData, HTTP_IO_BUFSIZE, acLocalError);
    switch (iNRead)
    {
    case 0:
      iZRead = 1;
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
  }
  if (ui32Offset != ui32ContentLength)
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
 * HTTPReadHeader
 *
 ***********************************************************************
 */
int
HTTPReadHeader(SOCKET_CONTEXT *psSocketCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError)
{
  const char          acRoutine[] = "HTTPReadHeader()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iHdrOk;
  int                 iIndex;
  int                 iNRead;

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
 * HTTPSetDynamicString
 *
 ***********************************************************************
 */
int
HTTPSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          acRoutine[] = "HTTPSetDynamicString()";
  char               *pcTempValue;
  int                 iLength;

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
 * HTTPSetURLDownloadLimit
 *
 ***********************************************************************
 */
void
HTTPSetURLDownloadLimit(HTTP_URL *psURL, K_UINT32 ui32Limit)
{
  psURL->ui32DownloadLimit = ui32Limit;
}


/*-
 ***********************************************************************
 *
 * HTTPSetURLHost
 *
 ***********************************************************************
 */
int
HTTPSetURLHost(HTTP_URL *psURL, char *pcHost, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLHost()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  struct hostent     *psHostEntry;
#ifdef WIN32
  DWORD               dwStatus;
  WORD                wVersion;
  WSADATA             wsaData;
#endif

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcHost, pcHost, acLocalError);
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
  if (psURL->pcHost[0] == 0)
  {
    if (psURL->iScheme != HTTP_SCHEME_FILE)
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
    psURL->ui32IP = inet_addr(psURL->pcHost);
    if (psURL->ui32IP == INADDR_NONE)
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
      psHostEntry = gethostbyname(psURL->pcHost);
      if (psHostEntry == NULL)
      {
/* FIXME Add hstrerror() for UNIX and WSAGetLastError() for Windows. */
        snprintf(pcError, MESSAGE_SIZE, "%s: gethostbyname(): DNS Lookup Failed.", acRoutine);
        return -1;
      }
      else
      {
        psURL->ui32IP = ((struct in_addr *)(psHostEntry->h_addr))->s_addr;
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
 * HTTPSetURLJobId
 *
 ***********************************************************************
 */
int
HTTPSetURLJobId(HTTP_URL *psURL, char *pcJobId, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLJobId()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcJobId, pcJobId, acLocalError);
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
 * HTTPSetURLMeth
 *
 ***********************************************************************
 */
int
HTTPSetURLMeth(HTTP_URL *psURL, char *pcMeth, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLMeth()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcMeth, pcMeth, acLocalError);
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
 * HTTPSetURLPass
 *
 ***********************************************************************
 */
int
HTTPSetURLPass(HTTP_URL *psURL, char *pcPass, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLPass()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcPass, pcPass, acLocalError);
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
 * HTTPSetURLPath
 *
 ***********************************************************************
 */
int
HTTPSetURLPath(HTTP_URL *psURL, char *pcPath, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLPath()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcPath, pcPath, acLocalError);
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
 * HTTPSetURLPort
 *
 ***********************************************************************
 */
int
HTTPSetURLPort(HTTP_URL *psURL, char *pcPort, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLPort()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pc;
  int                 iError;
  int                 iPort;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcPort, pcPort, acLocalError);
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
  if (psURL->pcPort[0] && strlen(psURL->pcPort) <= 5)
  {
    pc = psURL->pcPort;
    while (*pc)
    {
      if (!isdigit((int) *pc))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must contain all digits.", acRoutine, psURL->pcPort);
        return -1;
      }
      pc++;
    }
    iPort = atoi(psURL->pcPort);
    if (iPort < 1 || iPort > 65535)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value must be 1-65535.", acRoutine, psURL->pcPort);
      return -1;
    }
    psURL->ui16Port = (K_UINT16) iPort;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Port = [%s]: Value could not be converted.", acRoutine, psURL->pcPort);
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
HTTPSetURLQuery(HTTP_URL *psURL, char *pcQuery, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLQuery()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcQuery, pcQuery, acLocalError);
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
 * HTTPSetURLUser
 *
 ***********************************************************************
 */
int
HTTPSetURLUser(HTTP_URL *psURL, char *pcUser, char *pcError)
{
  const char          acRoutine[] = "HTTPSetURLUser()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  iError = HTTPSetDynamicString(&psURL->pcUser, pcUser, acLocalError);
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
 * HTTPSubmitRequest
 *
 ***********************************************************************
 */
int
HTTPSubmitRequest(HTTP_URL *psURL, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError)
{
  const char          acRoutine[] = "HTTPSubmitRequest()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acFileData[HTTP_IO_BUFSIZE];
  char                acSockData[HTTP_IO_BUFSIZE];
  char               *pcRequest;
  int                 iError;
  int                 iNRead;
  int                 iNSent;
  int                 iRequestLength;
  int                 iSocketType;
  K_UINT32            ui32Size;
  K_UINT64            ui64ContentLength;
  HTTP_MEMORY_LIST   *pMList;
  HTTP_STREAM_LIST   *pSList;
  SOCKET_CONTEXT     *psSocketCTX;

  /*-
   *********************************************************************
   *
   * If psURL is not defined, abort.
   *
   *********************************************************************
   */
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
    return -1;
  }

  /*-
   *********************************************************************
   *
   * Set the socket type based on the URL scheme.
   *
   *********************************************************************
   */
  switch (psURL->iScheme)
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
    snprintf(pcError, MESSAGE_SIZE, "%s: Scheme = [%d]: Unsupported Scheme.", acRoutine, psURL->iScheme);
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
      ui64ContentLength += (K_UINT64) pMList->ui32Size;
    }
    break;
  case HTTP_STREAM_INPUT:
    for (pSList = (HTTP_STREAM_LIST *)pInput; pSList != NULL; pSList = pSList->psNext)
    {
      ui64ContentLength += (K_UINT64) pSList->ui32Size;
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
    psURL->ui32ContentLength = (unsigned long) ui64ContentLength;
  }

  /*-
   *********************************************************************
   *
   * Build the Request Header.
   *
   *********************************************************************
   */
  pcRequest = HTTPBuildRequest(psURL, acLocalError);
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
#ifdef USE_SSL
  psSocketCTX = SocketConnect(psURL->ui32IP, psURL->ui16Port, iSocketType, (iSocketType == SOCKET_TYPE_SSL) ? psURL->psSSLProperties->psslCTX : NULL, acLocalError);
#else
  psSocketCTX = SocketConnect(psURL->ui32IP, psURL->ui16Port, iSocketType, NULL, acLocalError);
#endif
  if (psSocketCTX == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
  if (iSocketType == SOCKET_TYPE_SSL && psURL->psSSLProperties->iVerifyPeerCert)
  {
    iError = SSLVerifyCN(psSocketCTX->pssl, psURL->psSSLProperties->pcExpectedPeerCN, acLocalError);
    if (iError == -1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      SocketCleanup(psSocketCTX);
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
  iNSent = SocketWrite(psSocketCTX, pcRequest, iRequestLength, acLocalError);
  if (iNSent != iRequestLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Sent = [%d] != [%d]: Transmission Error: %s", acRoutine, iNSent, iRequestLength, acLocalError);
    HTTPFreeData(pcRequest);
    SocketCleanup(psSocketCTX);
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

  iNRead = HTTPReadHeader(psSocketCTX, acSockData, HTTP_IO_BUFSIZE - 1, acLocalError);
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
  iError = HTTPParseHeader(acSockData, iNRead, psResponseHeader, acLocalError);
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
  if (!psResponseHeader->iContentLengthFound)
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
  if (psURL->ui32DownloadLimit != 0 && psResponseHeader->ui32ContentLength > psURL->ui32DownloadLimit)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Content-Length = [%u] > [%u]: Length exceeds user defined limit.",
      acRoutine,
      psResponseHeader->ui32ContentLength,
      psURL->ui32DownloadLimit
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
    iError = HTTPReadDataIntoMemory(psSocketCTX, (void **) &pOutput, psResponseHeader->ui32ContentLength, acLocalError);
    break;
  case HTTP_STREAM_OUTPUT:
    iError = HTTPReadDataIntoStream(psSocketCTX, (FILE *) pOutput, psResponseHeader->ui32ContentLength, acLocalError);
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
 * HTTPUnEscape
 *
 ***********************************************************************
 */
char *
HTTPUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError)
{
  const char          acRoutine[] = "HTTPUnEscape()";
  char               *pc;
  char               *pcUnEscaped;
  int                 i;
  int                 iHexDigit1;
  int                 iHexDigit2;

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
      iHexDigit1 = HTTPHexToInt(*(pc + 1));
      iHexDigit2 = HTTPHexToInt(*(pc + 2));
      if (iHexDigit1 == -1 || iHexDigit2 == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Malformed Hex Value.", acRoutine);
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
