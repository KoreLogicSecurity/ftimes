/*-
 ***********************************************************************
 *
 * $Id: decoder.c,v 1.15 2007/02/23 00:22:35 mavrik Exp $
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
 * DecoderProcessArguments
 *
 ***********************************************************************
 */
int
DecoderProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "DecoderProcessArguments()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *********************************************************************
   *
   * Process arguments.
   *
   *********************************************************************
   */
  if (iArgumentCount >= 1)
  {
    psProperties->psSnapshotContext->pcFile = ppcArgumentVector[0];
    if (iArgumentCount >= 2)
    {
      if (iArgumentCount == 3 && strcmp(ppcArgumentVector[1], "-l") == 0)
      {
        iError = SupportSetLogLevel(ppcArgumentVector[2], &psProperties->iLogLevel, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: Level = [%s]: %s", acRoutine, ppcArgumentVector[2], acLocalError);
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
 * DecoderInitialize
 *
 ***********************************************************************
 */
int
DecoderInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
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
DecoderCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderCheckDependencies()";

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
DecoderFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderFinalize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acMessage[MESSAGE_SIZE] = { 0 };
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
DecoderWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
DecoderFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
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
DecoderFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  return ER_OK;
}
