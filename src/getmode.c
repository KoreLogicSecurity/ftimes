/*-
 ***********************************************************************
 *
 * $Id: getmode.c,v 1.16 2007/02/23 00:22:35 mavrik Exp $
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
 * GetModeProcessArguments
 *
 ***********************************************************************
 */
int
GetModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "GetModeProcessArguments()";
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
 * GetModeInitialize
 *
 ***********************************************************************
 */
int
GetModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "GetModeInitialize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *******************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
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
 * GetModeCheckDependencies
 *
 ***********************************************************************
 */
int
GetModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "GetModeCheckDependencies()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif

  if (psProperties->acBaseName[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing BaseName.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->bGetAndExec)
  {
    if (psProperties->acGetFileName[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing GetFileName.", acRoutine);
      return ER_MissingControl;
    }
  }

  if (psProperties->acURLGetRequest[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing URLGetRequest.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psGetURL == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing URLGetURL.", acRoutine);
    return ER_MissingControl;
  }

  if (psProperties->psGetURL->pcPath[0] == 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing path in URLGetURL.", acRoutine);
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
 * GetModeFinalize
 *
 ***********************************************************************
 */
int
GetModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "GetModeFinalize()";

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
  if (psProperties->acGetFileName[0])
  {
    if ((psProperties->pFileOut = fopen(psProperties->acGetFileName, "wb")) == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: GetFile = [%s]: %s", acRoutine, psProperties->acGetFileName, strerror(errno));
      return ER_fopen;
    }
    strncpy(psProperties->acOutFileName, psProperties->acGetFileName, FTIMES_MAX_PATH);
  }
  else
  {
    strncpy(psProperties->acOutFileName, "stdout", FTIMES_MAX_PATH);
    psProperties->pFileOut = stdout;
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
 * GetModeWorkHorse
 *
 ***********************************************************************
 */
int
GetModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "GetModeWorkHorse()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  iError = URLGetRequest(psProperties, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER_URLGetRequest;
  }

  if (psProperties->acGetFileName[0])
  {
#ifdef UNIX
    if (psProperties->pFileOut && psProperties->pFileOut != stdout)
    {
      fchmod(fileno(psProperties->pFileOut), 0600);
    }
#endif
    fclose(psProperties->pFileOut);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * GetModeFinishUp
 *
 ***********************************************************************
 */
int
GetModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
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
  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * GetModeFinalStage
 *
 ***********************************************************************
 */
int
GetModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "GetModeFinalStage()";
  char                acMode[10]; /* strlen("--mapmode") + 1 */
  int                 iError;

  /*-
   *********************************************************************
   *
   * Restart FTimes in the new run mode, if requested.
   *
   *********************************************************************
   */
  if (psProperties->bGetAndExec)
  {
    switch (psProperties->iNextRunMode)
    {
    case FTIMES_MAPFULL:
      strcpy(acMode, "--mapfull");
      break;
    case FTIMES_MAPLEAN:
      strcpy(acMode, "--maplean");
      break;
    case FTIMES_DIGFULL:
      strcpy(acMode, "--digfull");
      break;
    case FTIMES_DIGLEAN:
      strcpy(acMode, "--diglean");
      break;
    default:
      snprintf(pcError, MESSAGE_SIZE, "%s: Invalid RunMode. That shouldn't happen.", acRoutine);
      return ER_BadValue;
      break;
    }

    iError = execlp(psProperties->pcProgram, psProperties->pcProgram, acMode, psProperties->acGetFileName, (char *) 0);
    if (iError == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Command = [%s %s %s]: execlp(): %s", acRoutine, psProperties->pcProgram, acMode, psProperties->acGetFileName, strerror(errno));
      return ER_execlp;
    }
  }

  return ER_OK;
}
