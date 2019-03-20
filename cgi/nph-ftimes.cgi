#!/usr/bin/perl -w
######################################################################
#
# $Id: nph-ftimes.cgi,v 1.26 2007/02/23 00:22:35 mavrik Exp $
#
######################################################################
#
# Copyright 2000-2007 Klayton Monroe, All Rights Reserved.
#
######################################################################

use strict;
use Fcntl qw(:flock);

######################################################################
#
# Main Routine
#
######################################################################

  my (%hProperties, %hReturnCodes, $sLocalError);

  %hReturnCodes =
  (
    '200' => "OK",
    '250' => "Ping Test OK",
    '251' => "Link Test OK",
    '404' => "Not Found",
    '405' => "Method Not Allowed",
    '450' => "Invalid Query",
    '451' => "File Already Exists",
    '452' => "Username Undefined",
    '453' => "Username-ClientId Mismatch",
    '454' => "Content-Length Undefined",
    '455' => "Content-Length Exceeds Limit",
    '456' => "Content-Length Mismatch",
    '457' => "File Not Available",
    '460' => "RequiredMask-FieldMask Mismatch",
    '500' => "Internal Server Error",
    '550' => "Internal Server Initialization Error",
  );

  ####################################################################
  #
  # Punch in and go to work.
  #
  ####################################################################

  $hProperties{'StartTime'} = time();
  $hProperties{'Version'} = sprintf("%s %s", __FILE__, ('$Revision: 1.26 $' =~ /^.Revision: ([\d.]+)/));

  ####################################################################
  #
  # Create/Verify run time environment, and process GET/PUT requests.
  #
  ####################################################################

  if (!defined(CreateRunTimeEnvironment(\%hProperties, \$sLocalError)))
  {
    $hProperties{'ReturnStatus'} = 550;
    $hProperties{'ReturnReason'} = $hReturnCodes{$hProperties{'ReturnStatus'}};
    $hProperties{'ErrorMessage'} = $sLocalError;
  }
  else
  {
    if ($hProperties{'RequestMethod'} eq "GET")
    {
      $hProperties{'ReturnStatus'} = ProcessGetRequest(\%hProperties, \$sLocalError);
      $hProperties{'ReturnReason'} = $hReturnCodes{$hProperties{'ReturnStatus'}};
      $hProperties{'ErrorMessage'} = $sLocalError;
    }
    elsif ($hProperties{'RequestMethod'} eq "PING")
    {
      $hProperties{'ReturnStatus'} = ProcessPingRequest(\%hProperties, \$sLocalError);
      $hProperties{'ReturnReason'} = $hReturnCodes{$hProperties{'ReturnStatus'}};
      $hProperties{'ErrorMessage'} = $sLocalError;
    }
    elsif ($hProperties{'RequestMethod'} eq "PUT")
    {
      $hProperties{'ReturnStatus'} = ProcessPutRequest(\%hProperties, \$sLocalError);
      $hProperties{'ReturnReason'} = $hReturnCodes{$hProperties{'ReturnStatus'}};
      $hProperties{'ErrorMessage'} = $sLocalError;
    }
    else
    {
      $hProperties{'ReturnStatus'} = 405;
      $hProperties{'ReturnReason'} = $hReturnCodes{$hProperties{'ReturnStatus'}};
      $hProperties{'ErrorMessage'} = "Method ($hProperties{'RequestMethod'}) not allowed";
    }
  }
  $hProperties{'ServerContentLength'} = SendResponse(\%hProperties);

  ####################################################################
  #
  # Clean up and go home.
  #
  ####################################################################

  $hProperties{'StopTime'} = time();

  if ($hProperties{'EnableLogging'} =~ /^[Yy]$/)
  {
    LogMessage(\%hProperties);
  }

  1;


######################################################################
#
# CompareMasks
#
######################################################################

