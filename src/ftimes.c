/*-
 ***********************************************************************
 *
 * $Id: ftimes.c,v 1.60 2013/02/14 16:55:20 mavrik Exp $
 *
 ***********************************************************************
 *
 * Copyright 2000-2013 The FTimes Project, All Rights Reserved.
 *
 ***********************************************************************
 */
#include "all-includes.h"

#ifdef USE_EMBEDDED_PERL
/*-
 ***********************************************************************
 *
 * Glue code for embedded Perl (taken from perlembed.pod).
 *
 ***********************************************************************
 */
EXTERN_C void xs_init (pTHX);
EXTERN_C void boot_DynaLoader (pTHX_ CV *cv);

EXTERN_C void
xs_init(pTHX)
{
  char               *pcFile = __FILE__;

  newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, pcFile);
}
#endif

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

  iError = FTimesFinalize(psProperties, aacLocalError[1]);
  if (iError != ER_OK)
  {
    snprintf(aacLocalError[0], MESSAGE_SIZE, "%s: %s", acRoutine, aacLocalError[1]);
    ErrorHandler(XER_Finalize, aacLocalError[0], ERROR_CRITICAL);
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
  char                acLocalError[MESSAGE_SIZE] = "";
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
      ErrorFormatWinxError(GetLastError(), &pcMessage);
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
      ErrorFormatWinxError(GetLastError(), &pcMessage);
      snprintf(pcError, MESSAGE_SIZE, "%s: GetProcAddress(): Routine = [NtQueryInformationFile]: %s", acRoutine, pcMessage);
      return ER;
    }
  }
#endif
#ifdef USE_SSL
  SslBoot();
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
 * FTimesFinalize
 *
 ***********************************************************************
 */
