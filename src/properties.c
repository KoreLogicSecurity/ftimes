/*-
 ***********************************************************************
 *
 * $Id: properties.c,v 1.20 2004/04/23 19:44:17 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2004 Klayton Monroe, All Rights Reserved.
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
#define PROPERTIES_MAX_LINE_LENGTH  1024
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
PropertiesTestFile(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  char                acLocalError[MESSAGE_SIZE] = { 0 };
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acLine[PROPERTIES_MAX_LINE_LENGTH];
  char               *pc;
  int                 iError;
  int                 iLength;
  int                 iLineNumber;
  FILE               *pFile;

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
    if ((pFile = fopen(pcFilename, "r")) == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, pcFilename, strerror(errno));
      return ER_ReadPropertiesFile;
    }
  }

  for (acLine[0] = 0, iLineNumber = 1; fgets(acLine, PROPERTIES_MAX_LINE_LENGTH, pFile) != NULL; acLine[0] = 0, iLineNumber++)
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
    while (isspace((int) acLine[iLength - 1]))
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
  char                acLocalError[MESSAGE_SIZE] = { 0 };
#ifdef USE_SSL
  char                acTempFile[FTIMES_MAX_PATH];
#endif
  char               *pc;
  char               *pcControl;
  char               *pcE;
#ifdef WIN32
  char               *pcMessage;
#endif
  int                 i;
  int                 iError;
  int                 iRunMode;
  int                 iValue;
#ifdef UNIX
  struct stat         sStatEntry;
#endif
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
  pcE = &pc[iLength - 1];
  while (isspace((int) *pcE))
  {
    *pcE-- = 0;
    iLength--;
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
  pcE = &pcControl[strlen(pcControl) - 1];
  while (isspace((int) *pcE))
  {
    *pcE-- = 0;
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

  iRunMode = (psProperties->iRunMode == FTIMES_CFGTEST) ? psProperties->iTestRunMode : psProperties->iRunMode;

  if (strcasecmp(pcControl, KEY_URLAuthType) == 0 && RUN_MODE_IS_SET(MODES_URLAuthType, iRunMode))
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

  else if (strcasecmp(pcControl, KEY_AnalyzeDeviceFiles) == 0 && RUN_MODE_IS_SET(MODES_AnalyzeDeviceFiles, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeDeviceFilesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bAnalyzeDeviceFiles);
    psProperties->sFound.bAnalyzeDeviceFilesFound = TRUE;
  }

  else if ((strcasecmp(pcControl, KEY_AnalyzeRemoteFiles) == 0 || strcasecmp(pcControl, KEY_MapRemoteFiles) == 0) && RUN_MODE_IS_SET(MODES_AnalyzeRemoteFiles, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bAnalyzeRemoteFilesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bAnalyzeRemoteFiles);
    psProperties->sFound.bAnalyzeRemoteFilesFound = TRUE;
  }

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
      if (!isalnum((int) pc[iLength - 1]) && pc[iLength - 1] != '_' && pc[iLength - 1] != '-')
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], BaseNames must constructed from the following character set: [0-9a-zA-Z_-]", acRoutine, pcControl);
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

  else if (strcasecmp(pcControl, KEY_DateTime) == 0 && RUN_MODE_IS_SET(MODES_DateTime, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bDateTimeFound);
    if (iLength != FTIMES_DATETIME_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    for (i = 0; i < FTIMES_DATETIME_SIZE - 1; i++)
    {
      if (isdigit((int) pc[i]) == 0)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid date format.", acRoutine, pcControl);
        return ER;
      }
      psProperties->acRunDateTime[i] = pc[i];
    }
    psProperties->acRunDateTime[i] = 0;

    psProperties->sFound.bDateTimeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_DigString) == 0 && RUN_MODE_IS_SET(MODES_DigString, iRunMode))
  {
    iError = DigAddString(pc, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_EnableRecursion) == 0 && RUN_MODE_IS_SET(MODES_EnableRecursion, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bEnableRecursionFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bEnableRecursion);
    psProperties->sFound.bEnableRecursionFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_FileSizeLimit) == 0 && RUN_MODE_IS_SET(MODES_FileSizeLimit, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bFileSizeLimitFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], Value must contain only digits.", acRoutine, pcControl, pc);
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

  else if (strcasecmp(pcControl, KEY_URLGetRequest) == 0 && RUN_MODE_IS_SET(MODES_URLGetRequest, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLGetRequestFound);
    if (strcasecmp(pc, "MapFullConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "MapFullConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_MAPFULL;
    }
    else if (strcasecmp(pc, "MapLeanConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "MapLeanConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_MAPLEAN;
    }
    else if (strcasecmp(pc, "DigFullConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "DigFullConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_DIGFULL;
    }
    else if (strcasecmp(pc, "DigLeanConfig") == 0)
    {
      strncpy(psProperties->acURLGetRequest, "DigLeanConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_DIGLEAN;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [Dig{Full,Lean}Config|Map{Full,Lean}Config].", acRoutine, pcControl);
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

    psProperties->psGetURL = HTTPParseURL(pc, acLocalError);
    if (psProperties->psGetURL == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bURLGetURLFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Exclude) == 0 && RUN_MODE_IS_SET(MODES_Exclude, iRunMode))
  {
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Make sure that we have the start of a full path.
     *
     *******************************************************************
     */
