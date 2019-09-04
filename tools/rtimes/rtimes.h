/*-
 ***********************************************************************
 *
 * $Id: rtimes.h,v 1.25 2019/04/23 14:54:03 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2008-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _RTIMES_H_INCLUDED
#define _RTIMES_H_INCLUDED

/*-
 ***********************************************************************
 *
 * Defines.
 *
 ***********************************************************************
 */
#define PROGRAM "rtimes"
#define VERSION "1.0.11"

#define ER    -1
#define ER_OK  0

#define XER_OK                0
#define XER_Usage             1
#define XER_Abort             2
#define XER_BootStrap         3
#define XER_ProcessArguments  4
#define XER_WorkHorse         5

#define RTIMES_MAP_MODE 0x00000001

#define RTIMES_MIN_DATA_SIZE          1
#define RTIMES_MAX_DATA_SIZE 1073741824 /* 1 GB */

#define MESSAGE_SIZE (1024 * sizeof(TCHAR))

#define NEUTER_ENCODING_RTIMES 0
#define NEUTER_ENCODING_FULL 1
#define RTIMES_ERROR_LENGTH 512
#define RTIMES_TIME_FORMAT _T("%04d-%02d-%02d %02d:%02d:%02d.%03d")
#define RTIMES_TIME_FORMAT_LENGTH 24

#define RTIMES_DEFAULT_CLASS_STRING_SIZE (512 * sizeof(TCHAR))
#define RTIMES_DEFAULT_DATA_SIZE 1024
#define RTIMES_DEFAULT_NAME_SIZE 1024

#define RTIMES_KEYNAME_SIZE ((255 + 1) * 2) /* Microsoft states that 255 "characters" is the limit. We add 1 (for the terminator) and multiply by 2 (to handle Unicode). */

#define RTIMES_MAX_DO_OVERS 3
#define RTIMES_TYPE_SIZE (32 * sizeof(TCHAR))

#define RTIMES_FLAG_KEY                0x00000001
#define RTIMES_FLAG_VALUE              0x00000002
#define RTIMES_FLAG_DEFAULT_VALUE      0x00000004
#define RTIMES_FLAG_REQUESTED_TRUNCATE 0x00000008
#define RTIMES_FLAG_MANDATORY_TRUNCATE 0x00000010
#define RTIMES_FLAG_OWNER_MISSING      0x00000020
#define RTIMES_FLAG_GROUP_MISSING      0x00000040
#define RTIMES_FLAG_CLASS_MISSING      0x00000080
#define RTIMES_FLAG_WTIME_MISSING      0x00000100
#define RTIMES_FLAG_DATA_MISSING       0x00000200
#define RTIMES_FLAG_MANDATORY_NAME_TRUNCATE 0x00000400
#define RTIMES_FLAG_OWNER_INVALID      0x00000800
#define RTIMES_FLAG_DACL_INVALID       0x00001000
#define RTIMES_FLAG_DACL_MISSING       0x00002000
#define RTIMES_FLAG_DACL_OMITTED       0x00004000
#define RTIMES_FLAG_GROUP_INVALID      0x00008000

/*-
 ***********************************************************************
 *
 * Typedefs.
 *
 ***********************************************************************
 */
typedef enum _RTIMES_OPTION_IDS
{
  OPT_MaxData,
} RTIMES_OPTION_IDS;

typedef struct _RTIMES_PROPERTIES
{
  int                 iRunMode;
  DWORD               dwMaxData;
} RTIMES_PROPERTIES;

typedef struct _RTIMES_TODO_LIST
{
  HKEY                dwHive;
  TCHAR               atcHive[(256 * sizeof(TCHAR))];
  TCHAR               atcKey[(256 * sizeof(TCHAR))];
} RTIMES_TODO_LIST;

/*-
 ***********************************************************************
 *
 * Global Variables.
 *
 ***********************************************************************
 */

/*-
 ***********************************************************************
 *
 * Function Prototypes.
 *
 ***********************************************************************
 */
int                 main(int iArgumentCount, TCHAR *pptcArgumentVector[]);
void                FormatWinxError(DWORD dwError, TCHAR **pptcMessage);
int                 RTimesBootStrap(TCHAR *ptcError);
int                 RTimesEnumerateKeys(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError);
int                 RTimesEnumerateValues(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError);
RTIMES_PROPERTIES  *RTimesGetPropertiesReference(void);
int                 RTimesMapKey(FILE *pFile, HKEY hKey, TCHAR *ptcPath, TCHAR *ptcError);
int                 RTimesMd5ToHex(unsigned char *pucMd5, TCHAR *ptcHexHash);
TCHAR              *RTimesNeuterEncodeData(unsigned char *pucData, int iLength, int iOptions, TCHAR *ptcError);
TCHAR              *RTimesNewPath(TCHAR *ptcPath, TCHAR *ptcName, TCHAR *ptcDelimiter, TCHAR *ptcError);
RTIMES_PROPERTIES  *RTimesNewProperties(TCHAR *ptcError);
int                 RTimesOptionHandler(void *pvOption, TCHAR *ptcValue, void *pvProperties, TCHAR *ptcError);
int                 RTimesProcessArguments(int iArgumentCount, TCHAR *pptcArgumentVector[], RTIMES_PROPERTIES *psProperties, TCHAR *ptcError);
void                RTimesSetPropertiesReference(RTIMES_PROPERTIES *psProperties);
TCHAR              *RTimesSidToString(SID *psSid, TCHAR *ptcError);
void                RTimesUsage(void);
void                RTimesVersion(void);
int                 RTimesWorkHorse(RTIMES_PROPERTIES *psProperties, TCHAR *ptcError);
void                RTimesWriteValueRecord(FILE *pFile, DWORD dwFlags, DWORD dwType, BYTE *pbyteData, DWORD dwDataSize, TCHAR *ptcPath, TCHAR *ptcName);
int                 RTimesWTimeToString(FILETIME *pFileTime, TCHAR *ptcTime);

#endif /* !_RTIMES_H_INCLUDED */
