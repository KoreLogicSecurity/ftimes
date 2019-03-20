/*-
 ***********************************************************************
 *
 * $Id: ftimes-srm.h,v 1.10 2019/03/14 16:07:44 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2017-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FTIMES_SRM_H_INCLUDED
#define _FTIMES_SRM_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define PROGRAM_NAME "ftimes-srm"
#define VERSION "1.0.0"

#ifdef WIN32
  #ifdef MINGW32
    #define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000LL
    #define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180LL
  #else
    #define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000
    #define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180
  #endif
#else
  #define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000LL
  #define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180LL
#endif

#define ER       -1
#define ER_OK     0

#define XER_OK    0
#define XER_Usage 1
#define XER_Abort 2
#define XER_BootStrap 3
#define XER_ProcessArguments 4

#define FTIMES_SRM_MAX_ACTION_SIZE  7 /* Size of longest action. */
#define FTIMES_SRM_MAX_STATUS_SIZE  5 /* Size of longest status. */
#define FTIMES_SRM_MAX_REASON_SIZE 20 /* Size of longest reason. */

#define FTIMES_MAX_PATH                 4096
#ifdef WINNT
#define FTIMES_SLASHCHAR                 '\\'
#else
#define FTIMES_SLASHCHAR                  '/'
#endif

#define FTIMES_MAX_LINE                 8192

#define FTIMES_EXTENDED_PREFIX_SIZE        4

#define FTIMES_MAX_MD5_LENGTH (((MD5_HASH_SIZE)*2)+1)
#define FTIMES_MAX_SHA1_LENGTH (((SHA1_HASH_SIZE)*2)+1)
#define FTIMES_MAX_SHA256_LENGTH (((SHA256_HASH_SIZE)*2)+1)

#define PUTBIT(x, v, p) (x) = ((x) & ~(1 << (p))) | (((v) & 1)<< (p))
#define GETBIT(x, p) ((x) & (1 << (p))) >> (p)

#define FTIMES_MAX_32BIT_NUMBER_LENGTH    11 /* 4294967295 */
#define FTIMES_MAX_32BIT_SIZE             36 /* (prefix [+-](0x|0|)) 3 + (binary string) 32 + (NULL) 1 */
#define FTIMES_MAX_64BIT_SIZE             68 /* (prefix [+-](0x|0|)) 3 + (binary string) 64 + (NULL) 1 */
#define FTIMES_DATETIME_SIZE              15
#define FTIMES_TIME_SIZE                  20
#define FTIMES_PID_SIZE                   11
#define FTIMES_RUNTIME_FORMAT      "%H:%M:%S"
#define FTIMES_RUNDATE_FORMAT      "%Y-%m-%d"
#define FTIMES_RUNZONE_FORMAT            "%Z"
#define FTIMES_SUFFIX_SIZE                64
#define FTIMES_ZONE_SIZE                  64

#ifdef WIN32
#define FTIMES_TIME_FORMAT_SIZE           24
#define FTIMES_TIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d|%d"
#define FTIMES_OOB_TIME_FORMAT_SIZE       19
#define FTIMES_OOB_TIME_FORMAT "%04d%02d%02d%02d%02d%02d|%x"
#else
#define FTIMES_TIME_FORMAT_SIZE           20
#define FTIMES_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#endif

#define FTIMES_YMDHMS_FORMAT_SIZE         20
#define FTIMES_YMDHMS_FORMAT "%Y-%m-%d %H:%M:%S"

#define FTIMES_MAX_32BIT_SIZE             36 /* (prefix [+-](0x|0|)) 3 + (binary string) 32 + (NULL) 1 */
#define FTIMES_MAX_64BIT_SIZE             68 /* (prefix [+-](0x|0|)) 3 + (binary string) 64 + (NULL) 1 */

#define LF            "\n"
#define CRLF        "\r\n"

#ifdef WIN32
#define FTIMES_SRM_EXTENDED_PATH_PREFIX "\\\\?\\"
#endif

#define FTIMES_SRM_DEFAULT_BLOCKSIZE 32768

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
#ifdef UNIX
  #ifdef FALSE
    #undef FALSE
  #endif
  #ifdef TRUE
    #undef TRUE
  #endif
  typedef enum _BOOL
  {
    FALSE = 0,
    TRUE
  } BOOL;
#endif

enum InternalErrors
{
  ER_Failure = 256,
  ER_Warning,
  ER_NullFields
};

enum ErrorLevels
{
  ERROR_WARNING,
  ERROR_FAILURE,
  ERROR_CRITICAL
};

enum FTimesSrmActionList
{
  FTIMES_SRM_ACTION_REMOVE = 1,
  FTIMES_SRM_ACTION_REJECT
};

enum FTimesSrmStatusList
{
  FTIMES_SRM_STATUS_PASS = 1,
  FTIMES_SRM_STATUS_FAIL,
  FTIMES_SRM_STATUS_SKIP,
  FTIMES_SRM_STATUS_NOOP
};

enum FTimesSrmReasonList
{
  FTIMES_SRM_REASON_FILE_FTDATA_FAILURE = 1,
  FTIMES_SRM_REASON_FILE_DOES_NOT_EXIST,
  FTIMES_SRM_REASON_FILE_ATTRIB_FAILURE,
  FTIMES_SRM_REASON_FILE_IS_NOT_REGULAR,
  FTIMES_SRM_REASON_FILE_ACCESS_FAILURE,
  FTIMES_SRM_REASON_FILE_IOREAD_FAILURE,
  FTIMES_SRM_REASON_CONSTRAINTS_NOT_MET,
  FTIMES_SRM_REASON_FILE_UNLINK_SUCCESS,
  FTIMES_SRM_REASON_FILE_UNLINK_FAILURE
};

