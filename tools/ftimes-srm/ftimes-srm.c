/*-
 ***********************************************************************
 *
 * $Id: ftimes-srm.c,v 1.12 2019/07/23 20:27:02 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2017-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Global Variables.
 *
 ***********************************************************************
 */
FTIMES_SRM_PROPERTIES *gpsProperties = NULL;

/*-
 ***********************************************************************
 *
 * ErrorHandler
 *
 ***********************************************************************
 */
void
ErrorHandler(int iError, char *pcError, int iSeverity)
{
#define MAX_SEVERITY_SIZE 9
  char                acSeverity[MAX_SEVERITY_SIZE] = {};
  FTIMES_SRM_PROPERTIES *psProperties = FTimesSrmGetPropertiesReference();

  switch (iSeverity)
  {
  case ERROR_WARNING:
    snprintf(acSeverity, MAX_SEVERITY_SIZE, "warning");
    break;
  case ERROR_FAILURE:
    snprintf(acSeverity, MAX_SEVERITY_SIZE, "failure");
    break;
  case ERROR_CRITICAL:
    snprintf(acSeverity, MAX_SEVERITY_SIZE, "critical");
    break;
  default:
    snprintf(acSeverity, MAX_SEVERITY_SIZE, "unknown");
    break;
  }
  fprintf(psProperties->pLogHandle, "%s: Error='%s'; Severity='%s';\n", PROGRAM_NAME, pcError, acSeverity);

  return;
}

/*-
 ***********************************************************************
 *
 * FTimesSrmMain
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcSnapshot = NULL;
  FTIMES_SRM_PROPERTIES *psProperties = NULL;
  int                 iError = 0;
  SNAPSHOT_CONTEXT   *psSnapshotContext = NULL;

  /*-
   *********************************************************************
   *
   * Punch in and go to work.
   *
   *********************************************************************
   */
  iError = FTimesSrmBootStrap(acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Unable to bootstrap program (%s).'\n", PROGRAM_NAME, acLocalError);
    return XER_BootStrap;
  }
  psProperties = FTimesSrmGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Process command line arguments.
   *
   *********************************************************************
   */
  iError = FTimesSrmProcessArguments(iArgumentCount, ppcArgumentVector, psProperties, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stderr, "%s: Error='Unable to process arguments (%s).'\n", PROGRAM_NAME, acLocalError);
    return XER_ProcessArguments;
  }

  /*-
   *********************************************************************
   *
   * Write out a header.
   *
   *********************************************************************
   */
  iError = MapWriteHeader(psProperties->psFieldMask, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(psProperties->pLogHandle, "%s: Error='%s'\n", PROGRAM_NAME, acLocalError);
    return XER_Abort;
  }

  /*-
   *********************************************************************
   *
   * Process any files listed on the command line.
   *
   *********************************************************************
   */
  while ((pcSnapshot = OptionsGetNextOperand(psProperties->psOptionsContext)) != NULL)
  {
    psSnapshotContext = DecodeNewSnapshotContext2(pcSnapshot, acLocalError);
    if (psSnapshotContext == NULL)
    {
      fprintf(psProperties->pLogHandle, "%s: Error='Unable to initialize snapshot context (%s).'\n", PROGRAM_NAME, acLocalError);
      continue;
    }
    while (DecodeReadLine(psSnapshotContext, acLocalError) != NULL)
    {
      psSnapshotContext->sDecodeStats.ulAnalyzed++;
      iError = DecodeParseRecord(psSnapshotContext, acLocalError);
      if (iError != ER_OK)
      {
        if (psSnapshotContext->iCompressed)
        {
          psSnapshotContext->iSkipToNext = TRUE; /* Skip all records up to the next checkpoint. */
        }
        psSnapshotContext->sDecodeStats.ulSkipped++;
        continue;
      }
      psSnapshotContext->sDecodeStats.ulDecoded++;
      iError = FTimesSrmRemoveFile(psSnapshotContext, psProperties->psFieldMask, psProperties->iDryRun, acLocalError);
      if (iError != ER_OK)
      {
        fprintf(psProperties->pLogHandle, "%s: Error='%s'; Snapshot='%s'; Line='%d';\n", PROGRAM_NAME, acLocalError, psSnapshotContext->pcFile, psSnapshotContext->iLineNumber);
      }
    }
    if (ferror(psSnapshotContext->pFile))
    {
      fprintf(psProperties->pLogHandle, "%s: Error='%s'; Snapshot='%s'; Line='%d';\n", PROGRAM_NAME, acLocalError, psSnapshotContext->pcFile, psSnapshotContext->iLineNumber);
      psSnapshotContext->sDecodeStats.ulSkipped++;
    }
    DecodeFreeSnapshotContext2(psSnapshotContext);
  }

  /*-
   *********************************************************************
   *
   * Clean up and go home.
   *
   *********************************************************************
   */
  FTimesSrmFreeProperties(psProperties);

  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmAbort
 *
 ***********************************************************************
 */
