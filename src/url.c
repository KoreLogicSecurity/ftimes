/*-
 ***********************************************************************
 *
 * $Id: url.c,v 1.6 2003/08/13 21:39:49 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * URLGetRequest
 *
 ***********************************************************************
 */
int
URLGetRequest(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "URLGetRequest()";
  char                cLocalError[ERRBUF_SIZE],
                      cQuery[1024];
  char               *pcEscaped[4];
  int                 i,
                      iEscaped,
                      iError;
  HTTP_URL           *ptURL;
  HTTP_RESPONSE_HDR   sResponseHeader;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * If the URL is not defined, abort.
   *
   *********************************************************************
   */
  ptURL = psProperties->ptGetURL;
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return ER;
  }

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Initialize SSL Context. This need only be done once per URL.
   *
   *********************************************************************
   */
  if (ptURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, cLocalError) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  ptURL->psSSLProperties = psProperties->psSSLProperties;
#endif

  /*-
   *********************************************************************
   *
   * If AuthType is basic, require both a username and a password. If
   * a URL User and/or Pass is not defined, use the general Username
   * and Password. If either of those is not defined, abort.
   *
   *********************************************************************
   */
  if (psProperties->iURLAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    if (!ptURL->pcUser[0] || !ptURL->pcPass[0])
    {
      if (!psProperties->cURLUsername[0] || !psProperties->cURLPassword[0])
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Missing Username and/or Password.", cRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(ptURL, psProperties->cURLUsername, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(ptURL, psProperties->cURLPassword, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
      }
    }
    ptURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  pcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cBaseName, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cURLGetRequest, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }

  sprintf(cQuery, "VERSION=%s&CLIENTID=%s&REQUEST=%s",
           pcEscaped[0],
           pcEscaped[1],
           pcEscaped[2]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(pcEscaped[i]);
  }

  iError = HTTPSetURLQuery(ptURL, cQuery, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(ptURL, "GET", cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(ptURL, HTTP_IGNORE_INPUT, NULL, HTTP_STREAM_OUTPUT, psProperties->pFileOut, &sResponseHeader, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Check the HTTP response code.
   *
   *********************************************************************
   */
  if (sResponseHeader.iStatusCode < 200 || sResponseHeader.iStatusCode > 299)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Status = [%d], Reason = [%s]", cRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * URLPingRequest
 *
 ***********************************************************************
 */
int
URLPingRequest(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "URLPingRequest()";
  char                cLocalError[ERRBUF_SIZE],
                      cQuery[1024];
  char               *pcEscaped[5];
  int                 i,
                      iEscaped,
                      iError;
  HTTP_URL           *ptURL;
  HTTP_RESPONSE_HDR   sResponseHeader;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * If the URL is not defined, abort.
   *
   *********************************************************************
   */
  ptURL = psProperties->ptPutURL;
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return ER;
  }

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Initialize SSL Context. This need only be done once per URL.
   *
   *********************************************************************
   */
  if (ptURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, cLocalError) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  ptURL->psSSLProperties = psProperties->psSSLProperties;
#endif

  /*-
   *********************************************************************
   *
   * If AuthType is basic, require both a username and a password. If
   * a URL User and/or Pass is not defined, use the general Username
   * and Password. If either of those is not defined, abort.
   *
   *********************************************************************
   */
  if (psProperties->iURLAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    if (!ptURL->pcUser[0] || !ptURL->pcPass[0])
    {
      if (!psProperties->cURLUsername[0] || !psProperties->cURLPassword[0])
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Missing Username and/or Password.", cRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(ptURL, psProperties->cURLUsername, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(ptURL, psProperties->cURLPassword, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
      }
    }
    ptURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  pcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cBaseName, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cDataType, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cMaskString, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }

  sprintf(cQuery, "VERSION=%s&CLIENTID=%s&DATATYPE=%s&FIELDMASK=%s",
           pcEscaped[0],
           pcEscaped[1],
           pcEscaped[2],
           pcEscaped[3]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(pcEscaped[i]);
  }

  iError = HTTPSetURLQuery(ptURL, cQuery, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(ptURL, "PING", cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(ptURL, HTTP_IGNORE_INPUT, NULL, HTTP_IGNORE_OUTPUT, NULL, &sResponseHeader, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Check the HTTP response code.
   *
   *********************************************************************
   */
  if (sResponseHeader.iStatusCode < 200 || sResponseHeader.iStatusCode > 299)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Status = [%d], Reason = [%s]", cRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * URLPutRequest
 *
 ***********************************************************************
 */
int
URLPutRequest(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "URLPutRequest()";
  char                cLocalError[ERRBUF_SIZE],
                      cQuery[1024],
                      cOutFileHash[FTIMEX_MAX_MD5_LENGTH];
  char               *pcEscaped[8];
  unsigned char       ucMD5[MD5_HASH_SIZE];
  int                 i,
                      n,
                      iEscaped,
                      iError,
                      iLimit;
  HTTP_URL           *ptURL;
  HTTP_STREAM_LIST    sStreamList[2];
  char               *pcFilenames[2];
  HTTP_RESPONSE_HDR   sResponseHeader;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Build the XMIT list.
   *
   *********************************************************************
   */
  pcFilenames[0] = psProperties->cLogFileName;
  pcFilenames[1] = psProperties->cOutFileName;
  for (i = 0, iLimit = 2; i < iLimit; i++)
  {
    sStreamList[i].pFile = fopen(pcFilenames[i], "rb");
    if (sStreamList[i].pFile == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilenames[i], strerror(errno));
      return ER;
    }
    if (fseek(sStreamList[i].pFile, 0, SEEK_END) == ER)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilenames[i], strerror(errno));
      return ER;
    }
    sStreamList[i].ui32Size = (unsigned) ftell(sStreamList[i].pFile);
    if (sStreamList[i].ui32Size == (long) ER)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilenames[i], strerror(errno));
      return ER;
    }
    rewind(sStreamList[i].pFile);
    sStreamList[i].psNext = (i < iLimit - 1) ? &sStreamList[i + 1] : NULL;
  }

  /*-
   *********************************************************************
   *
   * Compute a digest for OutFile. Require a matches between it and the
   * the supplied digest.
   *
   *********************************************************************
   */
  if (MD5HashStream(sStreamList[1].pFile, ucMD5) != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Digest could not be computed.", cRoutine);
    return ER;
  }
  else
  {
    for (n = 0; n < MD5_HASH_SIZE; n++)
    {
      sprintf(&cOutFileHash[n * 2], "%02x", ucMD5[n]);
      cOutFileHash[FTIMEX_MAX_MD5_LENGTH - 1] = 0;
    }

    for (n = 0; n < FTIMEX_MAX_MD5_LENGTH - 1; n++)
    {
      if (cOutFileHash[n] != psProperties->cOutFileHash[n])
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Hash mismatch!: %s != %s", cRoutine, cOutFileHash, psProperties->cOutFileHash);
        return ER;
      }
    }
  }
  rewind(sStreamList[1].pFile);

  /*-
   *********************************************************************
   *
   * If the URL is not defined, abort.
   *
   *********************************************************************
   */
  ptURL = psProperties->ptPutURL;
  if (ptURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined URL.", cRoutine);
    return ER;
  }