typedef struct _FTIMES_SRM_HANDLE
{
  char               *pcDecodedName;
  FILE               *pFile;
#ifdef WIN32
  char               *pcFileA;
  HANDLE              hFile;
  int                 iFile;
  wchar_t            *pcFileW;
#endif
} FTIMES_SRM_HANDLE;

typedef enum _FTIMES_SRM_OPTION_IDS
{
  OPT_DryRun,
  OPT_FieldMask,
  OPT_LogToStdout,
  OPT_Version,
} FTIMES_SRM_OPTION_IDS;

typedef struct _FTIMES_FILE_DATA
{
  char               *pcNeuteredPath;
  char               *pcRawPath;
  int                 iAction;
  int                 iStatus;
  int                 iReason;
  int                 iExists;
  int                 iNeuteredPathLength;
  int                 iRawPathLength;
  unsigned char       aucFileMd5[MD5_HASH_SIZE];
  unsigned char       aucFileSha1[SHA1_HASH_SIZE];
  unsigned char       aucFileSha256[SHA256_HASH_SIZE];
  unsigned long       ulAttributeMask;
  APP_UI64            ui64FileSize;
  MD5_CONTEXT         sFileMd5;
  SHA1_CONTEXT        sFileSha1;
  SHA256_CONTEXT      sFileSha256;
#ifdef WINNT
  DWORD               dwFileAttributes;
  DWORD               dwFileSizeHigh;
  DWORD               dwFileSizeLow;
  int                 iUtf8RawPathLength;
  int                 iWideRawPathLength;
  wchar_t            *pwcRawPath;
#else
  struct stat         sStatEntry;
#endif
} FTIMES_FILE_DATA;

typedef struct _FTIMES_SRM_PROPERTIES
{
  FILE               *pLogHandle;
  int                 iDryRun;
  MASK_USS_MASK      *psFieldMask;
  OPTIONS_CONTEXT    *psOptionsContext;
} FTIMES_SRM_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
void                ErrorHandler(int iError, char *pcError, int iSeverity);

void                FTimesSrmAbort();
int                 FTimesSrmBootStrap(char *pcError);
char               *FTimesSrmDecodeString(char *pcEncoded, char *pcError);
#ifdef WINNT
void                FTimesSrmFormatWinxError(DWORD dwError, TCHAR **pptcMessage);
#endif
void                FTimesSrmFreeHandle(FTIMES_SRM_HANDLE *psHandle);
void                FTimesSrmFreeProperties(FTIMES_SRM_PROPERTIES *psProperties);
int                 FTimesSrmGetAttributes(FTIMES_FILE_DATA *psFTFileData, char *pcError);
FTIMES_SRM_HANDLE  *FTimesSrmGetHandle(char *pcDecodedName, char *pcError);
FTIMES_SRM_PROPERTIES *FTimesSrmGetPropertiesReference(void);
FTIMES_SRM_PROPERTIES *FTimesSrmNewProperties(char *pcError);
int                 FTimesSrmOptionHandler(OPTIONS_TABLE *psOption, char *pcValue, FTIMES_SRM_PROPERTIES *psProperties, char *pcError);
int                 FTimesSrmProcessArguments(int iArgumentCount, char *ppcArgumentVector[], FTIMES_SRM_PROPERTIES *psProperties, char *pcError);
int                 FTimesSrmRemoveFile(SNAPSHOT_CONTEXT *psSnapshotContext, MASK_USS_MASK *psFieldMask, int iDryRun, char *pcError);
void                FTimesSrmSetPropertiesReference(FTIMES_SRM_PROPERTIES *psProperties);
void                FTimesSrmUsage(void);
wchar_t            *FTimesSrmUtf8ToWide(char *pcString, int iUtf8Size, char *pcError);
void                FTimesSrmVersion(void);

void                MapFreeFTFileData(FTIMES_FILE_DATA *psFTFileData);
#ifdef WINNT
FTIMES_FILE_DATA   *MapNewFTFileDataW(wchar_t *pwcName, char *pcError);
#else
FTIMES_FILE_DATA   *MapNewFTFileData(char *pcName, char *pcError);
#endif
#ifdef WINNT
wchar_t            *MapUtf8ToWide(char *pcString, int iUtf8Size, char *pcError);
char               *MapWideToUtf8(wchar_t *pwcString, int iWideSize, char *pcError);
#endif
int                 MapWriteHeader(MASK_USS_MASK *psFieldMask, char *pcError);
int                 MapWriteRecord(MASK_USS_MASK *psFieldMask, FTIMES_FILE_DATA *psFTFileData, char *pcError);

int                 SupportChopEOLs(char *pcLine, int iStrict, char *pcError);
FILE               *SupportGetFileHandle(char *pcFile, char *pcError);
char               *SupportNeuterString(char *pcData, int iLength, char *pcError);
#ifdef WINNT
int                 SupportSetPrivileges(char *pcError);
#endif
int                 SupportStringToUInt64(char *pcData, APP_UI64 *pui64Value, char *pcError);
int                 SupportWriteData(FILE *pFile, char *pcData, int iLength, char *pcError);

time_t              TimeGetTime(char *pcDate, char *pcTime, char *pcZone, char *pcDateTime);
int                 TimeGetTimeValue(struct timeval *psTimeValue);
double              TimeGetTimeValueAsDouble(void);

#ifdef WIN32
int                 TimeFormatOutOfBandTime(FILETIME *psFileTime, char *pcTime);
#endif
#ifdef WIN32
int                 TimeFormatTime(FILETIME *psFileTime, char *pcTime);
#else
int                 TimeFormatTime(time_t *pTimeValue, char *pcTime);
#endif

#endif /* !_FTIMES_SRM_H_INCLUDED */
