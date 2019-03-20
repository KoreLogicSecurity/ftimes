/*
 ***********************************************************************
 *
 * $Id: ftimes.c,v 1.3 2003/01/16 21:08:09 mavrik Exp $
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
 * Global variables
 *
 ***********************************************************************
 */
#ifdef FTimes_UNIX
static MASK_TABLE gMaskTable[] =
{
  { "dev",         "dev",         DEV_SET        },
  { "inode",       "inode",       INODE_SET      },
  { "mode",        "mode",        MODE_SET       },
  { "nlink",       "nlink",       NLINK_SET      },
  { "uid",         "uid",         UID_SET        },
  { "gid",         "gid",         GID_SET        },
  { "rdev",        "rdev",        RDEV_SET       },
  { "atime",       "atime",       ATIME_SET      },
  { "mtime",       "mtime",       MTIME_SET      },
  { "ctime",       "ctime",       CTIME_SET      },
  { "size",        "size",        SIZE_SET       },
  { {0},           {0},           RESERVED_SET   }, /* reserved */
  { "magic",       "magic",       MAGIC_SET      },
  { "md5",         "md5",         MD5_SET        },
};
#endif
#ifdef FTimes_WIN32
static MASK_TABLE gMaskTable[] =
{
  { "volume",      "volume",      VOLUME_SET     },
  { "findex",      "findex",      FINDEX_SET     },
  { "attributes",  "attributes",  ATTRIBUTES_SET },
  { "atime",       "atime|ams",   ATIME_SET      },
  { "mtime",       "mtime|mms",   MTIME_SET      },
  { "ctime",       "ctime|cms",   CTIME_SET      },
  { "chtime",      "chtime|chms", CHTIME_SET     },
  { "size",        "size",        SIZE_SET       },
  { "altstreams",  "altstreams",  ALTSTREAMS_SET },
  { {0},           {0},           RESERVED_SET   }, /* reserved */
  { "magic",       "magic",       MAGIC_SET      },
  { "md5",         "md5",         MD5_SET        }
};
#endif
static const int giMaskTableLength = sizeof(gMaskTable) / sizeof(gMaskTable[0]);

static FTIMES_PROPERTIES *gpsProperties;

#ifdef FTimes_WINNT
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
  const char          cRoutine[] = "Main()";
  char                ccLocalError[2][ERRBUF_SIZE];
  FTIMES_PROPERTIES  *psProperties;
  int                 iError;

  ccLocalError[0][0] = ccLocalError[1][0] = 0;

  iError = FTimesBootstrap(ccLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(ccLocalError[0], ERRBUF_SIZE, "%s: %s", cRoutine, ccLocalError[1]);
    ErrorHandler(XER_BootStrap, ccLocalError[0], ERROR_CRITICAL);
  }
  psProperties = FTimesGetPropertiesReference();

  iError = FTimesProcessArguments(psProperties, iArgumentCount, ppcArgumentVector, ccLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(ccLocalError[0], ERRBUF_SIZE, "%s: %s", cRoutine, ccLocalError[1]);
    ErrorHandler(XER_ProcessArguments, ccLocalError[0], ERROR_CRITICAL);
  }

  if (psProperties->iLastRunModeStage > 0)
  {
    iError = FTimesStagesLoop(psProperties, ccLocalError[1]);
    if (iError != ER_OK)
    {
      snprintf(ccLocalError[0], ERRBUF_SIZE, "%s: %s", cRoutine, ccLocalError[1]);
      ErrorHandler(iError, ccLocalError[0], ERROR_CRITICAL);
    }
  }

  iError = FTimesFinalStage(psProperties, ccLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(ccLocalError[0], ERRBUF_SIZE, "%s: %s", cRoutine, ccLocalError[1]);
    ErrorHandler(XER_FinalStage, ccLocalError[0], ERROR_CRITICAL);
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
  const char          cRoutine[] = "FTimesBootstrap()";
  char                cLocalError[ERRBUF_SIZE];
  FTIMES_PROPERTIES  *psProperties;
#ifdef FTimes_WINNT
  char               *pcMessage;
  int                 iError;
#endif

  cLocalError[0] = 0;

  /*-
   *********************************************************************
   *
   * Setup the Message Handler's output stream.
   *
   *********************************************************************
   */
  MessageSetOutputStream(stderr);

#ifdef FTimes_WINNT
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
    snprintf(pcError, ERRBUF_SIZE, "%s: _setmode(): stdout: %s", cRoutine, strerror(errno));
    return ER;
  }

  iError = _setmode(_fileno(stderr), _O_BINARY);
  if (iError == -1)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: _setmode(): stderr: %s", cRoutine, strerror(errno));
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
      snprintf(pcError, ERRBUF_SIZE, "%s: LoadLibrary(): Library = [NTdll.dll]: %s", cRoutine, pcMessage);
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
      snprintf(pcError, ERRBUF_SIZE, "%s: GetProcAddress(): Routine = [NtQueryInformationFile]: %s", cRoutine, pcMessage);
      return ER;
    }
  }
