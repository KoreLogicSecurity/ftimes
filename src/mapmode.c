/*-
 ***********************************************************************
 *
 * $Id: mapmode.c,v 1.9 2003/08/13 21:39:49 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2003 Klayton Monroe, Cable & Wireless
 * All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * MapModeProcessArguments
 *
 ***********************************************************************
 */
int
MapModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          cRoutine[] = "MapModeProcessArguments()";
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
    if (psProperties->iRunMode == FTIMES_MAPAUTO)
    {
      iLength = strlen(ppcArgumentVector[0]);
      if (iLength > ALL_FIELDS_MASK_SIZE - 1)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Mask = [%s], Length = [%d]: Length exceeds %d bytes.", cRoutine, ppcArgumentVector[0], iLength, ALL_FIELDS_MASK_SIZE - 1);
        return ER_Length;
      }
      iError = CompareParseStringMask(ppcArgumentVector[0], &psProperties->ulFieldMask, psProperties->iRunMode, psProperties->ptMaskTable, psProperties->iMaskTableLength, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
        return iError;
      }
      strncpy(psProperties->cMaskString, ppcArgumentVector[0], ALL_FIELDS_MASK_SIZE);
    }
    else
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
 * MapModeInitialize
 *
 ***********************************************************************
 */
int
MapModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "MapModeInitialize()";
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
  if (psProperties->iRunMode == FTIMES_MAPAUTO)
  {
    psProperties->bMapRemoteFiles = TRUE;
  }

  /*-
   *******************************************************************
   *
   * Read the config file, if in map{full,lean} mode.
   *
   *******************************************************************
   */
  if (psProperties->iRunMode == FTIMES_MAPFULL || psProperties->iRunMode == FTIMES_MAPLEAN)
  {
    iError = PropertiesReadFile(psProperties->cConfigFile, psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return iError;
    }
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

  /*-
   *********************************************************************
   *
   * Set up the DevelopOutput function pointer.
   *
   *********************************************************************
   */
  psProperties->piDevelopMapOutput = (psProperties->bCompress) ? DevelopCompressedOutput : DevelopNormalOutput;

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapModeCheckDependencies
 *
 ***********************************************************************
 */
int
MapModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "MapModeCheckDependencies()";
#ifdef USE_SSL
  char                cLocalError[ERRBUF_SIZE];
#endif

  if (psProperties->cMaskString[0] == 0)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Missing FieldMask.", cRoutine);
    return ER_MissingControl;
  }

  if (psProperties->iRunMode == FTIMES_MAPFULL)
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
  else if (psProperties->iRunMode == FTIMES_MAPLEAN)
  {
    if (psProperties->cBaseName[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing BaseName.", cRoutine);
      return ER_MissingControl;
    }

    if (psProperties->cOutDirName[0] == 0 && strcmp(psProperties->cBaseName, "-") != 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing OutDir.", cRoutine);
      return ER_MissingControl;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapModeFinalize
 *
 ***********************************************************************
 */
int
MapModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "MapModeFinalize()";
  char                cLocalError[ERRBUF_SIZE];
#ifdef USE_XMAGIC
  char                cMessage[MESSAGE_SIZE];
#endif
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
  if (psProperties->bURLPutSnapshot && psProperties->iRunMode == FTIMES_MAPFULL)
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
   * Conditionally set up the Digest engine.
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MD5_SET) == MD5_SET)
  {
    AnalyzeEnableDigestEngine(psProperties);
  }

#ifdef USE_XMAGIC
  /*-
   *********************************************************************
   *
   * Conditionally set up the Magic engine.
   *
   *********************************************************************
   */
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    iError = AnalyzeEnableXMagicEngine(psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return iError;
    }
  }
#endif

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
  if (psProperties->iRunMode == FTIMES_MAPFULL || (psProperties->iRunMode == FTIMES_MAPLEAN && strcmp(psProperties->cBaseName, "-") != 0))
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
  if (psProperties->iRunMode == FTIMES_MAPFULL || (psProperties->iRunMode == FTIMES_MAPLEAN && strcmp(psProperties->cBaseName, "-") != 0))
  {
    iError = SupportMakeName(psProperties->cOutDirName, psProperties->cBaseName, psProperties->cDateTime, ".map", psProperties->cOutFileName, cLocalError);
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

  /*-
   *********************************************************************
   *
   * Write out a header record.
   *
   *********************************************************************
   */
  iError = MapWriteHeader(psProperties, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: Map Header: %s", cRoutine, cLocalError);
    return iError;
  }

  /*-
   *******************************************************************
   *
   * Establish an output config file name, if necessary.
   *
   *******************************************************************
   */
  if (psProperties->iRunMode == FTIMES_MAPFULL && psProperties->bURLPutSnapshot && psProperties->bURLCreateConfig)
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

  /*-
   *********************************************************************
   *
   * Print input filenames.
   *
   *********************************************************************
   */
#ifdef USE_XMAGIC
  if ((psProperties->ulFieldMask & MAGIC_SET) == MAGIC_SET)
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_MagicFile, psProperties->cMagicFileName);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

    snprintf(cMessage, MESSAGE_SIZE, "MagicHash=%s", psProperties->cMagicHash);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapModeWorkHorse
 *
 ***********************************************************************
 */
int
MapModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
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
 * MapModeFinishUp
 *
 ***********************************************************************
 */
int
MapModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                cLocalError[ERRBUF_SIZE];
  char                cMessage[MESSAGE_SIZE];
  int                 i;
  int                 iFirst;
  int                 iIndex;
  unsigned char       ucFileHash[MD5_HASH_SIZE];

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

  memset(ucFileHash, 0, MD5_HASH_SIZE);
  MD5Omega(&psProperties->sOutFileHashContext, ucFileHash);
  for (i = 0; i < MD5_HASH_SIZE; i++)
  {
    sprintf(&psProperties->cOutFileHash[i * 2], "%02x", ucFileHash[i]);
  }
  psProperties->cOutFileHash[FTIMEX_MAX_MD5_LENGTH - 1] = 0;

  /*-
   *********************************************************************
   *
   * Write out an upload config file, if requested. Don't stop on error.
   *
   *********************************************************************
   */
  if (psProperties->bURLCreateConfig && psProperties->iRunMode == FTIMES_MAPFULL)
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

  if (psProperties->iRunMode == FTIMES_MAPFULL && psProperties->bURLPutSnapshot && psProperties->bURLCreateConfig)
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

#ifdef WINNT
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

#ifdef WIN32
    snprintf(cMessage, MESSAGE_SIZE, "BytesAnalyzed=%I64u", AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
#endif
  }
  else
  {
    snprintf(cMessage, MESSAGE_SIZE, "AnalysisStages=None");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);
  }

  snprintf(cMessage, MESSAGE_SIZE, "CompleteRecords=%d", MapGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "IncompleteRecords=%d", MapGetIncompleteRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_MODEDATA_STRING, cMessage);

  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MapModeFinalStage
 *
 ***********************************************************************
 */
int
MapModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "MapModeFinalStage()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  /*-
   *********************************************************************
   *
   * Conditionally upload the collected data.
   *
   *********************************************************************
   */
  if (psProperties->bURLPutSnapshot && psProperties->iRunMode == FTIMES_MAPFULL)
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