sub CompareMasks
{
  my ($sRequiredMask, $sSuppliedMask) = @_;

  ####################################################################
  #
  # Squash input.
  #
  ####################################################################

  $sRequiredMask =~ tr/A-Z/a-z/;
  $sSuppliedMask =~ tr/A-Z/a-z/;

  ####################################################################
  #
  # Check syntax.
  #
  ####################################################################

  if ($sRequiredMask !~ /^(all|none)([+-][\w]+)*$/)
  {
    return undef;
  }
  if ($sSuppliedMask !~ /^(all|none)([+-][\w]+)*$/)
  {
    return undef;
  }

  ####################################################################
  #
  # Split input on +/- boundaries.
  #
  ####################################################################

  my @aRequiredFields = split(/[+-]/, $sRequiredMask);
  my @aSuppliedFields = split(/[+-]/, $sSuppliedMask);

  ####################################################################
  #
  # Split input non +/- boundaries.
  #
  ####################################################################

  my @aRequiredTokens = split(/[^+-]+/, $sRequiredMask);
  my @aSuppliedTokens = split(/[^+-]+/, $sSuppliedMask);

  ####################################################################
  #
  # Require base elements to be the same.
  #
  ####################################################################

  my $aRequiredBase = $aRequiredFields[0];
  my $aSuppliedBase = $aSuppliedFields[0];

  if ($aRequiredBase ne $aSuppliedBase)
  {
    return 0;
  }

  ####################################################################
  #
  # Remove base element from the fields array.
  #
  ####################################################################

  shift @aRequiredFields;
  shift @aSuppliedFields;

  ####################################################################
  #
  # Remove base element from the tokens array.
  #
  ####################################################################

  shift @aRequiredTokens;
  shift @aSuppliedTokens;

  ####################################################################
  #
  # Tally up the tokens.
  #
  ####################################################################

  my (%hRequiredHash, %hSuppliedHash);

  for (my $sIndex = 0; $sIndex < scalar(@aRequiredFields); $sIndex++)
  {
    $hRequiredHash{$aRequiredFields[$sIndex]} = ($aRequiredTokens[$sIndex] eq "+") ? 1 : 0;
  }
  for (my $sIndex = 0; $sIndex < scalar(@aSuppliedFields); $sIndex++)
  {
    $hSuppliedHash{$aSuppliedFields[$sIndex]} = ($aSuppliedTokens[$sIndex] eq "+") ? 1 : 0;
  }

  ####################################################################
  #
  # Delete +/- fields. This depends on the value of the base field.
  #
  ####################################################################

  if ($aRequiredBase eq "all")
  {
    foreach my $sField (keys(%hRequiredHash))
    {
      if ($hRequiredHash{$sField} == 1)
      {
        delete($hRequiredHash{$sField});
      }
    }
    foreach my $sField (keys(%hSuppliedHash))
    {
      if ($hSuppliedHash{$sField} == 1)
      {
        delete($hSuppliedHash{$sField});
      }
    }
  }
  else
  {
    foreach my $sField (keys(%hRequiredHash))
    {
      if ($hRequiredHash{$sField} == 0)
      {
        delete($hRequiredHash{$sField});
      }
    }
    foreach my $sField (keys(%hSuppliedHash))
    {
      if ($hSuppliedHash{$sField} == 0)
      {
        delete($hSuppliedHash{$sField});
      }
    }
  }

  ####################################################################
  #
  # Compare the normalized masks, and return true/false.
  #
  ####################################################################

  return ((join("|", sort(keys(%hRequiredHash)))) eq (join("|", sort(keys(%hSuppliedHash)))));
}


######################################################################
#
# CreateRunTimeEnvironment
#
######################################################################

