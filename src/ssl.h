/*-
 ***********************************************************************
 *
 * $Id: ssl.h,v 1.17 2012/01/04 03:12:28 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2012 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _SSL_H_INCLUDED
#define _SSL_H_INCLUDED

#include <openssl/crypto.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/rand.h>

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#ifndef MESSAGE_SIZE
#define MESSAGE_SIZE 1024
#endif

#define SSL_DEFAULT_CHAIN_LENGTH          1
#define SSL_MAX_CHAIN_LENGTH             10
#define SSL_MAX_COMMON_NAME_LENGTH      256
#define SSL_MAX_ERROR_MESSAGE_LENGTH   1024
#define SSL_MAX_PASSWORD_LENGTH          32
#define SSL_MAX_VERSION_LENGTH           32
#define SSL_SEED_LENGTH                1024
#define SSL_READ_BUFSIZE             0x4000
#define SSL_RETRY_LIMIT                  20

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _SSL_PROPERTIES
{
  char               *pcPublicCertFile;
  char               *pcPrivateKeyFile;
  char               *pcPassPhrase;
  char               *pcBundledCAsFile;
  char               *pcExpectedPeerCN;
  int                 iMaxChainLength;
  int                 iUseCertificate;
  int                 iVerifyPeerCert;
  SSL_CTX            *psslCTX;
} SSL_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
void                  SslBoot(void);
SSL                  *SslConnect(int iSocket, SSL_CTX *psslCTX, char *pcError);
void                  SslFreeProperties(SSL_PROPERTIES *psSslProperties);
unsigned char        *SslGenerateSeed(unsigned char *pucSeed, unsigned long iLength);
char                 *SslGetVersion(void);
SSL_CTX              *SslInitializeCTX(SSL_PROPERTIES *psProperties, char *pcError);
SSL_PROPERTIES       *SslNewProperties(char *pcError);
int                   SslPassPhraseHandler(char *pcPassPhrase, int iSize, int iRWFlag, void *pUserData);
int                   SslRead(SSL *ssl, char *pcData, int iLength, char *pcError);
void                  SslSessionCleanup(SSL *ssl);
int                   SslSetBundledCAsFile(SSL_PROPERTIES *psProperties, char *pcBundledCAsFile, char *pcError);
int                   SslSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
int                   SslSetExpectedPeerCN(SSL_PROPERTIES *psProperties, char *pcExpectedPeerCN, char *pcError);
int                   SslSetPassPhrase(SSL_PROPERTIES *psProperties, char *pcPassPhrase, char *pcError);
int                   SslSetPrivateKeyFile(SSL_PROPERTIES *psProperties, char *pcPrivateKeyFile, char *pcError);
int                   SslSetPublicCertFile(SSL_PROPERTIES *psProperties, char *pcPublicCertFile, char *pcError);
int                   SslVerifyCN(SSL *ssl, char *pcCN, char *pcError);
int                   SslWrite(SSL *ssl, char *pcData, int iLength, char *pcError);

#endif /* !_SSL_H_INCLUDED */
