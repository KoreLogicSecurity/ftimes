/*-
 ***********************************************************************
 *
 * $Id: dig.c,v 1.29 2007/02/23 00:22:35 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef WIN32
static char           gacNewLine[NEWLINE_LENGTH] = CRLF;
#else
static char           gacNewLine[NEWLINE_LENGTH] = LF;
#endif
static DIG_STRING *gppsSearchListNormal[DIG_MAX_CHAINS];
static DIG_STRING *gppsSearchListNoCase[DIG_MAX_CHAINS];
#ifdef USE_PCRE
static DIG_STRING *gppsSearchListRegExp[DIG_MIN_CHAINS]; /* There is only one index for this chain. */
#endif
#ifdef USE_XMAGIC
static DIG_STRING *gppsSearchListXMagic[DIG_MIN_CHAINS]; /* There is only one index for this chain. */
#endif
static FILE          *gpFile;
static int            giMatchLimit;
static int            giMaxStringLength;
static int            giSaveLength;
static MD5_CONTEXT   *gpsMD5Context;

static char gacDigStringTypes[][DIG_MAX_TYPE_SIZE] =
{
  { "normal" },
  { "nocase" },
#ifdef USE_PCRE
  { "regexp" },
#endif
#ifdef USE_XMAGIC
  { "xmagic" },
#endif
  { "" } /* Used by DigGetStringType() when an invalid type is specified. */
};
static const int giDigStringTypesLength = sizeof(gacDigStringTypes) / sizeof(gacDigStringTypes[0]);

/*-
 ***********************************************************************
 *
 * DigAddDigString
 *
 ***********************************************************************
 */