#ifdef WIN32
    if (!(isalpha((int) pc[0]) && pc[1] == ':'))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", acRoutine, pcControl, pc);
      return ER;
    }
#endif
#ifdef UNIX
    if (pc[0] != FTIMES_SLASHCHAR)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", acRoutine, pcControl, pc);
      return ER;
    }
#endif

    iError = SupportAddToList(pc, &psProperties->psExcludeList, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_Include) == 0 && RUN_MODE_IS_SET(MODES_Include, iRunMode))
  {
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Make sure that we have the start of a full path.
     *
     *******************************************************************
     */
#ifdef WIN32
    if (!(isalpha((int) pc[0]) && pc[1] == ':'))
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", acRoutine, pcControl, pc);
      return ER;
    }
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      if (GetFileAttributes(pc) == 0xffffffff)
      {
        ErrorFormatWin32Error(&pcMessage);
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s]: %s", acRoutine, pcControl, pc, pcMessage);
        return ER;
      }
    }
#endif
#ifdef UNIX
    if (pc[0] != FTIMES_SLASHCHAR)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", acRoutine, pcControl, pc);
      return ER;
    }
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      if (lstat(pc, &sStatEntry) == ER)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%s]: %s", acRoutine, pcControl, pc, strerror(errno));
        return ER;
      }
    }
