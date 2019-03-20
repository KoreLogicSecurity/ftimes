/*-
 ***********************************************************************
 *
 * $Id: url.c,v 1.17 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
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
  const char          acRoutine[] = "URLGetRequest()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acQuery[1024];
  char               *apcEscaped[4];
  int                 i;
  int                 iEscaped;
  int                 iError;
  HTTP_URL           *psURL;
  HTTP_RESPONSE_HDR   sResponseHeader;

  /*-
   *********************************************************************
   *
   * If the URL is not defined, abort.
   *
   *********************************************************************
   */
  psURL = psProperties->psGetURL;
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
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
  if (psURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, acLocalError) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  psURL->psSSLProperties = psProperties->psSSLProperties;
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
    if (!psURL->pcUser[0] || !psURL->pcPass[0])
    {
      if (!psProperties->acURLUsername[0] || !psProperties->acURLPassword[0])
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Missing Username and/or Password.", acRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(psURL, psProperties->acURLUsername, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(psURL, psProperties->acURLPassword, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
      }
    }
    psURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  apcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acBaseName, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acURLGetRequest, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }

  sprintf(acQuery, "VERSION=%s&CLIENTID=%s&REQUEST=%s",
           apcEscaped[0],
           apcEscaped[1],
           apcEscaped[2]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(apcEscaped[i]);
  }

  iError = HTTPSetURLQuery(psURL, acQuery, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(psURL, "GET", acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(psURL, HTTP_IGNORE_INPUT, NULL, HTTP_STREAM_OUTPUT, psProperties->pFileOut, &sResponseHeader, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    snprintf(pcError, MESSAGE_SIZE, "%s: Status = [%d], Reason = [%s]", acRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
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
  const char          acRoutine[] = "URLPingRequest()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acQuery[1024];
  char               *apcEscaped[5];
  int                 i;
  int                 iEscaped;
  int                 iError;
  HTTP_URL           *psURL;
  HTTP_RESPONSE_HDR   sResponseHeader;

  /*-
   *********************************************************************
   *
   * If the URL is not defined, abort.
   *
   *********************************************************************
   */
  psURL = psProperties->psPutURL;
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
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
  if (psURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, acLocalError) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  psURL->psSSLProperties = psProperties->psSSLProperties;
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
    if (!psURL->pcUser[0] || !psURL->pcPass[0])
    {
      if (!psProperties->acURLUsername[0] || !psProperties->acURLPassword[0])
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Missing Username and/or Password.", acRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(psURL, psProperties->acURLUsername, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(psURL, psProperties->acURLPassword, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
      }
    }
    psURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  apcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acBaseName, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acDataType, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->psFieldMask->pcMask, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }

  sprintf(acQuery, "VERSION=%s&CLIENTID=%s&DATATYPE=%s&FIELDMASK=%s",
           apcEscaped[0],
           apcEscaped[1],
           apcEscaped[2],
           apcEscaped[3]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(apcEscaped[i]);
  }

  iError = HTTPSetURLQuery(psURL, acQuery, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(psURL, "PING", acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(psURL, HTTP_IGNORE_INPUT, NULL, HTTP_IGNORE_OUTPUT, NULL, &sResponseHeader, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    snprintf(pcError, MESSAGE_SIZE, "%s: Status = [%d], Reason = [%s]", acRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
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
  const char          acRoutine[] = "URLPutRequest()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acQuery[1024];
  char                acOutFileHash[FTIMES_MAX_MD5_LENGTH];
  char               *apcEscaped[8];
  char               *apcFilenames[2];
  unsigned char       uacMD5[MD5_HASH_SIZE];
  int                 i;
  int                 n;
  int                 iEscaped;
  int                 iError;
  int                 iLimit;
  HTTP_URL           *psURL;
  HTTP_STREAM_LIST    sStreamList[2];
  HTTP_RESPONSE_HDR   sResponseHeader;

  /*-
   *********************************************************************
   *
   * Build the XMIT list.
   *
   *********************************************************************
   */
  apcFilenames[0] = psProperties->acLogFileName;
  apcFilenames[1] = psProperties->acOutFileName;
  for (i = 0, iLimit = 2; i < iLimit; i++)
  {
    sStreamList[i].pFile = fopen(apcFilenames[i], "rb");
    if (sStreamList[i].pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s]: %s", acRoutine, apcFilenames[i], strerror(errno));
      return ER;
    }
    if (fseek(sStreamList[i].pFile, 0, SEEK_END) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fseek(): File = [%s]: %s", acRoutine, apcFilenames[i], strerror(errno));
      return ER;
    }
    sStreamList[i].ui32Size = (unsigned) ftell(sStreamList[i].pFile);
    if (sStreamList[i].ui32Size == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: ftell(): File = [%s]: %s", acRoutine, apcFilenames[i], strerror(errno));
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
  if (MD5HashStream(sStreamList[1].pFile, uacMD5) != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Digest could not be computed.", acRoutine);
    return ER;
  }
  else
  {
    for (n = 0; n < MD5_HASH_SIZE; n++)
    {
      sprintf(&acOutFileHash[n * 2], "%02x", uacMD5[n]);
      acOutFileHash[FTIMES_MAX_MD5_LENGTH - 1] = 0;
    }

    for (n = 0; n < FTIMES_MAX_MD5_LENGTH - 1; n++)
    {
      if (acOutFileHash[n] != psProperties->acOutFileHash[n])
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Hash mismatch!: %s != %s", acRoutine, acOutFileHash, psProperties->acOutFileHash);
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
  psURL = psProperties->psPutURL;
  if (psURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined URL.", acRoutine);
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
 * if (psURL->iScheme == HTTP_SCHEME_HTTPS && SSLInitializeCTX(psProperties->psSSLProperties, acLocalError) == NULL)
 * {
 *   snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
 *   return ER;
 * }
 * psURL->psSSLProperties = psProperties->psSSLProperties;
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
    if (!psURL->pcUser[0] || !psURL->pcPass[0])
    {
      if (!psProperties->acURLUsername[0] || !psProperties->acURLPassword[0])
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Missing Username and/or Password.", acRoutine);
        return ER;
      }
      else
      {
        iError = HTTPSetURLUser(psURL, psProperties->acURLUsername, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
        iError = HTTPSetURLPass(psURL, psProperties->acURLPassword, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
        }
      }
    }
    psURL->iAuthType = HTTP_AUTH_TYPE_BASIC;
  }

  /*-
   *********************************************************************
   *
   * Set the query string.
   *
   *********************************************************************
   */
  iEscaped = 0;
  apcEscaped[iEscaped] = HTTPEscape(SupportGetMyVersion(), acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acBaseName, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acDataType, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->psFieldMask->pcMask, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acRunType, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acRunDateTime, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }
  apcEscaped[iEscaped] = HTTPEscape(psProperties->acOutFileHash, acLocalError);
  if (apcEscaped[iEscaped++] == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    for (i = 0; i < iEscaped; i++)
    {
      HTTPFreeData(apcEscaped[i]);
    }
    return ER;
  }

  sprintf(acQuery, "VERSION=%s&CLIENTID=%s&DATATYPE=%s&FIELDMASK=%s&RUNTYPE=%s&DATETIME=%s&LOGLENGTH=%u&OUTLENGTH=%u&MD5=%s",
           apcEscaped[0],
           apcEscaped[1],
           apcEscaped[2],
           apcEscaped[3],
           apcEscaped[4],
           apcEscaped[5],
           sStreamList[0].ui32Size,
           sStreamList[1].ui32Size,
           apcEscaped[6]
         );

  for (i = 0; i < iEscaped; i++)
  {
    HTTPFreeData(apcEscaped[i]);
  }

  iError = HTTPSetURLQuery(psURL, acQuery, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Set the request method.
   *
   *********************************************************************
   */
  iError = HTTPSetURLMeth(psURL, "PUT", acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Submit the request.
   *
   *********************************************************************
   */
  iError = HTTPSubmitRequest(psURL, HTTP_STREAM_INPUT, sStreamList, HTTP_IGNORE_OUTPUT, NULL, &sResponseHeader, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    snprintf(pcError, MESSAGE_SIZE, "%s: Status = [%d], Reason = [%s]", acRoutine, sResponseHeader.iStatusCode, sResponseHeader.acReasonPhrase);
    return ER;
  }

  return ER_OK;
}
