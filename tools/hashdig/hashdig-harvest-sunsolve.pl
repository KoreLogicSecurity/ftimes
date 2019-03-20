#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-harvest-sunsolve.pl,v 1.3 2003/03/26 20:47:29 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Harvest hashes from a directory of sunsolve output.
#
######################################################################

use strict;
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

  $program = "hashdig-harvest-sunsolve.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('c:o:qs:T:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The category flag, '-c', is optional. Default value is "K".
  #
  ####################################################################

  my ($category);

  $category = (exists($options{'c'})) ? uc($options{'c'}) : "K";

  if ($category !~ /^[AKU]$/)
  {
    print STDERR "$program: Category='$category' Error='Invalid category.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A filename, '-o', is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($filename);

  $filename = (exists($options{'o'})) ? $options{'o'} : undef;

  if (!defined($filename) || length($filename) < 1)
  {
    Usage($program);
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
  # The sort flag, '-s', is optional. Default value is "sort".
  #
  ####################################################################

  my ($sort);

  $sort = (exists($options{'s'})) ? $options{'s'} : "sort";

  ####################################################################
  #
  # The tmpDir flag, '-T', is optional. Default value is "/tmp".
  #
  ####################################################################

  my ($tmpDir);

  $tmpDir = (exists($options{'T'})) ? $options{'T'} : "/tmp";

  ####################################################################
  #
  # If there isn't one argument left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) != 1)
  {
    Usage($program);
  }

  ####################################################################
  #
  # Unlink output file, if necessary.
  #
  ####################################################################

  if ($filename ne "-" && -f $filename && !unlink($filename))
  {
    print STDERR "$program: File='$filename' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Open and read the input directory. Create a list of files.
  #
  ####################################################################

  my ($directory, @files);

  $directory = shift;
  $directory =~ s/\/*$//;
  if (!opendir(DIR, $directory))
  {
    print STDERR "$program: Directory='$directory' Error='$!'\n";
    exit(2);
  }
  @files = sort(grep(/^$directory\/hashdig-sunsolve.\d+$/, map("$directory/$_", readdir(DIR))));

  ####################################################################
  #
  # Open a pipe to the sort utility.
  #
  ####################################################################

  my ($command);

  $command = "$sort -u -T $tmpDir";
  if ($filename ne "-")
  {
    $command .= " -o $filename";
  }
  if (!open(SH, "|$command"))
  {
    print STDERR "$program: Command='$command' Error='$!'\n";
    exit(2);
  }

  ####################################################################
  #
  # Iterate over input files. Extract matches and pipe them to sort.
  #
  ####################################################################

  foreach my $inputFile (@files)
  {
    if (!open(FH, "<$inputFile"))
    {
      if (!$beQuiet)
      {
        print STDERR "$program: File='$inputFile' Error='$!'\n";
      }
      next;
    }
    while (my $record = <FH>)
    {
      if (my ($hash, $matches) = ($record =~ /^.*<TT>([0-9a-fA-F]{32})<\/TT>\s+-\s+<TT><\/TT>\s+-\s+(\d+)\smatch.*$/o))
      {
        my $value = ($matches) ? "K" : "U";
        if ($category eq "A" || ($value eq $category))
        {
          print SH lc($hash), "|", $value, "\n";
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
  my ($program) = @_;
  print STDERR "\n";
  print STDERR "Usage: $program [-c {A|K|U}] [-q] [-s file] [-T dir] -o {file|-} dir\n";
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
by hashdig-resolve-sunsolve.pl, filters them by category (see B<-c>),
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

Don't report errors (i.e. be quiet) while processing files.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH envrionment variable. Alternate sort utilities
must support the C<-o>, C<-T> and C<-u> options. This program was
designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is /tmp.

=back

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-harvest.pl, hashdig-make.pl, hashdig-resolve-sunsolve.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
