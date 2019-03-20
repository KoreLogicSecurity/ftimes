#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-dig2ctx.pl,v 1.4 2003/03/26 21:39:17 mavrik Exp $
#
######################################################################
#
# Copyright 2002-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Extract context around matched dig strings.
#
######################################################################

use strict;
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

  my ($program);

  $program = "ftimes-dig2ctx.pl";

  ####################################################################
  #
  # Validation expressions.
  #
  ####################################################################

  my $contextRegex = qq(^\\d+\$);
  my $ignoreRegex  = qq(^\\d+\$);
  my $lineRegex    = qq(^"(.+)"\\|(\\d+)\\|(.+)\$);
  my $prefixRegex  = qq(^\\d+\$);
  my $schemeRegex  = qq(^hex|url\$);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('c:e:f:hi:l:p:r:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # A ContextLength, '-c', is optional, but must be > zero.
  #
  ####################################################################

  my $reqContextLength = (exists($options{'c'})) ? $options{'c'} : 64;

  if ($reqContextLength !~ /$contextRegex/ || $reqContextLength <= 0)
  {
    print STDERR "$program: ContextLength='$reqContextLength' Error='Invalid context length.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A filename is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($fileHandle, $filename);

  if (!exists($options{'f'}))
  {
    Usage($program);
  }
  else
  {
    $filename = $options{'f'};
    if (!defined($filename) || length($filename) < 1)
    {
      Usage($program);
    }
    if (-f $filename)
    {
      if (!open(FH, $filename))
      {
        print STDERR "$program: File='$filename' Error='$!'\n";
        exit(2);
      }
      $fileHandle = \*FH;
    }
    else
    {
      if ($filename ne '-')
      {
        print STDERR "$program: File='$filename' Error='File must be regular.'\n";
        exit(2);
      }
      $fileHandle = \*STDIN;
    }
  }

  ####################################################################
  #
  # An EncodingScheme, '-e', is optional, but must be {hex|url}.
  #
  ####################################################################

  my $encodingScheme = (exists($options{'e'})) ? $options{'e'} : "url";

  $encodingScheme =~ tr/A-Z/a-z/;

  if ($encodingScheme !~ /$schemeRegex/)
  {
    print STDERR "$program: EncodingScheme='$encodingScheme' Error='Invalid encoding scheme.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A PrintHeader flag, '-h', is optional.
  #
  ####################################################################

  my $printHeader = (exists($options{'h'})) ? 1 : 0;

  ####################################################################
  #
  # An IgnoreNLines, '-i', is optional, but must be a decimal number.
  #
  ####################################################################

  my $ignoreNLines = (exists($options{'i'})) ? $options{'i'} : 0;

  if ($ignoreNLines !~ /$ignoreRegex/)
  {
    print STDERR "$program: IgnoreNLines='$ignoreNLines' Error='Invalid ignore count.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A LH (left-hand) boundary, '-l', is optional.
  #
  ####################################################################

  my $lhBoundary = (exists($options{'l'})) ? $options{'l'} : undef;

  ####################################################################
  #
  # A RH (right-hand) boundary, '-r', is optional.
  #
  ####################################################################

  my $rhBoundary = (exists($options{'r'})) ? $options{'r'} : undef;

  ####################################################################
  #
  # A PrefixLength, '-p', is optional, but must be <= ContextLength.
  #
  ####################################################################

  my $reqPrefixLength = (exists($options{'p'})) ? $options{'p'} : 0;

  if ($reqPrefixLength !~ /$prefixRegex/)
  {
    print STDERR "$program: PrefixLength='$reqPrefixLength' Error='Invalid prefix length.'\n";
    exit(2);
  }

  if ($reqPrefixLength > $reqContextLength)
  {
    print STDERR "$program: PrefixLength='$reqPrefixLength' Error='PrefixLength must not exceed ContextLength.'\n";
    exit(2);
  }

  ####################################################################
  #
  # If any arguments remain in the array, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($program);
  }

  ####################################################################
  #
  # Skip ignore lines.
  #
  ####################################################################

  my ($lineNumber);

  for ($lineNumber = 1; $lineNumber <= $ignoreNLines; $lineNumber++)
  {
    <$fileHandle>;
  }

  ####################################################################
  #
  # Print header line, if requested.
  #
  ####################################################################

  if ($printHeader)
  {
    print "dig_name|dig_offset|dig_string|ctx_offset|lh_length|mh_length|rh_length|ctx_string\n";
  }

  ####################################################################
  #
  # Loop over the remaining input. Put problem files on a blacklist.
  #
  ####################################################################

  my (%blackListed, $rawHandle, $lastFile, $lastOffset, $line);

  for ($lastFile = '', $lastOffset = 0; $line = <$fileHandle>; $lineNumber++)
  {
    ##################################################################
    #
    # Validate the line. Continue, if a file has been blacklisted.
    #
    ##################################################################

    my ($adjOffset, $digFile, $digOffset, $digString, $rawFile, $rawLength);

    $line =~ s/[\r\n]+$//;
    if ($line !~ /$lineRegex/)
    {
      print STDERR "$program: LineNumber='$lineNumber' Line='$line' Error='Line did not parse properly.'\n";
      next;
    }
    next if $blackListed{$1};

    $digFile   = $1;
    $rawFile   = URLDecode($digFile);
    $digOffset = $2;
    $digString = $3;
    $rawLength = length(URLDecode($digString));

    ##################################################################
    #
    # Only initialize variables when current file is new.
    #
    ##################################################################

    if ($digFile ne $lastFile)
    {
      close($rawHandle) if (defined($rawHandle));
      if (!open(RAW, "<$rawFile"))
      {
        print STDERR "$program: LineNumber='$lineNumber' URLFilename='$digFile' Error='$!'\n";
        $blackListed{$digFile} = 1;
        $lastFile = '';
        next;
      }
      $rawHandle = \*RAW;
      $lastFile = $digFile;
      $lastOffset = 0;
    }

    ##################################################################
    #
    # Calculate the next seek offset, and seek to it.
    #
    ##################################################################

    my ($adjPrefixLength);

    $adjOffset = ($digOffset < $reqPrefixLength) ? 0 : $digOffset - $reqPrefixLength;

    $adjPrefixLength = $digOffset - $adjOffset;

    if (!seek($rawHandle, $adjOffset - $lastOffset, 1))
    {
      print STDERR "$program: LineNumber='$lineNumber' URLFilename='$digFile' Offset='$adjOffset' Error='$!'\n";
      $blackListed{$digFile} = 1;
      $lastFile = '';
      close($rawHandle);
      next;
    }
    $lastOffset = $adjOffset;

    ##################################################################
    #
    # Read the context data from the file.
    #
    ##################################################################

    my ($adjContextLength, $rawData, $nRead);

    $rawData = '';
    $adjContextLength = ($adjPrefixLength < $reqPrefixLength) ? $reqContextLength - ($reqPrefixLength - $adjPrefixLength) : $reqContextLength;
    $nRead = read($rawHandle, $rawData, $adjContextLength);
    if (!defined($nRead))
    {
      print STDERR "$program: LineNumber='$lineNumber' URLFilename='$digFile' Offset='$adjOffset' Error='$!'\n";
      $blackListed{$digFile} = 1;
      $lastFile = '';
      close($rawHandle);
      next;
    }
    if ($nRead < $adjContextLength)
    {
      print STDERR "$program: LineNumber='$lineNumber' URLFilename='$digFile' Offset='$adjOffset' Error='Wanted $adjContextLength bytes, got $nRead.'\n";
    }
    $lastOffset += $nRead;

    ##################################################################
    #
    # Adjust LH, MH, and RH lengths to reflect reality.
    #
    ##################################################################

    my ($lhLength, $mhLength, $rhLength);

    if ($nRead < $adjPrefixLength)
    {
      $lhLength = $nRead;
      $mhLength = 0;
      $rhLength = 0;
    }
    else
    {
      if ($nRead < $adjPrefixLength + $rawLength)
      {
        $lhLength = $adjPrefixLength;
        $mhLength = $nRead - $lhLength;
        $rhLength = 0;
      }
      else
      {
        $lhLength = $adjPrefixLength;
        $mhLength = $rawLength;
        $rhLength = $nRead - $lhLength - $mhLength;
      }
    }

    ##################################################################
    #
    # Split off LH chunk, and apply LH regex if necessary.
    #
    ##################################################################

    my ($rawLHData, $rawMHData, $rawRHData);

    if ($lhLength > 0)
    {
      $rawLHData = substr($rawData, 0, $lhLength);
      if (defined($lhBoundary))
      {
        $rawLHData = (reverse(split(/$lhBoundary/, $rawLHData, -1)))[0];
        $adjOffset += ($lhLength - length($rawLHData));
      }
    }
    $rawLHData .= '';

    ##################################################################
    #
    # Split off MH chunk.
    #
    ##################################################################

    if ($mhLength > 0)
    {
      $rawMHData = substr($rawData, $lhLength, $mhLength);
    }
    $rawMHData .= '';

    ##################################################################
    #
    # Split off RH chunk, and apply RH regex if necessary.
    #
    ##################################################################

    if ($rhLength > 0)
    {
      $rawRHData = substr($rawData, $lhLength + $mhLength, $rhLength);
      if (defined($rhBoundary))
      {
        $rawRHData = (split(/$rhBoundary/, $rawRHData))[0];
      }
    }
    $rawRHData .= '';

    ##################################################################
    #
    # Encode and print what's left of the LH, MH, and RH chunks.
    #
    ##################################################################

    my ($encodedData);

    if ($encodingScheme eq "hex")
    {
      $encodedData = HexEncode($rawLHData . $rawMHData . $rawRHData);
    }
    else
    {
      $encodedData = URLEncode($rawLHData . $rawMHData . $rawRHData);
    }
    print "\"$digFile\"|$digOffset|$digString|$adjOffset|$lhLength|$mhLength|$rhLength|$encodedData\n";
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  close($fileHandle);

  1;


######################################################################
#
# HexEncode
#
######################################################################

sub HexEncode
{
  my ($hexString) = @_;

  $hexString =~ s/(.)/sprintf("%02x",unpack('C',$1))/seg;
  return $hexString;
}


######################################################################
#
# URLDecode
#
######################################################################

sub URLDecode
{
  my ($rawString) = @_;
  $rawString =~ s/\+/ /sg;
  $rawString =~ s/%([0-9a-fA-F]{2})/pack('C', hex($1))/seg;
  return $rawString;
}


######################################################################
#
# URLEncode
#
######################################################################

sub URLEncode
{
  my ($urlString) = @_;

  $urlString =~ s/([^!-\$&-*,-{}~ ])/sprintf("%%%02x",unpack('C',$1))/seg;
  $urlString =~ s/ /+/sg;
  return $urlString;
}


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($program) = @_;
  print STDERR "\n";
  print STDERR "Usage: $program [-h] [-e {url|hex}] [-c max-length] [-p max-length] [-l regex] [-r regex] [-i count] -f {file|-}\n";
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

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

ftimes(1), hipdig.pl

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
