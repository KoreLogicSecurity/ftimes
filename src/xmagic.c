/*-
 ***********************************************************************
 *
 * $Id: xmagic.c,v 1.113 2019/03/15 01:09:59 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define SKIP_WHITESPACE(pc) { while (*pc && isspace((int) *pc)) pc++; }
#define FIND_DELIMITER(pc) { while (*pc && (!isspace((int) *pc) || *(pc - 1) == '\\')) pc++; }

int                 giSystemByteOrder = -1;
APP_UI8             gaui08ByteOrderMagic[4] = {0x01, 0x02, 0x03, 0x04};

#ifdef USE_KLEL
XMAGIC_KLEL_TYPE_SPEC gasKlelTypes[] =
{
  { "belong_at",        KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64) },
  { "beshort_at",       KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64) },
  { "byte_at",          KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64) },
  { "f_size",           KLEL_TYPE_INT64 }, /* file size */
  { "lelong_at",        KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64) },
  { "leshort_at",       KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64) },
  { "row_entropy_1_at", KLEL_TYPE_REAL_FUNCTION2(KLEL_TYPE_INT64, KLEL_TYPE_INT64) },
  { "string_at",        KLEL_TYPE_STRING_FUNCTION1(KLEL_TYPE_INT64) },
};
#endif

/*-
 ***********************************************************************
 *
 * is80_ff
 *
 ***********************************************************************
 */
int
is80_ff(int c)
{
  return (c >= 0x80 && c <= 0xff);
}


#if defined(USE_KLEL) && !defined(HAVE_STRNLEN)
/*-
 ***********************************************************************
 *
 * strnlen
 *
 ***********************************************************************
 */
size_t strnlen(const char *pcString, size_t szMaxLength)
{
  const char *pcEnd = memchr(pcString, 0, szMaxLength);
  return (pcEnd != NULL) ? (size_t)(pcEnd - pcString) : szMaxLength;
};
#endif


/*-
 ***********************************************************************
 *
 * XMagicComputePercentage
 *
 ***********************************************************************
 */
double
XMagicComputePercentage(unsigned char *pucBuffer, int iLength, int iType)
{
  double              dSum = 0.0;
  double              dTotal = 0.0;
  int                 aiCodeCounts[XMAGIC_PERCENT_1BYTE_CODES];
  int                 i = 0;
  int               (*piRoutine)();

  /*-
   *********************************************************************
   *
   * Clear the code counts array.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_PERCENT_1BYTE_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength; i++)
  {
    aiCodeCounts[pucBuffer[i]]++;
  }

  /*-
   *********************************************************************
   *
   * Compute the percentage.
   *
   *********************************************************************
   */
  switch (iType)
  {
  case XMAGIC_PERCENT_CTYPE_80_FF:
    piRoutine = is80_ff;
    break;
  case XMAGIC_PERCENT_CTYPE_ALNUM:
    piRoutine = isalnum;
    break;
  case XMAGIC_PERCENT_CTYPE_ALPHA:
    piRoutine = isalpha;
    break;
  case XMAGIC_PERCENT_CTYPE_ASCII:
    piRoutine = isascii;
    break;
  case XMAGIC_PERCENT_CTYPE_CNTRL:
    piRoutine = iscntrl;
    break;
  case XMAGIC_PERCENT_CTYPE_DIGIT:
    piRoutine = isdigit;
    break;
  case XMAGIC_PERCENT_CTYPE_LOWER:
    piRoutine = islower;
    break;
  case XMAGIC_PERCENT_CTYPE_PRINT:
    piRoutine = isprint;
    break;
  case XMAGIC_PERCENT_CTYPE_PUNCT:
    piRoutine = ispunct;
    break;
  case XMAGIC_PERCENT_CTYPE_SPACE:
    piRoutine = isspace;
    break;
  case XMAGIC_PERCENT_CTYPE_UPPER:
    piRoutine = isupper;
    break;
  default:
    return 0.0;
    break;
  }

  for (i = 0; i < XMAGIC_PERCENT_1BYTE_CODES; i++)
  {
    dTotal += (double) aiCodeCounts[i];
    if (piRoutine(i))
    {
      dSum += (double) aiCodeCounts[i];
    }
  }

  return (dTotal == 0) ? 0.0 : dSum / dTotal * 100.0;
}


/*-
 ***********************************************************************
 *
 * XMagicComputePercentageCombo
 *
 ***********************************************************************
 */
char *
XMagicComputePercentageCombo(unsigned char *pucBuffer, int iLength, int iType)
{
  char               *pcCombo = NULL;
  double              dPercents[XMAGIC_COMBO_SLOT_COUNT];
  double              dSums[XMAGIC_COMBO_SLOT_COUNT];
  double              dTotal = 0.0;
  int                 aiCodeCounts[XMAGIC_PERCENT_1BYTE_CODES];
  int                 i = 0;
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * Allocate memory for the result string.
   *
   *********************************************************************
   */
  pcCombo = malloc(XMAGIC_COMBO_SIZE);
  if (pcCombo == NULL)
  {
    return NULL;
  }
  pcCombo[0] = 0;

  /*-
   *********************************************************************
   *
   * Clear the code counts, sums, and percentages arrays.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_PERCENT_1BYTE_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  for (i = 0; i < XMAGIC_COMBO_SLOT_COUNT; i++)
  {
    dSums[i] = dPercents[i] = 0.0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength; i++)
  {
    aiCodeCounts[pucBuffer[i]]++;
  }

  /*-
   *********************************************************************
   *
   * Tally up the combo counts.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_PERCENT_1BYTE_CODES; i++)
  {
    dTotal += (double) aiCodeCounts[i];
    switch (iType)
    {
    case XMAGIC_PERCENT_COMBO_CSPDAE:
      if (iscntrl(i) && !isspace(i))
      {
        dSums[0] += (double) aiCodeCounts[i];
      }
      else if (isspace(i))
      {
        dSums[1] += (double) aiCodeCounts[i];
      }
      else if (ispunct(i))
      {
        dSums[2] += (double) aiCodeCounts[i];
      }
      else if (isdigit(i))
      {
        dSums[3] += (double) aiCodeCounts[i];
      }
      else if (isalpha(i))
      {
        dSums[4] += (double) aiCodeCounts[i];
      }
      else
      {
        dSums[5] += (double) aiCodeCounts[i];
      }
      break;
    default:
      free(pcCombo);
      return NULL;
      break;
    }
  }

  /*-
   *********************************************************************
   *
   * Compute the percentages.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_COMBO_SLOT_COUNT; i++)
  {
    dPercents[i] = (dTotal == 0) ? 0.0 : dSums[i] / dTotal * 100.0;
  }

  /*-
   *********************************************************************
   *
   * Generate the result string.
   *
   *********************************************************************
   */
  switch (iType)
  {
  case XMAGIC_PERCENT_COMBO_CSPDAE:
    for (i = n = 0; i < XMAGIC_PERCENT_COMBO_CSPDAE_SLOTS; i++)
    {
      n += sprintf(&pcCombo[n], "%s%.2f", ((i) ? "," : ""), dPercents[i]);
    }
    pcCombo[n] = 0;
    break;
  default:
    free(pcCombo);
    return NULL;
    break;
  }

  return pcCombo;
}


/*-
 ***********************************************************************
 *
 * XMagicComputeRowAverage1
 *
 ***********************************************************************
 */
double
XMagicComputeRowAverage1(unsigned char *pucBuffer, int iLength)
{
  int                 aiCodeCounts[XMAGIC_ROW_AVERAGE_1_CODES];
  int                 i = 0;
  double              dAverage = 0.0;
  double              dSum = 0.0;

  /*-
   *********************************************************************
   *
   * Clear the code counts array.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_AVERAGE_1_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength; i++)
  {
    aiCodeCounts[pucBuffer[i]]++;
  }

  /*-
   *********************************************************************
   *
   * Compute the average.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_AVERAGE_1_CODES; i++)
  {
    dSum += (double) (i * aiCodeCounts[i]);
  }
  dAverage = dSum / iLength;

  return dAverage;
}


/*-
 ***********************************************************************
 *
 * XMagicComputeRowAverage2
 *
 ***********************************************************************
 */
double
XMagicComputeRowAverage2(unsigned char *pucBuffer, int iLength)
{
  int                 aiCodeCounts[XMAGIC_ROW_AVERAGE_2_CODES];
  int                 i = 0;
  double              dAverage = 0.0;
  double              dSum = 0.0;

  /*-
   *********************************************************************
   *
   * Clear the code counts array.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_AVERAGE_2_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength - 1; i++)
  {
    aiCodeCounts[(pucBuffer[i] << 8) | pucBuffer[i + 1]]++;
  }
  aiCodeCounts[(pucBuffer[i] << 8) | pucBuffer[0]]++;

  /*-
   *********************************************************************
   *
   * Compute the average.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_AVERAGE_2_CODES; i++)
  {
    dSum += (double) (i * aiCodeCounts[i]);
  }
  dAverage = dSum / iLength;

  return dAverage;
}


/*-
 ***********************************************************************
 *
 * XMagicComputeRowEntropy1
 *
 ***********************************************************************
 */
double
XMagicComputeRowEntropy1(unsigned char *pucBuffer, int iLength)
{
  int                 aiCodeCounts[XMAGIC_ROW_ENTROPY_1_CODES];
  int                 i = 0;
  double              dEntropy = 0.0;
  double              dProbability = 0.0;

  /*-
   *********************************************************************
   *
   * Clear the code counts array.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_ENTROPY_1_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength; i++)
  {
    aiCodeCounts[pucBuffer[i]]++;
  }

  /*-
   *********************************************************************
   *
   * Compute the entropy. H(Px) = - Sigma Px * log2(Px) = - Sigma Px * log10(Px) / log10(2) = - Sigma Px * log10(Px) * log2(10)
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_ENTROPY_1_CODES; i++)
  {
    if (aiCodeCounts[i] > 0)
    {
      dProbability = (double) aiCodeCounts[i] / iLength;
      dEntropy -= dProbability * log10(dProbability) * XMAGIC_LOG2_OF_10;
    }
  }

  return dEntropy;
}


/*-
 ***********************************************************************
 *
 * XMagicComputeRowEntropy2
 *
 ***********************************************************************
 */
double
XMagicComputeRowEntropy2(unsigned char *pucBuffer, int iLength)
{
  int                 aiCodeCounts[XMAGIC_ROW_ENTROPY_2_CODES];
  int                 i = 0;
  double              dEntropy = 0.0;
  double              dProbability = 0.0;

  /*-
   *********************************************************************
   *
   * Clear the code counts array.
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_ENTROPY_2_CODES; i++)
  {
    aiCodeCounts[i] = 0;
  }

  /*-
   *********************************************************************
   *
   * Tally up the the code counts.
   *
   *********************************************************************
   */
  for (i = 0; i < iLength - 1; i++)
  {
    aiCodeCounts[(pucBuffer[i] << 8) | pucBuffer[i + 1]]++;
  }
  aiCodeCounts[(pucBuffer[i] << 8) | pucBuffer[0]]++;

  /*-
   *********************************************************************
   *
   * Compute the entropy. H(Px) = - Sigma Px * log2(Px) = - Sigma Px * log10(Px) / log10(2) = - Sigma Px * log10(Px) * log2(10)
   *
   *********************************************************************
   */
  for (i = 0; i < XMAGIC_ROW_ENTROPY_2_CODES; i++)
  {
    if (aiCodeCounts[i] > 0)
    {
      dProbability = (double) aiCodeCounts[i] / iLength;
      dEntropy -= dProbability * log10(dProbability) * XMAGIC_LOG2_OF_10;
    }
  }

  return dEntropy;
}


/*-
 ***********************************************************************
 *
 * XMagicConvert2charHex
 *
 ***********************************************************************
 */
int
XMagicConvert2charHex(char *pcSRC, char *pcDST)
{
  int                 iConverted = 0;
  unsigned int        uiResult1 = 0;
  unsigned int        uiResult2 = 0;

  if (isxdigit((int) *(pcSRC + 1)) && isxdigit((int) *pcSRC))
  {
    uiResult2 = XMagicConvertHexToInt(*pcSRC);
    uiResult1 = XMagicConvertHexToInt(*(pcSRC + 1));
    if (uiResult2 == ER || uiResult1 == ER)
    {
      return -2;
    }
    else
    {
      *pcDST = (unsigned char) ((uiResult2 << 4) + (uiResult1));
    }
    iConverted = 2;
  }
  else if (isxdigit((int) *pcSRC))
  {
    uiResult1 = XMagicConvertHexToInt(*pcSRC);
    if (uiResult1 == ER)
    {
      return -1;
    }
    else
    {
      *pcDST = (unsigned char) uiResult1;
    }
    iConverted = 1;
  }
  else
  {
    iConverted = 0;
  }

  return iConverted;
}


/*-
 ***********************************************************************
 *
 * XMagicConvert3charOct
 *
 ***********************************************************************
 */
int
XMagicConvert3charOct(char *pcSRC, char *pcDST)
{
  int                 iConverted = 0;

  if (isdigit((int) *(pcSRC + 2)) && isdigit((int) *(pcSRC + 1)))
  {
    *pcDST = (unsigned char) ((((*pcSRC - '0')) << 6) + (((*(pcSRC + 1) - '0')) << 3) + ((*(pcSRC + 2) - '0')));
    iConverted = 3;
  }
  else if (isdigit((int) *(pcSRC + 1)))
  {
    *pcDST = (unsigned char) ((((*pcSRC - '0')) << 3) + ((*(pcSRC + 1) - '0')));
    iConverted = 2;
  }
  else
  {
    *pcDST = (unsigned char) (*pcSRC - '0');
    iConverted = 1;
  }

  return iConverted;
}


/*-
 ***********************************************************************
 *
 * XMagicConvertHexToInt
 *
 ***********************************************************************
 */
int
XMagicConvertHexToInt(int iC)
{
  if (isdigit(iC))
  {
    return iC - '0';
  }
  else if ((iC >= 'a') && (iC <= 'f'))
  {
    return iC + 10 - 'a';
  }
  else if ((iC >= 'A') && (iC <= 'F'))
  {
    return iC + 10 - 'A';
  }
  else
  {
    return -1;
  }
}


/*-
 ***********************************************************************
 *
 * XMagicFormatDescription
 *
 ***********************************************************************
 */
void
XMagicFormatDescription(void *pvValue, XMAGIC *psXMagic, char *pcDescription)
{
  const char          acRoutine[] = "XMagicFormatDescription()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acSafeBuffer[4 * XMAGIC_DESCRIPTION_BUFSIZE] = "";
  char               *pc = NULL;
  int                 i = 0;
  int                 iLength = 0;
  int                 iBytesLeft = XMAGIC_DESCRIPTION_BUFSIZE - 1;
  int                 n = 0;
  APP_UI32           *pui32Value = NULL;
  APP_UI64           *pui64Value = NULL;

  /*-
   *********************************************************************
   *
   * Conditionally prepend a leading space to the description buffer.
   *
   *********************************************************************
   */
  if ((psXMagic->ui32Level > 0) && (psXMagic->acDescription[0] != 0) && ((psXMagic->ui32Flags & XMAGIC_NO_SPACE) != XMAGIC_NO_SPACE))
  {
    pcDescription[0] = ' ';
    pcDescription[1] = 0;
    n = 1;
  }
  else
  {
    pcDescription[0] = 0;
    n = 0;
  }
  iBytesLeft -= n;

  /*-
   *********************************************************************
   *
   * Format the description as needed for each type.
   *
   *********************************************************************
   */
  if (psXMagic->iType == XMAGIC_STRING)
  {
    char *pcNeutered = NULL;
    char *pcValue = (char *) pvValue;

    /*-
     *******************************************************************
     *
     * If the test operator is not XMAGIC_OP_EQ, attempt to locate the
     * terminating NULL byte. It is possible that this terminator does
     * not exist, so the search must be limited to the maximum allowed
     * string length. Doing otherwise will lead to wasted effort. More
     * importantly, it may cause us to walk off the end of our buffer,
     * which could lead to incorrect/inconsistent results.
     *
     *******************************************************************
     */
    if (psXMagic->iTestOperator != XMAGIC_OP_EQ)
    {
      for (i = 0, pc = pcValue; i < XMAGIC_STRING_BUFSIZE; i++, pc++)
      {
        if (*pc == '\0')
        {
          break;
        }
      }
      iLength = i;
    }
    else
    {
      iLength = psXMagic->iStringLength;
    }

    /*-
     *******************************************************************
     *
     * Neuter the string and trim it to fit in the provided buffer.
     *
     *******************************************************************
     */
    pcNeutered = SupportNeuterString(pcValue, iLength, acLocalError);
    if (pcNeutered == NULL)
    {
      char ac[1] = { 0 };
      char acLocalMessage[MESSAGE_SIZE];
      snprintf(acLocalMessage, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(ER_Warning, acLocalMessage, ERROR_WARNING);
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, ac);
    }
    else
    {
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, pcNeutered);
      free(pcNeutered);
    }
  }
  else if (psXMagic->iType == XMAGIC_PSTRING)
  {
    char *pcNeutered = NULL;
    char *pcValue = (char *) pvValue;

    /*-
     *******************************************************************
     *
     * Since the actual length of the subject string may exceed the
     * maximum allowed string length, it must be limited.
     *
     *******************************************************************
     */
    i = (int) *pcValue;
    iLength = (i < XMAGIC_STRING_BUFSIZE) ? i : XMAGIC_STRING_BUFSIZE;
    pcValue++; /* Advance pointer to the beginning of the string. */

    /*-
     *******************************************************************
     *
     * Neuter the string and trim it to fit in the provided buffer.
     *
     *******************************************************************
     */
    pcNeutered = SupportNeuterString(pcValue, iLength, acLocalError);
    if (pcNeutered == NULL)
    {
      char ac[1] = { 0 };
      char acLocalMessage[MESSAGE_SIZE];
      snprintf(acLocalMessage, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      ErrorHandler(ER_Warning, acLocalMessage, ERROR_WARNING);
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, ac);
    }
    else
    {
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, pcNeutered);
      free(pcNeutered);
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_MD5 ||
    psXMagic->iType == XMAGIC_SHA1 ||
    psXMagic->iType == XMAGIC_SHA256
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->pcHash);
  }
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dPercent);
  }
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->pcCombo);
  }
