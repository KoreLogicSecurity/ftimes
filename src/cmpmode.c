/*-
 ***********************************************************************
 *
 * $Id: cmpmode.c,v 1.18 2007/02/23 00:22:35 mavrik Exp $
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
 * CmpModeProcessArguments
 *
 ***********************************************************************
 */
int
CmpModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "CmpModeProcessArguments()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError = 0;

  /*-
   *********************************************************************
   *
   * Process arguments.
   *
   *********************************************************************
   */
  if (iArgumentCount >= 3)
  {
    psProperties->psFieldMask = MaskParseMask(ppcArgumentVector[0], MASK_RUNMODE_TYPE_CMP, acLocalError);
    if (psProperties->psFieldMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    psProperties->psBaselineContext->pcFile = ppcArgumentVector[1];
    psProperties->psSnapshotContext->pcFile = ppcArgumentVector[2];
    if (iArgumentCount >= 4)
    {
      if (iArgumentCount == 5 && strcmp(ppcArgumentVector[3], "-l") == 0)
      {
        iError = SupportSetLogLevel(ppcArgumentVector[4], &psProperties->iLogLevel, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Level = [%s]: %s", acRoutine, ppcArgumentVector[4], acLocalError);
          return iError;
        }
      }
      else
      {
        return ER_Usage;
      }
    }
  }
  else
  {
    return ER_Usage;
  }
  return ER_OK;
}


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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  CMP_PROPERTIES     *psCmpProperties = NULL;

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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acMessage[MESSAGE_SIZE] = { 0 };
  char               *pcMask = NULL;
  CMP_PROPERTIES     *pcCmpProperties = CompareGetPropertiesReference();
  int                 iError = 0;

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
  pcCmpProperties->psCompareMask->ulMask &= psProperties->psBaselineContext->ulFieldMask; /* Remove fields not present in the baseline. */
  pcCmpProperties->psCompareMask->ulMask &= psProperties->psSnapshotContext->ulFieldMask; /* Remove fields not present in the snapshot. */
  if (pcCmpProperties->psCompareMask->ulMask == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: The baseline and snapshot have no fields in common. Only (M)issing and (N)ew changes will be detected.", acRoutine);
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

  pcMask = MaskBuildMask(pcCmpProperties->psCompareMask->ulMask, MASK_RUNMODE_TYPE_CMP, acLocalError);
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
  CompareFreeProperties(CompareGetPropertiesReference());
  return ER_OK;
}
