/*
 ***********************************************************************
 *
 * $Id: ssl.c,v 1.1.1.1 2002/01/18 03:17:46 mavrik Exp $
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
 * SSLBoot
 *
 ***********************************************************************
 */
void
SSLBoot(void)
{
  unsigned char       ucSeed[SSL_SEED_LENGTH];

  /*-
   *********************************************************************
   *
   * Initialize SSL library, error strings, and random seed.
   *
   *********************************************************************
   */
  SSL_library_init();
  SSL_load_error_strings();
  RAND_seed(SSLGenerateSeed(ucSeed, SSL_SEED_LENGTH), SSL_SEED_LENGTH);
}


/*-
 ***********************************************************************
 *
 * SSLConnect
 *
 ***********************************************************************
 */
SSL *
SSLConnect(int iSocket, SSL_CTX *psslCTX, char *pcError)
{
  const char          cRoutine[] = "SSLConnect()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  SSL                 *pssl;

  pssl = SSL_new(psslCTX);
  if (pssl == NULL)
  {
    ERR_error_string(ERR_get_error(), cLocalError);
    snprintf(pcError, ERRBUF_SIZE, "%s: SSL_new(): %s", cRoutine, cLocalError);
    return NULL;
  }

  iError = SSL_set_fd(pssl, iSocket);
  if (iError == 0)
  {
    ERR_error_string(ERR_get_error(), cLocalError);
    snprintf(pcError, ERRBUF_SIZE, "%s: SSL_connect(): %s", cRoutine, cLocalError);
    SSL_free(pssl);
    return NULL;
  }

  iError = SSL_connect(pssl);
  if (iError <= 0)
  {
    ERR_error_string(ERR_get_error(), cLocalError);
    snprintf(pcError, ERRBUF_SIZE, "%s: SSL_connect(): %s", cRoutine, cLocalError);
    SSL_free(pssl);
    return NULL;
  }

  return pssl;
}


/*-
 ***********************************************************************
 *
 * SSLFreeProperties
 *
 ***********************************************************************
 */
void
SSLFreeProperties(SSL_PROPERTIES *psSSLProperties)
{
  if (psSSLProperties != NULL)
  {
    if (psSSLProperties->pcPublicCertFile != NULL)
    {
      free(psSSLProperties->pcPublicCertFile);
    }
    if (psSSLProperties->pcPrivateKeyFile != NULL)
    {
      free(psSSLProperties->pcPrivateKeyFile);
    }
    if (psSSLProperties->pcPassPhrase != NULL)
    {
      free(psSSLProperties->pcPassPhrase);
    }
    if (psSSLProperties->pcBundledCAsFile != NULL)
    {
      free(psSSLProperties->pcBundledCAsFile);
    }
    if (psSSLProperties->pcExpectedPeerCN != NULL)
    {
      free(psSSLProperties->pcExpectedPeerCN);
    }
    if (psSSLProperties->psslCTX != NULL)
    {
      SSL_CTX_free(psSSLProperties->psslCTX);
    }
    free(psSSLProperties);
  }
}


/*-
 ***********************************************************************
 *
 * SSLGenerateSeed
 *
 ***********************************************************************
 */
unsigned char *
SSLGenerateSeed(unsigned char *ucSeed, unsigned long iLength)
{
  unsigned long       i,
                      j,
                      ulLRS32b;

#ifdef WIN32
  ulLRS32b = (unsigned long) GetTickCount() ^ (unsigned long) time(NULL);
#else
  ulLRS32b = (((unsigned long) getpid()) << 16) ^ (unsigned long) time(NULL);
#endif
  for (i = 0; i < iLength; i++)
  {
    for (j = 0, ucSeed[i] = 0; j < 8; j++)
    {
      ucSeed[i] |= (ulLRS32b & 1) << j;
      ulLRS32b = ((((ulLRS32b >> 7) ^ (ulLRS32b >> 6) ^ (ulLRS32b >> 2) ^ (ulLRS32b >> 0)) & 1) << 31) | (ulLRS32b >> 1);
    }
  }
  return ucSeed;
}


/*-
 ***********************************************************************
 *
 * SSLInitializeCTX
 *
 ***********************************************************************
 */
