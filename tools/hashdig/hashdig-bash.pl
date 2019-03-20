#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-bash.pl,v 1.6 2003/03/26 20:47:27 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Bash one HashDig database against another.
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

  $program = "hashdig-bash.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('i:r:s:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The iterator flag, '-i', is optional. Default value is undef.
  #
  ####################################################################

  my ($iterator);

  $iterator = (exists($options{'i'})) ? uc($options{'i'}) : undef;

  if (defined($iterator) && $iterator !~ /^[RS]$/)
  {
    print STDERR "$program: Iterator='$iterator' Error='Invalid iterator.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A reference, '-r', db is required.
  #
  ####################################################################

  my ($dbRFile);

  if (!exists($options{'r'}))
  {
    Usage($program);
  }
  else
  {
    $dbRFile = $options{'r'}; # The "R" is for Reference.
    if (!defined($dbRFile) || length($dbRFile) < 1)
    {
      Usage($program);
    }
  }

  ####################################################################
  #
  # A subject, '-s', db is required.
  #
  ####################################################################

  my ($dbSFile, $dbTFile);

  if (!exists($options{'s'}))
  {
    Usage($program);
  }
  else
  {
    $dbSFile = $options{'s'}; # The "S" is for Subject.
    $dbTFile = $options{'s'}; # The "T" is for Tag.
    if (!defined($dbSFile) || length($dbSFile) < 1)
    {
      Usage($program);
    }
  }

  if ($dbSFile eq $dbRFile)
  {
    print STDERR "$program: Reference='$dbRFile' Subject='$dbSFile' Error='Reference and subject databases must not be the same.'\n";
    exit(2);
  }

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
  # Tie onDiskRList to the reference db.
  #
  ####################################################################

  my (%onDiskRList);

  if (!tie(%onDiskRList, "DB_File", $dbRFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbRFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Tie onDiskSList to the subject db.
  #
  ####################################################################

  my (%onDiskSList);

  if (!tie(%onDiskSList, "DB_File", $dbSFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbSFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Tie onDiskTList to the subject db.
  #
  ####################################################################

  my (%onDiskTList);

  if (!tie(%onDiskTList, "DB_File", $dbTFile, O_RDWR, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbTFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Figure out which db will be the iterator.
  #
  ####################################################################

  my ($onDiskRSize, $onDiskSSize, $useSubject);

  if (!defined($iterator))
  {
    $onDiskRSize = -s $dbRFile;
    $onDiskSSize = -s $dbSFile;
    if (!defined($onDiskRSize) || !defined($onDiskSSize))
    {
      $iterator = "S";
    }
    else
    {
      $iterator = ($onDiskRSize >= $onDiskSSize) ? "S" : "R";
    }
  }

  ####################################################################
  #
  # Start slinging hashes. Reference values trump subject values.
  #
  ####################################################################

  my ($bashed, $tagged) = (0, 0);

  if ($iterator eq "S")
  {
    while (my ($hash, $category) = each(%onDiskSList))
    {
      if (exists($onDiskRList{$hash}))
      {
        $onDiskTList{$hash} = $onDiskRList{$hash};
        $tagged++;
      }
      $bashed++;
    }
  }
  else
  {
    while (my ($hash, $category) = each(%onDiskRList))
    {
      if (exists($onDiskSList{$hash}))
      {
        $onDiskTList{$hash} = $category;
        $tagged++;
      }
      $bashed++;
    }
  }

  ####################################################################
  #
  # Print activity report.
  #
  ####################################################################

  my (@counts);

  push(@counts, "Bashed='$bashed'");
  push(@counts, "Tagged='$tagged'");
  print join(' ', @counts), "\n";

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  untie(%onDiskRList);
  untie(%onDiskSList);
  untie(%onDiskTList);

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
  print STDERR "Usage: $program [-i {R|S}] -r db -s db\n";
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

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-dump.pl, hashdig-make.pl, hashdig-weed.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