sub CreateRunTimeEnvironment
{
  my ($phProperties, $psError) = @_;

  ####################################################################
  #
  # Put input/output streams in binary mode.
  #
  ####################################################################

  foreach my $sHandle (\*STDIN, \*STDOUT, \*STDERR)
  {
    binmode($sHandle);
  }

  ####################################################################
  #
  # Initialize regex variables.
  #
  ####################################################################

  my %hRegexes =
  (
    'ClientId'  => qq(&CLIENTID=([\\w-]{1,64})),
    'DataType'  => qq(&DATATYPE=(dig|map)),
    'DateTime'  => qq(&DATETIME=(\\d{14})),
    'EOL'       => qq(\$),
    'FieldMask' => qq(&FIELDMASK=([\\w+-]{1,512})),
    'LogLength' => qq(&LOGLENGTH=(\\d{1,20})), # 18446744073709551615
    'MD5'       => qq(&MD5=([0-9a-fA-F]{32})),
    'OutLength' => qq(&OUTLENGTH=(\\d{1,20})), # 18446744073709551615
    'Request'   => qq(&REQUEST=(Map(?:Full|Lean)Config|Dig(?:Full|Lean)Config)),
    'RunType'   => qq(&RUNTYPE=(baseline|linktest|snapshot)),
    'SOL'       => qq(^),
    'Version'   => qq(VERSION=(ftimes[\\w ().-]{1,64})),
  );

  $$phProperties{'GETRegex'} =
    $hRegexes{'SOL'} .
    $hRegexes{'Version'} .
    $hRegexes{'ClientId'} .
    $hRegexes{'Request'} .
    $hRegexes{'EOL'}
    ;

  $$phProperties{'PINGRegex'} =
    $hRegexes{'SOL'} .
    $hRegexes{'Version'} .
    $hRegexes{'ClientId'} .
    $hRegexes{'DataType'} .
    $hRegexes{'FieldMask'} .
    $hRegexes{'EOL'}
    ;

  $$phProperties{'PUTRegex'} =
    $hRegexes{'SOL'} .
    $hRegexes{'Version'} .
    $hRegexes{'ClientId'} .
    $hRegexes{'DataType'} .
    $hRegexes{'FieldMask'} .
    $hRegexes{'RunType'} .
    $hRegexes{'DateTime'} .
    $hRegexes{'LogLength'} .
    $hRegexes{'OutLength'} .
    $hRegexes{'MD5'} .
    $hRegexes{'EOL'}
    ;

  ####################################################################
  #
  # Initialize environment-specific variables.
  #
  ####################################################################

  $$phProperties{'ContentLength'}  = $ENV{'CONTENT_LENGTH'};
  $$phProperties{'QueryString'}    = $ENV{'QUERY_STRING'};
  $$phProperties{'RemoteAddress'}  = $ENV{'REMOTE_ADDR'};
  $$phProperties{'RemoteUser'}     = $ENV{'REMOTE_USER'};
  $$phProperties{'RequestMethod'}  = $ENV{'REQUEST_METHOD'};
  $$phProperties{'ServerSoftware'} = $ENV{'SERVER_SOFTWARE'};
  $$phProperties{'PropertiesFile'} = $ENV{'FTIMES_PROPERTIES_FILE'};

  ####################################################################
  #
  # Initialize platform-specific variables.
  #
  ####################################################################

  if ($^O =~ /MSWin32/i)
  {
    $$phProperties{'OSClass'} = "WINDOWS";
    $$phProperties{'Newline'} = "\r\n";
  }
  else
  {
    $$phProperties{'OSClass'} = "UNIX";
    $$phProperties{'Newline'} = "\n";
    umask(022);
  }

  ####################################################################
  #
  # Initialize site-specific variables.
  #
  ####################################################################

  my (%hGlobalConfigTemplate, $sLocalError);

  %hGlobalConfigTemplate = # This is the set of site-wide properties.
  (
    'BaseDirectory'     => qq(^(?:[A-Za-z]:)?/[\\w./-]+\$),
    'CapContentLength'  => qq(^[YyNn]\$),
    'EnableLogging'     => qq(^[YyNn]\$),
    'MaxContentLength'  => qq(^\\d{1,20}\$), # 18446744073709551615
    'RequireMask'       => qq(^[YyNn]\$),
    'RequireMatch'      => qq(^[YyNn]\$),
    'RequireUser'       => qq(^[YyNn]\$),
    'RequiredDigMask'   => qq(^[\\w+-]{1,512}\$),
    'RequiredMapMask'   => qq(^[\\w+-]{1,512}\$),
    'UseGMT'            => qq(^[YyNn]\$),
  );
  $$phProperties{'GlobalConfigTemplate'} = { %hGlobalConfigTemplate };

  GetGlobalConfigProperties($phProperties, \%hGlobalConfigTemplate, \$sLocalError);

  ####################################################################
  #
  # Initialize derived variables.
  #
  ####################################################################

  $$phProperties{'IncomingDirectory'} = $$phProperties{'BaseDirectory'} . "/incoming";
  $$phProperties{'LogfilesDirectory'} = $$phProperties{'BaseDirectory'} . "/logfiles";
  $$phProperties{'ProfilesDirectory'} = $$phProperties{'BaseDirectory'} . "/profiles";
  $$phProperties{'LogFile'} = $$phProperties{'LogfilesDirectory'} . "/nph-ftimes.log";

  ####################################################################
  #
  # Verify run time environment.
  #
  ####################################################################

  if (!defined(VerifyRunTimeEnvironment($phProperties, \%hGlobalConfigTemplate, \$sLocalError)))
  {
    $$psError = $sLocalError;
    return undef;
  }

  1;
}


######################################################################
#
# GetKeysAndValues
#
######################################################################

sub GetKeysAndValues
{
  my ($sFile, $phValidKeys, $phKeyValuePairs, $psError) = @_;

  ####################################################################
  #
  # Make sure that required inputs are defined.
  #
  ####################################################################

  if (!defined($sFile) || !defined($phValidKeys) || !defined($phKeyValuePairs))
  {
    $$psError = "Unable to proceed due to missing or undefined inputs" if (defined($psError));
    return undef;
  }

  ####################################################################
  #
  # Open properties file.
  #
  ####################################################################

  if (!open(FH, "< $sFile"))
  {
    $$psError = "File ($sFile) could not be opened ($!)" if (defined($psError));
    return undef;
  }

  ####################################################################
  #
  # Read properties file. Ignore case (when evaluating keys), unknown
  # keys, comments, and blank lines. Note: If $phValidKeys is empty,
  # then nothing will be returned.
  #
  ####################################################################

  while (my $sLine = <FH>)
  {
    $sLine =~ s/[\r\n]+$//; # Remove CRs and LFs.
    $sLine =~ s/#.*$//; # Remove comments.
    if ($sLine !~ /^\s*$/)
    {
      my ($sKey, $sValue) = ($sLine =~ /^([^=]*)=(.*)$/);
      $sKey =~ s/^\s+//; # Remove leading whitespace.
      $sKey =~ s/\s+$//; # Remove trailing whitespace.
      $sValue =~ s/^\s+//; # Remove leading whitespace.
      $sValue =~ s/\s+$//; # Remove trailing whitespace.
      if (defined($sKey) && length($sKey))
      {
        foreach my $sKnownKey (keys(%$phValidKeys))
        {
          if ($sKey =~ /^$sKnownKey$/i)
          {
            $$phKeyValuePairs{$sKnownKey} = $sValue;
          }
        }
      }
    }
  }
  close(FH);

  1;
}