int
DigAddDigString(char *pcString, int iType, char *pcError)
{
  const char          acRoutine[] = "DigAddDigString()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  DIG_STRING         *psCurrent;
  DIG_STRING         *psDigString;
  DIG_STRING         *psHead;
  DIG_STRING         *psTail;
  int                 iError;

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new DigString.
   *
   *********************************************************************
   */
  psDigString = DigNewDigString(pcString, iType, acLocalError);
  if (psDigString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * If the chain is empty, just insert the new DigString and return.
   *
   *********************************************************************
   */
  switch (iType)
  {
#ifdef USE_PCRE
  case DIG_STRING_TYPE_REGEXP:
    psHead = DigGetSearchList(iType, DIG_FIRST_CHAIN_INDEX);
    break;
#endif
#ifdef USE_XMAGIC
  case DIG_STRING_TYPE_XMAGIC:
    psHead = DigGetSearchList(iType, DIG_FIRST_CHAIN_INDEX);
    /* NOTE: Do not set a max string length for this type. Let it default to zero. */
    break;
#endif
  default:
    psHead = DigGetSearchList(iType, psDigString->pucDecodedString[0]);
    DigSetMaxStringLength(psDigString->iDecodedLength); /* Update the max string length. */
    break;
  }
  if (psHead == NULL)
  {
    iError = DigSetSearchList(psDigString, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    return ER_OK;
  }

  /*-
   *********************************************************************
   *
   * Otherwise, append the new DigString to the end of the chain, but
   * don't add duplicates.
   *
   *********************************************************************
   */
  for (psCurrent = psTail = psHead; psCurrent != NULL; psCurrent = psCurrent->psNext)
  {
    if (psCurrent->psNext == NULL)
    {
      psTail = psCurrent;
    }
    if (psCurrent->iDecodedLength == psDigString->iDecodedLength && memcmp(psCurrent->pucDecodedString, psDigString->pucDecodedString, psDigString->iDecodedLength) == 0)
    {
      snprintf(acLocalError, MESSAGE_SIZE, "Type = [%s], String = [%s]: Ignoring duplicate string.", DigGetStringType(iType), pcString);
      ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
      DigFreeDigString(psDigString);
      return ER_OK; /* Duplicates are not considered an error. */
    }
  }
  psTail->psNext = psDigString;

  return ER_OK;
}


#ifdef USE_PCRE
/*-
 ***********************************************************************
 *
 * DigAdjustRegExpOffsets
 *
 ***********************************************************************
 */
void
DigAdjustRegExpOffsets(int iTrimSize)
{
  DIG_STRING         *psDigString;

  for (psDigString = DigGetSearchList(DIG_STRING_TYPE_REGEXP, DIG_FIRST_CHAIN_INDEX); psDigString != NULL; psDigString = psDigString->psNext)
  {
    psDigString->iOffset = (psDigString->iLastOffset > iTrimSize) ? psDigString->iLastOffset - iTrimSize : 0;
    psDigString->iLastOffset = 0;
  }
}
#endif


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
  int                 iIndex;
  int                 iType;
  DIG_STRING         *psDigString;

  for (iType = DIG_STRING_TYPE_NORMAL; iType < DIG_STRING_TYPE_NOMORE; iType++)
  {
    for (iIndex = DIG_FIRST_CHAIN_INDEX; iIndex <= DIG_FINAL_CHAIN_INDEX; iIndex++)
    {
      for (psDigString = DigGetSearchList(iType, iIndex); psDigString != NULL; psDigString = psDigString->psNext)
      {
        psDigString->iHitsPerStream = 0;
        psDigString->iHitsPerBuffer = 0;
      }
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
  char               *pcNeutered;
  int                 iError;
  int                 iIndex;
  int                 iLimit;

  /*-
   *********************************************************************
   *
   * name          (3 * FTIMES_MAX_PATH) + 2 (for quotes)
   * type          DIG_MAX_TYPE_SIZE
   * tag           DIG_MAX_TAG_SIZE
   * offset        FTIMES_MAX_64BIT_SIZE
   * string        (3 * DIG_MAX_STRING_SIZE)
   * |'s           3
   * newline       2
   *
   *********************************************************************
   */
  char acOutput[(3 * FTIMES_MAX_PATH) + DIG_MAX_TYPE_SIZE + DIG_MAX_TAG_SIZE + FTIMES_MAX_64BIT_SIZE + (3 * DIG_MAX_STRING_SIZE) + 7];

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
   * Type = type
   *
   *********************************************************************
   */
  iIndex += sprintf(&acOutput[iIndex], "|%s", DigGetStringType(psSearchData->iType));

  /*-
   *********************************************************************
   *
   * Tag = tag
   *
   *********************************************************************
   */
  iIndex += sprintf(&acOutput[iIndex], "|%s", psSearchData->pcTag);

  /*-
   *********************************************************************
   *
   * Offset = offset
   *
   *********************************************************************
   */
#ifdef WIN32
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%I64u", (K_UINT64) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%I64u", (K_UINT64) psSearchData->ui64Offset);
#else
#ifdef USE_AP_SNPRINTF
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%qu", (unsigned long long) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%qu", (unsigned long long) psSearchData->ui64Offset);
#else
  iIndex += snprintf(&acOutput[iIndex], FTIMES_MAX_64BIT_SIZE, "|%llu", (unsigned long long) psSearchData->ui64Offset);
  snprintf(acOffset, FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long) psSearchData->ui64Offset);
#endif
#endif

  /*-
   *********************************************************************
   *
   * String = string
   *
   *********************************************************************
   */
  iLimit = (psSearchData->iLength < DIG_MAX_STRING_SIZE) ? psSearchData->iLength : DIG_MAX_STRING_SIZE;
#ifdef USE_XMAGIC
  if (psSearchData->iType == DIG_STRING_TYPE_XMAGIC)
  {
    iIndex += sprintf(&acOutput[iIndex], "|%s", (char *) psSearchData->pucData);
  }
  else
  {
#endif
    pcNeutered = SupportNeuterString(psSearchData->pucData, iLimit, acLocalError);
    if (pcNeutered == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    iIndex += sprintf(&acOutput[iIndex], "|%s", pcNeutered);
    free(pcNeutered);
#ifdef USE_XMAGIC
  }
#endif

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
 * DigFreeDigString
 *
 ***********************************************************************
 */
void
DigFreeDigString(DIG_STRING *psDigString)
{
  if (psDigString != NULL)
  {
    if (psDigString->pcTag != NULL)
    {
      free(psDigString->pcTag);
    }
    if (psDigString->pucEncodedString != NULL)
    {
      free(psDigString->pucEncodedString);
    }
    if (psDigString->pucDecodedString != NULL)
    {
      free(psDigString->pucDecodedString);
    }
#ifdef USE_PCRE
    if (psDigString->psPcre != NULL)
    {
      pcre_free(psDigString->psPcre);
    }
    if (psDigString->psPcreExtra != NULL)
    {
      pcre_free(psDigString->psPcreExtra);
    }
#endif
#ifdef USE_XMAGIC
    XMagicFreeXMagic(psDigString->psXMagic);
#endif
    free(psDigString);
  }
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
 * DigGetSaveLength
 *
 ***********************************************************************
 */
int
DigGetSaveLength(void)
{
  return giSaveLength;
}


/*-
 ***********************************************************************
 *
 * DigGetSearchList
 *
 ***********************************************************************
 */
DIG_STRING *
DigGetSearchList(int iType, int iIndex)
{
  switch (iType)
  {
  case DIG_STRING_TYPE_NORMAL:
    return (iIndex >= DIG_FIRST_CHAIN_INDEX && iIndex <= DIG_FINAL_CHAIN_INDEX) ? gppsSearchListNormal[iIndex] : NULL;
    break;
  case DIG_STRING_TYPE_NOCASE:
    return (iIndex >= DIG_FIRST_CHAIN_INDEX && iIndex <= DIG_FINAL_CHAIN_INDEX) ? gppsSearchListNoCase[iIndex] : NULL;
    break;
  /*-
   *********************************************************************
   *
   * Note: Various routines (e.g., DigGetStringCount) iterate over a
   * number of index values. Therefore, this code must return a pointer
   * to the RegExp or XMagic search lists when the caller supplies an
   * index value of DIG_FIRST_CHAIN_INDEX, and it must return NULL in
   * any other case.
   *
   *********************************************************************
   */
#ifdef USE_PCRE
  case DIG_STRING_TYPE_REGEXP:
    return (iIndex == DIG_FIRST_CHAIN_INDEX) ? gppsSearchListRegExp[iIndex] : NULL;
    break;
#endif
#ifdef USE_XMAGIC
  case DIG_STRING_TYPE_XMAGIC:
    return (iIndex == DIG_FIRST_CHAIN_INDEX) ? gppsSearchListXMagic[iIndex] : NULL;
    break;
#endif
  default:
    return NULL;
    break;
  }
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
  int                 iIndex;
  int                 iStringCount = 0;
  int                 iType;
  DIG_STRING         *psDigString;

  for (iType = DIG_STRING_TYPE_NORMAL; iType < DIG_STRING_TYPE_NOMORE; iType++)
  {
    for (iIndex = DIG_FIRST_CHAIN_INDEX; iIndex <= DIG_FINAL_CHAIN_INDEX; iIndex++)
    {
      for (psDigString = DigGetSearchList(iType, iIndex); psDigString != NULL; psDigString = psDigString->psNext)
      {
        iStringCount++;
      }
    }
  }
  return iStringCount;
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
  int                 iIndex;
  int                 iMatched = 0;
  int                 iType;
  DIG_STRING         *psDigString;

  for (iType = DIG_STRING_TYPE_NORMAL; iType < DIG_STRING_TYPE_NOMORE; iType++)
  {
    for (iIndex = DIG_FIRST_CHAIN_INDEX; iIndex <= DIG_FINAL_CHAIN_INDEX; iIndex++)
    {
      for (psDigString = DigGetSearchList(iType, iIndex); psDigString != NULL; psDigString = psDigString->psNext)
      {
        if (psDigString->iHitsPerJob > 0)
        {
          iMatched++;
        }
      }
    }
  }
  return iMatched;
}


/*-
 ***********************************************************************
 *
 * DigGetStringType
 *
 ***********************************************************************
 */
char *
DigGetStringType(int iType)
{
  return (iType >= 0 && iType < giDigStringTypesLength) ? gacDigStringTypes[iType] : gacDigStringTypes[giDigStringTypesLength - 1];
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
  int                 iIndex;
  int                 iType;
  K_UINT64            ui64Matches = 0;
  DIG_STRING         *psDigString;

  for (iType = DIG_STRING_TYPE_NORMAL; iType < DIG_STRING_TYPE_NOMORE; iType++)
  {
    for (iIndex = DIG_FIRST_CHAIN_INDEX; iIndex <= DIG_FINAL_CHAIN_INDEX; iIndex++)
    {
      for (psDigString = DigGetSearchList(iType, iIndex); psDigString != NULL; psDigString = psDigString->psNext)
      {
        if (psDigString->iHitsPerJob > 0)
        {
          ui64Matches += psDigString->iHitsPerJob;
        }
      }
    }
  }
  return ui64Matches;
}


/*-
 ***********************************************************************
 *
 * DigNewDigString
 *
 ***********************************************************************
 */
DIG_STRING *
DigNewDigString(char *pcString, int iType, char *pcError)
{
  const char          acRoutine[] = "DigNewDigString()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcTag = NULL;
  unsigned char      *pucDecodedString = NULL;
  unsigned char      *pucEncodedString = NULL;
  int                 i = 0;
  int                 iEndOfDigString = 0;
  int                 iHaveWhiteSpace = 0;
  int                 iLength = 0;
  int                 iWhitespaceCount = 0;
  int                 iWhitespaceIndex = 0;
  DIG_STRING         *psDigString = NULL;
#ifdef USE_PCRE
  const char         *pcPcreError = NULL;
  int                 iError = 0;
  int                 iPcreErrorOffset = 0;
#endif

  /*-
   *********************************************************************
   *
   * Check that the input string is not NULL and that it has length.
   *
   *********************************************************************
   */
  if (pcString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NULL input. That shouldn't happen.", acRoutine);
    return NULL;
  }

  iLength = strlen(pcString);
  if (iLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Length must be greater than zero.", acRoutine, iLength);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Scan the input string for white space. Count contiguous spaces as
   * a single logical space. When done, check the white space count. If
   * the count is zero, skip to the next section of code. If the count
   * is one, the user has supplied an optional tag, so find where it
   * begins. Otherwise, the user has supplied values containing embedded
   * white space, and that is not currently supported.
   *
   *********************************************************************
   */
  for (i = 0, iEndOfDigString = iLength; i < iLength; i++)
  {
    if (isspace((int) pcString[i]))
    {
      iWhitespaceCount += (i - iWhitespaceIndex > 1) ? 1 : 0;
      iWhitespaceIndex = i;
      if (iHaveWhiteSpace == 0)
      {
        iEndOfDigString = i;
        iHaveWhiteSpace = 1;
      }
    }
  }
  switch (iWhitespaceCount)
  {
  case 0:
    pcTag = NULL; /* There is no tag. */
    break;
  case 1:
    pcString[iEndOfDigString] = 0; /* Terminate the dig string. */
    pcTag = &pcString[iEndOfDigString + 1];
    while (*pcTag != 0 && isspace((int) *pcTag))
    {
      pcTag++;
    }
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Too much whitespace. Whitespace may only be used as a delimiter between the dig string value and its tag. Values or tags containing embedded whitespace are not currently supported.", acRoutine);
    return NULL;
    break;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory for a new DigString. The caller should free this
   * memory with DigFreeDigString().
   *
   *********************************************************************
   */
  psDigString = calloc(sizeof(DIG_STRING), 1);
  if (psDigString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Allocate memory and insert the encoded/decoded strings. The decode
   * step is not used for RegExp or XMagic values, but go through the
   * motions anyway so that all strings are handled the same way. This
   * adds to the workload, but avoids some special case code.
   *
   *********************************************************************
   */
  iLength = iEndOfDigString;
  pucEncodedString = calloc(iLength + 1, 1);
  if (pucEncodedString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    DigFreeDigString(psDigString);
    return NULL;
  }
  strncpy(pucEncodedString, pcString, iLength + 1);
  psDigString->pucEncodedString = pucEncodedString;
  psDigString->iEncodedLength = iLength;

  pucDecodedString = HTTPUnEscape(pcString, &iLength, acLocalError);
  if (pucDecodedString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    DigFreeDigString(psDigString);
    return NULL;
  }
  psDigString->pucDecodedString = pucDecodedString;
  psDigString->iDecodedLength = iLength;

  /*-
   *********************************************************************
   *
   * Allocate memory and insert the tag (if any) -- tags are optional.
   * Impose a tag length limit. This is required due to the fact that
   * DigDevelopOutput() uses a fixed-size output buffer.
   *
   *********************************************************************
   */
  iLength = (pcTag != NULL) ? strlen(pcTag) : 0;
  if (iLength > DIG_MAX_TAG_SIZE - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length = [%d]: Tag values must be less than %d bytes.", acRoutine, iLength, DIG_MAX_TAG_SIZE);
    DigFreeDigString(psDigString);
    return NULL;
  }
  psDigString->pcTag = calloc(iLength + 1, 1);
  if (psDigString->pcTag == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    DigFreeDigString(psDigString);
    return NULL;
  }
  strncpy(psDigString->pcTag, (pcTag != NULL) ? pcTag : "", iLength + 1);

  /*-
   *********************************************************************
   *
   * Initialize the structure according to the string's type.
   *
   *********************************************************************
   */
  switch (iType)
  {
  case DIG_STRING_TYPE_NORMAL:
    break;
  case DIG_STRING_TYPE_NOCASE:
    for (i = 0; i < psDigString->iDecodedLength; i++)
    {
      psDigString->pucDecodedString[i] = tolower(psDigString->pucDecodedString[i]);
    }
    break;
#ifdef USE_PCRE
  case DIG_STRING_TYPE_REGEXP:
    /*-
     *******************************************************************
     *
     * Compile and study the regular expression. Then, make sure that
     * there's exactly one capturing subpattern. Compile-time options
     * (?imsx) are not set here because the user can specify them as
     * needed in the dig strings.
     *
     *******************************************************************
     */
    psDigString->psPcre = pcre_compile(pcString, 0, &pcPcreError, &iPcreErrorOffset, NULL);
    if (psDigString->psPcre == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_compile(): %s", acRoutine, pcPcreError);
      DigFreeDigString(psDigString);
      return NULL;
    }
    psDigString->psPcreExtra = pcre_study(psDigString->psPcre, 0, &pcPcreError);
    if (pcPcreError != NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_study(): %s", acRoutine, pcPcreError);
      DigFreeDigString(psDigString);
      return NULL;
    }
    iError = pcre_fullinfo(psDigString->psPcre, psDigString->psPcreExtra, PCRE_INFO_CAPTURECOUNT, (void *) &psDigString->iCaptureCount);
    if (iError == ER_OK)
    {
      if (psDigString->iCaptureCount > PCRE_MAX_CAPTURE_COUNT)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Invalid capture count [%d]. The maximum number of capturing '()' subpatterns allowed is %d. Use '(?:)' if grouping is required.", acRoutine, psDigString->iCaptureCount, PCRE_MAX_CAPTURE_COUNT);
        DigFreeDigString(psDigString);
        return NULL;
      }
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_fullinfo(): Unexpected return value [%d]. That shouldn't happen.", acRoutine, iError);
      DigFreeDigString(psDigString);
      return NULL;
    }
    break;
#endif
#ifdef USE_XMAGIC
  case DIG_STRING_TYPE_XMAGIC:
    psDigString->psXMagic = XMagicLoadMagic(pcString, acLocalError);
    if (psDigString->psXMagic == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }
    break;
#endif
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type [%d]. That shouldn't happen.", acRoutine, iType);
    DigFreeDigString(psDigString);
    return NULL;
    break;
  }
  psDigString->iType = iType;
  psDigString->psNext = NULL;

  return psDigString;
}


/*-
 ***********************************************************************
 *
 * DigSearchData
 *
 ***********************************************************************
 */
int
DigSearchData(unsigned char *pucData, int iDataLength, int iStopShort, int iType, K_UINT64 ui64AbsoluteOffset, char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "DigSearchData()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  int                 i = 0;
  int                 iBytesLeft = iDataLength;
  int                 iMatch = 0;
  int                 iMinSearchLength = 0;
  int                 iMaxStringLength = DigGetMaxStringLength();
  int                 iOffset = 0;
  int                 iSaveLength = DigGetSaveLength();
  DIG_STRING         *psDigString;
  DIG_SEARCH_DATA     sSearchData;
#ifdef USE_PCRE
  int                 iDone = 0;
  int                 iMatchLength = 0;
  int                 iMatchOffset = 0;
  int                 iPcreOVector[PCRE_OVECTOR_ARRAY_SIZE];
#endif
#ifdef USE_XMAGIC
  int                 iCarrySize = AnalyzeGetCarrySize();
  int                 iStepSize = AnalyzeGetStepSize();
#endif

  iMinSearchLength = iStopShort ? iMaxStringLength : 1;

  switch (iType)
  {
  case DIG_STRING_TYPE_NORMAL:
    /*-
     *******************************************************************
     *
     * Advance the pointer to compensate for the save buffer.
     *
     *******************************************************************
     */
    if (ui64AbsoluteOffset != 0)
    {
      iOffset = (iSaveLength - iMaxStringLength) + 1;
      pucData += iOffset;
      iBytesLeft -= iOffset;
    }
    while (iBytesLeft >= iMinSearchLength)
    {
      for ((psDigString = gppsSearchListNormal[*pucData]); psDigString != NULL; psDigString = psDigString->psNext)
      {
        if (giMatchLimit == 0 || psDigString->iHitsPerStream < giMatchLimit)
        {
          if (
               iBytesLeft >= psDigString->iDecodedLength &&
               psDigString->pucDecodedString[psDigString->iDecodedLength - 1] == pucData[psDigString->iDecodedLength - 1] && /* If the last bytes don't match, there's no point in continuing. */
               memcmp(psDigString->pucDecodedString, pucData, psDigString->iDecodedLength) == 0
             )
          {
            psDigString->iHitsPerJob++;
            psDigString->iHitsPerStream++;
            psDigString->iHitsPerBuffer++;

            sSearchData.pcFile = pcFilename;
            sSearchData.pucData = pucData;
            sSearchData.iLength = psDigString->iDecodedLength;
            sSearchData.iType = psDigString->iType;
            sSearchData.pcTag = psDigString->pcTag;
            sSearchData.ui64Offset = ui64AbsoluteOffset + iDataLength - iBytesLeft;

            iError = DigDevelopOutput(&sSearchData, acLocalError);
            if (iError != ER_OK)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
              return iError;
            }
          }
        }
      }
      iBytesLeft--;
      pucData++;
    }
    break;
  case DIG_STRING_TYPE_NOCASE:
    /*-
     *******************************************************************
     *
     * Advance the pointer to compensate for the save buffer.
     *
     *******************************************************************
     */
    if (ui64AbsoluteOffset != 0)
    {
      iOffset = (iSaveLength - iMaxStringLength) + 1;
      pucData += iOffset;
      iBytesLeft -= iOffset;
    }
    while (iBytesLeft >= iMinSearchLength)
    {
      for ((psDigString = gppsSearchListNoCase[tolower(*pucData)]); psDigString != NULL; psDigString = psDigString->psNext)
      {
        if (giMatchLimit == 0 || psDigString->iHitsPerStream < giMatchLimit)
        {
          for (i = 0, iMatch = 1; i < psDigString->iDecodedLength && iMatch; i++) /* FIXME Searching in reverse order here would probably speed things up. */
          {
            if (pucData[i] >= 'A' && pucData[i] <= 'Z')
            {
              iMatch = (pucData[i] == psDigString->pucDecodedString[i] || (pucData[i] + 0x20) == psDigString->pucDecodedString[i]) ? 1 : 0;
            }
            else if (pucData[i] >= 'a' && pucData[i] <= 'z')
            {
              iMatch = (pucData[i] == psDigString->pucDecodedString[i] || (pucData[i] - 0x20) == psDigString->pucDecodedString[i]) ? 1 : 0;
            }
            else
            {
              iMatch = (pucData[i] == psDigString->pucDecodedString[i]) ? 1 : 0;
            }
          }
          if (iMatch && iBytesLeft >= psDigString->iDecodedLength)
          {
            psDigString->iHitsPerJob++;
            psDigString->iHitsPerStream++;
            psDigString->iHitsPerBuffer++;

            sSearchData.pcFile = pcFilename;
            sSearchData.pucData = pucData;
            sSearchData.iLength = psDigString->iDecodedLength;
            sSearchData.iType = psDigString->iType;
            sSearchData.pcTag = psDigString->pcTag;
            sSearchData.ui64Offset = ui64AbsoluteOffset + iDataLength - iBytesLeft;

            iError = DigDevelopOutput(&sSearchData, acLocalError);
            if (iError != ER_OK)
            {
              snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
              return iError;
            }
          }
        }
      }
      iBytesLeft--;
      pucData++;
    }
    break;
#ifdef USE_PCRE
  case DIG_STRING_TYPE_REGEXP:
    for ((psDigString = gppsSearchListRegExp[DIG_FIRST_CHAIN_INDEX]); psDigString != NULL; psDigString = psDigString->psNext)
    {
      iDone = 0;
      iOffset = psDigString->iOffset;
      while (!iDone && (giMatchLimit == 0 || psDigString->iHitsPerStream < giMatchLimit))
      {
        /*-
         ***************************************************************
         *
         * The PCRE_NOTEMPTY option is used here to squash any attempts
         * to match empty strings (e.g., (A*) or (a?b?)) and to prevent
         * infinite loops.
         *
         ***************************************************************
         */
        iError = pcre_exec(psDigString->psPcre, psDigString->psPcreExtra, pucData, iDataLength, iOffset, PCRE_NOTEMPTY, iPcreOVector, PCRE_OVECTOR_ARRAY_SIZE);
        if (iError < 0)
        {
          if (iError == PCRE_ERROR_NOMATCH)
          {
            iDone = 1;
          }
          else
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: pcre_exec(): Unexpected return value [%d]. That shouldn't happen.", acRoutine, iError);
            return ER;
          }
        }
        else
        {
          if (iError == 0) /* There's a match, but also an overflow. */
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: pcre_exec(): Unexpected return value [%d]. That shouldn't happen.", acRoutine, iError);
            return ER;
          }

          if (psDigString->iCaptureCount == 1)
          {
            iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_1H] - iPcreOVector[PCRE_CAPTURE_INDEX_1L];
            iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_1L];
          }
          else
          {
            iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_0H] - iPcreOVector[PCRE_CAPTURE_INDEX_0L];
            iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_0L];
          }

          psDigString->iHitsPerJob++;
          psDigString->iHitsPerStream++;
          psDigString->iHitsPerBuffer++;
          psDigString->iLastOffset = iMatchOffset + iMatchLength;

          sSearchData.pcFile = pcFilename;
          sSearchData.pucData = pucData + iMatchOffset;
          sSearchData.iLength = iMatchLength;
          sSearchData.iType = psDigString->iType;
          sSearchData.pcTag = psDigString->pcTag;
          sSearchData.ui64Offset = ui64AbsoluteOffset + iMatchOffset;

          iError = DigDevelopOutput(&sSearchData, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
            return iError;
          }

          iOffset = iPcreOVector[PCRE_CAPTURE_INDEX_0H]; /* The next place to continue searching. */
        }
      }
    }
    break;
