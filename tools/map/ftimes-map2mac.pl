#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-map2mac.pl,v 1.9 2004/04/21 01:29:59 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2004 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Create MAC/MACH timelines using FTimes map data.
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
  # Get Options.
  #
  ####################################################################

  my (%hOptions, $sRunMode);

  $sRunMode = shift || Usage($sProgram);

  if ($sRunMode =~ /^(-e|--extsort)$/)
  {
    if (!getopts('df:rs:T:w', \%hOptions))
    {
      Usage($sProgram);
    }
    $sRunMode = "extsort";
  }
  elsif ($sRunMode =~ /^(-i|--intsort)$/)
  {
    if (!getopts('df:rw', \%hOptions))
    {
      Usage($sProgram);
    }
    $sRunMode = "intsort";
  }
  else
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The Decode flag, '-d', is optional. Default value is 0.
  #
  ####################################################################

  my ($sDecode);

  $sDecode = (exists($hOptions{'d'})) ? 1 : 0;

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
  # The Reverse flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($sReverse);

  $sReverse = (exists($hOptions{'r'})) ? 1 : 0;

  ####################################################################
  #
  # The Sort flag, '-s', is optional. Default value is "sort".
  #
  ####################################################################

  my ($sSort);

  $sSort = (exists($hOptions{'s'})) ? $hOptions{'s'} : "sort";

  ####################################################################
  #
  # The TmpDir flag, '-T', is optional. Default value is $TMPDIR,
  # or if that is not defined, fall back to "/tmp".
  #
  ####################################################################

  my ($sTmpDir);

  $sTmpDir = (exists($hOptions{'T'})) ? $hOptions{'T'} : (defined($ENV{'TMPDIR'})) ? $ENV{'TMPDIR'} : "/tmp";

  ####################################################################
  #
  # The Windows flag, '-w', is optional. Default value is 0.
  #
  ####################################################################

  my ($sWindows);

  $sWindows = (exists($hOptions{'w'})) ? 1 : 0;

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
  # Process header.
  #
  ####################################################################

  my (@aFields, $sHeader, $sNIndex, $sMIndex, $sAIndex, $sCIndex, $sChIndex);

  if (!defined($sHeader = <$sFileHandle>))
  {
    print STDERR "$sProgram: File='$sFilename' Error='Unable to read header.'\n";
    exit(2);
  }
  $sHeader =~ s/[\r\n]+$//;
  @aFields = split(/\|/, $sHeader);
  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] eq "name")
    {
      $sNIndex = $sIndex;
    }
    elsif ($aFields[$sIndex] eq "mtime")
    {
      $sMIndex = $sIndex;
    }
    elsif ($aFields[$sIndex] eq "atime")
    {
      $sAIndex = $sIndex;
    }
    elsif ($aFields[$sIndex] eq "ctime")
    {
      $sCIndex = $sIndex;
    }
    elsif ($aFields[$sIndex] eq "chtime")
    {
      $sChIndex = $sIndex;
    }
  }
  if ($sWindows)
  {
    if (!defined($sNIndex) || !defined($sMIndex) || !defined($sAIndex) || !defined($sCIndex) || !defined($sChIndex))
    {
      print STDERR "$sProgram: File='$sFilename' Error='Missing one or more required header fields.'\n";
      exit(2);
    }
  }
  else
  {
    if (!defined($sNIndex) || !defined($sMIndex) || !defined($sAIndex) || !defined($sCIndex))
    {
      print STDERR "$sProgram: File='$sFilename' Error='Missing one or more required header fields.'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Open a pipe to the sort utility if in external sort mode.
  #
  ####################################################################

  if ($sRunMode =~ /^extsort$/)
  {
    my $sCommand = "$sSort -u -T $sTmpDir";
    if ($sReverse)
    {
      $sCommand .= " -r";
    }
    if (!open(SH, "|$sCommand"))
    {
      print STDERR "$sProgram: Command='$sCommand' Error='$!'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Process data.
  #
  ####################################################################

  my ($sMAC, $sName, %hUnique);

  if ($sWindows)
  {
    while (my $sLine = <$sFileHandle>)
    {
      $sLine =~ s/[\r\n]+$//;
      @aFields = split(/\|/, $sLine);
      foreach my $sTime (@aFields[$sMIndex, $sAIndex, $sCIndex, $sChIndex])
      {
        $sMAC  = ($sTime eq $aFields[$sMIndex]) ? "M" : ".";
        $sMAC .= ($sTime eq $aFields[$sAIndex]) ? "A" : ".";
        $sMAC .= ($sTime eq $aFields[$sCIndex]) ? "C" : ".";
        $sMAC .= ($sTime eq $aFields[$sChIndex]) ? "H" : ".";
        $sName = ($sDecode) ? URLDecode($aFields[$sNIndex]) : $aFields[$sNIndex];
        $sName =~ s/^"//;
        $sName =~ s/"$//;
        if ($sRunMode =~ /^extsort$/)
        {
          print SH "$sTime|$sMAC|$sName\n";
        }
        else
        {
          $hUnique{"$sTime|$sMAC|$sName"}++;
        }
      }
    }
  }
  else
  {
    while (my $sLine = <$sFileHandle>)
    {
      $sLine =~ s/[\r\n]+$//;
      @aFields = split(/\|/, $sLine);
      foreach my $sTime (@aFields[$sMIndex, $sAIndex, $sCIndex])
      {
        $sMAC  = ($sTime eq $aFields[$sMIndex]) ? "M" : ".";
        $sMAC .= ($sTime eq $aFields[$sAIndex]) ? "A" : ".";
        $sMAC .= ($sTime eq $aFields[$sCIndex]) ? "C" : ".";
        $sName = ($sDecode) ? URLDecode($aFields[$sNIndex]) : $aFields[$sNIndex];
        $sName =~ s/^"//;
        $sName =~ s/"$//;
        if ($sRunMode =~ /^extsort$/)
        {
          print SH "$sTime|$sMAC|$sName\n";
        }
        else
        {
          $hUnique{"$sTime|$sMAC|$sName"}++;
        }
      }
    }
  }

  ####################################################################
  #
  # Display sorted MAC list if in internal sort mode.
  #
  ####################################################################

  if ($sRunMode =~ /^intsort$/)
  {
    foreach my $sKey (($sReverse) ? reverse(sort(keys(%hUnique))) : sort(keys(%hUnique)))
    {
      print $sKey,"\n";
    }
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  close($sFileHandle);

  if ($sRunMode =~ /^extsort$/)
  {
    close(SH);
  }

  1;


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
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram {-e|--extsort} [-d] [-r] [-s file] [-T dir] [-w] -f {file|-}\n";
  print STDERR "       $sProgram {-i|--intsort} [-d] [-r] [-w] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-map2mac.pl - Create MAC/MACH timelines using FTimes map data

=head1 SYNOPSIS

B<ftimes-map2mac.pl> B<{-e|--extsort}> B<[-d]> B<[-r]> B<[-s file]> B<[-T dir]> B<[-w]> B<-f {file|-}>

B<ftimes-map2mac.pl> B<{-i|--intsort}> B<[-d]> B<[-r]> B<[-w]> B<-f {file|-}>

=head1 DESCRIPTION

This utility takes FTimes map data as input and produces a MAC or
MACH timeline as output. The letters M, A, and C, have the usual
meanings -- i.e. mtime, atime, and ctime. The H in MACH stands for
chtime which is NTFS specific. Output is written to stdout and has
the following format:

    datetime|mac/mach|name

Two sorting methods are supported: external and internal.  The
internal sorting method constructs timelines entirely within core
memory.  While this method is self-contained, its effectiveness
deteriorates as the amount of data increases.  The external sorting
method, on the other hand, requires an external sort utility, but
it is generally faster and can handle large data sets more effectively.
The external sorting method was designed to work with GNU sort.

=head1 OPTIONS

=over 4

=item B<-d>

Enables URL decoding of filenames.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-r>

Causes timeline to be output in reverse time order.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH environment variable. Alternate sort utilities
must support the C<-r>, C<-T>, and C<-u> options. This program's
external sorting method was designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is that specified by the TMPDIR environment
variable, or /tmp if TMPDIR is not set.

=item B<-w>

Enables support for Windows NT/2K map data.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1)

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