######################################################################
#
# GetGlobalConfigProperties
#
######################################################################

sub GetGlobalConfigProperties
{
  my ($phProperties, $phSiteProperties, $psError) = @_;

  ####################################################################
  #
  # BaseDirectory is the epicenter of activity.
  #
  ####################################################################

  $$phProperties{'BaseDirectory'} = "/var/ftimes";

  ####################################################################
  #
  # CapContentLength forces the script to abort when ContentLength
  # exceeds MaxContentLength.
  #
  ####################################################################

  $$phProperties{'CapContentLength'} = "N"; # [Y|N]

  ####################################################################
  #
  # When active, EnableLogging forces the script to generate a log
  # message for each request. If the designated LogFile can not be
  # opened, the log message will be written to STDERR.
  #
  ####################################################################

  $$phProperties{'EnableLogging'} = "Y"; # [Y|N]

  ####################################################################
  #
  # MaxContentLength specifies the largest upload in bytes the script
  # will accept. If CapContentLength is disabled, this control has no
  # effect.
  #
  ####################################################################

  $$phProperties{'MaxContentLength'} = 100000000; # 100 MB

  ####################################################################
  #
  # When active, RequireMask forces the script to abort if the client
  # does not supply the proper field mask.
  #
  ####################################################################

  $$phProperties{'RequireMask'} = "Y"; # [Y|N]

  ####################################################################
  #
  # When active, RequireMatch forces the script to abort if ClientId
  # does not match RemoteUser. When this value is disabled, any
  # authenticated user will be allowed to issue requests for a given
  # client. Disabling RequireUser implicitly disables RequireMatch.
  #
  ####################################################################

  $$phProperties{'RequireMatch'} = "Y"; # [Y|N]

  ####################################################################
  #
  # RequireUser forces the script to abort unless RemoteUser has been
  # set.
  #
  ####################################################################

  $$phProperties{'RequireUser'} = "Y"; # [Y|N]

  ####################################################################
  #
  # RequiredDigMask defines the required field mask for dig data.
  #
  ####################################################################

  $$phProperties{'RequiredDigMask'} = "all";

  ####################################################################
  #
  # RequiredMapMask defines the required field mask for map data.
  #
  ####################################################################

  $$phProperties{'RequiredMapMask'} = "all-magic";

  ####################################################################
  #
  # When active, UseGMT forces the script to convert all time values
  # to GMT. Otherwise, time values are converted to local time.
  #
  ####################################################################

  $$phProperties{'UseGMT'} = "N"; # [Y|N]

  ####################################################################
  #
  # Pull in any externally defined properties. These properties trump
  # internally defined properties.
  #
  ####################################################################

  GetKeysAndValues($$phProperties{'PropertiesFile'}, $phSiteProperties, $phProperties, undef);

  1;
}


######################################################################
#
# LogMessage
#
######################################################################

sub LogMessage
{
  my ($phProperties) = @_;

  ####################################################################
  #
  # Create date/time stamp and calculate duration.
  #
  ####################################################################

  my
  (
    $sSecond,
    $sMinute,
    $sHour,
    $sMonthDay,
    $sMonth,
    $sYear,
    $sWeekDay,
    $sYearDay,
    $sDaylightSavings
  ) = ($$phProperties{'UseGMT'} =~ /^[Yy]$/) ? gmtime($$phProperties{'StopTime'}) : localtime($$phProperties{'StopTime'});

  $$phProperties{'DateTime'} = sprintf("%04s-%02s-%02s %02s:%02s:%02s",
    $sYear + 1900,
    $sMonth + 1,
    $sMonthDay,
    $sHour,
    $sMinute,
    $sSecond
    );

  $$phProperties{'Duration'} = $$phProperties{'StopTime'} - $$phProperties{'StartTime'};

  ####################################################################
  #
  # Construct log message.
  #
  ####################################################################

  my (@aLogFields, @aOutputFields, $sLogMessage);

  @aLogFields =
  (
    'DateTime',
    'RemoteUser',
    'RemoteAddress',
    'RequestMethod',
    'ClientId',
    'ClientDataType',
    'ClientFilename',
    'ContentLength',
    'ServerContentLength',
    'Duration',
    'ReturnStatus',
    'ErrorMessage'
  );

  foreach my $sField (@aLogFields)
  {
    my $sValue = $$phProperties{$sField};
    if ($sField =~ /^ErrorMessage$/)
    {
      push(@aOutputFields, ((defined($sValue) && length($sValue)) ? "-- $sValue" : "--"));
    }
    else
    {
      push(@aOutputFields, ((defined($sValue) && length($sValue)) ? "$sValue" : "-"));
    }
  }
  $sLogMessage = join(" ", @aOutputFields);

  ####################################################################
  #
  # Deliver log message.
  #
  ####################################################################

  if (!open(LH, ">> " . $$phProperties{'LogFile'}))
  {
    print STDERR $sLogMessage, $$phProperties{'Newline'};
    return undef;
  }
  binmode(LH);
  flock(LH, LOCK_EX);
  print LH $sLogMessage, $$phProperties{'Newline'};
  flock(LH, LOCK_UN);
  close(LH);

  1;
}


