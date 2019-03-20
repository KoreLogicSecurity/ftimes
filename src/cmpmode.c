/*
 ***********************************************************************
 *
 * $Id: cmpmode.c,v 1.1.1.1 2002/01/18 03:17:03 mavrik Exp $
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
 * CmpModeProcessArguments
 *
 ***********************************************************************
 */
int
CmpModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "CmpModeProcessArguments()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError,
                      iLength;

  cLocalError[0] = 0;

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
      snprintf(pcError, ERRBUF_SIZE, "%s: Mask = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, ppcArgumentVector[0], iLength, ALL_FIELDS_MASK_SIZE - 1);
      return ER_Length;
    }
    if (strcasecmp(ppcArgumentVector[0], "NONE") == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Mask = [%s], Mask must include at least one field (e.g. NONE+md5).", cRoutine, ppcArgumentVector[0]);
      return ER_BadValue;
    }
    iError = CompareParseStringMask(ppcArgumentVector[0], &psProperties->ulFieldMask, psProperties->iRunMode, psProperties->ptMaskTable, psProperties->iMaskTableLength, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Mask = [%s]: %s", cRoutine, ppcArgumentVector[0], cLocalError);
      return iError;
    }
    strncpy(psProperties->cMaskString, ppcArgumentVector[0], ALL_FIELDS_MASK_SIZE);
    psProperties->pcBaselineFile = ppcArgumentVector[1];
    psProperties->pcSnapshotFile = ppcArgumentVector[2];
    if (iArgumentCount >= 4)
    {
      if (iArgumentCount == 5 && strcmp(ppcArgumentVector[3], "-l") == 0)
      {
        iError = SupportSetLogLevel(ppcArgumentVector[4], &psProperties->iLogLevel, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Level = [%s]: %s", cRoutine, ppcArgumentVector[4], cLocalError);
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
  const char          cRoutine[] = "CmpModeCheckDependencies()";

  if (psProperties->cMaskString[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing FieldMask.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->pcBaselineFile == NULL || psProperties->pcBaselineFile[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing baseline file.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->pcSnapshotFile == NULL || psProperties->pcSnapshotFile[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing snapshot file.", cRoutine);
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
  const char          cRoutine[] = "CmpModeFinalize()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Establish Log file stream, and update Message Handler.
   *
   *********************************************************************
   */
  strncpy(psProperties->cLogFileName, "stderr", FTIMES_MAX_PATH);
  psProperties->pFileLog = stderr;
   
  MessageSetNewLine(psProperties->cNewLine);
  MessageSetOutputStream(psProperties->pFileLog);
    
  /*-
   *******************************************************************
   *
   * Establish Out file stream.
   *
   *******************************************************************
   */
  strncpy(psProperties->cOutFileName, "stdout", FTIMES_MAX_PATH);
  psProperties->pFileOut = stdout;

  CompareSetNewLine(psProperties->cNewLine);
  CompareSetOutputStream(psProperties->pFileOut);

  /*-
   *********************************************************************
   *
   * Write out a Compare header record.
   *
   *********************************************************************
   */
  iError = CompareWriteHeader(psProperties->pFileOut, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Compare Header: %s", cRoutine, cLocalError);
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
  const char          cRoutine[] = "CmpModeWorkHorse()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  iError = CompareCreateDatabase(psProperties->pcBaselineFile, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }

  iError = CompareEnumerateChanges(psProperties->pcSnapshotFile, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  char                cMessage[MESSAGE_SIZE];

  CompareFreeDatabase();

  /*-
   *********************************************************************
   *
   * Print output filenames.
   *
   *********************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "LogFileName=%s", psProperties->cLogFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "OutFileName=%s", psProperties->cOutFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "DataType=%s", psProperties->cDataType);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "RecordsAnalyzed=%d", CompareGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "ChangedCount=%d", CompareGetChangedCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "MissingCount=%d", CompareGetMissingCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "NewCount=%d", CompareGetNewCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "UnknownCount=%d", CompareGetUnknownCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

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
  return ER_OK;
}
