/*
 ***********************************************************************
 *
 * $Id: properties.c,v 1.5 2003/01/16 21:08:09 mavrik Exp $
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Duplicate controls aren't allowed.", cRoutine, pcControl); \
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], Value must be [%s|%s].", cRoutine, pcControl, pc, s1, s2); \
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
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  iError = PropertiesReadFile(psProperties->cConfigFile, psProperties, cLocalError);
  if (iError != ER_OK)
  {
    fprintf(stdout, "SyntaxCheck Failed --> %s\n", cLocalError);
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
  const char          cRoutine[] = "PropertiesReadFile()";
  char                cLocalError[ERRBUF_SIZE];
  char                cLine[PROPERTIES_MAX_LINE_LENGTH];
  char               *pc;
  int                 iError;
  int                 iLength;
  int                 iLineNumber;
  FILE               *pFile;

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Check recursion level. Abort, if the level is too high.
   *
   *********************************************************************
   */
  if (psProperties->iImportRecursionLevel > PROPERTIES_MAX_RECURSION_LEVEL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Imports may not exceed %d levels of recursion.", cRoutine, pcFilename, PROPERTIES_MAX_RECURSION_LEVEL);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, pcFilename, strerror(errno));
      return ER_ReadPropertiesFile;
    }
  }

  for (cLine[0] = 0, iLineNumber = 1; fgets(cLine, PROPERTIES_MAX_LINE_LENGTH, pFile) != NULL; cLine[0] = 0, iLineNumber++)
  {
    /*-
     *******************************************************************
     *
     * Ignore full line comments.
     *
     *******************************************************************
     */
    if (cLine[0] == PROPERTIES_COMMENT_C)
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
    if (SupportChopEOLs(cLine, feof(pFile) ? 0 : 1, cLocalError) == ER)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, cLocalError);
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
    if ((pc = strstr(cLine, PROPERTIES_COMMENT_S)) != NULL)
    {
      *pc = 0;
    }
    iLength = strlen(cLine);

    /*-
     *******************************************************************
     *
     * Burn any trailing white space off line.
     *
     *******************************************************************
     */
    while (isspace((int) cLine[iLength - 1]))
    {
      cLine[iLength--] = 0;
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
      iError = PropertiesReadLine(cLine, psProperties, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, cLocalError);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s], Line = [%d]: %s", cRoutine, pcFilename, iLineNumber, strerror(errno));
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
  const char          cRoutine[] = "PropertiesReadLine()";
  char                cLocalError[ERRBUF_SIZE];
#ifdef USE_SSL
  char                cTempFile[FTIMES_MAX_PATH];
#endif
  char               *pc;
  char               *pcControl;
  char               *pcE;
#ifdef FTimes_WIN32
  char               *pcMessage;
#endif
  int                 i;
  int                 iError;
  int                 iRunMode;
  int                 iValue;
#ifdef FTimes_UNIX
  struct stat         sStatEntry;
