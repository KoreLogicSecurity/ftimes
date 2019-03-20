#!/usr/bin/perl
######################################################################
#
# $Id: nph-ftimes.cgi,v 1.7 2003/02/23 17:40:07 mavrik Exp $
#
######################################################################
#
# Copyright 2000-2003 Klayton Monroe, Cable & Wireless
# All Rights Reserved.
#
######################################################################

use strict;

######################################################################
#
# NPH Specific Initialization
#
######################################################################

  my %returnCodes =
  (
    '200' => "OK",
    '250' => "Ping Received", # ftimes only
    '251' => "LinkTest Succeeded",
    '252' => "Upload Succeeded, Relay Failed", # ftimes only
    '405' => "Method Not Allowed",
    '450' => "Invalid Query",
    '451' => "File Already Exists",
    '452' => "Username Undefined",
    '453' => "Username-ClientId Mismatch",
    '454' => "Content-Length Undefined",
    '455' => "Content-Length Exceeds Limit",
    '456' => "Content-Length Mismatch",
    '457' => "File Not Available",
    '460' => "RequiredMask-FieldMask Mismatch", # ftimes only
    '500' => "Internal Server Error",
    '550' => "Insufficient Access"
  );

#######################################################################
#
# Platform Specific Initialization
#
#######################################################################

  #####################################################################
  #
  # If the platform is WIN32, put the input stream in binary mode
  # and close stderr. Without binmode, WIN32 Perl will internally
  # handle carriage returns and line feeds, and that could result
  # in a content length mismatch. If stderr is not closed, IIS has
  # been known to clobber the nph response header.
  #
  #####################################################################

  my ($baseDirectory);

  if ($^O =~ /MSWin32/i)
  {
    $baseDirectory    = "c:/inetpub/wwwroot/integrity";
    binmode(STDIN);
    close(STDERR);
  }
  else
  {
    $baseDirectory    = "/integrity";
    umask 022;
  }

#######################################################################
#
# Site Specific Initialization
#
#######################################################################

  #####################################################################
  #
  # The dig and map variables specify the name of config files that
  # will be served when clients issue GET requests. These files are
  # expected to exist in $baseDirectory/profiles/$clientId/cfgfiles.
  #
  #####################################################################

  my $digFile         = "dig.cfg";
  my $mapFile         = "map.cfg";

  #####################################################################
  #
  # The require user variable forces the program to abort unless the
  # REMOTE_USER environment variable has been set. Apache sets this
  # REMOTE_USER when authentication is enabled.
  #
  #####################################################################

  my $requireUser     = 1; # 0 = disabled, 1 = enabled

  #####################################################################
  #
  # The require match variable forces the program to abort unless
  # $username matches $clientId. When this value is disabled, other
  # users may issue requests on behalf of a given client.
  #
  #####################################################################

  my $requireMatch    = 1; # 0 = disabled, 1 = enabled

  #####################################################################
  #
  # The enforce limit variable forces the program to abort whenever
  # $contentLength is greater than $maxLength.
  #
  #####################################################################

  my $enforceLimit    = 0; # 0 = disabled, 1 = enabled
  my $maxLength       = 50000000; # 50 MB

  #####################################################################
  #
  # The require mask variable forces the program to abort unless the
  # FIELDMASK query string variable matches $requiredMask. The value
  # assigned to $requiredMask depends on the value for the DATATYPE
  # query string variable.
  #
  #####################################################################

  my $requireMask     = 1; # 0 = disabled, 1 = enabled
  my $requiredMask    = "";
  my $requiredMapMask = "all-magic";
  my $requiredDigMask = "all";

  #####################################################################
  #
  # The following regex variables are used to validate query strings.
  #
  #####################################################################

  my $reVersion       = "VERSION=(.+)";
  my $reClientId      = "&CLIENTID=([A-Z]\\d{3}_[A-Z]{4}_\\d{4}_\\d{1})";
  my $reRequest       = "&REQUEST=(MapConfig|DigConfig)";
  my $reDataType      = "&DATATYPE=(dig|map)";
  my $reFieldMask     = "&FIELDMASK=([a-zA-Z+-]+)";
  my $reRunType       = "&RUNTYPE=(baseline|linktest|snapshot)";
  my $reDateTime      = "&DATETIME=(\\d{14})";
  my $reLogLength     = "&LOGLENGTH=(\\d{1,10})"; #4294967295
  my $reOutLength     = "&OUTLENGTH=(\\d{1,10})";
  my $reMD5           = "&MD5=([0-9a-fA-F]{32})";