#endif

    iError = SupportAddToList(pc, &psProperties->psIncludeList, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
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

  else if (strcasecmp(pcControl, KEY_LogFileName) == 0 && RUN_MODE_IS_SET(MODES_LogFileName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bLogFileNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acLogFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bLogFileNameFound = TRUE;
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

  else if (strcasecmp(pcControl, KEY_FieldMask) == 0 && RUN_MODE_IS_SET(MODES_FieldMask, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bFieldMaskFound);
    if (iLength < 1 || iLength > ALL_FIELDS_MASK_SIZE - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    if (iRunMode != FTIMES_PUTMODE) /* This guard was put in to keep posting platform independent. */
    {
      iError = CompareParseStringMask(pc, &psProperties->ulFieldMask, psProperties->iRunMode, psProperties->psMaskTable, psProperties->iMaskTableLength, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    strncpy(psProperties->acMaskString, pc, ALL_FIELDS_MASK_SIZE);
    psProperties->sFound.bFieldMaskFound = TRUE;
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

  else if (strcasecmp(pcControl, KEY_URLCreateConfig) == 0 && RUN_MODE_IS_SET(MODES_URLCreateConfig, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLCreateConfigFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bURLCreateConfig);
    psProperties->sFound.bURLCreateConfigFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLUnlinkOutput) == 0 && RUN_MODE_IS_SET(MODES_URLUnlinkOutput, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLUnlinkOutputFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bURLUnlinkOutput);
    psProperties->sFound.bURLUnlinkOutputFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_DataType) == 0 && RUN_MODE_IS_SET(MODES_DataType, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bDataTypeFound);
    if (strcasecmp(pc, "dig") == 0)
    {
      strncpy(psProperties->acDataType, "dig", FTIMES_MAX_DATA_TYPE);
    }
    else if (strcasecmp(pc, "map") == 0)
    {
      strncpy(psProperties->acDataType, "map", FTIMES_MAX_DATA_TYPE);
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value must be [dig|map].", acRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bDataTypeFound = TRUE;
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
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Value = [%d], Value out of range", acRoutine, pcControl, iValue);
      return ER;
    }
    else
    {
      psProperties->iMatchLimit = iValue;
      DigSetMatchLimit(psProperties->iMatchLimit);
    }
    psProperties->sFound.bMatchLimitFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_OutFileHash) == 0 && RUN_MODE_IS_SET(MODES_OutFileHash, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bOutFileHashFound);
    if (iLength != FTIMEX_MAX_MD5_LENGTH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    for (i = 0; i < FTIMEX_MAX_MD5_LENGTH - 1; i++)
    {
      psProperties->acOutFileHash[i] = tolower((int) pc[i]);
    }
    psProperties->acOutFileHash[i] = 0;
    psProperties->sFound.bOutFileHashFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_OutFileName) == 0 && RUN_MODE_IS_SET(MODES_OutFileName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bOutFileNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], Invalid length [%d].", acRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->acOutFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bOutFileNameFound = TRUE;
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

    psProperties->psPutURL = HTTPParseURL(pc, acLocalError);
    if (psProperties->psPutURL == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bURLPutURLFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_RequirePrivilege) == 0 && RUN_MODE_IS_SET(MODES_RequirePrivilege, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bRequirePrivilegeFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bRequirePrivilege);
    psProperties->sFound.bRequirePrivilegeFound = TRUE;
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
      iError = SSLSetBundledCAsFile(psProperties->psSSLProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLBundledCAsFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPassPhrase) == 0 && RUN_MODE_IS_SET(MODES_SSLPassPhrase, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPassPhraseFound);
    iError = SSLSetPassPhrase(psProperties->psSSLProperties, pc, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bSSLPassPhraseFound = TRUE;
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
      iError = SSLSetPublicCertFile(psProperties->psSSLProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLPublicCertFileFound = TRUE;
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
      psProperties->psSSLProperties->iMaxChainLength = iValue;
    }
    psProperties->sFound.bSSLMaxChainLengthFound = TRUE;
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
      iError = SSLSetPrivateKeyFile(psProperties->psSSLProperties, acTempFile, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLPrivateKeyFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLExpectedPeerCN) == 0 && RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLExpectedPeerCNFound);
    iError = SSLSetExpectedPeerCN(psProperties->psSSLProperties, pc, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s]: %s", acRoutine, pcControl, acLocalError);
      return ER;
    }
    psProperties->sFound.bSSLExpectedPeerCNFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLUseCertificate) == 0 && RUN_MODE_IS_SET(MODES_SSLUseCertificate, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLUseCertificateFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->psSSLProperties->iUseCertificate);
    psProperties->sFound.bSSLUseCertificateFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLVerifyPeerCert) == 0 && RUN_MODE_IS_SET(MODES_SSLVerifyPeerCert, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLVerifyPeerCertFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->psSSLProperties->iVerifyPeerCert);
    psProperties->sFound.bSSLVerifyPeerCertFound = TRUE;
  }
#endif

  else
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Control = [%s], The specified control is not valid in this mode of operation.", acRoutine, pcControl);
    return ER;
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
  DIG_SEARCH_LIST    *psSearchList;
  FILE_LIST          *psList;
  int                 i;

  if (RUN_MODE_IS_SET(MODES_AnalyzeDeviceFiles, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_AnalyzeDeviceFiles, psProperties->bAnalyzeDeviceFiles ? "Y" : "N");
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

  if (RUN_MODE_IS_SET(MODES_DataType, psProperties->iRunMode))
  {
    if (psProperties->acDataType[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_DataType, psProperties->acDataType);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_DateTime, psProperties->iRunMode))
  {
    if (psProperties->acDateTime[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_DateTime, psProperties->acDateTime);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_EnableRecursion, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_EnableRecursion, psProperties->bEnableRecursion ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_FieldMask, psProperties->iRunMode))
  {
    if (psProperties->acMaskString[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_FieldMask, psProperties->acMaskString);
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

  if (RUN_MODE_IS_SET(MODES_LogDir, psProperties->iRunMode))
  {
    if (psProperties->acLogDirName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_LogDir, psProperties->acLogDirName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_LogFileName, psProperties->iRunMode))
  {
    if (psProperties->acLogFileName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_LogFileName, psProperties->acLogFileName);
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

  if (RUN_MODE_IS_SET(MODES_OutFileHash, psProperties->iRunMode))
  {
    if (psProperties->acOutFileHash[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_OutFileHash, psProperties->acOutFileHash);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_OutFileName, psProperties->iRunMode))
  {
    if (psProperties->acOutFileName[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_OutFileName, psProperties->acOutFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
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

  if (RUN_MODE_IS_SET(MODES_URLCreateConfig, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_URLCreateConfig, psProperties->bURLCreateConfig ? "Y" : "N");
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
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLVerifyPeerCert, psProperties->psSSLProperties->iVerifyPeerCert ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLBundledCAsFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert && psProperties->psSSLProperties->pcBundledCAsFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLBundledCAsFile, psProperties->psSSLProperties->pcBundledCAsFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert && psProperties->psSSLProperties->pcExpectedPeerCN[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLExpectedPeerCN, psProperties->psSSLProperties->pcExpectedPeerCN);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLMaxChainLength, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%d", KEY_SSLMaxChainLength, psProperties->psSSLProperties->iMaxChainLength);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLUseCertificate, psProperties->iRunMode))
  {
    snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLUseCertificate, psProperties->psSSLProperties->iUseCertificate ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLPrivateKeyFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPrivateKeyFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPrivateKeyFile, psProperties->psSSLProperties->pcPrivateKeyFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPublicCertFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPublicCertFile[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPublicCertFile, psProperties->psSSLProperties->pcPublicCertFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPassPhrase, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPassPhrase[0])
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=########", KEY_SSLPassPhrase);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
#endif

  if (RUN_MODE_IS_SET(MODES_DigString, psProperties->iRunMode))
  {
    for (i = 0; i < 256; i++)
    {
      for (psSearchList = DigGetSearchList(i); psSearchList != NULL; psSearchList = psSearchList->psNext)
      {
        snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_DigString, psSearchList->acEscString);
        MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
      }
    }
  }

  if (RUN_MODE_IS_SET(MODES_Include, psProperties->iRunMode))
  {
    for (psList = psProperties->psIncludeList; psList != NULL; psList = psList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_Include, psList->acPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Exclude, psProperties->iRunMode))
  {
    for (psList = psProperties->psExcludeList; psList != NULL; psList = psList->psNext)
    {
      snprintf(acMessage, MESSAGE_SIZE, "%s=%s", KEY_Exclude, psList->acPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
    }
  }
}