#ifdef USE_PCRE
  else if (psXMagic->iType == XMAGIC_REGEXP)
  {
    /*-
     *******************************************************************
     *
     * Capturing ()'s are not supported when '!~' is the operator, so
     * just print out the description. Otherwise, neuter the captured
     * buffer and make it available for printing. Do not exceed the
     * number of bytes left in the output buffer -- no matter how big
     * the neutered string is.
     *
     *******************************************************************
     */
    if (psXMagic->iTestOperator == XMAGIC_OP_REGEXP_NE)
    {
      char ac[1] = { 0 };
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, ac);
    }
    else
    {
      char *pcNeutered = NULL;
      pcNeutered = SupportNeuterString((char *) psXMagic->aucCapturedData, psXMagic->iMatchLength, acLocalError);
      if (pcNeutered == NULL)
      {
        char ac[1] = { 0 };
        char acLocalMessage[MESSAGE_SIZE];
        snprintf(acLocalMessage, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(ER_Warning, acLocalMessage, ERROR_WARNING);
        n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, ac);
      }
      else
      {
        n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, pcNeutered);
        free(pcNeutered);
      }
    }
  }
#endif
  else if
  (
    psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_2
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dAverage);
  }
  else if
  (
    psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_2
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dEntropy);
  }
  else if (psXMagic->iType == XMAGIC_DATE || psXMagic->iType == XMAGIC_LEDATE || psXMagic->iType == XMAGIC_BEDATE)
  {
    time_t tTime = (time_t) *((APP_UI32 *) pvValue);
    for (i = 0, pc = ctime(&tTime); *pc; pc++)
    {
      if (*pc == '\r' || *pc == '\n')
      {
        acSafeBuffer[i++] = 0;
        break;
      }
      else
      {
        acSafeBuffer[i++] = *pc;
      }
    }
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, acSafeBuffer);
  }
  else if (psXMagic->iType == XMAGIC_UNIX_YMDHMS_LEDATE || psXMagic->iType == XMAGIC_UNIX_YMDHMS_BEDATE)
  {
    char acTime[XMAGIC_TIME_SIZE];
    time_t tTime = *((APP_UI32 *) pvValue);
/* FIXME Add logic to handle time values that are out of range. */
    strftime(acTime, XMAGIC_YMDHMS_FORMAT_SIZE, XMAGIC_YMDHMS_FORMAT, gmtime(&tTime));
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, acTime);
  }
  else if (psXMagic->iType == XMAGIC_WINX_YMDHMS_LEDATE || psXMagic->iType == XMAGIC_WINX_YMDHMS_BEDATE)
  {
    APP_UI64 ui64TimeValue = *((APP_UI64 *) pvValue);
    APP_UI64 ui64TimeDelta = (XMagicSwapUi64(ui64TimeValue, psXMagic->iType) - UNIX_EPOCH_IN_NT_TIME) / 10000000;
    char acTime[XMAGIC_TIME_SIZE];
/* FIXME Add logic to handle time values that are out of range. */
    time_t tTime = (time_t) ui64TimeDelta;
    strftime(acTime, XMAGIC_YMDHMS_FORMAT_SIZE, XMAGIC_YMDHMS_FORMAT, gmtime(&tTime));
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, acTime);
  }
  else if (psXMagic->iType == XMAGIC_UI64 || psXMagic->iType == XMAGIC_BEUI64 || psXMagic->iType == XMAGIC_LEUI64)
  {
    pui64Value = (APP_UI64 *) pvValue;
#ifdef WIN32
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, *pui64Value);
#else
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, (unsigned long long) *pui64Value);
#endif
  }
#ifdef USE_KLEL
  else if (psXMagic->iType == XMAGIC_KLELEXP)
  {
    if (strcmp(KlelGetCommandInterpreter(psXMagic->psKlelContext), "concat") == 0)
    {
      KLEL_COMMAND *psCommand = NULL;
      n += snprintf(&pcDescription[n], iBytesLeft, "%s", KlelGetCommandProgram(psXMagic->psKlelContext)); /* NOTE: This is really the type/subtype. */
      iBytesLeft -= n;
      psCommand = KlelGetCommand(psXMagic->psKlelContext);
      if (psCommand == NULL)
      {
        snprintf
        (
          acLocalError,
          MESSAGE_SIZE,
          "%s: KlelGetCommand(): KlelExp = [%s]: Failed to evaluate command (%s).",
          acRoutine,
          KlelGetName(psXMagic->psKlelContext),
          KlelGetError(psXMagic->psKlelContext)
        );
        ErrorHandler(ER_Failure, acLocalError, ERROR_FAILURE);
      }
      else
      {
        n += snprintf(&pcDescription[n], iBytesLeft, "%s", psCommand->ppcArgumentVector[1]);
        KlelFreeCommand(psCommand);
      }
    }
  }
#endif
  else
  {
    pui32Value = (APP_UI32 *) pvValue;
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, *pui32Value);
  }
}


/*-
 ***********************************************************************
 *
 * XMagicFreeXMagic
 *
 ***********************************************************************
 */
void
XMagicFreeXMagic(XMAGIC *psXMagic)
{
  if (psXMagic != NULL)
  {
    if (psXMagic->pcHash != NULL)
    {
      free(psXMagic->pcHash);
    }
    if (psXMagic->pcCombo != NULL)
    {
      free(psXMagic->pcCombo);
    }
#ifdef USE_PCRE
    if (psXMagic->psPcre != NULL)
    {
      free(psXMagic->psPcre);
    }
    if (psXMagic->psPcreExtra != NULL)
    {
      free(psXMagic->psPcreExtra);
    }
#endif
    free(psXMagic);
  }
}


/*-
 ***********************************************************************
 *
 * XMagicGetDescription
 *
 ***********************************************************************
 */
int
XMagicGetDescription(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicGetDescription()";
  int                 iLength = 0;

  /*-
   *********************************************************************
   *
   * Check for a backspace (i.e. no space) designator at the beginning
   * of the description. Note this is not documented in the man page.
   *
   *********************************************************************
   */
  if (*pcS == '\b')
  {
    psXMagic->ui32Flags |= XMAGIC_NO_SPACE;
    pcS++;
  }
  else if (*pcS == '\\' && *(pcS + 1) == 'b')
  {
    psXMagic->ui32Flags |= XMAGIC_NO_SPACE;
    pcS += 2;
  }

  /*-
   *********************************************************************
   *
   * Burn off any trailing white space.
   *
   *********************************************************************
   */
  while (pcE > pcS && isspace((int) *(pcE - 1)))
  {
    pcE--;
  }

  *pcE = 0;

  iLength = strlen(pcS);
  if (iLength > XMAGIC_DESCRIPTION_BUFSIZE - 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Description length exceeds %d bytes.", acRoutine, XMAGIC_DESCRIPTION_BUFSIZE - 1);
    return ER;
  }
  strncpy(psXMagic->acDescription, pcS, XMAGIC_DESCRIPTION_BUFSIZE);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicGetLine
 *
 ***********************************************************************
 */
char *
XMagicGetLine(FILE *pFile, int iMaxLine, unsigned int uiFlags, int *piLinesConsumed, char *pcError)
{
  const char          acRoutine[] = "XMagicGetLine()";
  char               *pcComment = NULL;
  char               *pcData = NULL;
  char               *pcLine = NULL;
  char               *pcTemp = NULL;
  int                 iDone = 0;
  int                 iLength = 0;
  int                 iNToKeep = 0;
  int                 iOffset = 0;

  /*-
   *********************************************************************
   *
   * Initialize the line count and allocate some memory.
   *
   *********************************************************************
   */
  *piLinesConsumed = 0;

  pcData = calloc(iMaxLine + 1, 1);
  if (pcData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Read and conditionally join lines to form a composite line.
   *
   *********************************************************************
   */
  for (iOffset = 0; !iDone; iOffset += iNToKeep)
  {
    pcData[0] = 0;
    pcTemp = fgets(pcData, iMaxLine, pFile);
    if (pcTemp == NULL)
    {
      if (ferror(pFile))
      {
        if (pcData != NULL)
        {
          free(pcData);
        }
        if (pcLine != NULL)
        {
          free(pcLine);
        }
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
        return NULL;
      }
      if (pcLine == NULL) /* EOF was reached before any portion of a composite line was read. */
      {
        if (pcData != NULL)
        {
          free(pcData);
        }
        return NULL;
      }
      else /* EOF was reached after some portion of a composite line was read. */
      {
        iNToKeep = 0;
        iDone = 1;
      }
    }
    else
    {
      (*piLinesConsumed)++;
      pcComment = NULL;
      while (isspace((int) *pcTemp))
      {
        pcTemp++;
      }
      iLength = strlen(pcTemp);
      while (iLength > 0 && (pcTemp[iLength - 1] == '\r' || pcTemp[iLength - 1] == '\n'))
      {
        pcTemp[--iLength] = 0;
      }
      pcComment = strchr(pcTemp, '#');
      if (pcComment != NULL && (uiFlags & XMAGIC_PRESERVE_COMMENTS) == 0)
      {
        if (pcTemp[iLength - 1] == '\\')
        {
          *(pcComment + 0) = '\\';
          *(pcComment + 1) = 0;
        }
        else
        {
          *(pcComment + 0) = 0;
        }
      }
      iLength = strlen(pcTemp);
      if (pcTemp[iLength - 1] == '\\')
      {
        pcTemp[--iLength] = 0;
      }
      else
      {
        iDone = 1;
      }
      iNToKeep = iLength;
      pcLine = realloc(pcLine, iOffset + iNToKeep + 1);
      if (pcLine == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
        return NULL;
      }
      strncpy(&pcLine[iOffset], pcTemp, iNToKeep + 1);
    }
  }
  free(pcData);

  /*-
   *********************************************************************
   *
   * Abort if the composite line exceeds the specified number of bytes.
   *
   *********************************************************************
   */
  if (iOffset > iMaxLine)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Composite line length exceeds %d bytes.", acRoutine, iMaxLine);
    free(pcLine);
    return NULL;
  }

  return pcLine;
}


/*-
 ***********************************************************************
 *
 * XMagicGetOffset
 *
 ***********************************************************************
 */
int
XMagicGetOffset(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicGetOffset()";
  char               *pc = pcS;
  char               *pcEnd = NULL;
  char               *pcTmp = NULL;

  /*-
   *********************************************************************
   *
   * Determine the test level.
   *
   *********************************************************************
   */
  while (*pc == '>' || *pc == '<')
  {
    if (*pc == '<')
    {
      psXMagic->ui32FallbackCount++;
    }
    psXMagic->ui32Level++;
    pc++;
  }

  if (psXMagic->ui32FallbackCount && psXMagic->ui32FallbackCount != psXMagic->ui32Level)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: FallbackCount = [%d] != [%d]: FallbackCount mismatch! Either use all '>'s or al '<'s for the test level.", acRoutine, psXMagic->ui32FallbackCount, psXMagic->ui32Level);
    return ER;
  }

  if (psXMagic->ui32Level > XMAGIC_MAX_LEVEL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Level = [%u]: Level must not exceed %d.", acRoutine, psXMagic->ui32Level, XMAGIC_MAX_LEVEL);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Check to see if the offset is relative.
   *
   *********************************************************************
   */
  if ((*pc == '+' || *pc == '&') && psXMagic->ui32Level > 0)
  {
    psXMagic->ui32Flags |= XMAGIC_RELATIVE_OFFSET;
    pc++;
  }

  /*-
   *********************************************************************
   *
   * Check to see if the offset is indirect.
   *
   *********************************************************************
   */
  if (*pc == '(' && psXMagic->ui32Level > 0)
  {
    psXMagic->ui32Flags |= XMAGIC_INDIRECT_OFFSET;
    pc++;

    /*-
     *******************************************************************
     *
     * Verify that there is an indirection terminator.
     *
     *******************************************************************
     */
    if (*(--pcE) != ')')
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: No indirection terminator ')' was found.", acRoutine);
      return ER;
    }
    *pcE = 0;

    /*-
     *******************************************************************
     *
     * Check to see if the X offset is relative.
     *
     *******************************************************************
     */
    if (*pc == '&')
    {
      psXMagic->ui32Flags |= XMAGIC_RELATIVE_X_OFFSET;
      pc++;
    }

    /*-
     *******************************************************************
     *
     * Scan backwards looking for a warp operator [%&*+-/<>^|]. If one
     * is found, make sure the character to its left is valid. If both
     * conditions are met, read in and convert the Y value.
     *
     *******************************************************************
     */
    pcTmp = pcE;
    while (pcTmp != pc)
    {
      if
      (
        *pcTmp == '%' ||
        *pcTmp == '&' ||
        *pcTmp == '*' ||
        *pcTmp == '+' ||
        *pcTmp == '-' ||
        *pcTmp == '/' ||
        *pcTmp == '<' ||
        *pcTmp == '>' ||
        *pcTmp == '^' ||
        *pcTmp == '|'
      )
      {
        switch (*(pcTmp - 1)) /* Check the character to the left. */
        {
        case '.': /* case 'B': case 'b': */ case 'S': case 's': case 'L': case 'l':
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
          break;
        default:
          snprintf(pcError, MESSAGE_SIZE, "%s: Indirect operators must be preceded by a digit (octal/decimal/hex) or a type ([BSLbsl]).", acRoutine);
          return ER;
          break;
        }
        psXMagic->sIndirection.ui32Value = strtoul((pcTmp + 1), &pcEnd, 0);
        if (pcEnd != pcE || errno == ERANGE)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Failed to convert indirect Y value (%s).", acRoutine, (pcTmp + 1));
          return ER;
        }
        switch (*pcTmp)
        {
        case '%':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_MOD;
          if (psXMagic->sIndirection.ui32Value == 0)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: Y value would cause a divide by zero error.", acRoutine);
            return ER;
          }
          break;
        case '&':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_AND;
          break;
        case '*':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_MUL;
          break;
        case '+':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_ADD;
          break;
        case '-':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_SUB;
          break;
        case '/':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_DIV;
          if (psXMagic->sIndirection.ui32Value == 0)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: Y value would cause a divide by zero error.", acRoutine);
            return ER;
          }
          break;
        case '<':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_LSHIFT;
          if (psXMagic->sIndirection.ui32Value > 31) /* This variable is unsigned, thus we assume it can't be < 0. */
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: Y values must be in the range [0-31] for shift operators.", acRoutine);
            return ER;
          }
          break;
        case '>':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_RSHIFT;
          if (psXMagic->sIndirection.ui32Value > 31) /* This variable is unsigned, thus we assume it can't be < 0. */
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: Y values must be in the range [0-31] for shift operators.", acRoutine);
            return ER;
          }
          break;
        case '^':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_XOR;
          break;
        case '|':
          psXMagic->sIndirection.iOperator = XMAGIC_WARP_OP_OR;
          break;
        }
        pcE = pcTmp;
        *pcE = 0; /* Overwrite the operator with a null byte. */
        break;
      }
      pcTmp--;
    }

    /*-
     *******************************************************************
     *
     * Scan backwards looking for [.[BSLbsl]]. This field is optional.
     *
     *******************************************************************
     */
    pcTmp = pcE;
    psXMagic->sIndirection.iType = XMAGIC_LONG; /* Assume host order when a type is not specified. */
    while (pcTmp != pc)
    {
      if (*pcTmp == '.')
      {
        if (*(pcTmp + 1) == 'B')
        {
          psXMagic->sIndirection.iType = XMAGIC_BYTE;
        }
        else if (*(pcTmp + 1) == 'S')
        {
          psXMagic->sIndirection.iType = XMAGIC_BESHORT;
        }
        else if (*(pcTmp + 1) == 'L')
        {
          psXMagic->sIndirection.iType = XMAGIC_BELONG;
        }
        else if (*(pcTmp + 1) == 'b')
        {
          psXMagic->sIndirection.iType = XMAGIC_BYTE;
        }
        else if (*(pcTmp + 1) == 's')
        {
          psXMagic->sIndirection.iType = XMAGIC_LESHORT;
        }
        else if (*(pcTmp + 1) == 'l')
        {
          psXMagic->sIndirection.iType = XMAGIC_LELONG;
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Indirect type must be one of [BSLbsl].", acRoutine);
          return ER;
        }
        pcE = pcTmp;
        *pcE = 0; /* Overwrite the '.' with a null byte. */
        break;
      }
      pcTmp--;
    }
  }

  /*-
   *********************************************************************
   *
   * At this point, all that should be left is the offset. Convert it
   * to a signed 32-bit number. This value is signed because relative
   * offsets can be negative.
   *
   *********************************************************************
   */
  psXMagic->i32XOffset = strtol(pc, &pcEnd, 0);
  if (pcEnd != pcE || errno == ERANGE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Failed to convert X value (%s) offset.", acRoutine, pc);
    return ER;
  }
  if (psXMagic->i32XOffset < 0 && (psXMagic->ui32Flags & XMAGIC_RELATIVE_OFFSET) != XMAGIC_RELATIVE_OFFSET)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Negative X values (%d) are only allowed for relative offsets.", acRoutine, psXMagic->i32XOffset);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicGetTestOperator
 *
 ***********************************************************************
 */
