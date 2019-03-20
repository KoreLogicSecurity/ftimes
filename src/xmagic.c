/*-
 ***********************************************************************
 *
 * $Id: xmagic.c,v 1.55 2007/02/23 00:22:36 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
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
#define FIND_DELIMETER(pc) { while (*pc && (!isspace((int) *pc) || *(pc - 1) == '\\')) pc++; }

int                 giSystemByteOrder = -1;
K_UINT08            gaui08ByteOrderMagic[4] = {0x01, 0x02, 0x03, 0x04};

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
  int                 iConverted;
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
  int                 iConverted;

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
  char                acLocalError[MESSAGE_SIZE];
  char                acSafeBuffer[4 * XMAGIC_DESCRIPTION_BUFSIZE];
  char               *pc;
  int                 i;
  int                 iLength;
  int                 iBytesLeft;
  int                 n;
  K_UINT32           *pui32Value;

  pcDescription[0] = n = 0;

  if ((psXMagic->ui32Level > 0) && (psXMagic->acDescription[0] != 0) && ((psXMagic->ui32Flags & XMAGIC_NO_SPACE) != XMAGIC_NO_SPACE))
  {
    pcDescription[0] = ' ';
    pcDescription[1] = 0;
    n = 1;
  }

  iBytesLeft = XMAGIC_DESCRIPTION_BUFSIZE - 1 - n;

  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    char *pcNeutered = NULL;
    /*-
     *******************************************************************
     *
     * If the operator is anything but XMAGIC_OP_EQ, use the first EOL
     * or NULL character as the string terminator. Find the terminator
     * and adjust the string length accordingly, but do not exceed the
     * maximum length (XMAGIC_STRING_BUFSIZE) in any case. After that,
     * neuter the string and trim it to fit in the provided buffer.
     *
     *******************************************************************
     */
    if (psXMagic->iOperator != XMAGIC_OP_EQ)
    {
      for (i = 0, pc = (char *) pvValue; i < XMAGIC_STRING_BUFSIZE; i++, pc++)
      {
        if (*pc == '\0' || *pc == '\n' || *pc == '\r')
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
    pcNeutered = SupportNeuterString((char *) pvValue, iLength, acLocalError);
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
    psXMagic->ui32Type == XMAGIC_MD5 ||
    psXMagic->ui32Type == XMAGIC_SHA1 ||
    psXMagic->ui32Type == XMAGIC_SHA256
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->pcHash);
  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dPercent);
  }
