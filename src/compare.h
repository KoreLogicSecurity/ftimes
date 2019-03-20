/*
 ***********************************************************************
 *
 * $Id: compare.h,v 1.1.1.1 2002/01/18 03:17:19 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
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
#define CMP_FIELD_SEPARATOR                 "|"
#define CMP_DEPLETED                         1
#define CMP_MAX_LINE_LENGTH               2000
#define CMP_NAME_LENGTH                    512
#define CMP_NEXT_NAME_SIZE                   4
#define CMP_NEXT_HASH_SIZE                   4
#define CMP_FOUND_SIZE                       1
#define CMP_HASHC_SIZE                      32
#define CMP_HASHB_SIZE                      16
#define CMP_HASH_MASK ((1<<(CMP_HASHB_SIZE))-1)
#define CMP_ARRAY_SIZE                  (1<<26)

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
  char                cName[ALL_FIELDS_NAME_SIZE];
  int                 iBitsToSet;
} ALL_FIELDS_TABLE;

typedef struct _CMP_FIELD_VALUE_TABLE
{
  char                cValue[CMP_MAX_LINE_LENGTH];
} CMP_FIELD_VALUE_TABLE;

typedef struct _CMP_PROPERTIES
{
  FILE               *pFileOut;
  int               (*piCompareRoutine)();
  int                 iFields;
  unsigned long       ulCompareMask;
  unsigned long       ulFieldsMask;
  unsigned long       ulChanged;
  unsigned long       ulFound;
  unsigned long       ulMissing;
  unsigned long       ulNew;
  unsigned long       ulUnknown;
} CMP_PROPERTIES;

/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 CompareCreateDatabase(char *pcFilename, char *pcError);
int                 CompareDecodeHeader(char *pcLine, char *pcError);
int                 CompareDecodeLine(char *pcLine, CMP_FIELD_VALUE_TABLE *decodeFields, char *pcError);
int                 CompareEnumerateChanges(char *pcFilename, char *pcError);
void                CompareFreeDatabase(void);
int                 CompareGetChangedCount(void);
int                 CompareGetMissingCount(void);
int                 CompareGetNewCount(void);
int                 CompareGetRecordCount(void);
int                 CompareGetUnknownCount(void);
/* int              CompareParseStringMask(char *pcMask, unsigned long *ulMask, int iRunMode, MASK_TABLE *pMaskTable, int iMaskTableLength, char *pcError); */
void                CompareSetMask(unsigned long ulMask);
void                CompareSetNewLine(char *pcNewLine);
void                CompareSetOutputStream(FILE *pFile);
int                 CompareWriteHeader(FILE *pFile, char *pcError);
int                 CompareWriteRecord(char cCategory, char *pcName, unsigned long ulChangedMask, unsigned long ulUnknownMask, char *pcError);
