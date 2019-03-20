/*-
 ***********************************************************************
 *
 * $Id: ssl.c,v 1.22 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * SslBoot
 *
 ***********************************************************************
 */
void
SslBoot(void)
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
  RAND_seed(SslGenerateSeed(aucSeed, SSL_SEED_LENGTH), SSL_SEED_LENGTH);
}


/*-
 ***********************************************************************
 *
 * SslConnect
 *
 ***********************************************************************
 */
SSL *
SslConnect(int iSocket, SSL_CTX *psslCTX, char *pcError)
{
  const char          acRoutine[] = "SslConnect()";
  char                acLocalError[MESSAGE_SIZE] = "";
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
 * SslFreeProperties
 *
 ***********************************************************************
 */
void
SslFreeProperties(SSL_PROPERTIES *psSslProperties)
{
  if (psSslProperties != NULL)
  {
    if (psSslProperties->pcPublicCertFile != NULL)
    {
      free(psSslProperties->pcPublicCertFile);
    }
    if (psSslProperties->pcPrivateKeyFile != NULL)
    {
      free(psSslProperties->pcPrivateKeyFile);
    }
    if (psSslProperties->pcPassPhrase != NULL)
    {
      free(psSslProperties->pcPassPhrase);
    }
    if (psSslProperties->pcBundledCAsFile != NULL)
    {
      free(psSslProperties->pcBundledCAsFile);
    }
    if (psSslProperties->pcExpectedPeerCN != NULL)
    {
      free(psSslProperties->pcExpectedPeerCN);
    }
    if (psSslProperties->psslCTX != NULL)
    {
      SSL_CTX_free(psSslProperties->psslCTX);
    }
    free(psSslProperties);
  }
}


/*-
 ***********************************************************************
 *
 * SslGenerateSeed
 *
 ***********************************************************************
 */
unsigned char *
SslGenerateSeed(unsigned char *pucSeed, unsigned long iLength)
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
 * SslGetVersion
 *
 ***********************************************************************
 */
char *
SslGetVersion(void)
{
  static char         acSslVersion[SSL_MAX_VERSION_LENGTH] = "";
  static char         acSslPatch[2] = "";
  static char         acSslPatchCodes[] = " abcdefghijklmnopqrstuvwxyz";
  int                 iPatch = 0;
  long                lVersion = SSLeay();

  /*-
   *********************************************************************
   *
   *  The OpenSSL version number allocates 8 bits to the major number,
   *  8 bits to the minor number, 8 bits to the fix number, 8 bits to
   *  the patch number, and 4 bits to the status number. This has the
   *  following breakout.
   *
   *    +--------+--------+--------+--------+----+
   *    |33333322|22222222|11111111|11      |    |
   *    |54321098|76543210|98765432|10987654|3210|
   *    |--------|--------|--------|--------|----+
   *    |MMMMMMMM|mmmmmmmm|ffffffff|pppppppp|ssss|
   *    +--------+--------+--------|--------+----+
   *     ^^^^^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^^^^^ ^^^^
   *            |        |       |         |    |
   *            |        |       |         |    +-----> s - status (0.....15)
   *            |        |       |         +----------> p - patch  (0....255)
   *            |        |       +--------------------> f - fix    (0....255)
   *            |        +----------------------------> m - minor  (0....255)
   *            +-------------------------------------> M - major  (0....255)
   *
   *    Note that this represents a 36-bit number, which may require
   *    64-bit handling at some point. See OPENSSL_VERSION_NUMBER(3)
   *    for details,
   *
   *    Previously the OPENSSL_VERSION_NUMBER macro was used to
   *    determine the version number. However, this causes a problem
   *    for dynamically linked programs -- if the library is updated
   *    and its version number changes, that change won't be reflected
   *    in the already compiled program. The remedy employed here is
   *    to obtain the version number at run time rather than compile
   *    time.
   *
   *********************************************************************
   */
  iPatch = (int)((lVersion >> 4) & 0xff);
  if (iPatch == 0)
  {
    acSslPatch[0] = 0;
  }
  else if (iPatch > 0 && iPatch < 27)
  {
    acSslPatch[0] = acSslPatchCodes[iPatch];
  }
  else
  {
    acSslPatch[0] = '?';
  }
  acSslPatch[1] = 0;

  snprintf(acSslVersion, SSL_MAX_VERSION_LENGTH, "ssl(%d.%d.%d%s)",
    (int)((lVersion >> 28) & 0xff),
    (int)((lVersion >> 20) & 0xff),
    (int)((lVersion >> 12) & 0xff),
    acSslPatch
    );

  return acSslVersion;
}


/*-
 ***********************************************************************
 *
 * SslInitializeCTX
 *
 ***********************************************************************
 */
SSL_CTX *
SslInitializeCTX(SSL_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "SslInitializeCTX()";
  char                acLocalError[MESSAGE_SIZE] = "";

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
    SSL_CTX_set_default_passwd_cb(psProperties->psslCTX, SslPassPhraseHandler);
    SSL_CTX_set_default_passwd_cb_userdata(psProperties->psslCTX, (void *) aucTaps);
  }

  if (psProperties->pcPassPhrase != NULL && psProperties->pcPassPhrase[0])
  {
    SSL_CTX_set_default_passwd_cb(psProperties->psslCTX, SslPassPhraseHandler);
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
 * SslNewProperties
 *
 ***********************************************************************
 */
SSL_PROPERTIES *
SslNewProperties(char *pcError)
{
  const char          acRoutine[] = "SslNewProperties()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;
  SSL_PROPERTIES     *psSslProperties;

  psSslProperties = (SSL_PROPERTIES *) malloc(sizeof(SSL_PROPERTIES));
  if (psSslProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  memset(psSslProperties, 0, sizeof(SSL_PROPERTIES));

  iError = SslSetPublicCertFile(psSslProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SslFreeProperties(psSslProperties);
    return NULL;
  }
  iError = SslSetPrivateKeyFile(psSslProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SslFreeProperties(psSslProperties);
    return NULL;
  }
  iError = SslSetPassPhrase(psSslProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SslFreeProperties(psSslProperties);
    return NULL;
  }
  iError = SslSetBundledCAsFile(psSslProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SslFreeProperties(psSslProperties);
    return NULL;
  }
  iError = SslSetExpectedPeerCN(psSslProperties, "", acLocalError);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    SslFreeProperties(psSslProperties);
    return NULL;
  }

  psSslProperties->iMaxChainLength = SSL_DEFAULT_CHAIN_LENGTH;

  return psSslProperties;
}


/*-
 ***********************************************************************
 *
 * SslPassPhraseHandler
 *
 ***********************************************************************
 */
int
SslPassPhraseHandler(char *pcPassPhrase, int iSize, int iRWFlag, void *pUserData)
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
 * SslRead
 *
 ***********************************************************************
 */
int
SslRead(SSL *ssl, char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SslRead()";
  char                acLocalError[MESSAGE_SIZE] = "";
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
 * SslSessionCleanup
 *
 ***********************************************************************
 */
void
SslSessionCleanup(SSL *ssl)
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
 * SslSetBundledCAsFile
 *
 ***********************************************************************
 */
int
SslSetBundledCAsFile(SSL_PROPERTIES *psProperties, char *pcBundledCAsFile, char *pcError)
{
  const char          acRoutine[] = "SslSetBundledCAsFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SslProperties.", acRoutine);
    return -1;
  }

  iError = SslSetDynamicString(&psProperties->pcBundledCAsFile, pcBundledCAsFile, acLocalError);
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
 * SslSetDynamicString
 *
 ***********************************************************************
 */
int
SslSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError)
{
  const char          acRoutine[] = "SslSetDynamicString()";
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
 * SslSetExpectedPeerCN
 *
 ***********************************************************************
 */
int
SslSetExpectedPeerCN(SSL_PROPERTIES *psProperties, char *pcExpectedPeerCN, char *pcError)
{
  const char          acRoutine[] = "SslSetExpectedPeerCN()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SslProperties.", acRoutine);
    return -1;
  }

  iError = SslSetDynamicString(&psProperties->pcExpectedPeerCN, pcExpectedPeerCN, acLocalError);
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
 * SslSetPassPhrase
 *
 ***********************************************************************
 */
int
SslSetPassPhrase(SSL_PROPERTIES *psProperties, char *pcPassPhrase, char *pcError)
{
  const char          acRoutine[] = "SslSetPassPhrase()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SslProperties.", acRoutine);
    return -1;
  }

  iError = SslSetDynamicString(&psProperties->pcPassPhrase, pcPassPhrase, acLocalError);
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
 * SslSetPrivateKeyFile
 *
 ***********************************************************************
 */
int
SslSetPrivateKeyFile(SSL_PROPERTIES *psProperties, char *pcPrivateKeyFile, char *pcError)
{
  const char          acRoutine[] = "SslSetPrivateKeyFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SslProperties.", acRoutine);
    return -1;
  }

  iError = SslSetDynamicString(&psProperties->pcPrivateKeyFile, pcPrivateKeyFile, acLocalError);
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
 * SslSetPublicCertFile
 *
 ***********************************************************************
 */
int
SslSetPublicCertFile(SSL_PROPERTIES *psProperties, char *pcPublicCertFile, char *pcError)
{
  const char          acRoutine[] = "SslSetPublicCertFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Undefined SslProperties.", acRoutine);
    return -1;
  }

  iError = SslSetDynamicString(&psProperties->pcPublicCertFile, pcPublicCertFile, acLocalError);
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
 * SslVerifyCN
 *
 ***********************************************************************
 */
int
SslVerifyCN(SSL *ssl, char *pcCN, char *pcError)
{
  const char          acRoutine[] = "SslVerifyCN()";
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
 * SslWrite
 *
 ***********************************************************************
 */
int
SslWrite(SSL *ssl, char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SslWrite()";
  char                acLocalError[MESSAGE_SIZE] = "";
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
