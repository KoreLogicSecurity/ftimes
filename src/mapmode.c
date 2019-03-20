/*-
 ***********************************************************************
 *
 * $Id: mapmode.c,v 1.37 2007/02/23 00:22:35 mavrik Exp $
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
 * MapModeProcessArguments
 *
 ***********************************************************************
 */
int
MapModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "MapModeProcessArguments()";
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
    if (psProperties->iRunMode == FTIMES_MAPAUTO)
    {
      psProperties->psFieldMask = MaskParseMask(ppcArgumentVector[0], MASK_RUNMODE_TYPE_MAP, acLocalError);
      if (psProperties->psFieldMask == NULL)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return ER;
      }
    }
    else
    {
      if (strcmp(ppcArgumentVector[0], "-") == 0)
      {
        strcpy(psProperties->acConfigFile, "-");
      }
      else
      {
        iError = SupportExpandPath(ppcArgumentVector[0], psProperties->acConfigFile, FTIMES_MAX_PATH, 1, acLocalError);
        if (iError != ER_OK)
        {
          snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
          iError = SupportSetLogLevel(ppcArgumentVector[2], &psProperties->iLogLevel, acLocalError);
          if (iError != ER_OK)
          {
            snprintf(pcError, MESSAGE_SIZE, "%s: Level = [%s]: %s", acRoutine, ppcArgumentVector[2], acLocalError);
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
  const char          acRoutine[] = "MapModeInitialize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acMapItem[FTIMES_MAX_PATH];
  int                 i;
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
  if (psProperties->iRunMode == FTIMES_MAPAUTO)
  {
    psProperties->bAnalyzeDeviceFiles = TRUE;
    psProperties->bAnalyzeRemoteFiles = TRUE;
  }
  psProperties->bHashSymbolicLinks = TRUE;

  /*-
   *******************************************************************
   *
   * Read the config file, if in map{full,lean} mode.
   *
   *******************************************************************
   */
  if (psProperties->iRunMode == FTIMES_MAPFULL || psProperties->iRunMode == FTIMES_MAPLEAN)
  {
    iError = PropertiesReadFile(psProperties->acConfigFile, psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    iError = SupportExpandPath(psProperties->ppcMapList[i], acMapItem, FTIMES_MAX_PATH, 0, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }

    iError = SupportAddToList(acMapItem, &psProperties->psIncludeList, "Include", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
  if (psProperties->acLogDirName[0] == 0)
  {
    snprintf(psProperties->acLogDirName, FTIMES_MAX_PATH, "%s", psProperties->acOutDirName);
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
  const char          acRoutine[] = "MapModeCheckDependencies()";
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif

  if (psProperties->psFieldMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing FieldMask.", acRoutine);
    return ER_MissingControl;
  }

#ifdef USE_XMAGIC
  /*-
   *********************************************************************
   *
   * Conditionally check that the block size is big enough to support
   * Magic.
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    if (psProperties->iAnalyzeBlockSize < XMAGIC_READ_BUFSIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: AnalyzeBlockSize (%d) must be at least %d bytes to support XMagic.", acRoutine, psProperties->iAnalyzeBlockSize, XMAGIC_READ_BUFSIZE);
      return ER;
    }
  }
#endif

  if (psProperties->iRunMode == FTIMES_MAPFULL)
  {
    if (psProperties->acBaseName[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing BaseName.", acRoutine);
      return ER_MissingControl;
    }

    if (psProperties->acOutDirName[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing OutDir.", acRoutine);
      return ER_MissingControl;
    }

    if (psProperties->bURLPutSnapshot && psProperties->psPutURL == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing URLPutURL.", acRoutine);
      return ER_MissingControl;
    }

    if (psProperties->bURLPutSnapshot && psProperties->psPutURL->pcPath[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing path in URLPutURL.", acRoutine);
      return ER_MissingControl;
    }

#ifdef USE_SSL
    if (SSLCheckDependencies(psProperties->psSSLProperties, acLocalError) != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_MissingControl;
    }
#endif
  }
  else if (psProperties->iRunMode == FTIMES_MAPLEAN)
  {
    if (psProperties->acBaseName[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing BaseName.", acRoutine);
      return ER_MissingControl;
    }

    if (psProperties->acOutDirName[0] == 0 && strcmp(psProperties->acBaseName, "-") != 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing OutDir.", acRoutine);
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
  const char          acRoutine[] = "MapModeFinalize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#ifdef USE_XMAGIC
  char                acMessage[MESSAGE_SIZE];
#endif
  int                 iError;

  /*-
   *********************************************************************
   *
   * Enforce privilege requirements, if requested.
   *
   *********************************************************************
   */
  if (psProperties->bRequirePrivilege)
  {
    iError = SupportRequirePrivilege(acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
    iError = URLPingRequest(psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_URLPingRequest;
    }
  }

  /*-
   *********************************************************************
   *
   * Conditionally set up the Digest engine. NOTE: The conditionals are
   * tested inside the subroutine.
   *
   *********************************************************************
   */
  AnalyzeEnableDigestEngine(psProperties);

#ifdef USE_XMAGIC
  /*-
   *********************************************************************
   *
   * Conditionally set up the Magic engine.
   *
   *********************************************************************
   */
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    iError = AnalyzeEnableXMagicEngine(psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
  if (psProperties->psIncludeList == NULL)
  {
    psProperties->psIncludeList = SupportIncludeEverything(acLocalError);
    if (psProperties->psIncludeList == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Failed to build a default Include list: %s", acRoutine, acLocalError);
      return ER_BadHandle;
    }
  }

  /*-
   *********************************************************************
   *
   * Prune the Include list.
   *
   *********************************************************************
   */
  psProperties->psIncludeList = SupportPruneList(psProperties->psIncludeList, "Include");
  if (psProperties->psIncludeList == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: There's nothing left to process in the Include list.", acRoutine);
    return ER_NothingToDo;
  }

  /*-
   *********************************************************************
   *
   * Conditionally check the Include and Exclude lists.
   *
   *********************************************************************
   */
  if (psProperties->bExcludesMustExist)
  {
    iError = SupportCheckList(psProperties->psExcludeList, "Exclude", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }
  }

  if (psProperties->bIncludesMustExist)
  {
    iError = SupportCheckList(psProperties->psIncludeList, "Include", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }
  }

  /*-
   *********************************************************************
   *
   * Establish Log file stream, and update Message Handler.
   *
   *********************************************************************
   */
  if (psProperties->iRunMode == FTIMES_MAPFULL || (psProperties->iRunMode == FTIMES_MAPLEAN && strcmp(psProperties->acBaseName, "-") != 0))
  {
    iError = SupportMakeName(psProperties->acLogDirName, psProperties->acBaseName, psProperties->acBaseNameSuffix, ".log", psProperties->acLogFileName, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Log File: %s", acRoutine, acLocalError);
      return iError;
    }

    iError = SupportAddToList(psProperties->acLogFileName, &psProperties->psExcludeList, "Exclude", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }

    psProperties->pFileLog = fopen(psProperties->acLogFileName, "wb+");
    if (psProperties->pFileLog == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: LogFile = [%s]: %s", acRoutine, psProperties->acLogFileName, strerror(errno));
      return ER_fopen;
    }
    setvbuf(psProperties->pFileLog, NULL, _IOLBF, 0);
  }
  else
  {
    strncpy(psProperties->acLogFileName, "stderr", FTIMES_MAX_PATH);
    psProperties->pFileLog = stderr;
  }

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
  if (psProperties->iRunMode == FTIMES_MAPFULL || (psProperties->iRunMode == FTIMES_MAPLEAN && strcmp(psProperties->acBaseName, "-") != 0))
  {
    iError = SupportMakeName(psProperties->acOutDirName, psProperties->acBaseName, psProperties->acBaseNameSuffix, ".map", psProperties->acOutFileName, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Out File: %s", acRoutine, acLocalError);
      return iError;
    }

    iError = SupportAddToList(psProperties->acOutFileName, &psProperties->psExcludeList, "Exclude", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return iError;
    }

    psProperties->pFileOut = fopen(psProperties->acOutFileName, "wb+");
    if (psProperties->pFileOut == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: OutFile = [%s]: %s", acRoutine, psProperties->acOutFileName, strerror(errno));
      return ER_fopen;
    }
    setvbuf(psProperties->pFileOut, NULL, _IOLBF, 0);
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

#ifdef USE_XMAGIC
  if (MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MAGIC))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_MagicFile, psProperties->acMagicFileName);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

    snprintf(acMessage, MESSAGE_SIZE, "MagicHash=%s", psProperties->acMagicHash);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }
#endif

  /*-
   *********************************************************************
   *
   * Write out a header.
   *
   *********************************************************************
   */
  iError = MapWriteHeader(psProperties, acLocalError);
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
 * MapModeWorkHorse
 *
 ***********************************************************************
 */
int
MapModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FILE_LIST           *psList = NULL;

  /*-
   *********************************************************************
   *
   * Process the Include list.
   *
   *********************************************************************
   */
  for (psList = psProperties->psIncludeList; psList != NULL; psList = psList->psNext)
  {
    if (SupportMatchExclude(psProperties->psExcludeList, psList->acPath) == NULL)
    {
      MapFile(psProperties, psList->acPath, acLocalError);
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
  char                acMessage[MESSAGE_SIZE];
  int                 i;
  int                 iFirst;
  int                 iIndex;
  unsigned char       aucFileHash[MD5_HASH_SIZE];

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

  memset(aucFileHash, 0, MD5_HASH_SIZE);
  MD5Omega(&psProperties->sOutFileHashContext, aucFileHash);
  MD5HashToHex(aucFileHash, psProperties->acOutFileHash);

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

  snprintf(acMessage, MESSAGE_SIZE, "OutFileHash=%s", psProperties->acOutFileHash);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "DataType=%s", psProperties->acDataType);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * Write out the statistics.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "DirectoriesEncountered=%d", MapGetDirectoryCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "FilesEncountered=%d", MapGetFileCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

#ifdef UNIX
  snprintf(acMessage, MESSAGE_SIZE, "SpecialsEncountered=%d", MapGetSpecialCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif

#ifdef WINNT
  snprintf(acMessage, MESSAGE_SIZE, "StreamsEncountered=%d", MapGetStreamCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif

  iIndex = 0;
  if (psProperties->iLastAnalysisStage > 0)
  {
    iIndex = sprintf(&acMessage[iIndex], "AnalysisStages=");
    for (i = 0, iFirst = 0; i < psProperties->iLastAnalysisStage; i++)
    {
        iIndex += sprintf(&acMessage[iIndex], "%s%s", (iFirst++ > 0) ? "," : "", psProperties->asAnalysisStages[i].acDescription);
    }
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

    snprintf(acMessage, MESSAGE_SIZE, "ObjectsAnalyzed=%u", AnalyzeGetFileCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

#ifdef UNIX
#ifdef USE_AP_SNPRINTF
    snprintf(acMessage, MESSAGE_SIZE, "BytesAnalyzed=%qu", (unsigned long long) AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#else
    snprintf(acMessage, MESSAGE_SIZE, "BytesAnalyzed=%llu", (unsigned long long) AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif
#endif

#ifdef WIN32
    snprintf(acMessage, MESSAGE_SIZE, "BytesAnalyzed=%I64u", AnalyzeGetByteCount());
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif
  }
  else
  {
    snprintf(acMessage, MESSAGE_SIZE, "AnalysisStages=None");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  snprintf(acMessage, MESSAGE_SIZE, "CompleteRecords=%d", MapGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "IncompleteRecords=%d", MapGetIncompleteRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

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
  const char          acRoutine[] = "MapModeFinalStage()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
    iError = URLPutRequest(psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_URLPutRequest;
    }

    if (psProperties->bURLUnlinkOutput)
    {
      FTimesEraseFiles(psProperties, pcError);
    }
  }

  return ER_OK;
}
