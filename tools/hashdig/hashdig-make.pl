#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-make.pl,v 1.19 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Create or update a HashDig database.
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

  if (!getopts('a:d:Fiqr', \%hOptions))
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
  # The ForceNew flag, '-F', is optional.
  #
  ####################################################################

  my ($sForceNew);

  $sForceNew = (exists($hOptions{'F'})) ? 1 : 0;

  ####################################################################
  #
  # The InsertOnly flag, '-i', is optional.
  #
  ####################################################################

  my ($sInsertOnly);

  $sInsertOnly = (exists($hOptions{'i'})) ? 1 : 0;

  ####################################################################
  #
  # The BeQuiet flag, '-q', is optional.
  #
  ####################################################################

  my ($sBeQuiet);

  $sBeQuiet = (exists($hOptions{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The ReverseFormat flag, '-r', is optional.
  #
  ####################################################################

  my ($sCIndex, $sHIndex, $sRecordRegex, $sReverseFormat);

  $sReverseFormat = (exists($hOptions{'r'})) ? 1 : 0;

  if ($sReverseFormat)
  {
    if ($sHashType =~ /^SHA1$/)
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{40}));
    }
    elsif ($sHashType =~ /^SHA256$/)
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{64}));
    }
    else # MD5
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{32}));
    }
    $sCIndex = 0;
    $sHIndex = 1;
  }
  else
  {
    if ($sHashType =~ /^SHA1$/)
    {
      $sRecordRegex = qq(([0-9a-fA-F]{40})\\|([KU]));
    }
    elsif ($sHashType =~ /^SHA256$/)
    {
      $sRecordRegex = qq(([0-9a-fA-F]{64})\\|([KU]));
    }
    else # MD5
    {
      $sRecordRegex = qq(([0-9a-fA-F]{32})\\|([KU]));
    }
    $sCIndex = 1;
    $sHIndex = 0;
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
  # Tie OnDiskList to the db.
  #
  ####################################################################

  my (%hOnDiskList, $sTieFlags);

  $sTieFlags = ($sForceNew) ? O_RDWR|O_CREAT|O_TRUNC : O_RDWR|O_CREAT;

  if (!tie(%hOnDiskList, "DB_File", $sDBFile, $sTieFlags, 0644, $DB_BTREE))
  {
    print STDERR "$sProgram: File='$sDBFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  my ($sAccepted, $sInserted, $sRejected, $sUpdated) = (0, 0, 0, 0);

  foreach my $sHDFile (@ARGV)
  {
    if (!open(FH, "< $sHDFile"))
    {
      if (!$sBeQuiet)
      {
        print STDERR "$sProgram: File='$sHDFile' Error='$!'\n";
      }
      next;
    }
    if ($sInsertOnly)
    {
      while (my $sRecord = <FH>)
      {
        if (my @aFields = $sRecord =~ /^$sRecordRegex$/o)
        {
          $aFields[$sHIndex] = lc($aFields[$sHIndex]);
          $hOnDiskList{$aFields[$sHIndex]} = $aFields[$sCIndex];
          $sAccepted++;
          $sInserted++;
        }
        else
        {
          if (!$sBeQuiet)
          {
            $sRecord =~ s/[\r\n]+$//;
            print STDERR "$sProgram: File='$sHDFile' Record='$sRecord' Error='Record did not parse properly.'\n";
          }
          $sRejected++;
        }
      }
    }
    else
    {
      while (my $sRecord = <FH>)
      {
        if (my @aFields = $sRecord =~ /^$sRecordRegex$/o)
        {
          $aFields[$sHIndex] = lc($aFields[$sHIndex]);
          if (exists($hOnDiskList{$aFields[$sHIndex]}))
          {
            if ($hOnDiskList{$aFields[$sHIndex]} ne $aFields[$sCIndex])
            {
              $hOnDiskList{$aFields[$sHIndex]} = $aFields[$sCIndex];
              $sUpdated++;
            }
          }
          else
          {
            $sInserted++;
            $hOnDiskList{$aFields[$sHIndex]} = $aFields[$sCIndex];
          }
          $sAccepted++;
        }
        else
        {
          if (!$sBeQuiet)
          {
            $sRecord =~ s/[\r\n]+$//;
            print STDERR "$sProgram: File='$sHDFile' Record='$sRecord' Error='Record did not parse properly.'\n";
          }
          $sRejected++;
        }
      }
    }
    close(FH);
  }

  ####################################################################
  #
  # Print activity report.
  #
  ####################################################################

  my (@aCounts);

  push(@aCounts, "Accepted='$sAccepted'");
  push(@aCounts, "Rejected='$sRejected'");
  push(@aCounts, "Inserted='$sInserted'");
  push(@aCounts, ($sInsertOnly) ? "Updated='NA'" : "Updated='$sUpdated'");
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
  print STDERR "Usage: $sProgram [-Fiqr] [-a hash-type] -d db file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-make.pl - Create or update a HashDig database

=head1 SYNOPSIS

B<hashdig-make.pl> B<[-Fiqr]> B<[-a hash-type]> B<-d db> B<file [file ...]>

=head1 DESCRIPTION

This utility reads one or more HashDig files (see hashdig-harvest(1))
and creates or updates the specified HashDig database. A HashDig
database is an ordered list of MD5 hashes -- each of which is tagged
as known (K) or unknown (U). HashDig databases are implemented as
BTrees and constructed using Perl's DB_File module. Enumerating
these databases yields the following format:

    hash|category

HashDig stores and manipulates MD5 hashes as lowercase, hexadecimal
strings. The primary rule of engagement is that imported hash/category
pairs trump existing pairs in the database. This is true unless the
pairs are identical. In that case the existing pairs are not modified.
If the B<-i> option is specified, imported pairs always trump existing
pairs. HashDig files are processed in command-line order. Typically,
HashDig files that are sorted in hash order yield much faster load
times.

=head1 OPTIONS

=over 4

=item B<-a hash-type>

Specifies the type of hashes that are to be processed. Currently, the
following hash types (or algorithms) are supported: 'MD5', 'SHA1', and
'SHA256'. The default hash type is that specified by the HASH_TYPE
environment variable or 'MD5' if HASH_TYPE is not set. The value for
this option is not case sensitive.

=item B<-d db>

Specifies the name of the database to create or update.

=item B<-F>

Force the specified database to be truncated on open.

=item B<-i>

Always insert. If a hash already exists, its value is overwritten.
This option improves performance when the database is new or the
B<-F> option has been specified. Enabling this option, however,
disables update tracking.

=item B<-q>

Don't report errors (i.e., be quiet) while processing files.

=item B<-r>

Accept records in the reverse HashDig format (i.e., category|hash).

=back

=head1 CAVEATS

Databases created from the same input may yield different, but
equivalent, DB files. Further, these DB files may not be portable
across different platforms or operating systems. Therefore, the
recommended method for exchanging or verifying a HashDig database
is to dump it to a HashDig file (see hashdig-dump(1)) and operate
on that instead.

Take care to avoid mixing DB files that are based on different hash
types. This can be easy to do if you're not careful.

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-bash(1), hashdig-dump(1), hashdig-harvest(1), hashdig-harvest-sunsolve(1), hashdig-weed(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
