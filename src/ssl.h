/*-
 ***********************************************************************
 *
 * $Id: ssl.h,v 1.8 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
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
void                  SSLBoot(void);
SSL                  *SSLConnect(int iSocket, SSL_CTX *psslCTX, char *pcError);
void                  SSLFreeProperties(SSL_PROPERTIES *psSSLProperties);
unsigned char        *SSLGenerateSeed(unsigned char *pucSeed, unsigned long iLength);
SSL_CTX              *SSLInitializeCTX(SSL_PROPERTIES *psProperties, char *pcError);
SSL_PROPERTIES       *SSLNewProperties(char *pcError);
int                   SSLPassPhraseHandler(char *pcPassPhrase, int iSize, int iRWFlag, void *pUserData);
int                   SSLRead(SSL *ssl, char *pcData, int iLength, char *pcError);
void                  SSLSessionCleanup(SSL *ssl);
int                   SSLSetBundledCAsFile(SSL_PROPERTIES *psProperties, char *pcBundledCAsFile, char *pcError);
int                   SSLSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
int                   SSLSetExpectedPeerCN(SSL_PROPERTIES *psProperties, char *pcExpectedPeerCN, char *pcError);
int                   SSLSetPassPhrase(SSL_PROPERTIES *psProperties, char *pcPassPhrase, char *pcError);
int                   SSLSetPrivateKeyFile(SSL_PROPERTIES *psProperties, char *pcPrivateKeyFile, char *pcError);
int                   SSLSetPublicCertFile(SSL_PROPERTIES *psProperties, char *pcPublicCertFile, char *pcError);
int                   SSLVerifyCN(SSL *ssl, char *pcCN, char *pcError);
int                   SSLWrite(SSL *ssl, char *pcData, int iLength, char *pcError);
