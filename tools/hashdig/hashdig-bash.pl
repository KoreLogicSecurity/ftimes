#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-bash.pl,v 1.16 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Bash one HashDig database against another.
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

  if (!getopts('i:r:s:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The Iterator flag, '-i', is optional.
  #
  ####################################################################

  my ($sIterator);

  $sIterator = (exists($hOptions{'i'})) ? uc($hOptions{'i'}) : undef;

  if (defined($sIterator) && $sIterator !~ /^[RS]$/)
  {
    print STDERR "$sProgram: Iterator='$sIterator' Error='Invalid iterator.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A reference, '-r', db is required.
  #
  ####################################################################

  my ($sDBRFile);

  if (!exists($hOptions{'r'}) || !defined($hOptions{'r'}) || length($hOptions{'r'}) < 1)
  {
    Usage($sProgram);
  }
  $sDBRFile = $hOptions{'r'}; # The "R" is for Reference.

  ####################################################################
  #
  # A subject, '-s', db is required.
  #
  ####################################################################

  my ($sDBSFile, $sDBTFile);

  if (!exists($hOptions{'s'}) || !defined($hOptions{'s'}) || length($hOptions{'s'}) < 1)
  {
    Usage($sProgram);
  }
  $sDBSFile = $hOptions{'s'}; # The "S" is for Subject.
  $sDBTFile = $hOptions{'s'}; # The "T" is for Tag.

  if ($sDBSFile eq $sDBRFile)
  {
    print STDERR "$sProgram: Reference='$sDBRFile' Subject='$sDBSFile' Error='Reference and subject databases must not be the same.'\n";
    exit(2);
  }

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
  # Tie OnDiskRList to the reference db.
  #
  ####################################################################

  my (%hOnDiskRList);

  if (!tie(%hOnDiskRList, "DB_File", $sDBRFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBRFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Tie OnDiskSList to the subject db.
  #
  ####################################################################

  my (%hOnDiskSList);

  if (!tie(%hOnDiskSList, "DB_File", $sDBSFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBSFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Tie OnDiskTList to the subject db.
  #
  ####################################################################

  my (%hOnDiskTList);

  if (!tie(%hOnDiskTList, "DB_File", $sDBTFile, O_RDWR, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBTFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Figure out which db will be the iterator.
  #
  ####################################################################

  my ($sOnDiskRSize, $sOnDiskSSize, $sUseSubject);

  if (!defined($sIterator))
  {
    $sOnDiskRSize = -s $sDBRFile;
    $sOnDiskSSize = -s $sDBSFile;
    if (!defined($sOnDiskRSize) || !defined($sOnDiskSSize))
    {
      $sIterator = "S";
    }
    else
    {
      $sIterator = ($sOnDiskRSize >= $sOnDiskSSize) ? "S" : "R";
    }
  }

  ####################################################################
  #
  # Start slinging hashes. Reference values trump subject values.
  #
  ####################################################################

  my ($sBashed, $sTagged) = (0, 0);

  if ($sIterator eq "S")
  {
    while (my ($sHash, $sCategory) = each(%hOnDiskSList))
    {
      if (exists($hOnDiskRList{$sHash}))
      {
        $hOnDiskTList{$sHash} = $hOnDiskRList{$sHash};
        $sTagged++;
      }
      $sBashed++;
    }
  }
  else
  {
    while (my ($sHash, $sCategory) = each(%hOnDiskRList))
    {
      if (exists($hOnDiskSList{$sHash}))
      {
        $hOnDiskTList{$sHash} = $sCategory;
        $sTagged++;
      }
      $sBashed++;
    }
  }

  ####################################################################
  #
  # Print activity report.
  #
  ####################################################################

  my (@aCounts);

  push(@aCounts, "Bashed='$sBashed'");
  push(@aCounts, "Tagged='$sTagged'");
  print join(' ', @aCounts), "\n";

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  untie(%hOnDiskRList);
  untie(%hOnDiskSList);
  untie(%hOnDiskTList);

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
  print STDERR "Usage: $sProgram [-i {R|S}] -r db -s db\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-bash.pl - Bash one HashDig database against another

=head1 SYNOPSIS

B<hashdig-bash.pl> B<[-i {R|S}]> B<-r db> B<-s db>

=head1 DESCRIPTION

This utility compares hashes in the subject database to those in
the reference database. The result is an updated subject database
where known hashes have been tagged as such. The primary rule of
engagement is that known hashes trump unknown hashes. The reference
database is not altered during this process.

=head1 OPTIONS

=over 4

=item B<-i {R|S}>

Specifies the database, reference (R) or subject (S), that will
serve as the iterator during analysis. By default, the smallest
database is used -- a decision that is based on file size as opposed
to the number of actual records. Databases that have been weeded
can have a deceivingly large size. The value for this option is not
case sensitive.

=item B<-r db>

Specifies the name of the reference database.

=item B<-s db>

Specifies the name of the subject database.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-dump(1), hashdig-make(1), hashdig-weed(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
