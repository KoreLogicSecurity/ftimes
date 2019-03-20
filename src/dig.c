/*-
 ***********************************************************************
 *
 * $Id: dig.c,v 1.5 2003/08/13 21:39:49 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static char           gcNewLine[NEWLINE_LENGTH] = CRLF;
#endif
#ifdef UNIX
static char           gcNewLine[NEWLINE_LENGTH] = LF;
#endif
static DIG_SEARCH_LIST *gppsSearchList[256];
static FILE          *gpFile;
static int            giMatchLimit;
static int            giMaxStringLength;
static int            giStringCount;
static MD5_CONTEXT   *gpMD5Context;


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
  const char          cRoutine[] = "DigAddString()";
  char                cLocalError[ERRBUF_SIZE],
                     *pcUnEscaped;
  int                 iLength;
  DIG_SEARCH_LIST    *pListNew,
                     *pListCurrent,
                     *pListTail;

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
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length must be greater than zero.", cRoutine, iLength);
    return ER_Length;
  }
  if (iLength > DIG_MAX_STRING_SIZE - 1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Length = [%d]: Length exceeds %d bytes.", cRoutine, iLength, DIG_MAX_STRING_SIZE - 1);
    return ER_Length;
  }

  /*-
   *********************************************************************
   *
   * Unescape the string.
   *
   *********************************************************************
   */
  pcUnEscaped = HTTPUnEscape(pcString, &iLength, cLocalError);
  if (pcUnEscaped == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
    for ( ; pListCurrent != NULL; pListCurrent = pListCurrent->pNext)
    {
      if (pListCurrent->pNext == NULL)
      {
        pListTail = pListCurrent;
      }
      if (memcmp(pListCurrent->cRawString, pcUnEscaped, iLength) == 0 && pListCurrent->iLength == iLength)
      {
        free(pListNew);
        HTTPFreeData(pcUnEscaped);
        return ER_OK; /* Duplicate */
      }
    }
    pListTail->pNext = pListNew;
  }

  /*-
   *********************************************************************
   *
   * Add the string to the list.
   *
   *********************************************************************
   */
  pListTail = pListNew;
  strncpy(pListTail->cEscString, pcString, DIG_MAX_STRING_SIZE);
  memcpy(pListTail->cRawString, (unsigned char *) pcUnEscaped, iLength);
  pListTail->iHits = 0;
  pListTail->iHitsPerFile = 0;
  pListTail->iLength = iLength;
  pListTail->pNext = NULL;

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
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->pNext)
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
  const char          cRoutine[] = "DigDevelopOutput()";
  char                cOffset[23],
                      cOutput[ERRBUF_SIZE],
                      cLocalError[ERRBUF_SIZE];
  int                 iIndex,
                      iError;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  iIndex = sprintf(cOutput, "\"%s\"", psSearchData->pcFile);

  /*-
   *********************************************************************
   *
   * Offset = offset
   *
   *********************************************************************
   */
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
  iIndex += snprintf(&cOutput[iIndex], 22, "|%qu", (K_UINT64) psSearchData->ui64Offset);
  snprintf(cOffset, 22, "%qu", (K_UINT64) psSearchData->ui64Offset);
#else
  iIndex += sprintf(&cOutput[iIndex], "|%qu", (K_UINT64) psSearchData->ui64Offset);
  sprintf(cOffset, "%qu", (K_UINT64) psSearchData->ui64Offset);
#endif
#endif
#ifdef WIN32
  iIndex += sprintf(&cOutput[iIndex], "|%I64u", (K_UINT64) psSearchData->ui64Offset);
  sprintf(cOffset, "%I64u", (K_UINT64) psSearchData->ui64Offset);
#endif

  /*-
   *********************************************************************
   *
   * String = string
   *
   *********************************************************************
   */
  iIndex += sprintf(&cOutput[iIndex], "|%s", psSearchData->pcString);

  /*-
   *********************************************************************
   *
   * Newline.
   *
   *********************************************************************
   */
  iIndex += sprintf(&cOutput[iIndex], "%s", gcNewLine);

  /*-
   *********************************************************************
   *
   * Record the collected data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(gpFile, cOutput, iIndex, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output file hash.
   *
   *********************************************************************
   */
  MD5Cycle(gpMD5Context, cOutput, iIndex);

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
  return (iIndex >=0 && iIndex <=255) ? gppsSearchList[iIndex] : NULL;
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
  int                 i,
                      iMatched;
  DIG_SEARCH_LIST    *psSearchList;

  for (i = 0, iMatched = 0; i < 256; i++)
  {
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->pNext)
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
    for (psSearchList = gppsSearchList[i]; psSearchList != NULL; psSearchList = psSearchList->pNext)
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
  const char          cRoutine[] = "DigSearchData()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  int                 iLength;
  int                 iStringLength;
  DIG_SEARCH_LIST    *psSearchList;
  DIG_SEARCH_DATA     sSearchData;

  cLocalError[0] = 0;

  iLength = iDataLength;
  iStringLength = iStopShort ? giMaxStringLength : 1;

  while (iLength >= iStringLength)
  {
    for ((psSearchList = gppsSearchList[*pucData]); psSearchList != NULL; psSearchList = psSearchList->pNext)
    {
      if (
           (giMatchLimit == 0 || psSearchList->iHitsPerFile < giMatchLimit) &&
           memcmp(psSearchList->cRawString, pucData, psSearchList->iLength) == 0 &&
           iLength >= psSearchList->iLength
         )
      {
        psSearchList->iHits++;
        psSearchList->iHitsPerFile++;

        sSearchData.pcFile = pcFilename;
        sSearchData.ui64Offset = ui64AbsoluteOffset + iDataLength - iLength;
        sSearchData.pcString = psSearchList->cEscString;

        iError = DigDevelopOutput(&sSearchData, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
DigSetHashBlock(MD5_CONTEXT *pMD5Context)
{
  gpMD5Context = pMD5Context;
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
  strcpy(gcNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF);
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
  const char          cRoutine[] = "DigWriteHeader()";
  char                cLocalError[ERRBUF_SIZE],
                      cHeaderData[FTIMES_MAX_LINE];
  int                 iError,
                      iIndex;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Initialize the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Alpha(gpMD5Context);

  /*-
   *********************************************************************
   *
   * Build the output's header.
   *
   *********************************************************************
   */
  iIndex = sprintf(cHeaderData, "name|offset|string%s", pcNewLine);

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(pFile, cHeaderData, iIndex, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Update the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Cycle(gpMD5Context, cHeaderData, iIndex);

  return ER_OK;
}