######################################################################
#
# ProcessGetRequest
#
######################################################################

sub ProcessGetRequest
{
  my ($phProperties, $psError) = @_;

  ####################################################################
  #
  # Proceed only if QueryString matches GETRegex.
  #
  ####################################################################

  my $sQueryString = URLDecode($$phProperties{'QueryString'});

  if ($sQueryString =~ /$$phProperties{'GETRegex'}/)
  {
    $$phProperties{'ClientVersion'}  = $1;
    $$phProperties{'ClientId'}       = $2 || "nobody";
    $$phProperties{'ClientRequest'}  = $3;

    $$phProperties{'ClientRequest'} =~ /^(MapFull|MapLean|DigFull|DigLean)Config$/;
    $$phProperties{'ClientFilename'} = lc($1) . ".cfg";

    ##################################################################
    #
    # Do username and client ID checks.
    #
    ##################################################################

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && (!defined($$phProperties{'RemoteUser'}) || !length($$phProperties{'RemoteUser'})))
    {
      $$psError = "Remote user is undefined or null";
      return 452;
    }

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && $$phProperties{'RequireMatch'} =~ /^[Yy]$/ && $$phProperties{'RemoteUser'} ne $$phProperties{'ClientId'})
    {
      $$psError = "Remote user ($$phProperties{'RemoteUser'}) does not match client ID ($$phProperties{'ClientId'})";
      return 453;
    }

    ##################################################################
    #
    # Do content length checks.
    #
    ##################################################################

    if (!defined($$phProperties{'ContentLength'}) || !length($$phProperties{'ContentLength'}))
    {
      $$psError = "Content length is undefined or null";
      return 454;
    }

    if ($$phProperties{'CapContentLength'} =~ /^[Yy]$/ && $$phProperties{'ContentLength'} > $$phProperties{'MaxContentLength'})
    {
      $$psError = "Content length ($$phProperties{'ContentLength'}) exceeds maximum allowed length ($$phProperties{'MaxContentLength'})";
      return 455;
    }

    ##################################################################
    #
    # Locate the requested file and serve it up.
    #
    ##################################################################

    my ($sGetFile);

    $sGetFile = $$phProperties{'ProfilesDirectory'} . "/" . $$phProperties{'ClientId'} . "/" . "cfgfiles" . "/" . $$phProperties{'ClientFilename'};
    if (-e $sGetFile)
    {
      if (!open(FH, "< $sGetFile"))
      {
        $$psError = "Requested file ($sGetFile) could not be opened ($!)";
        return 457;
      }
      binmode(FH);
      $$phProperties{'ReturnHandle'} = \*FH;
      $$psError = "Success";
      return 200;
    }
    else
    {
      $$psError = "Requested file ($sGetFile) does not exist";
      return 404;
    }
  }
  else
  {
    $$psError = "Invalid query string ($$phProperties{'QueryString'})";
    return 450;
  }
}


######################################################################
#
# ProcessPingRequest
#
######################################################################