#endif
#ifdef USE_SSL
  SSLBoot();
#endif

  psProperties = FTimesNewProperties(cLocalError);
  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    return ER;
  }
  FTimesSetPropertiesReference(psProperties);

  return ER_OK;
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
  const char          cRoutine[] = "FTimesNewProperties()";
#ifdef USE_SSL
  char                cLocalError[ERRBUF_SIZE];
#endif
  FTIMES_PROPERTIES  *psProperties;

  /*
   *********************************************************************
   *
   * Allocate and clear memory for the properties structure.
   *
   *********************************************************************
   */
  psProperties = (FTIMES_PROPERTIES *) malloc(sizeof(FTIMES_PROPERTIES));
  if (psProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, strerror(errno));
    return NULL;
  }
  memset(psProperties, 0, sizeof(FTIMES_PROPERTIES));

  /*
   *********************************************************************
   *
   * Initialize field masks. Treat these variables as constants.
   *
   *********************************************************************
   */
  psProperties->ptMaskTable = gMaskTable;
  psProperties->iMaskTableLength = giMaskTableLength;

  /*-
   *********************************************************************
   *
   * Initialize variables that require a non-zero value.
   *
   *********************************************************************
   */
  psProperties->tStartTime = TimeGetTime(psProperties->cStartDate, psProperties->cStartTime, psProperties->cStartZone, psProperties->cDateTime);
  memcpy(psProperties->cRunDateTime, psProperties->cDateTime, FTIMES_TIME_SIZE);

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
#ifdef FTimes_WIN32
  strncpy(psProperties->cNewLine, CRLF, NEWLINE_LENGTH);
#endif
#ifdef FTimes_UNIX
  strncpy(psProperties->cNewLine, LF, NEWLINE_LENGTH);
#endif

  /*-
   *********************************************************************
   *
   * Initialize RunType variable.
   *
   *********************************************************************
   */
  strncpy(psProperties->cRunType, "baseline", RUNTYPE_BUFSIZE);

