/*-
 ***********************************************************************
 *
 * $Id: digmode.c,v 1.36 2007/02/23 00:22:35 mavrik Exp $
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
 * DigModeProcessArguments
 *
 ***********************************************************************
 */
int
DigModeProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "DigModeProcessArguments()";
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
      iError = SupportAddToList(psProperties->acConfigFile, &psProperties->psExcludeList, "Exclude", acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return iError;
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
 * DigModeInitialize
 *
 ***********************************************************************
 */
int
DigModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DigModeInitialize()";
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
  if (psProperties->iRunMode == FTIMES_DIGAUTO)
  {
    psProperties->bAnalyzeDeviceFiles = TRUE;
    psProperties->bAnalyzeRemoteFiles = TRUE;
  }
  psProperties->psFieldMask = MaskParseMask("all", MASK_RUNMODE_TYPE_DIG, acLocalError); /* A mask is required by nph-ftimes.cgi. */
  if (psProperties->psFieldMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  /*-
   *******************************************************************
   *
   * Parse the config/strings file.
   *
   *******************************************************************
   */
  iError = PropertiesReadFile(psProperties->acConfigFile, psProperties, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
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
  const char          acRoutine[] = "DigModeCheckDependencies()";
  int                 iLargestDigString = DigGetMaxStringLength();
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#endif

  if (DigGetStringCount() <= 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Need at least one DigString.", acRoutine);
    return ER_MissingControl;
  }

  /*-
   *********************************************************************
   *
   * The carry size must be less than the block size.
   *
   *********************************************************************
   */
  if (psProperties->iAnalyzeCarrySize >= psProperties->iAnalyzeBlockSize)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: AnalyzeCarrySize (%d) must be less than AnalyzeBlockSize (%d).", acRoutine, psProperties->iAnalyzeCarrySize, psProperties->iAnalyzeBlockSize);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * The largest normal/nocase dig string must not be larger than the
   * carry size.
   *
   *********************************************************************
   */
  if (iLargestDigString > psProperties->iAnalyzeCarrySize)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: The largest DigStringNormal/DigStringNoCase value (%d) must not exceed AnalyzeCarrySize (%d).", acRoutine, iLargestDigString, psProperties->iAnalyzeCarrySize);
    return ER;
  }

#ifdef USE_XMAGIC
  /*-
   *********************************************************************
   *
   * If an XMagic dig string was specified, the minimum carry size is
   * the size of an integer. This limitation is based on the way that
   * XMagicGetValueOffset() works.
   *
   *********************************************************************
   */
  if (DigGetSearchList(DIG_STRING_TYPE_XMAGIC, 0) != NULL && psProperties->iAnalyzeCarrySize < sizeof(K_UINT32))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: AnalyzeCarrySize (%d) must be %d or larger when DigStringXMagic values are in use.", acRoutine, psProperties->iAnalyzeCarrySize, (int) sizeof(K_UINT32));
    return ER;
  }
#endif

  if (psProperties->iRunMode == FTIMES_DIGFULL)
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
  else if (psProperties->iRunMode == FTIMES_DIGLEAN)
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
 * DigModeFinalize
 *
 ***********************************************************************
 */
int
DigModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "DigModeFinalize()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
  if (psProperties->bURLPutSnapshot)
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
   * Set up the Dig engine.
   *
   *********************************************************************
   */
  AnalyzeEnableDigEngine(psProperties);

#ifdef USE_XMAGIC
  /*-
   *********************************************************************
   *
   * Forceably limit the step size if it's bigger than the block size.
   *
   *********************************************************************
   */
  if (psProperties->iAnalyzeStepSize > psProperties->iAnalyzeBlockSize)
  {
    snprintf(acLocalError, MESSAGE_SIZE, "AnalyzeStepSize (%d) is being reduced to match AnalyzeBlockSize (%d).", psProperties->iAnalyzeStepSize, psProperties->iAnalyzeBlockSize);
    ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
    psProperties->iAnalyzeStepSize = psProperties->iAnalyzeBlockSize;
    AnalyzeSetStepSize(psProperties->iAnalyzeStepSize);
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
  if (psProperties->iRunMode == FTIMES_DIGFULL || (psProperties->iRunMode == FTIMES_DIGLEAN && strcmp(psProperties->acBaseName, "-") != 0))
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
  if (psProperties->iRunMode == FTIMES_DIGFULL || (psProperties->iRunMode == FTIMES_DIGLEAN && strcmp(psProperties->acBaseName, "-") != 0))
  {
    iError = SupportMakeName(psProperties->acOutDirName, psProperties->acBaseName, psProperties->acBaseNameSuffix, ".dig", psProperties->acOutFileName, acLocalError);
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
  DigSetOutputStream(psProperties->pFileOut);

  /*-
   *********************************************************************
   *
   * Write out a Dig header record.
   *
   *********************************************************************
   */
  DigSetNewLine(psProperties->acNewLine);
  DigSetHashBlock(&psProperties->sOutFileHashContext);
  iError = DigWriteHeader(psProperties->pFileOut, psProperties->acNewLine, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Dig Header: %s", acRoutine, acLocalError);
    return iError;
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
 * DigModeFinishUp
 *
 ***********************************************************************
 */
int
DigModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
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

  /*-
   *********************************************************************
   *
   * List total number of strings.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "DigStrings=%d", DigGetStringCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * List total number of strings matched.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "DigStringsMatched=%d", DigGetStringsMatched());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * List total number of matches.
   *
   *********************************************************************
   */
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
  snprintf(acMessage, MESSAGE_SIZE, "TotalMatches=%qu", (unsigned long long) DigGetTotalMatches());
#else
  snprintf(acMessage, MESSAGE_SIZE, "TotalMatches=%llu", (unsigned long long) DigGetTotalMatches());
#endif
#endif
#ifdef WIN32
  snprintf(acMessage, MESSAGE_SIZE, "MatchCount=%I64u", DigGetTotalMatches());
#endif
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

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
  const char          acRoutine[] = "DigModeFinalStage()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
