/*-
 ***********************************************************************
 *
 * $Id: ftimes.h,v 1.167 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FTIMES_H_INCLUDED
#define _FTIMES_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define PROGRAM_NAME "ftimes"

#define LF            "\n"
#define CRLF        "\r\n"
#define NEWLINE_LENGTH  3

#ifdef WIN32
#ifdef MINGW32
#define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000LL
#define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180LL
#else
#define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000
#define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180
#endif
#define DEFAULT_STREAM_NAME_W L"::$DATA"
#define FTIMES_STREAM_INFO_SIZE  0x8000
#define FTIMES_MAX_STREAM_INFO_SIZE 0x00100000
#define FTIMES_INVALID_STREAM_COUNT 0xffffffff
#else
#define UNIX_EPOCH_IN_NT_TIME 0x019db1ded53e8000LL
#define UNIX_LIMIT_IN_NT_TIME 0x01e9fcf4ebcfe180LL
#endif

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

typedef enum _FTIMES_OPTION_IDS
{
  OPT_LogLevel,
  OPT_MagicFile,
  OPT_MemoryMapEnable,
  OPT_NamesAreCaseInsensitive,
  OPT_StrictTesting,
} FTIMES_OPTION_IDS;

#define N_100ns_UNITS_IN_1s         10000000
#define N_100ns_UNITS_IN_1us              10

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
#ifdef UNIX
#define FTIMES_TIME_FORMAT_SIZE           20
#define FTIMES_TIME_FORMAT "%Y-%m-%d %H:%M:%S"
#endif
#ifdef WIN32
#define FTIMES_TIME_FORMAT_SIZE           24
#define FTIMES_TIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d|%d"
#define FTIMES_OOB_TIME_FORMAT_SIZE       19
#define FTIMES_OOB_TIME_FORMAT "%04d%02d%02d%02d%02d%02d|%x"
#endif
#define FTIMES_YMDHMS_FORMAT_SIZE         20
#define FTIMES_YMDHMS_FORMAT "%Y-%m-%d %H:%M:%S"

#ifdef WIN32
#define FTIMES_DOT_W                     L"."
#define FTIMES_DOTDOT_W                 L".."
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
#define FTIMES_EXTENDED_PREFIX_SIZE        4
//END (\\?\)
#define FTIMES_ROOT_PATH                 "c:"
#define FTIMES_SLASH                     "\\"
#define FTIMES_SLASH_W                  L"\\"
#define FTIMES_SLASHCHAR                 '\\'
#define FTIMES_SLASHCHAR_W              L'\\'
#define FTIMES_TEMP_DIRECTORY        "\\temp"
//THIS CHANGE IS PART OF EXTENDED PREFIX SUPPORT (\\?\)
//#define FTIMES_MAX_PATH                  260
#define FTIMES_MAX_PATH                 4096
//END (\\?\)
#define FTIMES_MAX_ACL_SIZE             1024 /* This is an arbitrary limit. An ACL consists of zero or more ACEs, but the output buffer is fixed, so a limit must be imposed. */
#define FTIMES_MAX_SID_SIZE              192 /* S-255-18446744073709551615-4294967295[-4294967295]{0,14} --> 1 + (1 + 3) + (1 + 20) + ((1 + 10) * 15) + 1 */
#else
#define FTIMES_ROOT_PATH                  "/"
#define FTIMES_SLASH                      "/"
#define FTIMES_SLASHCHAR                  '/'
#define FTIMES_TEMP_DIRECTORY          "/tmp"
#define FTIMES_MAX_PATH                 4096
#endif
#define FTIMES_DOT                        "."
#define FTIMES_DOTCHAR                    '.'
#define FTIMES_DOTDOT                    ".."
#define FTIMES_SEPARATOR                  "="

#define FTIMES_MIN_BLOCK_SIZE              1
#define FTIMES_MAX_BLOCK_SIZE     1073741824 /* 1 GB */
#define FTIMES_MAX_KBPS              2097152 /* 2^31/1024 */

#define FTIMES_MIN_STRING_REPEATS          0
#define FTIMES_MAX_STRING_REPEATS 0x7fffffff

#define FTIMES_MIN_FILE_SIZE_LIMIT         0
#define FTIMES_MAX_FILE_SIZE_LIMIT        ~0

#define FTIMES_MIN_MMAP_SIZE        67108864 /* 64 MB */
#define FTIMES_MAX_MMAP_SIZE      1073741824 /* 1 GB */

#define FTIMES_MAX_HOSTNAME_LENGTH       256
#define FTIMES_MAX_MD5_LENGTH (((MD5_HASH_SIZE)*2)+1)
#define FTIMES_MAX_SHA1_LENGTH (((SHA1_HASH_SIZE)*2)+1)
#define FTIMES_MAX_SHA256_LENGTH (((SHA256_HASH_SIZE)*2)+1)
#define FTIMES_MAX_USERNAME_LENGTH        32
#define FTIMES_MAX_PASSWORD_LENGTH        32
#define FTIMES_MAX_PRIORITY_LENGTH        64

#define FTIMES_MAX_LINE                 8192

#define FTIMES_NONCE_SIZE                  9

