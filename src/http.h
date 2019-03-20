/*-
 ***********************************************************************
 *
 * $Id: http.h,v 1.24 2019/03/14 16:07:42 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _HTTP_H_INCLUDED
#define _HTTP_H_INCLUDED

/* #include "socket.h" */

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

#define HTTP_IO_BUFSIZE              0x4000

#define HTTP_AUTH_TYPE_NONE               0
#define HTTP_AUTH_TYPE_BASIC              1

#define HTTP_SCHEME_FILE                  1
#define HTTP_SCHEME_HTTP                  2
#define HTTP_SCHEME_HTTPS                 3

#define HTTP_DEFAULT_HTTP_METHOD       "GET"
#define HTTP_DEFAULT_JOB_ID               ""
#define HTTP_DEFAULT_PROXY_PASS           ""
#define HTTP_DEFAULT_PROXY_USER           ""

#define HTTP_DEFAULT_HTTP_PORT           80
#define HTTP_DEFAULT_HTTPS_PORT         443
#define HTTP_DEFAULT_PROXY_PORT        8080

#define HTTP_MAX_MEMORY_SIZE      0x4000000UL

#define HTTP_CONTENT_TYPE_SIZE          256
#define HTTP_JOB_ID_SIZE                256
#define HTTP_PORT_SIZE                    6
#define HTTP_REASON_PHRASE_SIZE         256
#define HTTP_SERVER_SIZE                256
#ifdef USE_DSV
#define HTTP_WEBJOB_PAYLOAD_SIGNATURE_SIZE 256
#endif

#define HTTP_IGNORE_INPUT                 0
#define HTTP_MEMORY_INPUT                 1
#define HTTP_STREAM_INPUT                 2

#define HTTP_IGNORE_OUTPUT                0
#define HTTP_MEMORY_OUTPUT                1
#define HTTP_STREAM_OUTPUT                2

#define HTTP_FLAG_USE_HTTP_1_0            0x00000001
#define HTTP_FLAG_CONTENT_LENGTH_OPTIONAL 0x00000002

#define FIELD_ContentLength       "Content-Length"
#define FIELD_JobId                       "Job-Id"
#define FIELD_TransferEncoding "Transfer-Encoding"
#ifdef USE_DSV
#define FIELD_WebJobPayloadSignature "WebJob-Payload-Signature"
#endif

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _HTTP_RESPONSE_HDR
{
  char                acContentType[HTTP_CONTENT_TYPE_SIZE];
  char                acJobId[HTTP_JOB_ID_SIZE];
  char                acReasonPhrase[HTTP_REASON_PHRASE_SIZE];
  char                acServer[HTTP_SERVER_SIZE];
#ifdef USE_DSV
  char                acWebJobPayloadSignature[HTTP_WEBJOB_PAYLOAD_SIGNATURE_SIZE];
#endif
  int                 iMajorVersion;
  int                 iMinorVersion;
  int                 iStatusCode;
  int                 iContentLengthFound;
  int                 iJobIdFound;
#ifdef USE_DSV
  int                 iWebJobPayloadSignatureFound;
#endif
  APP_UI32            ui32ContentLength;
} HTTP_RESPONSE_HDR;

typedef struct _HTTP_URL
{
  char               *pcUser;
  char               *pcPass;
  char               *pcHost;
  char               *pcPort;
  char               *pcPath;
  char               *pcQuery;
  char               *pcMeth;
  char               *pcJobId;
  int                 iAuthType;
  int                 iFlags;
  int                 iScheme;
  APP_UI32            ui32DownloadLimit;
  APP_UI32            ui32ContentLength;
  APP_UI32            ui32Ip;
  APP_UI16            ui16Port;

  char               *pcProxyHost;
  char               *pcProxyPass;
  char               *pcProxyPort;
  char               *pcProxyUser;
  int                 iProxyAuthType;
  int                 iUseProxy;
  APP_UI16            ui16ProxyPort;
  APP_UI32            ui32ProxyIp;

#ifdef USE_SSL
  SSL_PROPERTIES     *psSslProperties;
#endif
} HTTP_URL;

typedef struct _HTTP_MEMORY_LIST
{
  APP_UI32            ui32Size;
  char               *pcData;
  struct _HTTP_MEMORY_LIST *psNext;
} HTTP_MEMORY_LIST;

