/*-
 ***********************************************************************
 *
 * $Id: getmode.c,v 1.29 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

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
  char                acLocalError[MESSAGE_SIZE] = "";
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
  char                acLocalError[MESSAGE_SIZE] = "";
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
  if (SSLCheckDependencies(psProperties->psSslProperties, acLocalError) != ER_OK)
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
  char                acLocalError[MESSAGE_SIZE] = "";
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
  char                acMode[6]; /* strlen("--map") + 1 */
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
    case FTIMES_MAPMODE:
      strcpy(acMode, "--map");
      break;
    case FTIMES_DIGMODE:
      strcpy(acMode, "--dig");
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
