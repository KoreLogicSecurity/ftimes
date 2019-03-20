/*-
 ***********************************************************************
 *
 * $Id: ftimes-xpa.h,v 1.15 2019/03/14 16:07:44 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2009-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _FTIMES_XPA_H_INCLUDED
#define _FTIMES_XPA_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define PROGRAM_NAME "ftimes-xpa"
#define VERSION "1.2.1"
#define FTIMES_XPA_VERSION_0_1 0x00000001 /* Major.Minor */
#define FTIMES_XPA_VERSION_0_2 0x00000002 /* Major.Minor */
#define FTIMES_XPA_VERSION_0_3 0x00000003 /* Major.Minor */
#define FTIMES_XPA_VERSION FTIMES_XPA_VERSION_0_3

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

#define FTIMES_XPA_EXTENDED_PATH_PREFIX "\\\\?\\"

#define FTIMES_XPA_CHUNK_TYPE_NULL 0x58504100
#define FTIMES_XPA_CHUNK_TYPE_HEAD 0x58504101
#define FTIMES_XPA_CHUNK_TYPE_HKVP 0x58504102
#define FTIMES_XPA_CHUNK_TYPE_DATA 0x58504103
#define FTIMES_XPA_CHUNK_TYPE_DKVP 0x58504104
#define FTIMES_XPA_CHUNK_TYPE_JOIN 0x58504105
#define FTIMES_XPA_CHUNK_TYPE_TKVP 0x585041fe
#define FTIMES_XPA_CHUNK_TYPE_TAIL 0x585041ff

#define FTIMES_XPA_KEY_ID_NAME          0
#define FTIMES_XPA_KEY_ID_MD5           1
#define FTIMES_XPA_KEY_ID_SHA1          2
#define FTIMES_XPA_KEY_ID_DATA_SIZE     3
#define FTIMES_XPA_KEY_ID_FILE_REAL_SIZE 4
#define FTIMES_XPA_KEY_ID_FILE_READ_SIZE 5
#define FTIMES_XPA_KEY_ID_HEAD_FLAGS    6
#define FTIMES_XPA_KEY_ID_TAIL_FLAGS    7
#define FTIMES_XPA_KEY_ID_FILE_ATIME_RTUE    8 /* RTUE --> Relative To Unix Epoch */
#define FTIMES_XPA_KEY_ID_FILE_MTIME_RTUE    9 /* RTUE --> Relative To Unix Epoch */
#define FTIMES_XPA_KEY_ID_FILE_CTIME_RTUE   10 /* RTUE --> Relative To Unix Epoch */
#define FTIMES_XPA_KEY_ID_FILE_UID          11
#define FTIMES_XPA_KEY_ID_FILE_GID          12
#define FTIMES_XPA_KEY_ID_FILE_OSID         13
#define FTIMES_XPA_KEY_ID_FILE_GSID         14
#define FTIMES_XPA_KEY_ID_FILE_OWNER        15
#define FTIMES_XPA_KEY_ID_FILE_GROUP        16
#define FTIMES_XPA_KEY_ID_PATH_SEPARATOR    17

#define FTIMES_XPA_MEMBER_HEAD_FLAG_OPEN_ERROR  0x00010000
#define FTIMES_XPA_MEMBER_HEAD_FLAG_ATTR_ERROR  0x00020000

#define FTIMES_XPA_MEMBER_TAIL_FLAG_READ_ERROR  0x00010000

#define FTIMES_XPA_KIVL_SIZE 4 /* KIVL --> Key ID / Value Length */
#define FTIMES_XPA_MAX_VALUE_LENGTH 1048576

#define FTIMES_XPA_ARCHIVE_MODE 0x00000001

#define FTIMES_XPA_DEFAULT_BLOCKSIZE 32768
#define FTIMES_XPA_MIN_BLOCKSIZE 1
#define FTIMES_XPA_MAX_BLOCKSIZE 0x10000000 /* 2^28 = 268435456 */

#define FTIMES_XPA_OWNER_NAME_SIZE 256