void
FTimesSrmAbort()
{
  exit(XER_Abort);
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesSrmFormatWinxError
 *
 ***********************************************************************
 */
void
FTimesSrmFormatWinxError(DWORD dwError, TCHAR **pptcMessage)
{
  static TCHAR        atcMessage[MESSAGE_SIZE / 2];
  int                 i = 0;
  int                 j = 0;
  LPVOID              lpvMessage = NULL;
  TCHAR              *ptc = NULL;

  /*-
   *********************************************************************
   *
   * Initialize the result buffer/pointer.
   *
   *********************************************************************
   */
  atcMessage[0] = 0;

  *pptcMessage = atcMessage;

  /*-
   *********************************************************************
   *
   * Format the message that corresponds to the specified error code.
   *
   *********************************************************************
   */
  FormatMessage(
    FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    NULL,
    dwError,
    MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* default language */
    (LPTSTR) &lpvMessage,
    0,
    NULL
    );

  /*-
   *********************************************************************
   *
   * Replace linefeeds with spaces. Eliminate carriage returns.
   *
   *********************************************************************
   */
  for (i = 0, j = 0, ptc = (TCHAR *) lpvMessage; (i < (int) _tcslen(lpvMessage)) && (i < (MESSAGE_SIZE / 2)); i++, j++)
  {
    if (ptc[i] == _T('\n'))
    {
      atcMessage[j] = _T(' ');
    }
    else if (ptc[i] == _T('\r'))
    {
      j--;
      continue;
    }
    else
    {
      atcMessage[j] = ptc[i];
    }
  }
  atcMessage[j] = 0;

  /*-
   *********************************************************************
   *
   * Removing trailing spaces.
   *
   *********************************************************************
   */
  while (j > 1 && atcMessage[j - 1] == _T(' '))
  {
    atcMessage[--j] = 0;
  }

  LocalFree(lpvMessage);
}
#endif


/*-
 ***********************************************************************
 *
 * FTimesSrmFreeHandle
 *
 ***********************************************************************
 */
void
FTimesSrmFreeHandle(FTIMES_SRM_HANDLE *psHandle)
{
  if (psHandle)
  {
//FIXME What is the right way on WINX platforms to close a file handle
//      that trasitioned from hFile via CreateFileW() to iFile via
//      _open_osfhandle() to pFile via _fdopen()? When compiled using
//      Visual Studio 2005, the _close() was causing assertion
//      exceptions -- probably because the fclose() closed the
//      underlying file descriptor along with the file pointer, so the
//      _close() was attempting to close something that had already
//      been closed.
    if (psHandle->pFile)
    {
      fclose(psHandle->pFile);
    }
#ifdef WINNT
//FIXME See comment above.
//  if (psHandle->iFile != -1)
//  {
//    _close(psHandle->iFile);
//  }
    if (psHandle->pcFileA)
    {
      free(psHandle->pcFileA);
    }
    if (psHandle->pcFileW)
    {
      free(psHandle->pcFileW);
    }
#endif
    free(psHandle);
  }
}


/*-
 ***********************************************************************
 *
 * FTimesSrmGetHandle
 *
 ***********************************************************************
 */
FTIMES_SRM_HANDLE *
FTimesSrmGetHandle(char *pcDecodedName, char *pcError)
{
#ifdef WINNT
  char                acLocalError[MESSAGE_SIZE] = "";
#endif
  FTIMES_SRM_HANDLE  *psHandle = NULL;
#ifdef WINNT
  int                 iSize = 0;
  TCHAR              *ptcWinxError = NULL;
#endif

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new handle structure.
   *
   *********************************************************************
   */
  psHandle = (FTIMES_SRM_HANDLE *)calloc(sizeof(FTIMES_SRM_HANDLE), 1);
  if (psHandle == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return NULL;
  }
  psHandle->pcDecodedName = pcDecodedName;

  /*-
   *********************************************************************
   *
   * Open the specified file for binary read access. If the filename is
   * "-", use stdin.
   *
   *********************************************************************
   */
  if (strcmp(psHandle->pcDecodedName, "-") == 0)
  {
    psHandle->pFile = stdin;
    return psHandle;
  }

#ifdef WINNT
  iSize = strlen(psHandle->pcDecodedName) + strlen(FTIMES_SRM_EXTENDED_PATH_PREFIX) + 1;
  psHandle->pcFileA = (char *)calloc(iSize, 1);
  if (psHandle->pcFileA == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    goto ABORT;
  }
  snprintf(psHandle->pcFileA, iSize, "%s%s",
    (isalpha((int) psHandle->pcDecodedName[0]) && psHandle->pcDecodedName[1] == ':') ? FTIMES_SRM_EXTENDED_PATH_PREFIX : "",
    psHandle->pcDecodedName
    );
  psHandle->pcFileW = FTimesSrmUtf8ToWide(psHandle->pcFileA, iSize, acLocalError);
  if (psHandle->pcFileW == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
    goto ABORT;
  }
  psHandle->hFile = CreateFileW
  (
    psHandle->pcFileW,
    GENERIC_READ,
    FILE_SHARE_READ,
    NULL,
    OPEN_EXISTING,
    FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_SEQUENTIAL_SCAN,
    NULL
  );
  if (psHandle->hFile == INVALID_HANDLE_VALUE)
  {
    FTimesSrmFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s", ptcWinxError);
    goto ABORT;
  }
  psHandle->iFile = _open_osfhandle((intptr_t) psHandle->hFile, 0);
  if (psHandle->iFile == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    goto ABORT;
  }
  psHandle->pFile = _fdopen(psHandle->iFile, "rb");
  if (psHandle->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    goto ABORT;
  }
#else
  psHandle->pFile = fopen(psHandle->pcDecodedName, "rb");
  if (psHandle->pFile == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    goto ABORT;
  }
#endif

  return psHandle;
ABORT:
  FTimesSrmFreeHandle(psHandle);
  return NULL;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmUsage
 *
 ***********************************************************************
 */
void
FTimesSrmUsage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: %s [OPTION [...]] -- {-|snapshot} [snapshot [...]]\n", PROGRAM_NAME);
  fprintf(stderr, "  or   %s {-v|--version}\n", PROGRAM_NAME);
  fprintf(stderr, "\n");
  fprintf(stderr, "Where options can be any of the following:\n\
\n\
       {-1|--log-to-stdout}\n\
                       Log errors to stdout rather than stderr, which is the\n\
                       default.\n\
       {-m|--mask} {all|none}[{+|-}<field>[...]]\n\
                       The field mask specifies the attributes to be used in\n\
                       determining whether or not a given file is removed. If\n\
                       the current set of attributes matches those specified\n\
                       in a given snapshot, the file is slated for removal.\n\
                       Otherwise, the file's removal is abandoned. A mask of\n\
                       'none' causes files to be removed based on their name\n\
                       alone.\n\
       {-n|--dry-run}\n\
                       Perform a dry run. Don't actually remove any files.\n\
                       Rather, generate output indicating what action(s) would\n\
                       be taken barring any technical issues.\n"
  );
  fprintf(stderr, "\n");
  exit(XER_Usage);
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * FTimesSrmUtf8ToWide
 *
 ***********************************************************************
 */
wchar_t *
FTimesSrmUtf8ToWide(char *pcString, int iUtf8Size, char *pcError)
{
  int                 iWideSize = 0;
  TCHAR              *ptcWinxError = NULL;
  wchar_t            *pwcString = NULL;

  iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, NULL, 0); /* The byte count returned includes the NULL terminator. */
  if (iWideSize)
  {
    pwcString = malloc(iWideSize * sizeof(wchar_t));
    if (pwcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
      return NULL;
    }
    iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, pwcString, iWideSize);
    if (!iWideSize)
    {
      FTimesSrmFormatWinxError(GetLastError(), &ptcWinxError);
      snprintf(pcError, MESSAGE_SIZE, "%s", ptcWinxError);
      free(pwcString);
      return NULL;
    }
  }
  else
  {
    FTimesSrmFormatWinxError(GetLastError(), &ptcWinxError);
    snprintf(pcError, MESSAGE_SIZE, "%s", ptcWinxError);
    return NULL;
  }

  return pwcString;
}
#endif


/*-
 ***********************************************************************
 *
 * FTimesSrmVersion
 *
 ***********************************************************************
 */
void
FTimesSrmVersion(void)
{
  fprintf(stdout, "%s %s %d-bit\n", PROGRAM_NAME, VERSION, (int) (sizeof(&FTimesSrmVersion) * 8));
  exit(XER_OK);
}


/*-
 ***********************************************************************
 *
 * FTimesRemoveFile
 *
 ***********************************************************************
 */
