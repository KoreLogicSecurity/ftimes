/*-
 ***********************************************************************
 *
 * $Id: decoder.c,v 1.29 2019/04/22 20:04:01 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * DecoderInitialize
 *
 ***********************************************************************
 */
int
DecoderInitialize(void *pvProperties, char *pcError)
{
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;

  /*-
   *********************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;
  psProperties->pFileOut = stdout;

  DecodeBuildFromBase64Table();

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecoderCheckDependencies
 *
 ***********************************************************************
 */
int
DecoderCheckDependencies(void *pvProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderCheckDependencies()";
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;

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
 * DecoderFinalize
 *
 ***********************************************************************
 */
int
DecoderFinalize(void *pvProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderFinalize()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acMessage[MESSAGE_SIZE] = { 0 };
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;
  int                 iError = 0;

  /*-
   *********************************************************************
   *
   * Finalize the log stream.
   *
   *********************************************************************
   */
  strncpy(psProperties->acLogFileName, "stderr", FTIMES_MAX_PATH);
  psProperties->pFileLog = stderr;
  MessageSetOutputStream(psProperties->pFileLog);
  MessageSetAutoFlush(MESSAGE_AUTO_FLUSH_ON);

  /*-
   *******************************************************************
   *
   * Finalize the out stream.
   *
   *******************************************************************
   */
  strncpy(psProperties->acOutFileName, "stdout", FTIMES_MAX_PATH);
  psProperties->pFileOut = stdout;
  DecodeSetOutputStream(psProperties->pFileOut);

  /*-
   *******************************************************************
   *
   * Finalize the newline string.
   *
   *******************************************************************
   */
  MessageSetNewLine(psProperties->acNewLine);
  DecodeSetNewLine(psProperties->acNewLine);

  /*-
   *******************************************************************
   *
   * Open the snapshot.
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
   *********************************************************************
   *
   * Display properties.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "Snapshot=%s", psProperties->psSnapshotContext->pcFile);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  PropertiesDisplaySettings(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecoderWorkHorse
 *
 ***********************************************************************
 */
int
DecoderWorkHorse(void *pvProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;
  int                 iError = 0;

  /*-
   *******************************************************************
   *
   * Write out a header.
   *
   *******************************************************************
   */
  iError = DecodeWriteHeader(psProperties->psSnapshotContext, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }

  /*-
   *******************************************************************
   *
   * Read the snapshot, and process its data.
   *
   *******************************************************************
   */
  iError = DecodeReadSnapshot(psProperties->psSnapshotContext, acLocalError);
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
 * DecoderFinishUp
 *
 ***********************************************************************
 */
int
DecoderFinishUp(void *pvProperties, char *pcError)
{
  char                acMessage[MESSAGE_SIZE] = { 0 };
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;

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

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "RecordsAnalyzed=%lu", psProperties->psSnapshotContext->sDecodeStats.ulAnalyzed);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "RecordsDecoded=%lu", psProperties->psSnapshotContext->sDecodeStats.ulDecoded);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "RecordsSkipped=%lu", psProperties->psSnapshotContext->sDecodeStats.ulSkipped);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DecoderFinalStage
 *
 ***********************************************************************
 */
int
DecoderFinalStage(void *pvProperties, char *pcError)
{
//FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;

  return ER_OK;
}
