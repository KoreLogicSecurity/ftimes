/*-
 ***********************************************************************
 *
 * $Id: cmpmode.c,v 1.7 2004/04/04 07:09:49 mavrik Exp $
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
 * CmpModeProcessArguments
 *
 ***********************************************************************
 */
int
CmpModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "CmpModeProcessArguments()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;
  int                 iLength;

  /*-
   *********************************************************************
   *
   * Process arguments.
   *
   *********************************************************************
   */
  if (iArgumentCount >= 3)
  {
    iLength = strlen(ppcArgumentVector[0]);
    if (iLength > ALL_FIELDS_MASK_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Mask = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, ppcArgumentVector[0], iLength, ALL_FIELDS_MASK_SIZE - 1);
      return ER_Length;
    }
    if (strcasecmp(ppcArgumentVector[0], "NONE") == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Mask = [%s], Mask must include at least one field (e.g. NONE+md5).", acRoutine, ppcArgumentVector[0]);
      return ER_BadValue;
    }
    iError = CompareParseStringMask(ppcArgumentVector[0], &psProperties->ulFieldMask, psProperties->iRunMode, psProperties->psMaskTable, psProperties->iMaskTableLength, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Mask = [%s]: %s", acRoutine, ppcArgumentVector[0], acLocalError);
      return iError;
    }
    strncpy(psProperties->acMaskString, ppcArgumentVector[0], ALL_FIELDS_MASK_SIZE);
    psProperties->pcBaselineFile = ppcArgumentVector[1];
    psProperties->pcSnapshotFile = ppcArgumentVector[2];
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
  CMP_PROPERTIES     *psCmpProperties;

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
  CompareSetMask(psProperties->ulFieldMask);

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

  if (psProperties->acMaskString[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing FieldMask.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->pcBaselineFile == NULL || psProperties->pcBaselineFile[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing baseline file.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->pcSnapshotFile == NULL || psProperties->pcSnapshotFile[0] == 0)
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
  int                 iError;

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
   *********************************************************************
   *
   * Write out a Compare header record.
   *
   *********************************************************************
   */
  CompareSetNewLine(psProperties->acNewLine);
  iError = CompareWriteHeader(psProperties->pFileOut, psProperties->acNewLine, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Compare Header: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *********************************************************************
   *
   * Display properties.
   *
   *********************************************************************
   */
  PropertiesDisplaySettings(psProperties);

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
  int                 iError;

  iError = CompareLoadBaselineData(psProperties->pcBaselineFile, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  iError = CompareEnumerateChanges(psProperties->pcSnapshotFile, acLocalError);
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
 * CmpModeFinishUp
 *
 ***********************************************************************
 */
int
CmpModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                acMessage[MESSAGE_SIZE];

  /*-
   *********************************************************************
   *
   * Print output filenames.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "LogFileName=%s", psProperties->acLogFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "OutFileName=%s", psProperties->acOutFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "DataType=%s", psProperties->acDataType);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "RecordsAnalyzed=%d", CompareGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "ChangedCount=%d", CompareGetChangedCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "MissingCount=%d", CompareGetMissingCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "NewCount=%d", CompareGetNewCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "UnknownCount=%d", CompareGetUnknownCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "CrossedCount=%d", CompareGetCrossedCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

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