#define FTIMES_XPA_MAX_DECODED_PATH 4096
#define FTIMES_XPA_MAX_ENCODED_PATH (((FTIMES_XPA_MAX_DECODED_PATH)*3)+7) /* The 7 is for the "file://" prefix if present. */
#define FTIMES_XPA_ENCODED_PREFIX "file://"
#define FTIMES_XPA_ENCODED_PREFIX_LENGTH 7
#ifdef WIN32
#define FTIMES_XPA_PATH_SEPARATOR "\\"
#else
#define FTIMES_XPA_PATH_SEPARATOR "/"
#endif
#define FTIMES_XPA_PATH_SEPARATOR_LENGTH 1

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _FTIMES_XPA_HANDLE
{
  char               *pcDecodedName;
  FILE               *pFile;
#ifdef WIN32
  char               *pcFileA;
  HANDLE              hFile;
  int                 iFile;
  wchar_t            *pcFileW;
#endif
} FTIMES_XPA_HANDLE;

typedef struct _FTIMES_XPA_HEADER
{
  APP_UI32            ui32Magic;
  APP_UI32            ui32Version;
  APP_UI32            ui32ThisChunkSize;
  APP_UI32            ui32LastChunkSize;
  APP_UI32            ui32ChunkId;
} FTIMES_XPA_HEADER;
#define FTIMES_XPA_HEADER_SIZE_0_1 20
#define FTIMES_XPA_HEADER_SIZE_0_2 20
#define FTIMES_XPA_HEADER_SIZE FTIMES_XPA_HEADER_SIZE_0_2

typedef enum _FTIMES_XPA_OPTION_IDS
{
  OPT_Blocksize,
  OPT_ListFile,
} FTIMES_XPA_OPTION_IDS;

typedef struct _FTIMES_XPA_PROPERTIES
{
  char               *pcListFile;
  int                 iRunMode;
  APP_UI32            ui32Blocksize;
  OPTIONS_CONTEXT    *psOptionsContext;
  unsigned char      *pucData;
} FTIMES_XPA_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
void                FTimesXpaAbort();
void                FTimesXpaAddMember(char *pcDecodedName, unsigned char *pucData, APP_UI32 ui32Blocksize, char *pcError);
#ifdef WINNT
BOOL                FTimesXpaAdjustPrivileges(LPCTSTR lpcPrivilege);
#endif
int                 FTimesXpaBootStrap(char *pcError);
char               *FTimesXpaDecodeString(char *pcEncoded, char *pcError);
char               *FTimesXpaEncodeString(char *pcData, int iLength, char *pcError);
#ifdef WINNT
void                FTimesXpaFormatWinxError(DWORD dwError, TCHAR **pptcMessage);
#endif
void                FTimesXpaFreeHandle(FTIMES_XPA_HANDLE *psHandle);
void                FTimesXpaFreeProperties(FTIMES_XPA_PROPERTIES *psProperties);
FTIMES_XPA_HANDLE  *FTimesXpaGetHandle(char *pcDecodedName, char *pcError);
FTIMES_XPA_PROPERTIES *FTimesXpaGetPropertiesReference(void);
FTIMES_XPA_PROPERTIES *FTimesXpaNewProperties(char *ptcError);
int                 FTimesXpaOptionHandler(OPTIONS_TABLE *psOption, char *pcValue, FTIMES_XPA_PROPERTIES *psProperties, char *pcError);
int                 FTimesXpaProcessArguments(int iArgumentCount, char *ppcArgumentVector[], FTIMES_XPA_PROPERTIES *psProperties, char *pcError);
#ifdef WINNT
int                 FTimesXpaSetPrivileges(char *pcError);
#endif
void                FTimesXpaSetPropertiesReference(FTIMES_XPA_PROPERTIES *psProperties);
void                FTimesXpaUsage(void);
wchar_t            *FTimesXpaUtf8ToWide(char *pcString, int iUtf8Size, char *pcError);
void                FTimesXpaVersion(void);
int                 FTimesXpaWriteData(FILE *pFile, unsigned char *pucData, int iLength, char *pcError);
int                 FTimesXpaWriteHeader(FILE *pFile, FTIMES_XPA_HEADER *psFileHeader, char *pcError);
int                 FTimesXpaWriteKvp(FILE *pFile, int iKeyId, void *pvValue, int iValueLength, char *pcError);

#endif /* !_FTIMES_XPA_H_INCLUDED */
