/*
 ***********************************************************************
 *
 * $Id: putmode.c,v 1.1.1.1 2002/01/18 03:16:53 mavrik Exp $
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
 * PutModeProcessArguments
 *
 ***********************************************************************
 */
int
PutModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "PutModeProcessArguments()";
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
 * PutModeInitialize
 *
 ***********************************************************************
 */
int
PutModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "PutModeInitialize()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  /*-
   *******************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->bURLPutSnapshot = TRUE;
  psProperties->cOutFileHash[0] = 0;
  psProperties->cRunDateTime[0] = 0;
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
 * PutModeCheckDependencies
 *
 ***********************************************************************
 */
int
PutModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "PutModeCheckDependencies()";
#ifdef USE_SSL
  char                cLocalError[ERRBUF_SIZE];
#endif

  if (psProperties->cBaseName[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing BaseName.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->ptPutURL == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing URLPutURL.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->ptPutURL->pcPath[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing path in URLPutURL.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cLogFileName[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing LogFileName.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cOutFileName[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing OutFileName.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cOutFileHash[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing OutFileHash.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cDataType[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing DataType.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cMaskString[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing FieldMask.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->cRunDateTime[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing DateTime.", cRoutine);
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
   * overwrite cLogFileName b/c it's in use.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;
   
  MessageSetNewLine(psProperties->cNewLine);
  MessageSetOutputStream(psProperties->pFileLog);
    
  /*-
   *******************************************************************
   *
   * Establish Out file stream. Don't overwrite cOutFileName b/c it's
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
  const char          cRoutine[] = "PutModeWorkHorse()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Check the server uplink.
   *
   *********************************************************************
   */
  iError = URLPingRequest(psProperties, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER_URLPingRequest;
  }

  iError = URLPutRequest(psProperties, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