#endif
  unsigned int        iLength;

  cLocalError[0] = 0;

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
    snprintf(pcError, ERRBUF_SIZE, "%s: Line does not contain a control/value separator (i.e. '%s').", cRoutine, PROPERTIES_SEPARATOR_S);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value must be [basic|none].", cRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bURLAuthTypeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_BaseName) == 0 && RUN_MODE_IS_SET(MODES_BaseName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bBaseNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    while (iLength > 0)
    {
      if (!isalnum((int) pc[iLength - 1]) && pc[iLength - 1] != '_' && pc[iLength - 1] != '-')
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], BaseNames must constructed from the following character set: [0-9a-zA-Z_-]", cRoutine, pcControl);
        return ER;
      }
      iLength--;
    }
    strncpy(psProperties->cBaseName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bBaseNameFound = TRUE;
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    for (i = 0; i < FTIMES_DATETIME_SIZE - 1; i++)
    {
      if (isdigit((int) pc[i]) == 0)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid date format.", cRoutine, pcControl);
        return ER;
      }
      psProperties->cRunDateTime[i] = pc[i];
    }
    psProperties->cRunDateTime[i] = 0;

    psProperties->sFound.bDateTimeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_DigString) == 0 && RUN_MODE_IS_SET(MODES_DigString, iRunMode))
  {
    iError = DigAddString(pc, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cGetFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bGetFileNameFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_HashDirectories) == 0 && RUN_MODE_IS_SET(MODES_HashDirectories, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bHashDirectoriesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bHashDirectories);
    psProperties->sFound.bHashDirectoriesFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLGetRequest) == 0 && RUN_MODE_IS_SET(MODES_URLGetRequest, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLGetRequestFound);
    if (strcasecmp(pc, "MapConfig") == 0)
    {
      strncpy(psProperties->cURLGetRequest, "MapConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_MAPFULL;
    }
    else if (strcasecmp(pc, "DigConfig") == 0)
    {
      strncpy(psProperties->cURLGetRequest, "DigConfig", GET_REQUEST_BUFSIZE);
      psProperties->iNextRunMode = FTIMES_DIGFULL;
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value must be [MapConfig|DigConfig].", cRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bURLGetRequestFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLGetURL) == 0 && RUN_MODE_IS_SET(MODES_URLGetURL, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLGetURLFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }

    psProperties->ptGetURL = HTTPParseURL(pc, cLocalError);
    if (psProperties->ptGetURL == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
    psProperties->sFound.bURLGetURLFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_Exclude) == 0 && RUN_MODE_IS_SET(MODES_Exclude, iRunMode))
  {
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Make sure that we have the start of a full path.
     *
     *******************************************************************
     */
#ifdef FTimes_WIN32
    if (!(isalpha((int) pc[0]) && pc[1] == ':'))
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", cRoutine, pcControl, pc);
      return ER;
    }
#endif
#ifdef FTimes_UNIX
    if (pc[0] != FTIMES_SLASHCHAR)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", cRoutine, pcControl, pc);
      return ER;
    }
#endif

    iError = SupportAddToList(pc, &psProperties->ptExcludeList, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_Include) == 0 && RUN_MODE_IS_SET(MODES_Include, iRunMode))
  {
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }

    /*-
     *******************************************************************
     *
     * Make sure that we have the start of a full path.
     *
     *******************************************************************
     */
#ifdef FTimes_WIN32
    if (!(isalpha((int) pc[0]) && pc[1] == ':'))
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", cRoutine, pcControl, pc);
      return ER;
    }
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      if (GetFileAttributes(pc) == 0xffffffff)
      {
        ErrorFormatWin32Error(&pcMessage);
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s]: %s", cRoutine, pcControl, pc, pcMessage);
        return ER;
      }
    }
#endif
#ifdef FTimes_UNIX
    if (pc[0] != FTIMES_SLASHCHAR)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], A full path is required.", cRoutine, pcControl, pc);
      return ER;
    }
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      if (lstat(pc, &sStatEntry) == ER)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s]: %s", cRoutine, pcControl, pc, strerror(errno));
        return ER;
      }
    }
