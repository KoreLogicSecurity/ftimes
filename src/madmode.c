/*-
 ***********************************************************************
 *
 * $Id: madmode.c,v 1.13 2014/07/18 06:40:44 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2008-2014 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * MadModeInitialize
 *
 ***********************************************************************
 */
int
MadModeInitialize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "MadModeInitialize()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acMapItem[FTIMES_MAX_PATH];
  char               *pcMapItem = NULL;
  int                 iError = 0;

  /*-
   *******************************************************************
   *
   * Initialize variables.
   *
   *********************************************************************
   */
  psProperties->pFileLog = stderr;
  psProperties->pFileOut = stdout;
  psProperties->bHashSymbolicLinks = TRUE;
  psProperties->bLogDigStrings = TRUE;

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

  /*-
   *********************************************************************
   *
   * Set the priority.
   *
   *********************************************************************
   */
  iError = SupportSetPriority(psProperties, acLocalError);
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
  while ((pcMapItem = OptionsGetNextOperand(psProperties->psOptionsContext)) != NULL)
  {
    iError = SupportExpandPath(pcMapItem, acMapItem, FTIMES_MAX_PATH, 0, acLocalError);
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
 * MadModeCheckDependencies
 *
 ***********************************************************************
 */
int
MadModeCheckDependencies(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "MadModeCheckDependencies()";
  int                 iLargestDigString = DigGetMaxStringLength();
#ifdef USE_SSL
  char                acLocalError[MESSAGE_SIZE] = "";
#endif

  /*-
   *********************************************************************
   *
   * There must be a field mask.
   *
   *********************************************************************
   */
  if (psProperties->psFieldMask == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Missing FieldMask.", acRoutine);
    return ER_MissingControl;
  }

  /*-
   *********************************************************************
   *
   * There must be at least one dig string defined.
   *
   *********************************************************************
   */
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

  /*-
   *********************************************************************
   *
   * If an XMagic dig string was specified, the minimum carry size is
   * the size of an integer. This limitation is based on the way that
   * XMagicGetValueOffset() works.
   *
   *********************************************************************
   */
  if (DigGetSearchList(DIG_STRING_TYPE_XMAGIC, 0) != NULL && psProperties->iAnalyzeCarrySize < sizeof(APP_UI32))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: AnalyzeCarrySize (%d) must be %d or larger when DigStringXMagic values are in use.", acRoutine, psProperties->iAnalyzeCarrySize, (int) sizeof(APP_UI32));
    return ER;
  }
#endif

  /*-
   *********************************************************************
   *
   * Check mode-specific properties.
   *
   *********************************************************************
   */
  if (psProperties->iRunMode == FTIMES_MADMODE)
  {
    if (psProperties->acBaseName[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing BaseName.", acRoutine);
      return ER_MissingControl;
    }

    if (strcmp(psProperties->acBaseName, "-") == 0)
    {
      if (psProperties->bURLPutSnapshot)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Uploads are not allowed when the BaseName is \"-\". Either disable URLPutSnapshot or change the BaseName.", acRoutine);
        return ER;
      }
    }
    else
    {
      if (psProperties->acOutDirName[0] == 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Missing OutDir.", acRoutine);
        return ER_MissingControl;
      }
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
    if (SSLCheckDependencies(psProperties->psSslProperties, acLocalError) != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER_MissingControl;
    }
#endif
  }

#ifdef USE_PCRE
  /*-
   *********************************************************************
   *
   * Attribute filters are not compatible with certain options.
   *
   *********************************************************************
   */
  if (psProperties->bHaveAttributeFilters)
  {
    if (psProperties->bCompress)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Attribute filters are not supported when compression (Compress) is enabled.", acRoutine);
      return ER_IncompatibleOptions;
    }

    if (psProperties->bHashDirectories)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Attribute filters are not supported when directory hashing (HashDirectories) is enabled.", acRoutine);
      return ER_IncompatibleOptions;
    }
  }

  /*-
   *********************************************************************
   *
   * Attribute filters require the corresponding attribute to be set.
   *
   *********************************************************************
   */
   if ((psProperties->psExcludeFilterMd5List || psProperties->psIncludeFilterMd5List) && !MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_MD5))
   {
      snprintf(pcError, MESSAGE_SIZE, "%s: The specified attribute filter(s) require the MD5 attribute to be set. Either add this attribute to the FieldMask or remove/disable all include/exclude MD5 filters." , acRoutine);
      return ER;
   }

   if ((psProperties->psExcludeFilterSha1List || psProperties->psIncludeFilterSha1List) && !MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA1))
   {
      snprintf(pcError, MESSAGE_SIZE, "%s: The specified attribute filter(s) require the SHA1 attribute to be set. Either add this attribute to the FieldMask or remove/disable all include/exclude SHA1 filters." , acRoutine);
      return ER;
   }

   if ((psProperties->psExcludeFilterSha256List || psProperties->psIncludeFilterSha256List) && !MASK_BIT_IS_SET(psProperties->psFieldMask->ulMask, MAP_SHA256))
   {
      snprintf(pcError, MESSAGE_SIZE, "%s: The specified attribute filter(s) require the SHA256 attribute to be set. Either add this attribute to the FieldMask or remove/disable all include/exclude SHA256 filters." , acRoutine);
      return ER;
   }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MadModeFinalize
 *
 ***********************************************************************
 */