int
XMagicGetTestOperator(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicGetTestOperator()";

  if (psXMagic->iType == XMAGIC_STRING || psXMagic->iType == XMAGIC_PSTRING)
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid pstring/string operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_MD5 ||
    psXMagic->iType == XMAGIC_SHA1 ||
    psXMagic->iType == XMAGIC_SHA256
  )
  {
    if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid hash operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
#ifdef USE_PCRE
  else if (psXMagic->iType == XMAGIC_REGEXP)
  {
    if (strcasecmp(pcS, "=~") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_REGEXP_EQ;
    }
    else if (strcasecmp(pcS, "!~") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_REGEXP_NE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid regexp operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
#endif
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_2 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_2
  )
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "<=") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, ">=") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE;
    }
    else if (strcasecmp(pcS, "[]") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE_AND_LE;
    }
    else if (strcasecmp(pcS, "[)") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE_AND_LT;
    }
    else if (strcasecmp(pcS, "(]") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT_AND_LE;
    }
    else if (strcasecmp(pcS, "()") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT_AND_LT;
    }
    else if (strcasecmp(pcS, "][") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE_OR_GE;
    }
    else if (strcasecmp(pcS, "](") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE_OR_GT;
    }
    else if (strcasecmp(pcS, ")[") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT_OR_GE;
    }
    else if (strcasecmp(pcS, ")(") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT_OR_GT;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid entropy/average/percent operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE
  )
  {
    if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid combo operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
  else
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "<=") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iTestOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, ">=") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE;
    }
    else if (strcasecmp(pcS, "[]") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE_AND_LE;
    }
    else if (strcasecmp(pcS, "[)") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GE_AND_LT;
    }
    else if (strcasecmp(pcS, "(]") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT_AND_LE;
    }
    else if (strcasecmp(pcS, "()") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_GT_AND_LT;
    }
    else if (strcasecmp(pcS, "][") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE_OR_GE;
    }
    else if (strcasecmp(pcS, "](") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LE_OR_GT;
    }
    else if (strcasecmp(pcS, ")[") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT_OR_GE;
    }
    else if (strcasecmp(pcS, ")(") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_LT_OR_GT;
    }
    else if (strcasecmp(pcS, "&") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_AND;
    }
    else if (strcasecmp(pcS, "^") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_XOR;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iTestOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid number operator = [%s]", acRoutine, pcS);
      return ER;
    }
  }
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicGetTestValue
 *
 ***********************************************************************
 */
int
XMagicGetTestValue(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicGetTestValue()";
  char                acString[XMAGIC_STRING_BUFSIZE] = "";
  char               *pcEnd = NULL;
  char               *pcTmp = NULL;
  char               *pcTmpE = NULL;
  char               *pcTmpS = NULL;
  int                 i = 0;
  int                 iConverted = 0;
  int                 iError = 0;
  int                 iLength = 0;

  if (psXMagic->iTestOperator == XMAGIC_OP_NOOP)
  {
    return ER_OK;
  }

  if (psXMagic->iType == XMAGIC_STRING || psXMagic->iType == XMAGIC_PSTRING)
  {
    /*-
     *******************************************************************
     *
     * Grab the test string, which may contain escape sequences and/or
     * octal/hex bytes (including NULL bytes). Reject strings that are
     * longer than the maximum allowed length.
     *
     *******************************************************************
     */
    for (i = 0; pcS < pcE && i < XMAGIC_STRING_BUFSIZE - 1; i++, pcS++)
    {
      if (*pcS == '\\')
      {
        pcS++;
        switch (*pcS)
        {
        case 'a':
          acString[i] = '\a';
          break;
        case 'b':
          acString[i] = '\b';
          break;
        case 'f':
          acString[i] = '\f';
          break;
        case 'n':
          acString[i] = '\n';
          break;
        case 'r':
          acString[i] = '\r';
          break;
        case 't':
          acString[i] = '\t';
          break;
        case 'v':
          acString[i] = '\v';
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          iConverted = XMagicConvert3charOct(pcS, &acString[i]);
          pcS += iConverted - 1;
          break;
        case 'x':
          pcS++;
          iConverted = XMagicConvert2charHex(pcS, &acString[i]);
          switch (iConverted)
          {
          case -2:
            snprintf(pcError, MESSAGE_SIZE, "%s: invalid hex digits = [%c%c]", acRoutine, *pcS, *(pcS + 1));
            return ER;
            break;
          case -1:
            snprintf(pcError, MESSAGE_SIZE, "%s: invalid hex digit = [%c]", acRoutine, *pcS);
            return ER;
            break;
          case 0:
            acString[i] = *pcS;
            break;
          default:
            pcS += iConverted - 1;
            break;
          }
          break;
        default:
          acString[i] = *pcS;
          break;
        }
      }
      else
      {
        acString[i] = *pcS;
      }
    }
    acString[i] = 0;
    if (i > XMAGIC_STRING_BUFSIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: string length > %d", acRoutine, XMAGIC_STRING_BUFSIZE - 1);
      return ER;
    }
    psXMagic->iStringLength = i;
    memcpy(psXMagic->sValue.ui8String, acString, i);
  }
  else if
  (
    psXMagic->iType == XMAGIC_MD5 ||
    psXMagic->iType == XMAGIC_SHA1 ||
    psXMagic->iType == XMAGIC_SHA256
  )
  {
    psXMagic->iStringLength = strlen(pcS);
    if (psXMagic->iStringLength > XMAGIC_STRING_BUFSIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: hash length > %d", acRoutine, XMAGIC_STRING_BUFSIZE - 1);
      return ER;
    }
    strncpy((char *) psXMagic->sValue.ui8String, pcS, psXMagic->iStringLength + 1);
  }
#ifdef USE_PCRE
  else if (psXMagic->iType == XMAGIC_REGEXP)
  {
    const char *pcPcreError = NULL;
    int iError = 0;
    int iPcreErrorOffset = 0;

    /*-
     *******************************************************************
     *
     * Stow away the regular expression and its length.
     *
     *******************************************************************
     */
    psXMagic->iRegExpLength = strlen(pcS);
    if (psXMagic->iRegExpLength > XMAGIC_REGEXP_BUFSIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: regexp length > %d", acRoutine, XMAGIC_REGEXP_BUFSIZE - 1);
      return ER;
    }
    strncpy(psXMagic->acRegExp, pcS, XMAGIC_REGEXP_BUFSIZE);

    /*-
     *******************************************************************
     *
     * Compile and study the regular expression. Compile-time options
     * (?imsx) are not set here because the user can specify them as
     * needed in the magic incantations.
     *
     *******************************************************************
     */
    psXMagic->psPcre = pcre_compile(pcS, 0, &pcPcreError, &iPcreErrorOffset, NULL);
    if (psXMagic->psPcre == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_compile(): %s", acRoutine, pcPcreError);
      return ER;
    }
    psXMagic->psPcreExtra = pcre_study(psXMagic->psPcre, 0, &pcPcreError);
    if (pcPcreError != NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_study(): %s", acRoutine, pcPcreError);
      return ER;
    }
    iError = pcre_fullinfo(psXMagic->psPcre, psXMagic->psPcreExtra, PCRE_INFO_CAPTURECOUNT, (void *) &psXMagic->iCaptureCount);
    if (iError == ER_OK)
    {
      if (psXMagic->iCaptureCount > PCRE_MAX_CAPTURE_COUNT)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Invalid capture count [%d]. The maximum number of capturing '()' subpatterns allowed is %d. Use '(?:)' if grouping is required.", acRoutine, psXMagic->iCaptureCount, PCRE_MAX_CAPTURE_COUNT);
        return ER;
      }
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: pcre_fullinfo(): Unexpected return value [%d]. That shouldn't happen.", acRoutine, iError);
      return ER;
    }
  }
