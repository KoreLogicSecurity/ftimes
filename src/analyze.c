/*-
 ***********************************************************************
 *
 * $Id: analyze.c,v 1.14 2004/04/22 02:19:09 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2004 Klayton Monroe, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define ANALYZE_READ_BUFSIZE 0x4000
#define ANALYZE_FIRST_BUFFER      1
#define ANALYZE_FINAL_BUFFER      2

static K_UINT32       gui32Files;
static K_UINT64       gui64Bytes;


/*-
 ***********************************************************************
 *
 * AnalyzeGetByteCount
 *
 ***********************************************************************
 */
K_UINT64
AnalyzeGetByteCount(void)
{
  return gui64Bytes;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetFileCount
 *
 ***********************************************************************
 */
K_UINT32
AnalyzeGetFileCount(void)
{
  return gui32Files;
}


/*-
 ***********************************************************************
 *
 * AnalyzeFile
 *
 ***********************************************************************
 *
 * With each read, AnalyzeFile() places the data into the second
 * third of a buffer that is three times the defined read size.
 * This provides each analysis stage with some overhead and a safety
 * zone. Typically, a stage would use overhead to prepend leftover
 * data from the last read to the data for the current read. In
 * this way each stage sees a continuous stream of data. The safety
 * zone helps to ensure that pattern searching algorithms don't
 * slide into restricted memory. All data is initialized to zero
 * once per file.
 *
 *   +----------------+          +----------------+
 *   |                |          |                |
 *   |                |          |                |
 *   |    OVERHEAD    |          |    OVERHEAD    |
 *   |                |          |                |
 *   |                |    +---> |xxxxx SAVE xxxxx|    +---> ...
 *   +----------------+    |     +----------------+    |
 *   |                |    |     |                |    |
 *   |                |    |     |                |    |
 *   |    1st READ    |    |     |    2nd READ    |    |
 *   |                |    |     |                |    |
 *   |xxxxx SAVE xxxxx| ---+     |                | ---+
 *   +----------------+          +----------------+
 *   |00000 ZERO 00000|          |00000 ZERO 00000|
 *   |                |          |                |
 *   |   SAFETYZONE   |          |   SAFETYZONE   |
 *   |                |          |                |
 *   |                |          |                |
 *   +----------------+          +----------------+
 *
 ***********************************************************************
 */
int
AnalyzeFile(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iBufferType;
  int                 iError;
  int                 iNRead;
  unsigned char       ucBuffer[3 * ANALYZE_READ_BUFSIZE];
#ifdef WINNT
  BOOL                bResult;
  char               *pcMessage;
  HANDLE              hFile;
#else
  FILE               *pFile;
#endif
  K_UINT64            ui64FileSize;

#ifdef WINNT
  ui64FileSize = (((K_UINT64) psFTData->dwFileSizeHigh) << 32) | psFTData->dwFileSizeLow;
#else
  ui64FileSize = (K_UINT64) psFTData->sStatEntry.st_size;
#endif
  if (psProperties->ulFileSizeLimit != 0 && ui64FileSize > (K_UINT64) psProperties->ulFileSizeLimit)
  {
    memset(psFTData->aucFileMD5, 0xff, MD5_HASH_SIZE);
    return ER_OK;
  }

  memset(ucBuffer, 0, 3 * ANALYZE_READ_BUFSIZE);

#ifdef WINNT
  hFile = CreateFile
          (
            psFTData->pcRawPath,
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
            NULL
          );
  if (hFile == INVALID_HANDLE_VALUE)
  {
    ErrorFormatWin32Error(&pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, pcMessage);
    return ER_CreateFile;
  }
#else
  pFile = fopen(psFTData->pcRawPath, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return ER_fopen;
  }
#endif

  /*-
   *********************************************************************
   *
   * Update the global files counter.
   *
   *********************************************************************
   */
  gui32Files++;

  /*-
   *********************************************************************
   *
   * Initialize the buffer type. Each analysis stage executes different logic
   * based on the value of this flag.
   *
   *********************************************************************
   */
  iBufferType = ANALYZE_FIRST_BUFFER;

  while ((iBufferType & ANALYZE_FINAL_BUFFER) != ANALYZE_FINAL_BUFFER)
  {
    /*-
     *******************************************************************
     *
     * Read a hunk of data, and insert it in the middle of our buffer.
     *
     *******************************************************************
     */
#ifdef WINNT
    bResult = ReadFile(hFile, &ucBuffer[ANALYZE_READ_BUFSIZE], ANALYZE_READ_BUFSIZE, &iNRead, NULL);
#else
    iNRead = fread(&ucBuffer[ANALYZE_READ_BUFSIZE], 1, ANALYZE_READ_BUFSIZE, pFile);
#endif

    /*-
     *******************************************************************
     *
     * Update the global bytes counter.
     *
     *******************************************************************
     */
    gui64Bytes += iNRead;

    /*-
     *******************************************************************
     *
     * If there was a read error, close the file and return an error.
     *
     *******************************************************************
     */
#ifdef WINNT
    if (!bResult)
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, pcMessage);
      CloseHandle(hFile);
      return ER_ReadFile;
    }
#else
    if (ferror(pFile))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
      fclose(pFile);
      return ER_fread;
    }
