#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-weed.pl,v 1.17 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
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

  if (!getopts('a:d:f:q', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # A HashType, '-a', is optional. The 'a' is short for algorithm.
  #
  ####################################################################

  my ($sHashType);

  $sHashType = (exists($hOptions{'a'})) ? uc($hOptions{'a'}) : (defined($ENV{'HASH_TYPE'})) ? uc($ENV{'HASH_TYPE'}) : "MD5";

  if ($sHashType !~ /^(MD5|SHA1|SHA256)$/)
  {
    print STDERR "$sProgram: HashType='$sHashType' Error='Invalid hash type.'\n";
    exit(2);
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

  my ($sAccepted, $sDeleted, $sHashRegex, $sRejected) = (0, 0, "", 0);

  if ($sHashType =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  elsif ($sHashType =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <$sFileHandle>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash) = $sRecord =~ /^$sHashRegex$/)
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
  print STDERR "Usage: $sProgram [-q] [-a hash-type] -d db -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-weed.pl - Delete hashes from a HashDig database

=head1 SYNOPSIS

B<hashdig-weed.pl> B<[-q]> B<[-a hash-type]> B<-d db> B<-f {file|-}>

=head1 DESCRIPTION

This utility deletes specified hashes from a HashDig database that has
been created with hashdig-make(1). Input is expected to be plain text
with one hash per line. For MD5 hash DBs, each line must match the
following regular expression:

    ^[0-9a-fA-F]{32}$

For SHA1 hash DBs, each line must match the following regular
expression:

    ^[0-9a-fA-F]{40}$

For SHA256 hash DBs, each line must match the following regular
expression:

    ^[0-9a-fA-F]{64}$

Input that does not match the required expression will cause the
program to generate an error message.

=head1 OPTIONS

=over 4

=item B<-a hash-type>

Specifies the type of hashes that are to be deleted. Currently, the
following hash types (or algorithms) are supported: 'MD5', 'SHA1', and
'SHA256'. The default hash type is that specified by the HASH_TYPE
environment variable or 'MD5' if HASH_TYPE is not set. The value for
this option is not case sensitive.

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

hashdig-dump(1), hashdig-make(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