sub ProcessPingRequest
{
  my ($phProperties, $psError) = @_;

  ####################################################################
  #
  # Proceed only if QueryString matches PINGRegex.
  #
  ####################################################################

  my $sQueryString = URLDecode($$phProperties{'QueryString'});

  if ($sQueryString =~ /$$phProperties{'PINGRegex'}/)
  {
    $$phProperties{'ClientVersion'}   = $1;
    $$phProperties{'ClientId'}        = $2 || "nobody";
    $$phProperties{'ClientDataType'}  = $3;
    $$phProperties{'ClientFieldMask'} = $4;

    ##################################################################
    #
    # Do username and client ID checks.
    #
    ##################################################################

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && (!defined($$phProperties{'RemoteUser'}) || !length($$phProperties{'RemoteUser'})))
    {
      $$psError = "Remote user is undefined or null";
      return 452;
    }

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && $$phProperties{'RequireMatch'} =~ /^[Yy]$/ && $$phProperties{'RemoteUser'} ne $$phProperties{'ClientId'})
    {
      $$psError = "Remote user ($$phProperties{'RemoteUser'}) does not match client ID ($$phProperties{'ClientId'})";
      return 453;
    }

    ##################################################################
    #
    # Do content length checks.
    #
    ##################################################################

    if (!defined($$phProperties{'ContentLength'}) || !length($$phProperties{'ContentLength'}))
    {
      $$psError = "Content length is undefined or null";
      return 454;
    }

    if ($$phProperties{'CapContentLength'} =~ /^[Yy]$/ && $$phProperties{'ContentLength'} > $$phProperties{'MaxContentLength'})
    {
      $$psError = "Content length ($$phProperties{'ContentLength'}) exceeds maximum allowed length ($$phProperties{'MaxContentLength'})";
      return 455;
    }

    ##################################################################
    #
    # Do field mask check.
    #
    ##################################################################

    if ($$phProperties{'RequireMask'} =~ /^[Yy]$/)
    {
      my $sRequiredMask = ($$phProperties{'ClientDataType'} =~ /^map$/i) ? $$phProperties{'RequiredMapMask'} : $$phProperties{'RequiredDigMask'};
      if (!CompareMasks($sRequiredMask, $$phProperties{'ClientFieldMask'}))
      {
        $$psError = "Field mask ($$phProperties{'ClientFieldMask'}) does not match required field mask ($sRequiredMask)";
        return 460;
      }
    }

    $$psError = "Success";
    return 250;
  }
  else
  {
    $$psError = "Invalid query string ($$phProperties{'QueryString'})";
    return 450;
  }
}


######################################################################
#
# ProcessPutRequest
#
######################################################################

