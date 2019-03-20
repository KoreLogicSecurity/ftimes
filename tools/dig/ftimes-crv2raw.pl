#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-crv2raw.pl,v 1.6 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2006-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Carve blocks of data and assemble them into raw files.
#
######################################################################

use strict;
use Digest::MD5;
use Digest::SHA1;
use File::Basename;
use File::Path;
use Getopt::Std;

######################################################################
#
# Main Routine
#
######################################################################

  ####################################################################
  #
  # Punch in and go to work.
  #
  ####################################################################

  my ($sProgram);

  $sProgram = basename(__FILE__);

  ####################################################################
  #
  # Validation expressions.
  #
  ####################################################################

  my ($sErrorLimitRegex, $sHeaderRegex, $sIgnoreRegex, $sLineRegex);

  $sErrorLimitRegex = qq(\\d+);
  $sHeaderRegex = qq(name\|tag\|offset\|unit_size\|range_list);
  $sIgnoreRegex = qq(\\d+);
  $sLineRegex = qq("(.+)"\\|([\\w.-]+)\\|(\\d+|0x[0-9A-Fa-f]+)\\|(\\d+)\\|((?:\\d{1,10}(?:-\\d{1,10})?)(?:,(?:\\d{1,10}(?:-\\d{1,10})?))*));

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('d:e:Ff:i:mU', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # An output directory, '-d', is optional.
  #
  ####################################################################

  my ($sOutDir);

  $sOutDir = (exists($hOptions{'d'})) ? $hOptions{'d'} : "carve_tree";

  ####################################################################
  #
  # An ErrorLimit, '-e', is optional, but must be a decimal number.
  #
  ####################################################################

  my ($sErrorCount, $sErrorLimit);

  $sErrorLimit = (exists($hOptions{'e'})) ? $hOptions{'e'} : 1;

  if ($sErrorLimit !~ /^$sErrorLimitRegex$/)
  {
    print STDERR "$sProgram: ErrorLimit='$sErrorLimit' Error='Invalid error limit.'\n";
    exit(2);
  }

  $sErrorCount = 0;

  ####################################################################
  #
  # The ForceWrite, '-F', flag is optional.
  #
  ####################################################################

  my ($sForceWrite);

  $sForceWrite = (exists $hOptions{'F'}) ? 1 : 0;

  ####################################################################
  #
  # A filename is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($sFileHandle, $sFilename);

  if (!exists($hOptions{'f'}))
  {
    Usage($sProgram);
  }
  else
  {
    $sFilename = $hOptions{'f'};
    if (!defined($sFilename) || length($sFilename) < 1)
    {
      Usage($sProgram);
    }
    if (-f $sFilename)
    {
      if (!open(FH, "< $sFilename"))
      {
        print STDERR "$sProgram: File='$sFilename' Error='$!'\n";
        exit(2);
      }
      $sFileHandle = \*FH;
    }
    else
    {
      if ($sFilename ne '-')
      {
        print STDERR "$sProgram: File='$sFilename' Error='File must be regular.'\n";
        exit(2);
      }
      $sFileHandle = \*STDIN;
    }
  }

  ####################################################################
  #
  # The ShowMapOutput flag, '-m', is optional.
  #
  ####################################################################

  my ($sShowMapOutput);

  $sShowMapOutput = (exists($hOptions{'m'})) ? 1 : 0;

  ####################################################################
  #
  # An IgnoreNLines, '-i', is optional, but must be a decimal number.
  #
  ####################################################################

  my ($sIgnoreNLines);

  $sIgnoreNLines = (exists($hOptions{'i'})) ? $hOptions{'i'} : 0;

  if ($sIgnoreNLines !~ /^$sIgnoreRegex$/)
  {
    print STDERR "$sProgram: IgnoreNLines='$sIgnoreNLines' Error='Invalid ignore count.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A NoUrlDecode flag, '-U', is optional.
  #
  ####################################################################

  my ($sNoUrlDecode);

  $sNoUrlDecode = (exists($hOptions{'U'})) ? 1 : 0;

  ####################################################################
  #
  # If any arguments remain in the array, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # Create output directory, if necessary.
  #
  ####################################################################

  if (!-d $sOutDir && !mkdir($sOutDir, 0755))
  {
    print STDERR "$sProgram: Directory='$sOutDir' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Conditionally output a map header.
  #
  ####################################################################

  if ($sShowMapOutput)
  {
    print STDOUT "name|size|md5|sha1\n";
  }

  ####################################################################
  #
  # Skip ignore lines.
  #
  ####################################################################

  my ($sLineNumber);

  for ($sLineNumber = 1; $sLineNumber <= $sIgnoreNLines; $sLineNumber++)
  {
    <$sFileHandle>;
  }

  ####################################################################
  #
  # Loop over the remaining input. Put problem files on a blacklist.
  #
  ####################################################################

  my (%hBlackListed, $sRawHandle, $sLastFile, $sLastOffset, $sLine);

  for ($sLastFile = '', $sLastOffset = 0; $sLine = <$sFileHandle>; $sLineNumber++)
  {
    $sLine =~ s/[\r\n]+$//;

    ##################################################################
    #
    # Skip blank lines or lines that begin with a '#'.
    #
    ##################################################################

    next if ($sLine =~ /^$/ || $sLine =~ /^\#/);

    ##################################################################
    #
    # Validate the line. Continue, if a file has been blacklisted.
    #
    ##################################################################

    my $sCarveFile;
    my $sCarveType;
    my $sCarveOffset;
    my $sCarveUnitSize;
    my $sCarveRangeList;

    if ($sLine =~ /^$sLineRegex$/)
    {
      $sCarveFile = $1;
      $sCarveType = $2;
      $sCarveOffset = $3;
      $sCarveUnitSize = $4;
      $sCarveRangeList = $5;
    }
    elsif ($sLine =~ /^$sHeaderRegex$/)
    {
      next;
    }
    else
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Error='Line did not parse properly.'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      next;
    }

    my $sFirstRange = (split(/[-,]/, $sCarveRangeList))[0];

    if (!defined($sFirstRange) || $sCarveOffset != ($sFirstRange * $sCarveUnitSize))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Error='Offset does not correspond to the lower value of the first range.'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      next;
    }

    if ($hBlackListed{$sCarveFile})
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Warning='CarveFile has been blacklisted due to previous errors.'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      next;
    }

    $sCarveOffset = oct($sCarveOffset) if ($sCarveOffset =~ /^0x/);

    ##################################################################
    #
    # Only initialize variables when current file is new.
    #
    ##################################################################

    if ($sCarveFile ne $sLastFile)
    {
      close($sRawHandle) if (defined($sRawHandle));
      my $sRawFile = ($sNoUrlDecode) ? $sCarveFile : UrlDecode($sCarveFile);
      if (!open(RAW, "< $sRawFile"))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' CarveFile='$sCarveFile' Error='$!'\n";
        CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
        $hBlackListed{$sCarveFile} = 1;
        $sLastFile = '';
        next;
      }
      $sRawHandle = \*RAW;
      $sLastFile = $sCarveFile;
      $sLastOffset = 0;
    }

    ##################################################################
    #
    # Create a new output file, and make a place for it in the carve
    # tree. Do not overwrite existing files when the ForceWrite flag
    # is off.
    #
    # Note: The file's placement is derived from the subject's path.
    #
    ##################################################################

    my ($sOutFile, $sSubDir);

    $sOutFile = MakeCarveName($sOutDir, $sCarveFile, $sCarveOffset, $sCarveType);

    $sSubDir = dirname($sOutFile);

    if (!$sForceWrite && -f $sOutFile)
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' OutFile='$sOutFile' Error='Output file already exists.'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      $hBlackListed{$sCarveFile} = 1;
      $sLastFile = '';
      close($sRawHandle);
      next;
    }

    if (!-d $sSubDir && !mkpath($sSubDir, 0, 0755))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' SubDir='$sSubDir' Error='$!'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      $hBlackListed{$sCarveFile} = 1;
      $sLastFile = '';
      close($sRawHandle);
      next;
    }

    if (!open(OUT, "> $sOutFile"))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' OutFile='$sOutFile' Error='$!'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      $hBlackListed{$sCarveFile} = 1;
      $sLastFile = '';
      close($sRawHandle);
      next;
    }

    ##################################################################
    #
    # Initialize a new set of map attributes.
    #
    ##################################################################

    my ($oMd5, $oSha1, $sName, $sSize);

    $sName = $sOutFile;
    $sSize = 0;
    $oSha1 = Digest::SHA1->new;
    $oMd5 = Digest::MD5->new;

    ##################################################################
    #
    # Prepare the carve list, and carve the data from the file. Any
    # error in this loop must cause the subject to be blacklisted.
    #
    ##################################################################

    my (@aCarveList, $sError);

    if (!MakeCarveList($sCarveRangeList, $sCarveUnitSize, \@aCarveList, \$sError))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' OutFile='$sOutFile' Error='$sError'\n";
      CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
      $hBlackListed{$sCarveFile} = 1;
      $sLastFile = '';
      close($sRawHandle);
      next;
    }

    foreach my $sRange (@aCarveList)
    {
      ################################################################
      #
      # Verify the range. Then, grab the offset and byte count.
      #
      ################################################################

      if ($sRange !~ /^(\d+):(\d+)$/)
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' CarveFile='$sCarveFile' Range='$sRange' Error='Invalid range.'\n";
        CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
        $hBlackListed{$sCarveFile} = 1;
        $sLastFile = '';
        close($sRawHandle);
        last;
      }
      my ($sOffset, $sNWanted) = ($1, $2);

      ################################################################
      #
      # Seek (+/-) to the next offset.
      #
      ################################################################

      if (!SeekLoop($sRawHandle, $sOffset, $sLastOffset, \$sError))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' CarveFile='$sCarveFile' Offset='$sOffset' Error='$sError'\n";
        CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
        $hBlackListed{$sCarveFile} = 1;
        $sLastFile = '';
        close($sRawHandle);
        last;
      }
      $sLastOffset = $sOffset;

      ################################################################
      #
      # Read the requested amount of data, conditionally fold it into
      # a running hash, and write it out.
      #
      ################################################################

      my ($sNRead, $sRawData);

      $sRawData = '';
      $sNRead = read($sRawHandle, $sRawData, $sNWanted);
      if (!defined($sNRead))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' CarveFile='$sCarveFile' Offset='$sOffset' Error='$!'\n";
        CheckErrorCount($sProgram, $sErrorLimit, ++$sErrorCount);
        $hBlackListed{$sCarveFile} = 1;
        $sLastFile = '';
        close($sRawHandle);
        last;
      }
      if ($sNRead < $sNWanted)
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' CarveFile='$sCarveFile' Offset='$sOffset' Warning='Wanted $sNWanted bytes, got $sNRead.'\n";
      }
      $sLastOffset += $sNRead;
      $sSize += $sNRead;
      $oMd5->add($sRawData);
      $oSha1->add($sRawData);
      print OUT $sRawData;
    }

    ##################################################################
    #
    # Conditionally output map information about the carved file.
    #
    ##################################################################

    if ($sShowMapOutput)
    {
      my $sMd5 = $oMd5->hexdigest;
      my $sSha1 = $oSha1->hexdigest;
      print STDOUT "\"$sName\"|$sSize|$sSha1|$sMd5\n";
    }

    ##################################################################
    #
    # Cleanup and go to the next input line.
    #
    ##################################################################

    close(OUT);
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  close($sFileHandle);

  1;


