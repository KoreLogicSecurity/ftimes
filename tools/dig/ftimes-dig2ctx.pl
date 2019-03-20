#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-dig2ctx.pl,v 1.26 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2002-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Extract context around matched dig strings.
#
######################################################################

use strict;
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

  my $sContextRegex = qq(^\\d+\$);
  my $sIgnoreRegex  = qq(^\\d+\$);
  my $sLineRegex    = qq(^"(.+)"\\|(normal|nocase|regexp)\\|([^|]*)\\|(\\d+|0x[0-9A-Fa-f]+)\\|(.+)\$);
  my $sLineRegexLegacy1 = qq(^"(.+)"\\|(\\d+|0x[0-9A-Fa-f]+)\\|(.+)\$); # FTimes Releases < 3.5.0
  my $sLineRegexLegacy2 = qq(^"(.+)"\\|(normal|nocase|regexp)\\|(\\d+|0x[0-9A-Fa-f]+)\\|(.+)\$); # FTimes Releases < 3.7.0
  my $sPrefixRegex  = qq(^\\d+\$);
  my $sSchemeRegex  = qq(^(file|hex|url)\$);
  my $sHeaderRegex  = qq(^name\|(?:type\|)?(?:tag\|)?offset\|string\$);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('c:d:e:f:hi:Ll:p:Rr:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # A ContextLength, '-c', is optional, but must be > zero.
  #
  ####################################################################

  my $sReqContextLength = (exists($hOptions{'c'})) ? $hOptions{'c'} : 64;

  if ($sReqContextLength !~ /$sContextRegex/ || $sReqContextLength <= 0)
  {
    print STDERR "$sProgram: ContextLength='$sReqContextLength' Error='Invalid context length.'\n";
    exit(2);
  }

  ####################################################################
  #
  # An output directory, '-d', is optional.
  #
  ####################################################################

  my $sOutDir = (exists($hOptions{'d'})) ? $hOptions{'d'} : "digtree";

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
  # An EncodingScheme, '-e', is optional, but must be {file|hex|url}.
  #
  ####################################################################

  my $sEncodingScheme = (exists($hOptions{'e'})) ? $hOptions{'e'} : "url";

  if ($sEncodingScheme !~ /$sSchemeRegex/i)
  {
    print STDERR "$sProgram: EncodingScheme='$sEncodingScheme' Error='Invalid encoding scheme. Use {file|hex|url}.'\n";
    exit(2);
  }
  $sEncodingScheme =~ tr/A-Z/a-z/;

  ####################################################################
  #
  # A PrintHeader flag, '-h', is optional.
  #
  ####################################################################

  my $sPrintHeader = (exists($hOptions{'h'})) ? 1 : 0;

  ####################################################################
  #
  # An IgnoreNLines, '-i', is optional, but must be a decimal number.
  #
  ####################################################################

  my $sIgnoreNLines = (exists($hOptions{'i'})) ? $hOptions{'i'} : 0;

  if ($sIgnoreNLines !~ /$sIgnoreRegex/)
  {
    print STDERR "$sProgram: IgnoreNLines='$sIgnoreNLines' Error='Invalid ignore count.'\n";
    exit(2);
  }

  ####################################################################
  #
  # Preserve LH (left-hand) boundary, '-L', is optional.
  #
  ####################################################################

  my $sPreserveLHBoundary = (exists($hOptions{'L'})) ? 1 : 0;

  ####################################################################
  #
  # A LH (left-hand) boundary, '-l', is optional.
  #
  ####################################################################

  my $sLHBoundary = (exists($hOptions{'l'})) ? $hOptions{'l'} : undef;

  ####################################################################
  #
  # Preserve RH (right-hand) boundary, '-R', is optional.
  #
  ####################################################################

  my $sPreserveRHBoundary = (exists($hOptions{'R'})) ? 1 : 0;

  ####################################################################
  #
  # A RH (right-hand) boundary, '-r', is optional.
  #
  ####################################################################

  my $sRHBoundary = (exists($hOptions{'r'})) ? $hOptions{'r'} : undef;

  ####################################################################
  #
  # A PrefixLength, '-p', is optional, but must be <= ContextLength.
  #
  ####################################################################

  my $sReqPrefixLength = (exists($hOptions{'p'})) ? $hOptions{'p'} : 0;

  if ($sReqPrefixLength !~ /$sPrefixRegex/)
  {
    print STDERR "$sProgram: PrefixLength='$sReqPrefixLength' Error='Invalid prefix length.'\n";
    exit(2);
  }

  if ($sReqPrefixLength > $sReqContextLength)
  {
    print STDERR "$sProgram: PrefixLength='$sReqPrefixLength' Error='PrefixLength must not exceed ContextLength.'\n";
    exit(2);
  }

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

  if ($sEncodingScheme =~ /^file$/ && (-d $sOutDir || !mkdir($sOutDir, 0755)))
  {
    print STDERR "$sProgram: Directory='$sOutDir' Error='Directory exists or could not be created.'\n";
    exit(2);
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
  # Print header line, if requested.
  #
  ####################################################################

  if ($sPrintHeader)
  {
    print "dig_name|dig_offset|dig_string|ctx_offset|lh_length|mh_length|rh_length|ctx_string\n";
  }

  ####################################################################
  #
  # Loop over the remaining input. Put problem files on a blacklist.
  #
  ####################################################################

  my (%hBlackListed, $sRawHandle, $sLastFile, $sLastOffset, $sLine);

  for ($sLastFile = '', $sLastOffset = 0; $sLine = <$sFileHandle>; $sLineNumber++)
  {
    ##################################################################
    #
    # Validate the line. Continue, if a file has been blacklisted.
    #
    ##################################################################

    my ($sAdjOffset, $sDigFile, $sDigOffset, $sDigString, $sDigTag, $sDigType, $sRawFile, $sRawLength);

    $sLine =~ s/[\r\n]+$//;
    if ($sLine =~ /$sLineRegex/)
    {
      $sDigFile = $1;
      $sDigType = $2;
      $sDigTag = $3;
      $sDigOffset = $4;
      $sDigString = $5;
    }
    elsif ($sLine =~ /$sLineRegexLegacy2/) # FTimes Releases < 3.7.0
    {
      $sDigFile = $1;
      $sDigType = $2;
      $sDigOffset = $3;
      $sDigString = $4;
    }
    elsif ($sLine =~ /$sLineRegexLegacy1/) # FTimes Releases < 3.5.0
    {
      $sDigFile = $1;
      $sDigOffset = $2;
      $sDigString = $3;
    }
    elsif ($sLine =~ /$sHeaderRegex/)
    {
      next;
    }
    else
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Error='Line did not parse properly.'\n";
      next;
    }

    $sRawFile = URLDecode($sDigFile);
    if ($hBlackListed{$sDigFile})
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Warning='File has been blacklisted due to previous errors.'\n";
      next;
    }

    $sRawLength = length(URLDecode($sDigString));

    $sDigOffset = oct($sDigOffset) if ($sDigOffset =~ /^0x/);

    ##################################################################
    #
    # Only initialize variables when current file is new.
    #
    ##################################################################

    if ($sDigFile ne $sLastFile)
    {
      close($sRawHandle) if (defined($sRawHandle));
      if (!open(RAW, "< $sRawFile"))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' URLFilename='$sDigFile' Error='$!'\n";
        $hBlackListed{$sDigFile} = 1;
        $sLastFile = '';
        next;
      }
      $sRawHandle = \*RAW;
      $sLastFile = $sDigFile;
      $sLastOffset = 0;
    }

    ##################################################################
    #
    # Calculate the next seek offset (+/-), and seek to it.
    #
    ##################################################################

    my ($sAdjPrefixLength, $sTmpOffset);

    $sAdjOffset = ($sDigOffset < $sReqPrefixLength) ? 0 : $sDigOffset - $sReqPrefixLength;

    $sAdjPrefixLength = $sDigOffset - $sAdjOffset;

    $sTmpOffset = $sAdjOffset - $sLastOffset;

    while ($sTmpOffset != 0)
    {
      my $sToSeek = (abs($sTmpOffset) > 0x7fffffff) ? ($sTmpOffset >= 0) ? 0x7fffffff : -0x7fffffff : $sTmpOffset;
      if (!seek($sRawHandle, $sToSeek, 1))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' URLFilename='$sDigFile' Offset='$sAdjOffset' Error='$!'\n";
        $hBlackListed{$sDigFile} = 1;
        last;
      }
      $sTmpOffset -= $sToSeek;
    }
    if ($hBlackListed{$sDigFile})
    {
      $sLastFile = '';
      close($sRawHandle);
      next;
    }
    $sLastOffset = $sAdjOffset;

    ##################################################################
    #
    # Read the context data from the file.
    #
    ##################################################################

    my ($sAdjContextLength, $sRawData, $sNRead);

    $sRawData = '';
    $sAdjContextLength = ($sAdjPrefixLength < $sReqPrefixLength) ? $sReqContextLength - ($sReqPrefixLength - $sAdjPrefixLength) : $sReqContextLength;
    $sNRead = read($sRawHandle, $sRawData, $sAdjContextLength);
    if (!defined($sNRead))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' URLFilename='$sDigFile' Offset='$sAdjOffset' Error='$!'\n";
      $hBlackListed{$sDigFile} = 1;
      $sLastFile = '';
      close($sRawHandle);
      next;
    }
    if ($sNRead < $sAdjContextLength)
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' URLFilename='$sDigFile' Offset='$sAdjOffset' Error='Wanted $sAdjContextLength bytes, got $sNRead.'\n";
    }
    $sLastOffset += $sNRead;

    ##################################################################
    #
    # Adjust LH, MH, and RH lengths to reflect reality.
    #
    ##################################################################

    my ($sLHLength, $sMHLength, $sRHLength);

    if ($sNRead < $sAdjPrefixLength)
    {
      $sLHLength = $sNRead;
      $sMHLength = 0;
      $sRHLength = 0;
    }
    else
    {
      if ($sNRead < $sAdjPrefixLength + $sRawLength)
      {
        $sLHLength = $sAdjPrefixLength;
        $sMHLength = $sNRead - $sLHLength;
        $sRHLength = 0;
      }
      else
      {
        $sLHLength = $sAdjPrefixLength;
        $sMHLength = $sRawLength;
        $sRHLength = $sNRead - $sLHLength - $sMHLength;
      }
    }

    ##################################################################
    #
    # Split off LH chunk, and apply LH regex if necessary.
    #
    ##################################################################

    my ($sRawLHData, $sRawMHData, $sRawRHData);

    if ($sLHLength > 0)
    {
      $sRawLHData = substr($sRawData, 0, $sLHLength);
      if (defined($sLHBoundary))
      {
        if ($sPreserveLHBoundary)
        {
          $sRawLHData = join("", (reverse(split(/($sLHBoundary)/, $sRawLHData, -1)))[1,0]);
        }
        else
        {
          $sRawLHData = (reverse(split(/$sLHBoundary/, $sRawLHData, -1)))[0];
        }
        $sAdjOffset += ($sLHLength - length($sRawLHData));
      }
    }
    $sRawLHData .= '';

    ##################################################################
    #
    # Split off MH chunk.
    #
    ##################################################################

    if ($sMHLength > 0)
    {
      $sRawMHData = substr($sRawData, $sLHLength, $sMHLength);
    }
    $sRawMHData .= '';

    ##################################################################
    #
    # Split off RH chunk, and apply RH regex if necessary.
    #
    ##################################################################

    if ($sRHLength > 0)
    {
      $sRawRHData = substr($sRawData, $sLHLength + $sMHLength, $sRHLength);
      if (defined($sRHBoundary))
      {
        if ($sPreserveRHBoundary)
        {
          $sRawRHData = join("", (split(/($sRHBoundary)/, $sRawRHData))[0,1]);
        }
        else
        {
          $sRawRHData = (split(/$sRHBoundary/, $sRawRHData))[0];
        }
      }
    }
    $sRawRHData .= '';

    ##################################################################
    #
    # Encode and print what's left of the LH, MH, and RH chunks.
    #
    ##################################################################

    my ($sEncodedData);

    if ($sEncodingScheme eq "file")
    {
      my ($sDir, $sName);
      $sName = $sDigFile;
      $sName =~ s/^(?:[A-Za-z]:)?[\/\\]+//; # Remove leading path information.
      $sName =~ s/^[.]{2}([\/\\])/%2e%2e$1/g; # Remove leading "..".
      $sName =~ s/([\/\\])[.]{2}$/$1%2e%2e/g; # Remove trailing "..".
      $sName =~ s/([\/\\])[.]{2}([\/\\])/$1%2e%2e$2/g; # Remove embedded "..".
      $sName =~ s/([\/\\])[.]{2}([\/\\])/$1%2e%2e$2/g; # Remove embedded leftovers.
      $sName = $sOutDir . "/" . $sName . "." . $sAdjOffset . "_" . ($sDigOffset - $sAdjOffset) . "_" . $sMHLength;
      $sDir = dirname($sName);
      if ((!-d $sDir && !mkpath($sDir, 0, 0755)) || !open(OUT, "> $sName"))
      {
        print STDERR "$sProgram: LineNumber='$sLineNumber' OutFilename='$sName' Error='$!'\n";
        $hBlackListed{$sDigFile} = 1;
        $sLastFile = '';
        next;
      }
      print OUT $sRawLHData . $sRawMHData . $sRawRHData;
      close(OUT);
      $sEncodedData = $sName;
    }
    elsif ($sEncodingScheme eq "hex")
    {
      $sEncodedData = HexEncode($sRawLHData . $sRawMHData . $sRawRHData);
    }
    else
    {
      $sEncodedData = URLEncode($sRawLHData . $sRawMHData . $sRawRHData);
    }
    print "\"$sDigFile\"|$sDigOffset|$sDigString|$sAdjOffset|$sLHLength|$sMHLength|$sRHLength|$sEncodedData\n";
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
# HexEncode
#
######################################################################