int
FTimesFinalize(FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesFinalize()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
#ifdef USE_EMBEDDED_PERL
  char               *pacArgumentVector[] = { "", "-e", "0" };
  char              **ppcArgumentVector = pacArgumentVector;
  int                 iArgumentCount = 3;
  int                 iPerlStatus = 0;
  char                acScriptHandler[] =
"\n\
use strict; use warnings;\n\
\n\
######################################################################\n\
#\n\
# Redefine CORE::GLOBAL::exit() so that any calls to exit() made from\n\
# within Perl scripts or modules won't cause the interpreter to exit,\n\
# which would, in turn, cause ftimes to exit as well.\n\
#\n\
######################################################################\n\
\n\
package main;\n\
\n\
use subs qw(CORE::GLOBAL::exit);\n\
\n\
sub CORE::GLOBAL::exit { die(qq(ExitCode='$_[0]'\\n)); }\n\
\n\
######################################################################\n\
#\n\
# Embed::Persistent\n\
#\n\
######################################################################\n\
\n\
package Embed::Persistent;\n\
\n\
use Symbol qw(delete_package);\n\
\n\
our %hMTimeDeltas;\n\
\n\
######################################################################\n\
#\n\
# EvalScript\n\
#\n\
######################################################################\n\
\n\
sub EvalScript\n\
{\n\
  my ($sScript, @aArgumentVector) = @_;\n\
\n\
  my $sPackageName = LoadScript($sScript);\n\
  if (!defined($sPackageName))\n\
  {\n\
    return undef; # NOTE: $@ is set by LoadScript().\n\
  }\n\
\n\
  eval { $sPackageName->Worker(@ARGV=@aArgumentVector); };\n\
  if ($@) # NOTE: $@ becomes ERRSV over in C land.\n\
  {\n\
    $@ =~ s/[\\t\\r\\n]+/ /g; $@ =~ s/\\s+/ /g; $@ =~ s/^\\s+//; $@ =~ s/\\s+$//;\n\
    if ($@ =~ /^ExitCode='(\\d+)'/)\n\
    {\n\
      $@ = undef; # The script used the approved exit mechanism (i.e., it called exit() with an integer value), so clear the eval error.\n\
      return int($1); # Cast the return value as an integer so that it's handled correctly over in C land.\n\
    }\n\
    return undef; # This script didn't use the approved exit mechanism or failed for some other reason.\n\
  }\n\
\n\
  return 0; # The script ran without error, but didn't use the approved exit mechanism.\n\
}\n\
\n\
\n\
######################################################################\n\
#\n\
# GetPackageName\n\
#\n\
######################################################################\n\
\n\
sub GetPackageName\n\
{\n\
  my $sString = (defined($_[0]) && length($_[0]) > 0) ? $_[0] : qq(anonymous);\n\
\n\
  my @aPackageElements = qw(Embed Persistent);\n\
  foreach my $sElement (split('/', $sString))\n\
  {\n\
    next unless (defined($sElement) && length($sElement) > 0);\n\
    $sElement =~ s/([^0-9A-Za-z])/sprintf(qq(_%02x_), unpack('C', $1))/seg;\n\
    $sElement =~ s/^([0-9])/sprintf(qq(_%02x_), unpack('C', $1))/seg;\n\
    push(@aPackageElements, $sElement);\n\
  }\n\
\n\
  return join(qq(::), @aPackageElements);\n\
}\n\
\n\
\n\
######################################################################\n\
#\n\
# LoadScript\n\
# \n\
######################################################################\n\
\n\
sub LoadScript\n\
{\n\
  my ($sScript) = @_;\n\
  if (!defined($sScript))\n\
  {\n\
    $@ = qq(script not defined);\n\
    return undef;\n\
  }\n\
  my @aPackageElements = qw(Embed Persistent);\n\
  foreach my $sElement (split(/\\//, $sScript))\n\
  {\n\
    next unless (defined($sElement) && length($sElement) > 0);\n\
    $sElement =~ s/([^0-9A-Za-z])/sprintf(qq(_%02x_), unpack('C', $1))/seg;\n\
    $sElement =~ s/^(\\d)/sprintf(qq(_%02x_), unpack('C', $1))/seg;\n\
    push(@aPackageElements, $sElement);\n\
  }\n\
  my $sPackageName = join(qq(::), @aPackageElements);\n\
\n\
  if (!-f $sScript || !-r _)\n\
  {\n\
    $@ = qq(script does not exist, is not regular, or is unreadable);\n\
    return undef;\n\
  }\n\
  my $sMTimeDelta = -M $sScript;\n\
\n\
  if (!defined($hMTimeDeltas{$sPackageName}{'MTimeDelta'}) || $hMTimeDeltas{$sPackageName}{'MTimeDelta'} > $sMTimeDelta)\n\
  {\n\
    delete_package($sPackageName) if (defined($hMTimeDeltas{$sPackageName}{'MTimeDelta'}));\n\
    local *FH;\n\
    if (!open(FH, $sScript))\n\
    {\n\
      $@ = qq(script could not be opened ($!));\n\
      return undef;\n\
    }\n\
    my $sScriptSource;\n\
    while (my $sLine = <FH>)\n\
    {\n\
      last if ($sLine =~ /__(?:DATA|END)__/o);\n\
      $sScriptSource .= $sLine;\n\
    }\n\
    close(FH);\n\
    eval qq # Convert the script into a worker subroutine within its own unique package.\n\
    {\n\
      package $sPackageName;\n\
      sub Worker\n\
      {\n\
        $sScriptSource;\n\
      }\n\
    };\n\
    if ($@)\n\
    {\n\
      $@ =~ s/[\\t\\r\\n]+/ /g; $@ =~ s/\\s+/ /g; $@ =~ s/^\\s+//; $@ =~ s/\\s+$//;\n\
      return undef;\n\
    }\n\
    $hMTimeDeltas{$sPackageName}{'MTimeDelta'} = $sMTimeDelta;\n\
  }\n\
\n\
  return $sPackageName;\n\
}\n\
\n\
1;\n\
";
#endif

  /*-
   *********************************************************************
   *
   * Seed the random number generator, and generate a nonce.
   *
   *********************************************************************
   */
  TimeGetTimeValue(&psProperties->tvSRGEpoch);
  iError = SeedRandom((unsigned long) psProperties->tvJobEpoch.tv_usec, (unsigned long) psProperties->tvSRGEpoch.tv_usec, acLocalError);
  if (iError != ER_OK)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

  psProperties->pcNonce = MakeNonce(acLocalError);
  if (psProperties->pcNonce == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }

#ifdef USE_EMBEDDED_PERL
if (RUN_MODE_IS_SET(FTIMES_DIGMADMAP, psProperties->iRunMode))
{
  /*-
   *********************************************************************
   *
   * Initialize the Perl interpreter.
   *
   *********************************************************************
   */
  PERL_SYS_INIT(&iArgumentCount, &ppcArgumentVector);
  psProperties->psMyPerl = perl_alloc();
  if (psProperties->psMyPerl == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: perl_alloc(): Unable to allocate a Perl interpreter.\n", acRoutine);
    return ER;
  }
  PL_perl_destruct_level = 1; /* Setting this to 1 makes everything squeaky clean according to perlembed. */
  perl_construct(psProperties->psMyPerl);
  iPerlStatus = perl_parse(psProperties->psMyPerl, xs_init, iArgumentCount, ppcArgumentVector, NULL);
  if (iPerlStatus != 0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: perl_parse(): Failed to initialize the Perl interpreter.", acRoutine);
    return ER;
  }

  /*-
   *********************************************************************
   *
   * Initialize the script handler.
   *
   *********************************************************************
   */
  eval_sv(newSVpv(acScriptHandler, 0), G_KEEPERR);
  if (SvTRUE(ERRSV))
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: eval_sv(): Failed to initialize the Perl script handler (%s).", acRoutine, SvPV_nolen(ERRSV));
    return ER;
  }
}
#endif

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
    SslFreeProperties(psProperties->psSslProperties);