######################################################################
#
# CheckErrorCount
#
######################################################################

sub CheckErrorCount
{
  my ($sProgram, $sErrorLimit, $sErrorCount) = @_;

  ####################################################################
  #
  # Check the error count, and abort if things have gotten too bad.
  # A limit of zero means there is no limit at all.
  #
  ####################################################################

  if ($sErrorLimit > 0 && $sErrorCount >= $sErrorLimit)
  {
    print STDERR "$sProgram: Error='Too many errors -- error limit is $sErrorLimit.'\n";
    exit(2);
  }

  1;
}


######################################################################
#
# MakeCarveList
#
######################################################################

sub MakeCarveList
{
  my ($sCarveRangeList, $sCarveUnitSize, $paCarveList, $psError) = @_;

  ####################################################################
  #
  # Check inputs.
  #
  ####################################################################

  if (
       !defined($sCarveUnitSize) ||
       !defined($sCarveUnitSize) ||
       !defined($paCarveList) ||
       !defined($psError)
     )
  {
    $$psError = "There are missing or undefined inputs.";
    return undef;
  }

  if ($sCarveUnitSize !~ /^(?:\d{1,10}(?:-\d{1,10})?)(?:,(?:\d{1,10}(?:-\d{1,10})?))*$/)
  {
    $$psError = "CarveRangeList ($sCarveRangeList) is not valid. Check the man page for proper syntax.";
    return undef;
  }

  if ($sCarveUnitSize !~ /^\d+$/ || $sCarveUnitSize < 1 || ($sCarveUnitSize > 1 && $sCarveUnitSize % 2 != 0))
  {
    $$psError = "CarveUnitSize ($sCarveUnitSize) is not valid. This value must be one or a nonzero multiple of two.";
    return undef;
  }

  ####################################################################
  #
  # Convert one or more ranges to a carve list.
  #
  ####################################################################

  foreach my $sCarveRange (split(/,/, $sCarveRangeList, -1))
  {
    if ($sCarveRange !~ /^(\d{1,10})(?:-(\d{1,10}))?$/)
    {
      $$psError = "CarveRange ($sCarveRange) is undefined or invalid.";
      return undef;
    }
    my $sLowerRange = $1;
    my $sUpperRange = (defined($2)) ? $2 : $sLowerRange;
    if ($sLowerRange > $sUpperRange)
    {
      $$psError = "LowerRange ($sLowerRange) exceeds UpperRange ($sUpperRange).";
      return undef;
    }
    if ($sLowerRange < 0)
    {
      $$psError = "LowerRange ($sLowerRange) is negative.";
      return undef;
    }
    if ($sUpperRange < 0)
    {
      $$psError = "UpperRange ($sUpperRange) is negative.";
      return undef;
    }
    my $sOffset = int($sLowerRange * $sCarveUnitSize);
    my $sNWanted = int(($sUpperRange - $sLowerRange + 1) * $sCarveUnitSize);
    push(@$paCarveList, "$sOffset:$sNWanted");
  }

  1;
}


