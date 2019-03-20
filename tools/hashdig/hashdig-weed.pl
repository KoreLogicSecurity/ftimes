#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-weed.pl,v 1.13 2006/04/07 22:15:12 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2006 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Delete hashes from a HashDig database.
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

  if (!getopts('d:f:q', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # A database, '-d', is required.
  #
  ####################################################################

  my ($sDBFile);

  if (!exists($hOptions{'d'}) || !defined($hOptions{'d'}) || length($hOptions{'d'}) < 1)
  {
    Usage($sProgram);
  }
  $sDBFile = $hOptions{'d'};

  ####################################################################
  #
  # A filename, '-f', is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($sFileHandle, $sFilename);

  if (!exists($hOptions{'f'}) || !defined($hOptions{'f'}) || length($hOptions{'f'}) < 1)
  {
    Usage($sProgram);
  }
  $sFilename = $hOptions{'f'};

  if ($sFilename eq '-')
  {
    $sFileHandle = \*STDIN;
  }
  else
  {
    if (!-f $sFilename)
    {
      print STDERR "$sProgram: File='$sFilename' Error='File must exist and be regular.'\n";
      exit(2);
    }
    if (!open(FH, "< $sFilename"))
    {
      print STDERR "$sProgram: File='$sFilename' Error='$!'\n";
      exit(2);
    }
    $sFileHandle = \*FH;
  }

  ####################################################################
  #
  # The BeQuiet flag, '-q', is optional.
  #
  ####################################################################

  my ($sBeQuiet);

  $sBeQuiet = (exists($hOptions{'q'})) ? 1 : 0;

  ####################################################################
  #
  # If there's any arguments left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # Tie OnDiskList to the db.
  #
  ####################################################################

  my (%hOnDiskList);

  if (!tie(%hOnDiskList, "DB_File", $sDBFile, O_RDWR, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Read input and weed db.
  #
  ####################################################################

  my ($sAccepted, $sDeleted, $sRejected) = (0, 0, 0);

  while (my $sRecord = <$sFileHandle>)
  {
    if (my ($sHash) = $sRecord =~ /^([0-9a-fA-F]{32})$/)
    {
      $sHash = lc($sHash);
      if (exists($hOnDiskList{$sHash}))
      {
        delete($hOnDiskList{$sHash});
        $sDeleted++;
      }
      $sAccepted++;
    }
    else
    {
      if (!$sBeQuiet)
      {
        $sRecord =~ s/[\r\n]+$//;
        print STDERR "$sProgram: File='$sFilename' Record='$sRecord' Error='Record did not parse properly.'\n";
      }
      $sRejected++;
    }
  }

  ####################################################################
  #
  # Print activity report.
  #
  ####################################################################

  my (@aCounts);

  push(@aCounts, "Accepted='$sAccepted'");
  push(@aCounts, "Rejected='$sRejected'");
  push(@aCounts, "Deleted='$sDeleted'");
  print join(' ', @aCounts), "\n";

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  untie(%hOnDiskList);

  1;


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-q] -d db -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-weed.pl - Delete hashes from a HashDig database

=head1 SYNOPSIS

B<hashdig-weed.pl> B<[-q]> B<-d db> B<-f {file|-}>

=head1 DESCRIPTION

This utility deletes specified hashes from a HashDig database that
has been created with hashdig-make.pl. Input is expected to be plain
text with one hash per line. Each line must match the following
regular expression:

    ^[0-9a-fA-F]{32}$

Input that does not match this expression will cause the program
to generate an error message.

=head1 OPTIONS

=over 4

=item B<-d db>

Specifies the name of the database to weed.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-q>

Don't report errors (i.e., be quiet) while processing input.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-dump.pl, hashdig-make.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
