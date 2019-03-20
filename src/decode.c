/*-
 ***********************************************************************
 *
 * $Id: decode.c,v 1.12 2004/04/23 02:51:12 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2004 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

static unsigned char  gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int            giFromBase64[256];

static DECODE_TABLE   gasDecodeTable[] = {
  { "zname",       "name",          DecodeName                 },
  { "dev",         "dev",           Decode32bFieldBase16To10   },
  { "inode",       "inode",         Decode32bFieldBase16To10   },
  { "volume",      "volume",        Decode32bFieldBase16To10   },
  { "findex",      "findex",        Decode64bFieldBase16To10   },
  { "mode",        "mode",          Decode32bFieldBase16To08   },
  { "attributes",  "attributes",    Decode32bFieldBase16To10   },
  { "nlink",       "nlink",         Decode32bFieldBase16To10   },
  { "uid",         "uid",           Decode32bFieldBase16To10   },
  { "gid",         "gid",           Decode32bFieldBase16To10   },
  { "rdev",        "rdev",          Decode32bFieldBase16To10   },
  { "atime",       "atime",         DecodeTime                 },
  { "ams",         "ams",           DecodeMilliseconds         },
  { "mtime",       "mtime",         DecodeTime                 },
  { "mms",         "mms",           DecodeMilliseconds         },
  { "ctime",       "ctime",         DecodeTime                 },
  { "cms",         "cms",           DecodeMilliseconds         },
  { "chtime",      "chtime",        DecodeTime                 },
  { "chms",        "chms",          DecodeMilliseconds         },
  { "size",        "size",          Decode64bFieldBase16To10   },
  { "altstreams", "altstreams",     Decode32bFieldBase16To10   },
  { "magic",       "magic",         DecodeMagic                },
  { "md5",         "md5",           DecodeMd5                  }
};
#define DECODE_TABLE_SIZE sizeof(gasDecodeTable) / sizeof(gasDecodeTable[0])

static DECODE_TABLE   gasDecodeMap[DECODE_TABLE_SIZE];

static K_UINT32       gui32RecordsDecoded;
static K_UINT32       gui32RecordsLost;


/*-
 ***********************************************************************
 *
 * Decode32bFieldBase16To08
 *
 ***********************************************************************
 */