######################################################################
#
# MakeCarveName
#
######################################################################

sub MakeCarveName
{
  my ($sOutDir, $sCarveFile, $sCarveOffset, $sCarveType) = @_;

  my $sName = $sCarveFile;
  $sName =~ s/^(?:[A-Za-z]:)?[\/\\]+//; # Remove leading path information.
  $sName =~ s/^[.]{2}([\/\\])/%2e%2e$1/g; # Remove leading "..".
  $sName =~ s/([\/\\])[.]{2}$/$1%2e%2e/g; # Remove trailing "..".
  $sName =~ s/([\/\\])[.]{2}([\/\\])/$1%2e%2e$2/g; # Remove embedded "..".
  $sName =~ s/([\/\\])[.]{2}([\/\\])/$1%2e%2e$2/g; # Remove embedded leftovers.
  $sName = $sOutDir . "/" . $sName . "." . $sCarveOffset . "." . $sCarveType;

  return $sName;
}


######################################################################
#
# SeekLoop
#
######################################################################

sub SeekLoop
{
  my ($sFileHandle, $sNewOffset, $sOldOffset, $psError) = @_;

  ####################################################################
  #
  # Seek, relative to the old offset, in a loop until the new offset
  # is reached. Seeks may be positive or negative.
  #
  ####################################################################

  my $sTmpOffset = $sNewOffset - $sOldOffset;

  while ($sTmpOffset != 0)
  {
    my $sToSeek = (abs($sTmpOffset) > 0x7fffffff) ? ($sTmpOffset >= 0) ? 0x7fffffff : -0x7fffffff : $sTmpOffset;
    if (!seek($sFileHandle, $sToSeek, 1))
    {
      $$psError = $!;
      return undef;
    }
    $sTmpOffset -= $sToSeek;
  }

  1;
}


