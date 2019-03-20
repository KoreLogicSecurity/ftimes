#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-dig2ctx.pl,v 1.9 2004/04/26 03:22:44 mavrik Exp $
#
######################################################################
#
# Copyright 2002-2004 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Extract context around matched dig strings.
#
######################################################################

use strict;
use File::Basename;
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
  my $sLineRegex    = qq(^"(.+)"\\|(\\d+|0x[0-9A-Fa-f]+)\\|(.+)\$);
  my $sPrefixRegex  = qq(^\\d+\$);
  my $sSchemeRegex  = qq(^hex|url\$);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('c:e:f:hi:l:p:r:', \%hOptions))
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
      if (!open(FH, $sFilename))
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
  # An EncodingScheme, '-e', is optional, but must be {hex|url}.
  #
  ####################################################################

  my $sEncodingScheme = (exists($hOptions{'e'})) ? $hOptions{'e'} : "url";

  $sEncodingScheme =~ tr/A-Z/a-z/;

  if ($sEncodingScheme !~ /$sSchemeRegex/)
  {
    print STDERR "$sProgram: EncodingScheme='$sEncodingScheme' Error='Invalid encoding scheme.'\n";
    exit(2);
  }

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
  # A LH (left-hand) boundary, '-l', is optional.
  #
  ####################################################################

  my $sLHBoundary = (exists($hOptions{'l'})) ? $hOptions{'l'} : undef;

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

    my ($sAdjOffset, $sDigFile, $sDigOffset, $sDigString, $sRawFile, $sRawLength);

    $sLine =~ s/[\r\n]+$//;
    if ($sLine !~ /$sLineRegex/)
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' Line='$sLine' Error='Line did not parse properly.'\n";
      next;
    }
    next if $hBlackListed{$1};

    $sDigFile   = $1;
    $sRawFile   = URLDecode($sDigFile);
    $sDigOffset = $2;
    $sDigString = $3;
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
      if (!open(RAW, "<$sRawFile"))
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
    # Calculate the next seek offset, and seek to it.
    #
    ##################################################################

    my ($sAdjPrefixLength);

    $sAdjOffset = ($sDigOffset < $sReqPrefixLength) ? 0 : $sDigOffset - $sReqPrefixLength;

    $sAdjPrefixLength = $sDigOffset - $sAdjOffset;

    if (!seek($sRawHandle, $sAdjOffset - $sLastOffset, 1))
    {
      print STDERR "$sProgram: LineNumber='$sLineNumber' URLFilename='$sDigFile' Offset='$sAdjOffset' Error='$!'\n";
      $hBlackListed{$sDigFile} = 1;
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
        $sRawLHData = (reverse(split(/$sLHBoundary/, $sRawLHData, -1)))[0];
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
        $sRawRHData = (split(/$sRHBoundary/, $sRawRHData))[0];
      }
    }
    $sRawRHData .= '';

    ##################################################################
    #
    # Encode and print what's left of the LH, MH, and RH chunks.
    #
    ##################################################################

    my ($sEncodedData);

    if ($sEncodingScheme eq "hex")
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
  print STDERR "Usage: $sProgram [-h] [-e {url|hex}] [-c max-length] [-p max-length] [-l regex] [-r regex] [-i count] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-dig2ctx.pl.pl - Extract context around matched dig strings

=head1 SYNOPSIS

B<ftimes-dig2ctx.pl.pl> B<[-h]> B<[-e {url|hex}]> B<[-c max-length]> B<[-p max-length]> B<[-l regex]> B<[-r regex]> B<[-i count]> B<-f {file|-}>

=head1 DESCRIPTION

This utility extracts a variable amount of context around matched
dig strings using data collected with ftimes(1) or hipdig.pl. Data
collected by either of these tools has the following format:

    name|offset|string

Output from this utility is written to stdout and has the following
format:

    dig_name|dig_offset|dig_string|ctx_offset|lh_length|mh_length|rh_length|ctx_string

=head1 OPTIONS

=over 4

=item B<-c max-length>

Specifies the maximum context length.

=item B<-e {url|hex}>

Specifies the type of encoding to use when printing the context
(i.e. ctx_string).

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-h>

Print a header line.

=item B<-i count>

Specifies the number of input lines to ignore.

=item B<-l regex>

Specifies the left-hand boundary. This is a Perl regular expression
that can be used to limit the amount of context returned.

=item B<-p max-length>

Specifies the maximum prefix length.

=item B<-r regex>

Specifies the right-hand boundary. This is a Perl regular expression
that can be used to limit the amount of context returned.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), hipdig.pl

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