int
Decode32bFieldBase16To08(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "Decode32bFieldBase16To08()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  int                 n = 0;
  static int          iFirst = 1;
  static K_UINT32     ui32ValueNew[DECODE_TABLE_SIZE];
  static K_UINT32     ui32ValueOld[DECODE_TABLE_SIZE];
  static K_UINT32    *pui32ValueOld[DECODE_TABLE_SIZE];

  if (iFirst || iAction == DECODE_RESET)
  {
    for (i = 0; i < DECODE_TABLE_SIZE; i++)
    {
      ui32ValueNew[i] = 0;
      ui32ValueOld[i] = 0;
      pui32ValueOld[i] = &ui32ValueOld[i];
    }
    iFirst = 0;
    if (iAction == DECODE_RESET)
    {
      return ER_OK;
    }
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {

    iReturnValue = Decode32bValueBase16To10(pcToken, iLength, &ui32ValueNew[iIndex], pui32ValueOld[iIndex], acLocalError);
    if (iReturnValue == ER)
    {
      sprintf(pcError, "%s: Field = [%s]: %s", pcName, acRoutine, acLocalError);
      return iReturnValue;
    }
    n += sprintf(&pcOutput[n], "%o", (unsigned) ui32ValueNew[iIndex]);
    ui32ValueOld[iIndex] = ui32ValueNew[iIndex];

    /*
     * If we got this far, then it's ok to reset any NULL pointers.
     */
    pui32ValueOld[iIndex] = &ui32ValueOld[iIndex];
  }
  else
  {
    pui32ValueOld[iIndex] = NULL;
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * Decode32bFieldBase16To10
 *
 ***********************************************************************
 */
int
Decode32bFieldBase16To10(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "Decode32bFieldBase16To10()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  int                 n = 0;
  static int          iFirst = 1;
  static K_UINT32     ui32ValueNew[DECODE_TABLE_SIZE];
  static K_UINT32     ui32ValueOld[DECODE_TABLE_SIZE];
  static K_UINT32    *pui32ValueOld[DECODE_TABLE_SIZE];

  if (iFirst || iAction == DECODE_RESET)
  {
    for (i = 0; i < DECODE_TABLE_SIZE; i++)
    {
      ui32ValueNew[i] = 0;
      ui32ValueOld[i] = 0;
      pui32ValueOld[i] = &ui32ValueOld[i];
    }
    iFirst = 0;
    if (iAction == DECODE_RESET)
    {
      return ER_OK;
    }
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {

    iReturnValue = Decode32bValueBase16To10(pcToken, iLength, &ui32ValueNew[iIndex], pui32ValueOld[iIndex], acLocalError);
    if (iReturnValue == ER)
    {
      sprintf(pcError, "%s: Field = [%s]: %s", pcName, acRoutine, acLocalError);
      return iReturnValue;
    }
    n += sprintf(&pcOutput[n], "%u", (unsigned) ui32ValueNew[iIndex]);
    ui32ValueOld[iIndex] = ui32ValueNew[iIndex];

    /*
     * If we got this far, then it's ok to reset any NULL pointers.
     */
    pui32ValueOld[iIndex] = &ui32ValueOld[iIndex];
  }
  else
  {
    pui32ValueOld[iIndex] = NULL;
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * Decode32bValueBase16To10
 *
 ***********************************************************************
 */
int
Decode32bValueBase16To10(char *pcData, int iLength, K_UINT32 *pui32ValueNew, K_UINT32 *pui32ValueOld, char *pcError)
{
  const char          acRoutine[] = "Decode32bValueBase16To10()";
  int                 i;
  int                 iSign;
  K_UINT32            ui32;

  /*-
   *********************************************************************
   *
   * Read hex number from pcData and store result in *pui32ValueNew.
   * If hex number preceeded by +, then read hex number, add to
   * pui32ValueOld and store result in *pui32ValueNew. If hex number
   * preceeded by -, then read hex number, subtract from pui32ValueOld
   * and store result in *pui32ValueNew. Hex number must be null
   * terminated. Return the number of characters read.  On error,
   * return ER.
   *
   *********************************************************************
   */

  i = iSign = ui32 = 0;

  if (pui32ValueOld == NULL && (pcData[0] == '#' || pcData[0] == '+' || pcData[0] == '-'))
  {
    sprintf(pcError, "%s: Expected previous value to be defined!", acRoutine);
    return ER;
  }

  if (pcData[0] == '#')
  {
    ui32 = *pui32ValueOld;
    i++;
  }
  else
  {
    if (pcData[0] == '+')
    {
      iSign = 1;
      i++;
    }
    else if (pcData[0] == '-')
    {
      iSign = (-1);
      i++;
    }

    if (iLength <= 8)
    {
      while (i < iLength)
      {
        pcData[i] = tolower(pcData[i]);
        if ((pcData[i] >= '0') && (pcData[i] <= '9'))
        {
          ui32 = (ui32 << 4) | (pcData[i] - '0');
        }
        else if ((pcData[i] >= 'a') && (pcData[i] <= 'f'))
        {
          ui32 = (ui32 << 4) | ((pcData[i] - 'a') + 10);
        }
        else
        {
          sprintf(pcError, "%s: Bad hex digit.", acRoutine);
          return ER;
        }
        i++;
      }
    }
    else
    {
      sprintf(pcError, "%s: Field length exceeds 8 digits!", acRoutine);
      return ER;
    }
    if (iSign != 0)
    {
      ui32 = (*pui32ValueOld) + iSign * ui32;
    }
  }

  *pui32ValueNew = ui32;
  return i;
}


/*-
 ***********************************************************************
 *
 * Decode64bFieldBase16To10
 *
 ***********************************************************************
 */
int
Decode64bFieldBase16To10(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "Decode64bFieldBase16To10()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  int                 n = 0;
  static int          iFirst = 1;
  static K_UINT64     aui64ValueNew[DECODE_TABLE_SIZE];
  static K_UINT64     aui64ValueOld[DECODE_TABLE_SIZE];
  static K_UINT64    *apui64ValueOld[DECODE_TABLE_SIZE];

  if (iFirst || iAction == DECODE_RESET)
  {
    for (i = 0; i < DECODE_TABLE_SIZE; i++)
    {
      aui64ValueNew[i] = 0;
      aui64ValueOld[i] = 0;
      apui64ValueOld[i] = &aui64ValueOld[i];
    }
    iFirst = 0;
    if (iAction == DECODE_RESET)
    {
      return ER_OK;
    }
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {
    iReturnValue = Decode64bValueBase16To10(pcToken, iLength, &aui64ValueNew[iIndex], apui64ValueOld[iIndex], acLocalError);
    if (iReturnValue == ER)
    {
      sprintf(pcError, "%s: Field = [%s]: %s", pcName, acRoutine, acLocalError);
      return iReturnValue;
    }

#ifdef WIN32
    n += snprintf(&pcOutput[n], FTIMES_MAX_64BIT_SIZE, "%I64u", aui64ValueNew[iIndex]);
#endif
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
    n += snprintf(&pcOutput[n], FTIMES_MAX_64BIT_SIZE, "%qu", (unsigned long long) aui64ValueNew[iIndex]);
#else
    n += snprintf(&pcOutput[n], FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long) aui64ValueNew[iIndex]);
#endif
#endif
    aui64ValueOld[iIndex] = aui64ValueNew[iIndex];

    /*
     * If we got this far, then it's ok to reset any NULL pointers.
     */
    apui64ValueOld[iIndex] = &aui64ValueOld[iIndex];
  }
  else
  {
    apui64ValueOld[iIndex] = NULL;
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * Decode64bValueBase16To10
 *
 ***********************************************************************
 */
int
Decode64bValueBase16To10(char *pcData, int iLength, K_UINT64 *pui64ValueNew, K_UINT64 *pui64ValueOld, char *pcError)
{
  const char          acRoutine[] = "Decode64bValueBase16To10()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  K_UINT32            ui32UpperNew;
  K_UINT32            ui32LowerNew;
  K_UINT32            ui32LowerOld;
  K_UINT64            ui64;

  i = acLocalError[0] = 0;

  ui64 = (K_UINT64) 0;

  if (pui64ValueOld == NULL && pcData[0] == '#')
  {
    sprintf(pcError, "%s: Expected previous value to be defined!", acRoutine);
    return ER;
  }

  if (pcData[0] == '#')
  {
    ui32UpperNew = (K_UINT32) (*pui64ValueOld >> 32);
    ui32LowerOld = (K_UINT32) (*pui64ValueOld & 0xffffff);
    i++;
    iLength--;
    iReturnValue = Decode32bValueBase16To10(&pcData[i], iLength, &ui32LowerNew, &ui32LowerOld, acLocalError);
    if (iReturnValue == ER)
    {
      sprintf(pcError, "%s: %s", acRoutine, acLocalError);
      return iReturnValue;
    }

    *pui64ValueNew = (((K_UINT64) ui32UpperNew) << 32) | ui32LowerNew;
    return (1 + iReturnValue);
  }

  if (iLength <= 16)
  {
    while (i < iLength)
    {
      pcData[i] = tolower(pcData[i]);
      if ((pcData[i] >= '0') && (pcData[i] <= '9'))
      {
        ui64 = (ui64 << 4) | (pcData[i] - '0');
      }
      else if ((pcData[i] >= 'a') && (pcData[i] <= 'f'))
      {
        ui64 = (ui64 << 4) | ((pcData[i] - 'a') + 10);
      }
      else
      {
        sprintf(pcError, "%s: Bad hex digit.", acRoutine);
        return ER;
      }
      i++;
    }
  }
  else
  {
    sprintf(pcError, "%s: Field length exceeds 16 digits!", acRoutine);
    return ER;
  }

  *pui64ValueNew = ui64;
  return i;
}


/*-
 ***********************************************************************
 *
 * DecodeBuildFromBase64Table
 *
 ***********************************************************************
 */
void
DecodeBuildFromBase64Table(void)
{
  int                 i;

  for (i = 0; i < 256; i++)
  {
    giFromBase64[i] = (-1);
  }

  for (i = 0; gaucBase64[i] != '\0'; i++)
  {
    giFromBase64[gaucBase64[i]] = i;
  }
}


/*-
 ***********************************************************************
 *
 * DecodeFile
 *
 ***********************************************************************
 */
int
DecodeFile(char *pcFilename, FILE *pFileOut, char *pcNewLine, char *pcError)
{
  const char          acRoutine[] = "DecodeFile()";
  char                acHeader[FTIMES_MAX_LINE];
  char                acLine[FTIMES_MAX_LINE];
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acOutput[FTIMES_MAX_LINE];
  char               *pc;
  FILE               *pFile;
  int                 i;
  int                 iDecodeIndex;
  int                 iLineNumber;
  int                 iReturnValue;
  int                 iSkipToNextCheckpoint;
  int                 n;

  DecodeBuildFromBase64Table();

  /*-
   *********************************************************************
   *
   * Open the snapshot file.
   *
   *********************************************************************
   */
  if (strcmp(pcFilename, "-") == 0)
  {
    pFile = stdin;
  }
  else
  {
    pFile = fopen(pcFilename, "rb");
    if (pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
      return ER_fopen;
    }
  }

  /*-
   *********************************************************************
   *
   * Check the 1st line to see if it looks like a compressed snapshot.
   *
   *********************************************************************
   */
  iLineNumber = 1;
  fgets(acLine, FTIMES_MAX_LINE, pFile);

  /*-
   *********************************************************************
   *
   * Remove EOL characters.
   *
   *********************************************************************
   */
  if (SupportChopEOLs(acLine, feof(pFile) ? 0 : 1, acLocalError) == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
    fclose(pFile);
    return ER;
  }

  if (strncmp(acLine, "zname|", 6) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: Unrecognized header.", acRoutine, pcFilename, iLineNumber);
    fclose(pFile);
    return ER_BadValue;
  }

  /*-
   *********************************************************************
   *
   * Develop the decode map.
   *
   *********************************************************************
   */
  for (iDecodeIndex = 0, pc = strtok(acLine, DECODE_SEPARATOR_S); pc != NULL; pc = strtok(NULL, DECODE_SEPARATOR_S), iDecodeIndex++)
  {
    for (i = 0; i < DECODE_TABLE_SIZE; i++)
    {
      if (strcmp(gasDecodeTable[i].acZName, pc) == 0)
      {
        gasDecodeMap[iDecodeIndex] = gasDecodeTable[i]; /* This initializes all elements at the given index. */
        break;
      }
    }
    if (i == DECODE_TABLE_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d], Field = [%s]: Unrecognized field.", acRoutine, pcFilename, iLineNumber, pc);
      fclose(pFile);
      return ER_BadValue;
    }
  }

  /*-
   *********************************************************************
   *
   * Generate the output header.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iDecodeIndex; i++)
  {
    n += sprintf(&acHeader[n], "%s%s", gasDecodeMap[i].acUName, (i < iDecodeIndex - 1) ? DECODE_SEPARATOR_S : "");
  }
  fprintf(pFileOut, "%s%s", acHeader, pcNewLine);

  /*-
   *********************************************************************
   *
   * Process the data.
   *
   *********************************************************************
   */
  iSkipToNextCheckpoint = FALSE;
  for (acLine[0] = acOutput[0] = 0, iLineNumber = 2; fgets(acLine, FTIMES_MAX_LINE, pFile) != NULL; acLine[0] = acOutput[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Remove EOL characters.
     *
     *******************************************************************
     */
    if (SupportChopEOLs(acLine, feof(pFile) ? 0 : 1, acLocalError) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      fclose(pFile);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * This logic is used to recover from a damaged input stream.
     *
     *******************************************************************
     */
    if (iSkipToNextCheckpoint)
    {
      if (strncmp(acLine, "00", 2) == 0) /* Checkpoint */
      {
        iSkipToNextCheckpoint = FALSE;
        DecodeName("reset", 0, DECODE_RESET, NULL, 0, NULL, acLocalError);
        Decode32bFieldBase16To10("reset", 0, DECODE_RESET, NULL, 0, NULL, acLocalError);
        Decode64bFieldBase16To10("reset", 0, DECODE_RESET, NULL, 0, NULL, acLocalError);
        DecodeTime("reset", 0, DECODE_RESET, NULL, 0, NULL, acLocalError);
        DecodeMilliseconds("reset", 0, DECODE_RESET, NULL, 0, NULL, acLocalError);
        snprintf(pcError, MESSAGE_SIZE, "%s: Line = [%d]: Checkpoint located. Restarting decoder routines.", acRoutine, iLineNumber);
        ErrorHandler(ER_BadHandle, pcError, ERROR_WARNING);
      }
      else
      {
        gui32RecordsLost++;
        continue;
      }
    }

    /*-
     *******************************************************************
     *
     * Decode the data. If it is damaged, skip to the next checkpoint.
     *
     *******************************************************************
     */
    iReturnValue = DecodeLine(acLine, acOutput, acLocalError);
    if (iReturnValue == ER)
    {
      acOutput[0] = 0;
      iSkipToNextCheckpoint = TRUE;
      snprintf(pcError, MESSAGE_SIZE, "%s: Line = [%d]: %s", acRoutine, iLineNumber, acLocalError);
      ErrorHandler(ER_BadHandle, pcError, ERROR_FAILURE);
      gui32RecordsLost++;
    }
    else
    {
      gui32RecordsDecoded++;
    }

    /*-
     *******************************************************************
     *
     * Write out the decoded record.
     *
     *******************************************************************
     */
    if (acOutput[0] != 0)
    {
      fprintf(pFileOut, "%s%s", acOutput, pcNewLine);
    }
  }
  fclose(pFile);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeFormatOutOfBandTime
 *
 ***********************************************************************
 */
int
DecodeFormatOutOfBandTime(char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeFormatOutOfBandTime()";
  int                 i;
  int                 n;

  /*-
   *********************************************************************
   *
   * Input: YYYYMMDDHHMMSS  --> Output: YYYY-MM-DD HH:MM:SS
   *
   *********************************************************************
   */

  if (iLength != (int) strlen("YYYYMMDDHHMMSS"))
  {
    sprintf(pcError, "%s: Field length != %d!", acRoutine, (int) strlen("YYYYMMDDHHMMSS"));
    return ER;
  }

  for (i = n = 0; i < iLength; i++)
  {
    if (i == 4 || i == 6)
    {
      pcOutput[n++] = '-';
    }
    else if (i == 8)
    {
      pcOutput[n++] = ' ';
    }
    else if (i == 10 || i == 12)
    {
      pcOutput[n++] = ':';
    }
    pcOutput[n++] = pcToken[i];
  }
  return n;
}


#ifdef WIN32
/*-
 ***********************************************************************
 *
 * DecodeFormatTime
 *
 ***********************************************************************
 */
#define DECODE_TIME_FORMAT_SIZE 20
#define DECODE_TIME_FORMAT "%04d-%02d-%02d %02d:%02d:%02d"
int
DecodeFormatTime(FILETIME *pFileTime, char *pcTime)
{
  int                 iCount;
  SYSTEMTIME          systemTime;

  pcTime[0] = 0;

  if (!FileTimeToSystemTime(pFileTime, &systemTime))
  {
    return ER;
  }

  iCount = snprintf(
                     pcTime,
                     DECODE_TIME_FORMAT_SIZE,
                     DECODE_TIME_FORMAT,
                     systemTime.wYear,
                     systemTime.wMonth,
                     systemTime.wDay,
                     systemTime.wHour,
                     systemTime.wMinute,
                     systemTime.wSecond
                   );

  if (iCount != DECODE_TIME_FORMAT_SIZE - 1)
  {
    return ER;
  }

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * DecodeGetBase64Hash
 *
 ***********************************************************************
 */
int
DecodeGetBase64Hash(char *pcData, unsigned char *pucHash, int iLength, char *pcError)
{
  const char          acRoutine[] = "DecodeGetBase64Hash()";
  int                 i;
  int                 j;
  int                 iLeft;
  K_UINT32            ui32;

  if (iLength != 22)
  {
    sprintf(pcError, "%s: Hash must be 22 bytes long", acRoutine);
    return ER;
  }

  for (i = 0, j = 0, ui32 = 0, iLeft = 0; i < 22; i++)
  {
    if (giFromBase64[(int) pcData[i]] < 0)
    {
      sprintf(pcError, "%s: Illegal base64 character", acRoutine);
      return ER;
    }
    ui32 = (ui32 << 6) | giFromBase64[(int) pcData[i]];
    iLeft += 6;
    while (iLeft >= 8)
    {
      pucHash[j++] = (unsigned char) ((ui32 >> (iLeft - 8)) & 0xff);
      iLeft -= 8;
    }
  }
  return i;
}


/*-
 ***********************************************************************
 *
 * DecodeGetRecordsDecoded
 *
 ***********************************************************************
 */
K_UINT32
DecodeGetRecordsDecoded(void)
{
  return gui32RecordsDecoded;
}


/*-
 ***********************************************************************
 *
 * DecodeGetRecordsLost
 *
 ***********************************************************************
 */
K_UINT32
DecodeGetRecordsLost(void)
{
  return gui32RecordsLost;
}


/*-
 ***********************************************************************
 *
 * DecodeLine
 *
 ***********************************************************************
 */
int
DecodeLine(char *pcLine, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeLine()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char               *pcName;
  char               *pcToken;
  int                 iDone = 0;
  int                 iIndex;
  int                 iLength = 0;
  int                 iOffset = 0;
  int                 iReturnValue;
  int                 iTokenCount = 0;
  int                 n = 0;

  pcToken = pcLine;

  while (!iDone)
  {
    if (pcLine[iOffset] == DECODE_SEPARATOR_C || pcLine[iOffset] == 0)
    {
      if (pcLine[iOffset] == 0)
      {
        iDone = 1;
      }
      pcLine[iOffset] = 0;
      pcName = gasDecodeMap[iTokenCount].acZName;
      iIndex = iTokenCount;
      iReturnValue = gasDecodeMap[iTokenCount].piRoutine(pcName, iIndex, DECODE_DECODE, pcToken, iLength, &pcOutput[n], acLocalError);
      if (iReturnValue == ER)
      {
        sprintf(pcError, "%s: %s", acRoutine, acLocalError);
        return ER;
      }
      n += iReturnValue;
      pcOutput[n] = 0; /* Terminate the string. */
      if (!iDone)
      {
        iOffset++;
        iTokenCount++;
        pcToken = &pcLine[iOffset];
        iLength = 0;
      }
    }
    else
    {
      iOffset++;
      iLength++;
    }
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * DecodeMagic
 *
 ***********************************************************************
 */
int
DecodeMagic(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  if (iAction == DECODE_RESET)
  {
    return ER_OK;
  }

  pcOutput[0] = DECODE_SEPARATOR_C;

  return 1;
}


/*-
 ***********************************************************************
 *
 * DecodeMd5
 *
 ***********************************************************************
 */
int
DecodeMd5(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeMd5()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  int                 n = 0;
  unsigned char       ucFileMd5[16];

  if (iAction == DECODE_RESET)
  {
    return ER_OK;
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {
    if (pcToken[0] == 'D' && pcToken[1] == 0)
    {
      n += sprintf(&pcOutput[n], "DIRECTORY");
    }
    else if (pcToken[0] == 'L' && pcToken[1] == 0)
    {
      n += sprintf(&pcOutput[n], "SYMLINK");
    }
    else if (pcToken[0] == 'S' && pcToken[1] == 0)
    {
      n += sprintf(&pcOutput[n], "SPECIAL");
    }
    else
    {
      iReturnValue = DecodeGetBase64Hash(pcToken, ucFileMd5, iLength, acLocalError);
      if (iReturnValue == ER)
      {
        sprintf(pcError, "%s: %s", acRoutine, acLocalError);
        return iReturnValue;
      }
      for (i = 0; i < 16; i++)
      {
        n += sprintf(&pcOutput[n], "%02x", ucFileMd5[i]);
      }
    }
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * DecodeMilliseconds
 *
 ***********************************************************************
 */
int
DecodeMilliseconds(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeMilliseconds()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iReturnValue;
  int                 iTimeIndex;
  int                 n = 0;
  static int          iFirst = 1;
  static K_UINT32     ui32MilliNew[4];
  static K_UINT32     ui32MilliOld[4];
  static K_UINT32    *pui32MilliOld[4];

#define  A_MILLI_INDEX 0
#define  M_MILLI_INDEX 1
#define  C_MILLI_INDEX 2
#define CH_MILLI_INDEX 3

  if (iFirst || iAction == DECODE_RESET)
  {
    for (i = 0; i < 4; i++)
    {
      ui32MilliNew[i] = 0;
      ui32MilliOld[i] = 0;
      pui32MilliOld[i] = &ui32MilliOld[i];
    }
    iFirst = 0;
    if (iAction == DECODE_RESET)
    {
      return ER_OK;
    }
  }

  if (strcmp(pcName, "ams") == 0)
  {
    iTimeIndex = A_MILLI_INDEX;
  }
  else if (strcmp(pcName, "mms") == 0)
  {
    iTimeIndex = M_MILLI_INDEX;
  }
  else if (strcmp(pcName, "cms") == 0)
  {
    iTimeIndex = C_MILLI_INDEX;
  }
  else if (strcmp(pcName, "chms") == 0)
  {
    iTimeIndex = CH_MILLI_INDEX;
  }
  else
  {
    sprintf(pcError, "DecodeMilliseconds(%s): unknown field", pcName);
    return ER;
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {
    if (pcToken[0] == '#')
    {
      if (pui32MilliOld[iTimeIndex] != NULL)
      {
        ui32MilliNew[iTimeIndex] = *pui32MilliOld[iTimeIndex];
      }
      else
      {
        sprintf(pcError, "%s: Field = [%s]: Expected previous value to be defined!", acRoutine, pcName);
        return ER;
      }
    }
    else if (pcToken[0] == 'X')
    {
      if (pui32MilliOld[A_MILLI_INDEX] != NULL)
      {
        ui32MilliNew[iTimeIndex] = *pui32MilliOld[A_MILLI_INDEX];
      }
      else
      {
        sprintf(pcError, "%s: Field = [%s]: Expected atime value to be defined!", acRoutine, pcName);
        return ER;
      }
    }
    else if (pcToken[0] == 'Y')
    {
      if (pui32MilliOld[M_MILLI_INDEX] != NULL)
      {
        ui32MilliNew[iTimeIndex] = *pui32MilliOld[M_MILLI_INDEX];
      }
      else
      {
        sprintf(pcError, "%s: Field = [%s]: Expected mtime value to be defined!", acRoutine, pcName);
        return ER;
      }
    }
    else if (pcToken[0] == 'Z')
    {
      if (pui32MilliOld[C_MILLI_INDEX] != NULL)
      {
        ui32MilliNew[iTimeIndex] = *pui32MilliOld[C_MILLI_INDEX];
      }
      else
      {
        sprintf(pcError, "%s: Field = [%s]: Expected ctime value to be defined!", acRoutine, pcName);
        return ER;
      }
    }
    else
    {
      iReturnValue = Decode32bValueBase16To10(pcToken, iLength, &ui32MilliNew[iTimeIndex], pui32MilliOld[iTimeIndex], acLocalError);
      if (iReturnValue == ER)
      {
        sprintf(pcError, "DecodeMilliseconds(%s): %s", pcName, acLocalError);
        return iReturnValue;
      }
    }
    n += sprintf(&pcOutput[n], "%u", (unsigned) ui32MilliNew[iTimeIndex]);
    ui32MilliOld[iTimeIndex] = ui32MilliNew[iTimeIndex];

    /*
     * If we got this far, then it's ok to reset any NULL pointers.
     */
    pui32MilliOld[iTimeIndex] = &ui32MilliOld[iTimeIndex];
  }
  else
  {
    pui32MilliOld[iTimeIndex] = NULL;
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * DecodeName
 *
 ***********************************************************************
 */
int
DecodeName(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeName()";
  int                 i;
  int                 j;
  int                 n = 0;
  unsigned int        iFromLastLine;
  static char         acLastName[1024] = "";

  if (iAction == DECODE_RESET)
  {
    memset(acLastName, 0, 1024);
    return ER_OK;
  }

  if (iLength >= 2)
  {

    /*
     * Get 2 byte prefix (i.e., the number of repeated bytes in name)
     */
    for (i = iFromLastLine = 0; i < 2; i++)
    {
      iFromLastLine <<= 4;
      if ((pcToken[i] >= '0') && (pcToken[i] <= '9'))
      {
        iFromLastLine |= pcToken[i] - '0';
      }
      else if ((pcToken[i] >= 'a') && (pcToken[i] <= 'f'))
      {
        iFromLastLine |= (pcToken[i] - 'a') + 10;
      }
      else
      {
        sprintf(pcError, "%s: Illegal 2 byte prefix.", acRoutine);
        return ER;
      }
    }
    if (iFromLastLine > strlen(acLastName))
    {
      sprintf(pcError, "%s: Not enough bytes available to perform decode.", acRoutine);
      return ER;
    }

    /*
     * Copy rest of new name to last name.
     */
    if (iFromLastLine == 0)
    {
      acLastName[0] = '"';
      j = 1;
      i = 3;
    }
    else
    {
      j = iFromLastLine;
      i = 2;
    }
    while (pcToken[i] != 0 && pcToken[i] != '"')
    {
      acLastName[j++] = pcToken[i++];
    }
    if (pcToken[i] != '"')
    {
      sprintf(pcError, "%s: Names must end with a double quote.", acRoutine);
      return ER;
    }
    acLastName[j++] = pcToken[i++];
    acLastName[j] = 0;
    n = sprintf(pcOutput, "%s", acLastName);

    /*
     * Compute name md5 hash and insert into output record.
     */
    if (acLastName[0] != '"' || acLastName[j - 1] != '"')
    {
      sprintf(pcError, "%s: Names must be enclosed in double quotes.", acRoutine);
      return ER;
    }

  }
  else
  {
    sprintf(pcError, "%s: Name length must be greater than 2.", acRoutine);
    return ER;
  }
  return n;
}


/*-
 ***********************************************************************
 *
 * DecodeTime
 *
 ***********************************************************************
 */
int
DecodeTime(char *pcName, int iIndex, int iAction, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeTime()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acTimeString[64];
  int                 i;
  int                 iReturnValue;
  int                 iTimeIndex;
  int                 n = 0;
  static int          iFirst = 1;
  static K_UINT32     ui32SecondsNew[4];
  static K_UINT32     ui32SecondsOld[4];
  static K_UINT32    *pui32SecondsOld[4];

#define  A_TIME_INDEX 0
#define  M_TIME_INDEX 1
#define  C_TIME_INDEX 2
#define CH_TIME_INDEX 3

  if (iFirst || iAction == DECODE_RESET)
  {
    for (i = 0; i < 4; i++)
    {
      ui32SecondsNew[i] = 0;
      ui32SecondsOld[i] = 0;
      pui32SecondsOld[i] = &ui32SecondsOld[i];
    }
    iFirst = 0;
    if (iAction == DECODE_RESET)
    {
      return ER_OK;
    }
  }

  if (strcmp(pcName, "atime") == 0)
  {
    iTimeIndex = A_TIME_INDEX;
  }
  else if (strcmp(pcName, "mtime") == 0)
  {
    iTimeIndex = M_TIME_INDEX;
  }
  else if (strcmp(pcName, "ctime") == 0)
  {
    iTimeIndex = C_TIME_INDEX;
  }
  else if (strcmp(pcName, "chtime") == 0)
  {
    iTimeIndex = CH_TIME_INDEX;
  }
  else
  {
    sprintf(pcError, "%s: Field = [%s]: Unknown field.", acRoutine, pcName);
    return ER;
  }

  pcOutput[n++] = DECODE_SEPARATOR_C;

  if (iLength)
  {
    if (pcToken[0] == '~')
    {
      iReturnValue = DecodeFormatOutOfBandTime(&pcToken[1], iLength - 1, &pcOutput[n], acLocalError);
      if (iReturnValue == ER)
      {
        sprintf(pcError, "%s: Field = [%s]: %s", acRoutine, pcName, acLocalError);
        return iReturnValue;
      }
      n += iReturnValue;
      ui32SecondsOld[iTimeIndex] = 0;
    }
    else
    {
      if (pcToken[0] == '#')
      {
        if (pui32SecondsOld[iTimeIndex] != NULL)
        {
          ui32SecondsNew[iTimeIndex] = *pui32SecondsOld[iTimeIndex];
        }
        else
        {
          sprintf(pcError, "%s: Field = [%s]: Expected a previous value to be defined!", acRoutine, pcName);
          return ER;
        }
      }
      else if (pcToken[0] == 'X')
      {
        if (pui32SecondsOld[A_TIME_INDEX] != NULL)
        {
          ui32SecondsNew[iTimeIndex] = *pui32SecondsOld[A_TIME_INDEX];
        }
        else
        {
          sprintf(pcError, "%s: Field = [%s]: Expected atime value to be defined!", acRoutine, pcName);
          return ER;
        }
      }
      else if (pcToken[0] == 'Y')
      {
        if (pui32SecondsOld[M_TIME_INDEX] != NULL)
        {
          ui32SecondsNew[iTimeIndex] = *pui32SecondsOld[M_TIME_INDEX];
        }
        else
        {
          sprintf(pcError, "%s: Field = [%s]: Expected mtime value to be defined!", acRoutine, pcName);
          return ER;
        }
      }
      else if (pcToken[0] == 'Z')
      {
        if (pui32SecondsOld[C_TIME_INDEX] != NULL)
        {
          ui32SecondsNew[iTimeIndex] = *pui32SecondsOld[C_TIME_INDEX];
        }
        else
        {
          sprintf(pcError, "%s: Field = [%s]: Expected ctime value to be defined!", acRoutine, pcName);
          return ER;
        }
      }
      else
      {
        iReturnValue = Decode32bValueBase16To10(pcToken, iLength, &ui32SecondsNew[iTimeIndex], pui32SecondsOld[iTimeIndex], acLocalError);
        if (iReturnValue == ER)
        {
          sprintf(pcError, "%s: Field = [%s]: %s", acRoutine, pcName, acLocalError);
          return iReturnValue;
        }
      }
#ifdef WIN32
      {
        K_UINT64            u_int64time;
        FILETIME            time64;

        u_int64time = (((((K_UINT64) ui32SecondsNew[iTimeIndex]) * 1000)) * 10000) + UNIX_EPOCH_IN_NT_TIME;
        time64.dwLowDateTime = (K_UINT32) (u_int64time & (K_UINT32) 0xffffffff);
        time64.dwHighDateTime = (DWORD) (u_int64time >> 32);
        DecodeFormatTime(&time64, acTimeString);
      }
#endif
#ifdef UNIX
      TimeFormatTime((time_t *) &ui32SecondsNew[iTimeIndex], acTimeString);
#endif
      n += sprintf(&pcOutput[n], "%s", acTimeString);
      ui32SecondsOld[iTimeIndex] = ui32SecondsNew[iTimeIndex];

      /*
       * If we got this far, then it's ok to reset any NULL pointers.
       */
      pui32SecondsOld[iTimeIndex] = &ui32SecondsOld[iTimeIndex];
    }
  }
  else
  {
    pui32SecondsOld[iTimeIndex] = NULL;
  }
  return n;
}