int
FTimesSrmRemoveFile(SNAPSHOT_CONTEXT *psSnapshotContext, MASK_USS_MASK *psFieldMask, int iDryRun, char *pcError)
{
  APP_UI64            ui64TargetSize = 0;
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcDecodedName = NULL;
  char               *pcEncodedName = NULL;
  char               *pcTargetMd5 = NULL;
  char               *pcTargetSha1 = NULL;
  char               *pcTargetSha256 = NULL;
  char               *pcTargetSize = NULL;
  FTIMES_FILE_DATA   *psFTFileData = NULL;
  FTIMES_SRM_HANDLE  *psHandle = NULL;
  MASK_B2S_TABLE     *pasMaskTable = MaskGetTableReference(MASK_MASK_TYPE_CMP);
  unsigned char       aucData[FTIMES_SRM_DEFAULT_BLOCKSIZE] = {};
  unsigned char       aucTargetMd5[MD5_HASH_SIZE] = {};
  unsigned char       aucTargetSha1[SHA1_HASH_SIZE] = {};
  unsigned char       aucTargetSha256[SHA256_HASH_SIZE] = {};
  int                 i = 0;
  int                 iEncodedLength = 0;
  int                 iError = 0;
  int                 iNRead = 0;
  int                 iStatus = ER;
#ifdef WINNT
  wchar_t            *pwcDecodedName = NULL;
#endif

  /*-
   *********************************************************************
   *
   * Assign target values. Those that aren't present will be NULL.
   *
   *********************************************************************
   */
  for (i = 0; i < MaskGetTableLength(MASK_MASK_TYPE_CMP); i++)
  {
    if (MASK_BIT_IS_SET(psSnapshotContext->ulFieldMask, 1 << i))
    {
           if (strcmp(pasMaskTable[i].acName,   "name") == 0) {   pcEncodedName = psSnapshotContext->psCurrRecord->ppcFields[i]; }
      else if (strcmp(pasMaskTable[i].acName,   "size") == 0) {    pcTargetSize = psSnapshotContext->psCurrRecord->ppcFields[i]; }
      else if (strcmp(pasMaskTable[i].acName,    "md5") == 0) {     pcTargetMd5 = psSnapshotContext->psCurrRecord->ppcFields[i]; }
      else if (strcmp(pasMaskTable[i].acName,   "sha1") == 0) {    pcTargetSha1 = psSnapshotContext->psCurrRecord->ppcFields[i]; }
      else if (strcmp(pasMaskTable[i].acName, "sha256") == 0) {  pcTargetSha256 = psSnapshotContext->psCurrRecord->ppcFields[i]; }
    }
  }

  /*-
   *********************************************************************
   *
   * Inspect and conditionally decode the encoded filename.
   *
   *********************************************************************
   */
  if (pcEncodedName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "The name field is NULL. That should not happen!");
    return ER;
  }
  iEncodedLength = strlen(pcEncodedName);
  if (iEncodedLength < 1)
  {
    snprintf(pcError, MESSAGE_SIZE, "The name field is empty.");
    return ER;
  }
  if (!(iEncodedLength > 2 && pcEncodedName[0] == '"' && pcEncodedName[iEncodedLength - 1] == '"'))
  {
    snprintf(pcError, MESSAGE_SIZE, "The name field is not formatted properly.");
    return ER;
  }
  pcEncodedName[iEncodedLength - 1] = 0;
  iEncodedLength -= 2;
  pcDecodedName = FTimesSrmDecodeString(&pcEncodedName[1], acLocalError);
  if (pcDecodedName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "The name field could not be decoded (%s).", acLocalError);
    return ER;
  }
#ifdef WINNT
  pwcDecodedName = MapUtf8ToWide(pcDecodedName, strlen(pcDecodedName) + 1, acLocalError);
  if (pwcDecodedName == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "The name field could not be decoded (%s).", acLocalError);
    return ER;
  }
#endif

  /*-
   *******************************************************************
   *
   * Set target attributes (i.e., the constraints).
   *
   *******************************************************************
   */
  if (MASK_BIT_IS_SET(psFieldMask->ulMask,   SRM_SIZE))
  {
    iError = SupportStringToUInt64(pcTargetSize, &ui64TargetSize, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "The size field could not be converted (%s).", acLocalError);
      goto ABORT;
    }
  }
  if (MASK_BIT_IS_SET(psFieldMask->ulMask,    SRM_MD5)) {       MD5HexToHash(pcTargetMd5,    aucTargetMd5); }
  if (MASK_BIT_IS_SET(psFieldMask->ulMask,   SRM_SHA1)) {     SHA1HexToHash(pcTargetSha1,   aucTargetSha1); }
  if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256)) { SHA256HexToHash(pcTargetSha256, aucTargetSha256); }

  /*-
   *******************************************************************
   *
   * Get actual attributes.
   *
   *******************************************************************
   */
#ifdef WINNT
  psFTFileData = MapNewFTFileDataW(pwcDecodedName, acLocalError);
#else
  psFTFileData = MapNewFTFileData(pcDecodedName, acLocalError);
#endif
  if (psFTFileData == NULL)
  {
    psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
    psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
    psFTFileData->iReason = FTIMES_SRM_REASON_FILE_FTDATA_FAILURE;
    snprintf(pcError, MESSAGE_SIZE, "File \"%s\" ftdata failure (%s).", pcDecodedName, acLocalError);
    goto ABORT;
  }
  iError = FTimesSrmGetAttributes(psFTFileData, acLocalError);
  if (iError != ER_OK)
  {
    psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
    psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
    psFTFileData->iReason = (psFTFileData->iExists == 0) ? FTIMES_SRM_REASON_FILE_DOES_NOT_EXIST : FTIMES_SRM_REASON_FILE_ATTRIB_FAILURE;
    if (psFTFileData->iExists == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "File \"%s\" does not exist.", pcDecodedName);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "File \"%s\" attrib failure (%s).", pcDecodedName, acLocalError);
    }
    goto ABORT;
  }
