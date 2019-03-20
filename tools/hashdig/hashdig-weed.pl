#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-weed.pl,v 1.3 2003/03/26 20:47:31 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Delete hashes from a HashDig database.
#
######################################################################

use strict;
use DB_File;
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

  $program = "hashdig-weed.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('d:f:q', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # A database, '-d', is required.
  #
  ####################################################################

  my ($dbFile);

  if (!exists($options{'d'}))
  {
    Usage($program);
  }
  else
  {
    $dbFile = $options{'d'};
    if (!defined($dbFile) || length($dbFile) < 1)
    {
      Usage($program);
    }
  }

  ####################################################################
  #
  # A filename, '-f', is required, and can be '-' or a regular file.
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
  # The beQuiet flag, '-q', is optional. Default value is 0.
  #
  ####################################################################

  my ($beQuiet);

  $beQuiet = (exists($options{'q'})) ? 1 : 0;

  ####################################################################
  #
  # If there's any arguments left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($program);
  }

  ####################################################################
  #
  # Tie onDiskList to the db.
  #
  ####################################################################

  my (%onDiskList);

  if (!tie(%onDiskList, "DB_File", $dbFile, O_RDWR, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Read input and weed db.
  #
  ####################################################################

  my ($accepted, $deleted, $rejected) = (0, 0, 0);

  while (my $record = <$fileHandle>)
  {
    if (my ($hash) = $record =~ /^([0-9a-fA-F]{32})$/)
    {
      $hash = lc($hash);
      if (exists($onDiskList{$hash}))
      {
        delete($onDiskList{$hash});
        $deleted++;
      }
      $accepted++;
    }
    else
    {
      if (!$beQuiet)
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$program: File='$filename' Record='$record' Error='Record did not parse properly.'\n";
      }
      $rejected++;
    }
  }

  ####################################################################
  #
  # Print activity report.
  #
  ####################################################################

  my (@counts);

  push(@counts, "Accepted='$accepted'");
  push(@counts, "Rejected='$rejected'");
  push(@counts, "Deleted='$deleted'");
  print join(' ', @counts), "\n";

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  untie(%onDiskList);

  1;


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($program) = @_;
  print STDERR "\n";
  print STDERR "Usage: $program [-q] -d db -f {file|-}\n";
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

Don't report errors (i.e. be quiet) while processing input.

=back

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-dump.pl, hashdig-make.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
