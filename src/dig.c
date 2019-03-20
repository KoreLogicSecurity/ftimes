/*-
 ***********************************************************************
 *
 * $Id: dig.c,v 1.12 2004/04/25 15:22:57 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2004 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static char           gacNewLine[NEWLINE_LENGTH] = CRLF;
#endif
#ifdef UNIX
static char           gacNewLine[NEWLINE_LENGTH] = LF;
#endif
static DIG_SEARCH_LIST *gppsSearchList[256];
static FILE          *gpFile;
static int            giMatchLimit;
static int            giMaxStringLength;
static int            giStringCount;
static MD5_CONTEXT   *gpsMD5Context;


/*-
 ***********************************************************************
 *
 * DigAddString
 *
 ***********************************************************************
 */
int
DigAddString(char *pcString, char *pcError)
{
  const char          acRoutine[] = "DigAddString()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcUnEscaped;
  int                 iLength;
  DIG_SEARCH_LIST    *pListNew;
  DIG_SEARCH_LIST    *pListCurrent;
  DIG_SEARCH_LIST    *pListTail;

  /*-
   *********************************************************************
   *
   * Check the string's length.
   *
   *********************************************************************
   */
  iLength = strlen(pcString);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length must be greater than zero.", acRoutine, iLength);
    return ER_Length;
  }
  if (iLength > DIG_MAX_STRING_SIZE - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", acRoutine, iLength, DIG_MAX_STRING_SIZE - 1);
    return ER_Length;
  }

  /*-
   *********************************************************************
   *
   * Unescape the string.
   *
   *********************************************************************
   */
  pcUnEscaped = HTTPUnEscape(pcString, &iLength, acLocalError);
  if (pcUnEscaped == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER_BadValue;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory for a new list element. The caller is expected to
   * free this memory.
   *
   *********************************************************************
   */
  pListNew = pListTail = (DIG_SEARCH_LIST *) malloc(sizeof(DIG_SEARCH_LIST));
  if (pListNew == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    HTTPFreeData(pcUnEscaped);
    return ER_BadHandle;
  }
  memset(pListNew, 0, sizeof(DIG_SEARCH_LIST));

  /*-
   *********************************************************************
   *
   * Find a place for this string in the list. Don't add duplicates.
   *
   *********************************************************************
   */
  pListCurrent = gppsSearchList[(unsigned char) pcUnEscaped[0]];
  if (pListCurrent == NULL)
  {
    gppsSearchList[(unsigned char) pcUnEscaped[0]] = pListNew;
  }
  else
  {
    for ( ; pListCurrent != NULL; pListCurrent = pListCurrent->psNext)
    {
      if (pListCurrent->psNext == NULL)
      {
        pListTail = pListCurrent;
      }
      if (memcmp(pListCurrent->acRawString, pcUnEscaped, iLength) == 0 && pListCurrent->iLength == iLength)
      {
        free(pListNew);
        HTTPFreeData(pcUnEscaped);
        return ER_OK; /* Duplicate */
      }
    }
    pListTail->psNext = pListNew;
  }

  /*-
   *********************************************************************
   *
   * Add the string to the list.
   *
   *********************************************************************
   */
  pListTail = pListNew;
  strncpy(pListTail->acEscString, pcString, DIG_MAX_STRING_SIZE);
  memcpy(pListTail->acRawString, (unsigned char *) pcUnEscaped, iLength);
  pListTail->iHits = 0;
  pListTail->iHitsPerFile = 0;
  pListTail->iLength = iLength;
  pListTail->psNext = NULL;

  if (iLength > giMaxStringLength)
  {
    giMaxStringLength = iLength;
  }
  giStringCount++;

  HTTPFreeData(pcUnEscaped);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigClearCounts
 *
 ***********************************************************************
 */
void
DigClearCounts(void)
{
  int                 i;
  DIG_SEARCH_LIST    *psSearchList;

  for (i = 0; i < 256; i++)
  {
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->psNext)
    {
      psSearchList->iHitsPerFile = 0;
    }
  }
}


/*-
 ***********************************************************************
 *
 * DigDevelopOutput
 *
 ***********************************************************************
 */