#endif

    iError = SupportAddToList(pc, &psProperties->ptIncludeList, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
  }

  else if (strcasecmp(pcControl, KEY_Import) == 0 && RUN_MODE_IS_SET(MODES_Import, iRunMode))
  {
    psProperties->iImportRecursionLevel++;
    iError = PropertiesReadFile(pc, psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
    psProperties->iImportRecursionLevel--;
  }

  else if (strcasecmp(pcControl, KEY_LogDir) == 0 && RUN_MODE_IS_SET(MODES_LogDir, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bLogDirFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandDirectoryPath(pc, psProperties->cLogDirName, FTIMES_MAX_PATH, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cLogFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bLogFileNameFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_MagicFile) == 0 && RUN_MODE_IS_SET(MODES_MagicFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bMagicFileFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cMagicFileName, pc, FTIMES_MAX_PATH);
    psProperties->sFound.bMagicFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_NewLine) == 0 && RUN_MODE_IS_SET(MODES_NewLine, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bNewLineFound);
    if (strcasecmp(pc, "LF") == 0)
    {
      strncpy(psProperties->cNewLine, LF, NEWLINE_LENGTH);
    }
    else if (strcasecmp(pc, "CRLF") == 0)
    {
      strncpy(psProperties->cNewLine, CRLF, NEWLINE_LENGTH);
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value must be [LF|CRLF].", cRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bNewLineFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_OutDir) == 0 && RUN_MODE_IS_SET(MODES_OutDir, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bOutDirFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandDirectoryPath(pc, psProperties->cOutDirName, FTIMES_MAX_PATH, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    if (iRunMode != FTIMES_PUTMODE) /* This guard was put in to keep posting platform independent. */
    {
      iError = CompareParseStringMask(pc, &psProperties->ulFieldMask, psProperties->iRunMode, psProperties->ptMaskTable, psProperties->iMaskTableLength, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
    }
    strncpy(psProperties->cMaskString, pc, ALL_FIELDS_MASK_SIZE);
    psProperties->sFound.bFieldMaskFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_URLPassword) == 0 && RUN_MODE_IS_SET(MODES_URLPassword, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bURLPasswordFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PASSWORD_LENGTH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cURLPassword, pc, FTIMES_MAX_PASSWORD_LENGTH);
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
      strncpy(psProperties->cDataType, "dig", FTIMES_MAX_DATA_TYPE);
    }
    else if (strcasecmp(pc, "map") == 0)
    {
      strncpy(psProperties->cDataType, "map", FTIMES_MAX_DATA_TYPE);
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value must be [dig|map].", cRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bDataTypeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_RunType) == 0 && RUN_MODE_IS_SET(MODES_RunType, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bRunTypeFound);
    if (strcasecmp(pc, "baseline") == 0)
    {
      strncpy(psProperties->cRunType, "baseline", RUNTYPE_BUFSIZE);
    }
    else if (strcasecmp(pc, "linktest") == 0)
    {
      strncpy(psProperties->cRunType, "linktest", RUNTYPE_BUFSIZE);
    }
    else if (strcasecmp(pc, "snapshot") == 0)
    {
      strncpy(psProperties->cRunType, "snapshot", RUNTYPE_BUFSIZE);
    }
    else
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value must be [baseline|linktest|snapshot].", cRoutine, pcControl);
      return ER;
    }
    psProperties->sFound.bRunTypeFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_MapRemoteFiles) == 0 && RUN_MODE_IS_SET(MODES_MapRemoteFiles, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bMapRemoteFilesFound);
    EVALUATE_TWOSTATE(pc, "Y", "N", psProperties->bMapRemoteFiles);
    psProperties->sFound.bMapRemoteFilesFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_MatchLimit) == 0 && RUN_MODE_IS_SET(MODES_MatchLimit, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bMatchLimitFound);
    while (iLength > 0)
    {
      if (!isdigit((int) pc[iLength - 1]))
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], Value must be an integer.", cRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < FTIMES_MIN_STRING_REPEATS || iValue > FTIMES_MAX_STRING_REPEATS)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%d], Value out of range", cRoutine, pcControl, iValue);
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
    if (iLength != MD5_HASH_STRING_LENGTH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    for (i = 0; i < MD5_HASH_STRING_LENGTH - 1; i++)
    {
      psProperties->cOutFileHash[i] = tolower((int) pc[i]);
    }
    psProperties->cOutFileHash[i] = 0;
    psProperties->sFound.bOutFileHashFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_OutFileName) == 0 && RUN_MODE_IS_SET(MODES_OutFileName, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bOutFileNameFound);
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cOutFileName, pc, FTIMES_MAX_PATH);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }

    psProperties->ptPutURL = HTTPParseURL(pc, cLocalError);
    if (psProperties->ptPutURL == NULL)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Invalid length [%d].", cRoutine, pcControl, iLength);
      return ER;
    }
    strncpy(psProperties->cURLUsername, pc, FTIMES_MAX_USERNAME_LENGTH);
    psProperties->sFound.bURLUsernameFound = TRUE;
  }

