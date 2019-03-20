#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-dump.pl,v 1.2 2003/03/24 13:25:00 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Enumerate a HashDig database.
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

  $program = "hashdig-dump.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('c:hr', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The category flag, '-c', is optional. Default value is "A".
  #
  ####################################################################

  my ($category);

  $category = (exists($options{'c'})) ? uc($options{'c'}) : "A";

  if ($category !~ /^[AKU]$/)
  {
    print STDERR "$program: Category='$category' Error='Invalid category.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The hashOnly flag, '-h', is optional. Default value is 0.
  #
  ####################################################################

  my ($hashOnly);

  $hashOnly = (exists($options{'h'})) ? 1 : 0;

  ####################################################################
  #
  # The reverseFormat flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($cIndex, $hIndex, $reverseFormat);

  $reverseFormat = (exists($options{'r'})) ? 1 : 0;

  if ($reverseFormat)
  {
    $cIndex = 0;
    $hIndex = 1;
  }
  else
  {
    $cIndex = 1;
    $hIndex = 0;
  }

  ####################################################################
  #
  # If there isn't one argument left, it's an error.
  #
  ####################################################################

  my ($dbFile);

  if (scalar(@ARGV) != 1)
  {
    Usage($program);
  }
  $dbFile = shift;

  ####################################################################
  #
  # Tie onDiskList to the db.
  #
  ####################################################################

  my (%onDiskList);

  if (!tie(%onDiskList, "DB_File", $dbFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Enumerate db. Write output to stdout.
  #
  ####################################################################

  if ($category =~ /^[KU]$/)
  {
    if ($hashOnly)
    {
      while (my (@pair) = each(%onDiskList))
      {
        print "$pair[0]\n" unless ($pair[1] ne $category);
      }
    }
    else
    {
      while (my (@pair) = each(%onDiskList))
      {
        print "$pair[$hIndex]|$pair[$cIndex]\n" unless ($pair[1] ne $category);
      }
    }
  }
  else
  {
    if ($hashOnly)
    {
      while (my (@pair) = each(%onDiskList))
      {
        print "$pair[0]\n";
      }
    }
    else
    {
      while (my (@pair) = each(%onDiskList))
      {
        print "$pair[$hIndex]|$pair[$cIndex]\n";
      }
    }
  }

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
  print STDERR "Usage: $program [-c {A|K|U}] [-h|-r] db\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-dump.pl - Enumerate a HashDig database

=head1 SYNOPSIS

B<hashdig-dump.pl> B<[-c {A|K|U}]> B<[-h|-r]> B<db>

=head1 DESCRIPTION

This utility enumerates a HashDig database that has been created
with hashdig-make.pl. Output is written to stdout and has the the
following format:

    hash|category

=head1 OPTIONS

=over 4

=item B<-c category>

Specifies the hash category, {A|K|U}, to enumerate. Currently, the
following categories are supported: all (A), known (K), and unknown
(U). The default category is all.

=item B<-h>

Output hashes only. By default, hash and category information is
written to stdout. This option is useful when feeding hashes to
hashdig-resolve-sunsolve.pl.

=item B<-r>

Output hash and category information in the reverse HashDig format
(i.e. category|hash). This option is silently ignored if B<-h> has
been specified.

=back

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-dump.pl, hashdig-make.pl, hashdig-resolve-sunsolve.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
