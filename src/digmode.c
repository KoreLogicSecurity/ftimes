/*
 ***********************************************************************
 *
 * $Id: digmode.c,v 1.3 2002/01/29 15:20:05 mavrik Exp $
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
 * DigModeProcessArguments
 *
 ***********************************************************************
 */
int
DigModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "DigModeProcessArguments()";
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
    if (strcmp(ppcArgumentVector[0], "-") == 0)
    {
      strcpy(psProperties->cConfigFile, "-");
    }
    else
    {
      iError = SupportExpandPath(ppcArgumentVector[0], psProperties->cConfigFile, FTIMES_MAX_PATH, 1, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        return iError;
      }
      iError = SupportAddToList(psProperties->cConfigFile, &psProperties->ptExcludeList, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        return iError;
      }
    }
    if (iArgumentCount >= 2)
    {
      if (strcmp(ppcArgumentVector[1], "-l") == 0)
      {
        if (iArgumentCount >= 3)
        {
          iError = SupportSetLogLevel(ppcArgumentVector[2], &psProperties->iLogLevel, cLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, ERRBUF_SIZE, "%s: Level = [%s]: %s", cRoutine, ppcArgumentVector[2], cLocalError);
            return iError;
          }
          if (iArgumentCount >= 4)
          {
            psProperties->ppcMapList = &ppcArgumentVector[3];
          }
        }
        else
        {
          return ER_Usage;
        }
      }
      else
      {
        psProperties->ppcMapList = &ppcArgumentVector[1];
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
 * DigModeInitialize
 *
 ***********************************************************************
 */
int
DigModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "DigModeInitialize()";
  char                cLocalError[ERRBUF_SIZE],
                      cMapItem[FTIMES_MAX_PATH];
  int                 i,
                      iError;

  cLocalError[0] = 0;

  /*-
   *******************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;
  psProperties->pFileOut = stdout;
  if (psProperties->iRunMode == FTIMES_DIGAUTO)
  {
    psProperties->bMapRemoteFiles = TRUE;
  }
  psProperties->ulFieldMask = ~0;
  strcpy(psProperties->cMaskString, "all"); /* Required by nph-ftimes.cgi */

  /*-
   *******************************************************************
   *
   * Parse the config/strings file.
   *
   *******************************************************************
   */
  iError = PropertiesReadFile(psProperties->cConfigFile, psProperties, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return iError;
  }
  
  /*-
   *******************************************************************
   *
   * Add any command line items to the Include list.
   *
   *******************************************************************
   */
  for (i = 0; psProperties->ppcMapList != NULL && psProperties->ppcMapList[i] != NULL; i++)
  {
    iError = SupportExpandPath(psProperties->ppcMapList[i], cMapItem, FTIMES_MAX_PATH, 0, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return iError;
    }

    iError = SupportAddToList(cMapItem, &psProperties->ptIncludeList, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Item = [%s]: %s", cRoutine, cMapItem, cLocalError);
      return iError;
    }
  }

  /*-
   *********************************************************************
   *
   * LogDir defaults to OutDir.
   *
   *********************************************************************
   */
  if (psProperties->cLogDirName[0] == 0)
  {
    snprintf(psProperties->cLogDirName, FTIMES_MAX_PATH, "%s", psProperties->cOutDirName);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigModeCheckDependencies
 *
 ***********************************************************************
 */
int
DigModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "DigModeCheckDependencies()";
#ifdef USE_SSL
  char                cLocalError[ERRBUF_SIZE];
#endif

  if (DigGetStringCount() <= 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Need at least one DigString.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->iRunMode == FTIMES_DIGFULL)
  {
    if (psProperties->cBaseName[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing BaseName.", cRoutine);
      return ER_MissingControl;
    }

    if (psProperties->cOutDirName[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing OutDir.", cRoutine);
      return ER_MissingControl;
    }

    if (psProperties->bURLPutSnapshot && psProperties->ptPutURL == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing URLPutURL.", cRoutine);
      return ER_MissingControl;
    }

    if (psProperties->bURLPutSnapshot && psProperties->ptPutURL->pcPath[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing path in URLPutURL.", cRoutine);
      return ER_MissingControl;
    }

#ifdef USE_SSL
    if (SSLCheckDependencies(psProperties->psSSLProperties, cLocalError) != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER_MissingControl;
    }
#endif
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigModeFinalize
 *
 ***********************************************************************
 */
int
DigModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "DigModeFinalize()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Enforce privilege requirements, if requested.
   *
   *********************************************************************
   */
  if (psProperties->bRequirePrivilege)
  {
    iError = SupportRequirePrivilege(cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return iError;
    }
  }

  /*-
   *********************************************************************
   *
   * Conditionally check the server uplink.
   *
   *********************************************************************
   */
  if (psProperties->bURLPutSnapshot)
  {
    iError = URLPingRequest(psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER_URLPingRequest;
    }
  }

  /*-
   *********************************************************************
   *
   * Set up the Dig engine.
   *
   *********************************************************************
   */
  AnalyzeEnableDigEngine(psProperties);

  /*-
   *********************************************************************
   *
   * If the Include list is NULL, include everything.
   *
   *********************************************************************
   */
  if (psProperties->ptIncludeList == NULL)
  {
    psProperties->ptIncludeList = SupportIncludeEverything(psProperties->bMapRemoteFiles, cLocalError);
    if (psProperties->ptIncludeList == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Failed to build a default Include list: %s", cRoutine, cLocalError);
      return ER_BadHandle;
    }
  }

  /*-
   *********************************************************************
   *
   * Prune the Include and Exclude lists.
   *
   *********************************************************************
   */
  psProperties->ptExcludeList = SupportPruneList(psProperties->ptExcludeList, psProperties->bMapRemoteFiles);

  psProperties->ptIncludeList = SupportPruneList(psProperties->ptIncludeList, psProperties->bMapRemoteFiles);
  if (psProperties->ptIncludeList == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: There's nothing left to process in the Include list.", cRoutine);
    return ER_NothingToDo;
  }

  /*-
   *********************************************************************
   *
   * Establish Log file stream, and update Message Handler.
   *
   *********************************************************************
   */
  if (psProperties->iRunMode == FTIMES_DIGFULL)
  {
    iError = SupportMakeName(psProperties->cLogDirName, psProperties->cBaseName, psProperties->cDateTime, ".log", psProperties->cLogFileName, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Log File: %s", cRoutine, cLocalError);
      return iError;
    }

    iError = SupportAddToList(psProperties->cLogFileName, &psProperties->ptExcludeList, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: LogFile = [%s]: %s", cRoutine, psProperties->cLogFileName, cLocalError);
      return iError;
    }

    if ((psProperties->pFileLog = fopen(psProperties->cLogFileName, "wb+")) == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: LogFile = [%s]: %s", cRoutine, psProperties->cLogFileName, strerror(errno));
      return ER_fopen;
    }
  }
  else
  {
    strncpy(psProperties->cLogFileName, "stderr", FTIMES_MAX_PATH);
    psProperties->pFileLog = stderr;
  }

  MessageSetNewLine(psProperties->cNewLine);
  MessageSetOutputStream(psProperties->pFileLog);

  /*-
   *******************************************************************
   *
   * Establish Out file stream.
   *
   *******************************************************************
   */
  if (psProperties->iRunMode == FTIMES_DIGFULL)
  {
    iError = SupportMakeName(psProperties->cOutDirName, psProperties->cBaseName, psProperties->cDateTime, ".dig", psProperties->cOutFileName, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Out File: %s", cRoutine, cLocalError);
      return iError;
    }

    iError = SupportAddToList(psProperties->cOutFileName, &psProperties->ptExcludeList, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: OutFile = [%s]: %s", cRoutine, psProperties->cOutFileName, cLocalError);
      return iError;
    }

    if ((psProperties->pFileOut = fopen(psProperties->cOutFileName, "wb+")) == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: OutFile = [%s]: %s", cRoutine, psProperties->cOutFileName, strerror(errno));
      return ER_fopen;
    }
  }
  else
  {
    strncpy(psProperties->cOutFileName, "stdout", FTIMES_MAX_PATH);
    psProperties->pFileOut = stdout;
  }
  DigSetOutputStream(psProperties->pFileOut);

  /*-
   *********************************************************************
   *
   * Write out a Dig header record.
   *
   *********************************************************************
   */
  DigSetNewLine(psProperties->cNewLine);
  DigSetHashBlock(&psProperties->sOutFileHashContext);
  iError = DigWriteHeader(psProperties->pFileOut, psProperties->cNewLine, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Dig Header: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *******************************************************************
   *
   * Establish an output config file name, if necessary.
   *
   *******************************************************************
   */
  if (psProperties->iRunMode == FTIMES_DIGFULL && psProperties->bURLPutSnapshot && psProperties->bURLCreateConfig)
  {
    iError = SupportMakeName(psProperties->cOutDirName, psProperties->cBaseName, psProperties->cDateTime, ".cfg", psProperties->cCfgFileName, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Cfg File: %s", cRoutine, cLocalError);
      return iError;
    }
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
 * DigModeWorkHorse
 *
 ***********************************************************************
 */
int
DigModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                cLocalError[ERRBUF_SIZE];
  FILE_LIST           *pList;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Process everything in the Include list.
   *
   *********************************************************************
   */
  for (pList = psProperties->ptIncludeList; pList != NULL; pList = pList->pNext)
  {
    if (SupportMatchExclude(psProperties->ptExcludeList, pList->cPath) == NULL)
    {
      MapFile(psProperties, pList->cPath, cLocalError);
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigModeFinishUp
 *
 ***********************************************************************
 */
int
DigModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                cLocalError[ERRBUF_SIZE];
  char                cMessage[MESSAGE_SIZE];
  int                 i;
  int                 iFirst;
  int                 iIndex;
  unsigned char       ucFileHash[MD5_HASH_LENGTH];

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Close up the output stream, and complete the file digest.
   *
   *********************************************************************
   */
  if (psProperties->pFileOut && psProperties->pFileOut != stdout)
  {
    fflush(psProperties->pFileOut);
    fclose(psProperties->pFileOut);
    psProperties->pFileOut = NULL;
  }

  memset(ucFileHash, 0, MD5_HASH_LENGTH);
  md5_end(&psProperties->sOutFileHashContext, ucFileHash);
  for (i = 0; i < MD5_HASH_LENGTH; i++)
  {
    sprintf(&psProperties->cOutFileHash[i * 2], "%02x", ucFileHash[i]);
  }
  psProperties->cOutFileHash[MD5_HASH_STRING_LENGTH - 1] = 0;

  /*-
   *********************************************************************
   *
   * Write out an upload config file, if requested. Don't stop on error.
   *
   *********************************************************************
   */
  if (psProperties->bURLCreateConfig && psProperties->iRunMode == FTIMES_DIGFULL)
  {
    FTimesCreateConfigFile(psProperties, cLocalError);
  }

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

  if (psProperties->iRunMode == FTIMES_DIGFULL && psProperties->bURLPutSnapshot && psProperties->bURLCreateConfig)
  {
    snprintf(cMessage, MESSAGE_SIZE, "CfgFileName=%s", psProperties->cCfgFileName);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  }

  snprintf(cMessage, MESSAGE_SIZE, "OutFileHash=%s", psProperties->cOutFileHash);
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
  snprintf(cMessage, MESSAGE_SIZE, "DirectoriesEncountered=%d", MapGetDirectoryCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "FilesEncountered=%d", MapGetFileCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

#ifdef UNIX
  snprintf(cMessage, MESSAGE_SIZE, "SpecialsEncountered=%d", MapGetSpecialCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#endif

#ifdef FTimes_WINNT
  snprintf(cMessage, MESSAGE_SIZE, "StreamsEncountered=%d", MapGetStreamCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#endif

  iIndex = 0;
  if (psProperties->iLastAnalysisStage > 0)
  {
    iIndex = sprintf(&cMessage[iIndex], "AnalysisStages=");
    for (i = 0, iFirst = 0; i < psProperties->iLastAnalysisStage; i++)
    {
        iIndex += sprintf(&cMessage[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", psProperties->sAnalysisStages[i].cDescription);
    }  
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  
    snprintf(cMessage, MESSAGE_SIZE, "ObjectsAnalyzed=%lu", AnalyzeGetFileCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
    snprintf(cMessage, MESSAGE_SIZE, "BytesAnalyzed=%qu", AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#else
    sprintf(cMessage, "BytesAnalyzed=%qu", AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#endif
#endif

#ifdef FTimes_WIN32
    snprintf(cMessage, MESSAGE_SIZE, "BytesAnalyzed=%I64u", AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#endif
  }
  else
  {
    snprintf(cMessage, MESSAGE_SIZE, "AnalysisStages=None");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  }

  /*-
   *********************************************************************
   *
   * List total number of strings.
   *
   *********************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "DigStrings=%d", DigGetStringCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  /*-
   *********************************************************************
   *
   * List total number of strings matched.
   *
   *********************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "DigStringsMatched=%d", DigGetStringsMatched());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  /*-
   *********************************************************************
   *
   * List total number of matches.
   *
   *********************************************************************
   */
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
  /*-
   *******************************************************************
   *
   * Support certain versions of Solaris that can't grok %q.
   *
   *******************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "TotalMatches=%qu", DigGetTotalMatches());
#else
  /*-
   *******************************************************************
   *
   * Oddly, some versions of UNIX don't support %q in snprintf. So,
   * we print it out with sprintf instead.
   *
   *******************************************************************
   */
  sprintf(cMessage, "TotalMatches=%qu", DigGetTotalMatches());
#endif
#endif
#ifdef FTimes_WIN32
  snprintf(cMessage, MESSAGE_SIZE, "MatchCount=%I64u", DigGetTotalMatches());
#endif
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * DigModeFinalStage
 *
 ***********************************************************************
 */
int
DigModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "DigModeFinalStage()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  /*-
   *********************************************************************
   *
   * Conditionally upload the snapshot, and conditionally erase files.
   *
   *********************************************************************
   */
  if (psProperties->bURLPutSnapshot && psProperties->iRunMode == FTIMES_DIGFULL)
  {
    iError = URLPutRequest(psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER_URLPutRequest;
    }

    if (psProperties->bURLUnlinkOutput)
    {
      FTimesEraseFiles(psProperties, pcError);
    }
  }

  return ER_OK;
}
