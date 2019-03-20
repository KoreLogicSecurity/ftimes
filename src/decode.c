/*-
 ***********************************************************************
 *
 * $Id: decode.c,v 1.37 2007/02/23 00:22:35 mavrik Exp $
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
 * Globals
 *
 ***********************************************************************
 */
static unsigned char  gaucBase64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int            giFromBase64[256];

#ifdef WIN32
static char           gacNewLine[NEWLINE_LENGTH] = CRLF;
#else
static char           gacNewLine[NEWLINE_LENGTH] = LF;
#endif
static FILE          *gpFile;

/*-
 ***********************************************************************
 *
 * NOTE: The field order in this table must exactly match the order in
 *       gasCmpMaskTable (mask.c). Mask calculations rely on this.
 *
 ***********************************************************************
 */
static DECODE_TABLE   gasDecodeTable[] = {
  { "z_name",        "name",        DecodeProcessName                 },
  { "z_dev",         "dev",         DecodeProcessDevice               },
  { "z_inode",       "inode",       DecodeProcessInode                },
  { "z_volume",      "volume",      DecodeProcessVolume               },
  { "z_findex",      "findex",      DecodeProcessFileIndex            },
  { "z_mode",        "mode",        DecodeProcessMode                 },
  { "z_attributes",  "attributes",  DecodeProcessAttributes           },
  { "z_nlink",       "nlink",       DecodeProcessLinkCount            },
  { "z_uid",         "uid",         DecodeProcessUserId               },
  { "z_gid",         "gid",         DecodeProcessGroupId              },
  { "z_rdev",        "rdev",        DecodeProcessRDevice              },
  { "z_atime",       "atime",       DecodeProcessATime                },
  { "z_ams",         "ams",         DecodeProcessATimeMs              },
  { "z_mtime",       "mtime",       DecodeProcessMTime                },
  { "z_mms",         "mms",         DecodeProcessMTimeMs              },
  { "z_ctime",       "ctime",       DecodeProcessCTime                },
  { "z_cms",         "cms",         DecodeProcessCTimeMs              },
  { "z_chtime",      "chtime",      DecodeProcessChTime               },
  { "z_chms",        "chms",        DecodeProcessChTimeMs             },
  { "z_size",        "size",        DecodeProcessSize                 },
  { "z_altstreams",  "altstreams",  DecodeProcessAlternateDataStreams },
  { "z_md5",         "md5",         DecodeProcessMd5                  },
  { "z_sha1",        "sha1",        DecodeProcessSha1                 },
  { "z_sha256",      "sha256",      DecodeProcessSha256               },
  { "z_magic",       "magic",       DecodeProcessMagic                },
};
#define DECODE_TABLE_SIZE sizeof(gasDecodeTable) / sizeof(gasDecodeTable[0])


/*-
 ***********************************************************************
 *
 * Decode32BitHexToDecimal
 *
 ***********************************************************************
 */
int
Decode32BitHexToDecimal(char *pcData, int iLength, K_UINT32 *pui32ValueNew, K_UINT32 *pui32ValueOld, char *pcError)
{
  const char          acRoutine[] = "Decode32BitHexToDecimal()";
  int                 i = 0;
  int                 iSign = 0;
  K_UINT32            ui32 = 0;

  /*-
   *********************************************************************
   *
   * Read hex number from pcData and store result in *pui32ValueNew.
   * If hex number preceeded by +, then read hex number, add to
   * pui32ValueOld and store result in *pui32ValueNew. If hex number
   * preceeded by -, then read hex number, subtract from pui32ValueOld
   * and store result in *pui32ValueNew. Hex number must be null
   * terminated.
   *
   *********************************************************************
   */
  if (pui32ValueOld == NULL && (pcData[0] == '#' || pcData[0] == '+' || pcData[0] == '-'))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
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
          snprintf(pcError, MESSAGE_SIZE, "%s: Bad hex digit.", acRoutine);
          return ER;
        }
        i++;
      }
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Field length exceeds 8 digits!", acRoutine);
      return ER;
    }
    if (iSign != 0)
    {
      ui32 = (*pui32ValueOld) + iSign * ui32;
    }
  }

  *pui32ValueNew = ui32;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * Decode64BitHexToDecimal
 *
 ***********************************************************************
 */
int
Decode64BitHexToDecimal(char *pcData, int iLength, K_UINT64 *pui64ValueNew, K_UINT64 *pui64ValueOld, char *pcError)
{
  const char          acRoutine[] = "Decode64BitHexToDecimal()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i = 0;
  int                 iError = 0;
  K_UINT32            ui32UpperNew = 0;
  K_UINT32            ui32LowerNew = 0;
  K_UINT32            ui32LowerOld = 0;
  K_UINT64            ui64 = 0;

  if (pui64ValueOld == NULL && pcData[0] == '#')
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
    return ER;
  }

  if (pcData[0] == '#')
  {
    ui32UpperNew = (K_UINT32) (*pui64ValueOld >> 32);
    ui32LowerOld = (K_UINT32) (*pui64ValueOld & 0xffffff);
    i++;
    iLength--;
    iError = Decode32BitHexToDecimal(&pcData[i], iLength, &ui32LowerNew, &ui32LowerOld, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }

    *pui64ValueNew = (((K_UINT64) ui32UpperNew) << 32) | ui32LowerNew;
    return ER_OK;
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
        snprintf(pcError, MESSAGE_SIZE, "%s: Bad hex digit.", acRoutine);
        return ER;
      }
      i++;
    }
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Field length exceeds 16 digits!", acRoutine);
    return ER;
  }

  *pui64ValueNew = ui64;

  return ER_OK;
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
  int                 i = 0;

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
 * DecodeClearRecord
 *
 ***********************************************************************
 */
