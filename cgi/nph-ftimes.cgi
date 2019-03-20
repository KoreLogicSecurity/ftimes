#!/usr/bin/perl
######################################################################
#
# $Id: nph-ftimes.cgi,v 1.1.1.1 2002/01/18 03:16:35 mavrik Exp $
#
######################################################################
#
# Copyright 2000-2002 Klayton Monroe, Exodus Communications, Inc.
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
    '250' => "Ping Received",
    '251' => "LinkTest Succeeded",
    '252' => "Upload Succeeded, Relay Failed",
    '405' => "Method Not Allowed",
    '450' => "Invalid Query",
    '451' => "File Already Exists",
    '452' => "Undefined Username",
    '453' => "Username-ClientId Mismatch",
    '454' => "Content-Length Mismatch",
    '455' => "File Not Available",
    '460' => "RequiredMask-FieldMask Mismatch",
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

  my ($baseDirectory, $ftimes, $importFile);

  if ($^O =~ /MSWin32/i)
  { 
    $baseDirectory    = "c:/inetpub/wwwroot/integrity";
    $ftimes           = "c:/integrity/bin/ftimes.exe";
    binmode(STDIN);
    close(STDERR);
  }
  else
  { 
    $baseDirectory    = "/integrity";
    $ftimes           = "/usr/local/integrity/bin/ftimes";
    umask 022;
  } 

#######################################################################
#
# Site Specific Initialization
#
#######################################################################

  #####################################################################
  #
  # The import variable is used to reference an external file from
  # the configuration file that is constructed during a PUT request.
  # This is useful in situations where snapshots need to be relayed
  # to another server at some later point. If the variable is not
  # defined, no Import directive is added to the config file. If the
  # variable is set, its value should be a full path.
  #
  #####################################################################

  my $importFile      = undef;

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
  # Preprocess the query string.
  #
  ####################################################################

  my $queryString = $ENV{'QUERY_STRING'};
  $queryString =~ s/\+/ /g;
  $queryString =~ s/%([0-9a-fA-F]{2})/pack("c", hex($1))/ge;

  ####################################################################
  #
  # If this is a GET request, the client wants a config file.
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
        my $size = -s $getFile;
        if (open(FILE, "$getFile"))
        {
          $byteCount = $size;
          $returnCode = '200';
#FIXME Handle ReturnContent() failures.
          ReturnContent(\*FILE, $size, $returnCode, $returnCodes{$returnCode});
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
        $returnCode = '455';
        $context = "File=$getFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'};
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # If this is a PING request, check the client's output mask, return
  # a response, and quit.
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
      Finish($logHandle);
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'};
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # If this is a PUT request, read the data and store it directly
  # to disk. It's someone else's job to parse it. Our job is to
  # be efficient.
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
          ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
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
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '251';
        $context = undef;
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnSuccess($logHandle, $returnCode, $returnCodes{$returnCode});
        Finish($logHandle);
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
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$logFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $outFile)
      {
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$outFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $cfgFile)
      {
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$cfgFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      if (-e $rdyFile)
      {
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '451';
        $context = "File=$rdyFile";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      ##############################################################
      #
      # Suck in the log data and store it in a file. Abort, if the
      # file exists or can't be opened.
      #
      ##############################################################

      if (!open(FILE, ">$logFile"))
      {
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '500';
        $context = "File=$logFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      binmode(FILE);
      $byteCount = 0;
      my $n = ReadAndWriteData(\*FILE, $logLength);
      close(FILE);
      if ($n != $logLength)
      {
        $returnCode = '454';
        $context = "File=$logFile NRead=$n Length=$logLength";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
      $byteCount += $n;

      ##############################################################
      #
      # Suck in the out data and store it in a file. Abort, if the
      # file exists or can't be opened.
      #
      ##############################################################

      if (!open(FILE, ">$outFile"))
      {
        ReadAndChuckData($ENV{'CONTENT_LENGTH'}); # prevents a broken pipe
        $returnCode = '500';
        $context = "File=$outFile";
        $explanation = $!;
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }

      binmode(FILE);
      my $n = ReadAndWriteData(\*FILE, $outLength);
      close(FILE);
      if ($n != $outLength)
      {
        $returnCode = '454';
        $context = "File=$outFile NRead=$n Length=$outLength";
        $explanation = $returnCodes{$returnCode};
        LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
        ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
      }
      $byteCount += $n;

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
      if (defined $importFile)
      {
        print FILE "#\n";
        print FILE "Import=$importFile\n";
      }
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
      Finish($logHandle);
    }
    else
    {
      $returnCode = '450';
      $context = "QueryString=" . $ENV{'QUERY_STRING'};
      $explanation = $returnCodes{$returnCode};
      LogMessage($logHandle, $username, $cgiRemote, $cgiMethod, $cgiRequest, $byteCount, $startTime, $returnCode, $context, $explanation);
      ReturnFailure($logHandle, $returnCode, $returnCodes{$returnCode});
    }
  }

  ####################################################################
  #
  # Any other methods are not allowed.
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
  my ($fileHandle, $toread) = @_;
  my ($nread, $byteCount);

  $nread = $byteCount = 0;
  while($toread)
  {
    my $buf = ""; 
    $nread = sysread(STDIN, $buf, $toread);
    $byteCount += syswrite($fileHandle, $buf, $nread);
    $toread -= $nread;
  }
  return $byteCount;
}


######################################################################
#
# ReadAndChuckData
#
######################################################################

sub ReadAndChuckData
{
  my ($toread) = @_;

  while($toread)
  {
    my $buf = ""; 
    $toread -= sysread(STDIN, $buf, $toread);
  }
  return $toread;
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
  my ($fileHandle, $size, $statusCode, $reasonPhrase) = @_;
  print "HTTP/1.1 $statusCode $reasonPhrase\r\n";
  print "Server: $ENV{'SERVER_SOFTWARE'}\r\n";
  print "Content-Type: text/plain\r\n";
  print "Content-Length: $size\r\n";
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
