/*-
 ***********************************************************************
 *
 * $Id: dig.h,v 1.2 2003/02/23 17:40:08 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
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
#define DIG_MAX_STRING_SIZE 1024


/*-
 ***********************************************************************
 *
 * Typedefs
 *
 ***********************************************************************
 */
typedef struct _DIG_SEARCH_DATA
{
  char               *pcFile;
  char               *pcString;
  K_UINT64            ui64Offset;
} DIG_SEARCH_DATA;

typedef struct _DIG_SEARCH_LIST
{
  char                cEscString[DIG_MAX_STRING_SIZE];
  char                cRawString[DIG_MAX_STRING_SIZE];
  int                 iLength,
                      iHitsPerFile,
                      iHits;
  struct _DIG_SEARCH_LIST *pNext;
} DIG_SEARCH_LIST;


/*-
 ***********************************************************************
 *
 * Function Prototypes
 *
 ***********************************************************************
 */
int                 DigAddString(char *pcString, char *pcError);
void                DigClearCounts(void);
int                 DigDevelopOutput(DIG_SEARCH_DATA *pFTStringData, char *pcError);
int                 DigGetMatchLimit(void);
int                 DigGetMaxStringLength(void);
DIG_SEARCH_LIST    *DigGetSearchList(int iIndex);
int                 DigGetStringCount(void);
int                 DigGetStringsMatched(void);
K_UINT64            DigGetTotalMatches(void);
int                 DigSearchData(unsigned char *pucData, int iDataLength, int iStopShort, K_UINT64 ui64AbsoluteOffset, char *pcFilename, char *pcError);
void                DigSetHashBlock(struct hash_block *pHashBlock);
void                DigSetMatchLimit(int iMatchLimit);
void                DigSetNewLine(char *pcNewLine);
void                DigSetOutputStream(FILE *pFile);
int                 DigWriteHeader(FILE *pFile, char *pcNewLine, char *pcError);