#endif
#ifdef USE_XMAGIC
  case DIG_STRING_TYPE_XMAGIC:
    iMinSearchLength = iStopShort ? iCarrySize : sizeof(K_UINT32) - 1; /* The minimum search length is limited due to the way XMagicGetValueOffset() works. */
    for ((psDigString = gppsSearchListXMagic[DIG_FIRST_CHAIN_INDEX]); psDigString != NULL; psDigString = psDigString->psNext)
    {
      iOffset = 0;
      iBytesLeft = iDataLength;
      while (iBytesLeft > iMinSearchLength && (giMatchLimit == 0 || psDigString->iHitsPerStream < giMatchLimit))
      {
        char pcDescription[DIG_MAX_STRING_SIZE];
        int iXMagicBytesLeft = DIG_MAX_STRING_SIZE - 1;
        int iXMagicBytesUsed = 0;
        int iMatch = 0;

        iMatch = XMagicTestMagic(psDigString->psXMagic, &pucData[iOffset], iBytesLeft, pcDescription, &iXMagicBytesUsed, &iXMagicBytesLeft, acLocalError);
        switch (iMatch)
        {
        case XMAGIC_TEST_ERROR:
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          return ER;
          break;
        case XMAGIC_TEST_MATCH:
          pcDescription[iXMagicBytesUsed] = 0;

          psDigString->iHitsPerJob++;
          psDigString->iHitsPerStream++;
          psDigString->iHitsPerBuffer++;

          sSearchData.pcFile = pcFilename;
          sSearchData.pucData = pcDescription;
          sSearchData.iLength = iXMagicBytesUsed;
          sSearchData.iType = psDigString->iType;
          sSearchData.pcTag = psDigString->pcTag;
          sSearchData.ui64Offset = ui64AbsoluteOffset + iOffset;

          iError = DigDevelopOutput(&sSearchData, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
            return iError;
          }
          break;
        }

        iOffset += iStepSize;
        iBytesLeft -= iStepSize;
      }
    }
    break;