int
MadModeFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "MadModeFinalize()";
  char                acLocalError[MESSAGE_SIZE] = "";
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
  if (psProperties->bURLPutSnapshot && psProperties->iRunMode == FTIMES_MADMODE)
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
  if (psProperties->iRunMode == FTIMES_MADMODE && strcmp(psProperties->acBaseName, "-") != 0)
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
/* FIXME Remove this #ifdef at some point in the future. */
#ifdef WIN32
    /*-
     *****************************************************************
     *
     * NOTE: The buffer size was explicitly set to prevent binaries
     * made with Visual Studio 2005 (no service packs) from crashing
     * when run in lean mode. This problem may have been fixed in
     * Service Pack 1.
     *
     *****************************************************************
     */
    setvbuf(psProperties->pFileLog, NULL, _IOLBF, 1024);
#else
    setvbuf(psProperties->pFileLog, NULL, _IOLBF, 0);
#endif
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
  if (psProperties->iRunMode == FTIMES_MADMODE && strcmp(psProperties->acBaseName, "-") != 0)
  {
    iError = SupportMakeName(psProperties->acOutDirName, psProperties->acBaseName, psProperties->acBaseNameSuffix, ".mad", psProperties->acOutFileName, acLocalError);
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
/* FIXME Remove this #ifdef at some point in the future. */
#ifdef WIN32
    /*-
     *****************************************************************
     *
     * NOTE: The buffer size was explicitly set to prevent binaries
     * made with Visual Studio 2005 (no service packs) from crashing
     * when run in lean mode. This problem may have been fixed in
     * Service Pack 1.
     *
     *****************************************************************
     */
    setvbuf(psProperties->pFileOut, NULL, _IOLBF, 1024);
#else
    setvbuf(psProperties->pFileOut, NULL, _IOLBF, 0);
#endif
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
   * Write out two headers.
   *
   *********************************************************************
   */
  iError = MapWriteHeader(psProperties, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return iError;
  }
  iError = DigWriteHeader(psProperties, acLocalError);
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
 * MadModeWorkHorse
 *
 ***********************************************************************
 */
int
MadModeWorkHorse(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = "";
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
    if (SupportMatchExclude(psProperties->psExcludeList, psList->pcRegularPath) == NULL)
    {
      MapFile(psProperties, psList->pcRegularPath, acLocalError);
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MadModeFinishUp
 *
 ***********************************************************************
 */
int
MadModeFinishUp(FTIMES_PROPERTIES *psProperties, char *pcError)
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
   * List the number of dig records.
   *
   *********************************************************************
   */
#ifdef UNIX
#ifdef USE_AP_SNPRINTF
  snprintf(acMessage, MESSAGE_SIZE, "DigRecords=%qu", (unsigned long long) DigGetTotalMatches());
#else
  snprintf(acMessage, MESSAGE_SIZE, "DigRecords=%llu", (unsigned long long) DigGetTotalMatches());
#endif
#endif
#ifdef WIN32
  snprintf(acMessage, MESSAGE_SIZE, "DigRecords=%I64u", DigGetTotalMatches());
#endif
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *********************************************************************
   *
   * List the number of map records.
   *
   *********************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "MapRecords=%d", MapGetRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "PartialMapRecords=%d", MapGetIncompleteRecordCount());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  SupportDisplayRunStatistics(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * MadModeFinalStage
 *
 ***********************************************************************
 */
int
MadModeFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "MadModeFinalStage()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError;

  /*-
   *********************************************************************
   *
   * Conditionally upload the collected data.
   *
   *********************************************************************
   */
  if (psProperties->bURLPutSnapshot && psProperties->iRunMode == FTIMES_MADMODE)
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