#define FTIMES_FILETYPE_ERROR              0
#define FTIMES_FILETYPE_BLOCK              1
#define FTIMES_FILETYPE_CHARACTER          2
#define FTIMES_FILETYPE_DIRECTORY          3
#define FTIMES_FILETYPE_DOOR               4
#define FTIMES_FILETYPE_FIFO               5
#define FTIMES_FILETYPE_LINK               6
#define FTIMES_FILETYPE_REGULAR            7
#define FTIMES_FILETYPE_SOCKET             8
#define FTIMES_FILETYPE_WHITEOUT           9
#define FTIMES_FILETYPE_UNKNOWN           10

#ifdef WIN32
#ifndef IDLE_PRIORITY_CLASS
#define IDLE_PRIORITY_CLASS         0x00000040
#endif
#ifndef BELOW_NORMAL_PRIORITY_CLASS
#define BELOW_NORMAL_PRIORITY_CLASS 0x00004000
#endif
#ifndef NORMAL_PRIORITY_CLASS
#define NORMAL_PRIORITY_CLASS       0x00000020
#endif
#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif
#ifndef HIGH_PRIORITY_CLASS
#define HIGH_PRIORITY_CLASS         0x00000080
#endif
#define FTIMES_PRIORITY_LOW IDLE_PRIORITY_CLASS
#define FTIMES_PRIORITY_BELOW_NORMAL BELOW_NORMAL_PRIORITY_CLASS
#define FTIMES_PRIORITY_NORMAL NORMAL_PRIORITY_CLASS
#define FTIMES_PRIORITY_ABOVE_NORMAL ABOVE_NORMAL_PRIORITY_CLASS
#define FTIMES_PRIORITY_HIGH HIGH_PRIORITY_CLASS
#else
#define FTIMES_PRIORITY_LOW               20
#define FTIMES_PRIORITY_BELOW_NORMAL      10
#define FTIMES_PRIORITY_NORMAL             0
#define FTIMES_PRIORITY_ABOVE_NORMAL     -10
#define FTIMES_PRIORITY_HIGH             -20
#endif

#define FTIMES_TEST_NORMAL                 0
#define FTIMES_TEST_STRICT                 1

#define FTIMES_FILETYPE_BUFSIZE         1280 /* XMAGIC_MAX_LEVEL * XMAGIC_DESCRIPTION_BUFSIZE */

typedef struct _FTIMES_HASH_DATA
{
  MD5_CONTEXT         sMd5Context;
  SHA1_CONTEXT        sSha1Context;
  SHA256_CONTEXT      sSha256Context;
} FTIMES_HASH_DATA;

#ifdef WINNT
typedef struct _FTIMES_FILE_DATA
{
  char                acType[FTIMES_FILETYPE_BUFSIZE];
  char               *pcNeuteredPath;
  char               *pcRawPath;
  DWORD               dwFileAttributes;
  DWORD               dwFileIndexHigh;
  DWORD               dwFileIndexLow;
  DWORD               dwFileSizeHigh;
  DWORD               dwFileSizeLow;
  DWORD               dwVolumeSerialNumber;
  FILETIME            sFTATime;
  FILETIME            sFTCTime;
  FILETIME            sFTChTime;
  FILETIME            sFTMTime;
  int                 iFileExists;
  int                 iFiltered;
  int                 iFSType;
  int                 iNeuteredPathLength;
  int                 iStreamCount;
  int                 iUtf8RawPathLength;
  int                 iWideRawPathLength;
  SECURITY_DESCRIPTOR *psSd;
  SID                *psSidOwner;
  SID                *psSidGroup;
  unsigned char       aucFileMd5[MD5_HASH_SIZE];
  unsigned char       aucFileSha1[SHA1_HASH_SIZE];
  unsigned char       aucFileSha256[SHA256_HASH_SIZE];
  unsigned char      *pucStreamInfo;
  unsigned long       ulAttributeMask;
  wchar_t            *pwcRawPath;
  struct _FTIMES_FILE_DATA *psParent;
} FTIMES_FILE_DATA;
#else
typedef struct _FTIMES_FILE_DATA
{
  char                acType[FTIMES_FILETYPE_BUFSIZE];
  char               *pcNeuteredPath;
  char               *pcRawPath;
  int                 iFileExists;
  int                 iFiltered;
  int                 iFSType;
  int                 iNeuteredPathLength;
  int                 iRawPathLength;
  struct stat         sStatEntry;
  unsigned char       aucFileMd5[MD5_HASH_SIZE];
  unsigned char       aucFileSha1[SHA1_HASH_SIZE];
  unsigned char       aucFileSha256[SHA256_HASH_SIZE];
  unsigned long       ulAttributeMask;
  struct _FTIMES_FILE_DATA *psParent;
} FTIMES_FILE_DATA;
#endif

#define FILE_LIST_REGULAR 0
#define FILE_LIST_ENCODED 1
typedef struct _FILE_LIST
{
  char               *pcRegularPath;
  char               *pcEncodedPath;
  int                 iLength;
  int                 iType; /* FILE_LIST_REGULAR or FILE_LIST_ENCODED */
  struct _FILE_LIST  *psNext;
} FILE_LIST;

#ifdef USE_PCRE
typedef struct _FILTER_LIST
{
  char               *pcFilter;
  pcre               *psPcre;
  pcre_extra         *psPcreExtra;
  struct _FILTER_LIST *psNext;
} FILTER_LIST;
#endif

#define FTIMES_CMPDATA "cmp"
#define FTIMES_DIGDATA "dig"
#define FTIMES_MADDATA "mad"
#define FTIMES_MAPDATA "map"
#define FTIMES_MAX_DATA_TYPE 4