#endif
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type [%d]. That shouldn't happen.", acRoutine, iType);
    return ER;
    break;
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
 * DigSetMaxStringLength
 *
 ***********************************************************************
 */
void
DigSetMaxStringLength(int iStringLength)
{
  if (iStringLength > giMaxStringLength)
  {
    giMaxStringLength = iStringLength;
  }
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
 * DigSetSaveLength
 *
 ***********************************************************************
 */
void
DigSetSaveLength(int iSaveLength)
{
  giSaveLength = iSaveLength;
}


/*-
 ***********************************************************************
 *
 * DigSetSearchList
 *
 ***********************************************************************
 */
int
DigSetSearchList(DIG_STRING *psDigString, char *pcError)
{
  const char          acRoutine[] = "DigSetSearchList()";

  if (psDigString == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: NULL input. That shouldn't happen.", acRoutine);
    return ER;
  }

  switch (psDigString->iType)
  {
  case DIG_STRING_TYPE_NORMAL:
    gppsSearchListNormal[psDigString->pucDecodedString[0]] = psDigString;
    break;
  case DIG_STRING_TYPE_NOCASE:
    gppsSearchListNoCase[psDigString->pucDecodedString[0]] = psDigString;
    break;
#ifdef USE_PCRE
  case DIG_STRING_TYPE_REGEXP:
    gppsSearchListRegExp[DIG_FIRST_CHAIN_INDEX] = psDigString;
    break;
#endif
#ifdef USE_XMAGIC
  case DIG_STRING_TYPE_XMAGIC:
    gppsSearchListXMagic[DIG_FIRST_CHAIN_INDEX] = psDigString;
    break;
#endif
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type [%d]. That shouldn't happen.", acRoutine, psDigString->iType);
    return ER;
    break;
  }

  return ER_OK;
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
  iIndex = sprintf(acHeaderData, "name|type|tag|offset|string%s", pcNewLine);

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
