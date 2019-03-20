/*-
 ***********************************************************************
 *
 * $Id: cmpmode.c,v 1.34 2012/01/04 03:12:27 mavrik Exp $
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
 * CmpModeInitialize
 *
 ***********************************************************************
 */
int
CmpModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "CmpModeInitialize()";
  char                acLocalError[MESSAGE_SIZE] = "";
  CMP_PROPERTIES     *psCmpProperties = NULL;
  int                 iError = 0;

  /*-
   *********************************************************************
   *
   * Initialize Compare properties.
   *
   *********************************************************************
   */
  psCmpProperties = CompareNewProperties(acLocalError);
  if (psCmpProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    free(psProperties);
    return ER;
  }
  CompareSetPropertiesReference(psCmpProperties);

  /*-
   *********************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;
  psProperties->pFileOut = stdout;
  psCmpProperties->psCompareMask = psProperties->psFieldMask;

  DecodeBuildFromBase64Table();

  /*-
   *********************************************************************
   *
   * Set the priority.
   *
   *********************************************************************
   */
  iError = SupportSetPriority(psProperties, acLocalError);
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
 * CmpModeCheckDependencies
 *
 ***********************************************************************
 */
int
CmpModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "CmpModeCheckDependencies()";

  if (psProperties->psFieldMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing FieldMask.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psBaselineContext->pcFile == NULL || psProperties->psBaselineContext->pcFile[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing baseline file.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psSnapshotContext->pcFile == NULL || psProperties->psSnapshotContext->pcFile[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing snapshot file.", acRoutine);
    return ER_MissingControl;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CmpModeFinalize
 *
 ***********************************************************************
 */
int
CmpModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "CmpModeFinalize()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acMessage[MESSAGE_SIZE] = { 0 };
  char               *pcMask = NULL;
  CMP_PROPERTIES     *psCmpProperties = CompareGetPropertiesReference();
  int                 iError = 0;
  int                 iLength = 0;
  struct stat         statEntry = { 0 };

  /*-
   *********************************************************************
   *
   * Determine whether or not a backing file will be required. This is
   * done for two reasons: 1) there's no need to create a backing file
   * for small baselines and 2) if the backing file size ends up being
   * zero, attempts to map it can lead to EINVAL errors on some
   * platforms.
   *
   *********************************************************************
   */
  if
  (
       psProperties->iMemoryMapEnable
    && (stat(psProperties->psBaselineContext->pcFile, &statEntry) == ER_OK)
    && ((statEntry.st_mode & S_IFMT) == S_IFREG)
    && (statEntry.st_size > FTIMES_MIN_MMAP_SIZE)
  )
  {
    psCmpProperties->iMemoryMapFile = 1;
  }

  /*-
   *********************************************************************
   *
   * Conditionally initialize the name for a backing file.
   *
   *********************************************************************
   */
  if (psCmpProperties->iMemoryMapFile)
  {
    iLength = strlen(psProperties->acTempDirectory) +
      strlen(FTIMES_SLASH) +
      strlen(PROGRAM_NAME) +
      strlen("_") +
      strlen("4294967295") +
      strlen("_") +
      strlen(psProperties->pcNonce) +
      strlen(".mmap") +
      1;
    psCmpProperties->pcMemoryMapFile = calloc(iLength, 1);
    if (psCmpProperties->pcMemoryMapFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
      return ER;
    }
    snprintf(psCmpProperties->pcMemoryMapFile, iLength, "%s%s%s_%ld_%s.mmap",
      psProperties->acTempDirectory,
      FTIMES_SLASH,
      PROGRAM_NAME,
      (long) psProperties->tvJobEpoch.tv_sec,
      psProperties->pcNonce
      );
  }

  /*-
   *********************************************************************
   *
   * Establish Log file stream, and update Message Handler.
   *
   *********************************************************************
   */
  strncpy(psProperties->acLogFileName, "stderr", FTIMES_MAX_PATH);
  psProperties->pFileLog = stderr;

  MessageSetNewLine(psProperties->acNewLine);
  MessageSetOutputStream(psProperties->pFileLog);
  MessageSetAutoFlush(MESSAGE_AUTO_FLUSH_ON);

  /*-
   *******************************************************************
   *
   * Establish Out file stream.
   *
   *******************************************************************
   */
  strncpy(psProperties->acOutFileName, "stdout", FTIMES_MAX_PATH);
  psProperties->pFileOut = stdout;

  CompareSetOutputStream(psProperties->pFileOut);

  /*-
   *******************************************************************
   *
   * Open the baseline, and parse its header.
   *
   *******************************************************************
   */
  iError = DecodeOpenSnapshot(psProperties->psBaselineContext, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *******************************************************************
   *
   * Open the snapshot, and parse its header.
   *
   *******************************************************************
   */
  iError = DecodeOpenSnapshot(psProperties->psSnapshotContext, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *******************************************************************
   *
   * Finalize the compare mask. Abort if there's nothing to compare.
   *
   *******************************************************************
   */
  if (psCmpProperties->psCompareMask->ulMask)
  {
    psCmpProperties->psCompareMask->ulMask &= psProperties->psBaselineContext->ulFieldMask; /* Remove fields not present in the baseline. */
    psCmpProperties->psCompareMask->ulMask &= psProperties->psSnapshotContext->ulFieldMask; /* Remove fields not present in the snapshot. */
    if (psCmpProperties->psCompareMask->ulMask == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: The baseline and snapshot have no fields in common. Only (M)issing and (N)ew changes will be detected.", acRoutine);
      ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    }
  }
  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: No fields were specified in the compare mask. Only (M)issing and (N)ew changes will be detected.", acRoutine);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }

  /*-
   *******************************************************************
   *
   * Display properties.
   *
   *******************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "Baseline=%s", psProperties->psBaselineContext->pcFile);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "Snapshot=%s", psProperties->psSnapshotContext->pcFile);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "BaselineCompressed=%s", (psProperties->psBaselineContext->iCompressed) ? "Y" : "N");
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "SnapshotCompressed=%s", (psProperties->psSnapshotContext->iCompressed) ? "Y" : "N");
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  PropertiesDisplaySettings(psProperties);

  pcMask = MaskBuildMask(psProperties->psBaselineContext->ulFieldMask, MASK_RUNMODE_TYPE_CMP, acLocalError);
  if (pcMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  snprintf(acMessage, MESSAGE_SIZE, "BaselineFieldMask=%s", pcMask);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  free(pcMask);

  pcMask = MaskBuildMask(psProperties->psSnapshotContext->ulFieldMask, MASK_RUNMODE_TYPE_CMP, acLocalError);
  if (pcMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  snprintf(acMessage, MESSAGE_SIZE, "SnapshotFieldMask=%s", pcMask);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  free(pcMask);

  pcMask = MaskBuildMask(psCmpProperties->psCompareMask->ulMask, MASK_RUNMODE_TYPE_CMP, acLocalError);
  if (pcMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  snprintf(acMessage, MESSAGE_SIZE, "CompareMask=%s", pcMask);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  free(pcMask);

  /*-
   *******************************************************************
   *
   * Write out a header.
   *
   *******************************************************************
   */
  CompareSetNewLine(psProperties->acNewLine);
  iError = CompareWriteHeader(psProperties->pFileOut, psProperties->acNewLine, acLocalError);
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
 * CmpModeWorkHorse
 *
 ***********************************************************************
 */
int
CmpModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "CmpModeWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;

  iError = CompareLoadBaselineData(psProperties->psBaselineContext, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    fclose(psProperties->psBaselineContext->pFile);
    return iError;
  }
  fclose(psProperties->psBaselineContext->pFile);

  iError = CompareEnumerateChanges(psProperties->psBaselineContext, psProperties->psSnapshotContext, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    fclose(psProperties->psSnapshotContext->pFile);
    return iError;
  }
  fclose(psProperties->psSnapshotContext->pFile);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CmpModeFinishUp
 *
 ***********************************************************************
 */
int
CmpModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                acMessage[MESSAGE_SIZE] = { 0 };

  /*-
   *********************************************************************
   *
   * Print output filenames.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "LogFileName=%s", psProperties->acLogFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "OutFileName=%s", psProperties->acOutFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "DataType=%s", psProperties->acDataType);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "RecordsAnalyzed=%d", CompareGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "ChangedCount=%d", CompareGetChangedCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "MissingCount=%d", CompareGetMissingCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "NewCount=%d", CompareGetNewCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "UnknownCount=%d", CompareGetUnknownCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "CrossedCount=%d", CompareGetCrossedCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * CmpModeFinalStage
 *
 ***********************************************************************
 */
int
CmpModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  CMP_PROPERTIES     *psCmpProperties = CompareGetPropertiesReference();

  /*-
   *********************************************************************
   *
   * Conditionally unmap memory and delete the associated file. Since
   * the program is shutting down and the memory map file is no longer
   * needed, there's not much point in checking the return values for
   * these calls.
   *
   *********************************************************************
   */
  if (psCmpProperties->iMemoryMapFile)
  {
#ifdef WINNT
    UnmapViewOfFile(psCmpProperties->pvMemoryMap);
#else
    munmap(psCmpProperties->pvMemoryMap, psCmpProperties->iMemoryMapSize);
#endif
    unlink(psCmpProperties->pcMemoryMapFile);
  }

  CompareFreeProperties(psCmpProperties);

  return ER_OK;
}
