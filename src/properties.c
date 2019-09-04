/*-
 ***********************************************************************
 *
 * $Id: properties.c,v 1.75 2019/08/29 19:24:56 klm Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2019 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

/*-
 ***********************************************************************
 *
 * Defines
 *
 ***********************************************************************
 */
#define PROPERTIES_MAX_RECURSION_LEVEL 3
#define PROPERTIES_MAX_LINE         8192
#define PROPERTIES_COMMENT_C          '#'
#define PROPERTIES_COMMENT_S          "#"
#define PROPERTIES_SEPARATOR_C        '='
#define PROPERTIES_SEPARATOR_S        "="


/*-
 ***********************************************************************
 *
 * Macros
 *
 ***********************************************************************
 */
#define DUPLICATE_ERROR(b) \
  if ((b) == TRUE) \
  { \
    snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Duplicate controls aren't allowed.", acRoutine, pcControl); \
    return ER; \
  }

#define EVALUATE_TWOSTATE(pc, s1, s2, result) \
  if (strcasecmp(pc, s1) == 0) \
  {  \
    result = TRUE; \
  } \
  else if (strcasecmp(pc, s2) == 0) \
  {  \
    result = FALSE; \
  }  \
  else \
  { \
    snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be [%s|%s].", acRoutine, pcControl, pc, s1, s2); \
    return ER; \
  }


/*-
 ***********************************************************************
 *
 * PropertiesTestFile
 *
 ***********************************************************************
 */
int
PropertiesTestFile(void *pvProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = "";
  FTIMES_PROPERTIES  *psProperties = (FTIMES_PROPERTIES *)pvProperties;
  int                 iError;

  iError = PropertiesReadFile(psProperties->acConfigFile, psProperties, acLocalError);
  if (iError != ER_OK)
  {
    fprintf(stdout, "SyntaxCheck Failed --> %s\n", acLocalError);
  }
  else
  {
    fprintf(stdout, "SyntaxCheck Passed\n");
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PropertiesReadFile
 *
 ***********************************************************************
 */
int
PropertiesReadFile(char *pcFilename, FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "PropertiesReadFile()";
  char                acLocalError[MESSAGE_SIZE] = "";
  char                acLine[PROPERTIES_MAX_LINE];
  char               *pc;
  int                 iError;
  int                 iLength;
  int                 iLineNumber;
  FILE               *pFile;
  struct stat         statEntry;

  /*-
   *********************************************************************
   *
   * Check recursion level. Abort, if the level is too high.
   *
   *********************************************************************
   */
  if (psProperties->iImportRecursionLevel > PROPERTIES_MAX_RECURSION_LEVEL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Imports may not exceed %d levels of recursion.", acRoutine, pcFilename, PROPERTIES_MAX_RECURSION_LEVEL);
    return ER_ReadPropertiesFile;
  }

  if (strcmp(pcFilename, "-") == 0)
  {
    pFile = stdin;
  }
  else
  {
    char acMessage[MESSAGE_SIZE] = "";
    iError = stat(pcFilename, &statEntry);
    if (iError == -1)
    {
      /*-
       *****************************************************************
       *
       * Return OK if the specified config file (Import) doesn't exist.
       * By making this allowance, an admin can (more easily) create a
       * generic config file that spans multiple clients or an entire
       * FTimes deployment. If the recursion level is zero, then we are
       * dealing with a command line config file, and in that case, the
       * file must exist. Log it either way, for the user's benefit.
       *
       *****************************************************************
       */
      if (psProperties->iImportRecursionLevel > 0 && errno == ENOENT)
      {
        snprintf(acLocalError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
        ErrorHandler(ER_Warning, acLocalError, ERROR_WARNING);
        return ER_OK;
      }
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
      return ER_ReadPropertiesFile;
    }
    if (!((statEntry.st_mode & S_IFMT) == S_IFREG))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: A regular file is required.", acRoutine, pcFilename);
      return ER_ReadPropertiesFile;
    }
    if ((pFile = fopen(pcFilename, "r")) == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
      return ER_ReadPropertiesFile;
    }
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", ((psProperties->iImportRecursionLevel == 0) ? "Config" : "Import"), pcFilename);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  for (acLine[0] = 0, iLineNumber = 1; fgets(acLine, PROPERTIES_MAX_LINE, pFile) != NULL; acLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Ignore full line comments.
     *
     *******************************************************************
     */
    if (acLine[0] == PROPERTIES_COMMENT_C)
    {
      continue;
    }

    /*-
     *******************************************************************
     *
     * Remove EOL characters.
     *
     *******************************************************************
     */
    if (SupportChopEOLs(acLine, feof(pFile) ? 0 : 1, acLocalError) == ER)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
      if (pFile != stdin)
      {
        fclose(pFile);
      }
      return ER_ReadPropertiesFile;
    }

    /*-
     *******************************************************************
     *
     * Look for the first imbedded comment and mark it.
     *
     *******************************************************************
     */
    if ((pc = strstr(acLine, PROPERTIES_COMMENT_S)) != NULL)
    {
      *pc = 0;
    }
    iLength = strlen(acLine);

    /*-
     *******************************************************************
     *
     * Burn any trailing white space off line.
     *
     *******************************************************************
     */
    while (iLength > 0 && isspace((int) acLine[iLength - 1]))
    {
      acLine[iLength--] = 0;
    }

    /*-
     *******************************************************************
     *
     * If there's anything left over, process it.
     *
     *******************************************************************
     */
    if (iLength)
    {
      iError = PropertiesReadLine(acLine, psProperties, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, acLocalError);
        if (pFile != stdin)
        {
          fclose(pFile);
        }
        return ER_ReadPropertiesFile;
      }
    }
  }
  if (ferror(pFile))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s], Line = [%d]: %s", acRoutine, pcFilename, iLineNumber, strerror(errno));
    if (pFile != stdin)
    {
      fclose(pFile);
    }
    return ER_ReadPropertiesFile;
  }
  if (pFile != stdin)
  {
    fclose(pFile);
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PropertiesReadLine
 *
 ***********************************************************************
 */
int
PropertiesReadLine(char *pcLine, FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "PropertiesReadLine()";
  char                acLocalError[MESSAGE_SIZE] = "";
#ifdef USE_SSL
  char                acTempFile[FTIMES_MAX_PATH];
#endif
  char               *pc;
  char               *pcControl;
  char               *pcEnd;
  int                 iError;
  int                 iRunMode;
  int                 iValue;
  APP_UI64            ui64Value = 0;
  unsigned int        iLength;

  /*-
   *********************************************************************
   *
   * Process one line of input from the config file. It is assumed that
   * the input string has already been stripped of comments and EOLs.
   * The string is expected to have the following form: "control:value"
   *
   *********************************************************************
   */

  /*-
   *********************************************************************
   *
   * Look for the first separator, and mark it to isolate the control.
   *
   *********************************************************************
   */
  if ((pc = strstr(pcLine, PROPERTIES_SEPARATOR_S)) == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Line does not contain a control/value separator (i.e. '%s').", acRoutine, PROPERTIES_SEPARATOR_S);
    return ER;
  }
  *pc++ = 0;
  pcControl = pcLine;

  /*-
   *********************************************************************
   *
   * Burn any leading white space off value.
   *
   *********************************************************************
   */
  while (isspace((int) *pc))
  {
    pc++;
  }

  /*-
   *********************************************************************
   *
   * Burn any trailing white space off value.
   *
   *********************************************************************
   */
  iLength = strlen(pc);
  if (iLength > 0)
  {
    pcEnd = &pc[iLength - 1];
    while (iLength > 0 && isspace((int) *pcEnd))
    {
      *pcEnd-- = 0;
      iLength--;
    }
  }

  /*-
   *********************************************************************
   *
   * Burn any leading white space off control.
   *
   *********************************************************************
   */
  while (isspace((int) *pcControl))
  {
    pcControl++;
  }

  /*-
   *********************************************************************
   *
   * Burn any trailing white space off control.
   *
   *********************************************************************
   */
  iLength = strlen(pcControl);
  if (iLength > 0)
  {
    pcEnd = &pcControl[iLength - 1];
    while (iLength > 0 && isspace((int) *pcEnd))
    {
      *pcEnd-- = 0;
      iLength--;
    }
  }

  /*-
   *********************************************************************
   *
   * At this point pc should be pointing at a control value. Sift
   * through the various controls and do the appropriate things. If
   * the control is unrecognized, complain about it.
   *
   *********************************************************************
   */
  iLength = strlen(pc);

  iRunMode = (psProperties->iRunMode == FTIMES_CFGTEST) ? psProperties->iTestRunMode : psProperties->iRunMode;

  if (strcasecmp(pcControl, KEY_AnalyzeBlockSize) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeBlockSize, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeBlockSizeFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < FTIMES_MIN_BLOCK_SIZE || iValue > FTIMES_MAX_BLOCK_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value out of range.", acRoutine, pcControl, pc);
      return ER;
    }
    else
    {
      psProperties->iAnalyzeBlockSize = iValue;
      AnalyzeSetBlockSize(psProperties->iAnalyzeBlockSize);
    }
    psProperties->sFound.bAnalyzeBlockSizeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeByteCount) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeByteCount, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeByteCountFound);
    iError = SupportStringToUInt64(pc, &ui64Value, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    else
    {
      psProperties->ui64AnalyzeByteCount = ui64Value;
    }
    psProperties->sFound.bAnalyzeByteCountFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeCarrySize) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeCarrySize, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeCarrySizeFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < 0 || iValue > FTIMES_MAX_BLOCK_SIZE) /* A carry size of zero is OK. */
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value out of range.", acRoutine, pcControl, pc);
      return ER;
    }
    else
    {
      psProperties->iAnalyzeCarrySize = iValue;
      AnalyzeSetCarrySize(psProperties->iAnalyzeCarrySize);
    }
    psProperties->sFound.bAnalyzeCarrySizeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeDeviceFiles) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeDeviceFiles, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeDeviceFilesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bAnalyzeDeviceFiles);
    psProperties->sFound.bAnalyzeDeviceFilesFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeMaxDepth) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeMaxDepth, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeMaxDepthFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < 0 || iValue > FTIMES_MAX_DEPTH)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value out of range.", acRoutine, pcControl, pc);
      return ER;
    }
    else
    {
      psProperties->iAnalyzeMaxDepth = iValue;
    }
    psProperties->sFound.bAnalyzeMaxDepthFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeMaxDps) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeMaxDps, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeMaxDpsFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < 0 || iValue > FTIMES_MAX_KBPS)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value out of range.", acRoutine, pcControl, pc);
      return ER;
    }
    else
    {
      psProperties->iAnalyzeMaxDps = iValue;
    }
    psProperties->sFound.bAnalyzeMaxDpsFound = TRUE;
  }

  else if ((strcasecmp(pcControl, KEY_AnalyzeRemoteFiles) == 0 || strcasecmp(pcControl, KEY_MapRemoteFiles) == 0) && RUN_MODE_IS_SET(MODES_AnalyzeRemoteFiles, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeRemoteFilesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bAnalyzeRemoteFiles);
    psProperties->sFound.bAnalyzeRemoteFilesFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_AnalyzeStartOffset) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeStartOffset, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeStartOffsetFound);
    iError = SupportStringToUInt64(pc, &ui64Value, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    else
    {
      psProperties->ui64AnalyzeStartOffset = ui64Value;
    }
    psProperties->sFound.bAnalyzeStartOffsetFound = TRUE;
  }

