/*-
 ***********************************************************************
 *
 * $Id: compare.h,v 1.20 2007/02/23 00:22:35 mavrik Exp $
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

#define CMP_MAX_LINE                      8192
#define CMP_MODULUS                     (1<<16)
#define CMP_HASH_MASK         ((CMP_MODULUS)-1)
#define CMP_NODE_REQUEST_COUNT          200000
#define CMP_SEPARATOR_C                     '|'
#define CMP_SEPARATOR_S                     "|"

/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _CMP_DATA
{
  char                cCategory;
  char               *pcRecord;
  unsigned long       ulChangedMask;
  unsigned long       ulUnknownMask;
} CMP_DATA;

typedef struct _CMP_NODE
{
  unsigned char       aucHash[MD5_HASH_SIZE];
  char               *pcData;
  int                 iFound;
  int                 iNextIndex;
} CMP_NODE;

typedef struct _CMP_PROPERTIES
{
  char                acNewLine[NEWLINE_LENGTH];
  CMP_NODE           *psBaselineNodes;
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
  CMP_NODE           *psSnapshotNodes;
#endif
  FILE               *pFileOut;
  int                 aiBaselineKeys[CMP_MODULUS];
#ifdef USE_SNAPSHOT_COLLISION_DETECTION
  int                 aiSnapshotKeys[CMP_MODULUS];
#endif
  MASK_USS_MASK      *psCompareMask;
  unsigned long       ulCompareMask;
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
//int               CompareDecodeLine(char *pcLine, SNAPSHOT_CONTEXT *psBaseline, char **ppcDecodeFields, char *pcError); /* This is declared in ftimes.h */
//int               CompareEnumerateChanges(SNAPSHOT_CONTEXT *psBaseline, SNAPSHOT_CONTEXT *psSnapshot, char *pcError); /* This is declared in ftimes.h */
void                CompareFreeNodeData(int *piKeys, CMP_NODE *psNodes);
void                CompareFreeProperties(CMP_PROPERTIES *psProperties);
int                 CompareGetChangedCount(void);
int                 CompareGetCrossedCount(void);
int                 CompareGetMissingCount(void);
int                 CompareGetNewCount(void);
int                *CompareGetNodeIndexReference(unsigned char *pucHash, int *piKeys, CMP_NODE *psNodes);
CMP_PROPERTIES     *CompareGetPropertiesReference(void);
int                 CompareGetRecordCount(void);
int                 CompareGetUnknownCount(void);
//int               CompareLoadBaselineData(SNAPSHOT_CONTEXT *psBaseline, char *pcError); /* This is declared in ftimes.h */
CMP_PROPERTIES     *CompareNewProperties(char *pcError);
void                CompareSetNewLine(char *pcNewLine);
void                CompareSetOutputStream(FILE *pFile);
void                CompareSetPropertiesReference(CMP_PROPERTIES *psProperties);
int                 CompareWriteHeader(FILE *pFile, char *pcNewLine, char *pcError);
int                 CompareWriteRecord(CMP_PROPERTIES *psProperties, CMP_DATA *psData, char *pcError);
