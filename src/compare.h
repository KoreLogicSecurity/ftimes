/*-
 ***********************************************************************
 *
 * $Id: compare.h,v 1.8 2005/04/02 18:08:24 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2005 Klayton Monroe, All Rights Reserved.
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

#define CMP_HASHC_SIZE                      32
#define CMP_HASHB_SIZE                      16
#define CMP_MAX_LINE                      8192
#define CMP_MODULUS                     (1<<16)
#define CMP_HASH_MASK         ((CMP_MODULUS)-1)
#define CMP_NODE_REQUEST_COUNT          200000
#define CMP_SEPARATOR_C                     '|'
#define CMP_SEPARATOR_S                     "|"

#define ALL_FIELDS_NAME_SIZE                32
#define ALL_FIELDS_TABLE_LENGTH             23
#define ALL_FIELDS_MASK_SIZE ((ALL_FIELDS_TABLE_LENGTH) * (ALL_FIELDS_NAME_SIZE))

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _ALL_FIELDS_TABLE
{
  char                acName[ALL_FIELDS_NAME_SIZE];
  int                 iBitsToSet;
} ALL_FIELDS_TABLE;

typedef struct _CMP_DATA
{
  char                cCategory;
  char               *pcRecord;
  unsigned long       ulChangedMask;
  unsigned long       ulUnknownMask;
} CMP_DATA;

typedef struct _CMP_NODE
{
  unsigned char       aucHash[16];
  char               *pcData;
  int                 iFound;
  int                 iNextIndex;
} CMP_NODE;

typedef struct _CMP_PROPERTIES
{
  char                acNewLine[NEWLINE_LENGTH];
  char               *pcPackedData;
  CMP_NODE           *psBaselineNodes;
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
  CMP_NODE           *psSnapshotNodes;
#endif
  FILE               *pFileOut;
  int                 aiBaselineKeys[CMP_MODULUS];
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
  int                 aiSnapshotKeys[CMP_MODULUS];
#endif
  int                 iFields;
  int               (*piCompareRoutine)();
  unsigned long       ulCompareMask;
  unsigned long       ulFieldsMask;
  unsigned long       ulAnalyzed;
  unsigned long       ulChanged;
  unsigned long       ulMissing;
  unsigned long       ulNew;
  unsigned long       ulUnknown;
  unsigned long       ulCrossed;
} CMP_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define CMP_GET_NODE_INDEX(aucHash) (((aucHash[13] << 16) | (aucHash[14] << 8) | aucHash[15]) & (CMP_HASH_MASK))

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 CompareDecodeLine(char *pcLine, unsigned long ulFieldsMask, char aacDecodeFields[][CMP_MAX_LINE], char *pcError);
int                 CompareEnumerateChanges(char *pcFilename, char *pcError);
void                CompareFreeProperties(CMP_PROPERTIES *psProperties);
int                 CompareGetChangedCount(void);
int                 CompareGetCrossedCount(void);
int                 CompareGetMissingCount(void);
int                 CompareGetNewCount(void);
int                *CompareGetNodeIndexReference(unsigned char *pucHash, int *piKeys, CMP_NODE *psNodes);
CMP_PROPERTIES     *CompareGetPropertiesReference(void);
int                 CompareGetRecordCount(void);
int                 CompareGetUnknownCount(void);
int                 CompareLoadBaselineData(char *pcFilename, char *pcError);
CMP_PROPERTIES     *CompareNewProperties(char *pcError);
/* This is declared in ftimes.h.
int                 CompareParseStringMask(char *pcMask, unsigned long *ulMask, int iRunMode, MASK_TABLE *psMaskTable, int iMaskTableLength, char *pcError);
*/
int                 ComparePreprocessLine(FILE *pFile, int iToLower, char *pcLine, int *piLength, unsigned char *pucHash, char *pcError);
int                 CompareReadHeader(FILE *pFile, char *pcError);
void                CompareSetMask(unsigned long ulMask);
void                CompareSetNewLine(char *pcNewLine);
void                CompareSetOutputStream(FILE *pFile);
void                CompareSetPropertiesReference(CMP_PROPERTIES *psProperties);
int                 CompareWriteHeader(FILE *pFile, char *pcNewLine, char *pcError);
int                 CompareWriteRecord(CMP_PROPERTIES *psProperties, CMP_DATA *psData, char *pcError);
