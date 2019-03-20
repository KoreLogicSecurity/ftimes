/*-
 ***********************************************************************
 *
 * $Id: decode.h,v 1.31 2019/03/14 16:07:42 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#ifndef _DECODE_H_INCLUDED
#define _DECODE_H_INCLUDED

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
#define DECODE_FIELD_COUNT 28 /* This value must be updated as new fields are added. */
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
  APP_UI32            dev;
  APP_UI32           *pdev;
  APP_UI32            inode;
  APP_UI32           *pinode;
  APP_UI32            volume;
  APP_UI32           *pvolume;
  APP_UI64            findex;
  APP_UI64           *pfindex;
  APP_UI32            mode;
  APP_UI32           *pmode;
  APP_UI32            attributes;
  APP_UI32           *pattributes;
  APP_UI32            nlink;
  APP_UI32           *pnlink;
  APP_UI32            uid;
  APP_UI32           *puid;
  APP_UI32            gid;
  APP_UI32           *pgid;
  APP_UI32            rdev;
  APP_UI32           *prdev;
  APP_UI32            atime;
  APP_UI32           *patime;
  APP_UI32            ams;
  APP_UI32           *pams;
  APP_UI32            mtime;
  APP_UI32           *pmtime;
  APP_UI32            mms;
  APP_UI32           *pmms;
  APP_UI32            ctime;
  APP_UI32           *pctime;
  APP_UI32            cms;
  APP_UI32           *pcms;
  APP_UI32            chtime;
  APP_UI32           *pchtime;
  APP_UI32            chms;
  APP_UI32           *pchms;
  APP_UI64            size;
  APP_UI64           *psize;
  APP_UI32            altstreams;
  APP_UI32           *paltstreams;
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
  int                 iNamesAreCaseInsensitive;
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
int                 Decode32BitHexToDecimal(char *pcData, int iLength, APP_UI32 *pui32ValueNew, APP_UI32 *pui32ValueOld, char *pcError);
int                 Decode64BitHexToDecimal(char *pcData, int iLength, APP_UI64 *pui64ValueNew, APP_UI64 *pui64ValueOld, char *pcError);
void                DecodeBuildFromBase64Table(void);
void                DecodeClearRecord(DECODE_RECORD *psRecord, int iFieldCount);
int                 DecodeFormatOutOfBandTime(char *pcToken, int iLength, char *pcOutput, char *pcError);
int                 DecodeFormatTime(APP_UI32 *pui32Time, char *pcTime);
void                DecodeFreeSnapshotContext(SNAPSHOT_CONTEXT *psSnapshot);
void                DecodeFreeSnapshotContext2(SNAPSHOT_CONTEXT *psSnapshot);
int                 DecodeGetBase64Hash(char *pcData, unsigned char *pucHash, int iLength, char *pcError);
int                 DecodeGetTableLength(void);
SNAPSHOT_CONTEXT   *DecodeNewSnapshotContext(char *pcError);
SNAPSHOT_CONTEXT   *DecodeNewSnapshotContext2(char *pcSnapshot, char *pcError);
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

#endif /* !_DECODE_H_INCLUDED */