#######################################################################
#
# Main Routine
#
#######################################################################

  ####################################################################
  #
  # Attempt to fire up the log file. Keep going if it fails.
  #
  ####################################################################

  open(LH, ">>$baseDirectory/logfiles/nph-ftimes.log");

  ####################################################################
  #
  # These variables are included in every log entry.
  #
  ####################################################################

  my $logHandle       = \*LH;
  my $username        = $ENV{'REMOTE_USER'};
  my $cgiRemote       = $ENV{'REMOTE_ADDR'};
  my $cgiMethod       = $ENV{'REQUEST_METHOD'};
  my $cgiRequest      = undef;
  my $byteCount       = undef;
  my $startTime       = time;
  my $returnCode      = undef;
  my $context         = undef;
  my $explanation     = undef;

  ####################################################################
  #
  # Make sure that a username has been defined, if required.
  #
  ####################################################################

  if ($requireUser && (!defined $username || !length($username)))
  {
    $returnCode = '452';
    $context = undef;
    $explanation = $returnCodes{$returnCode};
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Make sure that a content length has been defined.
  #
  ####################################################################

  my $contentLength = $ENV{'CONTENT_LENGTH'};

  if (!defined $contentLength || !length($contentLength))
  {
    $returnCode = '454';
    $context = undef;
    $explanation = $returnCodes{$returnCode};
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Check that the content length is within the limit, if required.
  #
  ####################################################################

  if ($enforceLimit && $contentLength > $maxLength)
  {
    $returnCode = '455';
    $context = "Length=$contentLength Limit=$maxLength";
    $explanation = $returnCodes{$returnCode};
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Make sure that the required dig/map masks pass a syntax check.
  #
  ####################################################################

  if ($requireMask && !defined CompareMasks($requiredMapMask, $requiredMapMask))
  {
    $returnCode = '500';
    $context = "MapMask=$requiredMapMask";
    $explanation = "Invalid Mask";
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  if ($requireMask && !defined CompareMasks($requiredDigMask, $requiredDigMask))
  {
    $returnCode = '500';
    $context = "DigMask=$requiredDigMask";
    $explanation = "Invalid Mask";
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Make sure there is a writable drop zone.
  #
  ####################################################################

  my $dropZone = $baseDirectory . "/incoming";

  if (!-d $dropZone)
  {
    $returnCode = '500';
    $context = "DropZone=$dropZone";
    $explanation = $!;
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  if (!-W $dropZone)
  {
    $returnCode = '550';
    $context = "DropZone=$dropZone";
    $explanation = "Write access is required for this object";
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Make sure there is a readable profiles directory.
  #
  ####################################################################

  my $profiles = $baseDirectory . "/profiles";

  if (!-d $profiles)
  {
    $returnCode = '500';
    $context = "Profiles=$profiles";
    $explanation = $!;
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  if (!-R $profiles)
  {
    $returnCode = '550';
    $context = "Profiles=$profiles";
    $explanation = "Read access is required for this object";
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  ####################################################################
  #
  # Preprocess the query string (i.e. unencode it).
  #
  ####################################################################

  my $queryString = $ENV{'QUERY_STRING'};
  $queryString =~ s/\+/ /g;
  $queryString =~ s/%([0-9a-fA-F]{2})/pack("c", hex($1))/ge;

  ####################################################################
  #
  # GET Request -- Determine what was requested, and serve it up.
  #
  ####################################################################

  if ($cgiMethod eq "GET")
  {
    my $reGet  = "^" . $reVersion . $reClientId . $reRequest . "\$";

    if ($queryString =~ /$reGet/)
    {
      my $version    = $1;
      my $clientId   = $2;
      my $cgiRequest = $3;

      if ($requireUser && $requireMatch && $username ne $clientId)
      {
        $returnCode = '453';
        $context = "Username=$username ClientId=$clientId";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      my $getFile = $profiles . "/" . $clientId . "/" . "cfgfiles" . "/" . (($cgiRequest =~ /MapConfig/i) ? $mapFile : $digFile);

      if (-e $getFile)
      {
        if (open(FILE, "$getFile"))
        {
          $byteCount = -s $getFile;
          $returnCode = '200';
          ReturnContent(\*FILE, $byteCount, $returnCode, $returnCodes{$returnCode});
          $context = "File=$getFile";
          $explanation = $returnCodes{$returnCode};
          LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
          close(FILE);
          Finish($logHandle);
        }
        else
        {
          $byteCount = 0;
          $returnCode = '500';
          $context = "File=$getFile";
          $explanation = $!;
          LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
          ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
        }
      }
      else
      {
        $returnCode = '457';
        $context = "File=$getFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'}; # Extract actual value from ENV.
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # PING Request -- Check the client's mask, and respond to the ping.
  #
  ####################################################################

  elsif ($cgiMethod eq "PING")
  {
    my $rePing = "^" . $reVersion . $reClientId . $reDataType . $reFieldMask . "\$";

    if ($queryString =~ /$rePing/)
    {
      my $version   = $1;
      my $clientId  = $2;
      my $dataType  = $3;
      my $fieldMask = $4;

      if ($requireUser && $requireMatch && $username ne $clientId)
      {
        $returnCode = '453';
        $context = "Username=$username ClientId=$clientId";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      ################################################################
      #
      # This logic helps to ensure mask consistency.
      #
      ################################################################

      if ($requireMask)
      {
        $requiredMask = ($dataType =~ /^map$/i) ? $requiredMapMask : $requiredDigMask;
        if (!CompareMasks($requiredMask, $fieldMask))
        {
          $returnCode = '460';
          $context = "DataType=$dataType FieldMask=$fieldMask RequiredMask=$requiredMask";
          $explanation = $returnCodes{$returnCode};
          LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
          ReturnFailure($logHandle, $returnCode, ($returnCodes{$returnCode} . ", Use $requiredMask"));
        }
      }

      $returnCode = '250';
      $context = undef;
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnSuccess($logHandle, $returnCode, $returnCodes{$returnCode});
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'}; # Extract actual value from ENV.
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # PUT Request -- Write the uploaded data directly to disk.
  #
  ####################################################################

  elsif ($cgiMethod eq "PUT")
  {
    my $rePut  = "^" . $reVersion . $reClientId . $reDataType . $reFieldMask . $reRunType . $reDateTime . $reLogLength . $reOutLength . $reMD5 . "\$";

    if ($queryString =~ /$rePut/)
    {
      my $version   = $1;
      my $clientId  = $2;
      my $dataType  = $3;
      my $fieldMask = $4;
      my $runType   = $5;
      my $dateTime  = $6;
      my $logLength = $7;
      my $outLength = $8;
      my $md5       = $9;
      $cgiRequest   = $clientId . "_" . $dateTime . "_" . $runType;

      if ($requireUser && $requireMatch && $username ne $clientId)
      {
        $returnCode = '453';
        $context = "Username=$username ClientId=$clientId";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      ################################################################
      #
      # This logic helps to ensure mask consistency.
      #
      ################################################################

      if ($requireMask)
      {
        $requiredMask = ($dataType =~ /^map$/i) ? $requiredMapMask : $requiredDigMask;
        if (!CompareMasks($requiredMask, $fieldMask))
        {
          ReadAndChuckData($contentLength); # prevents a broken pipe
          $returnCode = '460';
          $context = "DataType=$dataType FieldMask=$fieldMask RequiredMask=$requiredMask";
          $explanation = $returnCodes{$returnCode};
          LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
          ReturnFailure($logHandle, $returnCode, ($returnCodes{$returnCode} . ", Use $requiredMask"));
        }
      }

      ##############################################################
      #
      # If this is a linktest, dump the data and return success.
      #
      ##############################################################

      if ($runType eq "linktest")
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '251';
        $context = undef;
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnSuccess($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      ##############################################################
      #
      # Otherwise, process the data.
      #
      ##############################################################

      my $logFile = $dropZone . "/" . $cgiRequest . ".log";
      my $outFile = $dropZone . "/" . $cgiRequest . "." . $dataType;
      my $cfgFile = $dropZone . "/" . $cgiRequest . ".cfg";
      my $rdyFile = $dropZone . "/" . $cgiRequest . ".rdy";

      ##############################################################
      #
      # Make sure that none of the output files exist.
      #
      ##############################################################

      if (-e $logFile)
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$logFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $outFile)
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$outFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $cfgFile)
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$cfgFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $rdyFile)
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$rdyFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      ##############################################################
      #
      # Initialize the read byte count.
      #
      ##############################################################

      $byteCount = 0;

      ##############################################################
      #
      # Suck in the log data and store it in a file. Abort, if the
      # file exists or can't be opened.
      #
      ##############################################################

      if (!open(FILE, ">$logFile"))
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '500';
        $context = "File=$logFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      binmode(FILE);
      my $nLog = ReadAndWriteData(\*FILE, $logLength);
      close(FILE);
      if ($nLog != $logLength)
      {
        $returnCode = '456';
        $context = "File=$logFile NRead=$nLog Length=$logLength";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
      $byteCount += $nLog;

      ##############################################################
      #
      # Suck in the out data and store it in a file. Abort, if the
      # file exists or can't be opened.
      #
      ##############################################################

      if (!open(FILE, ">$outFile"))
      {
        ReadAndChuckData($contentLength); # prevents a broken pipe
        $returnCode = '500';
        $context = "File=$outFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      binmode(FILE);
      my $nOut = ReadAndWriteData(\*FILE, $outLength);
      close(FILE);
      if ($nOut != $outLength)
      {
        $returnCode = '456';
        $context = "File=$outFile NRead=$nOut Length=$outLength";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
      $byteCount += $nOut;

      ##############################################################
      #
      # Create a put configuration file. This preserves important
      # elements from the query string and makes it easier to relay
      # the snapshot at some later time.
      #
      ##############################################################

      if (!open(FILE, ">$cfgFile"))
      {
        $returnCode = '500';
        $context = "File=$cfgFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      print FILE "#\n";
      print FILE "# This file contains most of the directives necessary to upload\n";
      print FILE "# the associated snapshot to a remote server. Before attempting an\n";
      print FILE "# upload, review the following information for completeness and\n";
      print FILE "# correctness. You are required to supply the server's URL and any\n";
      print FILE "# necessary credentials.\n";
      print FILE "#\n";
      print FILE "LogFileName=$logFile\n";
      print FILE "OutFileName=$outFile\n";
      print FILE "OutFileHash=$md5\n";
      print FILE "#\n";
      print FILE "BaseName=$clientId\n";
      print FILE "DataType=$dataType\n";
      print FILE "DateTime=$dateTime\n";
      print FILE "FieldMask=$fieldMask\n";
      print FILE "RunType=$runType\n";
      print FILE "#\n";
      close(FILE);

      ##############################################################
      #
      # Create the .rdy file. This file acts as a lock release. In
      # other words, its presence informs other applications that
      # it's safe to process this snapshot.
      #
      ##############################################################

      if (!open(FILE, ">$rdyFile"))
      {
        $returnCode = '500';
        $context = "File=$rdyFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
      print FILE "+";
      close(FILE);

      ####################################################################
      #
      # Return success.
      #
      ####################################################################

      $returnCode = '200';
      $context = undef;
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnSuccess($logHandle, $returnCode, $returnCodes{$returnCode});
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'}; # Extract actual value from ENV.
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # Unsupported Request -- Return an error.
  #
  ####################################################################

  else
  {
    $returnCode = '405';
    $context = undef;
    $explanation = $returnCodes{$returnCode};
    LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
    ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
  }

  1;


######################################################################
#
# Finish
#
######################################################################

sub Finish
{
  my ($logHandle) = @_;
  close($logHandle) if ($logHandle);
  exit;
}


######################################################################
#
# ReadAndWriteData
#
######################################################################

sub ReadAndWriteData
{
  my ($fileHandle, $toRead) = @_;
  my ($nRead, $nWritten);

  $nRead = $nWritten = 0;
  while($toRead)
  {
    my $data = "";
    $nRead = sysread(STDIN, $data, $toRead);
    $nWritten += syswrite($fileHandle, $data, $nRead);
    $toRead -= $nRead;
  }
  return $nWritten;
}


######################################################################
#
# ReadAndChuckData
#
######################################################################

sub ReadAndChuckData
{
  my ($toRead) = @_;

  while($toRead)
  {
    my $data = "";
    $toRead -= sysread(STDIN, $data, $toRead);
  }
  return $toRead;
}


######################################################################
#
# ReturnFailure
#
######################################################################

sub ReturnFailure
{
  my ($fileHandle, $statusCode, $reasonPhrase) = @_;
  print "HTTP/1.1 $statusCode $reasonPhrase\r\n";
  print "Server: $ENV{'SERVER_SOFTWARE'}\r\n";
  print "Content-Type: text/plain\r\n";
  print "Content-Length: 0\r\n";
  print "\r\n";
  Finish($fileHandle);
  1;
}


######################################################################
#
# ReturnSuccess
#
######################################################################

sub ReturnSuccess
{
  my ($fileHandle, $statusCode, $reasonPhrase) = @_;
  print "HTTP/1.1 $statusCode $reasonPhrase\r\n";
  print "Server: $ENV{'SERVER_SOFTWARE'}\r\n";
  print "Content-Type: text/plain\r\n";
  print "Content-Length: 0\r\n";
  print "\r\n";
  Finish($fileHandle);
  1;
}


######################################################################
#
# ReturnContent
#
######################################################################

sub ReturnContent
{
  my ($fileHandle, $contentLength, $statusCode, $reasonPhrase) = @_;
  print "HTTP/1.1 $statusCode $reasonPhrase\r\n";
  print "Server: $ENV{'SERVER_SOFTWARE'}\r\n";
  print "Content-Type: application/octet-stream\r\n";
  print "Content-Length: $contentLength\r\n";
  print "\r\n";
  while (<$fileHandle>)
  {
    print;
  }
  1;
}


######################################################################
#
# LogMessage
#
######################################################################

sub LogMessage
{
  my ($logHandle, $username, $address, $method, $request, $byteCount, $startTime, $status, $context, $explanation) = @_;
  my $time = (scalar localtime);
  my $runTime = time - $startTime;
  my $delimeter = " ";
  my @errorMessage;

  push(@errorMessage, ((defined $time)        ? $time               : "-"));
  push(@errorMessage, ((defined $username)    ? $username           : "-"));
  push(@errorMessage, ((defined $address)     ? $address            : "-"));
  push(@errorMessage, ((defined $method)      ? $method             : "-"));
  push(@errorMessage, ((defined $request)     ? $request            : "-"));
  push(@errorMessage, ((defined $byteCount)   ? $byteCount          : "-"));
  push(@errorMessage, ((defined $runTime)     ? $runTime            : "-"));
  push(@errorMessage, ((defined $status)      ? $status             : "-"));
  push(@errorMessage, ((defined $context)     ? "C{ $context }"     : "C{ - }"));
  push(@errorMessage, ((defined $explanation) ? "E{ $explanation }" : "E{ - }"));
  print $logHandle join($delimeter, @errorMessage) . "\n" if ($logHandle);
  1;
}


######################################################################
#
# CompareMasks
#
######################################################################

sub CompareMasks
{
  my ($requiredMask, $suppliedMask) = @_;

  ####################################################################
  #
  # Squash input.
  #
  ####################################################################

  $requiredMask =~ tr/A-Z/a-z/;
  $suppliedMask =~ tr/A-Z/a-z/;

  ####################################################################
  #
  # Check syntax.
  #
  ####################################################################

  if ($requiredMask !~ /^(all|none)([+-][a-z]+)*$/)
  {
    return undef;
  }
  if ($suppliedMask !~ /^(all|none)([+-][a-z]+)*$/)
  {
    return undef;
  }

  ####################################################################
  #
  # Split input on +/- boundaries.
  #
  ####################################################################

  my @requiredFields = split(/[+-]/, $requiredMask);
  my @suppliedFields = split(/[+-]/, $suppliedMask);

  ####################################################################
  #
  # Split input non +/- boundaries.
  #
  ####################################################################

  my @requiredTokens = split(/[^+-]+/, $requiredMask);
  my @suppliedTokens = split(/[^+-]+/, $suppliedMask);

  ####################################################################
  #
  # Require the base elements to be the same.
  #
  ####################################################################

  my $requiredBase = $requiredFields[0];
  my $suppliedBase = $suppliedFields[0];

  if ($requiredBase ne $suppliedBase)
  {
    return 0;
  }

  ####################################################################
  #
  # Remove the base element from the fields array.
  #
  ####################################################################

  shift @requiredFields;
  shift @suppliedFields;

  ####################################################################
  #
  # Remove the base element from the tokens array.
  #
  ####################################################################

  shift @requiredTokens;
  shift @suppliedTokens;

  ####################################################################
  #
  # Tally up the tokens.
  #
  ####################################################################

  my (%requiredHash, %suppliedHash);

  for (my $i = 0; $i < scalar(@requiredFields); $i++)
  {
    $requiredHash{$requiredFields[$i]} = ($requiredTokens[$i] eq "+") ? 1 : 0;
  }
  for (my $i = 0; $i < scalar(@suppliedFields); $i++)
  {
    $suppliedHash{$suppliedFields[$i]} = ($suppliedTokens[$i] eq "+") ? 1 : 0;
  }

  ####################################################################
  #
  # Delete the +/- fields. This depends on the value of the base field.
  #
  ####################################################################

  if ($requiredBase eq "all")
  {
    foreach my $field (keys %requiredHash)
    {
      if ($requiredHash{$field} == 1)
      {
        delete $requiredHash{$field};
      }
    }
    foreach my $field (keys %suppliedHash)
    {
      if ($suppliedHash{$field} == 1)
      {
        delete $suppliedHash{$field};
      }
    }
  }
  else
  {
    foreach my $field (keys %requiredHash)
    {
      if ($requiredHash{$field} == 0)
      {
        delete $requiredHash{$field};
      }
    }
    foreach my $field (keys %suppliedHash)
    {
      if ($suppliedHash{$field} == 0)
      {
        delete $suppliedHash{$field};
      }
    }
  }

  ####################################################################
  #
  # Compare the two normalized masks.
  #
  ####################################################################

  return ((join("|", sort keys %requiredHash)) eq (join("|", sort keys %suppliedHash)));
}
