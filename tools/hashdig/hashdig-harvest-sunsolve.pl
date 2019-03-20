#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-harvest-sunsolve.pl,v 1.16 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Harvest hashes from a directory of sunsolve output.
#
######################################################################

use strict;
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

  if (!getopts('c:o:qs:T:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The Category flag, '-c', is optional.
  #
  ####################################################################

  my ($sCategory);

  $sCategory = (exists($hOptions{'c'})) ? uc($hOptions{'c'}) : "K";

  if ($sCategory !~ /^[AKU]$/)
  {
    print STDERR "$sProgram: Category='$sCategory' Error='Invalid category.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A filename, '-o', is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($sFilename);

  $sFilename = (exists($hOptions{'o'})) ? $hOptions{'o'} : undef;

  if (!defined($sFilename) || length($sFilename) < 1)
  {
    Usage($sProgram);
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
  # The Sort flag, '-s', is optional.
  #
  ####################################################################

  my ($sSort);

  $sSort = (exists($hOptions{'s'})) ? $hOptions{'s'} : "sort";

  ####################################################################
  #
  # The TmpDir flag, '-T', is optional.
  # or if that is not defined, fall back to "/tmp".
  #
  ####################################################################

  my ($sTmpDir);

  $sTmpDir = (exists($hOptions{'T'})) ? $hOptions{'T'} : (defined($ENV{'TMPDIR'})) ? $ENV{'TMPDIR'} : "/tmp";

  ####################################################################
  #
  # If there isn't one argument left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) != 1)
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # Unlink output file, if necessary.
  #
  ####################################################################

  if ($sFilename ne "-" && -f $sFilename && !unlink($sFilename))
  {
    print STDERR "$sProgram: File='$sFilename' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Open and read the input directory. Create a list of files.
  #
  ####################################################################

  my ($sDirectory, @aFiles);

  $sDirectory = shift;
  $sDirectory =~ s/\/*$//;
  if (!opendir(DIR, $sDirectory))
  {
    print STDERR "$sProgram: Directory='$sDirectory' Error='$!'\n";
    exit(2);
  }
  @aFiles = sort(grep(/^$sDirectory\/hashdig-sunsolve.\d+$/, map("$sDirectory/$_", readdir(DIR))));
  closedir(DIR);

  ####################################################################
  #
  # Open a pipe to the sort utility.
  #
  ####################################################################

  my ($sCommand);

  $sCommand = "$sSort -u -T $sTmpDir";
  if ($sFilename ne "-")
  {
    $sCommand .= " -o $sFilename";
  }
  if (!open(SH, "| $sCommand"))
  {
    print STDERR "$sProgram: Command='$sCommand' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Iterate over input files. Extract matches and pipe them to sort.
  #
  ####################################################################

  foreach my $sInputFile (@aFiles)
  {
    if (!open(FH, "< $sInputFile"))
    {
      if (!$sBeQuiet)
      {
        print STDERR "$sProgram: File='$sInputFile' Error='$!'\n";
      }
      next;
    }
    while (my $sRecord = <FH>)
    {
      if (my ($sHash, $sMatches) = ($sRecord =~ /^.*<TT>([0-9a-fA-F]{32})<\/TT>\s+-\s+<TT><\/TT>\s+-\s+(\d+)\smatch.*$/o))
      {
        my $sValue = ($sMatches) ? "K" : "U";
        if ($sCategory eq "A" || ($sValue eq $sCategory))
        {
          print SH lc($sHash), "|", $sValue, "\n";
        }
      }
    }
    close(FH);
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  close(SH);

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
  print STDERR "Usage: $sProgram [-c {A|K|U}] [-q] [-s file] [-T dir] -o {file|-} dir\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-harvest-sunsolve.pl - Harvest hashes from a directory of sunsolve output

=head1 SYNOPSIS

B<hashdig-harvest-sunsolve.pl> B<[-c {A|K|U}]> B<[-q]> B<[-s file]> B<[-T dir]> B<-o {file|-}> B<dir>

=head1 DESCRIPTION

This utility extracts MD5 hashes from a directory of files created
by hashdig-resolve-sunsolve(1), filters them by category (see B<-c>),
and writes the results to the specified output file (see B<-o>).
Output is a sorted list of hash/category pairs having the following
format:

    hash|category

Input files located in B<dir> are expected to contain HTML and have
names that match the following regular expression:

    ^hashdig-sunsolve.\d+$

Filenames that do not match this expression are silently ignored.

=head1 OPTIONS

=over 4

=item B<-c category>

Specifies the hash category, {A|K|U}, that is to be harvested.
Currently, the following categories are supported: all (A), known
(K), and unknown (U). The default category is known. Output returned
by sunsolve specifies the number of times a given hash matched
something in Sun's Solaris Fingerprint Database. Thus, if the number
of matches is zero, the hash is tagged as unknown. Otherwise, it
is tagged as known.

=item B<-o {file|-}>

Specifies the name of the output file. A value of '-' will cause
the program to write to stdout.

=item B<-q>

Don't report errors (i.e., be quiet) while processing files.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH envrionment variable. Alternate sort utilities
must support the C<-o>, C<-T> and C<-u> options. This program was
designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is that specified by the TMPDIR environment
variable, or /tmp if TMPDIR is not set.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-harvest(1), hashdig-make(1), hashdig-resolve-sunsolve(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
