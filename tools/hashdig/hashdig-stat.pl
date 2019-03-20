#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-stat.pl,v 1.4 2005/05/27 00:54:48 mavrik Exp $
#
######################################################################
#
# Copyright 2004-2005 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Produce statistics on HashDig files.
#
######################################################################

use strict;
use DB_File;
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

  my (%hOptions);

  if (!getopts('t:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The FileType flag, '-t', is required.
  #
  ####################################################################

  my ($sFileType, $sProcessFile);

  $sFileType = (exists($hOptions{'t'})) ? $hOptions{'t'} : undef;

  if (!defined($sFileType))
  {
    Usage($sProgram);
  }

  if ($sFileType =~ /^DB$/i)
  {
    $sProcessFile = \&ProcessDBFile;
  }
  elsif ($sFileType =~ /^HD$/i)
  {
    $sProcessFile = \&ProcessHDFile;
  }
  else
  {
    print STDERR "$sProgram: FileType='$sFileType' Error='Invalid file type.'\n";
    exit(2);
  }

  ####################################################################
  #
  # If there isn't at least one argument left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) < 1)
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # Calculate statistics for each file.
  #
  ####################################################################

  my ($sKTotal, $sUTotal, $sITotal, $sATotal) = (0, 0, 0, 0);
  my ($sError);

  printf("%9s %9s %9s %9s %s\n", "KCount", "UCount", "ICount", "ACount", "Filename");

  foreach my $sFile (@ARGV)
  {
    my ($sKCount, $sUCount, $sICount, $sACount) = (0, 0, 0, 0);
    if (!defined(&$sProcessFile($sFile, \$sKCount, \$sUCount, \$sICount, \$sACount, \$sError)))
    {
      print STDERR "$sProgram: File='$sFile' Error='$sError'\n";
    }
    printf("%9d %9d %9d %9d %s\n", $sKCount, $sUCount, $sICount, $sACount, $sFile);
    $sKTotal += $sKCount;
    $sUTotal += $sUCount;
    $sITotal += $sICount;
    $sATotal += $sACount;
    if ($sICount > 0)
    {
      my $sRecordCount = ($sICount == 1) ? "$sICount record" : "$sICount records";
      print STDERR "$sProgram: File='$sFile' Error='Check file type, format, and integrity -- $sRecordCount did not parse properly.'\n";
    }
  }

  printf("%9d %9d %9d %9d %s\n", $sKTotal, $sUTotal, $sITotal, $sATotal, "totals");

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  1;


######################################################################
#
# ProcessDBFile
#
######################################################################

sub ProcessDBFile
{
  my ($sFile, $psKCount, $psUCount, $psICount, $psACount, $psError) = @_;

  ####################################################################
  #
  # Open the db file.
  #
  ####################################################################

  my (%hDBList);

  if (!tie(%hDBList, "DB_File", $sFile, O_RDONLY, 0644, $DB_BTREE))
  {
    $$psError = $!;
    return undef;
  }

  ####################################################################
  #
  # Enumerate the file and record known/unknown counts.
  #
  ####################################################################

  while (my ($sHash, $sCategory) = each(%hDBList))
  {
    if ($sHash =~ /^[0-9A-Fa-f]{32}$/ && $sCategory =~ /^([KU])$/)
    {
      $$psKCount++ if ($1 eq "K");
      $$psUCount++ if ($1 eq "U");
    }
    else
    {
      $$psICount++;
    }
    $$psACount++;
  }

  ####################################################################
  #
  # Close the db file.
  #
  ####################################################################

  untie(%hDBList);
}


######################################################################
#
# ProcessHDFile
#
######################################################################

sub ProcessHDFile
{
  my ($sFile, $psKCount, $psUCount, $psICount, $psACount, $psError) = @_;

  ####################################################################
  #
  # Open the hd file.
  #
  ####################################################################

  if (!open(FH, "< $sFile"))
  {
    $$psError = $!;
    return undef;
  }

  ####################################################################
  #
  # Enumerate the file and record known/unknown counts.
  #
  ####################################################################

  while (my $sLine = <FH>)
  {
    if ($sLine =~ /^[0-9a-fA-F]{32}\|([KU])$/o)
    {
      $$psKCount++ if ($1 eq "K");
      $$psUCount++ if ($1 eq "U");
    }
    elsif ($sLine =~ /^([KU])\|[0-9a-fA-F]{32}$/o)
    {
      $$psKCount++ if ($1 eq "K");
      $$psUCount++ if ($1 eq "U");
    }
    else
    {
      $$psICount++;
    }
    $$psACount++;
  }

  ####################################################################
  #
  # Close the hd file.
  #
  ####################################################################

  close(FH);
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
  print STDERR "Usage: $sProgram -t {db|hd} file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-stat.pl - Produce statistics on HashDig files

=head1 SYNOPSIS

B<hashdig-stat.pl> B<-t {db|hd}> B<file> B<[file ...]>

=head1 DESCRIPTION

This utility counts the number of known and unknown hashes in a
HashDig file (either .db or .hd format) and generates a report.
Output is written to stdout and has the following fields: KCount,
UCount, ICount, ACount, and Filename. The K, U, I, and A counts are
short for known, unknown, indeterminate, and all, respectively.

A non-zero ICount indicates that there were one or more parse errors
within a given file. If this happens, make sure the type option
(B<-t>) is correct for the specified file. Also, you may want to
check the file's integrity to ensure that it hasn't been corrupted.

=head1 OPTIONS

=over 4

=item B<-t {db|hd}>

Specifies the type of files that are to be processed. All files
processed in a given invocation must be of the same type. Currently,
the following types are supported: DB and HD. The value for this
option is not case sensitive.

=back

=head1 AUTHORS

Andy Bair and Klayton Monroe

=head1 SEE ALSO

hashdig-dump.pl, hashdig-make.pl, hashdig-harvest.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