######################################################################
#
# UrlDecode
#
######################################################################

sub UrlDecode
{
  my ($sRawString) = @_;
  $sRawString =~ s/\+/ /sg;
  $sRawString =~ s/%([0-9a-fA-F]{2})/pack('C', hex($1))/seg;
  return $sRawString;
}


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-FmU] [-e limit] [-d dir] [-i count] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}

=pod

=head1 NAME

ftimes-crv2raw.pl - Carve blocks of data and assemble them into raw files

=head1 SYNOPSIS

B<ftimes-crv2raw.pl> B<[-FmU]> B<[-d dir]> B<[-e limit]> B<[-i count]> B<-f {file|-}>

=head1 DESCRIPTION

This utility carves blocks of data and assembles them into raw files.
Input is taken from a '.crv' file, which has the following format:

    name|type|offset|unit_size|range_list

where

=over 4

=item name

This field contains the URL-encoded name of the subject file.  This is
the file that contains the data you wish to carve.  This field must
conform to the following syntax:

    "<name|path>"

If your '.crv' file does not use URL-encoded name fields, you should
also specify the B<-U> option to disable automatic URL-decoding.  If
only a name is specified, the corresponding subject file must reside
in the current working directory.  Both relative and full paths are
supported.

Note: The quotes in the above syntax are a required part of the field.