#define FTIMES_RECORD_PREFIX_SIZE 5

#define FTIMES_CFGTEST 0x00000001
#define FTIMES_CMPMODE 0x00000002
#define FTIMES_DECODER 0x00000004
#define FTIMES_DIGAUTO 0x00000008
#define FTIMES_DIGMODE 0x00000010
#define FTIMES_GETMODE 0x00000040
#define FTIMES_MAPAUTO 0x00000080
#define FTIMES_MAPMODE 0x00000100
#define FTIMES_VERSION 0x00000400
#define FTIMES_MADMODE 0x00001000

#define FTIMES_DIGMADMAP     0x00001110
#define FTIMES_DIGMAD        0x00001010
#define FTIMES_MADMAP        0x00001100

#define MODES_AnalyzeBlockSize    (FTIMES_DIGMADMAP)
#define MODES_AnalyzeByteCount    (FTIMES_DIGMADMAP)
#define MODES_AnalyzeCarrySize    (FTIMES_DIGMAD)
#ifdef USE_XMAGIC
#define MODES_AnalyzeStepSize     (FTIMES_DIGMAD)
#endif
#define MODES_AnalyzeDeviceFiles  (FTIMES_DIGMADMAP)
#define MODES_AnalyzeMaxDps       (FTIMES_DIGMADMAP)
#define MODES_AnalyzeRemoteFiles  (FTIMES_DIGMADMAP)
#define MODES_AnalyzeStartOffset  (FTIMES_DIGMADMAP)
#define MODES_BaseName            ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_BaseNameSuffix      (FTIMES_DIGMADMAP)
#define MODES_Compress            (FTIMES_MADMAP)
#define MODES_DigString           ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#define MODES_DigStringNoCase     ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#define MODES_DigStringNormal     ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#ifdef USE_PCRE
#define MODES_DigStringRegExp     ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#endif
#ifdef USE_XMAGIC
#define MODES_DigStringXMagic     ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#endif
#define MODES_EnableRecursion     (FTIMES_DIGMADMAP)
#define MODES_Exclude             ((FTIMES_DIGAUTO) | (FTIMES_MAPAUTO) | (FTIMES_DIGMADMAP))
#ifdef USE_PCRE
#define MODES_ExcludeFilter       ((FTIMES_DIGAUTO) | (FTIMES_MAPAUTO) | (FTIMES_DIGMADMAP))
#define MODES_ExcludeFilterMd5    ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#define MODES_ExcludeFilterSha1   ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#define MODES_ExcludeFilterSha256 ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#endif
#define MODES_ExcludesMustExist   (FTIMES_DIGMADMAP)
#define MODES_FieldMask           ((FTIMES_CMPMODE) | (FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#ifdef USE_FILE_HOOKS
#define MODES_FileHook            (FTIMES_DIGMADMAP)
#endif
#define MODES_FileSizeLimit       (FTIMES_DIGMADMAP)
#define MODES_GetAndExec          ((FTIMES_GETMODE))
#define MODES_GetFileName         ((FTIMES_GETMODE))
#define MODES_HashDirectories     (FTIMES_MADMAP)
#define MODES_HashSymbolicLinks   (FTIMES_MADMAP)
#define MODES_Import              ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_Include             ((FTIMES_DIGAUTO) | (FTIMES_MAPAUTO) | (FTIMES_DIGMADMAP))
#ifdef USE_PCRE
#define MODES_IncludeFilter       ((FTIMES_DIGAUTO) | (FTIMES_MAPAUTO) | (FTIMES_DIGMADMAP))
#define MODES_IncludeFilterMd5    ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#define MODES_IncludeFilterSha1   ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#define MODES_IncludeFilterSha256 ((FTIMES_MAPAUTO) | (FTIMES_MADMAP))
#endif
#define MODES_IncludesMustExist   (FTIMES_DIGMADMAP)
#define MODES_LogDigStrings       ((FTIMES_DIGAUTO) | (FTIMES_DIGMAD))
#define MODES_LogDir              (FTIMES_DIGMADMAP)
#define MODES_MagicFile           (FTIMES_MADMAP)
#define MODES_MatchLimit          (FTIMES_DIGMAD)
#define MODES_NewLine             (FTIMES_DIGMADMAP)
#define MODES_OutDir              (FTIMES_DIGMADMAP)
#define MODES_Priority            ((FTIMES_CMPMODE) | (FTIMES_DIGAUTO) | (FTIMES_MAPAUTO) | (FTIMES_DIGMADMAP))
#define MODES_RequirePrivilege    (FTIMES_DIGMADMAP)
#define MODES_RunType             (FTIMES_DIGMADMAP)
#define MODES_StrictControls      ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_URLAuthType         ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_URLGetRequest       ((FTIMES_GETMODE))
#define MODES_URLGetURL           ((FTIMES_GETMODE))
#define MODES_URLPassword         ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_URLPutSnapshot      (FTIMES_DIGMADMAP)
#define MODES_URLPutURL           (FTIMES_DIGMADMAP)
#define MODES_URLUnlinkOutput     (FTIMES_DIGMADMAP)
#define MODES_URLUsername         ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#ifdef USE_SSL
#define MODES_SSLBundledCAsFile   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLExpectedPeerCN   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLMaxChainLength   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLPassPhrase       ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLPrivateKeyFile   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLPublicCertFile   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLUseCertificate   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#define MODES_SSLVerifyPeerCert   ((FTIMES_DIGMADMAP) | (FTIMES_GETMODE))
#endif