typedef struct _HTTP_STREAM_LIST
{
  APP_UI32            ui32Size;
  FILE               *pFile;
  struct _HTTP_STREAM_LIST *psNext;
} HTTP_STREAM_LIST;

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define HTTP_TERMINATE_FIELD(pc) {while((*(pc))&&(*(pc)!=' ')&&(*(pc)!='\t')){(pc)++;}*(pc)=0;(pc)++;}

#define HTTP_TERMINATE_LINE(pc) {while(*(pc)){if(*(pc)=='\n'){if(*((pc)-1)=='\r'){*((pc)-1)=0;}*(pc)=0;break;}(pc)++;}}

#define HTTP_SKIP_SPACES_TABS(pc) {while((*(pc))&&((*(pc)==' ')||(*(pc)=='\t'))){(pc)++;}}

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
char                 *HttpBuildProxyConnectRequest(HTTP_URL *psUrl, char *pcError);
char                 *HttpBuildRequest(HTTP_URL *psUrl, char *pcError);
SOCKET_CONTEXT       *HttpConnect(HTTP_URL *psUrl, int iSocketType, char *pcError);
char                 *HttpEncodeBasic(char *pcUsername, char *pcPassword, char *pcError);
void                  HttpEncodeCredentials(char *pcCredentials, char *pcAuthorization);
char                 *HttpEscape(char *pcUnEscaped, char *pcError);
void                  HttpFreeData(char *pcData);
void                  HttpFreeUrl(HTTP_URL *psUrl);
int                   HttpHexToInt(int i);
HTTP_URL             *HttpNewUrl(char *pcError);
int                   HttpParseAddress(char *pcUserPassHostPort, HTTP_URL *psUrl, char *pcError);
int                   HttpParseGreLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
int                   HttpParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
int                   HttpParseHostPort(char *pcHostPort, HTTP_URL *psUrl, char *pcError);
int                   HttpParsePathQuery(char *pcPathQuery, HTTP_URL *psUrl, char *pcError);
int                   HttpParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
HTTP_URL             *HttpParseUrl(char *pcUrl, char *pcError);
int                   HttpParseUserPass(char *pcUserPass, HTTP_URL *psUrl, char *pcError);
int                   HttpReadDataIntoMemory(SOCKET_CONTEXT *psSocketCTX, void **ppvData, APP_UI32 ui32ContentLength, int iFlags, char *pcError);
int                   HttpReadDataIntoStream(SOCKET_CONTEXT *psSocketCTX, FILE *pFile, APP_UI32 ui32ContentLength, int iFlags, char *pcError);
int                   HttpReadHeader(SOCKET_CONTEXT *psSocketCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError);
int                   HttpSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
void                  HttpSetUrlDownloadLimit(HTTP_URL *psUrl, APP_UI32 ui32Limit);
int                   HttpSetUrlHost(HTTP_URL *psUrl, char *pcHost, char *pcError);
int                   HttpSetUrlJobId(HTTP_URL *psUrl, char *pcJobId, char *pcError);
int                   HttpSetUrlMeth(HTTP_URL *psUrl, char *pcMeth, char *pcError);
int                   HttpSetUrlPass(HTTP_URL *psUrl, char *pcPass, char *pcError);
int                   HttpSetUrlPath(HTTP_URL *psUrl, char *pcPath, char *pcError);
int                   HttpSetUrlPort(HTTP_URL *psUrl, char *pcPort, char *pcError);
int                   HttpSetUrlProxyHost(HTTP_URL *psUrl, char *pcProxyHost, char *pcError);
int                   HttpSetUrlProxyPass(HTTP_URL *psUrl, char *pcProxyPass, char *pcError);
int                   HttpSetUrlProxyPort(HTTP_URL *psUrl, char *pcProxyPort, char *pcError);
int                   HttpSetUrlProxyUser(HTTP_URL *psUrl, char *pcProxyUser, char *pcError);
int                   HttpSetUrlQuery(HTTP_URL *psUrl, char *pcQuery, char *pcError);
int                   HttpSetUrlUser(HTTP_URL *psUrl, char *pcUser, char *pcError);
int                   HttpSubmitRequest(HTTP_URL *psUrl, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
char                 *HttpUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError);

#endif /* !_HTTP_H_INCLUDED */
