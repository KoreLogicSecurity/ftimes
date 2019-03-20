/*-
 ***********************************************************************
 *
 * $Id: analyze.c,v 1.57 2012/01/04 03:12:27 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2012 The FTimes Project, All Rights Reserved.
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
#define ANALYZE_BLOCK_MULTIPLIER  3
#define ANALYZE_BLOCK_SIZE   0x8000
#define ANALYZE_CARRY_SIZE   0x0400
#define ANALYZE_FIRST_BLOCK       1
#define ANALYZE_FINAL_BLOCK       2

static APP_UI32       gui32Files;
static APP_UI64       gui64ByteCount;
static APP_UI64       gui64Bytes;
static APP_UI64       gui64StartOffset;
static double         gdDps;
static double         gdAnalysisTime;
static int            giAnalyzeBlockSize = ANALYZE_BLOCK_SIZE;
static int            giAnalyzeCarrySize = ANALYZE_CARRY_SIZE;
#ifdef USE_XMAGIC
static int            giAnalyzeStepSize = ANALYZE_BLOCK_SIZE;
#endif

#ifdef WINNT
static HANDLE         ghFile; /* Needed for memory mapped XMagic. */
#else
static int            giFile; /* Needed for memory mapped XMagic. */
#endif

/*-
 ***********************************************************************
 *
 * AnalyzeGetAnalysisTime
 *
 ***********************************************************************
 */