#define KEY_AnalyzeBlockSize    "AnalyzeBlockSize"
#define KEY_AnalyzeByteCount    "AnalyzeByteCount"
#define KEY_AnalyzeCarrySize    "AnalyzeCarrySize"
#ifdef USE_XMAGIC
#define KEY_AnalyzeStepSize     "AnalyzeStepSize"
#endif
#define KEY_AnalyzeDeviceFiles  "AnalyzeDeviceFiles"
#define KEY_AnalyzeMaxDps       "AnalyzeMaxDps"
#define KEY_AnalyzeRemoteFiles  "AnalyzeRemoteFiles"
#define KEY_AnalyzeStartOffset  "AnalyzeStartOffset"
#define KEY_BaseName            "BaseName"
#define KEY_BaseNameSuffix      "BaseNameSuffix"
#define KEY_Compress            "Compress"
#define KEY_DigString           "DigString"
#define KEY_DigStringNoCase     "DigStringNoCase"
#define KEY_DigStringNormal     "DigStringNormal"
#ifdef USE_PCRE
#define KEY_DigStringRegExp     "DigStringRegExp"
#endif
#ifdef USE_XMAGIC
#define KEY_DigStringXMagic     "DigStringXMagic"
#endif
#define KEY_EnableRecursion     "EnableRecursion"
#define KEY_Exclude             "Exclude"
#ifdef USE_PCRE
#define KEY_ExcludeFilter       "ExcludeFilter"
#define KEY_ExcludeFilterMd5    "ExcludeFilterMd5"
#define KEY_ExcludeFilterSha1   "ExcludeFilterSha1"
#define KEY_ExcludeFilterSha256 "ExcludeFilterSha256"
#endif
#define KEY_ExcludesMustExist   "ExcludesMustExist"
#define KEY_FieldMask           "FieldMask"
#ifdef USE_FILE_HOOKS
#define KEY_FileHook            "FileHook"
#endif
#define KEY_FileSizeLimit       "FileSizeLimit"
#define KEY_GetAndExec          "GetAndExec"
#define KEY_GetFileName         "GetFileName"
#define KEY_HashDirectories     "HashDirectories"
#define KEY_HashSymbolicLinks   "HashSymbolicLinks"
#define KEY_Import              "Import"
#define KEY_Include             "Include"
#ifdef USE_PCRE
#define KEY_IncludeFilter       "IncludeFilter"
#define KEY_IncludeFilterMd5    "IncludeFilterMd5"
#define KEY_IncludeFilterSha1   "IncludeFilterSha1"
#define KEY_IncludeFilterSha256 "IncludeFilterSha256"
#endif
#define KEY_IncludesMustExist   "IncludesMustExist"
#define KEY_LogDigStrings       "LogDigStrings"
#define KEY_LogDir              "LogDir"
#define KEY_MagicFile           "MagicFile"
#define KEY_MapRemoteFiles      "MapRemoteFiles"
#define KEY_MatchLimit          "MatchLimit"
#define KEY_NewLine             "NewLine"
#define KEY_OutDir              "OutDir"
#define KEY_Priority            "Priority"
#define KEY_RequirePrivilege    "RequirePrivilege"
#define KEY_RunType             "RunType"
#define KEY_StrictControls      "StrictControls"
#define KEY_URLAuthType         "URLAuthType"
#define KEY_URLGetRequest       "URLGetRequest"
#define KEY_URLGetURL           "URLGetURL"
#define KEY_URLPassword         "URLPassword"
#define KEY_URLPutSnapshot      "URLPutSnapshot"
#define KEY_URLPutURL           "URLPutURL"
#define KEY_URLUnlinkOutput     "URLUnlinkOutput"
#define KEY_URLUsername         "URLUsername"
#ifdef USE_SSL
#define KEY_SSLBundledCAsFile   "SSLBundledCAsFile"
#define KEY_SSLExpectedPeerCN   "SSLExpectedPeerCN"
#define KEY_SSLMaxChainLength   "SSLMaxChainLength"
#define KEY_SSLPassPhrase       "SSLPassPhrase"
#define KEY_SSLPrivateKeyFile   "SSLPrivateKeyFile"
#define KEY_SSLPublicCertFile   "SSLPublicCertFile"
#define KEY_SSLUseCertificate   "SSLUseCertificate"
#define KEY_SSLVerifyPeerCert   "SSLVerifyPeerCert"
#endif