#ifdef WINNT
  if ((psFTFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
#else
  if (!S_ISREG(psFTFileData->sStatEntry.st_mode))
#endif
  {
    psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
    psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
    psFTFileData->iReason = FTIMES_SRM_REASON_FILE_IS_NOT_REGULAR;
    snprintf(pcError, MESSAGE_SIZE, "File \"%s\" is not regular.", pcDecodedName);
    goto ABORT;
  }

  /*-
   *********************************************************************
   *
   * Conditionally open the file and compute hashes.
   *
   *********************************************************************
   */
  if
  (
       MASK_BIT_IS_SET(psFieldMask->ulMask,    SRM_MD5)
    || MASK_BIT_IS_SET(psFieldMask->ulMask,   SRM_SHA1)
    || MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256)
  )
  {
    psHandle = FTimesSrmGetHandle(pcDecodedName, acLocalError);
    if (psHandle == NULL)
    {
      psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
      psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
      psFTFileData->iReason = FTIMES_SRM_REASON_FILE_ACCESS_FAILURE;
      snprintf(pcError, MESSAGE_SIZE, "File \"%s\" access failure (%s).", pcDecodedName, acLocalError);
      goto ABORT;
    }
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_MD5))
    {
      MD5Alpha(&psFTFileData->sFileMd5);
    }
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA1))
    {
      SHA1Alpha(&psFTFileData->sFileSha1);
    }
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256))
    {
      SHA256Alpha(&psFTFileData->sFileSha256);
    }

    while (!feof(psHandle->pFile))
    {
      iNRead = fread(aucData, 1, FTIMES_SRM_DEFAULT_BLOCKSIZE, psHandle->pFile);
      if (ferror(psHandle->pFile))
      {
        psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
        psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
        psFTFileData->iReason = FTIMES_SRM_REASON_FILE_IOREAD_FAILURE;
        snprintf(pcError, MESSAGE_SIZE, "File \"%s\" handle failure (%s).", pcDecodedName, strerror(errno));
        goto ABORT;
      }
      if (iNRead == 0)
      {
        break; /* We're done. */
      }
      if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_MD5))
      {
        MD5Cycle(&psFTFileData->sFileMd5, aucData, iNRead);
      }
      if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA1))
      {
        SHA1Cycle(&psFTFileData->sFileSha1, aucData, iNRead);
      }
      if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256))
      {
        SHA256Cycle(&psFTFileData->sFileSha256, aucData, iNRead);
      }
    }
    FTimesSrmFreeHandle(psHandle);
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_MD5))
    {
      MD5Omega(&psFTFileData->sFileMd5, psFTFileData->aucFileMd5);
      psFTFileData->ulAttributeMask |= MAP_MD5;
    }
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA1))
    {
      SHA1Omega(&psFTFileData->sFileSha1, psFTFileData->aucFileSha1);
      psFTFileData->ulAttributeMask |= MAP_SHA1;
    }
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256))
    {
      SHA256Omega(&psFTFileData->sFileSha256, psFTFileData->aucFileSha256);
      psFTFileData->ulAttributeMask |= MAP_SHA256;
    }
  }

  /*-
   *********************************************************************
   *
   * Compare target/actual attributes and conditionally remove file.
   *
   *********************************************************************
   */
  if
  (
       (!MASK_BIT_IS_SET(psFieldMask->ulMask,    SRM_MD5) || memcmp(   aucTargetMd5,    psFTFileData->aucFileMd5,    MD5_HASH_SIZE) == 0)
    && (!MASK_BIT_IS_SET(psFieldMask->ulMask,   SRM_SHA1) || memcmp(  aucTargetSha1,   psFTFileData->aucFileSha1,   SHA1_HASH_SIZE) == 0)
    && (!MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256) || memcmp(aucTargetSha256, psFTFileData->aucFileSha256, SHA256_HASH_SIZE) == 0)
    && (!MASK_BIT_IS_SET(psFieldMask->ulMask,   SRM_SIZE) || ui64TargetSize == psFTFileData->ui64FileSize)
  )
  {
    psFTFileData->iAction = FTIMES_SRM_ACTION_REMOVE;
    if (iDryRun)
    {
      psFTFileData->iStatus = FTIMES_SRM_STATUS_NOOP;
    }
    else
    {
      if (unlink(psFTFileData->pcRawPath) == ER_OK)
      {
        psFTFileData->iStatus = FTIMES_SRM_STATUS_PASS;
        psFTFileData->iReason = FTIMES_SRM_REASON_FILE_UNLINK_SUCCESS;
      }
      else
      {
        psFTFileData->iStatus = FTIMES_SRM_STATUS_FAIL;
        psFTFileData->iReason = FTIMES_SRM_REASON_FILE_UNLINK_FAILURE;
        snprintf(pcError, MESSAGE_SIZE, "File \"%s\" remove failure (%s).", pcDecodedName, strerror(errno));
        goto ABORT;
      }
    }
  }
  else
  {
    psFTFileData->iAction = FTIMES_SRM_ACTION_REJECT;
    psFTFileData->iStatus = (iDryRun) ? FTIMES_SRM_STATUS_NOOP : FTIMES_SRM_STATUS_SKIP;
    psFTFileData->iReason = FTIMES_SRM_REASON_CONSTRAINTS_NOT_MET;
  }
  iStatus = ER_OK;

ABORT:
  if (psFTFileData != NULL)
  {
    iError = MapWriteRecord(psFieldMask, psFTFileData, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "File \"%s\" record failure (%s).", pcDecodedName, acLocalError);
      iStatus = ER;
    }
    MapFreeFTFileData(psFTFileData);
  }

  if (pcDecodedName != NULL)
  {
    free(pcDecodedName);
  }

  return iStatus;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmProcessArguments
 *
 ***********************************************************************
 */
