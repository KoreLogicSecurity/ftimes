/*
 ***********************************************************************
 *
 * $Id: http.h,v 1.1.1.1 2002/01/18 03:17:32 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
 *
 ***********************************************************************
 */

/* #include "socket.h" */

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#ifndef ERRBUF_SIZE
#define ERRBUF_SIZE 1024
#endif

#define HTTP_IO_BUFSIZE 0x4000

#define MAX_CREDENTIALS_LENGTH 64
#define MAX_AUTHORIZATION_LENGTH (4*(MAX_CREDENTIALS_LENGTH))

#define HTTP_AUTH_TYPE_NONE  0
#define HTTP_AUTH_TYPE_BASIC 1

#define HTTP_SCHEME_FILE  1
#define HTTP_SCHEME_HTTP  2
#define HTTP_SCHEME_HTTPS 3

#define HTTP_DEFAULT_HTTP_METHOD "GET"
#define HTTP_DEFAULT_HTTP_PORT 80
#define HTTP_DEFAULT_HTTPS_PORT 443

#define HTTP_MAX_MEMORY_SIZE (1<<26) /* 64MB */

#define HTTP_IGNORE_INPUT 0
#define HTTP_MEMORY_INPUT 1
#define HTTP_STREAM_INPUT 2

#define HTTP_IGNORE_OUTPUT 0
#define HTTP_MEMORY_OUTPUT 1
#define HTTP_STREAM_OUTPUT 2

#define FIELD_ContentLength    "Content-Length"
#define FIELD_TransferEncoding "Transfer-Encoding"

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _HTTP_RESPONSE_HDR
{
  char                cContentType[256],
                      cReasonPhrase[256],
                      cServer[256];
  int                 iMajorVersion,
                      iMinorVersion,
                      iStatusCode;
  int                 iContentLengthFound;
  K_UINT32            ui32ContentLength;
} HTTP_RESPONSE_HDR;

typedef struct _HTTP_URL
{
  char               *pcUser,
                     *pcPass,
                     *pcHost,
                     *pcPort,
                     *pcPath,
                     *pcQuery,
                     *pcMeth;
  int                 iAuthType,
                      iScheme;
  K_UINT32            ui32DownloadLimit,
                      ui32ContentLength,
                      ui32IP;
  K_UINT16            ui16Port;
#ifdef USE_SSL
  SSL_PROPERTIES     *psSSLProperties;
#endif
} HTTP_URL;

typedef struct _HTTP_MEMORY_LIST
{
  K_UINT32            ui32Size;
  char              *pcData;
  struct _HTTP_MEMORY_LIST *pNext;
} HTTP_MEMORY_LIST;

typedef struct _HTTP_STREAM_LIST
{
  K_UINT32            ui32Size;
  FILE              *pFile;
  struct _HTTP_STREAM_LIST *pNext;
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
char                 *HTTPBuildRequest(HTTP_URL *ptURL, char *pcError);
char                 *HTTPEncodeBasic(char *pcUsername, char *pcPassword, char *pcError);
void                  HTTPEncodeCredentials(char *pcCredentials, char *pcAuthorization);
char                 *HTTPEscape(char *pcUnEscaped, char *pcError);
void                  HTTPFreeData(char *pcData);
void                  HTTPFreeURL(HTTP_URL *ptURL);
int                   HTTPHexToInt(int i);
HTTP_URL             *HTTPNewURL(char *pcError);
int                   HTTPParseAddress(char *pcUserPassHostPort, HTTP_URL *ptURL, char *pcError);
int                   HTTPParseGRELine(char *pcLine, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError);
int                   HTTPParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError);
int                   HTTPParseHostPort(char *pcHostPort, HTTP_URL *ptURL, char *pcError);
int                   HTTPParsePathQuery(char *pcPathQuery, HTTP_URL *ptURL, char *pcError);
int                   HTTPParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *ptResponseHeader, char *pcError);
HTTP_URL             *HTTPParseURL(char *pcURL, char *pcError);
int                   HTTPParseUserPass(char *pcUserPass, HTTP_URL *ptURL, char *pcError);
int                   HTTPReadDataIntoMemory(SOCKET_CONTEXT *psockCTX, char **ppcData, K_UINT32 ui32ContentLength, char *pcError);
int                   HTTPReadDataIntoStream(SOCKET_CONTEXT *psockCTX, FILE *pFile, K_UINT32 ui32ContentLength, char *pcError);
int                   HTTPReadHeader(SOCKET_CONTEXT *psockCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError);
int                   HTTPSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
void                  HTTPSetURLDownloadLimit(HTTP_URL *ptURL, K_UINT32 ui32Limit);
int                   HTTPSetURLHost(HTTP_URL *ptURL, char *pcHost, char *pcError);
int                   HTTPSetURLMeth(HTTP_URL *ptURL, char *pcMeth, char *pcError);
int                   HTTPSetURLPass(HTTP_URL *ptURL, char *pcPass, char *pcError);
int                   HTTPSetURLPath(HTTP_URL *ptURL, char *pcPath, char *pcError);
int                   HTTPSetURLPort(HTTP_URL *ptURL, char *pcPort, char *pcError);
int                   HTTPSetURLQuery(HTTP_URL *ptURL, char *pcQuery, char *pcError);
int                   HTTPSetURLUser(HTTP_URL *ptURL, char *pcUser, char *pcError);
int                   HTTPSubmitRequest(HTTP_URL *ptURL, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
char                 *HTTPUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError);