typedef struct _CONTROLS_FOUND
{
  BOOL                bAnalyzeBlockSizeFound;
  BOOL                bAnalyzeByteCountFound;
  BOOL                bAnalyzeCarrySizeFound;
#ifdef USE_XMAGIC
  BOOL                bAnalyzeStepSizeFound;
#endif
  BOOL                bAnalyzeDeviceFilesFound;
  BOOL                bAnalyzeMaxDpsFound;
  BOOL                bAnalyzeRemoteFilesFound;
  BOOL                bAnalyzeStartOffsetFound;
  BOOL                bBaseNameFound;
  BOOL                bBaseNameSuffixFound;
  BOOL                bCompressFound;
  BOOL                bEnableRecursionFound;
  BOOL                bExcludesMustExistFound;
  BOOL                bFieldMaskFound;
  BOOL                bFileSizeLimitFound;
  BOOL                bGetAndExecFound;
  BOOL                bGetFileNameFound;
  BOOL                bHashDirectoriesFound;
  BOOL                bHashSymbolicLinksFound;
  BOOL                bIncludesMustExistFound;
  BOOL                bLogDigStringsFound;
  BOOL                bLogDirFound;
  BOOL                bMagicFileFound;
  BOOL                bMatchLimitFound;
  BOOL                bNewLineFound;
  BOOL                bOutDirFound;
  BOOL                bPriorityFound;
  BOOL                bRequirePrivilegeFound;
  BOOL                bRunTypeFound;
  BOOL                bURLAuthTypeFound;
  BOOL                bURLGetRequestFound;
  BOOL                bURLGetURLFound;
  BOOL                bURLPasswordFound;
  BOOL                bURLPutSnapshotFound;
  BOOL                bURLPutURLFound;
  BOOL                bURLUnlinkOutputFound;
  BOOL                bURLUsernameFound;
#ifdef USE_SSL
  BOOL                bSSLBundledCAsFileFound;
  BOOL                bSSLExpectedPeerCNFound;
  BOOL                bSSLMaxChainLengthFound;
  BOOL                bSSLPassPhraseFound;
  BOOL                bSSLPrivateKeyFileFound;
  BOOL                bSSLPublicCertFileFound;
  BOOL                bSSLUseCertificateFound;
  BOOL                bSSLVerifyPeerCertFound;
#endif
} CONTROLS_FOUND;

typedef struct _ANALYSIS_STAGES
{
#define STAGE_DESCRIPTION_SIZE 64
  char                acDescription[STAGE_DESCRIPTION_SIZE];
  int                 iError;
  int               (*piRoutine)();
} ANALYSIS_STAGES;

typedef struct _RUNMODE_STAGES
{
  char                acDescription[STAGE_DESCRIPTION_SIZE];
  int                 iError;
  int               (*piRoutine)();
} RUNMODE_STAGES;

typedef struct _FTIMES_PROPERTIES
{
#define MAX_ANALYSIS_STAGES 32
  ANALYSIS_STAGES     asAnalysisStages[MAX_ANALYSIS_STAGES];
  BOOL                bAnalyzeBlockSize;
  BOOL                bAnalyzeByteCount;
  BOOL                bAnalyzeCarrySize;
#ifdef USE_XMAGIC
  BOOL                bAnalyzeStepSize;
#endif
  BOOL                bAnalyzeDeviceFiles;
  BOOL                bAnalyzeRemoteFiles;
  BOOL                bCompress;
  BOOL                bEnableRecursion;
  BOOL                bExcludesMustExist;
  BOOL                bGetAndExec;
  BOOL                bHashDirectories;
  BOOL                bHashSymbolicLinks;
  BOOL                bHaveAttributeFilters;
  BOOL                bIncludesMustExist;
  BOOL                bLogDigStrings;
  BOOL                bRequirePrivilege;
  BOOL                bStrictControls;
  BOOL                bURLPutSnapshot;
  BOOL                bURLUnlinkOutput;
  char                acBaseName[FTIMES_MAX_PATH];
  char                acBaseNameSuffix[FTIMES_SUFFIX_SIZE];
  char                acCfgFileName[FTIMES_MAX_PATH];
  char                acConfigFile[FTIMES_MAX_PATH];
  char                acDataType[FTIMES_MAX_DATA_TYPE];
  char                acDateTime[FTIMES_TIME_SIZE];
  char                acDigRecordPrefix[FTIMES_RECORD_PREFIX_SIZE];
  char                acGetFileName[FTIMES_MAX_PATH];
  char                acLogDirName[FTIMES_MAX_PATH];
  char                acLogFileName[FTIMES_MAX_PATH];
  char                acMagicFileName[FTIMES_MAX_PATH];
  char                acMagicHash[FTIMES_MAX_MD5_LENGTH];
  char                acMapRecordPrefix[FTIMES_RECORD_PREFIX_SIZE];
  char                acNewLine[NEWLINE_LENGTH];
  char                acOutDirName[FTIMES_MAX_PATH];
  char                acOutFileName[FTIMES_MAX_PATH];
  char                acOutFileHash[FTIMES_MAX_MD5_LENGTH];
  char                acPid[FTIMES_PID_SIZE];
  char                acPriority[FTIMES_MAX_PRIORITY_LENGTH];
  char                acRunDateTime[FTIMES_TIME_SIZE];
#define RUNTYPE_BUFSIZE 16
  char                acRunType[RUNTYPE_BUFSIZE];
  char                acStartDate[FTIMES_TIME_SIZE];
  char                acStartTime[FTIMES_TIME_SIZE];
  char                acStartZone[FTIMES_ZONE_SIZE];
  char                acTempDirectory[FTIMES_MAX_PATH];
#define GET_REQUEST_BUFSIZE 16
  char                acURLGetRequest[GET_REQUEST_BUFSIZE];
  char                acURLPassword[FTIMES_MAX_PASSWORD_LENGTH];
  char                acURLUsername[FTIMES_MAX_USERNAME_LENGTH];
  char               *pcNonce;
  char               *pcProgram;
  char               *pcRunModeArgument;
  char              **ppcMapList;
  CONTROLS_FOUND      sFound;
  double              dStartTime;
  FILE               *pFileLog;
  FILE               *pFileOut;
  FILE_LIST          *psExcludeList;
  FILE_LIST          *psIncludeList;
#ifdef USE_PCRE
  FILTER_LIST        *psExcludeFilterList;
  FILTER_LIST        *psIncludeFilterList;
  FILTER_LIST        *psExcludeFilterMd5List;
  FILTER_LIST        *psIncludeFilterMd5List;
  FILTER_LIST        *psExcludeFilterSha1List;
  FILTER_LIST        *psIncludeFilterSha1List;
  FILTER_LIST        *psExcludeFilterSha256List;
  FILTER_LIST        *psIncludeFilterSha256List;
#endif
#ifdef USE_FILE_HOOKS
  HOOK_LIST          *psFileHookList;
#endif
#define MAX_RUNMODE_STAGES 32
  RUNMODE_STAGES      sRunModeStages[MAX_RUNMODE_STAGES];
  HTTP_URL           *psGetURL;
  HTTP_URL           *psPutURL;
  int                 iAnalyzeBlockSize;
  int                 iAnalyzeCarrySize;
  int                 iAnalyzeMaxDps;
#ifdef USE_XMAGIC
  int                 iAnalyzeStepSize;
#endif
  int                 iImportRecursionLevel;
  int                 iLastAnalysisStage;
  int                 iLastRunModeStage;
  int                 iLogLevel;
  int                 iMatchLimit;
  int                 iMemoryMapEnable;
  int                 iPriority;
  int                 iRunMode;
  int                 iNextRunMode;
  int                 iTestLevel;
  int                 iTestRunMode;
  int                 iURLAuthType;
  int               (*piDevelopDigOutput)();
  int               (*piDevelopMapOutput)();
  int               (*piRunModeFinalStage)();
  APP_UI64            ui64AnalyzeByteCount;
  APP_UI64            ui64AnalyzeStartOffset;
  MASK_USS_MASK      *psFieldMask;
  SNAPSHOT_CONTEXT   *psSnapshotContext;
  SNAPSHOT_CONTEXT   *psBaselineContext;
#ifdef USE_SSL
  SSL_PROPERTIES     *psSslProperties;
#endif
  MD5_CONTEXT         sOutFileHashContext;
  time_t              tStartTime;
  struct timeval      tvJobEpoch;
  struct timeval      tvSRGEpoch;
  unsigned long       ulFileSizeLimit;
#ifdef USE_XMAGIC
  XMAGIC             *psXMagic;
#endif
  OPTIONS_CONTEXT    *psOptionsContext;
#ifdef USE_EMBEDDED_PERL
  PerlInterpreter    *psMyPerl;
#endif
#ifdef USE_EMBEDDED_PYTHON
  PyObject           *psPyGlobals;
  PyObject           *psPythonMain;
#endif
} FTIMES_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes (analyze.c)
 *
 ***********************************************************************
 */
