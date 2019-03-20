#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-find.pl,v 1.3 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2006-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Find one or more hashes in a HashDig database
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

  if (!getopts('a:c:d:qr', \%hOptions))
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
  # The Category flag, '-c', is optional.
  #
  ####################################################################

  my ($sCategory, $sCategoryRegex);

  $sCategory = (exists($hOptions{'c'})) ? uc($hOptions{'c'}) : "A";

  if ($sCategory !~ /^[AKU]$/)
  {
    print STDERR "$sProgram: Category='$sCategory' Error='Invalid category.'\n";
    exit(2);
  }

  $sCategoryRegex = ($sCategory eq 'A') ? "[KU]" : $sCategory;

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
  # The BeQuiet flag, '-q', is optional.
  #
  ####################################################################

  my ($sBeQuiet);

  $sBeQuiet = (exists($hOptions{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The ReverseArguments flag, '-r', is optional.
  #
  ####################################################################

  my ($sReverseArguments);

  $sReverseArguments = (exists($hOptions{'r'})) ? 1 : 0;

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
  # Do the necessary prep work.
  #
  ####################################################################

  my (%hOnDiskList, $sFile, $sHash, $sHashRegex);

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

  if ($sReverseArguments)
  {
    $sHash = $sDBFile;
    if ($sHash !~ /^$sHashRegex$/)
    {
      print STDERR "$sProgram: Hash='$sHash' Error='Hash did not parse properly.'\n";
      exit(2);
    }
  }
  else
  {
    $sFile = $sDBFile;
    if (!tie(%hOnDiskList, "DB_File", $sFile, O_RDONLY, 0644, $DB_BTREE))
    {
      print STDERR "$sProgram: File='$sFile' Error='$!'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Iterate over input list.
  #
  ####################################################################

  foreach my $sItem (@ARGV)
  {
    if ($sReverseArguments)
    {
      my $sFile = $sItem;
      if (tie(%hOnDiskList, "DB_File", $sFile, O_RDONLY, 0644, $DB_BTREE))
      {
        if (exists($hOnDiskList{$sHash}))
        {
          print join("|", $sHash, $hOnDiskList{$sHash}, $sFile), "\n" if ($hOnDiskList{$sHash} =~ /^$sCategoryRegex$/);
        }
        untie(%hOnDiskList);
      }
      else
      {
        if (!$sBeQuiet)
        {
          print STDERR "$sProgram: File='$sItem' Error='$!'\n";
        }
      }
    }
    else
    {
      my $sHash = $sItem;
      if ($sHash =~ /^$sHashRegex$/)
      {
        $sHash = lc($1);
        if (exists($hOnDiskList{$sHash}))
        {
          print join("|", $sHash, $hOnDiskList{$sHash}, $sFile), "\n" if ($hOnDiskList{$sHash} =~ /^$sCategoryRegex$/);
        }
      }
      else
      {
        if (!$sBeQuiet)
        {
          print STDERR "$sProgram: Hash='$sItem' Error='Hash did not parse properly.'\n";
        }
      }
    }
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  if ($sReverseArguments == 0)
  {
    untie(%hOnDiskList);
  }

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
  print STDERR "Usage: $sProgram [-qr] [-a hash-type] [-c {A|K|U}] -d db hash [hash ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-find.pl - Find one or more hashes in a HashDig database

=head1 SYNOPSIS

B<hashdig-find.pl> B<[-qr]> B<[-a hash-type]> B<[-c {A|K|U}]> B<-d db> B<hash [hash ...]>

=head1 DESCRIPTION

This utility searches for a list of hashes from a HashDig database
that has been created with hashdig-make(1). MD5 hashes must match the
following regular expression:

    ^[0-9a-fA-F]{32}$

SHA1 hashes, must match the following regular expression:

    ^[0-9a-fA-F]{40}$

SHA256 hashes, must match the following regular expression:

    ^[0-9a-fA-F]{64}$

Input that does not match the required expression will cause the
program to generate an error message, which will be conditionally
printed based on whether or not the B<-q> flag has been set.

The output produced by this utility has the following format:

    hash|category|db

=head1 OPTIONS

=over 4

=item B<-a hash-type>

Specifies the type of hashes that are to be sought. Currently, the
following hash types (or algorithms) are supported: 'MD5', 'SHA1', and
'SHA256'. The default hash type is that specified by the HASH_TYPE
environment variable or 'MD5' if HASH_TYPE is not set. The value for
this option is not case sensitive.

=item B<-c category>

Specifies the hash category, {A|K|U}, to enumerate. Currently, the
following categories are supported: all (A), known (K), and unknown
(U). The default category is all.

=item B<-d db>

Specifies the name of the database to search.

=item B<-q>

Don't report errors (i.e., be quiet) while processing input.

=item B<-r>

Reverse the meaning of the B<db> and B<hash> arguments. This option
allows you to search for a single hash in one or more HashDig
databases.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-dump(1), hashdig-make(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