=item type

This field specifies the file type that is being carved from the
subject.  The value for this field is used as an extension, and it is
appended to the end of the output filename.  Type values are
restricted to the following character set: [0-9A-Za-z_.-]

=item offset

This field specifies the SOF (Start Of File) offset (in bytes)
relative to the beginning of the subject file.  The value for this
field is used as a suffix, and it is appended to the end of the output
filename.

=item unit_size

This field specifies the unit size (in bytes) of the blocks in the
range_list.  This value must be one or a nonzero multiple of two.

=item range_list

This field contains a comma delimited (with no intervening whitespace)
list of blocks or ranges that are to be carved.  The required syntax
is as follows:

    lower[[-upper][,lower[-upper]]...]

If a lower range value is specified without a corresponding upper
range value, the lower and upper values are assumed to be equal.  For
example, the following range list:

    0,512,1024

is equivalent to:

    0-0,512-512,1024-1024

The amount of data that will be carved for a given range is computed
as follows:

    carve_amount = (upper - lower + 1) * unit_size

Range lists are carved on a FIFO basis.  This makes it possible to
assemble carved blocks in any arbitrary order -- simply specify the
desired carve order when creating the '.crv' file.  For example, given
a unit_size of one and the following range list:

    512-1023,0-511,1024-1535

the carver will extract and assemble bytes 512-1023 first, bytes 0-511
second, and bytes 1024-1535 third.  Effectively, this represents a
block ordering of 2,1,3.  This stands in contrast to the following
range list, which has a block ordering of 1,2,3:

    0-511,512-1023,1024-1535

=back

=head1 OPTIONS

=over 4

=item B<-d dir>

Specifies the name of the output directory.  This is where carved
files will be stored.  If no directory is specified, a default
directory called 'carve_tree' is created in the current working
directory.  Carved output files are stored in directories/files that
are derived from the subject name (and path) with intermediate
directories being created as necessary.  The leading path prefix, if
any, is removed in the process so that all output files are contained
within the carve tree.  For example, the following input:

    "/evidence_locker_1/subject_1"|doc|26214400|512|51200-51220
    "/evidence_locker_2/subject_1"|doc|23533568|1|23533568-23544319
    "/evidence_locker_2/subject_2"|zip|11776256|1|11776256-11829164

will yield the following carve tree:

    carve_tree
      |
      + evidence_locker_1
      |   |
      |   - subject_1_26214400.doc (10752 bytes)
      |
      + evidence_locker_2
          |
          - subject_1_23533568.doc (10752 bytes)
          - subject_2_23552512.zip (52909 bytes)

Note: Unless the B<-F> is specified, this utility will abort if a file
in the output directory already exists.

=item B<-e limit>

Specifies the number of errors to allow before the carver will abort.
The default value is 1.  A value of zero means do not impose an error
limit.

=item B<-F>

Force existing output files to be overwritten.

=item B<-f {file|-}>

Specifies the name of the input file.  A value of '-' will cause input
to be read from stdin.

=item B<-i count>

Specifies the number of input lines to ignore.  By default, no lines
are ignored.

=item B<-m>

Causes the carver to display various map attributes for each file
carved.  This output is roughly equivalent to the following FTimes
FieldMask:

    none+size+md5+sha1

However, there are two differences between this output and regular
FTimes output: the name field is not URL-encoded, and it may be
specified as a relative path (depending on how it was specified in the
'.crv' file).

=item B<-U>

Do not attempt to URL-decode filenames -- i.e., assume that they are
not encoded.  This option is useful when you want to supply input from
a source other than FTimes-based utilities, which typically URL-encode
filenames.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1)

=head1 LICENSE

All documentation and code for this utility is distributed under same
terms and conditions as FTimes.

=cut