sub ProcessPutRequest
{
  my ($phProperties, $psError) = @_;

  ####################################################################
  #
  # Proceed only if QueryString matches PUTRegex.
  #
  ####################################################################

  my $sQueryString = URLDecode($$phProperties{'QueryString'});

  if ($sQueryString =~ /$$phProperties{'PUTRegex'}/)
  {
    my ($sLogLength, $sOutLength);

    $$phProperties{'ClientVersion'}   = $1;
    $$phProperties{'ClientId'}        = $2 || "nobody";
    $$phProperties{'ClientDataType'}  = $3;
    $$phProperties{'ClientFieldMask'} = $4;
    $$phProperties{'ClientRunType'}   = $5;
    $$phProperties{'ClientDateTime'}  = $6;
    $$phProperties{'ClientLogLength'} = $sLogLength = $7;
    $$phProperties{'ClientOutLength'} = $sOutLength = $8;
    $$phProperties{'ClientMD5'}       = $9;

    $$phProperties{'ClientFilename'} = $$phProperties{'ClientId'} . "_" . $$phProperties{'ClientDateTime'} . "_" . $$phProperties{'ClientRunType'};

    ##################################################################
    #
    # Do username and client ID checks.
    #
    ##################################################################

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && (!defined($$phProperties{'RemoteUser'}) || !length($$phProperties{'RemoteUser'})))
    {
      $$psError = "Remote user is undefined or null";
      return 452;
    }

    if ($$phProperties{'RequireUser'} =~ /^[Yy]$/ && $$phProperties{'RequireMatch'} =~ /^[Yy]$/ && $$phProperties{'RemoteUser'} ne $$phProperties{'ClientId'})
    {
      $$psError = "Remote user ($$phProperties{'RemoteUser'}) does not match client ID ($$phProperties{'ClientId'})";
      return 453;
    }

    ##################################################################
    #
    # Do content length checks.
    #
    ##################################################################

    if (!defined($$phProperties{'ContentLength'}) || !length($$phProperties{'ContentLength'}))
    {
      $$psError = "Content length is undefined or null";
      return 454;
    }

    if ($$phProperties{'CapContentLength'} =~ /^[Yy]$/ && $$phProperties{'ContentLength'} > $$phProperties{'MaxContentLength'})
    {
      $$psError = "Content length ($$phProperties{'ContentLength'}) exceeds maximum allowed length ($$phProperties{'MaxContentLength'})";
      return 455;
    }

    if ($$phProperties{'ContentLength'} != ($sOutLength + $sLogLength))
    {
      $$psError = "Content length ($$phProperties{'ContentLength'}) does not equal sum of individual stream lengths ($sOutLength + $sLogLength)";
      return 456;
    }

    ##################################################################
    #
    # Do field mask check.
    #
    ##################################################################

    if ($$phProperties{'RequireMask'} =~ /^[Yy]$/)
    {
      my $sRequiredMask = ($$phProperties{'ClientDataType'} =~ /^map$/i) ? $$phProperties{'RequiredMapMask'} : $$phProperties{'RequiredDigMask'};
      if (!CompareMasks($sRequiredMask, $$phProperties{'ClientFieldMask'}))
      {
        $$psError = "Field mask ($$phProperties{'ClientFieldMask'}) does not match required field mask ($sRequiredMask)";
        return 460;
      }
    }

    ##################################################################
    #
    # If this is a link test, dump the data and return success.
    #
    ##################################################################

    if ($$phProperties{'ClientRunType'} eq "linktest")
    {
      SysReadWrite(\*STDIN, undef, $$phProperties{'ContentLength'}, undef); # Slurp up data to prevent a broken pipe.
      $$psError = "Success";
      return 251;
    }

    ##################################################################
    #
    # Make output filenames.
    #
    ##################################################################

    my ($sLckFile, $sLogFile, $sOutFile, $sRdyFile);

    $sLckFile = $$phProperties{'IncomingDirectory'} . "/" . $$phProperties{'ClientFilename'} . ".lck";
    $sLogFile = $$phProperties{'IncomingDirectory'} . "/" . $$phProperties{'ClientFilename'} . ".log";
    $sOutFile = $$phProperties{'IncomingDirectory'} . "/" . $$phProperties{'ClientFilename'} . "." . $$phProperties{'ClientDataType'};
    $sRdyFile = $$phProperties{'IncomingDirectory'} . "/" . $$phProperties{'ClientFilename'} . ".rdy";

    ##################################################################
    #
    # Create a group lockfile and lock it. The purpose of the lock
    # is to prevent other instances of this script from writing to
    # any of the output files (.log, .{dig|map}, .rdy).
    #
    ##################################################################

    if (!open(LH, "> $sLckFile"))
    {
      $$psError = "File ($sLckFile) could not be opened ($!)";
      SysReadWrite(\*STDIN, undef, $$phProperties{'ContentLength'}, undef); # Slurp up data to prevent a broken pipe.
      return 500;
    }
    flock(LH, LOCK_EX);

    ##################################################################
    #
    # Make sure that none of the output files exist.
    #
    ##################################################################

    foreach my $sPutFile ($sOutFile, $sLogFile, $sRdyFile)
    {
      if (-e $sPutFile)
      {
        $$psError = "File ($sPutFile) already exists";
        SysReadWrite(\*STDIN, undef, $$phProperties{'ContentLength'}, undef); # Slurp up data to prevent a broken pipe.
        flock(LH, LOCK_UN); close(LH); unlink($sLckFile); # Unlock, close, and remove the group lockfile.
        return 451;
      }
    }

    ##################################################################
    #
    # Write the output files (.{dig|map}, .log, .rdy) to disk.
    #
    ##################################################################

    my (%hStreamLengths, $sLocalError);

    $hStreamLengths{$sOutFile} = $sOutLength;
    $hStreamLengths{$sLogFile} = $sLogLength;

    foreach my $sPutFile ($sLogFile, $sOutFile, $sRdyFile)
    {
      if (!open(FH, "> $sPutFile"))
      {
        $$psError = "File ($sPutFile) could not be opened ($!)";
        SysReadWrite(\*STDIN, undef, $$phProperties{'ContentLength'}, undef); # Slurp up data to prevent a broken pipe.
        flock(LH, LOCK_UN); close(LH); unlink($sLckFile); # Unlock, close, and remove the group lockfile.
        return 500;
      }
      binmode(FH);
      flock(FH, LOCK_EX);
      if ($sPutFile eq $sRdyFile)
      {
        print FH "Version=", $$phProperties{'Version'}, $$phProperties{'Newline'};
        foreach my $sKey (sort(keys(%{$$phProperties{'GlobalConfigTemplate'}})))
        {
          print FH $sKey, "=", $$phProperties{$sKey}, $$phProperties{'Newline'};
        }
      }
      else
      {
        my $sByteCount = SysReadWrite(\*STDIN, \*FH, $hStreamLengths{$sPutFile}, \$sLocalError);
        if (!defined($sByteCount))
        {
          $$psError = $sLocalError;
          flock(FH, LOCK_UN); close(FH);
          flock(LH, LOCK_UN); close(LH); unlink($sLckFile); # Unlock, close, and remove the group lockfile.
          return 500;
        }
        if ($sByteCount != $hStreamLengths{$sPutFile})
        {
          $$psError = "Stream length ($hStreamLengths{$sPutFile}) does not equal number of bytes processed ($sByteCount) for output file ($sPutFile)";
          flock(FH, LOCK_UN); close(FH);
          flock(LH, LOCK_UN); close(LH); unlink($sLckFile); # Unlock, close, and remove the group lockfile.
          return 456;
        }
      }
      flock(FH, LOCK_UN); close(FH);
    }
    flock(LH, LOCK_UN); close(LH); unlink($sLckFile); # Unlock, close, and remove the group lockfile.
    $$psError = "Success";
    return 200;
  }
  else
  {
    $$psError = "Invalid query string ($$phProperties{'QueryString'})";
    return 450;
  }
}


######################################################################
#
# SendResponse
#
######################################################################