double
AnalyzeGetAnalysisTime(void)
{
  return gdAnalysisTime;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetBlockSize
 *
 ***********************************************************************
 */
int
AnalyzeGetBlockSize(void)
{
  return giAnalyzeBlockSize;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetByteCount
 *
 ***********************************************************************
 */
APP_UI64
AnalyzeGetByteCount(void)
{
  return gui64Bytes;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetCarrySize
 *
 ***********************************************************************
 */
int
AnalyzeGetCarrySize(void)
{
  return giAnalyzeCarrySize;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetDigSaveBuffer
 *
 ***********************************************************************
 */
unsigned char *
AnalyzeGetDigSaveBuffer(int iCarrySize, char *pcError)
{
  const char          acRoutine[] = "AnalyzeGetDigSaveBuffer()";
  static unsigned char *pucBuffer = NULL;

  /*-
   *********************************************************************
   *
   * Allocate iCarrySize bytes of memory for use as a save buffer.
   * This memory should not be freed -- i.e., it should be allocated
   * once and remain active until the program exits.
   *
   *********************************************************************
   */
  if (pucBuffer == NULL)
  {
    pucBuffer = calloc(iCarrySize, 1);
    if (pucBuffer == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
  }

  return pucBuffer;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetDps
 *
 ***********************************************************************
 */
double
AnalyzeGetDps(void)
{
  return gdDps;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetFileCount
 *
 ***********************************************************************
 */
APP_UI32
AnalyzeGetFileCount(void)
{
  return gui32Files;
}


/*-
 ***********************************************************************
 *
 * AnalyzeGetStartOffset
 *
 ***********************************************************************
 */
APP_UI64
AnalyzeGetStartOffset(void)
{
  return gui64StartOffset;
}


#ifdef USE_XMAGIC
/*-
 ***********************************************************************
 *
 * AnalyzeGetStepSize
 *
 ***********************************************************************
 */
int
AnalyzeGetStepSize(void)
{
  return giAnalyzeStepSize;
}
#endif


/*-
 ***********************************************************************
 *
 * AnalyzeGetWorkBuffer
 *
 ***********************************************************************
 */
unsigned char *
AnalyzeGetWorkBuffer(int iBlockSize, char *pcError)
{
  const char          acRoutine[] = "AnalyzeGetWorkBuffer()";
  static unsigned char *pucBuffer = NULL;

  /*-
   *********************************************************************
   *
   * Allocate (ANALYZE_BLOCK_MULTIPLIER * iBlockSize) bytes of memory
   * for use as a work buffer. If the memory was previously allocated,
   * then just zero it out. This memory should not be freed -- i.e.,
   * it should be allocated once and remain active until the program
   * exits.
   *
   *********************************************************************
   */
  if (pucBuffer == NULL)
  {
    pucBuffer = calloc((ANALYZE_BLOCK_MULTIPLIER * iBlockSize), 1);
    if (pucBuffer == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
  }
  else
  {
    memset(pucBuffer, 0, (ANALYZE_BLOCK_MULTIPLIER * iBlockSize));
  }

  return pucBuffer;
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
AnalyzeFile(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 i;
  int                 iBlockSize = AnalyzeGetBlockSize();
  int                 iBlockTag;
  int                 iError;
  int                 iNRead;
  int                 iNToSeek = 0;
  unsigned char      *pucBuffer = NULL;
#ifdef WINNT
  char               *pcMessage;
  HANDLE              hFile;
  int                 iFile;
#endif
  FILE               *pFile;
  APP_UI64            ui64FileSize;
  APP_UI64            ui64NToSeek = 0;

  /*-
   *********************************************************************
   *
   * If the size of the specified file is greater than FileSizeLimit,
   * set its hash to a predefined value and return.
   *
   *********************************************************************
   */
#ifdef WINNT
  ui64FileSize = (((APP_UI64) psFTFileData->dwFileSizeHigh) << 32) | psFTFileData->dwFileSizeLow;
#else
  ui64FileSize = (APP_UI64) psFTFileData->sStatEntry.st_size;
#endif
  if (psProperties->ulFileSizeLimit != 0 && ui64FileSize > (APP_UI64) psProperties->ulFileSizeLimit)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      memset(psFTFileData->aucFileMd5, 0xff, MD5_HASH_SIZE);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      memset(psFTFileData->aucFileSha1, 0xff, SHA1_HASH_SIZE);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      memset(psFTFileData->aucFileSha256, 0xff, SHA256_HASH_SIZE);
    }
    return ER_OK;
  }

  /*-
   *********************************************************************
   *
   * Get a pointer to the work buffer.
   *
   *********************************************************************
   */
  pucBuffer = AnalyzeGetWorkBuffer(iBlockSize, acLocalError);
  if (pucBuffer == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Open the specified file. Since CreateFile() yields better file
   * access, it is the preferred open method on WINNT platforms. Once
   * a valid handle has been obtained, convert it to a file descriptor
   * so fread() may use it. The major benefit of this approach is that
   * it leads to simplified logic -- i.e., it eliminates the need for
   * multipe #ifdef statements to support platform-specific mechanics.
   * It also simplifies the process for detecting EOF, which wasn't
   * an issue until PCRE support was added.
   *
   * Note that MapGetFileHandle() is not used here because the desired
   * access flags differ. Here, we only need to read the file's data.
   * However, the flags used in MapGetFileHandle() were set to obtain
   * attributes for as many files as possible. More importantly, they
   * were not set to read the file's data. Unfortunately, this means
   * that the same file must be opened twice. We should look for ways
   * to combine the code so that a given file is only opened once. To
   * that end, we should look at DuplicateHandle(), which allows the
   * caller to specify desired access flags.
   *
   *********************************************************************
   */
#ifdef WINNT
  hFile = CreateFileW
          (
            psFTFileData->pwcRawPath,
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
    return ER;
  }
  ghFile = hFile; /* Needed for memory mapped XMagic. */
  iFile = _open_osfhandle((long) hFile, 0);
  if (iFile == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: open_osfhandle(): Handle association failed.", acRoutine);
    return ER;
  }
  pFile = fdopen(iFile, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fdopen(): %s", acRoutine, strerror(errno));
    return ER;
  }
#else
  pFile = fopen(psFTFileData->pcRawPath, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): %s", acRoutine, strerror(errno));
    return ER;
  }
  giFile = fileno(pFile); /* Needed for memory mapped XMagic. */
#endif

  /*-
   *********************************************************************
   *
   * Conditionally seek to the specified start offset. Return an error
   * if the file is regular and its size is less than this offset.
   *
   *********************************************************************
   */
  gui64StartOffset = 0;
  if (psProperties->ui64AnalyzeStartOffset)
  {
    if
    (
#ifdef UNIX
      S_ISREG(psFTFileData->sStatEntry.st_mode) &&
#endif
      ui64FileSize < psProperties->ui64AnalyzeStartOffset
    )
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File is smaller than the specified start offset.", acRoutine);
      fclose(pFile);
#ifdef WINNT
      CloseHandle(hFile);
#endif
      return ER;
    }
    ui64NToSeek = psProperties->ui64AnalyzeStartOffset;
    do
    {
      iNToSeek = (ui64NToSeek > (APP_UI64) 0x7fffffff) ? 0x7fffffff : (int) ui64NToSeek;
#ifdef HAVE_FSEEKO
      iError = fseeko(pFile, (off_t) iNToSeek, SEEK_CUR);
#else
      iError = fseek(pFile, iNToSeek, SEEK_CUR);
#endif
      if (iError == ER)
      {
#ifdef HAVE_FSEEKO
        snprintf(pcError, MESSAGE_SIZE, "%s: fseeko(): %s", acRoutine, strerror(errno));
#else
        snprintf(pcError, MESSAGE_SIZE, "%s: fseek(): %s", acRoutine, strerror(errno));
#endif
        fclose(pFile);
#ifdef WINNT
        CloseHandle(hFile);
#endif
        return ER;
      }
      ui64NToSeek -= iNToSeek;
      gui64StartOffset += iNToSeek;
    } while (ui64NToSeek > 0);
  }

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
   * Initialize the block tag. Each analysis stage executes different
   * logic based on the value of this tag.
   *
   *********************************************************************
   */
  iBlockTag = ANALYZE_FIRST_BLOCK;
  gui64ByteCount = 0;

  while ((iBlockTag & ANALYZE_FINAL_BLOCK) != ANALYZE_FINAL_BLOCK)
  {
    /*-
     *******************************************************************
     *
     * Read a block of data, and insert it in the middle of our buffer.
     *
     *******************************************************************
     */
    iNRead = fread(&pucBuffer[iBlockSize], 1, iBlockSize, pFile);

    /*-
     *******************************************************************
     *
     * Update the global byte counters.
     *
     *******************************************************************
     */
    gui64Bytes += iNRead;
    gui64ByteCount += iNRead;

    /*-
     *******************************************************************
     *
     * Determine the current DPS, and apply the brakes as needed.
     *
     *******************************************************************
     */
    AnalyzeThrottleDps(gui64Bytes, psProperties->iAnalyzeMaxDps);

    /*-
     *******************************************************************
     *
     * If there was a read error, close the file and return an error.
     *
     *******************************************************************
     */
    if (ferror(pFile))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
      fclose(pFile);
#ifdef WINNT
      CloseHandle(hFile);
#endif
      return ER;
    }

    /*-
     *******************************************************************
     *
     * If EOF or the specified byte count was reached, update the block
     * tag. If specified byte count was exceeded, reduce the read count
     * by the difference to ensure that extra bytes are not analyzed.
     *
     *******************************************************************
     */
    if (feof(pFile))
    {
      iBlockTag |= ANALYZE_FINAL_BLOCK;
    }
    else
    {
      if (psProperties->ui64AnalyzeByteCount && gui64ByteCount >= psProperties->ui64AnalyzeByteCount)
      {
        APP_UI64 ui64Delta = gui64ByteCount - psProperties->ui64AnalyzeByteCount;
        if (ui64Delta > (APP_UI64) iNRead)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Byte count delta exceeds the read count [%d]. That shouldn't happen.", acRoutine, iNRead);
          fclose(pFile);
#ifdef WINNT
          CloseHandle(hFile);
#endif
          return ER;
        }
        iNRead -= (int) ui64Delta;
        iBlockTag |= ANALYZE_FINAL_BLOCK;
      }
    }

    /*-
     *******************************************************************
     *
     * Run through the defined analysis stages. Warn the user if a
     * stage fails, but keep going.
     *
     *******************************************************************
     */
    for (i = 0; i < psProperties->iLastAnalysisStage; i++)
    {
      iError = psProperties->asAnalysisStages[i].piRoutine(&pucBuffer[iBlockSize], iNRead, iBlockTag, iBlockSize, psFTFileData, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        ErrorHandler(psProperties->asAnalysisStages[i].iError, pcError, ERROR_FAILURE);
      }
    }

    /*-
     *******************************************************************
     *
     * Update the block tag.
     *
     *******************************************************************
     */
    if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
    {
      iBlockTag ^= ANALYZE_FIRST_BLOCK;
#ifdef USE_XMAGIC
      /*-
       *****************************************************************
       *
       * If XMagic was the only type of analysis requested, we're done.
       *
       *****************************************************************
       */
      if (psProperties->iLastAnalysisStage == 1 && psProperties->asAnalysisStages[0].piRoutine == AnalyzeDoXMagic)
      {
        iBlockTag |= ANALYZE_FINAL_BLOCK;
      }
#endif
    }
  }
  fclose(pFile);
#ifdef WINNT
  CloseHandle(hFile);
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
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
  {
    strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "Md5Digest");
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDigest;
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoMd5Digest;
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
  {
    strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "Sha1Digest");
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDigest;
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoSha1Digest;
  }
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
  {
    strcpy(psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].acDescription, "Sha256Digest");
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage].iError = ER_DoDigest;
    psProperties->asAnalysisStages[psProperties->iLastAnalysisStage++].piRoutine = AnalyzeDoSha256Digest;
  }
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoMd5Digest
 *
 ***********************************************************************
 */
int
AnalyzeDoMd5Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  static MD5_CONTEXT sFileMD5Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    MD5Alpha(&sFileMD5Context);
  }

  MD5Cycle(&sFileMD5Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    MD5Omega(&sFileMD5Context, psFTFileData->aucFileMd5);
    psFTFileData->ulAttributeMask |= MAP_MD5;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoSha1Digest
 *
 ***********************************************************************
 */
int
AnalyzeDoSha1Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  static SHA1_CONTEXT sFileSha1Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    SHA1Alpha(&sFileSha1Context);
  }

  SHA1Cycle(&sFileSha1Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    SHA1Omega(&sFileSha1Context, psFTFileData->aucFileSha1);
    psFTFileData->ulAttributeMask |= MAP_SHA1;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * AnalyzeDoSha256Digest
 *
 ***********************************************************************
 */
int
AnalyzeDoSha256Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  static SHA256_CONTEXT sFileSha256Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    SHA256Alpha(&sFileSha256Context);
  }

  SHA256Cycle(&sFileSha256Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    SHA256Omega(&sFileSha256Context, psFTFileData->aucFileSha256);
    psFTFileData->ulAttributeMask |= MAP_SHA256;
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
AnalyzeDoDig(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoDig()";
  char                acLocalError[MESSAGE_SIZE] = "";
  unsigned char      *pucToSearch;
#ifdef USE_PCRE
  int                 iBlockSize = AnalyzeGetBlockSize();
#endif
  int                 iCarrySize = AnalyzeGetCarrySize();
  int                 iError;
  int                 iNToSearch;
  int                 iStopShort;
  int                 iType;
  unsigned char      *pucSaveBuffer = NULL;
  static int          iNToSave;
  static int          iSaveOffset;
  static APP_UI64     ui64SearchOffset;

  /*-
   *********************************************************************
   *
   * Get a pointer to the save buffer.
   *
   *********************************************************************
   */
  pucSaveBuffer = AnalyzeGetDigSaveBuffer(iCarrySize, acLocalError);
  if (pucSaveBuffer == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * If this is the final block, clear the stop short flag. Otherwise,
   * set the flag and make sure that we have a nonzero length.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    iStopShort = 0;
  }
  else
  {
    if (iBufferLength == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: A zero length block is illegal unless it is tagged as the final block.", acRoutine);
      return ER;
    }
    iStopShort = 1;
  }

  /*-
   *********************************************************************
   *
   * If this is the first block, initialize. Otherwise, prepend any
   * saved data to the incoming buffer.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    DigClearCounts();

    /*-
     *******************************************************************
     *
     * Make sure that the max string length is less than or equal to
     * the size of the save buffer. The dig algorithm stops searching
     * after x bytes have been processed. Where
     *
     *   x = (bufsize - (maxstring - 1))
     *
     * For example, if bufsize = 16 and maxstring = 6, then all but
     * the last 5 bytes may be searched (i.e., to stay in bounds). Thus,
     * the search routine needs to stop searching when it reaches byte
     * 11.
     *
     * If no search strings were defined, maxstring should be zero.
     *
     * There is no case where iNToSave should be set to a negative
     * value.
     *
     *******************************************************************
     */
    if (DigGetMaxStringLength() <= iCarrySize)
    {
      iNToSave = iCarrySize;
      DigSetSaveLength(iCarrySize);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Not enough overhead in the save buffer. The carry size (%d) must be no less than %d bytes for this job to work as intended.", acRoutine, iCarrySize, DigGetMaxStringLength());
      return ER;
    }

    ui64SearchOffset = 0;
    pucToSearch = pucBuffer;
    iNToSearch = iBufferLength;
  }
  else
  {
    pucToSearch = pucBuffer - iNToSave;
    memcpy(pucToSearch, pucSaveBuffer, iNToSave);
    iNToSearch = iBufferLength + iNToSave;
  }

  /*-
   *********************************************************************
   *
   * Search the input.
   *
   *********************************************************************
   */
  for (iType = DIG_STRING_TYPE_NORMAL; iType < DIG_STRING_TYPE_NOMORE; iType++)
  {
    iError = DigSearchData(pucToSearch, iNToSearch, iStopShort, iType, ui64SearchOffset, psFTFileData->pcNeuteredPath, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }
  }

  /*-
   *********************************************************************
   *
   * Save any data that could not be fully searched.
   *
   *********************************************************************
   */
  iSaveOffset = iNToSearch - iNToSave;
  memcpy(pucSaveBuffer, &pucToSearch[iSaveOffset], iNToSave);

  /*-
   *********************************************************************
   *
   * Update the search offset. This ensures that search routine is
   * passed the offset it expects. In other words, subtract off the
   * unsearched portion of the prior search buffer, and keep it for
   * next time.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    ui64SearchOffset += iBufferLength - iNToSave;
  }
  else
  {
    ui64SearchOffset += iBufferLength;
  }

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * Update dig offsets. The trim size is equal to the block size in
   * all but the first block.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    DigAdjustRegExpOffsets(iBlockSize - iCarrySize);
  }
  else
  {
    DigAdjustRegExpOffsets(iBlockSize);
  }
#endif

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
  char                acLocalError[MESSAGE_SIZE] = "";
  unsigned char       aucMD5[MD5_HASH_SIZE];
  int                 i;
  int                 iError;
  FILE               *pFile;
  APP_UI64            ui64Size;

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
      psProperties->psXMagic = XMagicLoadMagic(psProperties->acMagicFileName, acLocalError);
      if (psProperties->psXMagic == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return ER_XMagic;
      }
      else
      {
        if ((pFile = fopen(psProperties->acMagicFileName, "rb")) != NULL && MD5HashStream(pFile, aucMD5, &ui64Size) == ER_OK)
        {
          MD5HashToHex(aucMD5, psProperties->acMagicHash);
          fclose(pFile);
        }
        else
        {
          strcpy(psProperties->acMagicHash, "NONE");
        }
        break; /* Very important. This get's us out of the for loop. */
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
AnalyzeDoXMagic(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoXMagic()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acMessage[MESSAGE_SIZE] = "";
  int                 iError = 0;
  int                 iMemoryMapped = 0;
  int                 iMemoryMapSize = 0;
  void               *pvMemoryMap = NULL;
  static int          iFirst = 1;
  static int          iMemoryMapEnable = 0;
  static XMAGIC      *psXMagic = NULL;

  /*-
   *********************************************************************
   *
   * Obtain a reference to the XMagic, and set other static variables
   * on the first pass.
   *
   *********************************************************************
   */
  if (iFirst)
  {
    FTIMES_PROPERTIES *psProperties = FTimesGetPropertiesReference();
    psXMagic = psProperties->psXMagic;
    iMemoryMapEnable = psProperties->iMemoryMapEnable;
    iFirst = 0;
  }

  /*-
   *********************************************************************
   *
   * XMagic tests are only done once -- either on the first block or
   * on a memory mapped view of the file.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) != ANALYZE_FIRST_BLOCK)
  {
    return ER_OK;
  }

  /*-
   *********************************************************************
   *
   * Conditionally map the file into memory. If the file's size is
   * zero, do not attempt to map it as this will lead to EINVAL errors
   * on some platforms -- either for mmap() or munmap(). If the memory
   * can't be mapped for whatever reason, fallback to using the block
   * of data provided by the caller.
   *
   *********************************************************************
   */
  if (iMemoryMapEnable)
  {
#ifdef WINNT
    if (psFTFileData->dwFileSizeHigh == 0 && psFTFileData->dwFileSizeLow < FTIMES_MAX_MMAP_SIZE)
    {
      iMemoryMapSize = psFTFileData->dwFileSizeLow;
    }
#else
    if (psFTFileData->sStatEntry.st_size < FTIMES_MAX_MMAP_SIZE)
    {
      iMemoryMapSize = psFTFileData->sStatEntry.st_size;
    }
#endif
    else
    {
      iMemoryMapSize = FTIMES_MAX_MMAP_SIZE;
    }
    if (iMemoryMapSize > 0 && (pvMemoryMap = AnalyzeMapMemory(iMemoryMapSize)) != NULL)
    {
      pucBuffer = (unsigned char *) pvMemoryMap;
      iBufferLength = iMemoryMapSize;
      iMemoryMapped = 1;
    }
  }
  snprintf(acMessage, MESSAGE_SIZE, "AnalysisStage=XMagic MemoryMapped=%d BufferLength=%d", iMemoryMapped, iBufferLength);
  MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_DEBUGGER, MESSAGE_DEBUGGER_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * Execute XMagic tests.
   *
   *********************************************************************
   */
  iError = XMagicTestBuffer(psXMagic, pucBuffer, iBufferLength, psFTFileData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
  if (iError == ER)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    if (iMemoryMapped)
    {
      AnalyzeUnmapMemory(pvMemoryMap, iMemoryMapSize);
    }
    return ER_XMagic;
  }
  psFTFileData->ulAttributeMask |= MAP_MAGIC;
  if (iMemoryMapped)
  {
    AnalyzeUnmapMemory(pvMemoryMap, iMemoryMapSize);
  }

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * AnalyzeMapMemory
 *
 ***********************************************************************
 */
void *
AnalyzeMapMemory(int iMemoryMapSize)
{
#ifdef WINNT
  HANDLE              hMemoryMap = NULL;
#endif
  void               *pvMemoryMap = NULL;

  /*-
   *********************************************************************
   *
   * This routine does not detect or return errors because its caller
   * (AnalyzeDoXMagic) is designed to use an alternate data buffer if
   * the file can't be mapped into memory.
   *
   *********************************************************************
   */
#ifdef WINNT
  hMemoryMap = CreateFileMapping(ghFile, NULL, PAGE_READONLY, 0, 0, 0);
  if (hMemoryMap)
  {
    pvMemoryMap = MapViewOfFile(hMemoryMap, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(hMemoryMap);
  }
#else
  pvMemoryMap = mmap(NULL, iMemoryMapSize, PROT_READ, MAP_PRIVATE, giFile, 0);
#if defined(FTimes_HPUX) && !defined(MAP_FAILED)
#define MAP_FAILED ((void *)-1)
#endif
  if (pvMemoryMap == MAP_FAILED)
  {
    return NULL;
  }
#endif

  return pvMemoryMap;
}


/*-
 ***********************************************************************
 *
 * AnalyzeSetBlockSize
 *
 ***********************************************************************
 */
void
AnalyzeSetBlockSize(int iBlockSize)
{
  giAnalyzeBlockSize = iBlockSize;
}


/*-
 ***********************************************************************
 *
 * AnalyzeSetCarrySize
 *
 ***********************************************************************
 */
void
AnalyzeSetCarrySize(int iCarrySize)
{
  giAnalyzeCarrySize = iCarrySize;
}


#ifdef USE_XMAGIC
/*-
 ***********************************************************************
 *
 * AnalyzeSetStepSize
 *
 ***********************************************************************
 */
void
AnalyzeSetStepSize(int iStepSize)
{
  giAnalyzeStepSize = iStepSize;
}
#endif


/*-
 ***********************************************************************
 *
 * AnalyzeThrottleDps
 *
 ***********************************************************************
 */
void
AnalyzeThrottleDps(APP_UI64 ui64Bytes, int iMaxDps)
{
  static double       dNow = 0;
  static double       dStartTime = 0;
  static int          iFirst = 1;
  double              dKBytes = 0;
  double              dSleepTime = 0;
  int                 iSleepTime = 0;
#ifdef WINNT
  APP_SI64            i64Bytes = (APP_SI64) ui64Bytes;
#endif

  /*-
   *********************************************************************
   *
   * Get the current time. Record the start time on the first pass.
   *
   *********************************************************************
   */
  dNow = TimeGetTimeValueAsDouble();
  if (iFirst)
  {
    dStartTime = dNow;
    iFirst = 0;
  }

  /*-
   *********************************************************************
   *
   * Calculate the amount of time spent thus far. Force the result to
   * be at least one microsecond to thwart divide by zero issues.
   *
   *********************************************************************
   */
  gdAnalysisTime = (double) (dNow - dStartTime);
  if (gdAnalysisTime <= 0)
  {
    gdAnalysisTime = 0.000001;
  }

  /*-
   *********************************************************************
   *
   * Calculate the Data Processing Speed (DPS). If the maximum DPS is
   * zero, do not apply the brakes. Otherwise, set the sleep time to a
   * value in the range [1,60], and sleep. If the DPS value has a
   * fractional component (of any amount), round it up.
   *
   *********************************************************************
   */
#ifdef WINNT
  dKBytes = (double) ((double)  i64Bytes / (double) 1024);
#else
  dKBytes = (double) ((double) ui64Bytes / (double) 1024);
#endif
  gdDps = dKBytes / gdAnalysisTime;
  if (iMaxDps && gdDps > (double) iMaxDps)
  {
    dSleepTime = (double) ((dKBytes / (double) iMaxDps) - gdAnalysisTime);
    if (dSleepTime < 1)
    {
      iSleepTime = 1;
    }
    else if (dSleepTime > 60)
    {
      iSleepTime = 60;
    }
    else
    {
      iSleepTime = (int) dSleepTime;
      if ((double) (dSleepTime / (double) iSleepTime))
      {
        iSleepTime += 1;
      }
    }
#ifdef WINNT
    Sleep(iSleepTime * 1000);
#else
    sleep(iSleepTime);
#endif
  }
}


/*-
 ***********************************************************************
 *
 * AnalyzeUnmapMemory
 *
 ***********************************************************************
 */
void
AnalyzeUnmapMemory(void *pvMemoryMap, int iMemoryMapSize)
{
#ifdef WINNT
  UnmapViewOfFile(pvMemoryMap);
#else
  munmap(pvMemoryMap, iMemoryMapSize);
#endif
  return;
}
