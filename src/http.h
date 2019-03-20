/*-
 ***********************************************************************
 *
 * $Id: http.h,v 1.10 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2001-2007 Klayton Monroe, All Rights Reserved.
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

#define HTTP_DEFAULT_HTTP_PORT           80
#define HTTP_DEFAULT_HTTPS_PORT         443

#define HTTP_MAX_MEMORY_SIZE      0x4000000UL

#define HTTP_CONTENT_TYPE_SIZE          256
#define HTTP_JOB_ID_SIZE                256
#define HTTP_REASON_PHRASE_SIZE         256
#define HTTP_SERVER_SIZE                256

#define HTTP_IGNORE_INPUT                 0
#define HTTP_MEMORY_INPUT                 1
#define HTTP_STREAM_INPUT                 2

#define HTTP_IGNORE_OUTPUT                0
#define HTTP_MEMORY_OUTPUT                1
#define HTTP_STREAM_OUTPUT                2

#define FIELD_ContentLength       "Content-Length"
#define FIELD_JobId                       "Job-Id"
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
  char                acContentType[HTTP_CONTENT_TYPE_SIZE];
  char                acJobId[HTTP_JOB_ID_SIZE];
  char                acReasonPhrase[HTTP_REASON_PHRASE_SIZE];
  char                acServer[HTTP_SERVER_SIZE];
  int                 iMajorVersion;
  int                 iMinorVersion;
  int                 iStatusCode;
  int                 iContentLengthFound;
  int                 iJobIdFound;
  K_UINT32            ui32ContentLength;
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
  int                 iScheme;
  K_UINT32            ui32DownloadLimit;
  K_UINT32            ui32ContentLength;
  K_UINT32            ui32IP;
  K_UINT16            ui16Port;
#ifdef USE_SSL
  SSL_PROPERTIES     *psSSLProperties;
#endif
} HTTP_URL;

typedef struct _HTTP_MEMORY_LIST
{
  K_UINT32            ui32Size;
  char               *pcData;
  struct _HTTP_MEMORY_LIST *psNext;
} HTTP_MEMORY_LIST;

typedef struct _HTTP_STREAM_LIST
{
  K_UINT32            ui32Size;
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
char                 *HTTPBuildRequest(HTTP_URL *psURL, char *pcError);
char                 *HTTPEncodeBasic(char *pcUsername, char *pcPassword, char *pcError);
void                  HTTPEncodeCredentials(char *pcCredentials, char *pcAuthorization);
char                 *HTTPEscape(char *pcUnEscaped, char *pcError);
void                  HTTPFreeData(char *pcData);
void                  HTTPFreeURL(HTTP_URL *psURL);
int                   HTTPHexToInt(int i);
HTTP_URL             *HTTPNewURL(char *pcError);
int                   HTTPParseAddress(char *pcUserPassHostPort, HTTP_URL *psURL, char *pcError);
int                   HTTPParseGRELine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
int                   HTTPParseHeader(char *pcResponseHeader, int iResponseHeaderLength, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
int                   HTTPParseHostPort(char *pcHostPort, HTTP_URL *psURL, char *pcError);
int                   HTTPParsePathQuery(char *pcPathQuery, HTTP_URL *psURL, char *pcError);
int                   HTTPParseStatusLine(char *pcLine, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
HTTP_URL             *HTTPParseURL(char *pcURL, char *pcError);
int                   HTTPParseUserPass(char *pcUserPass, HTTP_URL *psURL, char *pcError);
int                   HTTPReadDataIntoMemory(SOCKET_CONTEXT *psSocketCTX, void **ppvData, K_UINT32 ui32ContentLength, char *pcError);
int                   HTTPReadDataIntoStream(SOCKET_CONTEXT *psSocketCTX, FILE *pFile, K_UINT32 ui32ContentLength, char *pcError);
int                   HTTPReadHeader(SOCKET_CONTEXT *psSocketCTX, char *pcResponseHeader, int iMaxResponseLength, char *pcError);
int                   HTTPSetDynamicString(char **ppcValue, char *pcNewValue, char *pcError);
void                  HTTPSetURLDownloadLimit(HTTP_URL *psURL, K_UINT32 ui32Limit);
int                   HTTPSetURLHost(HTTP_URL *psURL, char *pcHost, char *pcError);
int                   HTTPSetURLJobId(HTTP_URL *psURL, char *pcJobId, char *pcError);
int                   HTTPSetURLMeth(HTTP_URL *psURL, char *pcMeth, char *pcError);
int                   HTTPSetURLPass(HTTP_URL *psURL, char *pcPass, char *pcError);
int                   HTTPSetURLPath(HTTP_URL *psURL, char *pcPath, char *pcError);
int                   HTTPSetURLPort(HTTP_URL *psURL, char *pcPort, char *pcError);
int                   HTTPSetURLQuery(HTTP_URL *psURL, char *pcQuery, char *pcError);
int                   HTTPSetURLUser(HTTP_URL *psURL, char *pcUser, char *pcError);
int                   HTTPSubmitRequest(HTTP_URL *psURL, int iInputType, void *pInput, int iOutputType, void *pOutput, HTTP_RESPONSE_HDR *psResponseHeader, char *pcError);
char                 *HTTPUnEscape(char *pcEscaped, int *piUnEscapedLength, char *pcError);
