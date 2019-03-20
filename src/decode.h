/*-
 ***********************************************************************
 *
 * $Id: decode.h,v 1.15 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */

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

#ifndef NEWLINE_LENGTH 
#define NEWLINE_LENGTH 3
#endif

#define DECODE_CHECKPOINT_LENGTH 2
#define DECODE_CHECKPOINT_STRING "00"
#define DECODE_FIELDNAME_SIZE 32
#define DECODE_FIELD_COUNT 25
#ifndef FTIMES_MAX_LINE
#define DECODE_MAX_LINE 8192
#else
#define DECODE_MAX_LINE FTIMES_MAX_LINE
#endif
#ifndef FTIMES_MAX_PATH
#define DECODE_MAX_PATH 4096
#else
#define DECODE_MAX_PATH FTIMES_MAX_PATH
#endif
#define DECODE_SEPARATOR_C '|'
#define DECODE_SEPARATOR_S "|"
#define DECODE_TIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d"
#define DECODE_TIME_FORMAT_SIZE 20

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define DECODE_DEFINE_PREV_NUMBER_VALUE(field, pfield, value) field = value; pfield = &field;
#define DECODE_UNDEFINE_PREV_NUMBER_VALUE(field, pfield) field = 0; pfield = NULL;

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _DECODE_TABLE
{
  char                acZName[DECODE_FIELDNAME_SIZE];
  char                acUName[DECODE_FIELDNAME_SIZE];
  int               (*piRoutine) ();
} DECODE_TABLE;

typedef struct _DECODE_RECORD
{
  char                acLine[DECODE_MAX_LINE];
  char              **ppcFields;
  int                 iLineLength;
  unsigned char       aucHash[MD5_HASH_SIZE];
} DECODE_RECORD;

typedef struct _DECODE_STATE
{
  char                name[DECODE_MAX_PATH];
  K_UINT32            dev;
  K_UINT32           *pdev;
  K_UINT32            inode;
  K_UINT32           *pinode;
  K_UINT32            volume;
  K_UINT32           *pvolume;
  K_UINT64            findex;
  K_UINT64           *pfindex;
  K_UINT32            mode;
  K_UINT32           *pmode;
  K_UINT32            attributes;
  K_UINT32           *pattributes;
  K_UINT32            nlink;
  K_UINT32           *pnlink;
  K_UINT32            uid;
  K_UINT32           *puid;
  K_UINT32            gid;
  K_UINT32           *pgid;
  K_UINT32            rdev;
  K_UINT32           *prdev;
  K_UINT32            atime;
  K_UINT32           *patime;
  K_UINT32            ams;
  K_UINT32           *pams;
  K_UINT32            mtime;
  K_UINT32           *pmtime;
  K_UINT32            mms;
  K_UINT32           *pmms;
  K_UINT32            ctime;
  K_UINT32           *pctime;
  K_UINT32            cms;
  K_UINT32           *pcms;
  K_UINT32            chtime;
  K_UINT32           *pchtime;
  K_UINT32            chms;
  K_UINT32           *pchms;
  K_UINT64            size;
  K_UINT64           *psize;
  K_UINT32            altstreams;
  K_UINT32           *paltstreams;
} DECODE_STATE;

typedef struct _DECODE_STATS
{
  unsigned long       ulAnalyzed;
  unsigned long       ulDecoded;
  unsigned long       ulSkipped;
} DECODE_STATS;

typedef struct _SNAPSHOT_CONTEXT
{
  char               *pcFile;
#define DECODE_RECORD_COUNT 2
  DECODE_RECORD       asRecords[DECODE_RECORD_COUNT];
  DECODE_RECORD      *psCurrRecord;
  DECODE_RECORD      *psPrevRecord;
  DECODE_STATE        sDecodeState;
  DECODE_STATS        sDecodeStats;
  DECODE_TABLE       *psDecodeMap;
  FILE               *pFile;
  int                 iCompressed;
  int                 aiIndex2Map[DECODE_FIELD_COUNT];
  int                 iFieldCount;
  int                 iLegacyFile;
  int                 iLineNumber;
  int                 iSkipToNext;
  unsigned long       ulFieldMask;
} SNAPSHOT_CONTEXT;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 Decode32BitHexToDecimal(char *pcData, int iLength, K_UINT32 *pui32ValueNew, K_UINT32 *pui32ValueOld, char *pcError);
int                 Decode64BitHexToDecimal(char *pcData, int iLength, K_UINT64 *pui64ValueNew, K_UINT64 *pui64ValueOld, char *pcError);
void                DecodeBuildFromBase64Table(void);
void                DecodeClearRecord(DECODE_RECORD *psRecord, int iFieldCount);
int                 DecodeFormatOutOfBandTime(char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeFormatTime(K_UINT32 *pui32Time, char *pcTime);
void                DecodeFreeSnapshotContext(SNAPSHOT_CONTEXT *psSnapshot);
int                 DecodeGetBase64Hash(char *pcData, unsigned char *pucHash, int iLength, char *pcError);
int                 DecodeGetTableLength(void);
SNAPSHOT_CONTEXT   *DecodeNewSnapshotContext(char *pcError);
int                 DecodeOpenSnapshot(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
int                 DecodeParseHeader(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
int                 DecodeParseRecord(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
int                 DecodeProcessATime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessATimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessAlternateDataStreams(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessAttributes(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessCTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessCTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessChTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessChTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessDevice(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessFileIndex(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessGroupId(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessInode(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessLinkCount(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessMTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessMTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessMagic(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessMd5(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessMode(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessNada(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessName(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessRDevice(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessSha1(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessSha256(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessSize(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessUserId(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeProcessVolume(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError);
char               *DecodeReadLine(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
int                 DecodeReadSnapshot(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
void                DecodeSetNewLine(char *pcNewLine);
void                DecodeSetOutputStream(FILE *pFile);
int                 DecodeWriteHeader(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
int                 DecodeWriteRecord(SNAPSHOT_CONTEXT *psSnapshot, char *pcError);