int                 AnalyzeDoDig(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 AnalyzeDoMd5Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 AnalyzeDoSha1Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 AnalyzeDoSha256Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 AnalyzeDoXMagic(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError);
void                AnalyzeEnableDigEngine(FTIMES_PROPERTIES *psProperties);
void                AnalyzeEnableDigestEngine(FTIMES_PROPERTIES *psProperties);
int                 AnalyzeEnableXMagicEngine(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 AnalyzeFile(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError);
double              AnalyzeGetAnalysisTime(void);
int                 AnalyzeGetBlockSize(void);
APP_UI64            AnalyzeGetByteCount(void);
int                 AnalyzeGetCarrySize(void);
double              AnalyzeGetDps(void);
unsigned char      *AnalyzeGetDigSaveBuffer(int iCarrySize, char *pcError);
APP_UI32            AnalyzeGetFileCount(void);
APP_UI64            AnalyzeGetStartOffset(void);
#ifdef USE_XMAGIC
int                 AnalyzeGetStepSize(void);
#endif
unsigned char      *AnalyzeGetWorkBuffer(int iBlockSize, char *pcError);
void               *AnalyzeMapMemory(int iMemoryMapSize);
void                AnalyzeSetBlockSize(int iBlockSize);
void                AnalyzeSetCarrySize(int iCarrySize);
#ifdef USE_XMAGIC
void                AnalyzeSetStepSize(int iStepSize);
#endif
void                AnalyzeThrottleDps(APP_UI64 ui64Bytes, int iMaxDps);
void                AnalyzeUnmapMemory(void *pvMemoryMap, int iMemoryMapSize);

/*-
 ***********************************************************************
 *
 * Function Prototypes (compare.c)
 *
 ***********************************************************************
 */
int                 CompareDecodeLine(char *pcLine, SNAPSHOT_CONTEXT *psBaseline, char **ppcDecodeFields, char *pcError);
int                 CompareLoadBaselineData(SNAPSHOT_CONTEXT *psBaseline, char *pcError);
int                 CompareEnumerateChanges(SNAPSHOT_CONTEXT *psBaseline, SNAPSHOT_CONTEXT *psSnapshot, char *pcError);

/*-
 ***********************************************************************
 *
 * Function Prototypes (develop.c)
 *
 ***********************************************************************
 */
int                 DevelopHaveNothingOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 DevelopNoOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 DevelopNormalOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 DevelopCompressedOutput(FTIMES_PROPERTIES *psProperties, char *pcOutData, int *iWriteCount, FTIMES_FILE_DATA *psFTFileData, char *pcError);
int                 DevelopCompressHex(char *pcData, unsigned long ulHex, unsigned long ulOldHex);
int                 DevelopCountHexDigits(unsigned long ulHex);

/*-
 ***********************************************************************
 *
 * Function Prototypes (ftimes.c)
 *
 ***********************************************************************
 */
int                 main(int iArgumentCount, char *ppcArgumentVector[]);
int                 FTimesBootstrap(char *pcError);
int                 FTimesProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError);
int                 FTimesFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 FTimesStagesLoop(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 FTimesFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
void                FTimesDisplayStatistics(FTIMES_PROPERTIES *psProperties);
int                 FTimesOptionHandler(OPTIONS_TABLE *psOption, char *pcValue, FTIMES_PROPERTIES *psProperties, char *pcError);

void                FTimesUsage(void);
void                FTimesVersion(void);
void                FTimesSetPropertiesReference(FTIMES_PROPERTIES *psProperties);
long                FTimesGetEpoch(struct timeval *tvEpoch);
FTIMES_PROPERTIES  *FTimesGetPropertiesReference(void);
FTIMES_PROPERTIES  *FTimesNewProperties(char *pcError);
void                FTimesEraseFiles(FTIMES_PROPERTIES *psProperties, char *pcError);
#ifdef USE_SSL
int                 SSLCheckDependencies(SSL_PROPERTIES *psProperties, char *pcError);
#endif
void                FTimesFreeProperties(FTIMES_PROPERTIES *psProperties);
char               *FTimesGetEnvValue(char *pcName);
char               *MakeNonce(char *pcError);
int                 SeedRandom(unsigned long ulTime1, unsigned long ulTime2, char *pcError);

/*-
 ***********************************************************************
 *
 * Function Prototypes (xxxmode.c)
 *
 ***********************************************************************
 */
int                 CmpModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError);

int                 CmpModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError);