SSL_CTX *
SSLInitializeCTX(SSL_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "SSLInitializeCTX()";
  char                cLocalError[ERRBUF_SIZE];

  /*-
   *********************************************************************
   *
   * Initialize SSL context.
   *
   *********************************************************************
   */
  psProperties->psslCTX = SSL_CTX_new(SSLv3_client_method());
  if (psProperties->psslCTX == NULL)
  {
    ERR_error_string(ERR_get_error(), cLocalError);
    snprintf(pcError, ERRBUF_SIZE, "%s: SSL_CTX_new(): %s", cRoutine, cLocalError);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Setup SSL certificate verification. Load the bundled certificate
   * authorities file. A common name (CN) and a positive chain length
   * must be specified to activate PEER verification. If you want to
   * override the default handler, write a routine and reference it in
   * the 3rd argument to SSL_CTX_set_verify().
   *
   *********************************************************************
   */
  if (psProperties->iVerifyPeerCert)
  {
    if(psProperties->pcExpectedPeerCN == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Expected Peer Common Name.", cRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if(psProperties->pcBundledCAsFile == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Bundled Certificate Authorities File.", cRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if(psProperties->iMaxChainLength < 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: MaxChainLength must be greater than one.", cRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }

    SSL_CTX_set_verify_depth(psProperties->psslCTX, psProperties->iMaxChainLength);

    SSL_CTX_set_verify(psProperties->psslCTX, SSL_VERIFY_PEER, NULL);

    if (!SSL_CTX_load_verify_locations(psProperties->psslCTX, psProperties->pcBundledCAsFile, NULL))
    {
      ERR_error_string(ERR_get_error(), cLocalError);
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_CTX_load_verify_locations(): %s", cRoutine, cLocalError);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
  }
  else
  {
    SSL_CTX_set_verify(psProperties->psslCTX, SSL_VERIFY_NONE, NULL);
  }

  /*-
   *********************************************************************
   *
   * Set up a password handler, if needed.
   *
   *********************************************************************
   */
  if (psProperties->pcPassPhrase != NULL && psProperties->pcPassPhrase[0])
  {
    SSL_CTX_set_default_passwd_cb(psProperties->psslCTX, SSLPassPhraseHandler);
    SSL_CTX_set_default_passwd_cb_userdata(psProperties->psslCTX, (void *) psProperties->pcPassPhrase);
  }

  /*-
   *********************************************************************
   *
   * Load key file and certificate file, if needed.
   *
   *********************************************************************
   */
  if (psProperties->iUseCertificate)
  {
    if (psProperties->pcPublicCertFile == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Public Certificate File.", cRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if (psProperties->pcPrivateKeyFile == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Undefined Private Key File.", cRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }

    if (!SSL_CTX_use_certificate_file(psProperties->psslCTX, psProperties->pcPublicCertFile, SSL_FILETYPE_PEM))
    {
      ERR_error_string(ERR_get_error(), cLocalError);
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_CTX_use_certificate_file(): %s", cRoutine, cLocalError);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if (!SSL_CTX_use_PrivateKey_file(psProperties->psslCTX, psProperties->pcPrivateKeyFile, SSL_FILETYPE_PEM))
    {
      ERR_error_string(ERR_get_error(), cLocalError);
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_CTX_use_PrivateKey_file(): %s", cRoutine, cLocalError);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
  }

  return psProperties->psslCTX;
}


/*-
 ***********************************************************************
 *
 * SSLNewProperties
 *
 ***********************************************************************
 */
SSL_PROPERTIES *
SSLNewProperties(char *pcError)
{
  const char          cRoutine[] = "SSLNewProperties()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  SSL_PROPERTIES     *psSSLProperties;

  psSSLProperties = (SSL_PROPERTIES *) malloc(sizeof(SSL_PROPERTIES));
  if (psSSLProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  memset(psSSLProperties, 0, sizeof(SSL_PROPERTIES));

  iError = SSLSetPublicCertFile(psSSLProperties, "", cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetPrivateKeyFile(psSSLProperties, "", cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetPassPhrase(psSSLProperties, "", cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetBundledCAsFile(psSSLProperties, "", cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetExpectedPeerCN(psSSLProperties, "", cLocalError);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }

  psSSLProperties->iMaxChainLength = SSL_DEFAULT_CHAIN_LENGTH;

  return psSSLProperties;
}


/*-
 ***********************************************************************
 *
 * SSLPassPhraseHandler
 *
 ***********************************************************************
 */
int
SSLPassPhraseHandler(char *pcPassPhrase, int iSize, int iRWFlag, void *pUserData)
{
  int                 iLength;

  iLength = strlen((char *) pUserData);
  if (iLength < iSize)
  {
    strncpy(pcPassPhrase, (char *) pUserData, iLength);
    return iLength;
  }
  else
  {
    pcPassPhrase[0] = 0;
    return 0;
  }
}


/*-
 ***********************************************************************
 *
 * SSLRead
 *
 ***********************************************************************
 */
int
SSLRead(SSL *ssl, char *pcData, int iLength, char *pcError)
{
  const char          cRoutine[] = "SSLRead()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iDone,
                      iNRead,
                      iRRetries;

  iDone = iRRetries = 0;

  iNRead = SSL_read(ssl, pcData, iLength);

  while (iDone == 0)
  {
    iDone = 1;
    switch (SSL_get_error(ssl, iNRead))
    {
    case SSL_ERROR_NONE:
      return iNRead;
      break;

    case SSL_ERROR_SSL:
      ERR_error_string(ERR_get_error(), cLocalError);
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_SSL: %s", cRoutine, cLocalError);
      return -1;
      break;

    case SSL_ERROR_WANT_READ:
      if (++iRRetries < SSL_RETRY_LIMIT)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_READ", cRoutine);
        iNRead = SSL_read(ssl, pcData, iLength);
        iDone = 0;
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_READ: Retry limit reached!", cRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_WANT_WRITE:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_WRITE", cRoutine);
      return -1;
      break;

    case SSL_ERROR_WANT_X509_LOOKUP:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_X509_LOOKUP", cRoutine);
      return -1;
      break;

    case SSL_ERROR_SYSCALL:
      if (ERR_peek_error())
      {
        ERR_error_string(ERR_get_error(), cLocalError);
      }
      if (iNRead == -1)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_SYSCALL: Underlying I/O error: %s", cRoutine, strerror(errno));
        return -1;
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_SYSCALL: Unexpected EOF.", cRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_ZERO_RETURN:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): SSL_ERROR_ZERO_RETURN: The SSL connection has been closed.", cRoutine);
      return 0;
      break;

    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_read(): Undefined error.", cRoutine);
      return -1;
      break;
    }
  }
  return -1;
}


/*-
 ***********************************************************************
 *
 * SSLSessionCleanup
 *
 ***********************************************************************
 */
void
SSLSessionCleanup(SSL *ssl)
{
  if (ssl != NULL)
  {
    SSL_shutdown(ssl);
    SSL_free(ssl);
  }
}


/*-
 ***********************************************************************
 *
 * SSLSetBundledCAsFile
 *
 ***********************************************************************
 */
int
SSLSetBundledCAsFile(SSL_PROPERTIES *psProperties, char *pcBundledCAsFile, char *pcError)
{
  const char          cRoutine[] = "SSLSetBundledCAsFile()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSLProperties.", cRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcBundledCAsFile, pcBundledCAsFile, cLocalError);
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
 * SSLSetDynamicString
 *
 ***********************************************************************
 */
int
SSLSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          cRoutine[] = "SSLSetDynamicString()";
  char               *pcTempValue;

  if (*ppcValue == NULL || strlen(pcNewValue) > strlen(*ppcValue))
  {
    /*-
     *******************************************************************
     *
     * The caller may free this memory with SSLFreeProperties().
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
      free(*ppcValue);
    }
    *ppcValue = pcTempValue;
  }
  strcpy(*ppcValue, pcNewValue);

  return 0;
}


/*-
 ***********************************************************************
 *
 * SSLSetExpectedPeerCN
 *
 ***********************************************************************
 */
int
SSLSetExpectedPeerCN(SSL_PROPERTIES *psProperties, char *pcExpectedPeerCN, char *pcError)
{
  const char          cRoutine[] = "SSLSetExpectedPeerCN()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSLProperties.", cRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcExpectedPeerCN, pcExpectedPeerCN, cLocalError);
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
 * SSLSetPassPhrase
 *
 ***********************************************************************
 */
int
SSLSetPassPhrase(SSL_PROPERTIES *psProperties, char *pcPassPhrase, char *pcError)
{
  const char          cRoutine[] = "SSLSetPassPhrase()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSLProperties.", cRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPassPhrase, pcPassPhrase, cLocalError);
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
 * SSLSetPrivateKeyFile
 *
 ***********************************************************************
 */
int
SSLSetPrivateKeyFile(SSL_PROPERTIES *psProperties, char *pcPrivateKeyFile, char *pcError)
{
  const char          cRoutine[] = "SSLSetPrivateKeyFile()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSLProperties.", cRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPrivateKeyFile, pcPrivateKeyFile, cLocalError);
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
 * SSLSetPublicCertFile
 *
 ***********************************************************************
 */
int
SSLSetPublicCertFile(SSL_PROPERTIES *psProperties, char *pcPublicCertFile, char *pcError)
{
  const char          cRoutine[] = "SSLSetPublicCertFile()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Undefined SSLProperties.", cRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPublicCertFile, pcPublicCertFile, cLocalError);
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
 * SSLVerifyCN
 *
 ***********************************************************************
 */
int
SSLVerifyCN(SSL *ssl, char *pcCN, char *pcError)
{
  const char          cRoutine[] = "SSLVerifyCN()";
  char                cPeerCN[SSL_MAX_COMMON_NAME_LENGTH];
  X509               *x509Cert;

  if (SSL_get_verify_result(ssl) != X509_V_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Invalid Certificate.", cRoutine);
    return -1;
  }

  x509Cert = SSL_get_peer_certificate(ssl);
  if (x509Cert == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing Certificate.", cRoutine);
    return -1;
  }

  X509_NAME_get_text_by_NID(X509_get_subject_name(x509Cert), NID_commonName, cPeerCN, SSL_MAX_COMMON_NAME_LENGTH);

  X509_free(x509Cert);

  if (strcmp(cPeerCN, pcCN) != 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: CN = [%s] != [%s]: Common Name Mismatch.", cRoutine, cPeerCN, pcCN);
    return -1;
  }

  return 0;
}


/*-
 ***********************************************************************
 *
 * SSLWrite
 *
 ***********************************************************************
 */
int
SSLWrite(SSL *ssl, char *pcData, int iLength, char *pcError)
{
  const char          cRoutine[] = "SSLWrite()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iNSent,
                      iOffset,
                      iToSend,
                      iWRetries;

  iToSend = iLength;

  iOffset = iWRetries = 0;

  do
  {
    iNSent = SSL_write(ssl, &pcData[iOffset], iToSend);

    switch (SSL_get_error(ssl, iNSent))
    {
    case SSL_ERROR_NONE:
      iToSend -= iNSent;
      iOffset += iNSent;
      break;

    case SSL_ERROR_SSL:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_SSL", cRoutine);
      return -1;
      break;

    case SSL_ERROR_WANT_READ:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_READ", cRoutine);
      return -1;
      break;

    case SSL_ERROR_WANT_WRITE:
      /*-
       *****************************************************************
       *
       * Resend the same buffer (openssl knows what to do). Give up if
       * we reach SSL_RETRY_LIMIT.
       *
       *****************************************************************
       */
      if (++iWRetries >= SSL_RETRY_LIMIT)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_WRITE: Retry limit reached!", cRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_WANT_X509_LOOKUP:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_X509_LOOKUP", cRoutine);
      return -1;
      break;

    case SSL_ERROR_SYSCALL:
      if (ERR_peek_error())
      {
        ERR_error_string(ERR_get_error(), cLocalError);
      }
      if (iNSent == -1)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_SYSCALL: Underlying I/O error: %s", cRoutine, strerror(errno));
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_SYSCALL: Unexpected EOF.", cRoutine);
      }
      return -1;
      break;

    case SSL_ERROR_ZERO_RETURN:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): SSL_ERROR_ZERO_RETURN: The SSL connection has been closed.", cRoutine);
      return -1;
      break;

    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: SSL_write(): Undefined error.", cRoutine);
      return -1;
      break;
    }
  } while (iToSend > 0);

  return iOffset;
}
