/*-
 ***********************************************************************
 *
 * $Id: xmagic.c,v 1.6 2003/02/23 17:40:09 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
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

int                 SystemByteOrder = -1;
K_UINT08            ByteOrderMagic[4] = {0x01, 0x02, 0x03, 0x04};
XMAGIC             *XMagicTree;


/*-
 ***********************************************************************
 *
 * XMagicCompareNumbers
 *
 ***********************************************************************
 */
int
XMagicCompareNumbers(K_UINT32 ui32Value1, K_UINT32 ui32Value2, char cOperator)
{
  switch (cOperator)
  {
  case 'x':
    return 1;
    break;
  case '<':
    return (ui32Value1 < ui32Value2);
    break;
  case '=':
    return (ui32Value1 == ui32Value2);
    break;
  case '>':
    return (ui32Value1 > ui32Value2);
    break;
  case '!':
    return (ui32Value1 != ui32Value2);
    break;
  case '&':
    return ((ui32Value1 & ui32Value2) == ui32Value2);
    break;
  case '^':
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
 * XMagicCompareStrings
 *
 ***********************************************************************
 */
int
XMagicCompareStrings(unsigned char *pucS1, unsigned char *pucS2, int iLength, char cOperator)
{
  int                 iDelta;

  if (cOperator == 'x')
  {
    return 1;
  }

  for (iDelta = 0; iLength > 0; pucS1++, pucS2++, iLength--)
  {
    if (*pucS1 != *pucS2)
    {
      iDelta = *pucS1 - *pucS2;
      break;
    }
  }

  switch (cOperator)
  {
  case '<':
    return (iDelta < 0);
    break;
  case '=':
    return (iDelta == 0);
    break;
  case '>':
    return (iDelta > 0);
    break;
  case '!':
    return (iDelta);
    break;
  default:
    return 0;
    break;
  }
}


/*-
 ***********************************************************************
 *
 * XMagicCompareValues
 *
 ***********************************************************************
 */
int
XMagicCompareValues(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, K_UINT32 ui32Offset, char *pcDescription, char *pcError)
{
  int                 iMatch;
  K_UINT32            ui32Value;
  K_UINT32            ui32ValueTmp;
  K_UINT16            ui16ValueTmp;

  iMatch = 0;

  switch (psXMagic->ui32Type)
  {
  case XMAGIC_BYTE:
    ui32Value = psXMagic->ui32Mask & *(K_UINT08 *) (pucBuffer + ui32Offset);
    break;

  case XMAGIC_SHORT:
    ui32Value = psXMagic->ui32Mask & *(K_UINT16 *) (pucBuffer + ui32Offset);
    break;

  case XMAGIC_LESHORT:
  case XMAGIC_BESHORT:

    /*-
     *******************************************************************
     *
     * An alignment may or may not be necessary, so do it in all cases.
     *
     *******************************************************************
     */
    memcpy((unsigned char *) &ui16ValueTmp, &pucBuffer[ui32Offset], 2);
    ui32Value = psXMagic->ui32Mask & XMagicSwapShort(ui16ValueTmp, psXMagic->ui32Type);
    break;

  case XMAGIC_LONG:
  case XMAGIC_DATE:
    ui32Value = psXMagic->ui32Mask & *(K_UINT32 *) (pucBuffer + ui32Offset);
    break;

  case XMAGIC_LELONG:
  case XMAGIC_BELONG:
  case XMAGIC_LEDATE:
  case XMAGIC_BEDATE:

    /*-
     *******************************************************************
     *
     * An alignment may or may not be necessary, so do it in all cases.
     *
     *******************************************************************
     */
    memcpy((unsigned char *) &ui32ValueTmp, &pucBuffer[ui32Offset], 4);
    ui32Value = psXMagic->ui32Mask & XMagicSwapLong(ui32ValueTmp, psXMagic->ui32Type);
    break;

  case XMAGIC_STRING:
    ui32Value = (K_UINT32) (pucBuffer + ui32Offset);
    break;

  default:
    snprintf(pcError, ERRBUF_SIZE, "XMagicCompareValues(): invalid type");
    return ER_BadMagicType;
    break;
  }

  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    iMatch = XMagicCompareStrings((unsigned char *) ui32Value, psXMagic->sValue.ui08String, psXMagic->iStringLength, psXMagic->cOperator);
  }
  else
  {
    iMatch = XMagicCompareNumbers(ui32Value, psXMagic->sValue.ui32Number, psXMagic->cOperator);
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
    XMagicFormatDescription(ui32Value, psXMagic, pcDescription);
  }

  return iMatch;
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
  unsigned int        uiResult1;
  unsigned int        uiResult2;

  uiResult1 = uiResult2 = 0;

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
XMagicFormatDescription(K_UINT32 ui32Value, XMAGIC *psXMagic, char *pcDescription)
{
  char                cSafeBuffer[4 * XMAGIC_DESCRIPTION_BUFSIZE];
  char               *pc;
  int                 i;
  int                 iBytesLeft;
  int                 n;

  pcDescription[0] = n = 0;

  if ((psXMagic->ui32Level > 0) && (psXMagic->cDescription[0] != 0) && ((psXMagic->ui32Flags & XMAGIC_NO_SPACE) != XMAGIC_NO_SPACE))
  {
    pcDescription[0] = ' ';
    pcDescription[1] = 0;
    n = 1;
  }

  iBytesLeft = XMAGIC_DESCRIPTION_BUFSIZE - 1 - n;

  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    if (psXMagic->cOperator == '=')
    {
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->cDescription, psXMagic->sValue.ui08String);
    }
    else
    {

      /*-
       *****************************************************************
       *
       * Read from the buffer pointed to by value until NULL or EOL.
       * Convert any non-desirable characters to their hex representation
       * on the way. Do not exceed iBytesLeft in any case.
       *
       *****************************************************************
       */
      for (i = 0, pc = (unsigned char *) ui32Value; *pc && *pc != '\n' && *pc != '\r' && i < (iBytesLeft - 4); pc++)
      {
        if (*pc >= 0x20 && *pc <= 0x7e && *pc != '|')
        {
          cSafeBuffer[i++] = *pc;
        }
        else
        {
          cSafeBuffer[i++] = '<';
          i += sprintf(&cSafeBuffer[i], "%02x", (unsigned char) *pc);
          cSafeBuffer[i++] = '>';
        }
      }
      cSafeBuffer[i] = 0;
      n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->cDescription, cSafeBuffer);
    }
  }
  else if (psXMagic->ui32Type == XMAGIC_DATE || psXMagic->ui32Type == XMAGIC_LEDATE || psXMagic->ui32Type == XMAGIC_BEDATE)
  {
    for (i = 0, pc = ctime((time_t *) &ui32Value); *pc; pc++)
    {
      if (*pc == '\r' || *pc == '\n')
      {
        cSafeBuffer[i++] = 0;
        break;
      }
      else
      {
        cSafeBuffer[i++] = *pc;
      }
    }
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->cDescription, cSafeBuffer);
  }
  else
  {
    n += snprintf(&pcDescription[n], iBytesLeft, psXMagic->cDescription, ui32Value);
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
  const char          cRoutine[] = "XMagicGetDescription()";
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Description length exceeds %d bytes.", cRoutine, XMAGIC_DESCRIPTION_BUFSIZE - 1);
    return ER_Length;
  }
  strncpy(psXMagic->cDescription, pcS, XMAGIC_DESCRIPTION_BUFSIZE);

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
  const char          cRoutine[] = "XMagicGetOffset()";
  char               *pc;
  char               *pcEnd;
  char               *pcTmp;

  pc = pcS;

  while (*pc == '>')
  {
    psXMagic->ui32Level++;
    pc++;
  }

  if (psXMagic->ui32Level > XMAGIC_MAX_LEVEL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Level = [%lu]: Level must not exceed %d.", cRoutine, psXMagic->ui32Level, XMAGIC_MAX_LEVEL);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: indirection terminator ')' not found", cRoutine);
      return ER_BadValue;
    }

    *pcE = 0;

    /*-
     *******************************************************************
     *
     * Scan backwards looking for a [+-] value. This field is optional.
     *
     *******************************************************************
     */
    pcTmp = pcE;
    while (pcTmp != pc)
    {
      if (*pcTmp == '+' || *pcTmp == '-')
      {
        psXMagic->sIndirection.ui32YOffset = strtoul(pcTmp, &pcEnd, 0);
        if (pcEnd != pcE || errno == ERANGE)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: failed to convert indirect offset [+-] value = [%s]", cRoutine, pc);
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
     * Now backup and look for [.bsl] type. This field is optional.
     *
     *******************************************************************
     */
    pcTmp = pcE;
    psXMagic->sIndirection.ui32BSL = XMAGIC_LONG;        /* default value */
    while (pcTmp != pc)
    {
      if (*pcTmp == '.')
      {
        if (*(pcTmp + 1) == 'b')
        {
          psXMagic->sIndirection.ui32BSL = XMAGIC_BYTE;
        }
        else if (*(pcTmp + 1) == 's')
        {
          psXMagic->sIndirection.ui32BSL = XMAGIC_SHORT;
        }
        else if (*(pcTmp + 1) == 'l')
        {
          psXMagic->sIndirection.ui32BSL = XMAGIC_LONG;
        }
        else
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: invalid indirect type", cRoutine);
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

  psXMagic->ui32XOffset = strtoul(pc, &pcEnd, 0);
  if (pcEnd != pcE || errno == ERANGE)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: failed to convert offset = [%s]", cRoutine, pc);
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
  if (psXMagic->ui32Type == XMAGIC_STRING)
  {
    switch (*pcS)
    {
    case '<':
    case '=':
    case '>':
    case 'x':
      psXMagic->cOperator = *pcS;
      break;
    default:
      psXMagic->cOperator = '=';
      break;
    }
  }
  else
  {
    switch (*pcS)
    {
    case '<':
    case '!':
    case '=':
    case '>':
    case '&':
    case '^':
      psXMagic->cOperator = *pcS;
      break;
    case 'x':
      psXMagic->cOperator = *pcS;
      break;
    default:
      psXMagic->cOperator = '=';
      break;
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
  const char          cRoutine[] = "XMagicGetTestValue()";
  char                cString[4 * XMAGIC_STRING_BUFSIZE];
  char               *pcEnd;
  int                 i;
  int                 iConverted;

  if (psXMagic->cOperator == 'x')
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
          cString[i] = '\a';
          break;
        case 'b':
          cString[i] = '\b';
          break;
        case 'f':
          cString[i] = '\f';
          break;
        case 'n':
          cString[i] = '\n';
          break;
        case 'r':
          cString[i] = '\r';
          break;
        case 't':
          cString[i] = '\t';
          break;
        case 'v':
          cString[i] = '\v';
          break;
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
          iConverted = XMagicConvert3charOct(pcS, &cString[i]);
          pcS += iConverted - 1;
          break;
        case 'x':
          pcS++;
          iConverted = XMagicConvert2charHex(pcS, &cString[i]);
          switch (iConverted)
          {
          case -2:
            snprintf(pcError, ERRBUF_SIZE, "%s: invalid hex digits = [%c%c]", cRoutine, *pcS, *(pcS + 1));
            return ER_BadValue;
            break;
          case -1:
            snprintf(pcError, ERRBUF_SIZE, "%s: invalid hex digit = [%c]", cRoutine, *pcS);
            return ER_BadValue;
            break;
          case 0:
            cString[i] = *pcS;
            break;
          default:
            pcS += iConverted - 1;
            break;
          }
          break;
        default:
          cString[i] = *pcS;
          break;
        }
      }
      else
      {
        cString[i] = *pcS;
      }
      i++;
      pcS++;
    }
    cString[i] = 0;

    if (i > XMAGIC_STRING_BUFSIZE - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: string length > %d", cRoutine, XMAGIC_STRING_BUFSIZE - 1);
      return ER_BadValue;
    }

    psXMagic->iStringLength = i;
    memcpy(psXMagic->sValue.ui08String, cString, i);

  }
  else
  {
    psXMagic->sValue.ui32Number = strtoul(pcS, &pcEnd, 0);
    if (pcEnd != pcE || errno == ERANGE)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: failed to convert test value = [%s]", cRoutine, pcS);
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
  const char          cRoutine[] = "XMagicGetType()";
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
        snprintf(pcError, ERRBUF_SIZE, "%s: failed to convert type mask = [%s]", cRoutine, (pcTmp + 1));
        return ER_BadValue;
      }
      pcE = pcTmp;
      *pcE = 0;                        /* overwrite '&' with 0 */
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
    snprintf(pcError, ERRBUF_SIZE, "%s: invalid type = [%s]", cRoutine, pcS);
    return ER_BadMagicType;
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
K_UINT32
XMagicGetValueOffset(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, char *pcError)
{
  K_UINT32            ui32Value;
  K_UINT32            ui32Offset;
  K_UINT16            ui16Value;

  if (psXMagic->ui32XOffset > iNRead - sizeof(K_UINT32))
  {

    /*-
     *******************************************************************
     *
     * The offset is out of range.
     *
     *******************************************************************
     */
    return -1;
  }

  if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_INDIRECT_OFFSET) == XMAGIC_INDIRECT_OFFSET)
  {
    switch (psXMagic->sIndirection.ui32BSL)
    {
    case XMAGIC_BYTE:
      ui32Offset = psXMagic->sIndirection.ui32YOffset + *(K_UINT08 *) (pucBuffer + psXMagic->ui32XOffset);
      break;
    case XMAGIC_SHORT:

      /*-
       *****************************************************************
       *
       * An alignment may or may not be necessary, so do it in all cases.
       *
       *****************************************************************
       */
      memcpy((unsigned char *) &ui16Value, &pucBuffer[psXMagic->ui32XOffset], 2);
      ui32Offset = psXMagic->sIndirection.ui32YOffset + XMagicSwapShort(ui16Value, psXMagic->ui32Type);
      break;
    case XMAGIC_LONG:
    default:

      /*-
       *****************************************************************
       *
       * An alignment may or may not be necessary, so do it in all
       * cases.
       *
       *****************************************************************
       */
      memcpy((unsigned char *) &ui32Value, &pucBuffer[psXMagic->ui32XOffset], 4);
      ui32Offset = psXMagic->sIndirection.ui32YOffset + XMagicSwapLong(ui32Value, psXMagic->ui32Type);
      break;
    }
  }

  else if (psXMagic->ui32Level && (psXMagic->ui32Flags & XMAGIC_RELATIVE_OFFSET) == XMAGIC_RELATIVE_OFFSET)
  {
    ui32Offset = psXMagic->ui32XOffset + (psXMagic->psParent)->ui32XOffset;
  }

  else
  {
    ui32Offset = psXMagic->ui32XOffset;
  }

  if (ui32Offset > iNRead - sizeof(K_UINT32))
  {

    /*-
     *******************************************************************
     *
     * The offset is out of range.
     *
     *******************************************************************
     */
    return -1;
  }

  return ui32Offset;
}