sub HexEncode
{
  my ($sHexString) = @_;

  $sHexString =~ s/(.)/sprintf("%02x",unpack('C',$1))/seg;
  return $sHexString;
}


######################################################################
#
# URLDecode
#
######################################################################

sub URLDecode
{
  my ($sRawString) = @_;
  $sRawString =~ s/\+/ /sg;
  $sRawString =~ s/%([0-9a-fA-F]{2})/pack('C', hex($1))/seg;
  return $sRawString;
}


######################################################################
#
# URLEncode
#
######################################################################

sub URLEncode
{
  my ($sURLString) = @_;

  $sURLString =~ s/([^!-\$&-*,-{}~ ])/sprintf("%%%02x",unpack('C',$1))/seg;
  $sURLString =~ s/ /+/sg;
  return $sURLString;
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
  print STDERR "Usage: $sProgram [-hLR] [-d dir] [-e {file|hex|url}] [-c length] [-p length] [-l regex] [-r regex] [-i count] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-dig2ctx.pl - Extract context around matched dig strings

=head1 SYNOPSIS

B<ftimes-dig2ctx.pl> B<[-hLR]> B<[-d dir]> B<[-e {file|hex|url}]> B<[-c length]> B<[-p length]> B<[-l regex]> B<[-r regex]> B<[-i count]> B<-f {file|-}>

=head1 DESCRIPTION

This utility extracts a variable amount of context around matched
dig strings using data collected with ftimes(1) or hipdig(1). Data
collected by either of these tools has the following format:

    name|type|offset|string

or for FTimes releases < 3.5.0

    name|offset|string

Output from this utility is written to stdout and has the following
format:

    dig_name|dig_offset|dig_string|ctx_offset|lh_length|mh_length|rh_length|ctx_string

=head1 OPTIONS

=over 4

=item B<-c length>

Specifies the desired context length in bytes.  You may get less
than this amount depending on where the match occurrs and the size
of the input file.

=item B<-d dir>

Specifies the name of the output directory.  The default name is
digtree.  This option is ignored unless the encoding scheme, B<-e>,
is set to file.  Note: The program will abort if the specified or
default directory exists.

=item B<-e {file|hex|url}>

Specifies the type of encoding to use when printing the context
(i.e., ctx_string).

If file is specified, then a new file containing the requested
context in raw form will be created under the directory specified
by the B<-d> option.  The name and location of this file will be
listed in the ctx_string field.  The name format used for these
files is as follows:

  <relative_dig_name>.<ctx_offset>_<relative_dig_offset>_<mh_length>

where <relative_dig_name> is the same as <dig_name> except that
leading path information has been removed, and <relative_dig_offset>
is the offset of the dig string in the newly created file.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-h>

Print a header line.

=item B<-i count>

Specifies the number of input lines to ignore.

=item B<-L>

Preserve the contents of the left-hand boundary. This option is
disabled by default.

=item B<-l regex>

Specifies the left-hand boundary. This is a Perl regular expression
that can be used to limit the amount of context returned.

=item B<-p length>

Specifies the desired prefix length in bytes.  You may get less
than this amount depending on where the match occurrs in the input
file.

=item B<-R>

Preserve the contents of the right-hand boundary. This option is
disabled by default.

=item B<-r regex>

Specifies the right-hand boundary. This is a Perl regular expression
that can be used to limit the amount of context returned.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), hipdig(1)

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
