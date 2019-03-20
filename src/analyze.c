/*-
 ***********************************************************************
 *
 * $Id: analyze.c,v 1.33 2007/02/23 00:22:35 mavrik Exp $
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
 * Defines
 *
 ***********************************************************************
 */
#define ANALYZE_READ_BUFSIZE 0x4000
#define ANALYZE_BLOCK_MULTIPLIER  3
#define ANALYZE_BLOCK_SIZE   0x4000
#define ANALYZE_CARRY_SIZE   0x0400
#define ANALYZE_FIRST_BLOCK       1
#define ANALYZE_FINAL_BLOCK       2

static K_UINT32       gui32Files;
static K_UINT64       gui64Bytes;
static int            giAnalyzeBlockSize = ANALYZE_BLOCK_SIZE;
static int            giAnalyzeCarrySize = ANALYZE_CARRY_SIZE;
#ifdef USE_XMAGIC
static int            giAnalyzeStepSize = ANALYZE_BLOCK_SIZE;
#endif


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
K_UINT64
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
 * AnalyzeGetFileCount
 *
 ***********************************************************************
 */
K_UINT32
AnalyzeGetFileCount(void)
{
  return gui32Files;
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
AnalyzeFile(FTIMES_PROPERTIES *psProperties, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeFile()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 i;
  int                 iBlockSize = AnalyzeGetBlockSize();
  int                 iBlockTag;
  int                 iError;
  int                 iNRead;
  unsigned char      *pucBuffer = NULL;
#ifdef WINNT
  char               *pcMessage;
  HANDLE              hFile;
  int                 iFile;
#endif
  FILE               *pFile;
  K_UINT64            ui64FileSize;

  /*-
   *********************************************************************
   *
   * If the size of the specified file is greater than FileSizeLimit,
   * set its hash to a predefined value and return.
   *
   *********************************************************************
   */
#ifdef WINNT
  ui64FileSize = (((K_UINT64) psFTData->dwFileSizeHigh) << 32) | psFTData->dwFileSizeLow;
#else
  ui64FileSize = (K_UINT64) psFTData->sStatEntry.st_size;
#endif
  if (psProperties->ulFileSizeLimit != 0 && ui64FileSize > (K_UINT64) psProperties->ulFileSizeLimit)
  {
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
    {
      memset(psFTData->aucFileMd5, 0xff, MD5_HASH_SIZE);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
    {
      memset(psFTData->aucFileSha1, 0xff, SHA1_HASH_SIZE);
    }
    if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
    {
      memset(psFTData->aucFileSha256, 0xff, SHA256_HASH_SIZE);
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
   *********************************************************************
   */
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
    return ER;
  }
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
  pFile = fopen(psFTData->pcRawPath, "rb");
  if (pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): %s", acRoutine, strerror(errno));
    return ER;
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
   * Initialize the block tag. Each analysis stage executes different
   * logic based on the value of this tag.
   *
   *********************************************************************
   */
  iBlockTag = ANALYZE_FIRST_BLOCK;

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
     * If EOF was reached, update the block tag.
     *
     *******************************************************************
     */
    if (feof(pFile))
    {
      iBlockTag |= ANALYZE_FINAL_BLOCK;
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
      iError = psProperties->asAnalysisStages[i].piRoutine(&pucBuffer[iBlockSize], iNRead, iBlockTag, iBlockSize, psFTData, acLocalError);
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
AnalyzeDoMd5Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  static MD5_CONTEXT sFileMD5Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    MD5Alpha(&sFileMD5Context);
  }

  MD5Cycle(&sFileMD5Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    MD5Omega(&sFileMD5Context, psFTData->aucFileMd5);
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
AnalyzeDoSha1Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  static SHA1_CONTEXT sFileSha1Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    SHA1Alpha(&sFileSha1Context);
  }

  SHA1Cycle(&sFileSha1Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    SHA1Omega(&sFileSha1Context, psFTData->aucFileSha1);
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
AnalyzeDoSha256Digest(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  static SHA256_CONTEXT sFileSha256Context;

  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    SHA256Alpha(&sFileSha256Context);
  }

  SHA256Cycle(&sFileSha256Context, pucBuffer, iBufferLength);

  if ((iBlockTag & ANALYZE_FINAL_BLOCK) == ANALYZE_FINAL_BLOCK)
  {
    SHA256Omega(&sFileSha256Context, psFTData->aucFileSha256);
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
AnalyzeDoDig(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoDig()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
  static K_UINT64     ui64AbsoluteOffset;

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

    ui64AbsoluteOffset = 0;
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
    iError = DigSearchData(pucToSearch, iNToSearch, iStopShort, iType, ui64AbsoluteOffset, psFTData->pcNeuteredPath, acLocalError);
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
   * Update the absolute file offset. This ensures that searcher is
   * passed the offset it expects. In other words, subtract off the
   * unsearched portion of the prior search buffer, and keep it for
   * next time.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    ui64AbsoluteOffset += iBufferLength - iNToSave;
  }
  else
  {
    ui64AbsoluteOffset += iBufferLength;
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  unsigned char       aucMD5[MD5_HASH_SIZE];
  int                 i;
  int                 iError;
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
      psProperties->psXMagic = XMagicLoadMagic(psProperties->acMagicFileName, acLocalError);
      if (psProperties->psXMagic == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return iError;
      }
      else
      {
        if ((pFile = fopen(psProperties->acMagicFileName, "rb")) != NULL && MD5HashStream(pFile, aucMD5) == ER_OK)
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
AnalyzeDoXMagic(unsigned char *pucBuffer, int iBufferLength, int iBlockTag, int iBufferOverhead, FTIMES_FILE_DATA *psFTData, char *pcError)
{
  const char          acRoutine[] = "AnalyzeDoXMagic()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  static int          iFirst = 1;
  static XMAGIC      *psXMagic = NULL;

  /*-
   *********************************************************************
   *
   * Obtain a reference to the XMagic.
   *
   *********************************************************************
   */
  if (iFirst)
  {
    FTIMES_PROPERTIES *psProperties = FTimesGetPropertiesReference();
    psXMagic = psProperties->psXMagic;
    iFirst = 0;
  }

  /*-
   *********************************************************************
   *
   * Magic only inspects the first block of data.
   *
   *********************************************************************
   */
  if ((iBlockTag & ANALYZE_FIRST_BLOCK) == ANALYZE_FIRST_BLOCK)
  {
    iError = XMagicTestBuffer(psXMagic, pucBuffer, iBufferLength, psFTData->acType, FTIMES_FILETYPE_BUFSIZE, acLocalError);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_XMagic;
    }
  }

  return ER_OK;
}
#endif


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