#endif
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_2 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_2
  )
  {
    switch (psXMagic->iTestOperator)
    {
    case XMAGIC_OP_GE_AND_LE:
    case XMAGIC_OP_GE_AND_LT:
    case XMAGIC_OP_GT_AND_LE:
    case XMAGIC_OP_GT_AND_LT:
    case XMAGIC_OP_LE_OR_GE:
    case XMAGIC_OP_LE_OR_GT:
    case XMAGIC_OP_LT_OR_GE:
    case XMAGIC_OP_LT_OR_GT:
      iLength = pcE - pcS;
      pcTmp = calloc(iLength + 1, 1);
      if (pcTmp == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
        return ER;
      }
      strncpy(pcTmp, pcS, iLength + 1);
      pcTmpS = strstr(pcTmp, ",");
      if (pcTmpS == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: missing lower/upper test value delimiter", acRoutine);
        free(pcTmp);
        return ER;
      }
      *pcTmpS++ = 0;
      pcTmpE = pcTmp + iLength;
      psXMagic->sValue.dUpperNumber = strtod(pcTmpS, &pcEnd);
      if (pcEnd != pcTmpE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert upper test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      pcTmpE = pcTmpS - 1;
      pcTmpS = pcTmp;
      psXMagic->sValue.dLowerNumber = strtod(pcTmpS, &pcEnd);
      if (pcEnd != pcTmpE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert lower test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      free(pcTmp);
      break;
    default:
      psXMagic->sValue.dNumber = strtod(pcS, NULL);
      if (errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert test value = [%s]", acRoutine, pcS);
        return ER;
      }
      break;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE
  )
  {
    psXMagic->iStringLength = 0;
    psXMagic->sValue.ui8String[0] = 0;
  }
  else if
  (
    psXMagic->iType == XMAGIC_UI64 ||
    psXMagic->iType == XMAGIC_BEUI64 ||
    psXMagic->iType == XMAGIC_LEUI64 ||
    psXMagic->iType == XMAGIC_WINX_YMDHMS_BEDATE ||
    psXMagic->iType == XMAGIC_WINX_YMDHMS_LEDATE
  )
  {
    switch (psXMagic->iTestOperator)
    {
    case XMAGIC_OP_GE_AND_LE:
    case XMAGIC_OP_GE_AND_LT:
    case XMAGIC_OP_GT_AND_LE:
    case XMAGIC_OP_GT_AND_LT:
    case XMAGIC_OP_LE_OR_GE:
    case XMAGIC_OP_LE_OR_GT:
    case XMAGIC_OP_LT_OR_GE:
    case XMAGIC_OP_LT_OR_GT:
      iLength = pcE - pcS;
      pcTmp = calloc(iLength + 1, 1);
      if (pcTmp == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
        return ER;
      }
      strncpy(pcTmp, pcS, iLength + 1);
      pcTmpS = strstr(pcTmp, ",");
      if (pcTmpS == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: missing lower/upper test value delimiter", acRoutine);
        free(pcTmp);
        return ER;
      }
      *pcTmpS++ = 0;
      pcTmpE = pcTmp + iLength;
      iError = XMagicStringToUi64(pcTmpS, &psXMagic->sValue.ui64UpperNumber);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert upper test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      pcTmpE = pcTmpS - 1;
      pcTmpS = pcTmp;
      iError = XMagicStringToUi64(pcTmpS, &psXMagic->sValue.ui64LowerNumber);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert lower test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      free(pcTmp);
      break;
    default:
      iError = XMagicStringToUi64(pcS, &psXMagic->sValue.ui64Number);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert test value = [%s]", acRoutine, pcS);
        return ER;
      }
      break;
    }
  }
#ifdef USE_KLEL
  else if (psXMagic->iType == XMAGIC_KLELEXP)
  {
    psXMagic->psKlelContext = KlelCompile(pcS, KLEL_MUST_BE_GUARDED_COMMAND, XMagicKlelGetTypeOfVar, XMagicKlelGetValueOfVar, NULL);
    if (!KlelIsValid(psXMagic->psKlelContext))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: KlelCompile(): Failed to compile expression (%s).", acRoutine, KlelGetError(psXMagic->psKlelContext));
      return ER;
    }
    if (strcmp(KlelGetCommandInterpreter(psXMagic->psKlelContext), "concat") != 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: KlelGetCommandInterpreter(): Interpreter = [%s] != [concat]: Interpreter mismatch.", acRoutine, KlelGetCommandInterpreter(psXMagic->psKlelContext));
      return ER;
    }
  }
#endif
  else
  {
    switch (psXMagic->iTestOperator)
    {
    case XMAGIC_OP_GE_AND_LE:
    case XMAGIC_OP_GE_AND_LT:
    case XMAGIC_OP_GT_AND_LE:
    case XMAGIC_OP_GT_AND_LT:
    case XMAGIC_OP_LE_OR_GE:
    case XMAGIC_OP_LE_OR_GT:
    case XMAGIC_OP_LT_OR_GE:
    case XMAGIC_OP_LT_OR_GT:
      iLength = pcE - pcS;
      pcTmp = calloc(iLength + 1, 1);
      if (pcTmp == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
        return ER;
      }
      strncpy(pcTmp, pcS, iLength + 1);
      pcTmpS = strstr(pcTmp, ",");
      if (pcTmpS == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: missing lower/upper test value delimiter", acRoutine);
        free(pcTmp);
        return ER;
      }
      *pcTmpS++ = 0;
      pcTmpE = pcTmp + iLength;
      psXMagic->sValue.ui32UpperNumber = strtoul(pcTmpS, &pcEnd, 0);
      if (pcEnd != pcTmpE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert upper test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      pcTmpE = pcTmpS - 1;
      pcTmpS = pcTmp;
      psXMagic->sValue.ui32LowerNumber = strtoul(pcTmpS, &pcEnd, 0);
      if (pcEnd != pcTmpE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert lower test value = [%s]", acRoutine, pcTmpS);
        free(pcTmp);
        return ER;
      }
      free(pcTmp);
      break;
    default:
      psXMagic->sValue.ui32Number = strtoul(pcS, &pcEnd, 0);
      if (pcEnd != pcE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert test value = [%s]", acRoutine, pcS);
        return ER;
      }
      break;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicGetType
 *
 ***********************************************************************
 */
int
XMagicGetType(char *pcS, char *pcE, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicGetType()";
  char               *pcEnd = NULL;
  char               *pcTmp = pcE;

  /*-
   *********************************************************************
   *
   * Scan backwards looking for a warp operator. This field is optional.
   *
   *********************************************************************
   */
  while (pcTmp != pcS)
  {
    if
    (
      *pcTmp == '%' ||
      *pcTmp == '&' ||
      *pcTmp == '*' ||
      *pcTmp == '+' ||
      *pcTmp == '-' ||
      *pcTmp == '/' ||
      *pcTmp == '<' ||
      *pcTmp == '>' ||
      *pcTmp == '^' ||
      *pcTmp == '|'
    )
    {
      psXMagic->ui32WarpValue = strtoul((pcTmp + 1), &pcEnd, 0);
      if (pcEnd != pcE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Failed to convert warp value (%s).", acRoutine, (pcTmp + 1));
        return ER;
      }
      switch (*pcTmp)
      {
      case '%':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_MOD;
        if (psXMagic->ui32WarpValue == 0)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Warp value would cause a divide by zero error.", acRoutine);
          return ER;
        }
        break;
      case '&':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_AND;
        break;
      case '*':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_MUL;
        break;
      case '+':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_ADD;
        break;
      case '-':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_SUB;
        break;
      case '/':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_DIV;
        if (psXMagic->ui32WarpValue == 0)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Warp value would cause a divide by zero error.", acRoutine);
          return ER;
        }
        break;
      case '<':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_LSHIFT;
        if (psXMagic->ui32WarpValue > 31) /* This variable is unsigned, thus we assume it can't be < 0. */
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Warp values must be in the range [0-31] for shift operators.", acRoutine);
          return ER;
        }
        break;
      case '>':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_RSHIFT;
        if (psXMagic->ui32WarpValue > 31) /* This variable is unsigned, thus we assume it can't be < 0. */
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Warp values must be in the range [0-31] for shift operators.", acRoutine);
          return ER;
        }
        break;
      case '^':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_XOR;
        break;
      case '|':
        psXMagic->iWarpOperator = XMAGIC_WARP_OP_OR;
        break;
      }
      pcE = pcTmp;
      *pcE = 0; /* Overwrite the operator with a null byte. */
      psXMagic->ui32Flags |= XMAGIC_HAVE_WARP;
      break;
    }
    pcTmp--;
  }

  /*-
   *********************************************************************
   *
   * Scan backwards looking for ':'. If one is found, check that there
   * is a valid size to its right. This field is optional.
   *
   *********************************************************************
   */
  pcTmp = pcE;
  while (pcTmp != pcS)
  {
    if (*pcTmp == ':')
    {
      psXMagic->ui32Size = strtoul((pcTmp + 1), &pcEnd, 0);
      if (pcEnd != pcE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Failed to convert type size (%s).", acRoutine, (pcTmp + 1));
        return ER;
      }
      if (psXMagic->ui32Size <= 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Type size must be greater than zero.", acRoutine);
        return ER;
      }
      pcE = pcTmp;
      *pcE = 0; /* overwrite ':' with 0 */
      psXMagic->ui32Flags |= XMAGIC_HAVE_SIZE;
      break;
    }
    pcTmp--;
  }

  /*-
   *********************************************************************
   *
   * Identify the type. Ignore case, but require an exact match.
   *
   *********************************************************************
   */
  if (strcasecmp(pcS, "byte") == 0)
  {
    psXMagic->iType = XMAGIC_BYTE;
  }
  else if (strcasecmp(pcS, "char") == 0)
  {
    psXMagic->iType = XMAGIC_BYTE;
  }
  else if (strcasecmp(pcS, "short") == 0)
  {
    psXMagic->iType = XMAGIC_SHORT;
  }
  else if (strcasecmp(pcS, "long") == 0)
  {
    psXMagic->iType = XMAGIC_LONG;
  }
  else if (strcasecmp(pcS, "string") == 0)
  {
    psXMagic->iType = XMAGIC_STRING;
  }
  else if (strcasecmp(pcS, "md5") == 0)
  {
    psXMagic->iType = XMAGIC_MD5;
  }
#ifdef USE_PCRE
  else if (strcasecmp(pcS, "regexp") == 0)
  {
    psXMagic->iType = XMAGIC_REGEXP;
  }
#endif
  else if (strcasecmp(pcS, "percent_combo_cspdae") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_COMBO_CSPDAE;
  }
  else if (strcasecmp(pcS, "percent_ctype_80_ff") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_80_FF;
  }
  else if (strcasecmp(pcS, "percent_ctype_alnum") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_ALNUM;
  }
  else if (strcasecmp(pcS, "percent_ctype_alpha") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_ALPHA;
  }
  else if (strcasecmp(pcS, "percent_ctype_ascii") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_ASCII;
  }
  else if (strcasecmp(pcS, "percent_ctype_cntrl") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_CNTRL;
  }
  else if (strcasecmp(pcS, "percent_ctype_digit") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_DIGIT;
  }
  else if (strcasecmp(pcS, "percent_ctype_lower") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_LOWER;
  }
  else if (strcasecmp(pcS, "percent_ctype_print") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_PRINT;
  }
  else if (strcasecmp(pcS, "percent_ctype_punct") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_PUNCT;
  }
  else if (strcasecmp(pcS, "percent_ctype_space") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_SPACE;
  }
  else if (strcasecmp(pcS, "percent_ctype_upper") == 0)
  {
    psXMagic->iType = XMAGIC_PERCENT_CTYPE_UPPER;
  }
  else if (strcasecmp(pcS, "pstring") == 0)
  {
    psXMagic->iType = XMAGIC_PSTRING;
  }
  else if (strcasecmp(pcS, "row_average_1") == 0)
  {
    psXMagic->iType = XMAGIC_ROW_AVERAGE_1;
  }
  else if (strcasecmp(pcS, "row_average_2") == 0)
  {
    psXMagic->iType = XMAGIC_ROW_AVERAGE_2;
  }
  else if (strcasecmp(pcS, "row_entropy_1") == 0)
  {
    psXMagic->iType = XMAGIC_ROW_ENTROPY_1;
  }
  else if (strcasecmp(pcS, "row_entropy_2") == 0)
  {
    psXMagic->iType = XMAGIC_ROW_ENTROPY_2;
  }
  else if (strcasecmp(pcS, "sha1") == 0)
  {
    psXMagic->iType = XMAGIC_SHA1;
  }
  else if (strcasecmp(pcS, "sha256") == 0)
  {
    psXMagic->iType = XMAGIC_SHA256;
  }
  else if (strcasecmp(pcS, "date") == 0)
  {
    psXMagic->iType = XMAGIC_DATE;
  }
  else if (strcasecmp(pcS, "beshort") == 0)
  {
    psXMagic->iType = XMAGIC_BESHORT;
  }
  else if (strcasecmp(pcS, "belong") == 0)
  {
    psXMagic->iType = XMAGIC_BELONG;
  }
  else if (strcasecmp(pcS, "bedate") == 0)
  {
    psXMagic->iType = XMAGIC_BEDATE;
  }
  else if (strcasecmp(pcS, "leshort") == 0)
  {
    psXMagic->iType = XMAGIC_LESHORT;
  }
  else if (strcasecmp(pcS, "lelong") == 0)
  {
    psXMagic->iType = XMAGIC_LELONG;
  }
  else if (strcasecmp(pcS, "ledate") == 0)
  {
    psXMagic->iType = XMAGIC_LEDATE;
  }
  else if (strcasecmp(pcS, "unix_ymdhms_bedate") == 0)
  {
    psXMagic->iType = XMAGIC_UNIX_YMDHMS_BEDATE;
  }
  else if (strcasecmp(pcS, "unix_ymdhms_ledate") == 0)
  {
    psXMagic->iType = XMAGIC_UNIX_YMDHMS_LEDATE;
  }
  else if (strcasecmp(pcS, "winx_ymdhms_bedate") == 0)
  {
    psXMagic->iType = XMAGIC_WINX_YMDHMS_BEDATE;
  }
  else if (strcasecmp(pcS, "winx_ymdhms_ledate") == 0)
  {
    psXMagic->iType = XMAGIC_WINX_YMDHMS_LEDATE;
  }
  else if (strcasecmp(pcS, "ui64") == 0)
  {
    psXMagic->iType = XMAGIC_UI64;
  }
  else if (strcasecmp(pcS, "beui64") == 0)
  {
    psXMagic->iType = XMAGIC_BEUI64;
  }
  else if (strcasecmp(pcS, "leui64") == 0)
  {
    psXMagic->iType = XMAGIC_LEUI64;
  }
  else if (strcasecmp(pcS, "nleft") == 0)
  {
    psXMagic->iType = XMAGIC_NLEFT;
  }
#ifdef USE_KLEL
  else if (strcasecmp(pcS, "klelexp") == 0)
  {
    psXMagic->iType = XMAGIC_KLELEXP;
  }
#endif
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid type (%s).", acRoutine, pcS);
    return ER;
  }

  if
  (
    (psXMagic->iType == XMAGIC_STRING || psXMagic->iType == XMAGIC_PSTRING) &&
    (psXMagic->ui32Flags & XMAGIC_HAVE_WARP) == XMAGIC_HAVE_WARP
  )
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Warp values are not allowed for %s types.", acRoutine, pcS);
    return ER;
  }

  if
  (
    ((psXMagic->ui32Flags & XMAGIC_HAVE_WARP) == XMAGIC_HAVE_WARP) &&
    (
      psXMagic->iType == XMAGIC_MD5 ||
      psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER ||
#ifdef USE_PCRE
      psXMagic->iType == XMAGIC_REGEXP ||
#endif
      psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
      psXMagic->iType == XMAGIC_ROW_AVERAGE_2 ||
      psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
      psXMagic->iType == XMAGIC_ROW_ENTROPY_2 ||
      psXMagic->iType == XMAGIC_SHA1 ||
      psXMagic->iType == XMAGIC_SHA256
    )
  )
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Warp values are not allowed for %s types.", acRoutine, pcS);
    return ER;
  }

  if
  (
    ((psXMagic->ui32Flags & XMAGIC_HAVE_SIZE) == XMAGIC_HAVE_SIZE) &&
    !
    (
      psXMagic->iType == XMAGIC_MD5 ||
      psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
      psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER ||
#ifdef USE_PCRE
      psXMagic->iType == XMAGIC_REGEXP ||
#endif
      psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
      psXMagic->iType == XMAGIC_ROW_AVERAGE_2 ||
      psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
      psXMagic->iType == XMAGIC_ROW_ENTROPY_2 ||
      psXMagic->iType == XMAGIC_SHA1 ||
      psXMagic->iType == XMAGIC_SHA256
    )
  )
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Size values are not allowed for %s types.", acRoutine, pcS);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicGetValueOffset
 *
 ***********************************************************************
 */