int
DigDevelopOutput(DIG_SEARCH_DATA *psSearchData, char *pcError)
{
  const char          acRoutine[] = "DigDevelopOutput()";
  char                acOffset[FTIMES_MAX_64BIT_SIZE];
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iIndex;
  int                 iError;

  /*-
   *********************************************************************
   * 
   * name          1 (for quote) + (3 * FTIMES_MAX_PATH) + 1 (for quote)
   * offset        FTIMES_MAX_64BIT_SIZE
   * string        DIG_MAX_STRING_SIZE
   * |'s           2
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[(3 * FTIMES_MAX_PATH) + FTIMES_MAX_64BIT_SIZE + DIG_MAX_STRING_SIZE + 6];

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  iIndex = sprintf(acOutput, "\"%s\"", psSearchData->pcFile);

  /*-
   *********************************************************************
   *
   * Offset = offset
   *
   *********************************************************************
   */
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%qu", (unsigned long long) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%qu", (unsigned long long) psSearchData->ui64Offset);
#else
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%llu", (unsigned long long) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long) psSearchData->ui64Offset);
#endif
#endif
#ifdef WIN32
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%I64u", (K_UINT64) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%I64u", (K_UINT64) psSearchData->ui64Offset);
#endif

  /*-
   *********************************************************************
   *
   * String = string
   *
   *********************************************************************
   */
  iIndex += sprintf(&acOutput[iIndex], "|%s", psSearchData->pcString);

  /*-
   *********************************************************************
   *
   * Newline.
   *
   *********************************************************************
   */
  iIndex += sprintf(&acOutput[iIndex], "%s", gacNewLine);

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(gpFile, acOutput, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output file hash.
   *
   *********************************************************************
   */
  MD5Cycle(gpsMD5Context, acOutput, iIndex);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigGetMatchLimit
 *
 ***********************************************************************
 */
int
DigGetMatchLimit(void)
{
  return giMatchLimit;
}


/*-
 ***********************************************************************
 *
 * DigGetMaxStringLength
 *
 ***********************************************************************
 */
int
DigGetMaxStringLength(void)
{
  return giMaxStringLength;
}


/*-
 ***********************************************************************
 *
 * DigGetSearchList
 *
 ***********************************************************************
 */
DIG_SEARCH_LIST *
DigGetSearchList(int iIndex)
{
  return (iIndex >= 0 && iIndex <= 255) ? gppsSearchList[iIndex] : NULL;
}


/*-
 ***********************************************************************
 *
 * DigGetStringCount
 *
 ***********************************************************************
 */
int
DigGetStringCount(void)
{
  return giStringCount;
}


/*-
 ***********************************************************************
 *
 * DigGetStringsMatched
 *
 ***********************************************************************
 */
int
DigGetStringsMatched(void)
{
  int                 i;
  int                 iMatched;
  DIG_SEARCH_LIST    *psSearchList;

  for (i = 0, iMatched = 0; i < 256; i++)
  {
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->psNext)
    {
      if (psSearchList->iHits > 0)
      {
        iMatched++;
      }
    }
  }
  return iMatched;
}


/*-
 ***********************************************************************
 *
 * DigGetTotalMatches
 *
 ***********************************************************************
 */
K_UINT64
DigGetTotalMatches(void)
{
  int                 i;
  K_UINT64            ui64Matches;
  DIG_SEARCH_LIST    *psSearchList;

  for (i = 0, ui64Matches = 0; i < 256; i++)
  {
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->psNext)
    {
      if (psSearchList->iHits > 0)
      {
        ui64Matches += psSearchList->iHits;
      }
    }
  }
  return ui64Matches;
}


/*-
 ***********************************************************************
 *
 * DigSearchData
 *
 ***********************************************************************
 */
int
DigSearchData(unsigned char *pucData, int iDataLength, int iStopShort, K_UINT64 ui64AbsoluteOffset, char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "DigSearchData()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  int                 iLength;
  int                 iStringLength;
  DIG_SEARCH_LIST    *psSearchList;
  DIG_SEARCH_DATA     sSearchData;

  iLength = iDataLength;
  iStringLength = iStopShort ? giMaxStringLength : 1;

  while (iLength >= iStringLength)
  {
    for ((psSearchList = gppsSearchList[*pucData]); psSearchList != NULL; psSearchList = psSearchList->psNext)
    {
      if (
           (giMatchLimit == 0 || psSearchList->iHitsPerFile < giMatchLimit) &&
           memcmp(psSearchList->acRawString, pucData, psSearchList->iLength) == 0 &&
           iLength >= psSearchList->iLength
         )
      {
        psSearchList->iHits++;
        psSearchList->iHitsPerFile++;

        sSearchData.pcFile = pcFilename;
        sSearchData.ui64Offset = ui64AbsoluteOffset + iDataLength - iLength;
        sSearchData.pcString = psSearchList->acEscString;

        iError = DigDevelopOutput(&sSearchData, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return iError;
        }
      }
    }
    iLength--;
    pucData++;
  }
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigSetHashBlock
 *
 ***********************************************************************
 */
void
DigSetHashBlock(MD5_CONTEXT *psMD5Context)
{
  gpsMD5Context = psMD5Context;
}


/*-
 ***********************************************************************
 *
 * DigSetMatchLimit
 *
 ***********************************************************************
 */
void
DigSetMatchLimit(int iMatchLimit)
{
  giMatchLimit = iMatchLimit;
}


/*-
 ***********************************************************************
 *
 * DigSetNewLine
 *
 ***********************************************************************
 */
void
DigSetNewLine(char *pcNewLine)
{
  strcpy(gacNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
}


/*-
 ***********************************************************************
 *
 * DigSetOutputStream
 *
 ***********************************************************************
 */
void
DigSetOutputStream(FILE *pFile)
{
  gpFile = pFile;
}


/*-
 ***********************************************************************
 *
 * DigWriteHeader
 *
 ***********************************************************************
 */
int
DigWriteHeader(FILE *pFile, char *pcNewLine, char *pcError)
{
  const char          acRoutine[] = "DigWriteHeader()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acHeaderData[FTIMES_MAX_LINE];
  int                 iError;
  int                 iIndex;

  /*-
   *********************************************************************
   *
   * Initialize the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Alpha(gpsMD5Context);

  /*-
   *********************************************************************
   *
   * Build the output's header.
   *
   *********************************************************************
   */
  iIndex = sprintf(acHeaderData, "name|offset|string%s", pcNewLine);

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(pFile, acHeaderData, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Cycle(gpsMD5Context, acHeaderData, iIndex);

  return ER_OK;
}