#endif
    if (psProperties->pcNonce)
    {
      free(psProperties->pcNonce);
    }
    OptionsFreeOptionsContext(psProperties->psOptionsContext);
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
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcValue = NULL;
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
  TimeGetTimeValue(&psProperties->tvJobEpoch);
  psProperties->dStartTime = (double) psProperties->tvJobEpoch.tv_sec + (double) psProperties->tvJobEpoch.tv_usec * 0.000001;
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
  psProperties->psSslProperties = SslNewProperties(acLocalError);
  if (psProperties->psSslProperties == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    FTimesFreeProperties(psProperties);
    return NULL;
  }
#endif

  /*-
   *********************************************************************
   *
   * Initialize TempDirectory variable.
   *
   *********************************************************************
   */
#ifdef WIN32
  pcValue = FTimesGetEnvValue("TEMP");
#else
  pcValue = FTimesGetEnvValue("TMPDIR");
#endif
  if (pcValue == NULL)
  {
    pcValue = FTimesGetEnvValue("TMP");
  }
  if (pcValue == NULL || strlen(pcValue) > sizeof(psProperties->acTempDirectory) - 1)
  {
    pcValue = FTIMES_TEMP_DIRECTORY;
  }
  strncpy(psProperties->acTempDirectory, pcValue, sizeof(psProperties->acTempDirectory));

  /*-
   *********************************************************************
   *
   * Initialize MemoryMapEnable variable -- it should be on by default.
   * Next, check the environment to see if it should be disabled.
   *
   *********************************************************************
   */
  psProperties->iMemoryMapEnable = 1;
  pcValue = FTimesGetEnvValue("FTIMES_MMAP_ENABLE");
  if (pcValue != NULL && strlen(pcValue) == 1 && (pcValue[0] == '0' || pcValue[0] == 'N' || pcValue[0] == 'n'))
  {
    psProperties->iMemoryMapEnable = 0;
  }

  /*-
   *********************************************************************
   *
   * Initialize the output's MD5 hash.
   *
   *********************************************************************
   */
  MD5Alpha(&psProperties->sOutFileHashContext);

  return psProperties;
}


/*-
 ***********************************************************************
 *
 * FTimesOptionHandler
 *
 ***********************************************************************
 */