int
FTimesSrmProcessArguments(int iArgumentCount, char *ppcArgumentVector[], FTIMES_SRM_PROPERTIES *psProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = {};
  int                 iError = 0;
  OPTIONS_CONTEXT    *psOptionsContext = NULL;
  OPTIONS_TABLE       asOptions[] =
  {
    { OPT_DryRun,      "-n", "--dry-run",       0, 0, 0, 0, FTimesSrmOptionHandler },
    { OPT_FieldMask,   "-m", "--mask",          0, 0, 1, 0, FTimesSrmOptionHandler },
    { OPT_LogToStdout, "-1", "--log-to-stdout", 0, 0, 0, 0, FTimesSrmOptionHandler },
    { OPT_Version,     "-v", "--version",       0, 0, 0, 0, FTimesSrmOptionHandler },
  };

  /*-
   *********************************************************************
   *
   * Initialize the options context.
   *
   *********************************************************************
   */
  psProperties->psOptionsContext = psOptionsContext = OptionsNewOptionsContext(iArgumentCount, ppcArgumentVector, acLocalError);
  if (psOptionsContext == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Establish the run mode and/or allowed set of options.
   *
   *********************************************************************
   */
  OptionsSetOptions(psOptionsContext, asOptions, (sizeof(asOptions) / sizeof(asOptions[0])));

  /*-
   *********************************************************************
   *
   * Process options.
   *
   *********************************************************************
   */
  iError = OptionsProcessOptions2(psOptionsContext, (void *) psProperties, acLocalError);
  switch (iError)
  {
  case OPTIONS_ER:
    snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
    return ER;
    break;
  case OPTIONS_OK:
    break;
  case OPTIONS_USAGE:
  default:
    FTimesSrmUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * Handle mode-specific/special use cases here.
   *
   *********************************************************************
   */
  if (OptionsGetOperandCount(psOptionsContext) < 1)
  {
    FTimesSrmUsage();
  }

  /*-
   *********************************************************************
   *
   * If any required options are missing, it's an error.
   *
   *********************************************************************
   */
  if (OptionsHaveRequiredOptions(psOptionsContext) == 0)
  {
    FTimesSrmUsage();
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmBootStrap
 *
 ***********************************************************************
 */
int
FTimesSrmBootStrap(char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = {};
  FTIMES_SRM_PROPERTIES *psProperties;

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Suppress critical-error-handler message boxes.
   *
   *********************************************************************
   */
  SetErrorMode(SEM_FAILCRITICALERRORS);

  /*-
   *********************************************************************
   *
   * Attempt to elevate privileges. If this fails, it's not fatal.
   *
   *********************************************************************
   */
  if (SupportSetPrivileges(acLocalError) != ER_OK)
  {
//FIXME Perhaps this should be fatal under certain conditions.
//  snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
//  return ER;
  }

  /*-
   *********************************************************************
   *
   * Put stdout in binary mode.
   *
   *********************************************************************
   */
  if (_setmode(_fileno(stdout), _O_BINARY) == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "Unable to put stdout in binary mode (%s).", strerror(errno));
    return ER;
  }
#endif

  /*-
   *********************************************************************
   *
   * Construct the Base64 decoder table.
   *
   *********************************************************************
   */
  DecodeBuildFromBase64Table();

  /*-
   *********************************************************************
   *
   * Allocate and initialize a new properties structure.
   *
   *********************************************************************
   */
  psProperties = FTimesSrmNewProperties(acLocalError);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
    return ER;
  }
  FTimesSrmSetPropertiesReference(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmGetPropertiesReference
 *
 ***********************************************************************
 */
FTIMES_SRM_PROPERTIES *
FTimesSrmGetPropertiesReference(void)
{
  return gpsProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmSetPropertiesReference
 *
 ***********************************************************************
 */
void
FTimesSrmSetPropertiesReference(FTIMES_SRM_PROPERTIES *psProperties)
{
  gpsProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmOptionHandler
 *
 ***********************************************************************
 */
int
FTimesSrmOptionHandler(void *pvOption, char *pcValue, void *pvProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_SRM_PROPERTIES *psProperties = (FTIMES_SRM_PROPERTIES *)pvProperties;
//int                 iLength = 0;
  MASK_USS_MASK      *psFieldMask = NULL;
  OPTIONS_TABLE      *psOption = (OPTIONS_TABLE *)pvOption;

//iLength = (pcValue == NULL) ? 0 : strlen(pcValue);

  switch (psOption->iId)
  {
  case OPT_DryRun:
    psProperties->iDryRun = 1;
    break;
  case OPT_FieldMask:
    psFieldMask = MaskParseMask(pcValue, MASK_MASK_TYPE_SRM, acLocalError);
    if (psFieldMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
      return ER;
    }
    MaskFreeMask(psProperties->psFieldMask);
    psProperties->psFieldMask = psFieldMask;
    break;
  case OPT_LogToStdout:
    psProperties->pLogHandle = stdout;
    break;
  case OPT_Version:
    FTimesSrmVersion();
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "invalid option (%d)", psOption->iId);
    return ER;
    break;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmNewProperties
 *
 ***********************************************************************
 */
FTIMES_SRM_PROPERTIES *
FTimesSrmNewProperties(char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_SRM_PROPERTIES *psProperties = NULL;
  MASK_USS_MASK      *psFieldMask = NULL;

  /*-
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (FTIMES_SRM_PROPERTIES *)calloc(sizeof(FTIMES_SRM_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Set the default field mask.
   *
   *********************************************************************
   */
  psFieldMask = MaskParseMask("none+size+md5+sha1", MASK_MASK_TYPE_SRM, acLocalError);
  if (psFieldMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", acLocalError);
    MaskFreeMask(psFieldMask);
    return NULL;
  }
  psProperties->psFieldMask = psFieldMask;

  /*-
   *********************************************************************
   *
   * Set the default log handle.
   *
   *********************************************************************
   */
  psProperties->pLogHandle = stderr;

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesSrmFreeProperties
 *
 ***********************************************************************
 */
void
FTimesSrmFreeProperties(FTIMES_SRM_PROPERTIES *psProperties)
{
  if (psProperties)
  {
    if (psProperties->psOptionsContext != NULL)
    {
      OptionsFreeOptionsContext(psProperties->psOptionsContext);
    }
    MaskFreeMask(psProperties->psFieldMask);
    free(psProperties);
  }
}


/*-
 ***********************************************************************
 *
 * FTimesSrmDecodeString
 *
 ***********************************************************************
 */
char *
FTimesSrmDecodeString(char *pcEncoded, char *pcError)
{
  char               *pcDecoded = NULL;
  int                 i = 0;
  int                 iLength = strlen(pcEncoded);
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcDecoded = malloc(iLength + 1);
  if (pcDecoded == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return NULL;
  }
  pcDecoded[0] = 0;

  /*-
   *********************************************************************
   *
   * Convert each %HH value to its corresponding character value, and
   * convert plus signs to ' '.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    switch (pcEncoded[i])
    {
    case '%':
      if (isxdigit((int) pcEncoded[i + 1]) && isxdigit((int) pcEncoded[i + 2]))
      {
        char cUpper = tolower(pcEncoded[++i]);
        char cLower = tolower(pcEncoded[++i]);
        unsigned int ui = 0;
        ui =             (((cUpper >= '0') && (cUpper <= '9')) ? (cUpper - '0') : ((cUpper - 'a') + 10));
        ui = (ui << 4) | (((cLower >= '0') && (cLower <= '9')) ? (cLower - '0') : ((cLower - 'a') + 10));
        n += sprintf(&pcDecoded[n], "%c", ui);
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "Bad hex value in string");
        free(pcDecoded);
        return NULL;
      }
      break;
    case '+':
      pcDecoded[n++] = ' ';
      break;
    default:
      pcDecoded[n++] = pcEncoded[i];
      break;
    }
  }
  pcDecoded[n] = 0;

  return pcDecoded;
}


/*-
 ***********************************************************************
 *
 * MapWriteRecord
 *
 ***********************************************************************
 */
int
MapWriteRecord(MASK_USS_MASK *psFieldMask, FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
  const char          acRoutine[] = "MapWriteRecord()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;
  int                 iIndex = 0;
  int                 iSize = 0;
  char               *pc = NULL;
  static char        *pcRecordData = NULL;

  /*-
   *********************************************************************
   *
   * Allocate (or adjust) memory buffer.
   *
   *********************************************************************
   */
  iSize =
       FTIMES_SRM_MAX_ACTION_SIZE
     + FTIMES_SRM_MAX_STATUS_SIZE
     + FTIMES_SRM_MAX_REASON_SIZE
     + (2 * strlen("\"") + strlen(psFTFileData->pcNeuteredPath))
     + FTIMES_MAX_64BIT_SIZE
     + FTIMES_MAX_MD5_LENGTH
     + FTIMES_MAX_SHA1_LENGTH
     + FTIMES_MAX_SHA256_LENGTH
     + (7 * strlen("|"))
#ifdef WIN32
     + strlen(CRLF)
#else
     + strlen(LF)
#endif
     ;

  pc = realloc(pcRecordData, iSize);
  if (pc == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: realloc(): %s", acRoutine, strerror(errno));
    return ER;
  }
  pcRecordData = pc;

  /*-
   *********************************************************************
   *
   * Action = action
   *
   *********************************************************************
   */
  switch (psFTFileData->iAction)
  {
  case FTIMES_SRM_ACTION_REMOVE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_ACTION_SIZE, "remove");
    break;
  case FTIMES_SRM_ACTION_REJECT:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_ACTION_SIZE, "reject");
    break;
  }

  /*-
   *********************************************************************
   *
   * Status = status
   *
   *********************************************************************
   */
  pcRecordData[iIndex++] = '|';
  switch (psFTFileData->iStatus)
  {
  case FTIMES_SRM_STATUS_PASS:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_STATUS_SIZE, "pass");
    break;
  case FTIMES_SRM_STATUS_FAIL:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_STATUS_SIZE, "fail");
    break;
  case FTIMES_SRM_STATUS_SKIP:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_STATUS_SIZE, "skip");
    break;
  case FTIMES_SRM_STATUS_NOOP:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_STATUS_SIZE, "noop");
    break;
  }

  /*-
   *********************************************************************
   *
   * Reason = reason
   *
   *********************************************************************
   */
  pcRecordData[iIndex++] = '|';
  switch (psFTFileData->iReason)
  {
  case FTIMES_SRM_REASON_FILE_FTDATA_FAILURE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_ftdata_failure");
    break;
  case FTIMES_SRM_REASON_FILE_DOES_NOT_EXIST:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_does_not_exist");
    break;
  case FTIMES_SRM_REASON_FILE_ATTRIB_FAILURE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_attrib_failure");
    break;
  case FTIMES_SRM_REASON_FILE_IS_NOT_REGULAR:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_is_not_regular");
    break;
  case FTIMES_SRM_REASON_FILE_ACCESS_FAILURE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_access_failure");
    break;
  case FTIMES_SRM_REASON_FILE_IOREAD_FAILURE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_ioread_failure");
    break;
  case FTIMES_SRM_REASON_CONSTRAINTS_NOT_MET:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "constraints_not_met");
    break;
  case FTIMES_SRM_REASON_FILE_UNLINK_SUCCESS:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_unlink_success");
    break;
  case FTIMES_SRM_REASON_FILE_UNLINK_FAILURE:
    iIndex += snprintf(&pcRecordData[iIndex], FTIMES_SRM_MAX_REASON_SIZE, "file_unlink_failure");
    break;
  }

  /*-
   *********************************************************************
   *
   * File Name = name
   *
   *********************************************************************
   */
  pcRecordData[iIndex++] = '|';
  iIndex += sprintf(&pcRecordData[iIndex], "\"%s\"", psFTFileData->pcNeuteredPath);

  /*-
   *********************************************************************
   *
   * File Size = size
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SIZE))
  {
    pcRecordData[iIndex++] = '|';
    if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, MAP_SIZE))
    {
#ifdef WIN32
      iIndex += snprintf(&pcRecordData[iIndex], FTIMES_MAX_64BIT_SIZE, "%I64u", (APP_UI64)psFTFileData->ui64FileSize);
#else
      iIndex += snprintf(&pcRecordData[iIndex], FTIMES_MAX_64BIT_SIZE, "%llu", (unsigned long long)psFTFileData->ui64FileSize);
#endif
    }
  }

  /*-
   *********************************************************************
   *
   * File MD5 = md5
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_MD5))
  {
    pcRecordData[iIndex++] = '|';
    if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, MAP_MD5))
    {
      iIndex += MD5HashToHex(psFTFileData->aucFileMd5, &pcRecordData[iIndex]);
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA1 = sha1
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA1))
  {
    pcRecordData[iIndex++] = '|';
    if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, MAP_SHA1))
    {
      iIndex += SHA1HashToHex(psFTFileData->aucFileSha1, &pcRecordData[iIndex]);
    }
  }

  /*-
   *********************************************************************
   *
   * File SHA256 = sha256
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psFieldMask->ulMask, SRM_SHA256))
  {
    pcRecordData[iIndex++] = '|';
    if (MASK_BIT_IS_SET(psFTFileData->ulAttributeMask, MAP_SHA256))
    {
      iIndex += SHA256HashToHex(psFTFileData->aucFileSha256, &pcRecordData[iIndex]);
    }
  }

  /*-
   *********************************************************************
   *
   * EOL
   *
   *********************************************************************
   */
#ifdef WIN32
  iIndex += sprintf(&pcRecordData[iIndex], "%s", CRLF);
#else
  iIndex += sprintf(&pcRecordData[iIndex], "%s", LF);
#endif

  /*-
   *********************************************************************
   *
   * Write the output data.
   *
   *********************************************************************
   */
  iError = SupportWriteData(stdout, pcRecordData, iIndex, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapWriteHeader
 *
 ***********************************************************************
 */
int
MapWriteHeader(MASK_USS_MASK *psFieldMask, char *pcError)
{
  const char          acRoutine[] = "MapWriteHeader()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acHeaderData[FTIMES_MAX_LINE] = "";
  int                 i = 0;
  int                 iError = 0;
  int                 iIndex = 0;
  int                 iMaskTableLength = MaskGetTableLength(MASK_MASK_TYPE_SRM);
  MASK_B2S_TABLE     *psMaskTable = MaskGetTableReference(MASK_MASK_TYPE_SRM);
  unsigned long       ul = 0;

  /*-
   *********************************************************************
   *
   * Build the output's header.
   *
   *********************************************************************
   */
  iIndex = sprintf(acHeaderData, "action|status|reason|name");
  for (i = 0; i < iMaskTableLength; i++)
  {
    ul = (1 << i);
    if (MASK_BIT_IS_SET(psFieldMask->ulMask, ul))
    {
      iIndex += sprintf(&acHeaderData[iIndex], "|%s", (char *) psMaskTable[i].acName);
    }
  }
#ifdef WIN32
  iIndex += sprintf(&acHeaderData[iIndex], "%s", CRLF);
#else
  iIndex += sprintf(&acHeaderData[iIndex], "%s", LF);
#endif

  /*-
   *********************************************************************
   *
   * Write the output's header.
   *
   *********************************************************************
   */
  iError = SupportWriteData(stdout, acHeaderData, iIndex, acLocalError);
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
 * FTimesSrmGetAttributes
 *
 ***********************************************************************
 */
int
FTimesSrmGetAttributes(FTIMES_FILE_DATA *psFTFileData, char *pcError)
{
#ifdef WINNT
  char               *pcMessage;
  WIN32_FILE_ATTRIBUTE_DATA sFileAttributeData;

  psFTFileData->ulAttributeMask = 0;
  psFTFileData->iExists = -1; /* Indeterminate. */
  if (!GetFileAttributesExW(psFTFileData->pwcRawPath, GetFileExInfoStandard, &sFileAttributeData))
  {
    DWORD dwError = GetLastError();
    psFTFileData->iExists = (dwError == ERROR_FILE_NOT_FOUND || dwError == ERROR_PATH_NOT_FOUND) ? 0 : -1;
    FTimesSrmFormatWinxError(dwError, &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s", pcMessage);
    return ER;
  }
  psFTFileData->ulAttributeMask = MAP_ATTRIBUTES | MAP_SIZE;
  psFTFileData->dwFileAttributes = sFileAttributeData.dwFileAttributes;
  psFTFileData->ui64FileSize = (((APP_UI64)sFileAttributeData.nFileSizeHigh) << 32) | (APP_UI64)sFileAttributeData.nFileSizeLow;
#else
  if (lstat(psFTFileData->pcRawPath, &psFTFileData->sStatEntry) != ER_OK) /* Don't follow symbolic links here. */
  {
    psFTFileData->iExists = (errno == ENOENT || errno == ENOTDIR) ? 0 : -1;
    snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
    return ER;
  }
  psFTFileData->ulAttributeMask = MAP_LSTAT_MASK;
  psFTFileData->ui64FileSize = (APP_UI64)psFTFileData->sStatEntry.st_size;
#endif
  psFTFileData->iExists = 1;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapFreeFTFileData
 *
 ***********************************************************************
 */
void
MapFreeFTFileData(FTIMES_FILE_DATA *psFTFileData)
{
  if (psFTFileData != NULL)
  {
    if (psFTFileData->pcNeuteredPath != NULL)
    {
      free(psFTFileData->pcNeuteredPath);
    }
    if (psFTFileData->pcRawPath != NULL)
    {
      free(psFTFileData->pcRawPath);
    }
#ifdef WINNT
    if (psFTFileData->pwcRawPath != NULL)
    {
      free(psFTFileData->pwcRawPath);
    }
#endif
    free(psFTFileData);
  }
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapNewFTFileData
 *
 ***********************************************************************
 */
FTIMES_FILE_DATA *
MapNewFTFileDataW(wchar_t *pwcName, char *pcError)
{
  const char          acRoutine[] = "MapNewFTFileDataW()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_FILE_DATA   *psFTFileData = NULL;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the file data structure.
   *
   *********************************************************************
   */
  psFTFileData = (FTIMES_FILE_DATA *) calloc(sizeof(FTIMES_FILE_DATA), 1);
  if (psFTFileData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Initialize variables that require a nonzero value. Also note that
   * subsequent logic relies on the assertion that each hash value has
   * been initialized to all zeros.
   *
   *********************************************************************
   */
  /* Empty. */

  /*-
   *********************************************************************
   *
   * Create the new path. Impose a limit to keep things under control.
   *
   *********************************************************************
   */
  psFTFileData->iWideRawPathLength = FTIMES_EXTENDED_PREFIX_SIZE + wcslen(pwcName);
  if (psFTFileData->iWideRawPathLength - FTIMES_EXTENDED_PREFIX_SIZE > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length (%d) exceeds %d bytes.", acRoutine, psFTFileData->iWideRawPathLength, FTIMES_MAX_PATH - 1);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->pwcRawPath = malloc((psFTFileData->iWideRawPathLength + 1) * sizeof(wchar_t));
  if (psFTFileData->pwcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  _snwprintf(psFTFileData->pwcRawPath, psFTFileData->iWideRawPathLength + 1, L"\\\\?\\%s", pwcName); /* Include the extended path prefix since there is no parent. */

  /*-
   *********************************************************************
   *
   * Convert the new path to UTF-8.
   *
   *********************************************************************
   */
  psFTFileData->pcRawPath = MapWideToUtf8(&psFTFileData->pwcRawPath[FTIMES_EXTENDED_PREFIX_SIZE], psFTFileData->iWideRawPathLength - FTIMES_EXTENDED_PREFIX_SIZE + 1, acLocalError);
  if (psFTFileData->pcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->iUtf8RawPathLength = strlen(psFTFileData->pcRawPath);

  /*-
   *********************************************************************
   *
   * Neuter the new path.
   *
   *********************************************************************
   */
  psFTFileData->pcNeuteredPath = SupportNeuterString(psFTFileData->pcRawPath, psFTFileData->iUtf8RawPathLength, acLocalError);
  if (psFTFileData->pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }

  return psFTFileData;
}
#else
/*-
 ***********************************************************************
 *
 * MapNewFTFileData
 *
 ***********************************************************************
 */
FTIMES_FILE_DATA *
MapNewFTFileData(char *pcName, char *pcError)
{
  const char          acRoutine[] = "MapNewFTFileData()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_FILE_DATA   *psFTFileData = NULL;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the file data structure.
   *
   *********************************************************************
   */
  psFTFileData = (FTIMES_FILE_DATA *)calloc(sizeof(FTIMES_FILE_DATA), 1);
  if (psFTFileData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*
   *********************************************************************
   *
   * Initialize variables that require a nonzero value. Also note that
   * subsequent logic relies on the assertion that each hash value has
   * been initialized to all zeros.
   *
   *********************************************************************
   */
  /* Empty. */

  /*-
   *********************************************************************
   *
   * Create the new path. Impose a limit to keep things under control.
   *
   *********************************************************************
   */
  psFTFileData->iRawPathLength = strlen(pcName);
  if (psFTFileData->iRawPathLength > FTIMES_MAX_PATH - 1) /* Subtract one for the NULL. */
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Length (%d) exceeds %d bytes.", acRoutine, psFTFileData->iRawPathLength, FTIMES_MAX_PATH - 1);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  psFTFileData->pcRawPath = malloc(psFTFileData->iRawPathLength + 1);
  if (psFTFileData->pcRawPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  snprintf(psFTFileData->pcRawPath, FTIMES_MAX_PATH, "%s", pcName);

  /*-
   *********************************************************************
   *
   * Neuter the new path.
   *
   *********************************************************************
   */
  psFTFileData->pcNeuteredPath = SupportNeuterString(psFTFileData->pcRawPath, psFTFileData->iRawPathLength, acLocalError);
  if (psFTFileData->pcNeuteredPath == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    MapFreeFTFileData(psFTFileData);
    return NULL;
  }
  return psFTFileData;
}
#endif


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapUtf8ToWide
 *
 ***********************************************************************
 */
wchar_t *
MapUtf8ToWide(char *pcString, int iUtf8Size, char *pcError)
{
  char               *pcMessage = NULL;
  int                 iWideSize = 0;
  wchar_t            *pwcString = NULL;

  iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, NULL, 0); /* The byte count returned includes the NULL terminator. */
  if (iWideSize)
  {
    pwcString = malloc(iWideSize * sizeof(wchar_t));
    if (pwcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s", strerror(errno));
      return NULL;
    }
    iWideSize = MultiByteToWideChar(CP_UTF8, 0, pcString, iUtf8Size, pwcString, iWideSize);
    if (!iWideSize)
    {
      FTimesSrmFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s", pcMessage);
      free(pwcString);
      return NULL;
    }
  }
  else
  {
    FTimesSrmFormatWinxError(GetLastError(), &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s", pcMessage);
    return NULL;
  }

  return pwcString;
}
#endif


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * MapWideToUtf8
 *
 ***********************************************************************
 */
char *
MapWideToUtf8(wchar_t *pwcString, int iWideSize, char *pcError)
{
  const char          acRoutine[] = "MapWideToUtf8()";
  char               *pcMessage = NULL;
  char               *pcString = NULL;
  int                 iUtf8Size = 0;

  iUtf8Size = WideCharToMultiByte(CP_UTF8, 0, pwcString, iWideSize, NULL, 0, NULL, NULL); /* The byte count returned includes the NULL terminator. */
  if (iUtf8Size)
  {
    pcString = malloc(iUtf8Size);
    if (pcString == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: malloc(): %s", acRoutine, strerror(errno));
      return NULL;
    }
    iUtf8Size = WideCharToMultiByte(CP_UTF8, 0, pwcString, iWideSize, pcString, iUtf8Size, NULL, NULL);
    if (!iUtf8Size)
    {
      FTimesSrmFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: WideCharToMultiByte(): %s", acRoutine, pcMessage);
      free(pcString);
      return NULL;
    }
  }
  else
  {
    FTimesSrmFormatWinxError(GetLastError(), &pcMessage);
    snprintf(pcError, MESSAGE_SIZE, "%s: WideCharToMultiByte(): %s", acRoutine, pcMessage);
    return NULL;
  }

  return pcString;
}
#endif


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * SupportAdjustPrivileges
 *
 ***********************************************************************
 */
BOOL
SupportAdjustPrivileges(LPCTSTR lpcPrivilege)
{
  HANDLE              hToken;
  TOKEN_PRIVILEGES    sTokenPrivileges;
  BOOL                bResult;

  /*-
   *********************************************************************
   *
   * Open the access token associated with this process.
   *
   *********************************************************************
   */
  bResult = OpenProcessToken
              (
                GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                &hToken
              );
  if (bResult == FALSE)
  {
    return FALSE;
  }

  /*-
   *********************************************************************
   *
   * Retrieve the locally unique identifier (LUID) used on this system
   * that represents the specified privilege.
   *
   *********************************************************************
   */
  bResult = LookupPrivilegeValue(
                                NULL,
                                lpcPrivilege,
                                &sTokenPrivileges.Privileges[0].Luid
    );

  if (bResult == FALSE)
  {
    return FALSE;
  }

  sTokenPrivileges.PrivilegeCount = 1;

  /*-
   *********************************************************************
   *
   * One privilege to set.
   *
   *********************************************************************
   */
  sTokenPrivileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

  /*-
   *********************************************************************
   *
   * Enable the specified privilege. Note: Enabling and/or disabling
   * privileges requires TOKEN_ADJUST_PRIVILEGES access.
   *
   *********************************************************************
   */
  bResult = AdjustTokenPrivileges(
                                 hToken,
                                 FALSE,
                                 &sTokenPrivileges,
                                 0,
                                 (PTOKEN_PRIVILEGES) NULL,
                                 0
    );

  if (bResult == FALSE || GetLastError() != ERROR_SUCCESS)
  {
    return FALSE;
  }

  return TRUE;
}
#endif


/*-
 ***********************************************************************
 *
 * SupportChopEOLs
 *
 ***********************************************************************
 */
int
SupportChopEOLs(char *pcLine, int iStrict, char *pcError)
{
  const char          acRoutine[] = "SupportChopEOLs()";
  int                 iLineLength;
  int                 iSaveLength;

  /*-
   *********************************************************************
   *
   * Calculate line length.
   *
   *********************************************************************
   */
  iLineLength = iSaveLength = strlen(pcLine);

  /*-
   *********************************************************************
   *
   * Scan backwards over EOL characters.
   *
   *********************************************************************
   */
  while (iLineLength > 0 && ((pcLine[iLineLength - 1] == '\r') || (pcLine[iLineLength - 1] == '\n')))
  {
    iLineLength--;
  }

  /*-
   *********************************************************************
   *
   * If strict checking is on and EOL was not found, it's an error.
   *
   *********************************************************************
   */
  if (iStrict && iLineLength == iSaveLength)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: EOL required but not found.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Terminate line excluding any EOL characters.
   *
   *********************************************************************
   */
  pcLine[iLineLength] = 0;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportGetFileHandle
 *
 ***********************************************************************
 */
FILE *
SupportGetFileHandle(char *pcFile, char *pcError)
{
  const char          acRoutine[] = "SupportGetFileHandle()";
  static int          iStdinTaken = 0;
  FILE               *pFile = NULL;

  /*-
   *********************************************************************
   *
   * Open the specified file. If "-" was specified, bind the handle to
   * stdin, but do not do this more than once per invocation.
   *
   *********************************************************************
   */
  if (strcmp(pcFile, "-") == 0 && iStdinTaken == 0)
  {
    pFile = stdin;
    iStdinTaken = 1;
  }
  else
  {
    pFile = fopen(pcFile, "rb");
    if (pFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fopen(): %s", acRoutine, strerror(errno));
      return NULL;
    }
  }

  return pFile;
}


/*-
 ***********************************************************************
 *
 * SupportNeuterString
 *
 ***********************************************************************
 */
char *
SupportNeuterString(char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SupportNeuterString()";
  char               *pcNeutered = NULL;
  int                 i = 0;
  int                 n = 0;

  /*-
   *********************************************************************
   *
   * The caller is expected to free this memory.
   *
   *********************************************************************
   */
  pcNeutered = malloc((3 * iLength) + 1);
  if (pcNeutered == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, strerror(errno));
    return NULL;
  }
  pcNeutered[0] = 0;

  /*-
   *********************************************************************
   *
   * Encode non-printables and [|"'`%+#]. Conditionally encode '/' and
   * '\' depending on the target platform. Convert spaces to '+'. Avoid
   * isprint() here because it has led to unexpected results on Windows
   * platforms. In the past, isprint() on certain Windows systems has
   * decided that several characters in the range 0x7f - 0xff are
   * printable.
   *
   *********************************************************************
   */
  for (i = n = 0; i < iLength; i++)
  {
    if (pcData[i] > '~' || pcData[i] < ' ')
    {
      n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
    }
    else
    {
      switch (pcData[i])
      {
      case '|':
      case '"':
      case '\'':
      case '`':
      case '%':
      case '+':
      case '#':
#ifdef WINNT
      case '/':
#else
      case '\\':
#endif
        n += sprintf(&pcNeutered[n], "%%%02x", (unsigned char) pcData[i]);
        break;
      case ' ':
        pcNeutered[n++] = '+';
        break;
      default:
        pcNeutered[n++] = pcData[i];
        break;
      }
    }
  }
  pcNeutered[n] = 0;

  return pcNeutered;
}


#ifdef WINNT
/*-
 ***********************************************************************
 *
 * SupportSetPrivileges
 *
 ***********************************************************************
 */
int
SupportSetPrivileges(char *pcError)
{
  const char          acRoutine[] = "SupportSetPrivileges()";

  /*-
   *********************************************************************
   *
   * Attempt to obtain backup user rights.
   *
   *********************************************************************
   */
  if (SupportAdjustPrivileges(SE_BACKUP_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Can't set SE_BACKUP_NAME privilege.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Attempt to obtain restore user rights.
   *
   *********************************************************************
   */
  if (SupportAdjustPrivileges(SE_RESTORE_NAME) == FALSE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Can't set SE_RESTORE_NAME privilege.", acRoutine);
    return ER;
  }

  return ER_OK;
}
#endif


/*-
 ***********************************************************************
 *
 * SupportStringToUInt64
 *
 ***********************************************************************
 */
int
SupportStringToUInt64(char *pcData, APP_UI64 *pui64Value, char *pcError)
{
  int                 i = 0;
  int                 iLength = 0;
  APP_UI64            ui64Value = 0;
  APP_UI64            ui64Multiplier = 1;

  if (pcData == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "NULL input");
    return ER;
  }
  iLength = strlen(pcData);

#define SUPPORT_MAX_64BIT_NUMBER_SIZE 20 /* strlen("18446744073709551615") */
  if (iLength < 1 || iLength > SUPPORT_MAX_64BIT_NUMBER_SIZE)
  {
    snprintf(pcError, MESSAGE_SIZE, "input value is too short/long to be a valid 64-bit number");
    return ER;
  }

  for (i = iLength - 1; i >= 0; i--)
  {
    switch ((int) pcData[i])
    {
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      ui64Value += ((int) pcData[i] - 0x30) * ui64Multiplier;
      ui64Multiplier *= 10;
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "input value contains one or more invalid digits");
      return ER;
      break;
    }
  }
  *pui64Value = ui64Value;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * SupportWriteData
 *
 ***********************************************************************
 */
int
SupportWriteData(FILE *pFile, char *pcData, int iLength, char *pcError)
{
  const char          acRoutine[] = "SupportWriteData()";
  int                 iNWritten;

  iNWritten = fwrite(pcData, 1, iLength, pFile);
  if (ferror(pFile))
  {
    if (iNWritten != iLength)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): NWritten = [%d] != [%d]: WriteLength mismatch!: %s",
        acRoutine,
        iNWritten,
        iLength,
        (errno == 0) ? "unexpected error -- check device for sufficient space" : strerror(errno)
        );
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: fwrite(): %s",
        acRoutine,
        (errno == 0) ? "unexpected error -- check device for sufficient space" : strerror(errno)
        );
    }
    return ER;
  }
  if (fflush(pFile) != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: fflush(): %s", acRoutine, strerror(errno));
    return ER;
  }

  return ER_OK;
}