void
DecodeClearRecord(DECODE_RECORD *psRecord, int iFieldCount)
{
  int                 i = 0;

  psRecord->acLine[0] = 0;
  psRecord->iLineLength = 0;
  for (i = 0; i < iFieldCount; i++)
  {
    psRecord->ppcFields[i][0] = 0;
  }
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
  int                 i = 0;
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * Input: YYYYMMDDHHMMSS --> Output: YYYY-MM-DD HH:MM:SS
   *
   *********************************************************************
   */
  if (iLength != (int) strlen("YYYYMMDDHHMMSS"))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Field length != %d!", acRoutine, (int) strlen("YYYYMMDDHHMMSS"));
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
  pcOutput[n] = 0;

  return n;
}


/*-
 ***********************************************************************
 *
 * DecodeFormatTime
 *
 ***********************************************************************
 */
int
DecodeFormatTime(K_UINT32 *pui32Time, char *pcTime)
{
  int                 iError = 0;
#ifdef WIN32
  FILETIME            sFileTime;
  int                 iCount = 0;
  K_UINT64            ui64Time = 0;
  SYSTEMTIME          sSystemTime;
#else
  time_t              tTime = (time_t) *pui32Time;
#endif

  pcTime[0] = 0;

#ifdef WIN32
  ui64Time = (((((K_UINT64) *pui32Time) * 1000)) * 10000) + UNIX_EPOCH_IN_NT_TIME;
  sFileTime.dwLowDateTime = (K_UINT32) (ui64Time & (K_UINT32) 0xffffffff);
  sFileTime.dwHighDateTime = (DWORD) (ui64Time >> 32);
  if (!FileTimeToSystemTime(&sFileTime, &sSystemTime))
  {
/* FIXME Return an error message. */
    return ER;
  }
  iCount = snprintf(
    pcTime,
    DECODE_TIME_FORMAT_SIZE,
    DECODE_TIME_FORMAT,
    sSystemTime.wYear,
    sSystemTime.wMonth,
    sSystemTime.wDay,
    sSystemTime.wHour,
    sSystemTime.wMinute,
    sSystemTime.wSecond
    );
  if (iCount != DECODE_TIME_FORMAT_SIZE - 1)
  {
/* FIXME Return an error message. */
    return ER;
  }
#else
  iError = TimeFormatTime(&tTime, pcTime);
  if (iError == ER)
  {
/* FIXME Return an error message. */
    return ER;
  }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeFreeSnapshotContext
 *
 ***********************************************************************
 */
void
DecodeFreeSnapshotContext(SNAPSHOT_CONTEXT *psSnapshot)
{
  int                 i = 0;
  int                 j = 0;

  if (psSnapshot != NULL)
  {
    if (psSnapshot->psDecodeMap != NULL)
    {
      free(psSnapshot->psDecodeMap);
    }
    for (i = 0; i < DECODE_RECORD_COUNT; i++)
    {
      if (psSnapshot->asRecords[i].ppcFields != NULL)
      {
        for (j = 0; j < psSnapshot->iFieldCount; j++)
        {
          if (psSnapshot->asRecords[i].ppcFields[j] != NULL)
          {
            free(psSnapshot->asRecords[i].ppcFields[j]);
          }
        }
        free(psSnapshot->asRecords[i].ppcFields);
      }
    }
    free(psSnapshot);
  }
}


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
  int                 i = 0;
  int                 j = 0;
  int                 iLeft = 0;
  K_UINT32            ui32 = 0;

  for (i = j = iLeft = 0, ui32 = 0; i < iLength; i++)
  {
    if (giFromBase64[(int) pcData[i]] < 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Illegal base64 character", acRoutine);
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
 * DecodeGetTableLength
 *
 ***********************************************************************
 */
int
DecodeGetTableLength(void)
{
  return DECODE_TABLE_SIZE;
}


/*-
 ***********************************************************************
 *
 * DecodeNewSnapshotContext
 *
 ***********************************************************************
 */
SNAPSHOT_CONTEXT *
DecodeNewSnapshotContext(char *pcError)
{
  const char          acRoutine[] = "DecodeNewSnapshotContext()";
  SNAPSHOT_CONTEXT   *psSnapshot = NULL;
  int                 i = 0;
  int                 j = 0;

  psSnapshot = (SNAPSHOT_CONTEXT *) calloc(sizeof(SNAPSHOT_CONTEXT), 1);
  if (psSnapshot == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  psSnapshot->psDecodeMap = (DECODE_TABLE *) calloc(sizeof(DECODE_TABLE), DECODE_TABLE_SIZE);
  if (psSnapshot->psDecodeMap == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    DecodeFreeSnapshotContext(psSnapshot);
    return NULL;
  }

  for (i = 0; i < DECODE_RECORD_COUNT; i++)
  {
    psSnapshot->asRecords[i].ppcFields = (char **) calloc(DECODE_TABLE_SIZE, sizeof(char **));
    if (psSnapshot->asRecords[i].ppcFields == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
      DecodeFreeSnapshotContext(psSnapshot);
      return NULL;
    }

    for (j = 0; j < DECODE_TABLE_SIZE; j++)
    {
      psSnapshot->asRecords[i].ppcFields[j] = (char *) calloc(DECODE_MAX_LINE, 1);
      if (psSnapshot->asRecords[i].ppcFields[j] == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
        DecodeFreeSnapshotContext(psSnapshot);
        return NULL;
      }
    }
  }

  return psSnapshot;
}


/*-
 ***********************************************************************
 *
 * DecodeOpenSnapshot
 *
 ***********************************************************************
 */
int
DecodeOpenSnapshot(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeOpenSnapshot()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;

  psSnapshot->pFile = SupportGetFileHandle(psSnapshot->pcFile, acLocalError);
  if (psSnapshot->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, psSnapshot->pcFile, psSnapshot->iLineNumber, acLocalError);
    return ER;
  }

  if (DecodeReadLine(psSnapshot, acLocalError) == NULL && ferror(psSnapshot->pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, psSnapshot->pcFile, psSnapshot->iLineNumber, acLocalError);
    fclose(psSnapshot->pFile);
    return ER;
  }

  iError = DecodeParseHeader(psSnapshot, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, psSnapshot->pcFile, psSnapshot->iLineNumber, acLocalError);
    fclose(psSnapshot->pFile);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeParseHeader
 *
 ***********************************************************************
 */
int
DecodeParseHeader(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeParseHeader()";
  char                acLegacyName[DECODE_FIELDNAME_SIZE] = { 0 };
  char               *pc = NULL;
  int                 i = 0;
  int                 iIndex = 0;

  /*-
   *********************************************************************
   *
   * First, check to see that there's at least a name field.
   *
   *********************************************************************
   */
  if (strcmp(psSnapshot->psCurrRecord->acLine, "name") == 0 || strncmp(psSnapshot->psCurrRecord->acLine, "name|", 5) == 0)
  {
    psSnapshot->iCompressed = 0;
    psSnapshot->iLegacyFile = 0;
  }
  else if (strcmp(psSnapshot->psCurrRecord->acLine, "z_name") == 0 || strncmp(psSnapshot->psCurrRecord->acLine, "z_name|", 7) == 0)
  {
    psSnapshot->iCompressed = 1;
    psSnapshot->iLegacyFile = 0;
  }
  else if (strcmp(psSnapshot->psCurrRecord->acLine, "zname") == 0 || strncmp(psSnapshot->psCurrRecord->acLine, "zname|", 6) == 0)
  {
    psSnapshot->iCompressed = 1;
    psSnapshot->iLegacyFile = 1;
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Header magic is not recognized.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Then, parse the header, and construct a decode table.
   *
   *********************************************************************
   */
  for (iIndex = 0, pc = strtok(psSnapshot->psCurrRecord->acLine, DECODE_SEPARATOR_S); pc != NULL; pc = strtok(NULL, DECODE_SEPARATOR_S), iIndex++)
  {
    for (i = 0; i < DECODE_TABLE_SIZE; i++)
    {
      if (psSnapshot->iCompressed)
      {
        if (strcmp(gasDecodeTable[i].acZName, pc) == 0)
        {
          psSnapshot->aiIndex2Map[iIndex] = i;
          psSnapshot->psDecodeMap[i] = gasDecodeTable[i]; /* This initializes all elements at the given index. */
          PUTBIT(psSnapshot->ulFieldMask, 1, i);
          break;
        }
        if (psSnapshot->iLegacyFile)
        {
          snprintf(acLegacyName, DECODE_FIELDNAME_SIZE, "z_%s", pc);
          acLegacyName[DECODE_FIELDNAME_SIZE] = 0;
          if (strcmp(gasDecodeTable[i].acZName, acLegacyName) == 0 || strcmp("zname", pc) == 0)
          {
            psSnapshot->aiIndex2Map[iIndex] = i;
            psSnapshot->psDecodeMap[i] = gasDecodeTable[i]; /* This initializes all elements at the given index. */
            PUTBIT(psSnapshot->ulFieldMask, 1, i);
            break;
          }
        }
      }
      else
      {
        if (strcmp(gasDecodeTable[i].acUName, pc) == 0)
        {
          psSnapshot->aiIndex2Map[iIndex] = i;
          psSnapshot->psDecodeMap[i] = gasDecodeTable[i]; /* This initializes all elements at the given index. */
          psSnapshot->psDecodeMap[i].piRoutine = DecodeProcessNada; /* Override the normal decode routine. */
          PUTBIT(psSnapshot->ulFieldMask, 1, i);
          break;
        }
      }
    }
    if (i == DECODE_TABLE_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Field = [%s]: Unrecognized field.", acRoutine, pc);
      return ER;
    }
  }

  /*-
   *********************************************************************
   *
   * Finally, record the last field index -- it's used to represent the
   * length of the decode table.
   *
   *********************************************************************
   */
  psSnapshot->iFieldCount = iIndex;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeParseRecord
 *
 ***********************************************************************
 */
int
DecodeParseRecord(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acName[DECODE_MAX_LINE] = { 0 };
  char               *pcName = NULL;
  char               *pcToken = NULL;
  int                 i = 0;
  int                 iDone = 0;
  int                 iError = 0;
  int                 iLength = 0;
  int                 iOffset = 0;
  int                 iFieldCount = 1;
  int                 iFieldIndex = 0;

  /*-
   *********************************************************************
   *
   * Scan the line -- stopping at each delimiter to decode the field.
   *
   *********************************************************************
   */
  pcToken = psSnapshot->psCurrRecord->acLine;
  while (!iDone)
  {
    if (psSnapshot->psCurrRecord->acLine[iOffset] == DECODE_SEPARATOR_C || psSnapshot->psCurrRecord->acLine[iOffset] == 0)
    {
      if (iFieldCount > psSnapshot->iFieldCount)
      {
        snprintf(pcError, MESSAGE_SIZE, "File = [%s], Line = [%d], FieldCount = [%d] > [%d]: FieldCount exceeds expected value.",
          psSnapshot->pcFile,
          psSnapshot->iLineNumber,
          iFieldCount,
          psSnapshot->iFieldCount
          );
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
        return ER;
      }
      if (psSnapshot->psCurrRecord->acLine[iOffset] == 0)
      {
        iDone = 1;
      }
      psSnapshot->psCurrRecord->acLine[iOffset] = 0; /* Terminate the field value. */
      iError = psSnapshot->psDecodeMap[psSnapshot->aiIndex2Map[iFieldIndex]].piRoutine(
        &psSnapshot->sDecodeState,
        pcToken,
        iLength,
        psSnapshot->psCurrRecord->ppcFields[psSnapshot->aiIndex2Map[iFieldIndex]],
        acLocalError
        );
      if (iError == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "File = [%s], Line = [%d], Field = [%s]: %s",
          psSnapshot->pcFile,
          psSnapshot->iLineNumber,
          (psSnapshot->iCompressed) ? psSnapshot->psDecodeMap[psSnapshot->aiIndex2Map[iFieldIndex]].acZName : psSnapshot->psDecodeMap[psSnapshot->aiIndex2Map[iFieldIndex]].acUName,
          acLocalError
          );
        ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
        return ER;
      }
      if (!iDone)
      {
        iOffset++;
        iFieldCount++;
        iFieldIndex++;
        pcToken = &psSnapshot->psCurrRecord->acLine[iOffset];
        iLength = 0;
      }
    }
    else
    {
      iOffset++;
      iLength++;
    }
  }
  if (iFieldCount != psSnapshot->iFieldCount)
  {
    snprintf(pcError, MESSAGE_SIZE, "File = [%s], Line = [%d], FieldCount = [%d] != [%d]: FieldCount mismatch!",
      psSnapshot->pcFile,
      psSnapshot->iLineNumber,
      iFieldCount,
      psSnapshot->iFieldCount
      );
    ErrorHandler(ER_Failure, pcError, ERROR_FAILURE);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Compute an MD5 hash of the name field. If the name does not begin
   * with a slash ('/'), assume that it is Windows-based, and convert
   * to lower case before computing its hash. Note: Windows-based names
   * are not case sensitive. Usually, the original case is preserved at
   * file creation, but there is no guarantee that the case for a given
   * file will be the same from snapshot to snapshot. Therefore, always
   * convert Windows-based names to lower case before computing their
   * hash.
   *
   *********************************************************************
   */
  pcName = psSnapshot->psCurrRecord->ppcFields[0];
  iLength = strlen(pcName);
  if (pcName[1] != '/')
  {
    snprintf(acName, DECODE_MAX_LINE, "%s", pcName);
    for (i = 0; i < iLength; i++)
    {
      acName[i] = tolower((int) acName[i]);
    }
    pcName = acName;
  }
  MD5HashString((unsigned char *) pcName, iLength, psSnapshot->psCurrRecord->aucHash);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessATime
 *
 ***********************************************************************
 */
int
DecodeProcessATime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessATime()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acTimeString[DECODE_TIME_FORMAT_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->atime, psDecodeState->patime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == '~')
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->atime, psDecodeState->patime);
    iError = DecodeFormatOutOfBandTime(&pcToken[1], iLength - 1, acTimeString, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    snprintf(pcOutput, DECODE_TIME_FORMAT_SIZE, "%s", acTimeString);
    return ER_OK;
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->patime != NULL)
    {
      ui32Value = *psDecodeState->patime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->patime, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  iError = DecodeFormatTime(&ui32Value, pcOutput);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->atime, psDecodeState->patime, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessATimeMs
 *
 ***********************************************************************
 */
int
DecodeProcessATimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessATimeMs()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pams != NULL)
    {
      ui32Value = *psDecodeState->pams;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pams, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->ams, psDecodeState->pams, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessAlternateDataStreams
 *
 ***********************************************************************
 */
int
DecodeProcessAlternateDataStreams(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessAlternateDataStreams()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->altstreams, psDecodeState->paltstreams);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->paltstreams, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->altstreams, psDecodeState->paltstreams);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->altstreams, psDecodeState->paltstreams, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessAttributes
 *
 ***********************************************************************
 */
int
DecodeProcessAttributes(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessAttributes()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->attributes, psDecodeState->pattributes);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pattributes, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->attributes, psDecodeState->pattributes);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->attributes, psDecodeState->pattributes, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessCTime
 *
 ***********************************************************************
 */
int
DecodeProcessCTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessCTime()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acTimeString[DECODE_TIME_FORMAT_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->ctime, psDecodeState->pctime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == '~')
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->ctime, psDecodeState->pctime);
    iError = DecodeFormatOutOfBandTime(&pcToken[1], iLength - 1, acTimeString, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    snprintf(pcOutput, DECODE_TIME_FORMAT_SIZE, "%s", acTimeString);
    return ER_OK;
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pctime != NULL)
    {
      ui32Value = *psDecodeState->pctime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->patime != NULL)
    {
      ui32Value = *psDecodeState->patime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current atime value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Y':
    if (psDecodeState->pmtime != NULL)
    {
      ui32Value = *psDecodeState->pmtime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current mtime value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Z':
    if (psDecodeState->pctime != NULL)
    {
      ui32Value = *psDecodeState->pctime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current ctime value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pctime, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  iError = DecodeFormatTime(&ui32Value, pcOutput);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->ctime, psDecodeState->pctime, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessCTimeMs
 *
 ***********************************************************************
 */
int
DecodeProcessCTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessCTimeMs()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pcms != NULL)
    {
      ui32Value = *psDecodeState->pcms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->pams != NULL)
    {
      ui32Value = *psDecodeState->pams;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current ams value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Y':
    if (psDecodeState->pmms != NULL)
    {
      ui32Value = *psDecodeState->pmms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current mms value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pcms, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->cms, psDecodeState->pcms, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessChTime
 *
 ***********************************************************************
 */
int
DecodeProcessChTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessChTime()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acTimeString[DECODE_TIME_FORMAT_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == '~')
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    iError = DecodeFormatOutOfBandTime(&pcToken[1], iLength - 1, acTimeString, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    snprintf(pcOutput, DECODE_TIME_FORMAT_SIZE, "%s", acTimeString);
    return ER_OK;
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pchtime != NULL)
    {
      ui32Value = *psDecodeState->pchtime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->patime != NULL)
    {
      ui32Value = *psDecodeState->patime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current atime value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Y':
    if (psDecodeState->pmtime != NULL)
    {
      ui32Value = *psDecodeState->pmtime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current mtime value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Z':
    if (psDecodeState->pctime != NULL)
    {
      ui32Value = *psDecodeState->pctime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current ctime value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pchtime, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  iError = DecodeFormatTime(&ui32Value, pcOutput);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessChTimeMs
 *
 ***********************************************************************
 */
int
DecodeProcessChTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessChTimeMs()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pchms != NULL)
    {
      ui32Value = *psDecodeState->pchms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->pams != NULL)
    {
      ui32Value = *psDecodeState->pams;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current ams value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Y':
    if (psDecodeState->pmms != NULL)
    {
      ui32Value = *psDecodeState->pmms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current mms value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'Z':
    if (psDecodeState->pcms != NULL)
    {
      ui32Value = *psDecodeState->pcms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current cms value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pchms, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->chms, psDecodeState->pchms, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessDevice
 *
 ***********************************************************************
 */
int
DecodeProcessDevice(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessDevice()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->dev, psDecodeState->pdev);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pdev, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->dev, psDecodeState->pdev);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->dev, psDecodeState->pdev, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessFileIndex
 *
 ***********************************************************************
 */
int
DecodeProcessFileIndex(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessFileIndex()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT64            ui64Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->findex, psDecodeState->pfindex);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode64BitHexToDecimal(pcToken, iLength, &ui64Value, psDecodeState->pfindex, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->findex, psDecodeState->pfindex);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->findex, psDecodeState->pfindex, ui64Value);
#ifdef WIN32
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%I64u", ui64Value);
#else
  #ifdef USE_AP_SNPRINTF
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%qu", (unsigned long long) ui64Value);
  #else
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long) ui64Value);
  #endif
#endif
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessGroupId
 *
 ***********************************************************************
 */
int
DecodeProcessGroupId(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessGroupId()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->gid, psDecodeState->pgid);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pgid, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->gid, psDecodeState->pgid);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->gid, psDecodeState->pgid, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessInode
 *
 ***********************************************************************
 */
int
DecodeProcessInode(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessInode()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->inode, psDecodeState->pinode);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pinode, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->inode, psDecodeState->pinode);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->inode, psDecodeState->pinode, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessLinkCount
 *
 ***********************************************************************
 */
int
DecodeProcessLinkCount(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessLinkCount()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->nlink, psDecodeState->pnlink);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pnlink, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->nlink, psDecodeState->pnlink);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->nlink, psDecodeState->pnlink, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessMTime
 *
 ***********************************************************************
 */
int
DecodeProcessMTime(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessMTime()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acTimeString[DECODE_TIME_FORMAT_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->mtime, psDecodeState->pmtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == '~')
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->mtime, psDecodeState->pmtime);
    iError = DecodeFormatOutOfBandTime(&pcToken[1], iLength - 1, acTimeString, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    snprintf(pcOutput, DECODE_TIME_FORMAT_SIZE, "%s", acTimeString);
    return ER_OK;
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pmtime != NULL)
    {
      ui32Value = *psDecodeState->pmtime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->patime != NULL)
    {
      ui32Value = *psDecodeState->patime;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current atime value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pmtime, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  iError = DecodeFormatTime(&ui32Value, pcOutput);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->mtime, psDecodeState->pmtime, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessMTimeMs
 *
 ***********************************************************************
 */
int
DecodeProcessMTimeMs(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessMTimeMs()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->chtime, psDecodeState->pchtime);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  switch (pcToken[0])
  {
  case '#':
    if (psDecodeState->pmms != NULL)
    {
      ui32Value = *psDecodeState->pmms;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected previous value to be defined!", acRoutine);
      return ER;
    }
    break;
  case 'X':
    if (psDecodeState->pams != NULL)
    {
      ui32Value = *psDecodeState->pams;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Expected current ams value to be defined!", acRoutine);
      return ER;
    }
    break;
  default:
    iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pmms, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    break;
  }

  snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);

  DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->mms, psDecodeState->pmms, ui32Value);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessMagic
 *
 ***********************************************************************
 */
int
DecodeProcessMagic(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  snprintf(pcOutput, DECODE_MAX_LINE, "%s", pcToken);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessMd5
 *
 ***********************************************************************
 */
int
DecodeProcessMd5(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessMd5()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  unsigned char       aucFileMd5[MD5_HASH_SIZE] = { 0 };

  if (iLength <= 0)
  {
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == 'D' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "DIRECTORY");
  }
  else if (pcToken[0] == 'L' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SYMLINK");
  }
  else if (pcToken[0] == 'S' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SPECIAL");
  }
  else
  {
    iError = DecodeGetBase64Hash(pcToken, aucFileMd5, iLength, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    MD5HashToHex(aucFileMd5, pcOutput);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessMode
 *
 ***********************************************************************
 */
int
DecodeProcessMode(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessMode()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->mode, psDecodeState->pmode);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pmode, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->mode, psDecodeState->pmode);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->mode, psDecodeState->pmode, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%o", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessNada
 *
 ***********************************************************************
 */
int
DecodeProcessNada(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  snprintf(pcOutput, DECODE_MAX_LINE, "%s", pcToken);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessName
 *
 ***********************************************************************
 */
int
DecodeProcessName(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessName()";
  int                 i = 0;
  int                 j = 0;
  unsigned int        iFromLastLine = 0;

  if (iLength < 2)
  {
    pcOutput[0] = 0;
    snprintf(pcError, MESSAGE_SIZE, "%s: Name length must be >= 2.", acRoutine);
    return ER;
  }

  /*
   *********************************************************************
   *
   * Get 2-byte prefix (i.e., the number of repeated bytes in name)
   *
   *********************************************************************
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
      snprintf(pcError, MESSAGE_SIZE, "%s: Illegal 2-byte prefix.", acRoutine);
      return ER;
    }
  }
  if (iFromLastLine > strlen(psDecodeState->name))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Not enough bytes available to perform decode.", acRoutine);
    return ER;
  }

  /*
   *********************************************************************
   *
   * Copy the rest of the current name to the previous name.
   *
   *********************************************************************
   */
  if (iFromLastLine == 0)
  {
    psDecodeState->name[0] = '"';
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
    psDecodeState->name[j++] = pcToken[i++];
  }
  if (pcToken[i] != '"')
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Names must end with a double quote.", acRoutine);
    return ER;
  }
  psDecodeState->name[j++] = pcToken[i++];
  psDecodeState->name[j] = 0;

  snprintf(pcOutput, DECODE_MAX_LINE, "%s", psDecodeState->name);

  /*
   *********************************************************************
   *
   * Sanity check the name value.
   *
   *********************************************************************
   */
  if (psDecodeState->name[0] != '"' || psDecodeState->name[j - 1] != '"')
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Names must be enclosed in double quotes.", acRoutine);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessRDevice
 *
 ***********************************************************************
 */
int
DecodeProcessRDevice(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessRDevice()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->rdev, psDecodeState->prdev);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->prdev, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->rdev, psDecodeState->prdev);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->rdev, psDecodeState->prdev, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessSha1
 *
 ***********************************************************************
 */
int
DecodeProcessSha1(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessSha1()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  unsigned char       aucFileSha1[SHA1_HASH_SIZE] = { 0 };

  if (iLength <= 0)
  {
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == 'D' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "DIRECTORY");
  }
  else if (pcToken[0] == 'L' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SYMLINK");
  }
  else if (pcToken[0] == 'S' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SPECIAL");
  }
  else
  {
    iError = DecodeGetBase64Hash(pcToken, aucFileSha1, iLength, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }
    SHA1HashToHex(aucFileSha1, pcOutput);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessSha256
 *
 ***********************************************************************
 */
int
DecodeProcessSha256(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessSha256()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  unsigned char       aucFileSha256[SHA256_HASH_SIZE] = { 0 };

  if (iLength <= 0)
  {
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  if (pcToken[0] == 'D' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "DIRECTORY");
  }
  else if (pcToken[0] == 'L' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SYMLINK");
  }
  else if (pcToken[0] == 'S' && pcToken[1] == 0)
  {
    snprintf(pcOutput, DECODE_MAX_LINE, "SPECIAL");
  }
  else
  {
    iError = DecodeGetBase64Hash(pcToken, aucFileSha256, iLength, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }
    SHA256HashToHex(aucFileSha256, pcOutput);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessSize
 *
 ***********************************************************************
 */
int
DecodeProcessSize(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessSize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT64            ui64Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->size, psDecodeState->psize);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode64BitHexToDecimal(pcToken, iLength, &ui64Value, psDecodeState->psize, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->size, psDecodeState->psize);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->size, psDecodeState->psize, ui64Value);
#ifdef WIN32
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%I64u", ui64Value);
#else
  #ifdef USE_AP_SNPRINTF
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%qu", (unsigned long long) ui64Value);
  #else
    snprintf(pcOutput, FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long) ui64Value);
  #endif
#endif
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessUserId
 *
 ***********************************************************************
 */
int
DecodeProcessUserId(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessUserId()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->uid, psDecodeState->puid);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->puid, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->uid, psDecodeState->puid);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->uid, psDecodeState->puid, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeProcessVolume
 *
 ***********************************************************************
 */
int
DecodeProcessVolume(DECODE_STATE *psDecodeState, char *pcToken, int iLength, char *pcOutput, char *pcError)
{
  const char          acRoutine[] = "DecodeProcessVolume()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;
  K_UINT32            ui32Value = 0;

  if (iLength <= 0)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->volume, psDecodeState->pvolume);
    pcOutput[0] = 0;
    return ER_OK; /* It's not an error if a value is missing. */
  }

  /*-
   *********************************************************************
   *
   * If the decode routine returns an error, clear the previous value
   * and NULLify its pointer. A NULL pointer signifies that the value
   * is undefined (and, therefore, not to be used). If the decode
   * routine returns success, record the decoded value in the output
   * buffer, save it as the new previous value, and update its pointer
   * as it may have been NULLified in a previous round.
   *
   *********************************************************************
   */
  iError = Decode32BitHexToDecimal(pcToken, iLength, &ui32Value, psDecodeState->pvolume, acLocalError);
  if (iError == ER)
  {
    DECODE_UNDEFINE_PREV_NUMBER_VALUE(psDecodeState->volume, psDecodeState->pvolume);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  else
  {
    DECODE_DEFINE_PREV_NUMBER_VALUE(psDecodeState->volume, psDecodeState->pvolume, ui32Value);
    snprintf(pcOutput, FTIMES_MAX_32BIT_SIZE, "%u", ui32Value);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeReadLine
 *
 ***********************************************************************
 */
char *
DecodeReadLine(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeReadLine()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };

  do
  {
    /*-
     *******************************************************************
     *
     * Flip the current and previous records. This is how we maintain
     * state from line to line when decoding a compressed file. The
     * current line number is used to determine the new index values,
     * so it must not be modified anywhere else in the code.
     *
     *******************************************************************
     */
    psSnapshot->psCurrRecord = &psSnapshot->asRecords[(0 + psSnapshot->iLineNumber) % 2];
    psSnapshot->psPrevRecord = &psSnapshot->asRecords[(1 + psSnapshot->iLineNumber) % 2];
    psSnapshot->iLineNumber++;

    /*-
     *******************************************************************
     *
     * Wipe the slate clean for the current (i.e., new) record.
     *
     *******************************************************************
     */
    DecodeClearRecord(psSnapshot->psCurrRecord, psSnapshot->iFieldCount);

    /*-
     *******************************************************************
     *
     * Read a line, and prune any EOL characters.
     *
     *******************************************************************
     */
    if (fgets(psSnapshot->psCurrRecord->acLine, DECODE_MAX_LINE, psSnapshot->pFile) == NULL)
    {
      if (ferror(psSnapshot->pFile))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: fgets(): %s", acRoutine, strerror(errno));
        return NULL;
      }
      else
      {
        return NULL; /* Assume we've reached EOF. */
      }
    }

    if (SupportChopEOLs(psSnapshot->psCurrRecord->acLine, feof(psSnapshot->pFile) ? 0 : 1, acLocalError) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return NULL;
    }

    /*-
     *******************************************************************
     *
     * When compression is enabled, FTimes inserts a checkpoint record
     * every COMPRESS_RECOVERY_RATE records. A checkpoint records is
     * an uncompressed record. The following logic is used to recover
     * from a damaged input stream. All records between the bad record
     * and the checkpoint are thrown away. When a new checkpoint is
     * located, the decoder state values are reset (i.e., cleared).
     *
     * Note: This logic does not and should not apply to uncompressed
     * files -- i.e., those are decoded in pass-through mode.
     *
     * Note: Warning messages need to include the snapshot's filename
     * and line number because they are written directly to the error
     * stream.
     *
     *******************************************************************
     */
    if (psSnapshot->iCompressed && psSnapshot->iSkipToNext == TRUE)
    {
      if (strncmp(psSnapshot->psCurrRecord->acLine, DECODE_CHECKPOINT_STRING, DECODE_CHECKPOINT_LENGTH) == 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "File = [%s], Line = [%d]: Checkpoint located. Restarting decoders.", psSnapshot->pcFile, psSnapshot->iLineNumber);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
        memset(&psSnapshot->sDecodeState, 0, sizeof(DECODE_STATE));
        psSnapshot->iSkipToNext = FALSE;
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "File = [%s], Line = [%d]: Record skipped due to previous error.", psSnapshot->pcFile, psSnapshot->iLineNumber);
        ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
        psSnapshot->sDecodeStats.ulSkipped++;
      }
    }

  } while (psSnapshot->iSkipToNext == TRUE);

  psSnapshot->psCurrRecord->iLineLength = strlen(psSnapshot->psCurrRecord->acLine);

  return psSnapshot->psCurrRecord->acLine;
}


/*-
 ***********************************************************************
 *
 * DecodeReadSnapshot
 *
 ***********************************************************************
 */
int
DecodeReadSnapshot(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeReadSnapshot()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;

  /*-
   *********************************************************************
   *
   * Read and process snapshot data. If the read returns NULL, it
   * could mean an error has occured or EOF was reached. If a record
   * fails to parse (compressed files only), set a flag so that the
   * next read will automatically skip all records up to the next
   * checkpoint.
   *
   *********************************************************************
   */
  while (DecodeReadLine(psSnapshot, acLocalError) != NULL)
  {
    psSnapshot->sDecodeStats.ulAnalyzed++;
    iError = DecodeParseRecord(psSnapshot, acLocalError);
    if (iError != ER_OK)
    {
      if (psSnapshot->iCompressed)
      {
        psSnapshot->iSkipToNext = TRUE;
      }
      psSnapshot->sDecodeStats.ulSkipped++;
      continue;
    }
    iError = DecodeWriteRecord(psSnapshot, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, psSnapshot->pcFile, psSnapshot->iLineNumber, acLocalError);
      return ER;
    }
    psSnapshot->sDecodeStats.ulDecoded++;
  }
  if (ferror(psSnapshot->pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, psSnapshot->pcFile, psSnapshot->iLineNumber, acLocalError);
    psSnapshot->sDecodeStats.ulSkipped++;
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecodeSetNewLine
 *
 ***********************************************************************
 */
void
DecodeSetNewLine(char *pcNewLine)
{
  strncpy(gacNewLine, (strcmp(pcNewLine, CRLF) == 0) ? CRLF : LF, NEWLINE_LENGTH);
}


/*-
 ***********************************************************************
 *
 * DecodeSetOutputStream
 *
 ***********************************************************************
 */
void
DecodeSetOutputStream(FILE *pFile)
{
  gpFile = pFile;
}


/*-
 ***********************************************************************
 *
 * DecodeWriteHeader
 *
 ***********************************************************************
 */
int
DecodeWriteHeader(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeWriteHeader()";
  char                acHeader[DECODE_MAX_LINE] = { 0 };
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i = 0;
  int                 iError = 0;
  int                 n = 0;

  for (i = n = 0; i < psSnapshot->iFieldCount; i++)
  {
    n += sprintf(&acHeader[n], "%s%s", (i > 0) ? DECODE_SEPARATOR_S : "", psSnapshot->psDecodeMap[psSnapshot->aiIndex2Map[i]].acUName);
  }
  n += sprintf(&acHeader[n], "%s", gacNewLine);

  iError = SupportWriteData(gpFile, acHeader, n, acLocalError);
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
 * DecodeWriteRecord
 *
 ***********************************************************************
 */
int
DecodeWriteRecord(SNAPSHOT_CONTEXT *psSnapshot, char *pcError)
{
  const char          acRoutine[] = "DecodeWriteRecord()";
  char                acOutput[(DECODE_FIELD_COUNT)*(DECODE_MAX_LINE)]; /* Don't initialize this with '{ 0 }' -- it's a hugh performance hit. */
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i = 0;
  int                 iError = 0;
  int                 n = 0;

  for (i = n = 0; i < psSnapshot->iFieldCount; i++)
  {
    n += sprintf(&acOutput[n], "%s%s", (i > 0) ? DECODE_SEPARATOR_S : "", psSnapshot->psCurrRecord->ppcFields[psSnapshot->aiIndex2Map[i]]);
  }
  n += sprintf(&acOutput[n], "%s", gacNewLine);

  iError = SupportWriteData(gpFile, acOutput, n, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  return ER_OK;
}
