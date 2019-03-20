/*-
 ***********************************************************************
 *
 * $Id: putmode.c,v 1.7 2004/04/22 02:19:10 mavrik Exp $
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
 * PutModeProcessArguments
 *
 ***********************************************************************
 */
int
PutModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "PutModeProcessArguments()";
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
  if (iArgumentCount >= 1)
  {
    iLength = strlen(ppcArgumentVector[0]);
    if (iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", acRoutine, ppcArgumentVector[0], iLength, FTIMES_MAX_PATH - 1);
      return ER_Length;
    }
    strncpy(psProperties->acConfigFile, ppcArgumentVector[0], FTIMES_MAX_PATH);
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
 * PutModeInitialize
 *
 ***********************************************************************
 */
int
PutModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "PutModeInitialize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *******************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->bURLPutSnapshot = TRUE;
  psProperties->acOutFileHash[0] = 0;
  psProperties->acRunDateTime[0] = 0;
  psProperties->pFileLog = stderr;
  psProperties->pFileOut = stdout;

  /*-
   *******************************************************************
   *
   * Read the config file.
   *
   *******************************************************************
   */
  iError = PropertiesReadFile(psProperties->acConfigFile, psProperties, acLocalError);
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
 * PutModeCheckDependencies
 *
 ***********************************************************************
 */
int
PutModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "PutModeCheckDependencies()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif

  if (psProperties->acBaseName[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing BaseName.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psPutURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing URLPutURL.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psPutURL->pcPath[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing path in URLPutURL.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acLogFileName[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing LogFileName.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acOutFileName[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing OutFileName.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acOutFileHash[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing OutFileHash.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acDataType[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing DataType.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acMaskString[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing FieldMask.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->acRunDateTime[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing DateTime.", acRoutine);
    return ER_MissingControl;
  }

#ifdef USE_SSL
  if (SSLCheckDependencies(psProperties->psSSLProperties, acLocalError) != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER_MissingControl;
  }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PutModeFinalize
 *
 ***********************************************************************
 */
int
PutModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  /*-
   *********************************************************************
   *
   * Establish Log file stream, and update Message Handler. Don't
   * overwrite acLogFileName b/c it's in use.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;

  MessageSetNewLine(psProperties->acNewLine);
  MessageSetOutputStream(psProperties->pFileLog);

  /*-
   *******************************************************************
   *
   * Establish Out file stream. Don't overwrite acOutFileName b/c it's
   * in use.
   *
   *******************************************************************
   */
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
 * PutModeWorkHorse
 *
 ***********************************************************************
 */
int
PutModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "PutModeWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *********************************************************************
   *
   * Check the server uplink.
   *
   *********************************************************************
   */
  iError = URLPingRequest(psProperties, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER_URLPingRequest;
  }

  iError = URLPutRequest(psProperties, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER_URLPutRequest;
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PutModeFinishUp
 *
 ***********************************************************************
 */
int
PutModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  /*-
   *********************************************************************
   *
   * Print output filenames.
   *
   *********************************************************************
   */
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, "LogFileName=stderr");

  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, "OutFileName=stdout");

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PutModeFinalStage
 *
 ***********************************************************************
 */
int
PutModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  return ER_OK;
}