#ifdef USE_SSL
/*-
 ***********************************************************************
 *
 * NOTE: SSL Context was initialized in URLPingRequest().
 *
 * *
 * *********************************************************************
 * *
 * * Initialize SSL Context. This need only be done once per URL.
 * *
 * *********************************************************************
 * *
 *
 * if (ptURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, cLocalError) == NULL)
 * {
 *   snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
 *   return ER;
 * }
 * ptURL->psSSLProperties = psProperties->psSSLProperties;
 *
 ***********************************************************************
 */
#endif

  /*-
   *********************************************************************
   *
   * If AuthType is basic, require both a username and a password. If
   * a URL User and/or Pass is not defined, use the general Username
   * and Password. If either of those is not defined, abort.
   *
   *********************************************************************
   */
  if (psProperties->iURLAuthType == HTTP_AUTH_TYPE_BASIC)
  {
    if (!ptURL->pcUser[0] || !ptURL->pcPass[0])
    {
      if (!psProperties->cURLUsername[0] || !psProperties->cURLPassword[0])
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Missing Username and/or Password.", cRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(ptURL, psProperties->cURLUsername, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(ptURL, psProperties->cURLPassword, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
          return ER;
        }
      }
    }
    ptURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  pcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cBaseName, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cDataType, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cMaskString, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cRunType, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cRunDateTime, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }
  pcEscaped[iEscaped] = HTTPEscape(psProperties->cOutFileHash, cLocalError);
  if (pcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(pcEscaped[i]);
    }
    return ER;
  }

  sprintf(cQuery, "VERSION=%s&CLIENTID=%s&DATATYPE=%s&FIELDMASK=%s&RUNTYPE=%s&DATETIME=%s&LOGLENGTH=%lu&OUTLENGTH=%lu&MD5=%s",
           pcEscaped[0],
           pcEscaped[1],
           pcEscaped[2],
           pcEscaped[3],
           pcEscaped[4],
           pcEscaped[5],
           sStreamList[0].ui32Size,
           sStreamList[1].ui32Size,
           pcEscaped[6]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(pcEscaped[i]);
  }

  iError = HTTPSetURLQuery(ptURL, cQuery, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(ptURL, "PUT", cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(ptURL, HTTP_STREAM_INPUT, sStreamList, HTTP_IGNORE_OUTPUT, NULL, &sResponseHeader, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    for (i = 0, iLimit = 2; i < iLimit; i++)
    {
      fclose(sStreamList[i].pFile);
    }
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Close the input files.
   *
   *********************************************************************
   */
  for (i = 0, iLimit = 2; i < iLimit; i++)
  {
    fclose(sStreamList[i].pFile);
  }

  /*-
   *********************************************************************
   *
   * Check the HTTP response code.
   *
   *********************************************************************
   */
  if (sResponseHeader.iStatusCode < 200 || sResponseHeader.iStatusCode > 299)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Status = [%d], Reason = [%s]", cRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
    return ER;
  }

  return ER_OK;
}