#ifdef USE_PCRE
  else if (psXMagic->ui32Type == XMAGIC_REGEXP)
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
    if (psXMagic->iOperator == XMAGIC_OP_REGEXP_NE)
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
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dAverage);
  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2
  )
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->acDescription, psXMagic->dEntropy);
  }
  else if (psXMagic->ui32Type == XMAGIC_DATE || psXMagic->ui32Type == XMAGIC_LEDATE || psXMagic->ui32Type == XMAGIC_BEDATE)
  {
    for (i = 0, pc = ctime((time_t *) pvValue); *pc; pc++)
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
  else
  {
    pui32Value = (K_UINT32 *) pvValue;
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
    if (psXMagic->psPcre != NULL)
    {
      free(psXMagic->psPcre);
    }
    if (psXMagic->psPcreExtra != NULL)
    {
      free(psXMagic->psPcreExtra);
    }
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
  int                 iLength;

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
    return ER_Length;
  }
  strncpy(psXMagic->acDescription, pcS, XMAGIC_DESCRIPTION_BUFSIZE);

  return ER_OK;
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
  char               *pc;
  char               *pcEnd;
  char               *pcTmp;

  pc = pcS;

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
    return ER_BadValue;
  }

  if (psXMagic->ui32Level > XMAGIC_MAX_LEVEL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Level = [%u]: Level must not exceed %d.", acRoutine, psXMagic->ui32Level, XMAGIC_MAX_LEVEL);
    return ER_BadValue;
  }

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
      snprintf(pcError, MESSAGE_SIZE, "%s: indirection terminator ')' not found", acRoutine);
      return ER_BadValue;
    }

    *pcE = 0;

    /*-
     *******************************************************************
     *
     * Scan backwards looking for [+-]. If one of the two characters is
     * found, check that the character to its left is allowed. If both
     * conditions are met, read in and convert the [y] value.
     *
     *******************************************************************
     */
    pcTmp = pcE;
    while (pcTmp != pc)
    {
      if (*pcTmp == '+' || *pcTmp == '-')
      {
        switch (*(pcTmp - 1)) /* Check the character to the left. */
        {
        case '.': /* case 'B': case 'b': */ case 'S': case 's': case 'L': case 'l':
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
          break;
        default:
          snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert indirect offset [+-] value = [%s]", acRoutine, pc);
          return ER_BadValue;
          break;
        }
        psXMagic->sIndirection.i32YOffset = strtoul(pcTmp, &pcEnd, 0);
        if (pcEnd != pcE || errno == ERANGE)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert indirect offset [+-] value = [%s]", acRoutine, pc);
          return ER_BadValue;
        }
        else
        {
          pcE = pcTmp;
          *pcE = 0;                /* overwrite '+' or '-' with 0 */
          break;
        }
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
    psXMagic->sIndirection.uiType = XMAGIC_LONG; /* Assume host order when a type is not specified. */
    while (pcTmp != pc)
    {
      if (*pcTmp == '.')
      {
        if (*(pcTmp + 1) == 'B')
        {
          psXMagic->sIndirection.uiType = XMAGIC_BYTE;
        }
        else if (*(pcTmp + 1) == 'S')
        {
          psXMagic->sIndirection.uiType = XMAGIC_BESHORT;
        }
        else if (*(pcTmp + 1) == 'L')
        {
          psXMagic->sIndirection.uiType = XMAGIC_BELONG;
        }
        else if (*(pcTmp + 1) == 'b')
        {
          psXMagic->sIndirection.uiType = XMAGIC_BYTE;
        }
        else if (*(pcTmp + 1) == 's')
        {
          psXMagic->sIndirection.uiType = XMAGIC_LESHORT;
        }
        else if (*(pcTmp + 1) == 'l')
        {
          psXMagic->sIndirection.uiType = XMAGIC_LELONG;
        }
        else
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: invalid indirect type", acRoutine);
          return ER_BadValue;
        }
        pcE = pcTmp;
        *pcE = 0;                /* overwrite '.' with 0 */
        break;
      }
      pcTmp--;
    }
  }

  if ((*pc == '+' || *pc == '&') && psXMagic->ui32Level > 0)
  {
    psXMagic->ui32Flags |= XMAGIC_RELATIVE_OFFSET;
    pc++;
  }

  psXMagic->i32XOffset = strtol(pc, &pcEnd, 0); /* Use strtol() here because relative offsets can be negative. */
  if (pcEnd != pcE || errno == ERANGE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert offset = [%s]", acRoutine, pc);
    return ER_BadValue;
  }
  if (psXMagic->i32XOffset < 0 && (psXMagic->ui32Flags & XMAGIC_RELATIVE_OFFSET) != XMAGIC_RELATIVE_OFFSET)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: negative x values [%d] are not allowed for anything but relative offsets", acRoutine, psXMagic->i32XOffset);
    return ER_BadValue;
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

  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid string operator = [%s]", acRoutine, pcS);
      return ER_BadMagicOperator;
    }
  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_MD5 ||
    psXMagic->ui32Type == XMAGIC_SHA1 ||
    psXMagic->ui32Type == XMAGIC_SHA256
  )
  {
    if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid hash operator = [%s]", acRoutine, pcS);
      return ER_BadMagicOperator;
    }
  }
#ifdef USE_PCRE
  else if (psXMagic->ui32Type == XMAGIC_REGEXP)
  {
    if (strcasecmp(pcS, "=~") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_REGEXP_EQ;
    }
    else if (strcasecmp(pcS, "!~") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_REGEXP_NE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid regexp operator = [%s]", acRoutine, pcS);
      return ER_BadMagicOperator;
    }
  }
#endif
  else if
  (
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2
  )
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "<=") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LE;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, ">=") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GE;
    }
    else if (strcasecmp(pcS, "[]") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GE_AND_LE;
    }
    else if (strcasecmp(pcS, "[)") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GE_AND_LT;
    }
    else if (strcasecmp(pcS, "(]") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GT_AND_LE;
    }
    else if (strcasecmp(pcS, "()") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GT_AND_LT;
    }
    else if (strcasecmp(pcS, "][") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LE_OR_GE;
    }
    else if (strcasecmp(pcS, "](") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LE_OR_GT;
    }
    else if (strcasecmp(pcS, ")[") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LT_OR_GE;
    }
    else if (strcasecmp(pcS, ")(") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LT_OR_GT;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid entropy/average/percent operator = [%s]", acRoutine, pcS);
      return ER_BadMagicOperator;
    }
  }
  else
  {
    if (strcasecmp(pcS, "<") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LT;
    }
    else if (strcasecmp(pcS, "<=") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_LE;
    }
    else if (strcasecmp(pcS, "!=") == 0 || strcasecmp(pcS, "!") == 0) /* NOTE: The '!' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_NE;
    }
    else if (strcasecmp(pcS, "==") == 0 || strcasecmp(pcS, "=") == 0) /* NOTE: The '=' operator is being phased out. */
    {
      psXMagic->iOperator = XMAGIC_OP_EQ;
    }
    else if (strcasecmp(pcS, ">") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GT;
    }
    else if (strcasecmp(pcS, ">=") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_GE;
    }
    else if (strcasecmp(pcS, "&") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_AND;
    }
    else if (strcasecmp(pcS, "^") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_XOR;
    }
    else if (strcasecmp(pcS, "x") == 0)
    {
      psXMagic->iOperator = XMAGIC_OP_NOOP;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid number operator = [%s]", acRoutine, pcS);
      return ER_BadMagicOperator;
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
  char                acString[4 * XMAGIC_STRING_BUFSIZE];
  char               *pcEnd;
  char               *pcTmp;
  char               *pcTmpE;
  char               *pcTmpS;
  int                 i;
  int                 iConverted;
  int                 iLength;

  if (psXMagic->iOperator == XMAGIC_OP_NOOP)
  {
    return ER_OK;
  }

  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    /*-
     *******************************************************************
     *
     * Grab the test string, and process escape sequences.
     *
     *******************************************************************
     */
    i = 0;
    while (pcS < pcE && i < XMAGIC_STRING_BUFSIZE - 1)
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
            return ER_BadValue;
            break;
          case -1:
            snprintf(pcError, MESSAGE_SIZE, "%s: invalid hex digit = [%c]", acRoutine, *pcS);
            return ER_BadValue;
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
      i++;
      pcS++;
    }
    acString[i] = 0;

    if (i > XMAGIC_STRING_BUFSIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: string length > %d", acRoutine, XMAGIC_STRING_BUFSIZE - 1);
      return ER_BadValue;
    }

    psXMagic->iStringLength = i;
    memcpy(psXMagic->sValue.ui08String, acString, i);

  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_MD5 ||
    psXMagic->ui32Type == XMAGIC_SHA1 ||
    psXMagic->ui32Type == XMAGIC_SHA256
  )
  {
    psXMagic->iStringLength = strlen(pcS);
    if (psXMagic->iStringLength > XMAGIC_STRING_BUFSIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: hash length > %d", acRoutine, XMAGIC_STRING_BUFSIZE - 1);
      return ER;
    }
    strncpy(psXMagic->sValue.ui08String, pcS, psXMagic->iStringLength + 1);
  }
