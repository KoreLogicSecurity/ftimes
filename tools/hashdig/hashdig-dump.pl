#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-dump.pl,v 1.14 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Enumerate a HashDig database.
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

  if (!getopts('c:hr', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The Category flag, '-c', is optional.
  #
  ####################################################################

  my ($sCategory);

  $sCategory = (exists($hOptions{'c'})) ? uc($hOptions{'c'}) : "A";

  if ($sCategory !~ /^[AKU]$/)
  {
    print STDERR "$sProgram: Category='$sCategory' Error='Invalid category.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The HashOnly flag, '-h', is optional.
  #
  ####################################################################

  my ($sHashOnly);

  $sHashOnly = (exists($hOptions{'h'})) ? 1 : 0;

  ####################################################################
  #
  # The ReverseFormat flag, '-r', is optional.
  #
  ####################################################################

  my ($sCIndex, $sHIndex, $sReverseFormat);

  $sReverseFormat = (exists($hOptions{'r'})) ? 1 : 0;

  if ($sReverseFormat)
  {
    $sCIndex = 0;
    $sHIndex = 1;
  }
  else
  {
    $sCIndex = 1;
    $sHIndex = 0;
  }

  ####################################################################
  #
  # If there isn't one argument left, it's an error.
  #
  ####################################################################

  my ($sDBFile);

  if (scalar(@ARGV) != 1)
  {
    Usage($sProgram);
  }
  $sDBFile = shift;

  ####################################################################
  #
  # Tie OnDiskList to the db.
  #
  ####################################################################

  my (%hOnDiskList);

  if (!tie(%hOnDiskList, "DB_File", $sDBFile, O_RDONLY, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Enumerate db. Write output to stdout.
  #
  ####################################################################

  if ($sCategory =~ /^[KU]$/)
  {
    if ($sHashOnly)
    {
      while (my (@aPair) = each(%hOnDiskList))
      {
        print "$aPair[0]\n" unless ($aPair[1] ne $sCategory);
      }
    }
    else
    {
      while (my (@aPair) = each(%hOnDiskList))
      {
        print "$aPair[$sHIndex]|$aPair[$sCIndex]\n" unless ($aPair[1] ne $sCategory);
      }
    }
  }
  else
  {
    if ($sHashOnly)
    {
      while (my (@aPair) = each(%hOnDiskList))
      {
        print "$aPair[0]\n";
      }
    }
    else
    {
      while (my (@aPair) = each(%hOnDiskList))
      {
        print "$aPair[$sHIndex]|$aPair[$sCIndex]\n";
      }
    }
  }

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
  print STDERR "Usage: $sProgram [-c {A|K|U}] [-h|-r] db\n";
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
with hashdig-make(1). Output is written to stdout and has the
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
hashdig-resolve-sunsolve(1).

=item B<-r>

Output hash and category information in the reverse HashDig format
(i.e., category|hash). This option is silently ignored if B<-h> has
been specified.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-dump(1), hashdig-make(1), hashdig-resolve-sunsolve(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
