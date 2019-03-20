/*
 ***********************************************************************
 *
 * $Id: getmode.c,v 1.3 2003/01/16 21:08:09 mavrik Exp $
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
 * GetModeProcessArguments
 *
 ***********************************************************************
 */
int
GetModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "GetModeProcessArguments()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;
  int                 iLength;

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
    iLength = strlen(ppcArgumentVector[0]);
    if (iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, ppcArgumentVector[0], iLength, FTIMES_MAX_PATH - 1);
      return ER_Length;
    }
    strncpy(psProperties->cConfigFile, ppcArgumentVector[0], FTIMES_MAX_PATH);
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
 * GetModeInitialize
 *
 ***********************************************************************
 */
int
GetModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "GetModeInitialize()";
  char                cLocalError[ERRBUF_SIZE];
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
  iError = PropertiesReadFile(psProperties->cConfigFile, psProperties, cLocalError);
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
 * GetModeCheckDependencies
 *
 ***********************************************************************
 */
int
GetModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "GetModeCheckDependencies()";
#ifdef USE_SSL
  char                cLocalError[ERRBUF_SIZE];
#endif

  if (psProperties->cBaseName[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing BaseName.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->bGetAndExec)
  {
    if (psProperties->cGetFileName[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing GetFileName.", cRoutine);
      return ER_MissingControl;
    }
  }

  if (psProperties->cURLGetRequest[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing URLGetRequest.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->ptGetURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing URLGetURL.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->ptGetURL->pcPath[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing path in URLGetURL.", cRoutine);
    return ER_MissingControl;
  }

#ifdef USE_SSL
  if (SSLCheckDependencies(psProperties->psSSLProperties, cLocalError) != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  const char          cRoutine[] = "GetModeFinalize()";

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
  if (psProperties->cGetFileName[0])
  {
    if ((psProperties->pFileOut = fopen(psProperties->cGetFileName, "wb")) == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: GetFile = [%s]: %s", cRoutine, psProperties->cGetFileName, strerror(errno));
      return ER_fopen;
    }
    strncpy(psProperties->cOutFileName, psProperties->cGetFileName, FTIMES_MAX_PATH);
  }
  else
  {
    strncpy(psProperties->cOutFileName, "stdout", FTIMES_MAX_PATH);
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
  const char          cRoutine[] = "GetModeWorkHorse()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  iError = URLGetRequest(psProperties, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER_URLGetRequest;
  }

  if (psProperties->cGetFileName[0])
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
  const char          cRoutine[] = "GetModeFinalStage()";
  char                cMode[10]; /* strlen("--mapmode") + 1 */
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
      strcpy(cMode, "--mapfull");
      break;
    case FTIMES_DIGFULL:
      strcpy(cMode, "--digfull");
      break;
    default:
      snprintf(pcError, ERRBUF_SIZE, "%s: Invalid RunMode. That shouldn't happen.", cRoutine);
      return ER_BadValue;
      break;
    }

    iError = execlp(psProperties->pcProgram, psProperties->pcProgram, cMode, psProperties->cGetFileName, 0);
    if (iError == ER)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Command = [%s %s %s]: execlp(): %s", cRoutine, psProperties->pcProgram, cMode, psProperties->cGetFileName, strerror(errno));
      return ER_execlp;
    }
  }

  return ER_OK;
}