#ifdef USE_PCRE
  else if (psXMagic->ui32Type == XMAGIC_REGEXP)
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
     * Compile and study the regular expression. Then, make sure that
     * there's exactly one capturing subpattern. Compile-time options
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
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2
  )
  {
    switch (psXMagic->iOperator)
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
  else
  {
    psXMagic->sValue.ui32Number = strtoul(pcS, &pcEnd, 0);
    if (pcEnd != pcE || errno == ERANGE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert test value = [%s]", acRoutine, pcS);
      return ER_BadValue;
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
  char               *pcEnd;
  char               *pcTmp;

  /*-
   *********************************************************************
   *
   * Scan backwards looking for a '&' mask. This field is optional.
   *
   *********************************************************************
   */
  pcTmp = pcE;
  while (pcTmp != pcS)
  {
    if (*pcTmp == '&')
    {
      psXMagic->ui32Mask = strtoul((pcTmp + 1), &pcEnd, 0);
      if (pcEnd != pcE || errno == ERANGE)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert type mask = [%s]", acRoutine, (pcTmp + 1));
        return ER_BadValue;
      }
      pcE = pcTmp;
      *pcE = 0; /* overwrite '&' with 0 */
      psXMagic->ui32Flags |= XMAGIC_HAVE_MASK;
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
        snprintf(pcError, MESSAGE_SIZE, "%s: failed to convert type size = [%s]", acRoutine, (pcTmp + 1));
        return ER_BadValue;
      }
      if (psXMagic->ui32Size <= 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: type size must be greater than zero", acRoutine);
        return ER_BadValue;
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
    psXMagic->ui32Type = XMAGIC_BYTE;
  }
  else if (strcasecmp(pcS, "char") == 0)
  {
    psXMagic->ui32Type = XMAGIC_BYTE;
  }
  else if (strcasecmp(pcS, "short") == 0)
  {
    psXMagic->ui32Type = XMAGIC_SHORT;
  }
  else if (strcasecmp(pcS, "long") == 0)
  {
    psXMagic->ui32Type = XMAGIC_LONG;
  }
  else if (strcasecmp(pcS, "string") == 0)
  {
    psXMagic->ui32Type = XMAGIC_STRING;
  }
  else if (strcasecmp(pcS, "md5") == 0)
  {
    psXMagic->ui32Type = XMAGIC_MD5;
  }
#ifdef USE_PCRE
  else if (strcasecmp(pcS, "regexp") == 0)
  {
    psXMagic->ui32Type = XMAGIC_REGEXP;
  }
#endif
  else if (strcasecmp(pcS, "percent_ctype_alnum") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_ALNUM;
  }
  else if (strcasecmp(pcS, "percent_ctype_alpha") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_ALPHA;
  }
  else if (strcasecmp(pcS, "percent_ctype_ascii") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_ASCII;
  }
  else if (strcasecmp(pcS, "percent_ctype_cntrl") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_CNTRL;
  }
  else if (strcasecmp(pcS, "percent_ctype_digit") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_DIGIT;
  }
  else if (strcasecmp(pcS, "percent_ctype_lower") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_LOWER;
  }
  else if (strcasecmp(pcS, "percent_ctype_print") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_PRINT;
  }
  else if (strcasecmp(pcS, "percent_ctype_punct") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_PUNCT;
  }
  else if (strcasecmp(pcS, "percent_ctype_space") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_SPACE;
  }
  else if (strcasecmp(pcS, "percent_ctype_upper") == 0)
  {
    psXMagic->ui32Type = XMAGIC_PERCENT_CTYPE_UPPER;
  }
  else if (strcasecmp(pcS, "row_average_1") == 0)
  {
    psXMagic->ui32Type = XMAGIC_ROW_AVERAGE_1;
  }
  else if (strcasecmp(pcS, "row_average_2") == 0)
  {
    psXMagic->ui32Type = XMAGIC_ROW_AVERAGE_2;
  }
  else if (strcasecmp(pcS, "row_entropy_1") == 0)
  {
    psXMagic->ui32Type = XMAGIC_ROW_ENTROPY_1;
  }
  else if (strcasecmp(pcS, "row_entropy_2") == 0)
  {
    psXMagic->ui32Type = XMAGIC_ROW_ENTROPY_2;
  }
  else if (strcasecmp(pcS, "sha1") == 0)
  {
    psXMagic->ui32Type = XMAGIC_SHA1;
  }
  else if (strcasecmp(pcS, "sha256") == 0)
  {
    psXMagic->ui32Type = XMAGIC_SHA256;
  }
  else if (strcasecmp(pcS, "date") == 0)
  {
    psXMagic->ui32Type = XMAGIC_DATE;
  }
  else if (strcasecmp(pcS, "beshort") == 0)
  {
    psXMagic->ui32Type = XMAGIC_BESHORT;
  }
  else if (strcasecmp(pcS, "belong") == 0)
  {
    psXMagic->ui32Type = XMAGIC_BELONG;
  }
  else if (strcasecmp(pcS, "bedate") == 0)
  {
    psXMagic->ui32Type = XMAGIC_BEDATE;
  }
  else if (strcasecmp(pcS, "leshort") == 0)
  {
    psXMagic->ui32Type = XMAGIC_LESHORT;
  }
  else if (strcasecmp(pcS, "lelong") == 0)
  {
    psXMagic->ui32Type = XMAGIC_LELONG;
  }
  else if (strcasecmp(pcS, "ledate") == 0)
  {
    psXMagic->ui32Type = XMAGIC_LEDATE;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: invalid type = [%s]", acRoutine, pcS);
    return ER_BadMagicType;
  }

  if (psXMagic->ui32Type == XMAGIC_STRING && (psXMagic->ui32Flags & XMAGIC_HAVE_MASK) == XMAGIC_HAVE_MASK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Mask values are not allowed for %s types.", acRoutine, pcS);
    return ER_BadValue;
  }

  if
  (
    ((psXMagic->ui32Flags & XMAGIC_HAVE_MASK) == XMAGIC_HAVE_MASK) &&
    (
      psXMagic->ui32Type == XMAGIC_MD5 ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER ||
#ifdef USE_PCRE
      psXMagic->ui32Type == XMAGIC_REGEXP ||
#endif
      psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
      psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2 ||
      psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
      psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2 ||
      psXMagic->ui32Type == XMAGIC_SHA1 ||
      psXMagic->ui32Type == XMAGIC_SHA256
    )
  )
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Mask values are not allowed for %s types.", acRoutine, pcS);
    return ER_BadValue;
  }

  if
  (
    ((psXMagic->ui32Flags & XMAGIC_HAVE_SIZE) == XMAGIC_HAVE_SIZE) &&
    !
    (
      psXMagic->ui32Type == XMAGIC_MD5 ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
      psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER ||
#ifdef USE_PCRE
      psXMagic->ui32Type == XMAGIC_REGEXP ||
#endif
      psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
      psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2 ||
      psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
      psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2 ||
      psXMagic->ui32Type == XMAGIC_SHA1 ||
      psXMagic->ui32Type == XMAGIC_SHA256
    )
  )
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Size values are not allowed for %s types.", acRoutine, pcS);
    return ER_BadValue;
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
K_INT32
XMagicGetValueOffset(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic)
{
  K_INT32             i32AbsoluteOffset = 0;
  K_UINT32            ui32Value = 0;
  K_UINT16            ui16Value = 0;

  if (psXMagic->i32XOffset > (K_INT32) (iNRead - sizeof(K_UINT32)))
  {
    return -1; /* The offset is out of range. */
  }

  if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_INDIRECT_OFFSET) == XMAGIC_INDIRECT_OFFSET)
  {
    if (psXMagic->i32XOffset < 0) /* Negative indirect offsets are not currently allowed. */
    {
      return -1; /* The offset is out of range. */
    }
    switch (psXMagic->sIndirection.uiType)
    {
    case XMAGIC_BYTE:
      i32AbsoluteOffset = psXMagic->sIndirection.i32YOffset + *(K_UINT08 *) (pucBuffer + psXMagic->i32XOffset);
      break;
    case XMAGIC_BESHORT:
    case XMAGIC_LESHORT:
      memcpy((unsigned char *) &ui16Value, &pucBuffer[psXMagic->i32XOffset], 2); /* Align data. */
      i32AbsoluteOffset = psXMagic->sIndirection.i32YOffset + XMagicSwapShort(ui16Value, psXMagic->sIndirection.uiType);
      break;
    case XMAGIC_BELONG:
    case XMAGIC_LELONG:
    default:
      memcpy((unsigned char *) &ui32Value, &pucBuffer[psXMagic->i32XOffset], 4); /* Align data. */
      i32AbsoluteOffset = psXMagic->sIndirection.i32YOffset + XMagicSwapLong(ui32Value, psXMagic->sIndirection.uiType);
      break;
    }
  }
  else if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_RELATIVE_OFFSET) == XMAGIC_RELATIVE_OFFSET)
  {
    if (
         psXMagic->psParent != NULL &&
         (
           ((psXMagic->psParent)->ui32Flags & XMAGIC_RELATIVE_OFFSET) == XMAGIC_RELATIVE_OFFSET ||
           ((psXMagic->psParent)->ui32Flags & XMAGIC_INDIRECT_OFFSET) == XMAGIC_INDIRECT_OFFSET
         )
       )
    {
      K_INT32 i32AbsoluteParentOffset = 0;
      i32AbsoluteParentOffset = XMagicGetValueOffset(pucBuffer, iNRead, psXMagic->psParent);
      if (i32AbsoluteParentOffset < 0)
      {
        return -1; /* This shouldn't happen -- parent offsets had to be tested to get to this point. */
      }
      i32AbsoluteOffset = psXMagic->i32XOffset + i32AbsoluteParentOffset;
    }
    else
    {
      i32AbsoluteOffset = psXMagic->i32XOffset + (psXMagic->psParent)->i32XOffset;
    }
  }
  else
  {
    i32AbsoluteOffset = psXMagic->i32XOffset;
  }

  if (i32AbsoluteOffset > (K_INT32) (iNRead - sizeof(K_UINT32)) || i32AbsoluteOffset < 0)
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
  char                acLine[XMAGIC_MAX_LINE];
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE               *pFile;
  int                 iLineNumber;
  int                 iParentExists;
  XMAGIC             *psHead = NULL;
  XMAGIC             *psLast = NULL;
  XMAGIC             *psXMagic = NULL;

  giSystemByteOrder = (*(K_UINT32 *) gaui08ByteOrderMagic == 0x01020304) ? XMAGIC_MSB : XMAGIC_LSB;

  if ((pFile = fopen(pcFilename, "r")) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
    return NULL;
  }

  for (acLine[0] = 0, iLineNumber = 1; fgets(acLine, XMAGIC_MAX_LINE, pFile) != NULL; acLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Check the file's magic.
     *
     *******************************************************************
     */
    if (iLineNumber == 1 && strncmp(acLine, "# XMagic", 8) != 0)
    {
      fclose(pFile);
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: magic != [# XMagic]", acRoutine, pcFilename, iLineNumber);
      XMagicFreeXMagic(psHead);
      return NULL;
    }

    /*-
     *******************************************************************
     *
     * Ignore full line comments (i.e. '#' in position 0).
     *
     *******************************************************************
     */
    if (acLine[0] == '#')
    {
      continue;
    }

    /*-
     *******************************************************************
     *
     * Remove EOL characters.
     *
     *******************************************************************
     */
    if (SupportChopEOLs(acLine, feof(pFile) ? 0 : 1, acLocalError) == ER)
    {
      fclose(pFile);
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      XMagicFreeXMagic(psHead);
      return NULL;
    }

    /*-
     *******************************************************************
     *
     * If there's anything left over, process it.
     *
     *******************************************************************
     */
    if (strlen(acLine) > 0)
    {
      psXMagic = XMagicNewXMagic(acLocalError);
      if (psXMagic == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        XMagicFreeXMagic(psHead);
        return NULL;
      }

      if (XMagicParseLine(acLine, psXMagic, acLocalError) != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
        XMagicFreeXMagic(psHead);
        XMagicFreeXMagic(psXMagic);
        return NULL;
      }

      if (psHead == NULL && psLast == NULL)
      {
        if (psXMagic->ui32Level != 0)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: The first test must be a level zero test and use an absolute offset.", acRoutine, pcFilename, iLineNumber);
          XMagicFreeXMagic(psHead);
          XMagicFreeXMagic(psXMagic);
          return NULL;
        }
        psXMagic->psParent = NULL;
        psHead = psXMagic;
      }
      else
      {

        /*-
         ***************************************************************
         *
         * If psXMagic->ui32Level == psLast->ui32Level, then we have a
         * sibling.
         *
         ***************************************************************
         */
        if (psXMagic->ui32Level == psLast->ui32Level)
        {
          psXMagic->psParent = psLast->psParent;
          psLast->psSibling = psXMagic;
        }

        /*-
         ***************************************************************
         *
         * If psXMagic->ui32Level > psLast->ui32Level and the delta == 1,
         * then we have a child.
         *
         ***************************************************************
         */
        else if (psXMagic->ui32Level > psLast->ui32Level)
        {
          if ((psXMagic->ui32Level - psLast->ui32Level) > 1)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: test level(s) skipped", acRoutine, pcFilename, iLineNumber);
            XMagicFreeXMagic(psHead);
            XMagicFreeXMagic(psXMagic);
            return NULL;
          }
          psXMagic->psParent = psLast;
          psLast->psChild = psXMagic;
        }

        /*-
         ***************************************************************
         *
         * Otherwise, we need to crawl back up the family tree until we
         * find a sibling. If no sibling was found, then we have no
         * parent, and that's just plain wrong.
         *
         ***************************************************************
         */
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
            snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: missing parent magic", acRoutine, pcFilename, iLineNumber);
            XMagicFreeXMagic(psHead);
            XMagicFreeXMagic(psXMagic);
            return NULL;
          }
        }
      }
      psLast = psXMagic;
    }
  }
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    XMagicFreeXMagic(psHead);
    return NULL;
  }
  fclose(pFile);

  return psHead;
}


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

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */
  psXMagic->ui32Mask = ~0;
  psXMagic->iOperator = XMAGIC_OP_EQ;
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
int
XMagicParseLine(char *pcLine, XMAGIC *psXMagic, char *pcError)
{
  const char          acRoutine[] = "XMagicParseLine()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcE;
  char               *pcS;
  int                 iError;
  int                 iEndFound = 0;

  pcE = pcS = pcLine;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: offset: unexpected NULL found", acRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetOffset(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: type: unexpected NULL found", acRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetType(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: operator: unexpected NULL found", acRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetTestOperator(pcS, pcE, psXMagic, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMETER(pcE);

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
    return iError;
  }

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
    return iError;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicSwapLong
 *
 ***********************************************************************
 */
K_UINT32
XMagicSwapLong(K_UINT32 ui32Value, K_UINT32 ui32MagicType)
{
  if (giSystemByteOrder == XMAGIC_MSB)
  {
    if (ui32MagicType == XMAGIC_LELONG || ui32MagicType == XMAGIC_LEDATE)
    {
      return (K_UINT32) ((ui32Value >> 24) & 0xff) | ((ui32Value >> 8) & 0xff00) | ((ui32Value & 0xff00) << 8) | ((ui32Value & 0xff) << 24);
    }
  }
  else
  {
    if (ui32MagicType == XMAGIC_BELONG || ui32MagicType == XMAGIC_BEDATE)
    {
      return (K_UINT32) ((ui32Value >> 24) & 0xff) | ((ui32Value >> 8) & 0xff00) | ((ui32Value & 0xff00) << 8) | ((ui32Value & 0xff) << 24);
    }
  }
  return ui32Value;
}


/*-
 ***********************************************************************
 *
 * XMagicSwapShort
 *
 ***********************************************************************
 */
K_UINT16
XMagicSwapShort(K_UINT16 ui16Value, K_UINT32 ui32MagicType)
{
  if (giSystemByteOrder == XMAGIC_MSB)
  {
    if (ui32MagicType == XMAGIC_LESHORT)
    {
      return (K_UINT16) ((ui16Value >> 8) & 0xff) | ((ui16Value & 0xff) << 8);
    }
  }
  else
  {
    if (ui32MagicType == XMAGIC_BESHORT)
    {
      return (K_UINT16) ((ui16Value >> 8) & 0xff) | ((ui16Value & 0xff) << 8);
    }
  }
  return ui16Value;
}


/*-
 ***********************************************************************
 *
 * XMagicTestAverage
 *
 ***********************************************************************
 */
int
XMagicTestAverage(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  switch (psXMagic->ui32Type)
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

  switch (psXMagic->iOperator)
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
    return (psXMagic->dAverage > psXMagic->sValue.dLowerNumber && psXMagic->dAverage <= psXMagic->sValue.dUpperNumber);
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iBytesLeft;
  int                 iBytesUsed;
  int                 iMatch;

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
  iBytesUsed = 0;
  iBytesLeft = iDescriptionLength - 1;
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
XMagicTestEntropy(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  switch (psXMagic->ui32Type)
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

  switch (psXMagic->iOperator)
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
    return (psXMagic->dEntropy > psXMagic->sValue.dLowerNumber && psXMagic->dEntropy <= psXMagic->sValue.dUpperNumber);
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE               *pFile;
  int                 iError;
  int                 iNRead;
  unsigned char       aucBuffer[XMAGIC_READ_BUFSIZE];

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
    return ER_fopen;
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
    return ER_fread;
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
    return ER_XMagic;
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
  char                acDescriptionLocal[XMAGIC_DESCRIPTION_BUFSIZE];
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iChildMatch = 0;
  int                 iLength = 0;
  int                 iMatch = 0;
  int                 iMatches = 0;
  int                 iOriginalBytesUsed = *iBytesUsed;
  int                 iOriginalBytesLeft = *iBytesLeft;
  K_INT32            i32AbsoluteOffset = 0;
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
XMagicTestHash(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  unsigned char       aucMd5[MD5_HASH_SIZE];
  unsigned char       aucSha1[SHA1_HASH_SIZE];
  unsigned char       aucSha256[SHA256_HASH_SIZE];
  unsigned char      *puc;
  int                 i;
  int                 iHashLength;
  int                 n;

  psXMagic->pcHash[0] = 0;

  switch (psXMagic->ui32Type)
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

  switch (psXMagic->iOperator)
  {
  case XMAGIC_OP_EQ:
    return (strcasecmp(psXMagic->sValue.ui08String, psXMagic->pcHash) == 0) ? 1 : 0;
    break;
  case XMAGIC_OP_NE:
    return (strcasecmp(psXMagic->sValue.ui08String, psXMagic->pcHash) == 0) ? 0 : 1;
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
XMagicTestNumber(K_UINT32 ui32Value1, K_UINT32 ui32Value2, int iOperator)
{
  switch (iOperator)
  {
  case XMAGIC_OP_NOOP:
    return 1;
    break;
  case XMAGIC_OP_LT:
    return (ui32Value1 < ui32Value2);
    break;
  case XMAGIC_OP_LE:
    return (ui32Value1 <= ui32Value2);
    break;
  case XMAGIC_OP_EQ:
    return (ui32Value1 == ui32Value2);
    break;
  case XMAGIC_OP_NE:
    return (ui32Value1 != ui32Value2);
    break;
  case XMAGIC_OP_GT:
    return (ui32Value1 > ui32Value2);
    break;
  case XMAGIC_OP_GE:
    return (ui32Value1 >= ui32Value2);
    break;
  case XMAGIC_OP_AND:
    return ((ui32Value1 & ui32Value2) == ui32Value2);
    break;
  case XMAGIC_OP_XOR:
    return (((ui32Value1 ^ ui32Value2) & ui32Value2) == ui32Value2);
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
XMagicTestPercent(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  psXMagic->dPercent = XMagicComputePercentage(&pucBuffer[iOffset], ((int) psXMagic->ui32Size <= iLength) ? psXMagic->ui32Size : iLength, psXMagic->ui32Type);

  switch (psXMagic->iOperator)
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
    return (psXMagic->dPercent > psXMagic->sValue.dLowerNumber && psXMagic->dPercent <= psXMagic->sValue.dUpperNumber);
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


#ifdef USE_PCRE
/*-
 ***********************************************************************
 *
 * XMagicTestRegExp
 *
 ***********************************************************************
 */
int
XMagicTestRegExp(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  const char          acRoutine[] = "XMagicTestRegExp()";
  int                 iError = 0;
  int                 iMatchLength = 0;
  int                 iMatchOffset = 0;
#ifndef PCRE_OVECTOR_ARRAY_SIZE
#define PCRE_OVECTOR_ARRAY_SIZE 30
#endif
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
    pucBuffer + iOffset,
    (psXMagic->ui32Size < (unsigned) (iLength - iOffset)) ? psXMagic->ui32Size : iLength - iOffset,
    0,
    PCRE_NOTEMPTY,
    iPcreOVector,
    PCRE_OVECTOR_ARRAY_SIZE
    );
  if (iError < 0)
  {
    if (iError == PCRE_ERROR_NOMATCH)
    {
      return (psXMagic->iOperator == XMAGIC_OP_REGEXP_NE) ? 1 : 0;
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
    if (psXMagic->iCaptureCount == 1)
    {
      iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_1H] - iPcreOVector[PCRE_CAPTURE_INDEX_1L];
      iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_1L];
    }
    else
    {
      iMatchLength = iPcreOVector[PCRE_CAPTURE_INDEX_0H] - iPcreOVector[PCRE_CAPTURE_INDEX_0L];
      iMatchOffset = iPcreOVector[PCRE_CAPTURE_INDEX_0L];
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
    return (psXMagic->iOperator == XMAGIC_OP_REGEXP_NE) ? 0 : 1;
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
  char               *pcLinkBuffer;
  int                 n;

  switch (psStatEntry->st_mode & S_IFMT)
  {
  case S_IFIFO:
    snprintf(pcDescription, iDescriptionLength, "named pipe (fifo)");
    break;
  case S_IFCHR:
    snprintf(pcDescription, iDescriptionLength, "character special (%lu/%lu)", (unsigned long) major(psStatEntry->st_rdev), (unsigned long) minor(psStatEntry->st_rdev));
    break;
  case S_IFDIR:
    snprintf(pcDescription, iDescriptionLength, "directory");
    break;
  case S_IFBLK:
    snprintf(pcDescription, iDescriptionLength, "block special (%lu/%lu)", (unsigned long) major(psStatEntry->st_rdev), (unsigned long) minor(psStatEntry->st_rdev));
    break;
  case S_IFREG:
    break;
  case S_IFLNK:
    if ((pcLinkBuffer = (char *) malloc(iDescriptionLength)) == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
      return ER_BadHandle;
    }
    n = readlink(pcFilename, pcLinkBuffer, iDescriptionLength - 1);
    if (n == ER)
    {
      pcDescription[0] = 0;
      snprintf(pcError, MESSAGE_SIZE, "%s: unreadable symbolic link: %s", acRoutine, strerror(errno));
      free(pcLinkBuffer);
      return ER_readlink;
    }
    else
    {
      pcLinkBuffer[n] = 0;
      snprintf(pcDescription, iDescriptionLength, "symbolic link to %s", pcLinkBuffer);
      free(pcLinkBuffer);
    }
    break;
  case S_IFSOCK:
    snprintf(pcDescription, iDescriptionLength, "socket");
    break;
#ifdef S_IFWHT
  case S_IFWHT:
    snprintf(pcDescription, iDescriptionLength, "whiteout");
    break;
#endif
#ifdef S_IFDOOR
  case S_IFDOOR:
    snprintf(pcDescription, iDescriptionLength, "door");
    break;
#endif
  default:
    snprintf(pcDescription, iDescriptionLength, "unknown");
    return ER_Special;
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
XMagicTestString(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcError)
{
  unsigned char      *pucSubject;
  unsigned char      *pucTest;
  int                 iDelta;
  int                 iStringLength;

  /*-
   *********************************************************************
   *
   * The NOOP operator yields an automatic match.
   *
   *********************************************************************
   */
  if (psXMagic->iOperator == XMAGIC_OP_NOOP)
  {
    return 1;
  }

  /*-
   *********************************************************************
   *
   * If the number of bytes left in the read buffer at the specified
   * offset is less than the length of the specified test string, the
   * match automatically fails. Otherwise, the subject and test
   * strings are compared from left to right -- one pair of characters
   * at a time. If a mismatch is found, the difference between the
   * subject and test characters is computed, and the test loop is
   * terminated. The delta value is used to determine whether or not
   * there's a match in the following case statement.
   *
   *********************************************************************
   */
  iStringLength = (psXMagic->iStringLength < iLength - iOffset) ? psXMagic->iStringLength : iLength - iOffset;
  if (iStringLength < psXMagic->iStringLength)
  {
    return 0;
  }

  iDelta = 0;
  pucSubject = pucBuffer + iOffset;
  pucTest = psXMagic->sValue.ui08String;
  while (iStringLength > 0)
  {
    if (*pucSubject != *pucTest)
    {
      iDelta = *pucSubject - *pucTest;
      break;
    }
    pucSubject++;
    pucTest++;
    iStringLength--;
  }

  switch (psXMagic->iOperator)
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
XMagicTestValue(XMAGIC *psXMagic, unsigned char *pucBuffer, int iLength, K_INT32 iOffset, char *pcDescription, char *pcError)
{
  const char          acRoutine[] = "XMagicTestValue()";
  char                acLocalError[MESSAGE_SIZE];
  int                 iMatch = 0;
  K_UINT32            ui32Value;
  K_UINT32            ui32ValueTmp;
  K_UINT16            ui16ValueTmp;
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
  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    iMatch = XMagicTestString(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    pvValue = (void *) (pucBuffer + iOffset);
  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_MD5 ||
    psXMagic->ui32Type == XMAGIC_SHA1 ||
    psXMagic->ui32Type == XMAGIC_SHA256
  )
  {
    iMatch = XMagicTestHash(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else if
  (
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALNUM ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ALPHA ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_ASCII ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_CNTRL ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_DIGIT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_LOWER ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PRINT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_PUNCT ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_SPACE ||
    psXMagic->ui32Type == XMAGIC_PERCENT_CTYPE_UPPER
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
  else if (psXMagic->ui32Type == XMAGIC_REGEXP)
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
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_AVERAGE_2
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
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_1 ||
    psXMagic->ui32Type == XMAGIC_ROW_ENTROPY_2
  )
  {
    iMatch = XMagicTestEntropy(psXMagic, pucBuffer, iLength, iOffset, acLocalError);
    if (iMatch == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }
  else
  {
    switch (psXMagic->ui32Type)
    {
    case XMAGIC_BYTE:
      ui32Value = psXMagic->ui32Mask & *(K_UINT08 *) (pucBuffer + iOffset);
      break;
    case XMAGIC_SHORT:
      ui32Value = psXMagic->ui32Mask & *(K_UINT16 *) (pucBuffer + iOffset);
      break;
    case XMAGIC_LESHORT:
    case XMAGIC_BESHORT:
      memcpy((unsigned char *) &ui16ValueTmp, &pucBuffer[iOffset], 2); /* Forced alignment. */
      ui32Value = psXMagic->ui32Mask & XMagicSwapShort(ui16ValueTmp, psXMagic->ui32Type);
      break;
    case XMAGIC_LONG:
    case XMAGIC_DATE:
      ui32Value = psXMagic->ui32Mask & *(K_UINT32 *) (pucBuffer + iOffset);
      break;
    case XMAGIC_LELONG:
    case XMAGIC_BELONG:
    case XMAGIC_LEDATE:
    case XMAGIC_BEDATE:
      memcpy((unsigned char *) &ui32ValueTmp, &pucBuffer[iOffset], 4); /* Forced alignment. */
      ui32Value = psXMagic->ui32Mask & XMagicSwapLong(ui32ValueTmp, psXMagic->ui32Type);
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: invalid type = [%d]", acRoutine, psXMagic->ui32Type);
      return ER;
      break;
    }
    iMatch = XMagicTestNumber(ui32Value, psXMagic->sValue.ui32Number, psXMagic->iOperator);
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