#ifdef USE_SSL
  else if (strcasecmp(pcControl, KEY_SSLBundledCAsFile) == 0 && RUN_MODE_IS_SET(MODES_SSLBundledCAsFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLBundledCAsFileFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandPath(pc, cTempFile, FTIMES_MAX_PATH, 1, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
      iError = SSLSetBundledCAsFile(psProperties->psSSLProperties, cTempFile, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLBundledCAsFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPassPhrase) == 0 && RUN_MODE_IS_SET(MODES_SSLPassPhrase, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPassPhraseFound);
    iError = SSLSetPassPhrase(psProperties->psSSLProperties, pc, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
      return ER;
    }
    psProperties->sFound.bSSLPassPhraseFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLPublicCertFile) == 0 && RUN_MODE_IS_SET(MODES_SSLPublicCertFile, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLPublicCertFileFound);
    if (psProperties->iRunMode != FTIMES_CFGTEST || psProperties->iTestLevel == FTIMES_TEST_STRICT)
    {
      iError = SupportExpandPath(pc, cTempFile, FTIMES_MAX_PATH, 1, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
      iError = SSLSetPublicCertFile(psProperties->psSSLProperties, cTempFile, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
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
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%s], Value must be 1-10.", cRoutine, pcControl, pc);
        return ER;
      }
      iLength--;
    }
    iValue = atoi(pc);
    if (iValue < 1 || iValue > 10 /*SSL_MAX_CHAIN_LENGTH*/)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], Value = [%d], Value must be 1-10.", cRoutine, pcControl, iValue);
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
      iError = SupportExpandPath(pc, cTempFile, FTIMES_MAX_PATH, 1, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
      iError = SSLSetPrivateKeyFile(psProperties->psSSLProperties, cTempFile, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
        return ER;
      }
    }
    psProperties->sFound.bSSLPrivateKeyFileFound = TRUE;
  }

  else if (strcasecmp(pcControl, KEY_SSLExpectedPeerCN) == 0 && RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, iRunMode))
  {
    DUPLICATE_ERROR(psProperties->sFound.bSSLExpectedPeerCNFound);
    iError = SSLSetExpectedPeerCN(psProperties->psSSLProperties, pc, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s]: %s", cRoutine, pcControl, cLocalError);
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
    snprintf(pcError, ERRBUF_SIZE, "%s: Control = [%s], The specified control is not valid in this mode of operation.", cRoutine, pcLine);
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
  char                cMessage[MESSAGE_SIZE];
  DIG_SEARCH_LIST    *pSearchList;
  FILE_LIST          *pList;
  int                 i;

  if (RUN_MODE_IS_SET(MODES_BaseName, psProperties->iRunMode))
  {
    if (psProperties->cBaseName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_BaseName, psProperties->cBaseName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Compress, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_Compress, psProperties->bCompress ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_DataType, psProperties->iRunMode))
  {
    if (psProperties->cDataType[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_DataType, psProperties->cDataType);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_DateTime, psProperties->iRunMode))
  {
    if (psProperties->cDateTime[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_DateTime, psProperties->cDateTime);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_FieldMask, psProperties->iRunMode))
  {
    if (psProperties->cMaskString[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_FieldMask, psProperties->cMaskString);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_GetAndExec, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_GetAndExec, psProperties->bGetAndExec ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_GetFileName, psProperties->iRunMode))
  {
    if (psProperties->cGetFileName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_GetFileName, psProperties->cGetFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_HashDirectories, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_HashDirectories, psProperties->bHashDirectories ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_LogDir, psProperties->iRunMode))
  {
    if (psProperties->cLogDirName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_LogDir, psProperties->cLogDirName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_LogFileName, psProperties->iRunMode))
  {
    if (psProperties->cLogFileName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_LogFileName, psProperties->cLogFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_MagicFile, psProperties->iRunMode))
  {
    if (psProperties->cMagicFileName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_MagicFile, psProperties->cMagicFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_MapRemoteFiles, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_MapRemoteFiles, psProperties->bMapRemoteFiles ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_MatchLimit, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%d", KEY_MatchLimit, psProperties->iMatchLimit);
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_NewLine, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_NewLine, (strcmp(psProperties->cNewLine, LF) == 0) ? "LF" : "CRLF");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_OutDir, psProperties->iRunMode))
  {
    if (psProperties->cOutDirName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_OutDir, psProperties->cOutDirName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_OutFileHash, psProperties->iRunMode))
  {
    if (psProperties->cOutFileHash[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_OutFileHash, psProperties->cOutFileHash);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_OutFileName, psProperties->iRunMode))
  {
    if (psProperties->cOutFileName[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_OutFileName, psProperties->cOutFileName);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_RequirePrivilege, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_RequirePrivilege, psProperties->bRequirePrivilege ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_RunType, psProperties->iRunMode))
  {
    if (psProperties->cRunType[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_RunType, psProperties->cRunType);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLGetURL, psProperties->iRunMode))
  {
    if (psProperties->ptGetURL)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s://%s:%s%s",
                KEY_URLGetURL,
#ifdef USE_SSL
                (psProperties->ptGetURL->iScheme == HTTP_SCHEME_HTTPS) ? "https" : "http",
#else
                "http",
#endif
                psProperties->ptGetURL->pcHost,
                psProperties->ptGetURL->pcPort,
                psProperties->ptGetURL->pcPath
              );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLGetRequest, psProperties->iRunMode))
  {
    if (psProperties->cURLGetRequest[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLGetRequest, psProperties->cURLGetRequest);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLPutSnapshot, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLPutSnapshot, psProperties->bURLPutSnapshot ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_URLPutURL, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot && psProperties->ptPutURL)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s://%s:%s%s",
                KEY_URLPutURL,
#ifdef USE_SSL
                (psProperties->ptPutURL->iScheme == HTTP_SCHEME_HTTPS) ? "https" : "http",
#else
                "http",
#endif
                psProperties->ptPutURL->pcHost,
                psProperties->ptPutURL->pcPort,
                psProperties->ptPutURL->pcPath
              );
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLCreateConfig, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLCreateConfig, psProperties->bURLCreateConfig ? "Y" : "N");
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLUnlinkOutput, psProperties->iRunMode))
  {
    if (psProperties->bURLPutSnapshot)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLUnlinkOutput, psProperties->bURLUnlinkOutput ? "Y" : "N");
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLAuthType, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLAuthType, (psProperties->iURLAuthType == HTTP_AUTH_TYPE_BASIC) ? "basic" : "none");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_URLUsername, psProperties->iRunMode))
  {
    if (psProperties->cURLUsername[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_URLUsername, psProperties->cURLUsername);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_URLPassword, psProperties->iRunMode))
  {
    if (psProperties->cURLPassword[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=########", KEY_URLPassword);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

#ifdef USE_SSL
  if (RUN_MODE_IS_SET(MODES_SSLVerifyPeerCert, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLVerifyPeerCert, psProperties->psSSLProperties->iVerifyPeerCert ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLBundledCAsFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert && psProperties->psSSLProperties->pcBundledCAsFile[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLBundledCAsFile, psProperties->psSSLProperties->pcBundledCAsFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLExpectedPeerCN, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert && psProperties->psSSLProperties->pcExpectedPeerCN[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLExpectedPeerCN, psProperties->psSSLProperties->pcExpectedPeerCN);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLMaxChainLength, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iVerifyPeerCert)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%d", KEY_SSLMaxChainLength, psProperties->psSSLProperties->iMaxChainLength);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLUseCertificate, psProperties->iRunMode))
  {
    snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLUseCertificate, psProperties->psSSLProperties->iUseCertificate ? "Y" : "N");
    MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
  }

  if (RUN_MODE_IS_SET(MODES_SSLPrivateKeyFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPrivateKeyFile[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPrivateKeyFile, psProperties->psSSLProperties->pcPrivateKeyFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPublicCertFile, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPublicCertFile[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_SSLPublicCertFile, psProperties->psSSLProperties->pcPublicCertFile);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_SSLPassPhrase, psProperties->iRunMode))
  {
    if (psProperties->psSSLProperties->iUseCertificate && psProperties->psSSLProperties->pcPassPhrase[0])
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=########", KEY_SSLPassPhrase);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }
#endif

  if (RUN_MODE_IS_SET(MODES_DigString, psProperties->iRunMode))
  {
    for (i = 0; i < 256; i++)
    {
      for (pSearchList = DigGetSearchList(i); pSearchList != NULL; pSearchList = pSearchList->pNext)
      {
        snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_DigString, pSearchList->cEscString);
        MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
      }
    }
  }

  if (RUN_MODE_IS_SET(MODES_Include, psProperties->iRunMode))
  {
    for (pList = psProperties->ptIncludeList; pList != NULL; pList = pList->pNext)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_Include, pList->cPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }

  if (RUN_MODE_IS_SET(MODES_Exclude, psProperties->iRunMode))
  {
    for (pList = psProperties->ptExcludeList; pList != NULL; pList = pList->pNext)
    {
      snprintf(cMessage, MESSAGE_SIZE, "%s=%s", KEY_Exclude, pList->cPath);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, cMessage);
    }
  }
}
