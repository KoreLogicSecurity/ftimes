/*
 ***********************************************************************
 *
 * $Id: decoder.c,v 1.1.1.1 2002/01/18 03:17:31 mavrik Exp $
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
 * DecoderProcessArguments
 *
 ***********************************************************************
 */
int
DecoderProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "DecoderProcessArguments()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

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
        iError = SupportSetLogLevel(ppcArgumentVector[2], &psProperties->iLogLevel, cLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, ERRBUF_SIZE, "%s: Level = [%s]: %s", cRoutine, ppcArgumentVector[2], cLocalError);
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
  const char          cRoutine[] = "DecoderCheckDependencies()";

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
 * DecoderFinalize
 *
 ***********************************************************************
 */
int
DecoderFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                cLocalError[ERRBUF_SIZE];

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
  const char          cRoutine[] = "DecoderWorkHorse()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  iError = DecodeFile(psProperties->pcSnapshotFile, psProperties->pFileOut, psProperties->cNewLine, cLocalError);
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
 * DecoderFinishUp
 *
 ***********************************************************************
 */
int
DecoderFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                cMessage[MESSAGE_SIZE];

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

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "RecordsDecoded=%lu", DecodeGetRecordsDecoded());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "RecordsLost=%lu", DecodeGetRecordsLost());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

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
