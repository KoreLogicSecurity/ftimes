/*-
 ***********************************************************************
 *
 * $Id: ftimes.c,v 1.31 2007/02/23 00:22:35 mavrik Exp $
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
 * Global variables
 *
 ***********************************************************************
 */
static FTIMES_PROPERTIES *gpsProperties;

#ifdef WINNT
HINSTANCE             NtdllHandle;
NQIF                  NtdllNQIF;
#endif

/*-
 ***********************************************************************
 *
 * Main
 *
 ***********************************************************************
 */
int
main(int iArgumentCount, char *ppcArgumentVector[])
{
  const char          acRoutine[] = "Main()";
  char                aacLocalError[2][MESSAGE_SIZE];
  FTIMES_PROPERTIES  *psProperties;
  int                 iError;

  aacLocalError[0][0] = aacLocalError[1][0] = 0;

  iError = FTimesBootstrap(aacLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(aacLocalError[0], MESSAGE_SIZE, "%s: %s", acRoutine, aacLocalError[1]);
    ErrorHandler(XER_BootStrap, aacLocalError[0], ERROR_CRITICAL);
  }
  psProperties = FTimesGetPropertiesReference();

  iError = FTimesProcessArguments(psProperties, iArgumentCount, ppcArgumentVector, aacLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(aacLocalError[0], MESSAGE_SIZE, "%s: %s", acRoutine, aacLocalError[1]);
    ErrorHandler(XER_ProcessArguments, aacLocalError[0], ERROR_CRITICAL);
  }

  if (psProperties->iLastRunModeStage > 0)
  {
    iError = FTimesStagesLoop(psProperties, aacLocalError[1]);
    if (iError != ER_OK)
    {
      snprintf(aacLocalError[0], MESSAGE_SIZE, "%s: %s", acRoutine, aacLocalError[1]);
      ErrorHandler(iError, aacLocalError[0], ERROR_CRITICAL);
    }
  }

  iError = FTimesFinalStage(psProperties, aacLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(aacLocalError[0], MESSAGE_SIZE, "%s: %s", acRoutine, aacLocalError[1]);
    ErrorHandler(XER_FinalStage, aacLocalError[0], ERROR_CRITICAL);
  }

  return XER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesBootstrap
 *
 ***********************************************************************
 */
int
FTimesBootstrap(char *pcError)
{
  const char          acRoutine[] = "FTimesBootstrap()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FTIMES_PROPERTIES  *psProperties;
#ifdef WINNT
  char               *pcMessage;
  int                 iError;
#endif

  /*-
   *********************************************************************
   *
   * Require certain truths. If these tests produce an error, there is
   * something wrong with the code.
   *
   *********************************************************************
   */
  if (DECODE_FIELD_COUNT != MaskGetTableLength(MASK_RUNMODE_TYPE_CMP))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: DecodeFieldCount/CmpMaskTableSize mismatch!: %d != %d", acRoutine, DECODE_FIELD_COUNT, MaskGetTableLength(MASK_RUNMODE_TYPE_CMP));
    return ER;
  }

  if (DECODE_FIELD_COUNT != DecodeGetTableLength())
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: DecodeFieldCount/DecodeTableSize mismatch!: %d != %d", acRoutine, DECODE_FIELD_COUNT, DecodeGetTableLength());
    return ER;
  }

  if (FTIMES_MAX_LINE != DECODE_MAX_LINE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: FTimesMaxLine/DecodeMaxLine mismatch!: %d != %d", acRoutine, FTIMES_MAX_LINE, DECODE_MAX_LINE);
    return ER;
  }

  if (FTIMES_MAX_LINE != CMP_MAX_LINE)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: FTimesMaxLine/CmpMaxLine mismatch!: %d != %d", acRoutine, FTIMES_MAX_LINE, CMP_MAX_LINE);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Setup the Message Handler's output stream.
   *
   *********************************************************************
   */
  MessageSetOutputStream(stderr);

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Suppress critical-error-handler message boxes.
   *
   *********************************************************************
   */
  SetErrorMode(SEM_FAILCRITICALERRORS);

  /*-
   *********************************************************************
   *
   * Put stdout/stderr in binary mode to prevent CRLF mappings.
   *
   *********************************************************************
   */
  iError = _setmode(_fileno(stdout), _O_BINARY);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: _setmode(): stdout: %s", acRoutine, strerror(errno));
    return ER;
  }

  iError = _setmode(_fileno(stderr), _O_BINARY);
  if (iError == -1)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: _setmode(): stderr: %s", acRoutine, strerror(errno));
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Load NT's native library, if necessary.
   *
   *********************************************************************
   */
  if (NtdllHandle == NULL)
  {
    NtdllHandle = LoadLibrary("NTdll.dll");
    if (NtdllHandle == NULL)
    {
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: LoadLibrary(): Library = [NTdll.dll]: %s", acRoutine, pcMessage);
      return ER;
    }
  }

  if (NtdllNQIF == NULL)
  {
    NtdllNQIF = (NQIF) GetProcAddress(NtdllHandle, "NtQueryInformationFile");
    if (NtdllNQIF == NULL)
    {
      FreeLibrary(NtdllHandle);
      ErrorFormatWin32Error(&pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: GetProcAddress(): Routine = [NtQueryInformationFile]: %s", acRoutine, pcMessage);
      return ER;
    }
  }
#endif
#ifdef USE_SSL
  SSLBoot();
#endif

  psProperties = FTimesNewProperties(acLocalError);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  FTimesSetPropertiesReference(psProperties);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesFreeProperties
 *
 ***********************************************************************
 */
void
FTimesFreeProperties(FTIMES_PROPERTIES *psProperties)
{
  if (psProperties != NULL)
  {
    DecodeFreeSnapshotContext(psProperties->psBaselineContext);
    DecodeFreeSnapshotContext(psProperties->psSnapshotContext);
#ifdef USE_SSL
    SSLFreeProperties(psProperties->psSSLProperties);
#endif
    free(psProperties);
  }
}


/*-
 ***********************************************************************
 *
 * FTimesNewProperties
 *
 ***********************************************************************
 */
FTIMES_PROPERTIES *
FTimesNewProperties(char *pcError)
{
  const char          acRoutine[] = "FTimesNewProperties()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  FTIMES_PROPERTIES  *psProperties;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (FTIMES_PROPERTIES *) calloc(sizeof(FTIMES_PROPERTIES), 1);
  if (psProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */
  psProperties->tStartTime = TimeGetTime(psProperties->acStartDate, psProperties->acStartTime, psProperties->acStartZone, psProperties->acDateTime);
  memcpy(psProperties->acRunDateTime, psProperties->acDateTime, FTIMES_TIME_SIZE);

  /*
   *********************************************************************
   *
   * Initialize function pointers.
   *
   *********************************************************************
   */
  psProperties->piDevelopMapOutput = DevelopNoOutput;

  /*-
   *********************************************************************
   *
   * Initialize LogLevel variable.
   *
   *********************************************************************
   */
  psProperties->iLogLevel = MESSAGE_LANDMARK;
  MessageSetLogLevel(psProperties->iLogLevel);

  /*-
   *********************************************************************
   *
   * Initialize NewLine variable.
   *
   *********************************************************************
   */
#ifdef WIN32
  strncpy(psProperties->acNewLine, CRLF, NEWLINE_LENGTH);
#else
  strncpy(psProperties->acNewLine, LF, NEWLINE_LENGTH);
#endif

  /*-
   *********************************************************************
   *
   * Initialize Pid variable.
   *
   *********************************************************************
   */
  snprintf(psProperties->acPid, FTIMES_PID_SIZE, "%d", (int) getpid());

  /*-
   *********************************************************************
   *
   * Initialize RunType variable.
   *
   *********************************************************************
   */
  strncpy(psProperties->acRunType, "baseline", RUNTYPE_BUFSIZE);

  /*-
   *********************************************************************
   *
   * Initialize EnableRecursion variable.
   *
   *********************************************************************
   */
  psProperties->bEnableRecursion = TRUE;

  /*-
   *********************************************************************
   *
   * Initialize Analyze*Size variables.
   *
   *********************************************************************
   */
  psProperties->iAnalyzeBlockSize = AnalyzeGetBlockSize();
  psProperties->iAnalyzeCarrySize = AnalyzeGetCarrySize();
#ifdef USE_XMAGIC
  psProperties->iAnalyzeStepSize = AnalyzeGetStepSize();
#endif

  /*-
   *********************************************************************
   *
   * Initialize baseline context.
   *
   *********************************************************************
   */
  psProperties->psBaselineContext = DecodeNewSnapshotContext(acLocalError);
  if (psProperties->psBaselineContext == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    FTimesFreeProperties(psProperties);
    return NULL;
  }

  /*-
   *********************************************************************
   *
   * Initialize snapshot context.
   *
   *********************************************************************
   */
  psProperties->psSnapshotContext = DecodeNewSnapshotContext(acLocalError);
  if (psProperties->psSnapshotContext == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    FTimesFreeProperties(psProperties);
    return NULL;
  }

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Initialize SSL properties.
   *
   *********************************************************************
   */
  psProperties->psSSLProperties = SSLNewProperties(acLocalError);
  if (psProperties->psSSLProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    FTimesFreeProperties(psProperties);
    return NULL;
  }
#endif

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesProcessArguments
 *
 ***********************************************************************
 */
int
FTimesProcessArguments(FTIMES_PROPERTIES *psProperties, int iArgumentCount, char *ppcArgumentVector[], char *pcError)
{
  const char          acRoutine[] = "FTimesProcessArguments()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  psProperties->pcProgram = ppcArgumentVector[0];

  iArgumentCount--;

  if (iArgumentCount < 1)
  {
    FTimesUsage();
  }

  psProperties->pcRunModeArgument = ppcArgumentVector[1];

  iArgumentCount--;

  /*-
   *********************************************************************
   *
   * Set the DataType and RunMode function pointers.
   *
   *********************************************************************
   */
  if (strcmp(psProperties->pcRunModeArgument, "--cfgtest") == 0)
  {
    psProperties->iRunMode = FTIMES_CFGTEST;

    psProperties->piRunModeProcessArguments = CfgTestProcessArguments;

    psProperties->piRunModeFinalStage = PropertiesTestFile;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--compare") == 0)
  {
    psProperties->iRunMode = FTIMES_CMPMODE;
    strcpy(psProperties->acDataType, FTIMES_CMPDATA);

    psProperties->piRunModeProcessArguments = CmpModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinishUp;

    psProperties->piRunModeFinalStage = CmpModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--decoder") == 0)
  {
    psProperties->iRunMode = FTIMES_DECODER;

    psProperties->piRunModeProcessArguments = DecoderProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinishUp;

    psProperties->piRunModeFinalStage = DecoderFinalStage;
  }
  else if (strncmp(psProperties->pcRunModeArgument, "--dig", 5) == 0)
  {
    if (strcmp(psProperties->pcRunModeArgument, "--digauto") == 0)
    {
      psProperties->iRunMode = FTIMES_DIGAUTO;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--digfull") == 0)
    {
      psProperties->iRunMode = FTIMES_DIGFULL;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--diglean") == 0)
    {
      psProperties->iRunMode = FTIMES_DIGLEAN;
    }
    else
    {
      FTimesUsage();
    }

    strcpy(psProperties->acDataType, FTIMES_DIGDATA);

    psProperties->piRunModeProcessArguments = DigModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinishUp;

    psProperties->piRunModeFinalStage = DigModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--getmode") == 0)
  {
    psProperties->iRunMode = FTIMES_GETMODE;
    psProperties->piRunModeProcessArguments = GetModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinishUp;

    psProperties->piRunModeFinalStage = GetModeFinalStage;
  }
  else if (strncmp(psProperties->pcRunModeArgument, "--map", 5) == 0)
  {
    if (strcmp(psProperties->pcRunModeArgument, "--mapauto") == 0)
    {
      psProperties->iRunMode = FTIMES_MAPAUTO;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--mapfull") == 0)
    {
      psProperties->iRunMode = FTIMES_MAPFULL;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--maplean") == 0)
    {
      psProperties->iRunMode = FTIMES_MAPLEAN;
    }
    else
    {
      FTimesUsage();
    }

    strcpy(psProperties->acDataType, FTIMES_MAPDATA);

    psProperties->piRunModeProcessArguments = MapModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinishUp;

    psProperties->piRunModeFinalStage = MapModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--version") == 0)
  {
    psProperties->iRunMode = FTIMES_VERSION;
    psProperties->piRunModeFinalStage = FTimesVersion;
  }
  else
  {
    FTimesUsage();
  }

  /*-
   *********************************************************************
   *
   * Invoke the RunMode specific routine.
   *
   *********************************************************************
   */
  if (psProperties->piRunModeProcessArguments)
  {
    iError = psProperties->piRunModeProcessArguments(psProperties, iArgumentCount, (iArgumentCount) ? &ppcArgumentVector[2] : NULL, acLocalError);
    if (iError != ER_OK)
    {
      if (iError == ER_Usage)
      {
        FTimesUsage();
      }
      else
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return ER;
      }
    }
  }

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesStagesLoop
 *
 ***********************************************************************
 */
int
FTimesStagesLoop(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesStagesLoop()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  char                acMessage[MESSAGE_SIZE];
  int                 i;
  int                 iError;

  /*-
   *******************************************************************
   *
   * Display some basic information.
   *
   *******************************************************************
   */
  snprintf(acMessage, MESSAGE_SIZE, "Program=%s %s", SupportGetMyVersion(), psProperties->pcRunModeArgument);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "SystemOS=%s", SupportGetSystemOS());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "Hostname=%s", SupportGetHostname());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  /*-
   *******************************************************************
   *
   * Loop over the RunMode stages.
   *
   *******************************************************************
   */
  for (i = 0; i < psProperties->iLastRunModeStage; i++)
  {
    if (psProperties->sRunModeStages[i].piRoutine != NULL)
    {
      snprintf(acMessage, MESSAGE_SIZE, "Stage%d=%s", i + 1, psProperties->sRunModeStages[i].acDescription);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_LANDMARK, MESSAGE_LANDMARK_STRING, acMessage);
      iError = psProperties->sRunModeStages[i].piRoutine(psProperties, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return psProperties->sRunModeStages[i].iError;
      }
    }
  }

  MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_ALWAYSON, NULL, NULL);

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesFinalStage
 *
 ***********************************************************************
 */
int
FTimesFinalStage(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesFinalStage()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  /*-
   *********************************************************************
   *
   * Flush the Message Handler, and close the LogFile stream.
   *
   *********************************************************************
   */
  if (psProperties->pFileLog && psProperties->pFileLog != stderr)
  {
    MessageHandler(MESSAGE_FLUSH_IT, MESSAGE_INFORMATION, NULL, NULL);
    fclose(psProperties->pFileLog);
    psProperties->pFileLog = NULL;
  }

  /*-
   *********************************************************************
   *
   * Invoke the RunMode specific routine.
   *
   *********************************************************************
   */
  if (psProperties->piRunModeFinalStage)
  {
    iError = psProperties->piRunModeFinalStage(psProperties, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
  }

#ifdef WINNT
  /*-
   *********************************************************************
   *
   * Release any library handles.
   *
   *********************************************************************
   */
  if (NtdllHandle)
  {
    FreeLibrary(NtdllHandle);
  }
#endif

  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesUsage
 *
 ***********************************************************************
 */
void
FTimesUsage(void)
{
  fprintf(stderr, "\n");
  fprintf(stderr, "Usage: ftimes --cfgtest file mode [-s]\n");
  fprintf(stderr, "       ftimes --compare mask baseline snapshot [-l level]\n");
  fprintf(stderr, "       ftimes --decoder snapshot [-l level]\n");
  fprintf(stderr, "       ftimes --digauto file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --digfull file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --diglean file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --getmode file [-l level]\n");
  fprintf(stderr, "       ftimes --mapauto mask [-l level] [list]\n");
  fprintf(stderr, "       ftimes --mapfull file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --maplean file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --version\n");
  fprintf(stderr, "\n");
  exit(XER_Usage);
}


/*-
 ***********************************************************************
 *
 * FTimesVersion
 *
 ***********************************************************************
 */
int
FTimesVersion(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  fprintf(stdout, "%s\n", SupportGetMyVersion());
  return ER_OK;
}


/*-
 ***********************************************************************
 *
 * FTimesSetPropertiesReference
 *
 ***********************************************************************
 */
void
FTimesSetPropertiesReference(FTIMES_PROPERTIES *psProperties)
{
  gpsProperties = psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesGetPropertiesReference
 *
 ***********************************************************************
 */
FTIMES_PROPERTIES *
FTimesGetPropertiesReference(void)
{
  return gpsProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesEraseFiles
 *
 ***********************************************************************
 */
void
FTimesEraseFiles(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesEraseFiles()";
  char                acLocalError[MESSAGE_SIZE] = { 0 };
  int                 iError;

  iError = SupportEraseFile(psProperties->acLogFileName, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, psProperties->acLogFileName, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
  iError = SupportEraseFile(psProperties->acOutFileName, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: File = [%s]: %s", acRoutine, psProperties->acOutFileName, acLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
}


#ifdef USE_SSL
/*-
 ***********************************************************************
 *
 * SSLCheckDependencies
 *
 ***********************************************************************
 */
int
SSLCheckDependencies(SSL_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "SSLCheckDependencies()";

  if (psProperties->iUseCertificate)
  {
    if (psProperties->pcPublicCertFile == NULL || psProperties->pcPublicCertFile[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing SSLPublicCertFile.", acRoutine);
      return ER;
    }

    if (psProperties->pcPrivateKeyFile == NULL || psProperties->pcPrivateKeyFile[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing SSLPrivateKeyFile.", acRoutine);
      return ER;
    }
  }

  if (psProperties->iVerifyPeerCert)
  {
    if (psProperties->pcBundledCAsFile == NULL || psProperties->pcBundledCAsFile[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing SSLBundledCAsFile.", acRoutine);
      return ER;
    }

    if (psProperties->pcExpectedPeerCN == NULL || psProperties->pcExpectedPeerCN[0] == 0)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Missing SSLExpectedPeerCN.", acRoutine);
      return ER;
    }
  }

  return ER_OK;
}
#endif
