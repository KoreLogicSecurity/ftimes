#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-make.pl,v 1.5 2003/03/26 20:47:30 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Create or update a HashDig database.
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

  $program = "hashdig-make.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('d:Fiqr', \%options))
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
  # The forceNew flag, '-F', is optional. Default value is 0.
  #
  ####################################################################

  my ($forceNew);

  $forceNew = (exists($options{'F'})) ? 1 : 0;

  ####################################################################
  #
  # The insertOnly flag, '-i', is optional. Default value is 0.
  #
  ####################################################################

  my ($insertOnly);

  $insertOnly = (exists($options{'i'})) ? 1 : 0;

  ####################################################################
  #
  # The beQuiet flag, '-q', is optional. Default value is 0.
  #
  ####################################################################

  my ($beQuiet);

  $beQuiet = (exists($options{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The reverseFormat flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($cIndex, $hIndex, $recordRegex, $reverseFormat);

  $reverseFormat = (exists($options{'r'})) ? 1 : 0;

  if ($reverseFormat)
  {
    $recordRegex = qq(^([KU])\\|([0-9a-fA-F]{32})\$);
    $cIndex = 0;
    $hIndex = 1;
  }
  else
  {
    $recordRegex = qq(^([0-9a-fA-F]{32})\\|([KU])\$);
    $cIndex = 1;
    $hIndex = 0;
  }

  ####################################################################
  #
  # If there isn't at least one argument left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) < 1)
  {
    Usage($program);
  }

  ####################################################################
  #
  # Tie onDiskList to the db.
  #
  ####################################################################

  my (%onDiskList, $tieFlags);

  $tieFlags = ($forceNew) ? O_RDWR|O_CREAT|O_TRUNC : O_RDWR|O_CREAT;

  if (!tie(%onDiskList, "DB_File", $dbFile, $tieFlags, 0644, $DB_BTREE))
  {
    print STDERR "$program: File='$dbFile' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  my ($accepted, $inserted, $rejected, $updated) = (0, 0, 0, 0);

  foreach my $hdFile (@ARGV)
  {
    if (!open(FH, "<$hdFile"))
    {
      if (!$beQuiet)
      {
        print STDERR "$program: File='$hdFile' Error='$!'\n";
      }
      next;
    }
    if ($insertOnly)
    {
      while (my $record = <FH>)
      {
        if (my @fields = $record =~ /$recordRegex/o)
        {
          $fields[$hIndex] = lc($fields[$hIndex]);
          $onDiskList{$fields[$hIndex]} = $fields[$cIndex];
          $accepted++;
          $inserted++;
        }
        else
        {
          if (!$beQuiet)
          {
            $record =~ s/[\r\n]+$//;
            print STDERR "$program: File='$hdFile' Record='$record' Error='Record did not parse properly.'\n";
          }
          $rejected++;
        }
      }
    }
    else
    {
      while (my $record = <FH>)
      {
        if (my @fields = $record =~ /$recordRegex/o)
        {
          $fields[$hIndex] = lc($fields[$hIndex]);
          if (exists($onDiskList{$fields[$hIndex]}))
          {
            if ($onDiskList{$fields[$hIndex]} ne $fields[$cIndex])
            {
              $onDiskList{$fields[$hIndex]} = $fields[$cIndex];
              $updated++;
            }
          }
          else
          {
            $inserted++;
            $onDiskList{$fields[$hIndex]} = $fields[$cIndex];
          }
          $accepted++;
        }
        else
        {
          if (!$beQuiet)
          {
            $record =~ s/[\r\n]+$//;
            print STDERR "$program: File='$hdFile' Record='$record' Error='Record did not parse properly.'\n";
          }
          $rejected++;
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

  my (@counts);

  push(@counts, "Accepted='$accepted'");
  push(@counts, "Rejected='$rejected'");
  push(@counts, "Inserted='$inserted'");
  push(@counts, ($insertOnly) ? "Updated='NA'" : "Updated='$updated'");
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
  print STDERR "Usage: $program [-F] [-i] [-q] [-r] -d db file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-make.pl - Create or update a HashDig database

=head1 SYNOPSIS

B<hashdig-make.pl> B<[-F]> B<[-i]> B<[-q]> B<[-r]> B<-d db> B<file [file ...]>

=head1 DESCRIPTION

This utility reads one or more HashDig files (see hashdig-harvest.pl)
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

Don't report errors (i.e. be quiet) while processing files.

=item B<-r>

Accept records in the reverse HashDig format (i.e. category|hash).

=back

=head1 CAVEATS

Databases created from the same input may yield different, but
equivalent, db files.  Further, these db files may not be portable
across different platforms or operating systems. Therefore, the
recommended method for exchanging or verifying a HashDig database
is to dump it to a HashDig file (see hashdig-dump.pl) and operate
on that instead.

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-bash.pl, hashdig-dump.pl, hashdig-harvest.pl, hashdig-harvest-sunsolve.pl, hashdig-weed.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