sub SendResponse
{
  my ($phProperties) = @_;

  ####################################################################
  #
  # Send response header.
  #
  ####################################################################

  my ($sHandle, $sHeader, $sLength, $sReason, $sServer, $sStatus);

  $sHandle = $$phProperties{'ReturnHandle'};
  $sStatus = $$phProperties{'ReturnStatus'};
  $sReason = $$phProperties{'ReturnReason'};
  $sServer = $$phProperties{'ServerSoftware'};
  $sLength = (defined($sHandle)) ? -s $sHandle : 0;

  $sHeader  = "HTTP/1.1 $sStatus $sReason\r\n";
  $sHeader .= "Server: $sServer\r\n";
  $sHeader .= "Content-Type: application/octet-stream\r\n";
  $sHeader .= "Content-Length: $sLength\r\n";
  $sHeader .= "\r\n";

  syswrite(STDOUT, $sHeader, length($sHeader));

  ####################################################################
  #
  # Send content if any.
  #
  ####################################################################

  if (defined($sHandle))
  {
    SysReadWrite($sHandle, \*STDOUT, $sLength, undef);
    close($sHandle);
  }

  return $sLength;
}


######################################################################
#
# SysReadWrite
#
######################################################################

sub SysReadWrite
{
  my ($sReadHandle, $sWriteHandle, $sLength, $psError) = @_;

  ####################################################################
  #
  # Read/Write data, but discard data if write handle is undefined.
  #
  ####################################################################

  my ($sData, $sEOF, $sNRead, $sNProcessed, $sNWritten);

  for ($sEOF = $sNRead = $sNProcessed = 0; !$sEOF && $sLength > 0; $sLength -= $sNRead)
  {
    $sNRead = sysread($sReadHandle, $sData, ($sLength > 0x4000) ? 0x4000 : $sLength);
    if (!defined($sNRead))
    {
      $$psError = "Error reading from input stream ($!)" if (defined($psError));
      return undef;
    }
    elsif ($sNRead == 0)
    {
      $sEOF = 1;
    }
    else
    {
      if (defined($sWriteHandle))
      {
        $sNWritten = syswrite($sWriteHandle, $sData, $sNRead);
        if (!defined($sNWritten))
        {
          $$psError = "Error writing to output stream ($!)" if (defined($psError));
          return undef;
        }
      }
      else
      {
        $sNWritten = $sNRead;
      }
      $sNProcessed += $sNWritten;
    }
  }

  return $sNProcessed;
}


######################################################################
#
# URLDecode
#
######################################################################

sub URLDecode
{
  my ($sData) = @_;

  $sData =~ s/\+/ /sg;
  $sData =~ s/%([0-9a-fA-F]{2})/pack('C', hex($1))/seg;

  return $sData;
}


######################################################################
#
# VerifyRunTimeEnvironment
#
######################################################################

sub VerifyRunTimeEnvironment
{
  my ($phProperties, $phRequiredProperties, $psError) = @_;

  ####################################################################
  #
  # Make sure all required properties are defined and valid.
  #
  ####################################################################

  foreach my $sProperty (keys(%$phRequiredProperties))
  {
    my $sValue = $$phProperties{$sProperty};
    if (!defined($sValue) || $sValue !~ /$$phRequiredProperties{$sProperty}/)
    {
      $$psError = "$sProperty property ($sValue) is undefined or invalid";
      return undef;
    }
  }

  ####################################################################
  #
  # Make sure the logfiles directory is readable.
  #
  ####################################################################

  if (!-d $$phProperties{'LogfilesDirectory'} || !-R _)
  {
    $$psError = "Logfiles directory ($$phProperties{'LogfilesDirectory'}) does not exist or is not readable";
    return undef;
  }

  ####################################################################
  #
  # Make sure the profiles directory is readable.
  #
  ####################################################################

  if (!-d $$phProperties{'ProfilesDirectory'} || !-R _)
  {
    $$psError = "Profiles directory ($$phProperties{'ProfilesDirectory'}) does not exist or is not readable";
    return undef;
  }

  ####################################################################
  #
  # Make sure the incoming directory is writable.
  #
  ####################################################################

  if (!-d $$phProperties{'IncomingDirectory'} || !-W _)
  {
    $$psError = "Incoming directory ($$phProperties{'IncomingDirectory'}) does not exist or is not writeable";
    return undef;
  }

  ####################################################################
  #
  # Make sure that the required dig/map masks pass a syntax check.
  #
  ####################################################################

  if ($$phProperties{'RequireMask'} =~ /^[Yy]$/ && !defined(CompareMasks($$phProperties{'RequiredMapMask'}, $$phProperties{'RequiredMapMask'})))
  {
    $$psError = "Invalid mask ($$phProperties{'RequiredMapMask'})";
    return undef;
  }

  if ($$phProperties{'RequireMask'} =~ /^[Yy]$/ && !defined(CompareMasks($$phProperties{'RequiredDigMask'}, $$phProperties{'RequiredDigMask'})))
  {
    $$psError = "Invalid mask ($$phProperties{'RequiredDigMask'})";
    return undef;
  }

  1;
}