/*-
 ***********************************************************************
 *
 * XMagicLoadMagic
 *
 ***********************************************************************
 */
int
XMagicLoadMagic(char *pcFilename, char *pcError)
{
  const char          cRoutine[] = "XMagicLoadMagic()";
  char                cLine[XMAGIC_MAX_LINE_LENGTH];
  char                cLocalError[ERRBUF_SIZE];
  FILE               *pFile;
  int                 iLineNumber;
  int                 iParentExists;
  XMAGIC             *pHead;
  XMAGIC             *pLast;
  XMAGIC             *psXMagic;

  cLocalError[0] = 0;

  pHead = pLast = psXMagic = NULL;

  SystemByteOrder = (*(K_UINT32 *) ByteOrderMagic == 0x01020304) ? XMAGIC_MSB : XMAGIC_LSB;

  if ((pFile = fopen(pcFilename, "r")) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilename, strerror(errno));
    return ER_fopen;
  }

  for (cLine[0] = 0, iLineNumber = 1; fgets(cLine, XMAGIC_MAX_LINE_LENGTH, pFile) != NULL; cLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Check the file's magic.
     *
     *******************************************************************
     */
    if (iLineNumber == 1 && strncmp(cLine, "# XMagic", 8) != 0)
    {
      fclose(pFile);
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: magic != [# XMagic]", cRoutine, pcFilename, iLineNumber);
      return ER_XMagic;
    }

    /*-
     *******************************************************************
     *
     * Ignore full line comments (i.e. '#' in position 0).
     *
     *******************************************************************
     */
    if (cLine[0] == '#')
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
    if (SupportChopEOLs(cLine, feof(pFile) ? 0 : 1, cLocalError) == ER)
    {
      fclose(pFile);
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, cLocalError);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * If there's anything left over, process it.
     *
     *******************************************************************
     */
    if (strlen(cLine) > 0)
    {

      /*-
       *****************************************************************
       *
       * Allocate memory for a XMAGIC structure.
       *
       *****************************************************************
       */
      psXMagic = (XMAGIC *) malloc(sizeof(XMAGIC)); /* The caller must free this storage. */
      if (psXMagic == NULL)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
        return ER_BadHandle;
      }

      /*-
       *****************************************************************
       *
       * Initialize the XMAGIC structure.
       *
       *****************************************************************
       */
      memset(psXMagic, 0, sizeof(XMAGIC));
      psXMagic->ui32Mask = ~0;
      psXMagic->cOperator = '=';
      psXMagic->psParent = NULL;
      psXMagic->psSibling = NULL;
      psXMagic->psChild = NULL;

      if (XMagicParseLine(cLine, psXMagic, cLocalError) != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, cLocalError);
        free(psXMagic);
        return ER_XMagic;
      }

      if (pHead == NULL && pLast == NULL)
      {
        psXMagic->psParent = NULL;
        pHead = psXMagic;
      }
      else
      {

        /*-
         ***************************************************************
         *
         * If psXMagic->ui32Level == pLast->ui32Level, then we have a
         * sibling.
         *
         ***************************************************************
         */
        if (psXMagic->ui32Level == pLast->ui32Level)
        {
          psXMagic->psParent = pLast->psParent;
          pLast->psSibling = psXMagic;
        }

        /*-
         ***************************************************************
         *
         * If psXMagic->ui32Level > pLast->ui32Level and the delta == 1,
         * then we have a child.
         *
         ***************************************************************
         */
        else if (psXMagic->ui32Level > pLast->ui32Level)
        {
          if ((psXMagic->ui32Level - pLast->ui32Level) > 1)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: test level(s) skipped", cRoutine, pcFilename, iLineNumber);
            free(psXMagic);
            return ER_XMagic;
          }
          psXMagic->psParent = pLast;
          pLast->psChild = psXMagic;
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
          while ((pLast = pLast->psParent) != NULL)
          {
            if (psXMagic->ui32Level == pLast->ui32Level)
            {
              psXMagic->psParent = pLast->psParent;
              pLast->psSibling = psXMagic;
              iParentExists = 1;
              break;
            }
          }
          if (!iParentExists)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: missing parent magic", cRoutine, pcFilename, iLineNumber);
            free(psXMagic);
            return ER_XMagic;
          }
        }
      }
      pLast = psXMagic;
    }
  }
  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
    return ER_fgets;
  }
  fclose(pFile);

  XMagicTree = pHead;

  return ER_OK;
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
  const char          cRoutine[] = "XMagicParseLine()";
  char                cLocalError[ERRBUF_SIZE];
  char               *pcE;
  char               *pcS;
  int                 iError;
  int                 iEndFound;

  cLocalError[0] = 0;

  iEndFound = 0;

  pcE = pcS = pcLine;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: offset: unexpected NULL found", cRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetOffset(pcS, pcE, psXMagic, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: type: unexpected NULL found", cRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetType(pcS, pcE, psXMagic, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  pcE++;

  SKIP_WHITESPACE(pcE);

  pcS = pcE;

  FIND_DELIMETER(pcE);

  if (*pcE == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: operator: unexpected NULL found", cRoutine);
    return ER_BadValue;
  }

  *pcE = 0;

  iError = XMagicGetTestOperator(pcS, pcE, psXMagic, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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

  iError = XMagicGetTestValue(pcS, pcE, psXMagic, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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

  iError = XMagicGetDescription(pcS, pcE, psXMagic, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  if (SystemByteOrder == XMAGIC_MSB)
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
  if (SystemByteOrder == XMAGIC_MSB)
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
 * XMagicTestBuffer
 *
 ***********************************************************************
 */
int
XMagicTestBuffer(unsigned char *pucBuffer, int iBufferLength, char *pcDescription, int iDescriptionLength, char *pcError)
{
  char                cLocalError[ERRBUF_SIZE];
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
    snprintf(pcDescription, iDescriptionLength, XMAGIC_ISEMPTY);
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
  iMatch = XMagicTestMagic(pucBuffer, iBufferLength, XMagicTree, pcDescription, &iBytesUsed, &iBytesLeft, cLocalError);
  if (iMatch)
  {
    pcDescription[iBytesUsed] = 0;
  }
  else if (iMatch == ER)
  {
    return ER;
  }
  else
  {
    snprintf(pcDescription, iDescriptionLength, XMAGIC_DEFAULT);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * XMagicTestFile
 *
 ***********************************************************************
 */
int
XMagicTestFile(char *pcFilename, char *pcDescription, int iDescriptionLength, char *pcError)
{
  const char          cRoutine[] = "XMagicTestFile()";
  char                cLocalError[ERRBUF_SIZE];
  FILE               *pFile;
  int                 iError;
  int                 iNRead;
  unsigned char       ucBuffer[XMAGIC_READ_BUFSIZE];

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Open the specified file.
   *
   *********************************************************************
   */
  if ((pFile = fopen(pcFilename, "rb")) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
  iNRead = fread(ucBuffer, 1, XMAGIC_READ_BUFSIZE, pFile);

  if (ferror(pFile))
  {
    fclose(pFile);
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
  iError = XMagicTestBuffer(ucBuffer, iNRead, pcDescription, iDescriptionLength, cLocalError);
  if (iError == ER)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
XMagicTestMagic(unsigned char *pucBuffer, int iNRead, XMAGIC *psXMagic, char *pcDescription, int *iBytesUsed, int *iBytesLeft, char *pcError)
{
  char                cDescriptionLocal[XMAGIC_DESCRIPTION_BUFSIZE];
  char                cLocalError[ERRBUF_SIZE];
  int                 iMatch;
  int                 iMatches;
  K_UINT32            ui32Offset;
  XMAGIC             *psMyXMagic;

  cLocalError[0] = 0;

  for (iMatches = 0, psMyXMagic = psXMagic; psMyXMagic != NULL; psMyXMagic = psMyXMagic->psSibling)
  {
    ui32Offset = XMagicGetValueOffset(pucBuffer, iNRead, psMyXMagic, cLocalError);
    if (ui32Offset == -1)
    {
      continue;
    }

    iMatch = XMagicCompareValues(pucBuffer, iNRead, psMyXMagic, ui32Offset, cDescriptionLocal, cLocalError);

    if (iMatch)
    {
      *iBytesUsed += snprintf(&pcDescription[*iBytesUsed], *iBytesLeft, "%s", cDescriptionLocal);
      *iBytesLeft -= *iBytesUsed;

      if (psMyXMagic->psChild != NULL)
      {
        XMagicTestMagic(pucBuffer, iNRead, psMyXMagic->psChild, pcDescription, iBytesUsed, iBytesLeft, cLocalError);
      }

      if (psXMagic->ui32Level == 0)
      {
        return 1;
      }
      else
      {
        iMatches++;
      }
    }
  }

  return (iMatches) ? 1 : 0;
}


#ifdef UNIX
/*-
 ***********************************************************************
 *
 * XMagicTestSpecial
 *
 ***********************************************************************
 */
int
XMagicTestSpecial(char *pcFilename, struct stat *pStat, char *pcDescription, int iDescriptionLength, char *pcError)
{
  const char          cRoutine[] = "XMagicTestSpecial()";
  char               *pcLinkBuffer;
  int                 n;

  if ((pcLinkBuffer = (char *) malloc(iDescriptionLength)) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return ER_BadHandle;
  }

  switch (pStat->st_mode & S_IFMT)
  {
  case S_IFIFO:
    snprintf(pcDescription, iDescriptionLength, "named pipe (fifo)");
    break;
  case S_IFCHR:
    snprintf(pcDescription, iDescriptionLength, "character special (%lu/%lu)", (unsigned long) major(pStat->st_rdev), (unsigned long) minor(pStat->st_rdev));
    break;
  case S_IFDIR:
    snprintf(pcDescription, iDescriptionLength, "directory");
    break;
  case S_IFBLK:
    snprintf(pcDescription, iDescriptionLength, "block special (%lu/%lu)", (unsigned long) major(pStat->st_rdev), (unsigned long) minor(pStat->st_rdev));
    break;
  case S_IFREG:
    break;
  case S_IFLNK:
    n = readlink(pcFilename, pcLinkBuffer, iDescriptionLength - 1);
    if (n == ER)
    {
      pcDescription[0] = 0;
      snprintf(pcError, ERRBUF_SIZE, "%s: unreadable symbolic link: %s", cRoutine, strerror(errno));
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
