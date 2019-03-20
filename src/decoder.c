/*-
 ***********************************************************************
 *
 * $Id: decoder.c,v 1.9 2005/04/02 18:08:24 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2005 Klayton Monroe, All Rights Reserved.
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
    psProperties->pcSnapshotFile = ppcArgumentVector[0];
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
 * DecoderFinalize
 *
 ***********************************************************************
 */
int
DecoderFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
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
 * DecoderWorkHorse
 *
 ***********************************************************************
 */
int
DecoderWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DecoderWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  iError = DecodeFile(psProperties->pcSnapshotFile, psProperties->pFileOut, psProperties->acNewLine, acLocalError);
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
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "OutFileName=%s", psProperties->acOutFileName);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "RecordsDecoded=%u", DecodeGetRecordsDecoded());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "RecordsLost=%u", DecodeGetRecordsLost());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, acMessage);

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