#endif

    /*-
     *******************************************************************
     *
     * If EOF was reached, update the buffer type.
     *
     *******************************************************************
     */
#ifdef WINNT
    if (iNRead == 0)
    {
      iBufferType |= ANALYZE_FINAL_BUFFER;
    }
#else
    if (feof(pFile))
    {
      iBufferType |= ANALYZE_FINAL_BUFFER;
    }
#endif

    /*-
     *******************************************************************
     *
     * Run through the defined analysis stages. Warn the user if a
     * stage fails, and keep on going.
     *
     *******************************************************************
     */
    for (i = 0; i < psProperties->iLastAnalysisStage; i++)
    {
      iError = psProperties->asAnalysisStages[i].piRoutine(&ucBuffer[ANALYZE_READ_BUFSIZE], iNRead, iBufferType, ANALYZE_READ_BUFSIZE, psFTData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(psProperties->asAnalysisStages[i].iError, pcError, ERROR_FAILURE);
      }
    }

    /*-
     *******************************************************************
     *
     * Update the buffer type.
     *
     *******************************************************************
     */
    if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
    {
      iBufferType ^= ANALYZE_FIRST_BUFFER;
    }
  }
#ifdef WINNT
  CloseHandle(hFile);
#else
  fclose(pFile);
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * AnalyzeEnableDigestEngine
 *
 ***********************************************************************
 */
void
AnalyzeEnableDigestEngine(FTIMES_PROPERTIES *psProperties)
{
  strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "Digest");
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDigest;
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoDigest;
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoDigest
 *
 ***********************************************************************
 */
int
AnalyzeDoDigest(unsigned char *pucBuffer, int iBufferLength, int iBufferType, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  static MD5_CONTEXT sFileMD5Context;

  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    MD5Alpha(&sFileMD5Context);
  }

  MD5Cycle(&sFileMD5Context, pucBuffer, iBufferLength);

  if ((iBufferType & ANALYZE_FINAL_BUFFER) == ANALYZE_FINAL_BUFFER)
  {
    MD5Omega(&sFileMD5Context, psFTData->aucFileMD5);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * AnalyzeEnableDigEngine
 *
 ***********************************************************************
 */
void
AnalyzeEnableDigEngine(FTIMES_PROPERTIES *psProperties)
{
  strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "Search");
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDig;
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoDig;
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoDig
 *
 ***********************************************************************
 */
int
AnalyzeDoDig(unsigned char *pucBuffer, int iBufferLength, int iBufferType, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoDig()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  unsigned char      *pucToSearch;
  int                 iError;
  int                 iStopShort;
  int                 iMaxStringLength;
  int                 iNToSearch;
  static unsigned char aucSave[ANALYZE_READ_BUFSIZE];
  static int          iNToSave;
  static int          iSaveOffset;
  static K_UINT64     ui64AbsoluteOffset;

  /*-
   *********************************************************************
   *
   * If this is the final buffer, clear the stop short flag. Otherwise,
   * set the flag and make sure that we have nonzero length buffer.
   *
   *********************************************************************
   */
  if ((iBufferType & ANALYZE_FINAL_BUFFER) == ANALYZE_FINAL_BUFFER)
  {
    iStopShort = 0;
  }
  else
  {
    if (iBufferLength == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: A zero length buffer is illegal unless it is marked as the final buffer.", acRoutine);
      return ER_Length;
    }
    iStopShort = 1;
  }

  /*-
   *********************************************************************
   *
   * If this is the first buffer, initialize. Otherwise, prepend any
   * saved data to the incoming buffer.
   *
   *********************************************************************
   */
  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    DigClearCounts();
    /*-
     *******************************************************************
     *
     * Check to see that there is enough overhead memory available. We
     * set iNToSave to one less than the maximum search string because
     * the search algorithm must stop searching after x bytes have been
     * processed. Where
     *
     *   x = (bufsize - (maxstring - 1))
     *
     * For example, if bufsize = 16 and maxstring = 6, then all but the
     * last 5 bytes can be searched (i.e. to stay in bounds). Therefore,
     * the search routine must stop searching when it reaches byte 11.
     *
     * If no search strings were defined, maxstring should be zero. In
     * that case, iNToSave must not be allowed to go negative.
     *
     *******************************************************************
     */
    iMaxStringLength = DigGetMaxStringLength();
    if (iMaxStringLength <= iBufferOverhead)
    {
      iNToSave = (iMaxStringLength <= 0) ? 0 : iMaxStringLength - 1;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Not enough overhead to perform analysis.", acRoutine);
      return ER_Overflow;
    }
    ui64AbsoluteOffset = 0;
    pucToSearch = pucBuffer;
    iNToSearch = iBufferLength;
  }
  else
  {
    pucToSearch = pucBuffer - iNToSave;
    memcpy(pucToSearch, aucSave, iNToSave);
    iNToSearch = iBufferLength + iNToSave;
  }

  /*-
   *********************************************************************
   *
   * Search the input.
   *
   *********************************************************************
   */
  iError = DigSearchData(pucToSearch, iNToSearch, iStopShort, ui64AbsoluteOffset, psFTData->pcNeuteredPath, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Save any data that could not be fully searched. The amount of data
   * saved is equal to the the length of the longest search string.
   *
   *********************************************************************
   */
  iSaveOffset = iNToSearch - iNToSave;
  memcpy(aucSave, &pucToSearch[iSaveOffset], iNToSave);

  /*-
   *********************************************************************
   *
   * Update the absolute file offset. This ensures that searcher is
   * passed the offset it expects. In other words, subtract off the
   * unsearched portion of the prior search buffer, and keep it for
   * next time.
   *
   *********************************************************************
   */
  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    ui64AbsoluteOffset += iBufferLength - iNToSave;
  }
  else
  {
    ui64AbsoluteOffset += iBufferLength;
  }

  return ER_OK;
}