int                 CmpModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError);

int                 CmpModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError);

int                 CmpModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError);

int                 CmpModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DecoderFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 DigModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 GetModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MadModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError);

/*-
 ***********************************************************************
 *
 * Function Prototypes (dig.c)
 *
 ***********************************************************************
 */
int                 DigDevelopOutput(FTIMES_PROPERTIES *psProperties, DIG_SEARCH_DATA *psSearchData, char *pcError);
void                DigSetPropertiesReference(FTIMES_PROPERTIES *psProperties);
int                 DigWriteHeader(FTIMES_PROPERTIES *psProperties, char *pcError);

/*-
 ***********************************************************************
 *
 * Function Prototypes (hook.c)
 *
 ***********************************************************************
 */
#ifdef USE_FILE_HOOKS
HOOK_LIST          *HookMatchHook(HOOK_LIST *psHookList, FTIMES_FILE_DATA *psFTFileData);
#endif

/*-
 ***********************************************************************
 *
 * Function Prototypes (map.c)
 *
 ***********************************************************************
 */
#ifdef USE_EMBEDDED_PYTHON
void                MapFreePythonArguments(size_t szArgumentCount, wchar_t **ppwcArgumentVector);
wchar_t           **MapConvertPythonArguments(size_t tArgumentCount, char **ppcArgumentVector);
#endif
void                MapDirHashAlpha(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData);
void                MapDirHashCycle(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData, FTIMES_FILE_DATA *psFTFileData);
void                MapDirHashOmega(FTIMES_PROPERTIES *psProperties, FTIMES_HASH_DATA *psFTHashData, FTIMES_FILE_DATA *psFTFileData);
char               *MapDirname(char *pcPath);
#ifdef USE_FILE_HOOKS
int                 MapExecuteHook(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError);
#endif
#ifdef USE_EMBEDDED_PYTHON
int                 MapExecutePythonScript(FTIMES_PROPERTIES *psProperties, HOOK_LIST *psHook, KLEL_COMMAND *psCommand, FTIMES_FILE_DATA *psFTFileData, char *pcMessage);
#endif
int                 MapFile(FTIMES_PROPERTIES *psProperties, char *pcPath, char *pcError);
void                MapFreeFTFileData(FTIMES_FILE_DATA *psFTFileData);
unsigned long       MapGetAttributes(FTIMES_FILE_DATA *psFTFileData);
int                 MapGetDirectoryCount();
int                 MapGetFileCount();
int                 MapGetIncompleteRecordCount();
int                 MapGetRecordCount();
int                 MapGetSpecialCount();
#ifndef WINNT
FTIMES_FILE_DATA   *MapNewFTFileData(FTIMES_FILE_DATA *psParentFTFileData, char *pcName, char *pcError);
#endif
int                 MapTree(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psParentFTData, char *pcError);
int                 MapWriteHeader(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 MapWriteRecord(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError);
#ifdef WINNT
int                 MapCountNamedStreams(HANDLE hFile, int *piStreamCount, unsigned char **ppucStreamInfo, char *pcError);
HANDLE              MapGetFileHandleW(wchar_t *pwcPath);
int                 MapGetStreamCount();
FTIMES_FILE_DATA   *MapNewFTFileDataW(FTIMES_FILE_DATA *psParentFTFileData, wchar_t *pwcName, char *pcError);
void                MapStream(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, FTIMES_HASH_DATA *psDirFTHashData, char *pcError);
wchar_t            *MapUtf8ToWide(char *pcString, int iUtf8Size, char *pcError);
char               *MapWideToUtf8(wchar_t *pwcString, int iWideSize, char *pcError);
#endif

/*-
 ***********************************************************************
 *
 * Function Prototypes (properties.c)
 *
 ***********************************************************************
 */
void                PropertiesDisplaySettings(FTIMES_PROPERTIES *psProperties);
int                 PropertiesReadFile(char *pcFilename, FTIMES_PROPERTIES *psProperties, char *pcError);
int                 PropertiesReadLine(char *pcLine, FTIMES_PROPERTIES *psProperties, char *pcError);
int                 PropertiesTestFile(FTIMES_PROPERTIES *psProperties, char *pcError);

/*-
 ***********************************************************************
 *
 * Function Prototypes (support.c)
 *
 ***********************************************************************
 */
FILE_LIST          *SupportAddListItem(FILE_LIST *psItem, FILE_LIST *psHead, char *pcError);
int                 SupportAddToList(char *pcPath, FILE_LIST **ppList, char *pcListName, char *pcError);
#ifdef WIN32
BOOL                SupportAdjustPrivileges(LPCTSTR lpcPrivilege);
#endif
int                 SupportCheckList(FILE_LIST *psHead, char *pcListName, char *pcError);
int                 SupportChopEOLs(char *pcLine, int iStrict, char *pcError);
void                SupportDisplayRunStatistics(FTIMES_PROPERTIES *psProperties);
FILE_LIST          *SupportDropListItem(FILE_LIST *psHead, FILE_LIST *psDrop);
int                 SupportEraseFile(char *pcName, char *pcError);
int                 SupportExpandDirectoryPath(char *pcPath, char *pcFullPath, int iFullPathSize, char *pcError);
int                 SupportExpandPath(char *pcPath, char *pcFullPath, int iFullPathSize, int iForceExpansion, char *pcError);
void                SupportFreeData(void *pcData);
void                SupportFreeListItem(FILE_LIST *psItem);
FILE               *SupportGetFileHandle(char *pcFile, char *pcError);
int                 SupportGetFileType(char *pcPath, char *pcError);
char               *SupportGetHostname(void);
char               *SupportGetSystemOS(void);
FILE_LIST          *SupportIncludeEverything(char *pcError);
int                 SupportMakeName(char *pcPath, char *pcBaseName, char *pcBaseNameSuffix, char *pcExtension, char *pcFilename, char *pcError);
FILE_LIST          *SupportMatchExclude(FILE_LIST *psHead, char *pcPath);
FILE_LIST          *SupportMatchSubTree(FILE_LIST *psHead, FILE_LIST *psTarget);
char               *SupportNeuterString(char *pcData, int iLength, char *pcError);
FILE_LIST          *SupportNewListItem(char *pcPath, int iType, char *pcError);
FILE_LIST          *SupportPruneList(FILE_LIST *psList, char *pcListName);
int                 SupportRequirePrivilege(char *pcError);
int                 SupportSetLogLevel(char *pcLevel, int *piLevel, char *pcError);
int                 SupportSetPriority(FTIMES_PROPERTIES *psProperties, char *pcError);
#ifdef WIN32
int                 SupportSetPrivileges(char *pcError);
#endif
int                 SupportStringToUInt64(char *pcData, APP_UI64 *pui64Value, char *pcError);
int                 SupportWriteData(FILE *pFile, char *pcData, int iLength, char *pcError);

#ifdef USE_PCRE
int                 SupportAddFilter(char *pcFilter, FILTER_LIST **psHead, char *pcError);
void                SupportFreeFilter(FILTER_LIST *psFilter);
FILTER_LIST        *SupportMatchFilter(FILTER_LIST *psFilterList, char *acPath);
FILTER_LIST        *SupportNewFilter(char *pcFilter, char *pcError);
#endif

/*-
 ***********************************************************************
 *
 * Function Prototypes (time.c)
 *
 ***********************************************************************
 */
int                 TimeGetTimeValue(struct timeval *psTimeValue);
double              TimeGetTimeValueAsDouble(void);
time_t              TimeGetTime(char *pcDate, char *pcTime, char *pcZone, char *pcDateTime);
#ifdef WIN32
int                 TimeFormatTime(FILETIME *psFileTime, char *pcTime);
int                 TimeFormatOutOfBandTime(FILETIME *psFileTime, char *pcTime);
#endif
#ifdef UNIX
int                 TimeFormatTime(time_t *pTimeValue, char *pcTime);
#endif

/*-
 ***********************************************************************
 *
 * Function Prototypes (url.c)
 *
 ***********************************************************************
 */
int                 URLGetRequest(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 URLPingRequest(FTIMES_PROPERTIES *psProperties, char *pcError);
int                 URLPutRequest(FTIMES_PROPERTIES *psProperties, char *pcError);

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#define PUTBIT(x, v, p) (x) = ((x) & ~(1 << (p))) | (((v) & 1)<< (p))
#define GETBIT(x, p) ((x) & (1 << (p))) >> (p)
#define MEMORY_FREE(pv) if (pv) { free(pv); }
#define RUN_MODE_IS_SET(mask, mode) (((mask) & (mode)) == (mode))

/*-
 ***********************************************************************
 *
 * External Variables
 *
 ***********************************************************************
 */
#ifdef WINNT
extern NQIF         NtdllNQIF;
#endif

#endif /* !_FTIMES_H_INCLUDED */