int
FTimesOptionHandler(OPTIONS_TABLE *psOption, char *pcValue, FTIMES_PROPERTIES *psProperties, char *pcError)
{
  const char          acRoutine[] = "FTimesOptionHandler()";
  char                acLocalError[MESSAGE_SIZE] = "";
  int                 iError = 0;
  int                 iLength = 0;

  iLength = (pcValue == NULL) ? 0 : strlen(pcValue);

  switch (psOption->iId)
  {
  case OPT_LogLevel:
    iError = SupportSetLogLevel(pcValue, &psProperties->iLogLevel, acLocalError);
    if (iError != ER_OK)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s]: Argument (%s) does not pass muster (%s).", acRoutine, psOption->atcFullName, pcValue, acLocalError);
      return ER;
    }
    break;
  case OPT_MagicFile:
    if (iLength < 1 || iLength > FTIMES_MAX_PATH - 1)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s]: Argument length must be in the range [1-%d].", acRoutine, psOption->atcFullName, FTIMES_MAX_PATH - 1);
      return ER;
    }
    strncpy(psProperties->acMagicFileName, pcValue, FTIMES_MAX_PATH);
    break;
  case OPT_MemoryMapEnable:
    if (strcasecmp(pcValue, "1") == 0 || strcasecmp(pcValue, "Y") == 0)
    {
      psProperties->iMemoryMapEnable = 1;
    }
    else if (strcasecmp(pcValue, "0") == 0 || strcasecmp(pcValue, "N") == 0)
    {
      psProperties->iMemoryMapEnable = 0;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: option=[%s]: Argument (%s) does not pass muster. Use one of \"Y\" or \"N\".", acRoutine, psOption->atcFullName, pcValue);
      return ER;
    }
    break;
  case OPT_NamesAreCaseInsensitive:
    psProperties->psSnapshotContext->iNamesAreCaseInsensitive = 1;
    psProperties->psBaselineContext->iNamesAreCaseInsensitive = 1;
    break;
  case OPT_StrictTesting:
    psProperties->iTestLevel = FTIMES_TEST_STRICT;
    break;
  default:
    snprintf(pcError, MESSAGE_SIZE, "%s: Invalid option ID (%d). This should not happen.", acRoutine, psOption->iId);
    return ER;
    break;
  }

  return ER_OK;
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
  char                acLocalError[MESSAGE_SIZE] = "";
  char               *pcBaseline = NULL;
  char               *pcConfigFile = NULL;
  char               *pcMask = NULL;
  char               *pcMode = NULL;
  char               *pcSnapshot = NULL;
  int                 iError = 0;
  int                 iOperandCount = 0;
  OPTIONS_CONTEXT    *psOptionsContext = NULL;
  static OPTIONS_TABLE asCfgtestOptions[] =
  {
    { OPT_StrictTesting, "-s", "--StrictTesting", 0, 0, 0, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asCompareOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
    { OPT_MemoryMapEnable, "", "--MemoryMapEnable", 0, 0, 1, 0, FTimesOptionHandler },
    { OPT_NamesAreCaseInsensitive, "", "--NamesAreCaseInsensitive", 0, 0, 0, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asDecodeOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asDigOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asGetOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asMadOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
    { OPT_MagicFile, "", "--MagicFile", 0, 0, 1, 0, FTimesOptionHandler },
  };
  static OPTIONS_TABLE asMapOptions[] =
  {
    { OPT_LogLevel, "-l", "--LogLevel", 0, 0, 1, 0, FTimesOptionHandler },
    { OPT_MagicFile, "", "--MagicFile", 0, 0, 1, 0, FTimesOptionHandler },
  };

  psProperties->pcProgram = ppcArgumentVector[0];

  if (iArgumentCount < 2)
  {
    FTimesUsage();
  }

  /*-
   *********************************************************************
   *
   * Initialize the options context.
   *
   *********************************************************************
   */
  psOptionsContext = OptionsNewOptionsContext(iArgumentCount, ppcArgumentVector, acLocalError);
  if (psOptionsContext == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
  }
  psProperties->psOptionsContext = psOptionsContext;

  /*-
   *********************************************************************
   *
   * Determine the run mode.
   *
   *********************************************************************
   */
  psProperties->pcRunModeArgument = OptionsGetFirstArgument(psOptionsContext);
  if (psProperties->pcRunModeArgument == NULL)
  {
    FTimesUsage();
  }
  else
  {
    if (strcmp(psProperties->pcRunModeArgument, "--cfgtest") == 0)
    {
      psProperties->iRunMode = FTIMES_CFGTEST;
      //
      OptionsSetOptions(psOptionsContext, asCfgtestOptions, (sizeof(asCfgtestOptions) / sizeof(asCfgtestOptions[0])));
      //
      psProperties->piRunModeFinalStage = PropertiesTestFile;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--compare") == 0)
    {
      psProperties->iRunMode = FTIMES_CMPMODE;
      //
      OptionsSetOptions(psOptionsContext, asCompareOptions, (sizeof(asCompareOptions) / sizeof(asCompareOptions[0])));
      //
      strcpy(psProperties->acDataType, FTIMES_CMPDATA);
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "CmpModeFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = CmpModeFinishUp;
      //
      psProperties->piRunModeFinalStage = CmpModeFinalStage;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--decode") == 0 || strcmp(psProperties->pcRunModeArgument, "--decoder") == 0)
    {
      psProperties->iRunMode = FTIMES_DECODER;
      //
      OptionsSetOptions(psOptionsContext, asDecodeOptions, (sizeof(asDecodeOptions) / sizeof(asDecodeOptions[0])));
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DecoderFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DecoderFinishUp;
      //
      psProperties->piRunModeFinalStage = DecoderFinalStage;
    }
    else if (strncmp(psProperties->pcRunModeArgument, "--dig", 5) == 0)
    {
      if (strcmp(psProperties->pcRunModeArgument, "--digauto") == 0)
      {
        psProperties->iRunMode = FTIMES_DIGAUTO;
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--dig") == 0)
      {
        psProperties->iRunMode = FTIMES_DIGMODE;
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--digfull") == 0)
      {
        psProperties->iRunMode = FTIMES_DIGMODE; /* Note that --digfull is maintained for backwards compatibility. */
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--diglean") == 0)
      {
        psProperties->iRunMode = FTIMES_DIGMODE; /* Note that --diglean is maintained for backwards compatibility. */
      }
      else
      {
        FTimesUsage();
      }
      //
      OptionsSetOptions(psOptionsContext, asDigOptions, (sizeof(asDigOptions) / sizeof(asDigOptions[0])));
      //
      strcpy(psProperties->acDataType, FTIMES_DIGDATA);
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "DigModeFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = DigModeFinishUp;
      //
      psProperties->piRunModeFinalStage = DigModeFinalStage;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--get") == 0 || strcmp(psProperties->pcRunModeArgument, "--getmode") == 0)
    {
      psProperties->iRunMode = FTIMES_GETMODE;
      //
      OptionsSetOptions(psOptionsContext, asGetOptions, (sizeof(asGetOptions) / sizeof(asGetOptions[0])));
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "GetModeFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = GetModeFinishUp;
      //
      psProperties->piRunModeFinalStage = GetModeFinalStage;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "--mad") == 0)
    {
      psProperties->iRunMode = FTIMES_MADMODE;
      //
      OptionsSetOptions(psOptionsContext, asMadOptions, (sizeof(asMadOptions) / sizeof(asMadOptions[0])));
      //
      strcpy(psProperties->acDataType, FTIMES_MADDATA);
      //
      strcpy(psProperties->acDigRecordPrefix, "dig|");
      strcpy(psProperties->acMapRecordPrefix, "map|");
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MadModeInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MadModeInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MadModeCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MadModeCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MadModeFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MadModeFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MadModeWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MadModeWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MadModeFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MadModeFinishUp;
      //
      psProperties->piRunModeFinalStage = MadModeFinalStage;
    }
    else if (strncmp(psProperties->pcRunModeArgument, "--map", 5) == 0)
    {
      if (strcmp(psProperties->pcRunModeArgument, "--mapauto") == 0)
      {
        psProperties->iRunMode = FTIMES_MAPAUTO;
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--map") == 0)
      {
        psProperties->iRunMode = FTIMES_MAPMODE;
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--mapfull") == 0)
      {
        psProperties->iRunMode = FTIMES_MAPMODE; /* Note that --mapfull is maintained for backwards compatibility. */
      }
      else if (strcmp(psProperties->pcRunModeArgument, "--maplean") == 0)
      {
        psProperties->iRunMode = FTIMES_MAPMODE; /* Note that --maplean is maintained for backwards compatibility. */
      }
      else
      {
        FTimesUsage();
      }
      //
      OptionsSetOptions(psOptionsContext, asMapOptions, (sizeof(asMapOptions) / sizeof(asMapOptions[0])));
      //
      strcpy(psProperties->acDataType, FTIMES_MAPDATA);
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeInitialize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Initialize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeInitialize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeCheckDependencies");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_CheckDependencies;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeCheckDependencies;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeFinalize");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_Finalize;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinalize;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeWorkHorse");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_WorkHorse;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeWorkHorse;
      //
      strcpy(psProperties->sRunModeStages[psProperties->iLastRunModeStage].acDescription, "MapModeFinishUp");
      psProperties->sRunModeStages[psProperties->iLastRunModeStage].iError = XER_FinishUp;
      psProperties->sRunModeStages[psProperties->iLastRunModeStage++].piRoutine = MapModeFinishUp;
      //
      psProperties->piRunModeFinalStage = MapModeFinalStage;
    }
    else if (strcmp(psProperties->pcRunModeArgument, "-v") == 0 || strcmp(psProperties->pcRunModeArgument, "--version") == 0)
    {
      FTimesVersion();
    }
    else
    {
      FTimesUsage();
    }
  }
  OptionsSetArgumentType(psOptionsContext, OPTIONS_ARGUMENT_TYPE_MODE);

  /*-
   *********************************************************************
   *
   * Process options.
   *
   *********************************************************************
   */
  iError = OptionsProcessOptions(psOptionsContext, (void *) psProperties, acLocalError);
  switch (iError)
  {
  case OPTIONS_ER:
    snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
    return ER;
    break;
  case OPTIONS_OK:
    break;
  case OPTIONS_USAGE:
  default:
    FTimesUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * Handle any special cases and/or remaining arguments.
   *
   *********************************************************************
   */
  iOperandCount = OptionsGetOperandCount(psOptionsContext);
  switch (psProperties->iRunMode)
  {
  case FTIMES_CFGTEST:
    if (iOperandCount != 2)
    {
      FTimesUsage();
    }
    pcConfigFile = OptionsGetFirstOperand(psOptionsContext);
    if (pcConfigFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get config file argument.", acRoutine);
      return ER;
    }
    strncpy(psProperties->acConfigFile, pcConfigFile, FTIMES_MAX_PATH);
    pcMode = OptionsGetNextOperand(psOptionsContext);
    if (pcMode == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mode argument.", acRoutine);
      return ER;
    }
    if (strcasecmp(pcMode, "digauto") == 0)
    {
      psProperties->iTestRunMode = FTIMES_DIGAUTO;
    }
    else if
    (
      strcasecmp(pcMode, "dig") == 0 ||
      strcasecmp(pcMode, "digfull") == 0 ||
      strcasecmp(pcMode, "diglean") == 0
    )
    {
      psProperties->iTestRunMode = FTIMES_DIGMODE;
    }
    else if
    (
      strcasecmp(pcMode, "get") == 0 ||
      strcasecmp(pcMode, "getmode") == 0
    )
    {
      psProperties->iTestRunMode = FTIMES_GETMODE;
    }
    else if (strcasecmp(pcMode, "mad") == 0)
    {
      psProperties->iTestRunMode = FTIMES_MADMODE;
    }
    else if
    (
      strcasecmp(pcMode, "map") == 0 ||
      strcasecmp(pcMode, "mapfull") == 0 ||
      strcasecmp(pcMode, "maplean") == 0
    )
    {
      psProperties->iTestRunMode = FTIMES_MAPMODE;
    }
    else
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Mode = [%s]: Mode must be one of {dig|digauto|get|mad|map}.", acRoutine, pcMode);
      return ER;
    }
    break;
  case FTIMES_CMPMODE:
    if (iOperandCount != 3)
    {
      FTimesUsage();
    }
    pcMask = OptionsGetFirstOperand(psOptionsContext);
    if (pcMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mask argument.", acRoutine);
      return ER;
    }
    psProperties->psFieldMask = MaskParseMask(pcMask, MASK_RUNMODE_TYPE_CMP, acLocalError);
    if (psProperties->psFieldMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    pcBaseline = OptionsGetNextOperand(psOptionsContext);
    if (pcBaseline == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mask argument.", acRoutine);
      return ER;
    }
    psProperties->psBaselineContext->pcFile = pcBaseline;
    pcSnapshot = OptionsGetNextOperand(psOptionsContext);
    if (pcSnapshot == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mask argument.", acRoutine);
      return ER;
    }
    psProperties->psSnapshotContext->pcFile = pcSnapshot;
    break;
  case FTIMES_DECODER:
    if (iOperandCount != 1)
    {
      FTimesUsage();
    }
    pcSnapshot = OptionsGetNextOperand(psOptionsContext);
    if (pcSnapshot == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mask argument.", acRoutine);
      return ER;
    }
    psProperties->psSnapshotContext->pcFile = pcSnapshot;
    break;
  case FTIMES_DIGAUTO:
  case FTIMES_DIGMODE:
  case FTIMES_MADMODE:
  case FTIMES_MAPMODE:
    if (iOperandCount < 1)
    {
      FTimesUsage();
    }
    pcConfigFile = OptionsGetFirstOperand(psOptionsContext);
    if (pcConfigFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get config file argument.", acRoutine);
      return ER;
    }
    if (strcmp(pcConfigFile, "-") == 0)
    {
      strcpy(psProperties->acConfigFile, "-");
    }
    else
    {
      iError = SupportExpandPath(pcConfigFile, psProperties->acConfigFile, FTIMES_MAX_PATH, 1, acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return ER;
      }
      iError = SupportAddToList(psProperties->acConfigFile, &psProperties->psExcludeList, "Exclude", acLocalError);
      if (iError != ER_OK)
      {
        snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
        return ER;
      }
    }
    /* Note: Any remaining operands will be treated as includes. */
    break;
  case FTIMES_GETMODE:
    if (iOperandCount != 1)
    {
      FTimesUsage();
    }
    pcConfigFile = OptionsGetFirstOperand(psOptionsContext);
    if (pcConfigFile == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get config file argument.", acRoutine);
      return ER;
    }
    strncpy(psProperties->acConfigFile, pcConfigFile, FTIMES_MAX_PATH);
    break;
  case FTIMES_MAPAUTO:
    if (iOperandCount < 1)
    {
      FTimesUsage();
    }
    pcMask = OptionsGetFirstOperand(psOptionsContext);
    if (pcMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: Unable to get mask argument.", acRoutine);
      return ER;
    }
    psProperties->psFieldMask = MaskParseMask(pcMask, MASK_RUNMODE_TYPE_MAP, acLocalError);
    if (psProperties->psFieldMask == NULL)
    {
      snprintf(pcError, MESSAGE_SIZE, "%s: %s", acRoutine, acLocalError);
      return ER;
    }
    /* Note: Any remaining operands will be treated as includes. */
    break;
  default:
    FTimesUsage();
    break;
  }

  /*-
   *********************************************************************
   *
   * If any required arguments are missing, it's an error.
   *
   *********************************************************************
   */
  if (OptionsHaveRequiredOptions(psOptionsContext) == 0)
  {
    FTimesUsage();
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
  char                acLocalError[MESSAGE_SIZE] = "";
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
  snprintf(acMessage, MESSAGE_SIZE, "Program=%s %s", VersionGetVersion(), psProperties->pcRunModeArgument);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "SystemOS=%s", SupportGetSystemOS());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "Hostname=%s", SupportGetHostname());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  if (psProperties->tStartTime == ER)
  {
    snprintf(acMessage, MESSAGE_SIZE, "JobEpoch=NA");
  }
  else
  {
    snprintf(acMessage, MESSAGE_SIZE, "JobEpoch=%s %s %s", psProperties->acStartDate, psProperties->acStartTime, psProperties->acStartZone);
  }
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

  snprintf(acMessage, MESSAGE_SIZE, "JobPid=%s", psProperties->acPid);
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);

#ifdef UNIX
  snprintf(acMessage, MESSAGE_SIZE, "JobUid=%d", (int) getuid());
  MessageHandler(MESSAGE_QUEUE_IT, MESSAGE_INFORMATION, MESSAGE_PROPERTY_STRING, acMessage);
#endif

  snprintf(acMessage, MESSAGE_SIZE, "LogLevel=%d", psProperties->iLogLevel);
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
  char                acLocalError[MESSAGE_SIZE] = "";
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

#ifdef USE_EMBEDDED_PERL
  /*-
   *********************************************************************
   *
   * Release the Perl interpreter.
   *
   *********************************************************************
   */
  if (psProperties->psMyPerl != NULL)
  {
    PL_perl_destruct_level = 1; /* This must be set to 1 since perl_construct() reset it to 0 according to perlembed. */
    perl_destruct(psProperties->psMyPerl);
    perl_free(psProperties->psMyPerl);
    PERL_SYS_TERM();
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
  fprintf(stderr, "       ftimes --compare mask baseline snapshot [-l {0-6}]\n");
  fprintf(stderr, "       ftimes --decode snapshot [-l {0-6}]\n");
  fprintf(stderr, "       ftimes --dig file [-l {0-6}] [target [...]]\n");
  fprintf(stderr, "       ftimes --digauto file [-l {0-6}] [target [...]]\n");
  fprintf(stderr, "       ftimes --get file [-l {0-6}]\n");
  fprintf(stderr, "       ftimes --mad file [-l {0-6}] [target [...]]\n");
  fprintf(stderr, "       ftimes --map file [-l {0-6}] [target [...]]\n");
  fprintf(stderr, "       ftimes --mapauto mask [-l {0-6}] [target [...]]\n");
  fprintf(stderr, "       ftimes {-v|--version}\n");
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
void
FTimesVersion(void)
{
  fprintf(stdout, "%s\n", VersionGetVersion());
  exit(XER_OK);
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
  DigSetPropertiesReference(psProperties);
}


/*-
 ***********************************************************************
 *
 * FTimesGetEnvValue
 *
 ***********************************************************************
 */
char *
FTimesGetEnvValue(char *pcName)
{
#ifdef WIN32
  char               *pcValue = NULL;
  DWORD               dwCount = 0;

#define FTIMES_MAX_ENV_SIZE 32767 /* Source = MSDN documentation for GetEnvironmentVariable() */
  pcValue = calloc(FTIMES_MAX_ENV_SIZE, 1);
  if (pcValue == NULL)
  {
    return NULL;
  }
  dwCount = GetEnvironmentVariable(TEXT(pcName), pcValue, FTIMES_MAX_ENV_SIZE);
  return (dwCount == 0 || dwCount > FTIMES_MAX_ENV_SIZE) ? NULL : pcValue;
#else
  return getenv(pcName);
#endif
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
  char                acLocalError[MESSAGE_SIZE] = "";
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


/*-
 ***********************************************************************
 *
 * MakeNonce
 *
 ***********************************************************************
 */
char *
MakeNonce(char *pcError)
{
  const char          acRoutine[] = "MakeNonce()";
  char               *pcNonce = NULL;
  unsigned char       a = 0;
  unsigned char       b = 0;
  unsigned char       c = 0;
  unsigned char       d = 0;
  long                lValue = 0;

#ifdef WIN32
  lValue = (GetTickCount() << 16) | rand();
#else
  lValue = random();
#endif
  a = (unsigned char) ((lValue >> 24) & 0xff);
  b = (unsigned char) ((lValue >> 16) & 0xff);
  c = (unsigned char) ((lValue >>  8) & 0xff);
  d = (unsigned char) ((lValue >>  0) & 0xff);
  pcNonce = calloc(FTIMES_NONCE_SIZE, 1);
  if (pcNonce == NULL)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: calloc(): %s", acRoutine, strerror(errno));
    return NULL;
  }
  snprintf(pcNonce, FTIMES_NONCE_SIZE, "%02x%02x%02x%02x", a, b, c, d);
  return pcNonce;
}


/*-
 ***********************************************************************
 *
 * SeedRandom
 *
 ***********************************************************************
 */
int
SeedRandom(unsigned long ulTime1, unsigned long ulTime2, char *pcError)
{
  const char          acRoutine[] = "SeedRandom()";

  if (ulTime1 == (unsigned long) ~0 || ulTime2 == (unsigned long) ~0)
  {
    snprintf(pcError, MESSAGE_SIZE, "%s: Seed values are blatantly abnormal!: Time1 = [%08lx], Time2 = [%08lx]", acRoutine, ulTime1, ulTime2);
    return ER;
  }

#ifdef WIN32
    srand((unsigned long) (0xE97482AD ^ ((ulTime1 << 16) | (ulTime1 >> 16)) ^ ulTime2 ^ getpid() ^ GetTickCount()));
#else
  srandom((unsigned long) (0xE97482AD ^ ((ulTime1 << 16) | (ulTime1 >> 16)) ^ ulTime2 ^ getpid()));
#endif
  return ER_OK;
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