#ifdef USE_XMAGIC
/*-
 ***********************************************************************
 *
 * AnalyzeEnableXMagicEngine
 *
 ***********************************************************************
 */
int
AnalyzeEnableXMagicEngine(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "AnalyzeEnableXMagicEngine()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  unsigned char       aucMD5[MD5_HASH_SIZE];
  int                 i;
  int                 iError;
  int                 iIndex;
  FILE               *pFile;

  /*-
   *********************************************************************
   *
   * Set up the XMagic analysis function pointer.
   *
   *********************************************************************
   */
  strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "XMagic");
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoXMagic;
  psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoXMagic;

  /*-
   *********************************************************************
   *
   * Seek out and load Magic.
   *
   *********************************************************************
   */
  for (i = 0; i < 3; i++)
  {
    switch (i)
    {
    case 0:
      if (psProperties->acMagicFileName[0])
      {
        iError = SupportExpandPath(psProperties->acMagicFileName, psProperties->acMagicFileName, FTIMES_MAX_PATH, 1, acLocalError);
      }
      else
      {
        continue;
      }
      break;
    case 1:
      iError = SupportExpandPath(XMAGIC_DEFAULT_LOCATION, psProperties->acMagicFileName, FTIMES_MAX_PATH, 1, acLocalError);
      break;
    case 2:
      iError = SupportExpandPath(XMAGIC_CURRENT_LOCATION, psProperties->acMagicFileName, FTIMES_MAX_PATH, 1, acLocalError);
      break;
    default:
      iError = ER_BadValue;
      snprintf(acLocalError, MESSAGE_SIZE, "bad loop index");
      break;
    }
    if (iError == ER_OK)
    {
      iError = XMagicLoadMagic(psProperties->acMagicFileName, acLocalError);
      if (iError == ER_OK)
      {
        if ((pFile = fopen(psProperties->acMagicFileName, "rb")) != NULL && MD5HashStream(pFile, aucMD5) == ER_OK)
        {
          for (iIndex = 0; iIndex < MD5_HASH_SIZE; iIndex++)
          {
            sprintf(&psProperties->acMagicHash[iIndex * 2], "%02x", aucMD5[iIndex]);
          }
          psProperties->acMagicHash[FTIMEX_MAX_MD5_LENGTH - 1] = 0;
          fclose(pFile);
        }
        else
        {
          strcpy(psProperties->acMagicHash, "NONE");
        }
        break; /* Very important. This get's us out of the for loop. */
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return iError;
      }
    }
    psProperties->acMagicFileName[0] = 0;
  }

  if (psProperties->acMagicFileName[0] == 0)
  {
    strcpy(psProperties->acMagicFileName, "NA");
    strcpy(psProperties->acMagicHash, "NA");
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoXMagic
 *
 ***********************************************************************
 */
int
AnalyzeDoXMagic(unsigned char *pucBuffer, int iBufferLength, int iBufferType, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoXMagic()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *********************************************************************
   *
   * Magic only inspects the first block of data.
   *
   *********************************************************************
   */
  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    iError = XMagicTestBuffer(pucBuffer, iBufferLength, psFTData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_XMagic;
    }
  }

  return ER_OK;
}
#endif