APP_SI32
XMagicGetValueOffset(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic)
{
  APP_SI32            i32AbsoluteOffset = 0;
  APP_SI32            i32AbsoluteParentOffset = 0;
  APP_SI32            i32AbsoluteXOffset = 0;
  APP_SI32            i32XOffset = 0;
  APP_UI16            ui16ValueTmp = 0;
  APP_UI32            ui32Value = 0;
  APP_UI32            ui32ValueTmp = 0;

  /*-
   *********************************************************************
   *
   * Make sure that the X offset is within range -- i.e., it's located
   * within the boundaries of the buffer.
   *
   *********************************************************************
   */
/* FIXME What should the behavior be if the type being tested is 1, 2, or 3 bytes long? */
  if (psXMagic->i32XOffset > (APP_SI32) (iNRead - sizeof(APP_UI32)))
  {
    return -1; /* The offset is out of range. */
  }

  /*-
   *********************************************************************
   *
   * Determine the parent's absolute offset.
   *
   *********************************************************************
   */
  if (psXMagic->ui32Level && psXMagic->psParent != NULL)
  {
    if
    (
      ((psXMagic->psParent)->ui32Flags & XMAGIC_RELATIVE_OFFSET) == XMAGIC_RELATIVE_OFFSET ||
      ((psXMagic->psParent)->ui32Flags & XMAGIC_INDIRECT_OFFSET) == XMAGIC_INDIRECT_OFFSET
    )
    {
      i32AbsoluteParentOffset = XMagicGetValueOffset(pucBuffer, iNRead, psXMagic->psParent);
      if (i32AbsoluteParentOffset < 0)
      {
        return -1; /* This shouldn't happen -- parent offsets had to be tested to get to this point. */
      }
    }
    else
    {
      i32AbsoluteParentOffset = (psXMagic->psParent)->i32XOffset;
    }
  }
  else
  {
    i32AbsoluteParentOffset = 0;
  }

  /*-
   *********************************************************************
   *
   * Determine the X offset.
   *
   *********************************************************************
   */
  if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_INDIRECT_OFFSET) == XMAGIC_INDIRECT_OFFSET)
  {
    if (psXMagic->i32XOffset < 0) /* Negative indirect offsets are not currently allowed. */
    {
      return -1; /* The offset is out of range. */
    }
    if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_RELATIVE_X_OFFSET) == XMAGIC_RELATIVE_X_OFFSET)
    {
      i32AbsoluteXOffset = i32AbsoluteParentOffset + psXMagic->i32XOffset;
    }
    else
    {
      i32AbsoluteXOffset = psXMagic->i32XOffset;
    }
    switch (psXMagic->sIndirection.iType)
    {
    case XMAGIC_BYTE:
      switch (psXMagic->sIndirection.iOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) % psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) & psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) * psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) + psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) - psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) / psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) << psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) >> psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) ^ psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset)) | psXMagic->sIndirection.ui32Value;
        break;
      default:
        ui32Value = *((APP_UI8 *) (pucBuffer + i32AbsoluteXOffset));
        break;
      }
      ui32Value &= 0x000000ff; /* Mask the result with a value that's appropriate for the type. */
      i32XOffset = (APP_SI32) ui32Value;
      break;
    case XMAGIC_BESHORT:
    case XMAGIC_LESHORT:
      memcpy((unsigned char *) &ui16ValueTmp, &pucBuffer[i32AbsoluteXOffset], 2); /* Align data. */
      switch (psXMagic->sIndirection.iOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) % psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) & psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) * psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) + psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) - psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) / psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) << psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) >> psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) ^ psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType) | psXMagic->sIndirection.ui32Value;
        break;
      default:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->sIndirection.iType);
        break;
      }
      ui32Value &= 0x0000ffff; /* Mask the result with a value that's appropriate for the type. */
      i32XOffset = (APP_SI32) ui32Value;
      break;
    case XMAGIC_BELONG:
    case XMAGIC_LELONG:
    default:
      memcpy((unsigned char *) &ui32ValueTmp, &pucBuffer[i32AbsoluteXOffset], 4); /* Align data. */
      switch (psXMagic->sIndirection.iOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) % psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) & psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) * psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) + psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) - psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) / psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) << psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) >> psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) ^ psXMagic->sIndirection.ui32Value;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType) | psXMagic->sIndirection.ui32Value;
        break;
      default:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->sIndirection.iType);
        break;
      }
      ui32Value &= 0xffffffff; /* Mask the result with a value that's appropriate for the type. */
/* FIXME What should the behavior be for large numbers? */
      i32XOffset = (APP_SI32) ui32Value;
      break;
    }
  }
  else
  {
    i32XOffset = psXMagic->i32XOffset;
  }

  /*-
   *********************************************************************
   *
   * Conditionally adjust the X offset.
   *
   *********************************************************************
   */
  if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_RELATIVE_OFFSET) == XMAGIC_RELATIVE_OFFSET)
  {
    i32XOffset += i32AbsoluteParentOffset;
  }

  /*-
   *********************************************************************
   *
   * Set the absolute offset, and perform a sanity check.
   *
   *********************************************************************
   */
  i32AbsoluteOffset = i32XOffset;
/* FIXME What should the behavior be if the type being tested is 1, 2, or 3 bytes long? */
  if (i32AbsoluteOffset > (APP_SI32) (iNRead - sizeof(APP_UI32)) || i32AbsoluteOffset < 0)
  {
    return -1; /* The offset is out of range. */
  }

  return i32AbsoluteOffset;
}


/*-
 ***********************************************************************
 *
 * XMagicLoadMagic
 *
 ***********************************************************************
 */
