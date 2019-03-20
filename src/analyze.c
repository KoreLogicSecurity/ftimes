/*
 ***********************************************************************
 *
 * $Id: analyze.c,v 1.7 2003/01/16 21:08:09 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
 * All Rights Reserved.
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
  const char          cRoutine[] = "AnalyzeFile()";
  char                cLocalError[ERRBUF_SIZE];
  int                 i;
  int                 iBufferType;
  int                 iError;
  int                 iNRead;
  unsigned char       ucBuffer[3 * ANALYZE_READ_BUFSIZE];
#ifdef FTimes_WINNT
  BOOL                bResult;
  char               *pcMessage;
  HANDLE              hFile;
#else
  FILE               *pFile;
#endif

  memset(ucBuffer, 0, 3 * ANALYZE_READ_BUFSIZE);

#ifdef FTimes_WINNT
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
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, pcMessage);
    return ER_CreateFile;
  }
#else
  pFile = fopen(psFTData->pcRawPath, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
#ifdef FTimes_WINNT
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
#ifdef FTimes_WINNT
    if (!bResult)
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, pcMessage);
      CloseHandle(hFile);
      return ER_ReadFile;
    }
#else
    if (ferror(pFile))
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
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
#ifdef FTimes_WINNT
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
      iError = psProperties->sAnalysisStages[i].piRoutine(&ucBuffer[ANALYZE_READ_BUFSIZE], iNRead, iBufferType, ANALYZE_READ_BUFSIZE, psFTData, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        ErrorHandler(psProperties->sAnalysisStages[i].iError, pcError, ERROR_FAILURE);
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
#ifdef FTimes_WINNT
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
  strcpy(psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].cDescription, "Digest");
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDigest;
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoDigest;
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
  static struct hash_block sFileHashBlock;

  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    md5_begin(&sFileHashBlock);
  }

  md5_middle(&sFileHashBlock, pucBuffer, iBufferLength);

  if ((iBufferType & ANALYZE_FINAL_BUFFER) == ANALYZE_FINAL_BUFFER)
  {
    md5_end(&sFileHashBlock, psFTData->ucFileMD5);
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
  strcpy(psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].cDescription, "Search");
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDig;
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoDig;
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
  const char          cRoutine[] = "AnalyzeDoDig()";
  char                cLocalError[ERRBUF_SIZE];
  unsigned char      *pucToSearch;
  int                 iError;
  int                 iStopShort;
  int                 iMaxStringLength;
  int                 iNToSearch;
  static unsigned char ucSave[ANALYZE_READ_BUFSIZE];
  static int          iNToSave;
  static int          iSaveOffset;
  static K_UINT64     ui64AbsoluteOffset;

  cLocalError[0] = 0;

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
      snprintf(pcError, ERRBUF_SIZE, "%s: A zero length buffer is illegal unless it is marked as the final buffer.", cRoutine);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Not enough overhead to perform analysis.", cRoutine);
      return ER_Overflow;
    }
    ui64AbsoluteOffset = 0;
    pucToSearch = pucBuffer;
    iNToSearch = iBufferLength;
  }
  else
  {
    pucToSearch = pucBuffer - iNToSave;
    memcpy(pucToSearch, ucSave, iNToSave);
    iNToSearch = iBufferLength + iNToSave;
  }

  /*-
   *********************************************************************
   *
   * Search the input.
   *
   *********************************************************************
   */
  iError = DigSearchData(pucToSearch, iNToSearch, iStopShort, ui64AbsoluteOffset, psFTData->pcNeuteredPath, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  memcpy(ucSave, &pucToSearch[iSaveOffset], iNToSave);

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
  const char          cRoutine[] = "AnalyzeEnableXMagicEngine()";
  char                cLocalError[ERRBUF_SIZE];
  unsigned char       ucMD5[MD5_HASH_LENGTH];
  int                 i,
                      iError,
                      iIndex;
  FILE               *pFile;

  /*-
   *********************************************************************
   *
   * Set up the XMagic analysis function pointer.
   *
   *********************************************************************
   */
  strcpy(psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].cDescription, "XMagic");
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoXMagic;
  psProperties->sAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoXMagic;

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
      if (psProperties->cMagicFileName[0])
      {
        iError = SupportExpandPath(psProperties->cMagicFileName, psProperties->cMagicFileName, FTIMES_MAX_PATH, 1, cLocalError);
      }
      else
      {
        continue;
      }
      break;
    case 1:
      iError = SupportExpandPath(XMAGIC_DEFAULT_LOCATION, psProperties->cMagicFileName, FTIMES_MAX_PATH, 1, cLocalError);
      break;
    case 2:
      iError = SupportExpandPath(XMAGIC_CURRENT_LOCATION, psProperties->cMagicFileName, FTIMES_MAX_PATH, 1, cLocalError);
      break;
    default:
      iError = ER_BadValue;
      snprintf(cLocalError, ERRBUF_SIZE, "bad loop index");
      break;
    }
    if (iError == ER_OK)
    {
      iError = XMagicLoadMagic(psProperties->cMagicFileName, cLocalError);
      if (iError == ER_OK)
      {
        if ((pFile = fopen(psProperties->cMagicFileName, "rb")) != NULL && md5_file(pFile, ucMD5) == ER_OK)
        {
          for (iIndex = 0; iIndex < MD5_HASH_LENGTH; iIndex++)
          {
            sprintf(&psProperties->cMagicHash[iIndex * 2], "%02x", ucMD5[iIndex]);
          }
          psProperties->cMagicHash[MD5_HASH_STRING_LENGTH - 1] = 0;
          fclose(pFile);
        }
        else
        {
          strcpy(psProperties->cMagicHash, "NONE");
        }
        break; /* Very important. This get's us out of the for loop. */
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        return iError;
      }
    }
    psProperties->cMagicFileName[0] = 0;
  }

  if (psProperties->cMagicFileName[0] == 0)
  {
    strcpy(psProperties->cMagicFileName, "NA");
    strcpy(psProperties->cMagicHash, "NA");
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
  const char          cRoutine[] = "AnalyzeDoXMagic()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Magic only inspects the first block of data.
   *
   *********************************************************************
   */
  if ((iBufferType & ANALYZE_FIRST_BUFFER) == ANALYZE_FIRST_BUFFER)
  {
    iError = XMagicTestBuffer(pucBuffer, iBufferLength, psFTData->cType, FTIMES_FILETYPE_BUFSIZE, cLocalError);
    if (iError == ER)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER_XMagic;
    }
  }

  return ER_OK;
}
#endif