#ifdef USE_XMAGIC
  else if (strcasecmp(pcControl, KEY_AnalyzeStepSize) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeStepSize, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeStepSizeFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < FTIMES_MIN_BLOCK_SIZE || iValue > FTIMES_MAX_BLOCK_SIZE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value out of range.", acRoutine, pcControl, pc);
      return ER;
    }
    else
    {
      psProperties->iAnalyzeStepSize = iValue;
      AnalyzeSetStepSize(psProperties->iAnalyzeStepSize);
    }
    psProperties->sFound.bAnalyzeStepSizeFound = TRUE;
  }
#endif

  else if (strcasecmp(pcControl, KEY_BaseName) == 0 && RUN_MODE_IS_SET(MODES_BaseName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bBaseNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    while (iLength > 0)
    {
      if (!isalnum((int) pc[iLength - 1]) && pc[iLength - 1] != '_' && pc[iLength - 1] != '-' && pc[iLength - 1] != '.')
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], BaseNames must constructed from the following character set: [0-9a-zA-Z._-].", acRoutine, pcControl);
        return ER;
      }
      iLength--;
    }
    strncpy(psProperties->acBaseName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bBaseNameFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_BaseNameSuffix) == 0 && RUN_MODE_IS_SET(MODES_BaseNameSuffix, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bBaseNameSuffixFound);
    if (strcasecmp(pc, "datetime") == 0)
    {
      strncpy(psProperties->acBaseNameSuffix, psProperties->acDateTime, FTIMES_SUFFIX_SIZE);
    }
    else if (strcasecmp(pc, "none") == 0)
    {
      psProperties->acBaseNameSuffix[0] = 0;
    }
    else if (strcasecmp(pc, "pid") == 0)
    {
      strncpy(psProperties->acBaseNameSuffix, psProperties->acPid, FTIMES_SUFFIX_SIZE);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [datetime|none|pid].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bBaseNameSuffixFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Compress) == 0 && RUN_MODE_IS_SET(MODES_Compress, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bCompressFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bCompress);
    psProperties->sFound.bCompressFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_DigStringNoCase) == 0 && RUN_MODE_IS_SET(MODES_DigStringNoCase, iRunMode))
  {
    iError = DigAddDigString(pc, DIG_STRING_TYPE_NOCASE, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if ((strcasecmp(pcControl, KEY_DigStringNormal) == 0 || strcasecmp(pcControl, KEY_DigString) == 0) && RUN_MODE_IS_SET(MODES_DigStringNormal, iRunMode))
  {
    iError = DigAddDigString(pc, DIG_STRING_TYPE_NORMAL, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

#ifdef USE_PCRE
  else if (strcasecmp(pcControl, KEY_DigStringRegExp) == 0 && RUN_MODE_IS_SET(MODES_DigStringRegExp, iRunMode))
  {
    iError = DigAddDigString(pc, DIG_STRING_TYPE_REGEXP, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

#ifdef USE_XMAGIC
  else if (strcasecmp(pcControl, KEY_DigStringXMagic) == 0 && RUN_MODE_IS_SET(MODES_DigStringXMagic, iRunMode))
  {
    iError = DigAddDigString(pc, DIG_STRING_TYPE_XMAGIC, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

  else if (strcasecmp(pcControl, KEY_EnableRecursion) == 0 && RUN_MODE_IS_SET(MODES_EnableRecursion, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bEnableRecursionFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bEnableRecursion);
    psProperties->sFound.bEnableRecursionFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Exclude) == 0 && RUN_MODE_IS_SET(MODES_Exclude, iRunMode))
  {
    iError = SupportAddToList(pc, &psProperties->psExcludeList, "Exclude", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

#ifdef USE_PCRE
  else if (strcasecmp(pcControl, KEY_ExcludeFilter) == 0 && RUN_MODE_IS_SET(MODES_ExcludeFilter, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psExcludeFilterList, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_ExcludeFilterMd5) == 0 && RUN_MODE_IS_SET(MODES_ExcludeFilterMd5, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psExcludeFilterMd5List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_ExcludeFilterSha1) == 0 && RUN_MODE_IS_SET(MODES_ExcludeFilterSha1, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psExcludeFilterSha1List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_ExcludeFilterSha256) == 0 && RUN_MODE_IS_SET(MODES_ExcludeFilterSha256, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psExcludeFilterSha256List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

#ifdef USE_KLEL_FILTERS
  else if (strcasecmp(pcControl, KEY_ExcludeFilterKlel) == 0 && RUN_MODE_IS_SET(MODES_ExcludeFilterKlel, iRunMode))
  {
    iError = FilterAddFilter(pc, &psProperties->psExcludeFilterListKlel, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

  else if (strcasecmp(pcControl, KEY_ExcludesMustExist) == 0 && RUN_MODE_IS_SET(MODES_ExcludesMustExist, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bExcludesMustExistFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bExcludesMustExist);
    psProperties->sFound.bExcludesMustExistFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_FieldMask) == 0 && RUN_MODE_IS_SET(MODES_FieldMask, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bFieldMaskFound);
    psProperties->psFieldMask = MaskParseMask(pc, MASK_MASK_TYPE_MAP, acLocalError);
    if (psProperties->psFieldMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bFieldMaskFound = TRUE;
  }

#ifdef USE_FILE_HOOKS
  else if (strcasecmp(pcControl, KEY_FileHook) == 0 && RUN_MODE_IS_SET(MODES_FileHook, iRunMode))
  {
    iError = HookAddHook(pc, &psProperties->psFileHookList, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

  else if (strcasecmp(pcControl, KEY_FileSizeLimit) == 0 && RUN_MODE_IS_SET(MODES_FileSizeLimit, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bFileSizeLimitFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    psProperties->ulFileSizeLimit = strtoul(pc, NULL, 10);
    if (errno == ERANGE)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s]: %s", acRoutine, pcControl, pc, strerror(errno));
      return ER;
    }
    psProperties->sFound.bFileSizeLimitFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_GetAndExec) == 0 && RUN_MODE_IS_SET(MODES_GetAndExec, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bGetAndExecFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bGetAndExec);
    psProperties->sFound.bGetAndExecFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_GetFileName) == 0 && RUN_MODE_IS_SET(MODES_GetFileName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bGetFileNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acGetFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bGetFileNameFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_HashDirectories) == 0 && RUN_MODE_IS_SET(MODES_HashDirectories, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bHashDirectoriesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bHashDirectories);
    psProperties->sFound.bHashDirectoriesFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_HashSymbolicLinks) == 0 && RUN_MODE_IS_SET(MODES_HashSymbolicLinks, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bHashSymbolicLinksFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bHashSymbolicLinks);
    psProperties->sFound.bHashSymbolicLinksFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Import) == 0 && RUN_MODE_IS_SET(MODES_Import, iRunMode))
  {
    psProperties->iImportRecursionLevel++;
    iError = PropertiesReadFile(pc, psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->iImportRecursionLevel--;
  }

  else if (strcasecmp(pcControl, KEY_Include) == 0 && RUN_MODE_IS_SET(MODES_Include, iRunMode))
  {
    iError = SupportAddToList(pc, &psProperties->psIncludeList, "Include", acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

#ifdef USE_PCRE
  else if (strcasecmp(pcControl, KEY_IncludeFilter) == 0 && RUN_MODE_IS_SET(MODES_IncludeFilter, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psIncludeFilterList, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_IncludeFilterMd5) == 0 && RUN_MODE_IS_SET(MODES_IncludeFilterMd5, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psIncludeFilterMd5List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_IncludeFilterSha1) == 0 && RUN_MODE_IS_SET(MODES_IncludeFilterSha1, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psIncludeFilterSha1List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_IncludeFilterSha256) == 0 && RUN_MODE_IS_SET(MODES_IncludeFilterSha256, iRunMode))
  {
    iError = SupportAddFilter(pc, &psProperties->psIncludeFilterSha256List, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif

#ifdef USE_KLEL_FILTERS
  else if (strcasecmp(pcControl, KEY_IncludeFilterKlel) == 0 && RUN_MODE_IS_SET(MODES_IncludeFilterKlel, iRunMode))
  {
    iError = FilterAddFilter(pc, &psProperties->psIncludeFilterListKlel, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }
#endif


  else if (strcasecmp(pcControl, KEY_IncludesMustExist) == 0 && RUN_MODE_IS_SET(MODES_IncludesMustExist, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bIncludesMustExistFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bIncludesMustExist);
    psProperties->sFound.bIncludesMustExistFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_LogDigStrings) == 0 && RUN_MODE_IS_SET(MODES_LogDigStrings, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bLogDigStringsFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bLogDigStrings);
    psProperties->sFound.bLogDigStringsFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_LogDir) == 0 && RUN_MODE_IS_SET(MODES_LogDir, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bLogDirFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandDirectoryPath(pc, psProperties->acLogDirName, FTIMES_MAX_PATH, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bLogDirFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_MagicFile) == 0 && RUN_MODE_IS_SET(MODES_MagicFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bMagicFileFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acMagicFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bMagicFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_MatchLimit) == 0 && RUN_MODE_IS_SET(MODES_MatchLimit, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bMatchLimitFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < FTIMES_MIN_STRING_REPEATS || iValue > FTIMES_MAX_STRING_REPEATS)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%d], Value is out of range.", acRoutine, pcControl, iValue);
      return ER;
    }
    else
    {
      psProperties->iMatchLimit = iValue;
    }
    psProperties->sFound.bMatchLimitFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_NewLine) == 0 && RUN_MODE_IS_SET(MODES_NewLine, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bNewLineFound);
    if (strcasecmp(pc, "LF") == 0)
    {
      strncpy(psProperties->acNewLine, LF, NEWLINE_LENGTH);
    }
    else if (strcasecmp(pc, "CRLF") == 0)
    {
      strncpy(psProperties->acNewLine, CRLF, NEWLINE_LENGTH);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [LF|CRLF].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bNewLineFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_OutDir) == 0 && RUN_MODE_IS_SET(MODES_OutDir, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bOutDirFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandDirectoryPath(pc, psProperties->acOutDirName, FTIMES_MAX_PATH, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bOutDirFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Priority) == 0 && RUN_MODE_IS_SET(MODES_Priority, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bPriorityFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PRIORITY_LENGTH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER_Length;
    }
    strncpy(psProperties->acPriority, pc, FTIMES_MAX_PRIORITY_LENGTH);
    psProperties->sFound.bPriorityFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_RequirePrivilege) == 0 && RUN_MODE_IS_SET(MODES_RequirePrivilege, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bRequirePrivilegeFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bRequirePrivilege);
    psProperties->sFound.bRequirePrivilegeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_RunType) == 0 && RUN_MODE_IS_SET(MODES_RunType, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bRunTypeFound);
    if (strcasecmp(pc, "baseline") == 0)
    {
      strncpy(psProperties->acRunType, "baseline", RUNTYPE_BUFSIZE);
    }
    else if (strcasecmp(pc, "linktest") == 0)
    {
      strncpy(psProperties->acRunType, "linktest", RUNTYPE_BUFSIZE);
    }
    else if (strcasecmp(pc, "snapshot") == 0)
    {
      strncpy(psProperties->acRunType, "snapshot", RUNTYPE_BUFSIZE);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [baseline|linktest|snapshot].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bRunTypeFound = TRUE;
  }

#ifdef USE_SSL
  else if (strcasecmp(pcControl, KEY_SSLBundledCAsFile) == 0 && RUN_MODE_IS_SET(MODES_SSLBundledCAsFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLBundledCAsFileFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandPath(pc, acTempFile, FTIMES_MAX_PATH, 1, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
      iError = SslSetBundledCAsFile(psProperties->psSslProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLBundledCAsFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLExpectedPeerCN) == 0 && RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLExpectedPeerCNFound);
    iError = SslSetExpectedPeerCN(psProperties->psSslProperties, pc, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bSSLExpectedPeerCNFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLMaxChainLength) == 0 && RUN_MODE_IS_SET(MODES_SSLMaxChainLength, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLMaxChainLengthFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must be 1-10.", acRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < 1 || iValue > 10 /*SSL_MAX_CHAIN_LENGTH*/)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%d], Value must be 1-10.", acRoutine, pcControl, iValue);
      return ER;
    }
    else
    {
      psProperties->psSslProperties->iMaxChainLength = iValue;
    }
    psProperties->sFound.bSSLMaxChainLengthFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPassPhrase) == 0 && RUN_MODE_IS_SET(MODES_SSLPassPhrase, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPassPhraseFound);
    iError = SslSetPassPhrase(psProperties->psSslProperties, pc, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bSSLPassPhraseFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPrivateKeyFile) == 0 && RUN_MODE_IS_SET(MODES_SSLPrivateKeyFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPrivateKeyFileFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandPath(pc, acTempFile, FTIMES_MAX_PATH, 1, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
      iError = SslSetPrivateKeyFile(psProperties->psSslProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLPrivateKeyFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPublicCertFile) == 0 && RUN_MODE_IS_SET(MODES_SSLPublicCertFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPublicCertFileFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandPath(pc, acTempFile, FTIMES_MAX_PATH, 1, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
      iError = SslSetPublicCertFile(psProperties->psSslProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLPublicCertFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLUseCertificate) == 0 && RUN_MODE_IS_SET(MODES_SSLUseCertificate, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLUseCertificateFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->psSslProperties->iUseCertificate);
    psProperties->sFound.bSSLUseCertificateFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLVerifyPeerCert) == 0 && RUN_MODE_IS_SET(MODES_SSLVerifyPeerCert, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLVerifyPeerCertFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->psSslProperties->iVerifyPeerCert);
    psProperties->sFound.bSSLVerifyPeerCertFound = TRUE;
  }
#endif

  else if (strcasecmp(pcControl, KEY_StrictControls) == 0 && RUN_MODE_IS_SET(MODES_StrictControls, iRunMode))
  {
    EVALUATE_TWOSTATE(pc, "y", "n", psProperties->bStrictControls);
  }

  else if (strcasecmp(pcControl, KEY_URLAuthType) == 0 && RUN_MODE_IS_SET(MODES_URLAuthType, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLAuthTypeFound);
    if (strcasecmp(pc, "basic") == 0)
    {
      psProperties->iURLAuthType = HTTP_AUTH_TYPE_BASIC;
    }
    else if (strcasecmp(pc, "none") == 0)
    {
      psProperties->iURLAuthType = HTTP_AUTH_TYPE_NONE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [basic|none].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bURLAuthTypeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLGetRequest) == 0 && RUN_MODE_IS_SET(MODES_URLGetRequest, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLGetRequestFound);
    if (strcasecmp(pc, "DigConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "DigConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_DIGMODE;
    }
    else if (strcasecmp(pc, "MapConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "MapConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_MAPMODE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [DigConfig|MapConfig].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bURLGetRequestFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLGetURL) == 0 && RUN_MODE_IS_SET(MODES_URLGetURL, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLGetURLFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    psProperties->psGetURL = HttpParseUrl(pc, acLocalError);
    if (psProperties->psGetURL == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bURLGetURLFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLPassword) == 0 && RUN_MODE_IS_SET(MODES_URLPassword, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLPasswordFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PASSWORD_LENGTH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acURLPassword, pc, FTIMES_MAX_PASSWORD_LENGTH);
    psProperties->sFound.bURLPasswordFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLPutSnapshot) == 0 && RUN_MODE_IS_SET(MODES_URLPutSnapshot, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLPutSnapshotFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bURLPutSnapshot);
    psProperties->sFound.bURLPutSnapshotFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLPutURL) == 0 && RUN_MODE_IS_SET(MODES_URLPutURL, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLPutURLFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    psProperties->psPutURL = HttpParseUrl(pc, acLocalError);
    if (psProperties->psPutURL == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bURLPutURLFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLUnlinkOutput) == 0 && RUN_MODE_IS_SET(MODES_URLUnlinkOutput, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLUnlinkOutputFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bURLUnlinkOutput);
    psProperties->sFound.bURLUnlinkOutputFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLUsername) == 0 && RUN_MODE_IS_SET(MODES_URLUsername, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLUsernameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_USERNAME_LENGTH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acURLUsername, pc, FTIMES_MAX_USERNAME_LENGTH);
    psProperties->sFound.bURLUsernameFound = TRUE;
  }

  else
  {
    if (psProperties->bStrictControls)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], The specified control is not valid in this mode of operation.", acRoutine, pcControl);
      return ER;
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * PropertiesDisplaySettings
 *
 ***********************************************************************
 */
void
PropertiesDisplaySettings(FTIMES_PROPERTIES *psProperties)
{
  char                acMessage[MESSAGE_SIZE];
#ifdef USE_FILE_HOOKS
  char               *pcExpression = NULL;
#endif
  DIG_STRING         *psDigString;
  FILE_LIST          *psList;
#ifdef USE_PCRE
  FILTER_LIST        *psFilterList;
#endif
#ifdef USE_FILE_HOOKS
  HOOK_LIST          *psHook = NULL;
#endif
  int                 i;

  if (RUN_MODE_IS_SET(MODES_AnalyzeBlockSize, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_AnalyzeBlockSize, psProperties->iAnalyzeBlockSize);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeByteCount, psProperties->iRunMode))
  {
#ifdef WIN32
    snprintf(acMessage, MESSAGE_SIZE, "%s=%I64u", KEY_AnalyzeByteCount, (APP_UI64) psProperties->ui64AnalyzeByteCount);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#else
#ifdef USE_AP_SNPRINTF
    snprintf(acMessage, MESSAGE_SIZE, "%s=%qu", KEY_AnalyzeByteCount, (unsigned long long) psProperties->ui64AnalyzeByteCount);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#else
    snprintf(acMessage, MESSAGE_SIZE, "%s=%llu", KEY_AnalyzeByteCount, (unsigned long long) psProperties->ui64AnalyzeByteCount);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif
#endif
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeCarrySize, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_AnalyzeCarrySize, psProperties->iAnalyzeCarrySize);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeStartOffset, psProperties->iRunMode))
  {
#ifdef WIN32
    snprintf(acMessage, MESSAGE_SIZE, "%s=%I64u", KEY_AnalyzeStartOffset, (APP_UI64) psProperties->ui64AnalyzeStartOffset);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#else
#ifdef USE_AP_SNPRINTF
    snprintf(acMessage, MESSAGE_SIZE, "%s=%qu", KEY_AnalyzeStartOffset, (unsigned long long) psProperties->ui64AnalyzeStartOffset);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#else
    snprintf(acMessage, MESSAGE_SIZE, "%s=%llu", KEY_AnalyzeStartOffset, (unsigned long long) psProperties->ui64AnalyzeStartOffset);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif
#endif
  }

#ifdef USE_XMAGIC
  if (RUN_MODE_IS_SET(MODES_AnalyzeStepSize, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_AnalyzeStepSize, psProperties->iAnalyzeStepSize);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }
#endif

  if (RUN_MODE_IS_SET(MODES_AnalyzeDeviceFiles, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_AnalyzeDeviceFiles, psProperties->bAnalyzeDeviceFiles ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeMaxDepth, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_AnalyzeMaxDepth, psProperties->iAnalyzeMaxDepth);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeMaxDps, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d (KB/s)", KEY_AnalyzeMaxDps, psProperties->iAnalyzeMaxDps);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_AnalyzeRemoteFiles, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_AnalyzeRemoteFiles, psProperties->bAnalyzeRemoteFiles ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_BaseName, psProperties->iRunMode))
  {
    if (psProperties->acBaseName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_BaseName, psProperties->acBaseName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_BaseNameSuffix, psProperties->iRunMode))
  {
    if (psProperties->acBaseNameSuffix[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_BaseNameSuffix, psProperties->acBaseNameSuffix);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Compress, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_Compress, psProperties->bCompress ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_EnableRecursion, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_EnableRecursion, psProperties->bEnableRecursion ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_ExcludesMustExist, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludesMustExist, psProperties->bExcludesMustExist ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_FieldMask, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_FieldMask, psProperties->psFieldMask->pcMask);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }
  else
  {
    if (psProperties->iRunMode == FTIMES_DIGMODE)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s (immutable)", KEY_FieldMask, psProperties->psFieldMask->pcMask);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_FileSizeLimit, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%lu", KEY_FileSizeLimit, psProperties->ulFileSizeLimit);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_GetAndExec, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_GetAndExec, psProperties->bGetAndExec ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_GetFileName, psProperties->iRunMode))
  {
    if (psProperties->acGetFileName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_GetFileName, psProperties->acGetFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_HashDirectories, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_HashDirectories, psProperties->bHashDirectories ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_HashSymbolicLinks, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_HashSymbolicLinks, psProperties->bHashSymbolicLinks ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_IncludesMustExist, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludesMustExist, psProperties->bIncludesMustExist ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_LogDir, psProperties->iRunMode))
  {
    if (psProperties->acLogDirName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_LogDir, psProperties->acLogDirName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_MagicFile, psProperties->iRunMode))
  {
    if (psProperties->acMagicFileName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_MagicFile, psProperties->acMagicFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_MatchLimit, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_MatchLimit, psProperties->iMatchLimit);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_NewLine, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_NewLine, (strcmp(psProperties->acNewLine, LF) == 0) ? "LF" : "CRLF");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_OutDir, psProperties->iRunMode))
  {
    if (psProperties->acOutDirName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_OutDir, psProperties->acOutDirName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Priority, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_Priority, psProperties->acPriority);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_RequirePrivilege, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_RequirePrivilege, psProperties->bRequirePrivilege ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_RunType, psProperties->iRunMode))
  {
    if (psProperties->acRunType[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_RunType, psProperties->acRunType);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLGetURL, psProperties->iRunMode))
  {
    if (psProperties->psGetURL)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s://%s:%s%s",
                KEY_URLGetURL,
#ifdef USE_SSL
                (psProperties->psGetURL->iScheme == HTTP_SCHEME_HTTPS) ? "https" : "http",
#else
                "http",
#endif
                psProperties->psGetURL->pcHost,
                psProperties->psGetURL->pcPort,
                psProperties->psGetURL->pcPath
              );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLGetRequest, psProperties->iRunMode))
  {
    if (psProperties->acURLGetRequest[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLGetRequest, psProperties->acURLGetRequest);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  /*-
   *********************************************************************
   *
   * Conditionally print URL/SSL properties.
   *
   *********************************************************************
   */
if (RUN_MODE_IS_SET(MODES_URLPutSnapshot, psProperties->iRunMode) && psProperties->bURLPutSnapshot)
{
  if (RUN_MODE_IS_SET(MODES_URLPutSnapshot, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLPutSnapshot, psProperties->bURLPutSnapshot ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_URLPutURL, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot && psProperties->psPutURL)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s://%s:%s%s",
                KEY_URLPutURL,
#ifdef USE_SSL
                (psProperties->psPutURL->iScheme == HTTP_SCHEME_HTTPS) ? "https" : "http",
#else
                "http",
#endif
                psProperties->psPutURL->pcHost,
                psProperties->psPutURL->pcPort,
                psProperties->psPutURL->pcPath
              );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLUnlinkOutput, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLUnlinkOutput, psProperties->bURLUnlinkOutput ? "Y" : "N");
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLAuthType, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLAuthType, (psProperties->iURLAuthType == HTTP_AUTH_TYPE_BASIC) ? "basic" : "none");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_URLUsername, psProperties->iRunMode))
  {
    if (psProperties->acURLUsername[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLUsername, psProperties->acURLUsername);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLPassword, psProperties->iRunMode))
  {
    if (psProperties->acURLPassword[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=########", KEY_URLPassword);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

#ifdef USE_SSL
  if (RUN_MODE_IS_SET(MODES_SSLVerifyPeerCert, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLVerifyPeerCert, psProperties->psSslProperties->iVerifyPeerCert ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLBundledCAsFile, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iVerifyPeerCert && psProperties->psSslProperties->pcBundledCAsFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLBundledCAsFile, psProperties->psSslProperties->pcBundledCAsFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iVerifyPeerCert && psProperties->psSslProperties->pcExpectedPeerCN[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLExpectedPeerCN, psProperties->psSslProperties->pcExpectedPeerCN);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLMaxChainLength, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iVerifyPeerCert)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_SSLMaxChainLength, psProperties->psSslProperties->iMaxChainLength);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLUseCertificate, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLUseCertificate, psProperties->psSslProperties->iUseCertificate ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLPrivateKeyFile, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iUseCertificate && psProperties->psSslProperties->pcPrivateKeyFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPrivateKeyFile, psProperties->psSslProperties->pcPrivateKeyFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPublicCertFile, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iUseCertificate && psProperties->psSslProperties->pcPublicCertFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPublicCertFile, psProperties->psSslProperties->pcPublicCertFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPassPhrase, psProperties->iRunMode))
  {
    if (psProperties->psSslProperties->iUseCertificate && psProperties->psSslProperties->pcPassPhrase[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=########", KEY_SSLPassPhrase);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif
}

  if (RUN_MODE_IS_SET(MODES_DigStringNormal, psProperties->iRunMode) && psProperties->bLogDigStrings)
  {
    for (i = 0; i < DIG_MAX_CHAINS; i++)
    {
      for (psDigString = DigGetSearchList(DIG_STRING_TYPE_NORMAL, i); psDigString != NULL; psDigString = psDigString->psNext)
      {
        snprintf(acMessage, MESSAGE_SIZE, "%s=%s%s%s",
          KEY_DigStringNormal,
          psDigString->pucEncodedString,
          (psDigString->pcTag[0]) ? " " : "",
          (psDigString->pcTag[0]) ? psDigString->pcTag : ""
          );
        MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
      }
    }
  }

  if (RUN_MODE_IS_SET(MODES_DigStringNoCase, psProperties->iRunMode) && psProperties->bLogDigStrings)
  {
    for (i = 0; i < DIG_MAX_CHAINS; i++)
    {
      for (psDigString = DigGetSearchList(DIG_STRING_TYPE_NOCASE, i); psDigString != NULL; psDigString = psDigString->psNext)
      {
        snprintf(acMessage, MESSAGE_SIZE, "%s=%s%s%s",
          KEY_DigStringNoCase,
          psDigString->pucEncodedString,
          (psDigString->pcTag[0]) ? " " : "",
          (psDigString->pcTag[0]) ? psDigString->pcTag : ""
          );
        MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
      }
    }
  }

#ifdef USE_PCRE
  if (RUN_MODE_IS_SET(MODES_DigStringRegExp, psProperties->iRunMode) && psProperties->bLogDigStrings)
  {
    for (psDigString = DigGetSearchList(DIG_STRING_TYPE_REGEXP, 0); psDigString != NULL; psDigString = psDigString->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s%s%s",
        KEY_DigStringRegExp,
        psDigString->pucEncodedString,
        (psDigString->pcTag[0]) ? " " : "",
        (psDigString->pcTag[0]) ? psDigString->pcTag : ""
        );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif

#ifdef USE_XMAGIC
  if (RUN_MODE_IS_SET(MODES_DigStringXMagic, psProperties->iRunMode) && psProperties->bLogDigStrings)
  {
    for (psDigString = DigGetSearchList(DIG_STRING_TYPE_XMAGIC, 0); psDigString != NULL; psDigString = psDigString->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s%s%s",
        KEY_DigStringXMagic,
        psDigString->pucEncodedString,
        (psDigString->pcTag[0]) ? " " : "",
        (psDigString->pcTag[0]) ? psDigString->pcTag : ""
        );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif

  if (RUN_MODE_IS_SET(MODES_Include, psProperties->iRunMode))
  {
    for (psList = psProperties->psIncludeList; psList != NULL; psList = psList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=file://%s", KEY_Include, psList->pcEncodedPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Exclude, psProperties->iRunMode))
  {
    for (psList = psProperties->psExcludeList; psList != NULL; psList = psList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=file://%s", KEY_Exclude, psList->pcEncodedPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

#ifdef USE_PCRE
  if (RUN_MODE_IS_SET(MODES_IncludeFilter, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psIncludeFilterList; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludeFilter, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_ExcludeFilter, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psExcludeFilterList; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludeFilter, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_IncludeFilterMd5, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psIncludeFilterMd5List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludeFilterMd5, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_ExcludeFilterMd5, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psExcludeFilterMd5List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludeFilterMd5, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_IncludeFilterSha1, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psIncludeFilterSha1List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludeFilterSha1, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_ExcludeFilterSha1, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psExcludeFilterSha1List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludeFilterSha1, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_IncludeFilterSha256, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psIncludeFilterSha256List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludeFilterSha256, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_ExcludeFilterSha256, psProperties->iRunMode))
  {
    for (psFilterList = psProperties->psExcludeFilterSha256List; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludeFilterSha256, psFilterList->pcFilter);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif

#ifdef USE_KLEL_FILTERS
  if (RUN_MODE_IS_SET(MODES_IncludeFilterKlel, psProperties->iRunMode))
  {
    FILTER_LIST_KLEL *psFilterList = NULL;
    for (psFilterList = psProperties->psIncludeFilterListKlel; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_IncludeFilterKlel, psFilterList->pcExpression);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_ExcludeFilterKlel, psProperties->iRunMode))
  {
    FILTER_LIST_KLEL *psFilterList = NULL;
    for (psFilterList = psProperties->psExcludeFilterListKlel; psFilterList != NULL; psFilterList = psFilterList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_ExcludeFilterKlel, psFilterList->pcExpression);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif

#ifdef USE_FILE_HOOKS
{
  if (RUN_MODE_IS_SET(MODES_FileHook, psProperties->iRunMode))
  {
    for (psHook = psProperties->psFileHookList; psHook != NULL; psHook = psHook->psNext)
    {
      pcExpression = KlelExpressionToString(psHook->psContext, KLEL_EXPRESSION_PLUS_EVERYTHING);
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_FileHook, pcExpression);
      free(pcExpression);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
}
#endif
}
