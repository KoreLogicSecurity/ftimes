/*-
 ***********************************************************************
 *
 * $Id: ssl.c,v 1.10 2007/02/23 00:22:35 mavrik Exp $
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
 * SSLBoot
 *
 ***********************************************************************
 */
void
SSLBoot(void)
{
  unsigned char       aucSeed[SSL_SEED_LENGTH];

  /*-
   *********************************************************************
   *
   * Initialize SSL library, error strings, and random seed.
   *
   *********************************************************************
   */
  SSL_library_init();
  SSL_load_error_strings();
  RAND_seed(SSLGenerateSeed(aucSeed, SSL_SEED_LENGTH), SSL_SEED_LENGTH);
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
  const char          acRoutine[] = "SSLConnect()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  SSL                 *pssl;

  pssl = SSL_new(psslCTX);
  if (pssl == NULL)
  {
    ERR_error_string(ERR_get_error(), acLocalError);
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_new(): %s", acRoutine, acLocalError);
    return NULL;
  }

  iError = SSL_set_fd(pssl, iSocket);
  if (iError == 0)
  {
    ERR_error_string(ERR_get_error(), acLocalError);
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_set_fd(): %s", acRoutine, acLocalError);
    SSL_free(pssl);
    return NULL;
  }

  iError = SSL_connect(pssl);
  if (iError <= 0)
  {
    ERR_error_string(ERR_get_error(), acLocalError);
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_connect(): %s", acRoutine, acLocalError);
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
SSLGenerateSeed(unsigned char *pucSeed, unsigned long iLength)
{
  unsigned long       i;
  unsigned long       j;
  unsigned long       ulLRS32b;

#ifdef WIN32
  ulLRS32b = (unsigned long) GetTickCount() ^ (unsigned long) time(NULL);
#else
  ulLRS32b = (((unsigned long) getpid()) << 16) ^ (unsigned long) time(NULL);
#endif
  for (i = 0; i < iLength; i++)
  {
    for (j = 0, pucSeed[i] = 0; j < 8; j++)
    {
      pucSeed[i] |= (ulLRS32b & 1) << j;
      ulLRS32b = ((((ulLRS32b >> 7) ^ (ulLRS32b >> 6) ^ (ulLRS32b >> 2) ^ (ulLRS32b >> 0)) & 1) << 31) | (ulLRS32b >> 1);
    }
  }
  return pucSeed;
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
  const char          acRoutine[] = "SSLInitializeCTX()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };

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
    ERR_error_string(ERR_get_error(), acLocalError);
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_CTX_new(): %s", acRoutine, acLocalError);
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
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Expected Peer Common Name.", acRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if(psProperties->pcBundledCAsFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Bundled Certificate Authorities File.", acRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if(psProperties->iMaxChainLength < 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: MaxChainLength must be greater than one.", acRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }

    SSL_CTX_set_verify_depth(psProperties->psslCTX, psProperties->iMaxChainLength);

    SSL_CTX_set_verify(psProperties->psslCTX, SSL_VERIFY_PEER, NULL);

    if (!SSL_CTX_load_verify_locations(psProperties->psslCTX, psProperties->pcBundledCAsFile, NULL))
    {
      ERR_error_string(ERR_get_error(), acLocalError);
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_CTX_load_verify_locations(): %s", acRoutine, acLocalError);
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
  if (SSL_POOL_SEED && SSL_POOL_TAPS > 1)
  {
    static unsigned char aucPool[SSL_POOL_SIZE];
    static unsigned char aucTaps[SSL_POOL_TAPS];
    SSL_NEW_POOL(aucPool, SSL_POOL_SIZE, SSL_POOL_SEED);
    SSL_TAP_POOL(aucTaps, aucPool);
    SSL_CTX_set_default_passwd_cb(psProperties->psslCTX, SSLPassPhraseHandler);
    SSL_CTX_set_default_passwd_cb_userdata(psProperties->psslCTX, (void *) aucTaps);
  }

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
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Public Certificate File.", acRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if (psProperties->pcPrivateKeyFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Undefined Private Key File.", acRoutine);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }

    if (!SSL_CTX_use_certificate_file(psProperties->psslCTX, psProperties->pcPublicCertFile, SSL_FILETYPE_PEM))
    {
      ERR_error_string(ERR_get_error(), acLocalError);
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_CTX_use_certificate_file(): %s", acRoutine, acLocalError);
      SSL_CTX_free(psProperties->psslCTX);
      return NULL;
    }
    if (!SSL_CTX_use_PrivateKey_file(psProperties->psslCTX, psProperties->pcPrivateKeyFile, SSL_FILETYPE_PEM))
    {
      ERR_error_string(ERR_get_error(), acLocalError);
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_CTX_use_PrivateKey_file(): %s", acRoutine, acLocalError);
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
  const char          acRoutine[] = "SSLNewProperties()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  SSL_PROPERTIES     *psSSLProperties;

  psSSLProperties = (SSL_PROPERTIES *) malloc(sizeof(SSL_PROPERTIES));
  if (psSSLProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(psSSLProperties, 0, sizeof(SSL_PROPERTIES));

  iError = SSLSetPublicCertFile(psSSLProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetPrivateKeyFile(psSSLProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetPassPhrase(psSSLProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetBundledCAsFile(psSSLProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SSLFreeProperties(psSSLProperties);
    return NULL;
  }
  iError = SSLSetExpectedPeerCN(psSSLProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    strncpy(pcPassPhrase, (char *) pUserData, iLength + 1);
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
  const char          acRoutine[] = "SSLRead()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iDone;
  int                 iNRead;
  int                 iRRetries;

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
      ERR_error_string(ERR_get_error(), acLocalError);
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_SSL: %s", acRoutine, acLocalError);
      return -1;
      break;

    case SSL_ERROR_WANT_READ:
      if (++iRRetries < SSL_RETRY_LIMIT)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_READ", acRoutine);
        iNRead = SSL_read(ssl, pcData, iLength);
        iDone = 0;
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_READ: Retry limit reached!", acRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_WANT_WRITE:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_WRITE", acRoutine);
      return -1;
      break;

    case SSL_ERROR_WANT_X509_LOOKUP:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_WANT_X509_LOOKUP", acRoutine);
      return -1;
      break;

    case SSL_ERROR_SYSCALL:
      if (ERR_peek_error())
      {
        ERR_error_string(ERR_get_error(), acLocalError);
      }
      if (iNRead == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_SYSCALL: Underlying I/O error: %s", acRoutine, strerror(errno));
        return -1;
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_SYSCALL: Unexpected EOF.", acRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_ZERO_RETURN:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): SSL_ERROR_ZERO_RETURN: The SSL connection has been closed.", acRoutine);
      return 0;
      break;

    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_read(): Undefined error.", acRoutine);
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
  const char          acRoutine[] = "SSLSetBundledCAsFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSLProperties.", acRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcBundledCAsFile, pcBundledCAsFile, acLocalError);
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
 * SSLSetDynamicString
 *
 ***********************************************************************
 */
int
SSLSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          acRoutine[] = "SSLSetDynamicString()";
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
 * SSLSetExpectedPeerCN
 *
 ***********************************************************************
 */
int
SSLSetExpectedPeerCN(SSL_PROPERTIES *psProperties, char *pcExpectedPeerCN, char *pcError)
{
  const char          acRoutine[] = "SSLSetExpectedPeerCN()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSLProperties.", acRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcExpectedPeerCN, pcExpectedPeerCN, acLocalError);
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
 * SSLSetPassPhrase
 *
 ***********************************************************************
 */
int
SSLSetPassPhrase(SSL_PROPERTIES *psProperties, char *pcPassPhrase, char *pcError)
{
  const char          acRoutine[] = "SSLSetPassPhrase()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSLProperties.", acRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPassPhrase, pcPassPhrase, acLocalError);
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
 * SSLSetPrivateKeyFile
 *
 ***********************************************************************
 */
int
SSLSetPrivateKeyFile(SSL_PROPERTIES *psProperties, char *pcPrivateKeyFile, char *pcError)
{
  const char          acRoutine[] = "SSLSetPrivateKeyFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSLProperties.", acRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPrivateKeyFile, pcPrivateKeyFile, acLocalError);
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
 * SSLSetPublicCertFile
 *
 ***********************************************************************
 */
int
SSLSetPublicCertFile(SSL_PROPERTIES *psProperties, char *pcPublicCertFile, char *pcError)
{
  const char          acRoutine[] = "SSLSetPublicCertFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SSLProperties.", acRoutine);
    return -1;
  }

  iError = SSLSetDynamicString(&psProperties->pcPublicCertFile, pcPublicCertFile, acLocalError);
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
 * SSLVerifyCN
 *
 ***********************************************************************
 */
int
SSLVerifyCN(SSL *ssl, char *pcCN, char *pcError)
{
  const char          acRoutine[] = "SSLVerifyCN()";
  char                acPeerCN[SSL_MAX_COMMON_NAME_LENGTH];
  X509               *psX509Cert;

  if (SSL_get_verify_result(ssl) != X509_V_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_get_verify_result(): Invalid Certificate.", acRoutine);
    return -1;
  }

  psX509Cert = SSL_get_peer_certificate(ssl);
  if (psX509Cert == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: SSL_get_peer_certificate(): Missing Certificate.", acRoutine);
    return -1;
  }

  X509_NAME_get_text_by_NID(X509_get_subject_name(psX509Cert), NID_commonName, acPeerCN, SSL_MAX_COMMON_NAME_LENGTH);

  X509_free(psX509Cert);

  if (strcmp(acPeerCN, pcCN) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: CN = [%s] != [%s]: Common Name Mismatch.", acRoutine, acPeerCN, pcCN);
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
  const char          acRoutine[] = "SSLWrite()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iNSent;
  int                 iOffset;
  int                 iToSend;
  int                 iWRetries;

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
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_SSL", acRoutine);
      return -1;
      break;

    case SSL_ERROR_WANT_READ:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_READ", acRoutine);
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
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_WRITE: Retry limit reached!", acRoutine);
        return -1;
      }
      break;

    case SSL_ERROR_WANT_X509_LOOKUP:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_WANT_X509_LOOKUP", acRoutine);
      return -1;
      break;

    case SSL_ERROR_SYSCALL:
      if (ERR_peek_error())
      {
        ERR_error_string(ERR_get_error(), acLocalError);
      }
      if (iNSent == -1)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_SYSCALL: Underlying I/O error: %s", acRoutine, strerror(errno));
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_SYSCALL: Unexpected EOF.", acRoutine);
      }
      return -1;
      break;

    case SSL_ERROR_ZERO_RETURN:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): SSL_ERROR_ZERO_RETURN: The SSL connection has been closed.", acRoutine);
      return -1;
      break;

    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: SSL_write(): Undefined error.", acRoutine);
      return -1;
      break;
    }
  } while (iToSend > 0);

  return iOffset;
}