XMAGIC *
XMagicLoadMagic(char *pcFilename, char *pcError)
{
  const char          acRoutine[] = "XMagicLoadMagic()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcLine = NULL;
  FILE               *pFile = NULL;
  int                 iError = ER_OK;
  int                 iFinalLineNumber = 0;
  int                 iFirstLineNumber = 0;
  int                 iLineNumber = 0;
  int                 iLinesConsumed = 0;
  int                 iParentExists = 0;
  XMAGIC             *psHead = NULL;
  XMAGIC             *psLast = NULL;
  XMAGIC             *psXMagic = NULL;
  APP_UI32           *pui32ByteOrderMagic = (APP_UI32 *) gaui08ByteOrderMagic;

  giSystemByteOrder = (*pui32ByteOrderMagic == 0x01020304) ? XMAGIC_MSB : XMAGIC_LSB;

  /*-
   *********************************************************************
   *
   * Make sure the file is magical.
   *
   *********************************************************************
   */
  if ((pFile = fopen(pcFilename, "r")) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
    return NULL;
  }
  pcLine = XMagicGetLine(pFile, XMAGIC_MAX_LINE, XMAGIC_PRESERVE_COMMENTS, &iLinesConsumed, acLocalError);
  if (iLinesConsumed < 1)
  {
    iFirstLineNumber = iLineNumber;
    iFinalLineNumber = iLineNumber;
  }
  else
  {
    iFirstLineNumber = iLineNumber + 1;
    iFinalLineNumber = iLineNumber + iLinesConsumed;
  }
  iLineNumber += iLinesConsumed;
  if (pcLine == NULL)
  {
    if (feof(pFile))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: File has no magic header. The first 8 bytes must be \"# XMagic\".", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber);
      iError = ER;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: %s", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber, acLocalError);
      iError = ER;
    }
  }
  else
  {
    if (strncmp(pcLine, "# XMagic", 8) != 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: File has an invalid magic header. The first 8 bytes must be \"# XMagic\".", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber);
      iError = ER;
    }
  }
  if (iError != ER_OK)
  {
    fclose(pFile);
    free(pcLine);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Conjure up a magic tree.
   *
   *********************************************************************
   */
  while (1)
  {
    if (pcLine != NULL)
    {
      free(pcLine);
    }
    pcLine = XMagicGetLine(pFile, XMAGIC_MAX_LINE, 0, &iLinesConsumed, acLocalError);
    if (iLinesConsumed < 1)
    {
      iFirstLineNumber = iLineNumber;
      iFinalLineNumber = iLineNumber;
    }
    else
    {
      iFirstLineNumber = iLineNumber + 1;
      iFinalLineNumber = iLineNumber + iLinesConsumed;
    }
    iLineNumber += iLinesConsumed;
    if (pcLine == NULL)
    {
      if (feof(pFile))
      {
        /* EOF reached. We're done. */
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: %s", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber, acLocalError);
        iError = ER;
      }
      break;
    }
    if (pcLine[0] == 0)
    {
      continue; /* Silently ignore blank lines. */
    }
    psXMagic = XMagicParseLine(pcLine, acLocalError);
    if (psXMagic == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: %s", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber, acLocalError);
      iError = ER;
      break;
    }
    if (psHead == NULL && psLast == NULL)
    {
      if (psXMagic->ui32Level != 0)
      {
        XMagicFreeXMagic(psXMagic);
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: The first test must be a level zero test and use an absolute offset.", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber);
        iError = ER;
        break;
      }
      psXMagic->psParent = NULL;
      psHead = psXMagic;
    }
    else
    {
      /*-
       *****************************************************************
       *
       * If this level equals the last level, we have a sibling. If
       * this level is greater than the last level and the delta
       * between levels is exactly 1, we have a child -- deltas
       * greater than 1 are considered an error. Otherwise, we need to
       * crawl back up the family tree until we find a sibling. If no
       * sibling is found, then we have no parent, and that's just
       * plain wrong.
       *
       *****************************************************************
       */
      if (psXMagic->ui32Level == psLast->ui32Level)
      {
        psXMagic->psParent = psLast->psParent;
        psLast->psSibling = psXMagic;
      }
      else if (psXMagic->ui32Level > psLast->ui32Level)
      {
        if ((psXMagic->ui32Level - psLast->ui32Level) > 1)
        {
          XMagicFreeXMagic(psXMagic);
          snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: One or more test levels skipped.", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber);
          iError = ER;
          break;
        }
        psXMagic->psParent = psLast;
        psLast->psChild = psXMagic;
      }
      else
      {
        iParentExists = 0;
        while ((psLast = psLast->psParent) != NULL)
        {
          if (psXMagic->ui32Level == psLast->ui32Level)
          {
            psXMagic->psParent = psLast->psParent;
            psLast->psSibling = psXMagic;
            iParentExists = 1;
            break;
          }
        }
        if (!iParentExists)
        {
          XMagicFreeXMagic(psXMagic);
          snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Lines = [%d-%d]: Parent magic is missing. That should not happen.", acRoutine, pcFilename, iFirstLineNumber, iFinalLineNumber);
          iError = ER;
          break;
        }
      }
    }
    psLast = psXMagic;
  }
  fclose(pFile);

  /*
   *********************************************************************
   *
   * Abort if there was an error or the magic tree is not defined.
   *
   *********************************************************************
   */
  if (iError != ER_OK)
  {
    XMagicFreeXMagic(psHead);
    return NULL;
  }

  if (psHead == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: Apparently, you have no magic.", acRoutine, pcFilename);
    return NULL;
  }

  return psHead;
}


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelBeLongAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelBeLongAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;

  return KlelCreateInteger
  (
      (pucData[iOffset + 0] << 24)
    | (pucData[iOffset + 1] << 16)
    | (pucData[iOffset + 2] <<  8)
    | (pucData[iOffset + 3] <<  0)
  );
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelBeShortAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelBeShortAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;

  return KlelCreateInteger((pucData[iOffset] << 8) | pucData[iOffset + 1]);
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelByteAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelByteAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;

  return KlelCreateInteger(pucData[iOffset]);
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelComputeRowEntropy1At
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelComputeRowEntropy1At(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iLength = 0;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;
  iLength = (int)ppsArgs[1]->llInteger;

  return KlelCreateReal(XMagicComputeRowEntropy1(&pucData[iOffset], iLength));
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelGetTypeOfVar
 *
 ***********************************************************************
 */
KLEL_EXPR_TYPE
XMagicKlelGetTypeOfVar(const char *pcName, void *pvContext)
{
  int                 i = 0;

  for (i = 0; i < sizeof(gasKlelTypes) / sizeof(gasKlelTypes[0]); i++)
  {
    if (strcmp(gasKlelTypes[i].pcName, pcName) == 0)
    {
      return gasKlelTypes[i].iType;
    }
  }

  return KLEL_TYPE_UNKNOWN;
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelGetValueOfVar
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelGetValueOfVar(const char *pcName, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);

  if (strcmp(pcName, "byte_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64), "byte_at", XMagicKlelByteAt);
  }
  else if (strcmp(pcName, "beshort_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64), "beshort_at", XMagicKlelBeShortAt);
  }
  else if (strcmp(pcName, "leshort_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64), "leshort_at", XMagicKlelLeShortAt);
  }
  else if (strcmp(pcName, "belong_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64), "belong_at", XMagicKlelBeLongAt);
  }
  else if (strcmp(pcName, "lelong_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_INT64_FUNCTION1(KLEL_TYPE_INT64), "lelong_at", XMagicKlelLeLongAt);
  }
  else if (strcmp(pcName, "string_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_STRING_FUNCTION1(KLEL_TYPE_INT64), "string_at", XMagicKlelStringAt);
  }
  else if (strcmp(pcName, "row_entropy_1_at") == 0)
  {
    return KlelCreateFunction(KLEL_TYPE_REAL_FUNCTION2(KLEL_TYPE_INT64, KLEL_TYPE_INT64), "row_entropy_1_at", XMagicKlelComputeRowEntropy1At);
  }
  else if (strcmp(pcName, "f_size") == 0)
  {
    return KlelCreateInteger(psData->iLength);
  }

  return NULL; /* Returning NULL here causes KL-EL to retrieve the value of the specified variable, should it exist in the standard library. */
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelLeLongAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelLeLongAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;

  return KlelCreateInteger
  (
      (pucData[iOffset + 3] << 24)
    | (pucData[iOffset + 2] << 16)
    | (pucData[iOffset + 1] <<  8)
    | (pucData[iOffset + 0] <<  0)
  );
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelLeShortAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelLeShortAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  unsigned char *pucData = psData->pucData;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;

  return KlelCreateInteger((pucData[iOffset + 1] << 8) | pucData[iOffset]);
}
#endif


#ifdef USE_KLEL
/*-
 ***********************************************************************
 *
 * XMagicKlelStringAt
 *
 ***********************************************************************
 */
KLEL_VALUE *
XMagicKlelStringAt(KLEL_VALUE **ppsArgs, void *pvContext)
{
  XMAGIC_DATA_BLOCK *psData = (XMAGIC_DATA_BLOCK *)KlelGetPrivateData((KLEL_CONTEXT *)pvContext);
  char *pcData = (char *)psData->pucData;
  int iLength = 0;
  int iOffset = 0;

  KLEL_ASSERT(ppsArgs           != NULL);
  KLEL_ASSERT(ppsArgs[0]        != NULL);
  KLEL_ASSERT(ppsArgs[1]        == NULL);
  KLEL_ASSERT(ppsArgs[0]->iType == KLEL_EXPR_INTEGER);

  iOffset = (int)ppsArgs[0]->llInteger;
  iLength = psData->iLength - iOffset;

  return KlelCreateString(strnlen(&pcData[iOffset], iLength), &pcData[iOffset]);
}
#endif


/*-
 ***********************************************************************
 *
 * XMagicNewXMagic
 *
 ***********************************************************************
 */
XMAGIC *
XMagicNewXMagic(char *pcError)
{
  const char          acRoutine[] = "XMagicNewXMagic()";
  XMAGIC             *psXMagic = NULL;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the XMagic structure.
   *
   *********************************************************************
   */
  psXMagic = (XMAGIC *) calloc(sizeof(XMAGIC), 1);
  if (psXMagic == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the hash value.
   *
   *********************************************************************
   */
  psXMagic->pcHash = calloc(XMAGIC_MAX_HASH_LENGTH + 1, 1);
  if (psXMagic->pcHash == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the combo buffer.
   *
   *********************************************************************
   */
  psXMagic->pcCombo = calloc(XMAGIC_COMBO_SIZE, 1);
  if (psXMagic->pcCombo == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */
  psXMagic->iTestOperator = XMAGIC_OP_EQ;
  psXMagic->ui32Size = XMAGIC_SUBJECT_BUFSIZE;

  return psXMagic;
}


/*-
 ***********************************************************************
 *
 * XMagicParseLine
 *
 ***********************************************************************
 */
XMAGIC *
XMagicParseLine(char *pcLine, char *pcError)
{
  const char          acRoutine[] = "XMagicParseLine()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcE = pcLine;
  char               *pcS = pcLine;
  int                 iError = 0;
  int                 iEndFound = 0;
  XMAGIC             *psXMagic = NULL;

  psXMagic = XMagicNewXMagic(acLocalError);
  if (psXMagic == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  FIND_DELIMITER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: offset: unexpected NULL found", acRoutine);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  *pcE = 0;

  iError = XMagicGetOffset(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMITER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: type: unexpected NULL found", acRoutine);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  *pcE = 0;

  iError = XMagicGetType(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMITER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: operator: unexpected NULL found", acRoutine);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  *pcE = 0;

  iError = XMagicGetTestOperator(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

#ifdef USE_KLEL
  /*-
   *********************************************************************
   *
   * Conditionally consume all remaining bytes.
   *
   *********************************************************************
   */
  if (psXMagic->iType == XMAGIC_KLELEXP)
  {
    while (*pcE)
    {
      pcE++;
    }
  }
  else
  {
    FIND_DELIMITER(pcE);
  }
#else
  FIND_DELIMITER(pcE);
#endif

  /*-
   *********************************************************************
   *
   * The test field may not be followed by a description field. A NULL
   * here is not necessarily an error.
   *
   *********************************************************************
   */
  if (*pcE == 0)
  {
    iEndFound = 1;
  }
  else
  {
    *pcE = 0;
  }

  iError = XMagicGetTestValue(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

#ifdef USE_KLEL
  /*-
   *********************************************************************
   *
   * This is where we bail out for KL-EL expressions.
   *
   *********************************************************************
   */
  if (psXMagic->iType == XMAGIC_KLELEXP)
  {
    return psXMagic;
  }
#endif

  if (!iEndFound)
  {
    pcE++;
  }

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  while (*pcE)
  {
    pcE++;
  }

  iError = XMagicGetDescription(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    XMagicFreeXMagic(psXMagic);
    return NULL;
  }

  return psXMagic;
}


/*-
 ***********************************************************************
 *
 * XMagicStringToUi64
 *
 ***********************************************************************
 */
int
XMagicStringToUi64(char *pcNumber, APP_UI64 *pui64Value)
{
  int                 i = 0;
  int                 iLength = strlen(pcNumber);
  APP_UI64            ui64 = 0;

  if (iLength < 1 || iLength > 20)
  {
    return ER;
  }

  if (pcNumber[0] == '0' && (pcNumber[1] == 'x' || pcNumber[1] == 'X'))
  {
    if (iLength > 18)
    {
      return ER;
    }
    i = 2;
    while (i < iLength)
    {
      pcNumber[i] = tolower(pcNumber[i]);
      if ((pcNumber[i] >= '0') && (pcNumber[i] <= '9'))
      {
        ui64 = (ui64 << 4) | (pcNumber[i] - '0');
      }
      else if ((pcNumber[i] >= 'a') && (pcNumber[i] <= 'f'))
      {
        ui64 = (ui64 << 4) | ((pcNumber[i] - 'a') + 10);
      }
      else
      {
        return ER;
      }
      i++;
    }
  }
  else
  {
    i = 0;
    while (i < iLength)
    {
      if ((pcNumber[i] >= '0') && (pcNumber[i] <= '9'))
      {
        ui64 = (ui64 * 10) + (pcNumber[i] - '0');
      }
      else
      {
        return ER;
      }
      i++;
    }
  }

  *pui64Value = ui64;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicSwapUi16
 *
 ***********************************************************************
 */
APP_UI16
XMagicSwapUi16(APP_UI16 ui16Value, APP_UI32 ui32MagicType)
{
  if (giSystemByteOrder == XMAGIC_MSB)
  {
    if (ui32MagicType == XMAGIC_LESHORT)
    {
      return (APP_UI16) ((ui16Value >> 8) & 0xff) | ((ui16Value & 0xff) << 8);
    }
  }
  else
  {
    if (ui32MagicType == XMAGIC_BESHORT)
    {
      return (APP_UI16) ((ui16Value >> 8) & 0xff) | ((ui16Value & 0xff) << 8);
    }
  }
  return ui16Value;
}


/*-
 ***********************************************************************
 *
 * XMagicSwapUi32
 *
 ***********************************************************************
 */
APP_UI32
XMagicSwapUi32(APP_UI32 ui32Value, APP_UI32 ui32MagicType)
{
  if (giSystemByteOrder == XMAGIC_MSB)
  {
    if (ui32MagicType == XMAGIC_LELONG || ui32MagicType == XMAGIC_LEDATE || ui32MagicType == XMAGIC_UNIX_YMDHMS_LEDATE)
    {
      return (APP_UI32)
        ((ui32Value >> 24) & 0x00ff) |
        ((ui32Value >>  8) & 0xff00) |
        ((ui32Value & 0xff00) <<  8) |
        ((ui32Value & 0x00ff) << 24)
        ;
    }
  }
  else
  {
    if (ui32MagicType == XMAGIC_BELONG || ui32MagicType == XMAGIC_BEDATE || ui32MagicType == XMAGIC_UNIX_YMDHMS_BEDATE)
    {
      return (APP_UI32)
        ((ui32Value >> 24) & 0x00ff) |
        ((ui32Value >>  8) & 0xff00) |
        ((ui32Value & 0xff00) <<  8) |
        ((ui32Value & 0x00ff) << 24)
        ;
    }
  }
  return ui32Value;
}


/*-
 ***********************************************************************
 *
 * XMagicSwapUi64
 *
 ***********************************************************************
 */
APP_UI64
XMagicSwapUi64(APP_UI64 ui64Value, APP_UI32 ui32MagicType)
{
  if (giSystemByteOrder == XMAGIC_MSB)
  {
    if (ui32MagicType == XMAGIC_WINX_YMDHMS_LEDATE || ui32MagicType == XMAGIC_LEUI64)
    {
      return (APP_UI64)
        ((ui64Value >> 56) & 0x000000ff) |
        ((ui64Value >> 40) & 0x0000ff00) |
        ((ui64Value >> 24) & 0x00ff0000) |
        ((ui64Value >>  8) & 0xff000000) |
        ((ui64Value & 0xff000000) <<  8) |
        ((ui64Value & 0x00ff0000) << 24) |
        ((ui64Value & 0x0000ff00) << 40) |
        ((ui64Value & 0x000000ff) << 56)
        ;
    }
  }
  else
  {
    if (ui32MagicType == XMAGIC_WINX_YMDHMS_BEDATE || ui32MagicType == XMAGIC_BEUI64)
    {
      return (APP_UI64)
        ((ui64Value >> 56) & 0x000000ff) |
        ((ui64Value >> 40) & 0x0000ff00) |
        ((ui64Value >> 24) & 0x00ff0000) |
        ((ui64Value >>  8) & 0xff000000) |
        ((ui64Value & 0xff000000) <<  8) |
        ((ui64Value & 0x00ff0000) << 24) |
        ((ui64Value & 0x0000ff00) << 40) |
        ((ui64Value & 0x000000ff) << 56)
        ;
    }
  }
  return ui64Value;
}


/*-
 ***********************************************************************
 *
 * XMagicTestAverage
 *
 ***********************************************************************
 */
int
XMagicTestAverage(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  switch (psXMagic->iType)
  {
    case XMAGIC_ROW_AVERAGE_1:
      psXMagic->dAverage = XMagicComputeRowAverage1(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength);
      break;
    case XMAGIC_ROW_AVERAGE_2:
      psXMagic->dAverage = XMagicComputeRowAverage2(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength);
      break;
  default:
    return 0;
    break;
  }

  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_LT:
    return (psXMagic->dAverage < psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_LE:
    return (psXMagic->dAverage <= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GT:
    return (psXMagic->dAverage > psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE:
    return (psXMagic->dAverage >= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_EQ:
    return (psXMagic->dAverage == psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_NE:
    return (psXMagic->dAverage != psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE_AND_LE:
    return (psXMagic->dAverage >= psXMagic->sValue.dLowerNumber && psXMagic->dAverage <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GE_AND_LT:
    return (psXMagic->dAverage >= psXMagic->sValue.dLowerNumber && psXMagic->dAverage < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LE:
    return (psXMagic->dAverage > psXMagic->sValue.dLowerNumber && psXMagic->dAverage <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LT:
    return (psXMagic->dAverage > psXMagic->sValue.dLowerNumber && psXMagic->dAverage < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GE:
    return (psXMagic->dAverage <= psXMagic->sValue.dLowerNumber || psXMagic->dAverage >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GT:
    return (psXMagic->dAverage <= psXMagic->sValue.dLowerNumber || psXMagic->dAverage > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GE:
    return (psXMagic->dAverage < psXMagic->sValue.dLowerNumber || psXMagic->dAverage >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GT:
    return (psXMagic->dAverage < psXMagic->sValue.dLowerNumber || psXMagic->dAverage > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


/*-
 ***********************************************************************
 *
 * XMagicTestBuffer
 *
 ***********************************************************************
 */
int
XMagicTestBuffer(XMAGIC *psXMagic, unsigned char *pucBuffer, int iBufferLength, char *pcDescription, int iDescriptionLength, char *pcError)
{
  const char          acRoutine[] = "XMagicTestBuffer()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iBytesLeft = iDescriptionLength - 1;
  int                 iBytesUsed = 0;
  int                 iMatch = 0;

  /*-
   *********************************************************************
   *
   * If the buffer is empty, return a predefined description.
   *
   *********************************************************************
   */
  if (iBufferLength == 0)
  {
    snprintf(pcDescription, iDescriptionLength, "%s", XMAGIC_ISEMPTY);
    return ER_OK;
  }

  /*-
   *********************************************************************
   *
   * If no magic was found, return the default description.
   *
   *********************************************************************
   */
  iMatch = XMagicTestMagic(psXMagic, pucBuffer, iBufferLength, pcDescription, &iBytesUsed, &iBytesLeft, acLocalError);
  switch (iMatch)
  {
  case XMAGIC_TEST_ERROR:
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
    break;
  case XMAGIC_TEST_MATCH:
    pcDescription[iBytesUsed] = 0;
    break;
  default:
    snprintf(pcDescription, iDescriptionLength, "%s", XMAGIC_DEFAULT);
    break;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicTestEntropy
 *
 ***********************************************************************
 */
int
XMagicTestEntropy(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  switch (psXMagic->iType)
  {
    case XMAGIC_ROW_ENTROPY_1:
      psXMagic->dEntropy = XMagicComputeRowEntropy1(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength);
      break;
    case XMAGIC_ROW_ENTROPY_2:
      psXMagic->dEntropy = XMagicComputeRowEntropy2(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength);
      break;
  default:
    return 0;
    break;
  }

  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_LT:
    return (psXMagic->dEntropy < psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_LE:
    return (psXMagic->dEntropy <= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GT:
    return (psXMagic->dEntropy > psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE:
    return (psXMagic->dEntropy >= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_EQ:
    return (psXMagic->dEntropy == psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_NE:
    return (psXMagic->dEntropy != psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE_AND_LE:
    return (psXMagic->dEntropy >= psXMagic->sValue.dLowerNumber && psXMagic->dEntropy <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GE_AND_LT:
    return (psXMagic->dEntropy >= psXMagic->sValue.dLowerNumber && psXMagic->dEntropy < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LE:
    return (psXMagic->dEntropy > psXMagic->sValue.dLowerNumber && psXMagic->dEntropy <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LT:
    return (psXMagic->dEntropy > psXMagic->sValue.dLowerNumber && psXMagic->dEntropy < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GE:
    return (psXMagic->dEntropy <= psXMagic->sValue.dLowerNumber || psXMagic->dEntropy >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GT:
    return (psXMagic->dEntropy <= psXMagic->sValue.dLowerNumber || psXMagic->dEntropy > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GE:
    return (psXMagic->dEntropy < psXMagic->sValue.dLowerNumber || psXMagic->dEntropy >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GT:
    return (psXMagic->dEntropy < psXMagic->sValue.dLowerNumber || psXMagic->dEntropy > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


/*-
 ***********************************************************************
 *
 * XMagicTestFile
 *
 ***********************************************************************
 */
int
XMagicTestFile(XMAGIC *psXMagic, char *pcFilename, char *pcDescription, int iDescriptionLength, char *pcError)
{
  const char          acRoutine[] = "XMagicTestFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FILE               *pFile = NULL;
  int                 iError = 0;
  int                 iNRead = 0;
  unsigned char       aucBuffer[XMAGIC_READ_BUFSIZE] = "";

  /*-
   *********************************************************************
   *
   * Open the specified file.
   *
   *********************************************************************
   */
  if ((pFile = fopen(pcFilename, "rb")) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Read one block of data, and close the file. XMagic only inspects
   * the first block of data.
   *
   *********************************************************************
   */
  iNRead = fread(aucBuffer, 1, XMAGIC_READ_BUFSIZE, pFile);

  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, MESSAGE_SIZE, "%s: fread(): %s", acRoutine, strerror(errno));
    return ER;
  }

  fclose(pFile);

  /*-
   *********************************************************************
   *
   * Check magic.
   *
   *********************************************************************
   */
  iError = XMagicTestBuffer(psXMagic, aucBuffer, iNRead, pcDescription, iDescriptionLength, acLocalError);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicTestMagic
 *
 ***********************************************************************
 */
int
XMagicTestMagic(XMAGIC *psXMagic, unsigned char *pucBuffer, int iNRead, char *pcDescription, int *iBytesUsed, int *iBytesLeft, char *pcError)
{
  const char          acRoutine[] = "XMagicTestMagic()";
  char                acDescriptionLocal[XMAGIC_DESCRIPTION_BUFSIZE] = "";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iChildMatch = 0;
  int                 iLength = 0;
  int                 iMatch = 0;
  int                 iMatches = 0;
  int                 iOriginalBytesUsed = *iBytesUsed;
  int                 iOriginalBytesLeft = *iBytesLeft;
  APP_SI32            i32AbsoluteOffset = 0;
  XMAGIC             *psMyXMagic = NULL;

  /*-
   *********************************************************************
   *
   * Traverse the magic tree. Recursively invoke this routine when a
   * match is found and the child node is defined -- child magic may be
   * used as a logical AND. Sibling magic, if it exists, must be tested
   * whether or not a match was found with the previous sibling. This
   * is necessary to preserve its traditional role as an accumulator.
   * Sibling magic may also be used as a logical OR. However, if a test
   * was defined to run in fallback mode, sibling magic will not be
   * tested at any level except for level zero. In fallback mode, any
   * child test that fails causes the entire test to fail, and any
   * accumulated descriptions are discarded. Likewise, if the test for
   * the deepest child in an ANDed chain passes, the entire test is
   * considered a match.
   *
   *********************************************************************
   */
  for (psMyXMagic = psXMagic; psMyXMagic != NULL; psMyXMagic = psMyXMagic->psSibling)
  {
    /*-
     *******************************************************************
     *
     * Get the current absolute offset. If it's out of range or there
     * was a computational error, it's considered a failed match. The
     * offset can legitimately be out of range if the buffer is too
     * small. If the fallback count is nonzero, clear any child data
     * that has accumulated in the description buffer, and return to
     * the parent. If the current level is zero, there should not be
     * anything in the description buffer, so just continue with the
     * next sibling.
     *
     *******************************************************************
     */
    i32AbsoluteOffset = XMagicGetValueOffset(pucBuffer, iNRead, psMyXMagic);
    if (i32AbsoluteOffset < 0)
    {
      if (psMyXMagic->ui32Level == 0)
      {
        continue; /* This is either mode, and we're at the top of the tree, so continue with the next sibling. */
      }
      else
      {
        if (psMyXMagic->ui32FallbackCount > 0)
        {
          iMatches = 0;
/* FIXME Need to check the number of bytes used/left before updating the buffer. */
          *iBytesUsed = iOriginalBytesUsed;
          *iBytesLeft = iOriginalBytesLeft;
          pcDescription[*iBytesUsed] = 0;
          return XMAGIC_TEST_FALSE; /* This is fallback mode, and we're not at the top of the tree, so return. */
        }
        else
        {
          continue; /* This is accumulate mode, and we're not at the top of the tree, so continue with the next sibling. */
        }
      }
    }

    /*-
     *******************************************************************
     *
     * Test the value.
     *
     *******************************************************************
     */
    iMatch = XMagicTestValue(psMyXMagic, pucBuffer, iNRead, i32AbsoluteOffset, acDescriptionLocal, acLocalError);
    switch (iMatch)
    {
    case XMAGIC_TEST_ERROR:
      if (psMyXMagic->ui32Level == 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
      }
      return XMAGIC_TEST_ERROR;
      break;
    case XMAGIC_TEST_FALSE:
      if (psMyXMagic->ui32Level == 0)
      {
        continue; /* This is either mode, and we're at the top of the tree, so continue with the next sibling. */
      }
      else
      {
        if (psMyXMagic->ui32FallbackCount > 0)
        {
          iMatches = 0;
/* FIXME Need to check the number of bytes used/left before updating the buffer. */
          *iBytesUsed = iOriginalBytesUsed;
          *iBytesLeft = iOriginalBytesLeft;
          pcDescription[*iBytesUsed] = 0;
          return XMAGIC_TEST_FALSE; /* This is fallback mode, and we're not at the top of the tree, so return. */
        }
        else
        {
          continue; /* This is accumulate mode, and we're not at the top of the tree, so continue with the next sibling. */
        }
      }
      break;
    case XMAGIC_TEST_MATCH:
      iMatches++;
/* FIXME Need to check the number of bytes used/left before updating the buffer. */
      iLength = snprintf(&pcDescription[*iBytesUsed], *iBytesLeft, "%s", acDescriptionLocal);
      *iBytesUsed += iLength;
      *iBytesLeft -= iLength;
      if (psMyXMagic->psChild != NULL)
      {
        iChildMatch = XMagicTestMagic(psMyXMagic->psChild, pucBuffer, iNRead, pcDescription, iBytesUsed, iBytesLeft, acLocalError);
        switch (iChildMatch)
        {
        case XMAGIC_TEST_ERROR:
          if (psMyXMagic->ui32Level == 0)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
          }
          else
          {
            snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
          }
          return XMAGIC_TEST_ERROR;
          break;
        case XMAGIC_TEST_FALSE:
          if (psMyXMagic->ui32Level == 0)
          {
            if (psMyXMagic->psChild->ui32FallbackCount > 0)
            {
              iMatches = 0;
/* FIXME Need to check the number of bytes used/left before updating the buffer. */
              *iBytesUsed = iOriginalBytesUsed;
              *iBytesLeft = iOriginalBytesLeft;
              pcDescription[*iBytesUsed] = 0;
              continue; /* This is fallback mode, and we're at the top of the tree, so continue with the next sibling. */
            }
            else
            {
              return XMAGIC_TEST_MATCH; /* This is accumulate mode, and we're at the top of the tree, so the overall result is a match. */
            }
          }
          else
          {
            if (psMyXMagic->ui32FallbackCount > 0)
            {
              iMatches = 0;
/* FIXME Need to check the number of bytes used/left before updating the buffer. */
              *iBytesUsed = iOriginalBytesUsed;
              *iBytesLeft = iOriginalBytesLeft;
              pcDescription[*iBytesUsed] = 0;
              return XMAGIC_TEST_FALSE; /* The is fallback mode, and we're not at the top of the tree, so return. */
            }
            else
            {
              continue; /* This is accumulate mode, and we're not at the top of the tree, so continue with the next sibling. */
            }
          }
          break;
        case XMAGIC_TEST_MATCH:
          if (psMyXMagic->ui32Level == 0)
          {
            return XMAGIC_TEST_MATCH; /* This is either mode, and we're at the top of the tree, so the overall result is a match. */
          }
          else
          {
            if (psMyXMagic->ui32FallbackCount > 0)
            {
              return XMAGIC_TEST_MATCH; /* This is fallback mode, and we're not at the top of the tree, so return. */
            }
            else
            {
              continue; /* This is accumulate mode, and we're not at the top of the tree, so continue with the next sibling. */
            }
          }
          break;
        }
      }
      else
      {
        if (psMyXMagic->ui32Level == 0)
        {
          return XMAGIC_TEST_MATCH; /* This is either mode, and we're at the top of the tree, so the overall result is a match. */
        }
        else
        {
          if (psMyXMagic->ui32FallbackCount > 0)
          {
            return XMAGIC_TEST_MATCH; /* This is fallback mode, and we're not at the top of the tree, so return. */
          }
          else
          {
            continue; /* This is accumulate mode, and we're not at the top of the tree, so continue with the next sibling. */
          }
        }
      }
      break;
    }
  }

  return (iMatches) ? XMAGIC_TEST_MATCH : XMAGIC_TEST_FALSE;
}


/*-
 ***********************************************************************
 *
 * XMagicTestHash
 *
 ***********************************************************************
 */
int
XMagicTestHash(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  unsigned char       aucMd5[MD5_HASH_SIZE] = "";
  unsigned char       aucSha1[SHA1_HASH_SIZE] = "";
  unsigned char       aucSha256[SHA256_HASH_SIZE] = "";
  unsigned char      *puc = NULL;
  int                 i = 0;
  int                 iHashLength = 0;
  int                 n = 0;

  psXMagic->pcHash[0] = 0;

  switch (psXMagic->iType)
  {
  case XMAGIC_MD5:
    MD5HashString(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, aucMd5);
    iHashLength = MD5_HASH_SIZE;
    puc = aucMd5;
    break;
  case XMAGIC_SHA1:
    SHA1HashString(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, aucSha1);
    iHashLength = SHA1_HASH_SIZE;
    puc = aucSha1;
    break;
  case XMAGIC_SHA256:
    SHA256HashString(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, aucSha256);
    iHashLength = SHA256_HASH_SIZE;
    puc = aucSha256;
    break;
  default:
    return 0;
    break;
  }

  for (i = n = 0; i < iHashLength; i++)
  {
    n += sprintf(&psXMagic->pcHash[n], "%02x", puc[i]);
  }
  psXMagic->pcHash[n] = 0;

  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_EQ:
    return (strcasecmp((char *) psXMagic->sValue.ui8String, psXMagic->pcHash) == 0) ? 1 : 0;
    break;
  case XMAGIC_OP_NE:
    return (strcasecmp((char *) psXMagic->sValue.ui8String, psXMagic->pcHash) == 0) ? 0 : 1;
    break;
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


/*-
 ***********************************************************************
 *
 * XMagicTestNumber
 *
 ***********************************************************************
 */
int
XMagicTestNumber(XMAGIC *psXMagic, APP_UI32 ui32Value)
{
  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  case XMAGIC_OP_LT:
    return (ui32Value < psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_LE:
    return (ui32Value <= psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_EQ:
    return (ui32Value == psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_NE:
    return (ui32Value != psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_GT:
    return (ui32Value > psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_GE:
    return (ui32Value >= psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_GE_AND_LE:
    return (ui32Value >= psXMagic->sValue.ui32LowerNumber && ui32Value <= psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_GE_AND_LT:
    return (ui32Value >= psXMagic->sValue.ui32LowerNumber && ui32Value < psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LE:
    return (ui32Value > psXMagic->sValue.ui32LowerNumber && ui32Value <= psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LT:
    return (ui32Value > psXMagic->sValue.ui32LowerNumber && ui32Value < psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GE:
    return (ui32Value <= psXMagic->sValue.ui32LowerNumber || ui32Value >= psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GT:
    return (ui32Value <= psXMagic->sValue.ui32LowerNumber || ui32Value > psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GE:
    return (ui32Value < psXMagic->sValue.ui32LowerNumber || ui32Value >= psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GT:
    return (ui32Value < psXMagic->sValue.ui32LowerNumber || ui32Value > psXMagic->sValue.ui32UpperNumber);
    break;
  case XMAGIC_OP_AND:
    return ((ui32Value & psXMagic->sValue.ui32Number) == psXMagic->sValue.ui32Number);
    break;
  case XMAGIC_OP_XOR:
    return (((ui32Value ^ psXMagic->sValue.ui32Number) & psXMagic->sValue.ui32Number) == psXMagic->sValue.ui32Number);
    break;
  default:
    return 0;
    break;
  }
}


/*-
 ***********************************************************************
 *
 * XMagicTestNumber64
 *
 ***********************************************************************
 */
int
XMagicTestNumber64(XMAGIC *psXMagic, APP_UI64 ui64Value)
{
  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  case XMAGIC_OP_LT:
    return (ui64Value < psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_LE:
    return (ui64Value <= psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_EQ:
    return (ui64Value == psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_NE:
    return (ui64Value != psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_GT:
    return (ui64Value > psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_GE:
    return (ui64Value >= psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_GE_AND_LE:
    return (ui64Value >= psXMagic->sValue.ui64LowerNumber && ui64Value <= psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_GE_AND_LT:
    return (ui64Value >= psXMagic->sValue.ui64LowerNumber && ui64Value < psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LE:
    return (ui64Value > psXMagic->sValue.ui64LowerNumber && ui64Value <= psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LT:
    return (ui64Value > psXMagic->sValue.ui64LowerNumber && ui64Value < psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GE:
    return (ui64Value <= psXMagic->sValue.ui64LowerNumber || ui64Value >= psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GT:
    return (ui64Value <= psXMagic->sValue.ui64LowerNumber || ui64Value > psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GE:
    return (ui64Value < psXMagic->sValue.ui64LowerNumber || ui64Value >= psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GT:
    return (ui64Value < psXMagic->sValue.ui64LowerNumber || ui64Value > psXMagic->sValue.ui64UpperNumber);
    break;
  case XMAGIC_OP_AND:
    return ((ui64Value & psXMagic->sValue.ui64Number) == psXMagic->sValue.ui64Number);
    break;
  case XMAGIC_OP_XOR:
    return (((ui64Value ^ psXMagic->sValue.ui64Number) & psXMagic->sValue.ui64Number) == psXMagic->sValue.ui64Number);
    break;
  default:
    return 0;
    break;
  }
}


/*-
 ***********************************************************************
 *
 * XMagicTestPercent
 *
 ***********************************************************************
 */
int
XMagicTestPercent(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  psXMagic->dPercent = XMagicComputePercentage(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, psXMagic->iType);

  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_LT:
    return (psXMagic->dPercent < psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_LE:
    return (psXMagic->dPercent <= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GT:
    return (psXMagic->dPercent > psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE:
    return (psXMagic->dPercent >= psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_EQ:
    return (psXMagic->dPercent == psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_NE:
    return (psXMagic->dPercent != psXMagic->sValue.dNumber);
    break;
  case XMAGIC_OP_GE_AND_LE:
    return (psXMagic->dPercent >= psXMagic->sValue.dLowerNumber && psXMagic->dPercent <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GE_AND_LT:
    return (psXMagic->dPercent >= psXMagic->sValue.dLowerNumber && psXMagic->dPercent < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LE:
    return (psXMagic->dPercent > psXMagic->sValue.dLowerNumber && psXMagic->dPercent <= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_GT_AND_LT:
    return (psXMagic->dPercent > psXMagic->sValue.dLowerNumber && psXMagic->dPercent < psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GE:
    return (psXMagic->dPercent <= psXMagic->sValue.dLowerNumber || psXMagic->dPercent >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LE_OR_GT:
    return (psXMagic->dPercent <= psXMagic->sValue.dLowerNumber || psXMagic->dPercent > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GE:
    return (psXMagic->dPercent < psXMagic->sValue.dLowerNumber || psXMagic->dPercent >= psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_LT_OR_GT:
    return (psXMagic->dPercent < psXMagic->sValue.dLowerNumber || psXMagic->dPercent > psXMagic->sValue.dUpperNumber);
    break;
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


/*-
 ***********************************************************************
 *
 * XMagicTestPercentCombo
 *
 ***********************************************************************
 */
int
XMagicTestPercentCombo(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  char               *pcCombo = NULL;

  pcCombo = XMagicComputePercentageCombo(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, psXMagic->iType);
  if (pcCombo == NULL)
  {
    psXMagic->pcCombo[0] = 0;
    return 0;
  }
  snprintf(psXMagic->pcCombo, XMAGIC_COMBO_SIZE, "%s", pcCombo);
  free(pcCombo);

  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


#ifdef USE_PCRE
/*-
 ***********************************************************************
 *
 * XMagicTestRegExp
 *
 ***********************************************************************
 */
int
XMagicTestRegExp(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  const char          acRoutine[] = "XMagicTestRegExp()";
  int                 iBytesLeft = iLength - iOffset;
  int                 iError = 0;
  int                 iMatchLength = 0;
  int                 iMatchOffset = 0;
  int                 iPcreOVector[PCRE_OVECTOR_ARRAY_SIZE];

  /*-
   *********************************************************************
   *
   * Initialize output variables.
   *
   *********************************************************************
   */
  psXMagic->iMatchLength = 0;
  memset(psXMagic->aucCapturedData, 0, XMAGIC_REGEXP_CAPTURE_BUFSIZE);

  /*-
   *********************************************************************
   *
   * The PCRE_NOTEMPTY option is used here to squash any attempts to
   * match empty strings (e.g., (A*) or (a?b?)) and to prevent infinite
   * loops. Make sure that the length of the subject string is limited
   * to the lesser of the user-specified size (i.e., regexp:size) or
   * the difference between the user-specified offset and the end of
   * the read buffer.
   *
   *********************************************************************
   */
  iError = pcre_exec(
    psXMagic->psPcre,
    psXMagic->psPcreExtra,
    (char *) (pucBuffer + iOffset),
    (psXMagic->ui32Size < (unsigned) iBytesLeft) ? psXMagic->ui32Size : iBytesLeft,
    0,
    PCRE_NOTEMPTY,
    iPcreOVector,
    PCRE_OVECTOR_ARRAY_SIZE
    );
  if (iError < 0)
  {
    if (iError == PCRE_ERROR_NOMATCH)
    {
      return (psXMagic->iTestOperator == XMAGIC_OP_REGEXP_NE) ? 1 : 0;
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
    if (psXMagic->iCaptureCount == 0)
    {
      iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_0H] - iPcreOVector[PCRE_CAPTURE_INDEX_0L];
      iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_0L];
    }
    else
    {
      iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_1H] - iPcreOVector[PCRE_CAPTURE_INDEX_1L];
      iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_1L];
    }
    if (iMatchLength > XMAGIC_REGEXP_CAPTURE_BUFSIZE) /* Make sure we don't have a capture overflow. */
    {
      char acLocalMessage[MESSAGE_SIZE];
      snprintf(acLocalMessage, MESSAGE_SIZE, "%s: Capture buffer was truncated from %d to %d bytes.", acRoutine, iMatchLength, XMAGIC_REGEXP_CAPTURE_BUFSIZE);
      ErrorHandler(ER_Warning, acLocalMessage, ERROR_WARNING);
      iMatchLength = XMAGIC_REGEXP_CAPTURE_BUFSIZE;
    }
    memcpy(psXMagic->aucCapturedData, &pucBuffer[iOffset + iMatchOffset], iMatchLength);
    psXMagic->iMatchLength = iMatchLength;
    return (psXMagic->iTestOperator == XMAGIC_OP_REGEXP_NE) ? 0 : 1;
  }
}
#endif


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * XMagicTestSpecial
 *
 ***********************************************************************
 */
int
XMagicTestSpecial(char *pcFilename, struct stat *psStatEntry, char *pcDescription, int iDescriptionLength, char *pcError)
{
  const char          acRoutine[] = "XMagicTestSpecial()";
  char               *pcLinkBuffer = NULL;
  int                 n = 0;

  switch (psStatEntry->st_mode & S_IFMT)
  {
  case S_IFIFO:
    snprintf(pcDescription, iDescriptionLength, "special/fifo");
    break;
  case S_IFCHR:
    snprintf(pcDescription, iDescriptionLength, "special/character: major=\"%lu\"; minor=\"%lu\";", (unsigned long) major(psStatEntry->st_rdev), (unsigned long) minor(psStatEntry->st_rdev));
    break;
  case S_IFDIR:
    snprintf(pcDescription, iDescriptionLength, "special/directory");
    break;
  case S_IFBLK:
    snprintf(pcDescription, iDescriptionLength, "special/block: major=\"%lu\"; minor=\"%lu\";", (unsigned long) major(psStatEntry->st_rdev), (unsigned long) minor(psStatEntry->st_rdev));
    break;
  case S_IFREG:
    break;
  case S_IFLNK:
    if ((pcLinkBuffer = (char *) malloc(iDescriptionLength)) == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
      return ER;
    }
    n = readlink(pcFilename, pcLinkBuffer, iDescriptionLength - 1);
    if (n == ER)
    {
      pcDescription[0] = 0;
      snprintf(pcError, MESSAGE_SIZE, "%s: unreadable symbolic link: %s", acRoutine, strerror(errno));
      free(pcLinkBuffer);
      return ER;
    }
    else
    {
      pcLinkBuffer[n] = 0;
      snprintf(pcDescription, iDescriptionLength, "special/symlink: path=\"%s\";", pcLinkBuffer);
      free(pcLinkBuffer);
    }
    break;
  case S_IFSOCK:
    snprintf(pcDescription, iDescriptionLength, "special/socket");
    break;
#ifdef S_IFWHT
  case S_IFWHT:
    snprintf(pcDescription, iDescriptionLength, "special/whiteout");
    break;
#endif
#ifdef S_IFDOOR
  case S_IFDOOR:
    snprintf(pcDescription, iDescriptionLength, "special/door");
    break;
#endif
  default:
    snprintf(pcDescription, iDescriptionLength, "special/unknown");
    return ER;
    break;
  }
  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * XMagicTestString
 *
 ***********************************************************************
 */
int
XMagicTestString(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcError)
{
  unsigned char      *pucSubject = pucBuffer + iOffset;
  unsigned char      *pucTest = psXMagic->sValue.ui8String;
  int                 i = 0;
  int                 iBytesLeft = iLength - iOffset;
  int                 iDelta = 0;
  int                 iStringLength = 0;
  int                 iActualLength = 0;

  /*-
   *********************************************************************
   *
   * The NOOP test operator yields an automatic match.
   *
   *********************************************************************
   */
  if (psXMagic->iTestOperator == XMAGIC_OP_NOOP)
  {
    return 1;
  }

  /*-
   *********************************************************************
   *
   * If there aren't enough bytes left, the match automatically fails.
   * The NULL byte for string types and the length byte for pstring
   * types must be covered by this test.
   *
   *********************************************************************
   */
  if (iBytesLeft < psXMagic->iStringLength + 1)
  {
    return 0;
  }

  /*-
   *********************************************************************
   *
   * Set the string length. For pstring types, this value must be the
   * smaller of the two strings. Also, the subject pointer needs to be
   * advanced to get past the length byte.
   *
   *********************************************************************
   */
  if (psXMagic->iType == XMAGIC_PSTRING)
  {
    iActualLength = (int) *pucSubject;
    iStringLength = (iActualLength < psXMagic->iStringLength) ? iActualLength : psXMagic->iStringLength;
    pucSubject++;
  }
  else
  {
    iStringLength = psXMagic->iStringLength;
  }

  /*-
   *********************************************************************
   *
   * Compare the subject and test strings from left to right by
   * computing the difference between each pair of characters. If the
   * result is nonzero, the loop is terminated, and the delta is used
   * to determine whether there is a match according to the various
   * test operators in the following case statement.
   *
   *********************************************************************
   */
  for (i = 0; i < iStringLength; i++)
  {
    iDelta = ((int) pucSubject[i]) - ((int) pucTest[i]);
    if (iDelta != 0)
    {
      break;
    }
  }

  /*-
   *********************************************************************
   *
   * Do one final check to catch the case where the subject string is
   * a substring of the test string. For string types, this is handled
   * implicitly by the above for-loop, but for pstring types, it must
   * be handled explicitly here. Note that if the test string is a
   * substring of the subject string, it is considered a match. Since
   * pstring types aren't NULL-terminated, a zero is used instead.
   *
   *********************************************************************
   */
  if (i == iStringLength && psXMagic->iType == XMAGIC_PSTRING && iActualLength < psXMagic->iStringLength)
  {
    iDelta = 0 - (int) pucTest[i];
  }

  /*-
   *********************************************************************
   *
   * Use the computed delta to determine if there's a match.
   *
   *********************************************************************
   */
  switch (psXMagic->iTestOperator)
  {
  case XMAGIC_OP_LT:
    return (iDelta < 0);
    break;
  case XMAGIC_OP_EQ:
    return (iDelta == 0);
    break;
  case XMAGIC_OP_NE:
    return (iDelta != 0);
    break;
  case XMAGIC_OP_GT:
    return (iDelta > 0);
    break;
  default:
    return 0;
    break;
  }

  return 0; /* Not reached. */
}


/*-
 ***********************************************************************
 *
 * XMagicTestValue
 *
 ***********************************************************************
 */
int
XMagicTestValue(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, APP_SI32 iOffset, char *pcDescription, char *pcError)
{
  const char          acRoutine[] = "XMagicTestValue()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iMatch = 0;
  APP_UI16            ui16ValueTmp = 0;
  APP_UI32            ui32Value = 0;
  APP_UI32            ui32ValueTmp = 0;
  APP_UI64            ui64Value = 0;
  APP_UI64            ui64ValueTmp = 0;
  void               *pvValue = NULL;

  /*-
   *********************************************************************
   *
   * Check input variables.
   *
   *********************************************************************
   */
  if (iOffset < 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Offset = [%d]: Offsets can't be negative.", acRoutine, iOffset);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Do type checking and any required prep work. Then, test the value.
   *
   *********************************************************************
   */
  if (psXMagic->iType == XMAGIC_STRING || psXMagic->iType == XMAGIC_PSTRING)
  {
    iMatch = XMagicTestString(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    pvValue = (void *) (pucBuffer + iOffset);
  }
  else if
  (
    psXMagic->iType == XMAGIC_MD5 ||
    psXMagic->iType == XMAGIC_SHA1 ||
    psXMagic->iType == XMAGIC_SHA256
  )
  {
    iMatch = XMagicTestHash(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else if (psXMagic->iType == XMAGIC_PERCENT_COMBO_CSPDAE)
  {
    iMatch = XMagicTestPercentCombo(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_80_FF ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->iType == XMAGIC_PERCENT_CTYPE_UPPER
  )
  {
    iMatch = XMagicTestPercent(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
#ifdef USE_PCRE
  else if (psXMagic->iType == XMAGIC_REGEXP)
  {
    iMatch = XMagicTestRegExp(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
#endif
  else if
  (
    psXMagic->iType == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->iType == XMAGIC_ROW_AVERAGE_2
  )
  {
    iMatch = XMagicTestAverage(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->iType == XMAGIC_ROW_ENTROPY_2
  )
  {
    iMatch = XMagicTestEntropy(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else if
  (
    psXMagic->iType == XMAGIC_UNIX_YMDHMS_BEDATE ||
    psXMagic->iType == XMAGIC_UNIX_YMDHMS_LEDATE
  )
  {
    memcpy((unsigned char *) &ui32ValueTmp, &pucBuffer[iOffset], 4); /* Forced alignment. */
    ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType);
    iMatch = XMagicTestNumber(psXMagic, ui32Value);
    pvValue = (void *) &ui32Value;
  }
  else if
  (
    psXMagic->iType == XMAGIC_UI64 ||
    psXMagic->iType == XMAGIC_BEUI64 ||
    psXMagic->iType == XMAGIC_LEUI64 ||
    psXMagic->iType == XMAGIC_WINX_YMDHMS_BEDATE ||
    psXMagic->iType == XMAGIC_WINX_YMDHMS_LEDATE
  )
  {
    memcpy((unsigned char *) &ui64ValueTmp, &pucBuffer[iOffset], 8); /* Forced alignment. */
    ui64Value = XMagicSwapUi64(ui64ValueTmp, psXMagic->iType);
    iMatch = XMagicTestNumber64(psXMagic, ui64Value);
    pvValue = (void *) &ui64Value;
  }
  else if (psXMagic->iType == XMAGIC_NLEFT)
  {
    ui32Value = (APP_UI32) (iLength - iOffset);
    iMatch = XMagicTestNumber(psXMagic, ui32Value);
    pvValue = (void *) &ui32Value;
  }
#ifdef USE_KLEL
  else if (psXMagic->iType == XMAGIC_KLELEXP)
  {
    KLEL_VALUE *psResult = NULL;
    XMAGIC_DATA_BLOCK sData = { pucBuffer, iLength };
    KlelSetPrivateData(psXMagic->psKlelContext, (void *)&sData);
    psResult = KlelExecute(psXMagic->psKlelContext);
    if (psResult == NULL)
    {
      snprintf
      (
        acLocalError,
        MESSAGE_SIZE,
        "%s: KlelExecute(): KlelExp = [%s]: Failed to evaluate expression (%s).",
        acRoutine,
        KlelGetName(psXMagic->psKlelContext),
        KlelGetError(psXMagic->psKlelContext)
      );
      ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
    }
    else
    {
      iMatch = (psResult->bBoolean) ? 1 : 0;
      KlelFreeResult(psResult);
    }
  }
#endif
  else
  {
    switch (psXMagic->iType)
    {
    case XMAGIC_BYTE:
      switch (psXMagic->iWarpOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) % psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) & psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) * psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) + psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) - psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) / psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) << psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) >> psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) ^ psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset)) | psXMagic->ui32WarpValue;
        break;
      default:
        ui32Value = *((APP_UI8 *) (pucBuffer + iOffset));
        break;
      }
      ui32Value &= 0x000000ff; /* Mask the result with a value that's appropriate for the type. */
      break;
    case XMAGIC_SHORT:
      switch (psXMagic->iWarpOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) % psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) & psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) * psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) + psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) - psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) / psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) << psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) >> psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) ^ psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset)) | psXMagic->ui32WarpValue;
        break;
      default:
        ui32Value = *((APP_UI16 *) (pucBuffer + iOffset));
        break;
      }
      ui32Value &= 0x0000ffff; /* Mask the result with a value that's appropriate for the type. */
      break;
    case XMAGIC_LESHORT:
    case XMAGIC_BESHORT:
      memcpy((unsigned char *) &ui16ValueTmp, &pucBuffer[iOffset], 2); /* Forced alignment. */
      switch (psXMagic->iWarpOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) % psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) & psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) * psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) + psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) - psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) / psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) << psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) >> psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) ^ psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType) | psXMagic->ui32WarpValue;
        break;
      default:
        ui32Value = XMagicSwapUi16(ui16ValueTmp, psXMagic->iType);
        break;
      }
      ui32Value &= 0x0000ffff; /* Mask the result with a value that's appropriate for the type. */
      break;
    case XMAGIC_LONG:
    case XMAGIC_DATE:
      switch (psXMagic->iWarpOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) % psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) & psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) * psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) + psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) - psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) / psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) << psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) >> psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) ^ psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset)) | psXMagic->ui32WarpValue;
        break;
      default:
        ui32Value = *((APP_UI32 *) (pucBuffer + iOffset));
        break;
      }
      ui32Value &= 0xffffffff; /* Mask the result with a value that's appropriate for the type. */
      break;
    case XMAGIC_LELONG:
    case XMAGIC_BELONG:
    case XMAGIC_LEDATE:
    case XMAGIC_BEDATE:
      memcpy((unsigned char *) &ui32ValueTmp, &pucBuffer[iOffset], 4); /* Forced alignment. */
      switch (psXMagic->iWarpOperator)
      {
      case XMAGIC_WARP_OP_MOD:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) % psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_AND:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) & psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_MUL:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) * psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_ADD:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) + psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_SUB:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) - psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_DIV:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) / psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_LSHIFT:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) << psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_RSHIFT:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) >> psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_XOR:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) ^ psXMagic->ui32WarpValue;
        break;
      case XMAGIC_WARP_OP_OR:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType) | psXMagic->ui32WarpValue;
        break;
      default:
        ui32Value = XMagicSwapUi32(ui32ValueTmp, psXMagic->iType);
        break;
      }
      ui32Value &= 0xffffffff; /* Mask the result with a value that's appropriate for the type. */
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid type = [%d]", acRoutine, psXMagic->iType);
      return ER;
      break;
    }
    iMatch = XMagicTestNumber(psXMagic, ui32Value);
    pvValue = (void *) &ui32Value;
  }

  /*-
   *********************************************************************
   *
   * Fill out the description.
   *
   *********************************************************************
   */
  if (iMatch)
  {
    XMagicFormatDescription(pvValue, psXMagic, pcDescription);
  }

  return iMatch;
}