#ifdef USE_SSL
  /*-
   *********************************************************************
   *
   * Initialize SSL properties.
   *
   *********************************************************************
   */
  psProperties->psSSLProperties = SSLNewProperties(cLocalError);
  if (psProperties->psSSLProperties == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
    free(psProperties);
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
  const char          cRoutine[] = "FTimesProcessArguments()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  cLocalError[0] = 0;

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
    strcpy(psProperties->cDataType, FTIMES_CMPDATA);

    psProperties->piRunModeProcessArguments = CmpModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "CmpModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "CmpModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "CmpModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "CmpModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "CmpModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinishUp;

    psProperties->piRunModeFinalStage = CmpModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--decoder") == 0)
  {
    psProperties->iRunMode = FTIMES_DECODER;

    psProperties->piRunModeProcessArguments = DecoderProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DecoderInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DecoderCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DecoderFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DecoderWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DecoderFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinishUp;

    psProperties->piRunModeFinalStage = DecoderFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--digauto") == 0)
  {
    psProperties->iRunMode = FTIMES_DIGAUTO;
    strcpy(psProperties->cDataType, FTIMES_DIGDATA);

    psProperties->piRunModeProcessArguments = DigModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinishUp;

    psProperties->piRunModeFinalStage = DigModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--digfull") == 0)
  {
    psProperties->iRunMode = FTIMES_DIGFULL;
    strcpy(psProperties->cDataType, FTIMES_DIGDATA);

    psProperties->piRunModeProcessArguments = DigModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "DigModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinishUp;

    psProperties->piRunModeFinalStage = DigModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--getmode") == 0)
  {
    psProperties->iRunMode = FTIMES_GETMODE;
    psProperties->piRunModeProcessArguments = GetModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "GetModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "GetModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "GetModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "GetModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "GetModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinishUp;

    psProperties->piRunModeFinalStage = GetModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--mapauto") == 0)
  {
    psProperties->iRunMode = FTIMES_MAPAUTO;
    strcpy(psProperties->cDataType, FTIMES_MAPDATA);

    psProperties->piRunModeProcessArguments = MapModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinishUp;

    psProperties->piRunModeFinalStage = MapModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--mapfull") == 0)
  {
    psProperties->iRunMode = FTIMES_MAPFULL;
    strcpy(psProperties->cDataType, FTIMES_MAPDATA);

    psProperties->piRunModeProcessArguments = MapModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "MapModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinishUp;

    psProperties->piRunModeFinalStage = MapModeFinalStage;
  }
  else if (strcmp(psProperties->pcRunModeArgument, "--putmode") == 0)
  {
    psProperties->iRunMode = FTIMES_PUTMODE;

    psProperties->piRunModeProcessArguments = PutModeProcessArguments;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "PutModeInitialize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = PutModeInitialize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "PutModeCheckDependencies");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = PutModeCheckDependencies;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "PutModeFinalize");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = PutModeFinalize;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "PutModeWorkHorse");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = PutModeWorkHorse;

    strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].cDescription, "PutModeFinishUp");
    psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
    psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = PutModeFinishUp;

    psProperties->piRunModeFinalStage = PutModeFinalStage;
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
    iError = psProperties->piRunModeProcessArguments(psProperties, iArgumentCount, (iArgumentCount) ? &ppcArgumentVector[2] : NULL, cLocalError);
    if (iError != ER_OK)
    {
      if (iError == ER_Usage)
      {
        FTimesUsage();
      }
      else
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  const char          cRoutine[] = "FTimesStagesLoop()";
  char                cLocalError[ERRBUF_SIZE];
  char                cMessage[MESSAGE_SIZE];
  int                 i;
  int                 iError;

  cLocalError[0] = 0;

  /*-
   *******************************************************************
   *
   * Display some basic information.
   *
   *******************************************************************
   */
  snprintf(cMessage, MESSAGE_SIZE, "Program=%s %s %s", PROGRAM_NAME, VERSION, psProperties->pcRunModeArgument);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "SystemOS=%s", SupportGetSystemOS());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

  snprintf(cMessage, MESSAGE_SIZE, "Hostname=%s", SupportGetHostname());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_EXECDATA_STRING, cMessage);

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
      snprintf(cMessage, MESSAGE_SIZE, "Stage%d=%s", i + 1, psProperties->sRunModeStages[i].cDescription);
      MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_LANDMARK, MESSAGE_LANDMARK_STRING, cMessage);
      iError = psProperties->sRunModeStages[i].piRoutine(psProperties, cLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
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
  const char          cRoutine[] = "FTimesFinalStage()";
  char                cLocalError[ERRBUF_SIZE];
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
    iError = psProperties->piRunModeFinalStage(psProperties, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: %s", cRoutine, cLocalError);
      return ER;
    }
  }

#ifdef FTimes_WINNT
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
  fprintf(stderr, "       ftimes --getmode file [-l level]\n");
  fprintf(stderr, "       ftimes --mapauto mask [-l level] [list]\n");
  fprintf(stderr, "       ftimes --mapfull file [-l level] [list]\n");
  fprintf(stderr, "       ftimes --putmode file [-l level]\n");
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
  fprintf(stdout, "%s %s\n", PROGRAM_NAME, VERSION);
  return ER_OK;;
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
 * FTimesCreateConfigFile
 *
 ***********************************************************************
 */
int
FTimesCreateConfigFile(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          cRoutine[] = "FTimesCreateConfigFile()";
  FILE               *pFile;

  if ((pFile = fopen(psProperties->cCfgFileName, "wb+")) == NULL)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: CfgFile = [%s]: %s", cRoutine, psProperties->cCfgFileName, strerror(errno));
    return ER_fopen;
  }

  fprintf(pFile, "#%s", psProperties->cNewLine);
  fprintf(pFile, "# This file contains most of the directives necessary to upload%s", psProperties->cNewLine);
  fprintf(pFile, "# the associated snapshot to a remote server. Before attempting an%s", psProperties->cNewLine);
  fprintf(pFile, "# upload, review the following information for completeness and%s", psProperties->cNewLine);
  fprintf(pFile, "# correctness. You are required to supply the server's URL and any%s", psProperties->cNewLine);
  fprintf(pFile, "# necessary credentials.%s", psProperties->cNewLine);
  fprintf(pFile, "#%s", psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_BaseName, FTIMES_SEPARATOR, psProperties->cBaseName, psProperties->cNewLine);
  fprintf(pFile, "#%s", psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_LogFileName, FTIMES_SEPARATOR, psProperties->cLogFileName, psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_OutFileName, FTIMES_SEPARATOR, psProperties->cOutFileName, psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_OutFileHash, FTIMES_SEPARATOR, psProperties->cOutFileHash, psProperties->cNewLine);
  fprintf(pFile, "#%s", psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_DataType, FTIMES_SEPARATOR, psProperties->cDataType, psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_DateTime, FTIMES_SEPARATOR, psProperties->cDateTime, psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_FieldMask, FTIMES_SEPARATOR, psProperties->cMaskString, psProperties->cNewLine);
  fprintf(pFile, "%s%s%s%s", KEY_RunType, FTIMES_SEPARATOR, psProperties->cRunType, psProperties->cNewLine);
  fprintf(pFile, "#%s", psProperties->cNewLine);
  fprintf(pFile, "%s%s%s://%s:%s%s%s",
           KEY_URLPutURL,
           FTIMES_SEPARATOR,
#ifdef USE_SSL
           (psProperties->ptPutURL->iScheme == HTTP_SCHEME_HTTPS) ? "https" : "http",
#else
           "http",
#endif
           psProperties->ptPutURL->pcHost,
           psProperties->ptPutURL->pcPort,
           psProperties->ptPutURL->pcPath,
           psProperties->cNewLine
         );
#ifdef USE_SSL
  fprintf(pFile, "#%s", psProperties->cNewLine);
  if (psProperties->ptPutURL->iScheme == HTTP_SCHEME_HTTPS)
  {
    fprintf(pFile, "%s%s%s%s", KEY_SSLUseCertificate, FTIMES_SEPARATOR, (psProperties->psSSLProperties->iUseCertificate) ? "Y" : "N", psProperties->cNewLine);
    if (psProperties->psSSLProperties->iUseCertificate)
    {
      fprintf(pFile, "%s%s%s%s", KEY_SSLPublicCertFile, FTIMES_SEPARATOR, psProperties->psSSLProperties->pcPublicCertFile, psProperties->cNewLine);
      fprintf(pFile, "%s%s%s%s", KEY_SSLPrivateKeyFile, FTIMES_SEPARATOR, psProperties->psSSLProperties->pcPrivateKeyFile, psProperties->cNewLine);
    }

    fprintf(pFile, "%s%s%s%s", KEY_SSLVerifyPeerCert, FTIMES_SEPARATOR, (psProperties->psSSLProperties->iVerifyPeerCert) ? "Y" : "N", psProperties->cNewLine);
    if (psProperties->psSSLProperties->iVerifyPeerCert)
    {
      fprintf(pFile, "%s%s%s%s", KEY_SSLBundledCAsFile, FTIMES_SEPARATOR, psProperties->psSSLProperties->pcBundledCAsFile, psProperties->cNewLine);
      fprintf(pFile, "%s%s%s%s", KEY_SSLExpectedPeerCN, FTIMES_SEPARATOR, psProperties->psSSLProperties->pcExpectedPeerCN, psProperties->cNewLine);
      fprintf(pFile, "%s%s%d%s", KEY_SSLMaxChainLength, FTIMES_SEPARATOR, psProperties->psSSLProperties->iMaxChainLength, psProperties->cNewLine);
    }
  }
#endif
  fprintf(pFile, "#%s", psProperties->cNewLine);
  fclose(pFile);

  return ER_OK;
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
  const char          cRoutine[] = "FTimesEraseFiles()";
  char                cLocalError[ERRBUF_SIZE];
  int                 iError;

  iError = SupportEraseFile(psProperties->cLogFileName, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, psProperties->cLogFileName, cLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
  iError = SupportEraseFile(psProperties->cOutFileName, cLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, psProperties->cOutFileName, cLocalError);
    ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
  }
  if (psProperties->bURLCreateConfig)
  {
    iError = SupportEraseFile(psProperties->cCfgFileName, cLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: File = [%s]: %s", cRoutine, psProperties->cCfgFileName, cLocalError);
      ErrorHandler(ER_Warning, pcError, ERROR_WARNING);
    }
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
  const char          cRoutine[] = "SSLCheckDependencies()";

  if (psProperties->iUseCertificate)
  {
    if (psProperties->pcPublicCertFile == NULL || psProperties->pcPublicCertFile[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing SSLPublicCertFile.", cRoutine);
      return ER;
    }

    if (psProperties->pcPrivateKeyFile == NULL || psProperties->pcPrivateKeyFile[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing SSLPrivateKeyFile.", cRoutine);
      return ER;
    }
  }

  if (psProperties->iVerifyPeerCert)
  {
    if (psProperties->pcBundledCAsFile == NULL || psProperties->pcBundledCAsFile[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing SSLBundledCAsFile.", cRoutine);
      return ER;
    }

    if (psProperties->pcExpectedPeerCN == NULL || psProperties->pcExpectedPeerCN[0] == 0)
    {
      snprintf(pcError, ERRBUF_SIZE, "%s: Missing SSLExpectedPeerCN.", cRoutine);
      return ER;
    }
  }

  return ER_OK;
}
#endif
