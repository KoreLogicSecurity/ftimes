#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-filter.pl,v 1.5 2003/08/02 14:59:21 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Filter filenames by directory type.
#
######################################################################

use strict;
use File::Basename;
use FileHandle;
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

  $program = "hashdig-filter.pl";

  ####################################################################
  #
  # Initialize filters.
  #
  ####################################################################

  my @binDirs =
     (
       "/bin",
       "/kernel",
       "/kvm",
       "/opt/bin",
       "/opt/local/bin",
       "/opt/local/sbin",
       "/opt/sbin",
       "/platform",
       "/sbin",
       "/usr/bin",
       "/usr/ccs",
       "/usr/kernel",
       "/usr/local/bin",
       "/usr/local/sbin",
       "/usr/sbin",
       "/usr/ucb"
     );

  my @devDirs =
     (
       "/dev",
       "/devices"
     );

  my @etcDirs =
     (
       "/etc",
       "/opt/etc",
       "/opt/local/etc",
       "/usr/local/etc"
     );

  my @libDirs =
     (
       "/lib",
       "/libexec",
       "/opt/lib",
       "/opt/libexec",
       "/opt/local/lib",
       "/opt/local/libexec",
       "/usr/lib",
       "/usr/libexec",
       "/usr/local/lib",
       "/usr/local/libexec",
       "/usr/ucblib"
     );

  my @manDirs =
     (
       "/opt/man",
       "/opt/local/man",
       "/opt/local/share/man",
       "/opt/share/man",
       "/usr/local/man",
       "/usr/local/share/man",
       "/usr/man",
       "/usr/share/man"
     );

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('p:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The prefix flag, '-p', is optional. Default value is "".
  #
  ####################################################################

  my ($prefix);

  $prefix = (exists($options{'p'})) ? $options{'p'} : "";
  $prefix =~ s/\/+$//;

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
  # Finalize regular expressions.
  #
  ####################################################################

  my $binRegex = '\|(?:' . $prefix . join('|' . $prefix, @binDirs) . ')';
  my $devRegex = '\|(?:' . $prefix . join('|' . $prefix, @devDirs) . ')';
  my $etcRegex = '\|(?:' . $prefix . join('|' . $prefix, @etcDirs) . ')';
  my $libRegex = '\|(?:' . $prefix . join('|' . $prefix, @libDirs) . ')';
  my $manRegex = '\|(?:' . $prefix . join('|' . $prefix, @manDirs) . ')';

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  my (@handles, %handleList);

  @handles = ("bin", "dev", "etc", "lib", "man", "other");

  foreach my $inputFile (@ARGV)
  {
    ##################################################################
    #
    # Open file handles.
    #
    ##################################################################

    if (!open(FH, "<$inputFile"))
    {
      print STDERR "$program: File='$inputFile' Error='$!'\n";
      next;
    }

    if (!defined(OpenFileHandles($inputFile, \@handles, \%handleList)))
    {
      print STDERR "$program: File='$inputFile' Error='Unable to create output files.'\n";
      close(FH);
      next;
    }

    ##################################################################
    #
    # Filter input keeping a tally of matches.
    #
    ##################################################################

    my $handle;
    my $binCounter = 0;
    my $devCounter = 0;
    my $etcCounter = 0;
    my $libCounter = 0;
    my $manCounter = 0;
    my $otherCounter = 0;
    my $allCounter = 0;

    while(my $record = <FH>)
    {
      $record =~ s/[\r\n]+$//;
      if ($record =~ /$binRegex/o)
      {
        $handle = $handleList{'bin'};
        print $handle "$record\n";
        $binCounter++;
      }
      elsif ($record =~ /$devRegex/o)
      {
        $handle = $handleList{'dev'};
        print $handle "$record\n";
        $devCounter++;
      }
      elsif ($record =~ /$etcRegex/o)
      {
        $handle = $handleList{'etc'};
        print $handle "$record\n";
        $etcCounter++;
      }
      elsif ($record =~ /$libRegex/o)
      {
        $handle = $handleList{'lib'};
        print $handle "$record\n";
        $libCounter++;
      }
      elsif ($record =~ /$manRegex/o)
      {
        $handle = $handleList{'man'};
        print $handle "$record\n";
        $manCounter++;
      }
      else
      {
        $handle = $handleList{'other'};
        print $handle "$record\n";
        $otherCounter++;
      }
      $allCounter++;
    }

    ##################################################################
    #
    # Close file handles.
    #
    ##################################################################

    foreach my $handle (keys(%handleList))
    {
      close($handleList{$handle});
    }
    close(FH);

    ##################################################################
    #
    # Print tally report.
    #
    ##################################################################

    my (@counts);

    push(@counts, "all='$allCounter'");
    push(@counts, "bin='$binCounter'");
    push(@counts, "dev='$devCounter'");
    push(@counts, "etc='$etcCounter'");
    push(@counts, "lib='$libCounter'");
    push(@counts, "man='$manCounter'");
    push(@counts, "other='$otherCounter'");
    print "File='$inputFile' ", join(' ', @counts), "\n";
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  1;


######################################################################
#
# OpenFileHandles
#
######################################################################

sub OpenFileHandles
{
  my ($inputFile, $pHandles, $pHandleList) = @_;

  my ($failures, $outBase);

  my ($name, $path, $suffix) = fileparse($inputFile);

  $outBase = $name;

  $failures = 0;

  foreach my $extension (@$pHandles)
  {
    $$pHandleList{$extension} = new FileHandle(">$outBase.$extension");
    if (!defined($$pHandleList{$extension}))
    {
      $failures++;
    }
  }

  if ($failures)
  {
    foreach my $extension (@$pHandles)
    {
      unlink("$outBase.$extension");
    }
    return undef;
  }

  return 1;
}


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($program) = @_;
  print STDERR "\n";
  print STDERR "Usage: $program [-p prefix] file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-filter.pl - Filter filenames by directory type

=head1 SYNOPSIS

B<hashdig-filter.pl> B<[-p prefix]> B<file [file ...]>

=head1 DESCRIPTION

This utility filters filenames by directory type. Input is expected
to be one or more files created by hashdig-bind.pl. Output is written
to a set of files, in the current working directory, having the
following format:

    category|hash|name

The naming convention for output files is:

    <filename>.{bin|dev|etc|lib|man|other}

Any input that does not match one of the defined filters is written
to the 'other' file.

=head1 OPTIONS

=over 4

=item B<-p prefix>

Specifies a prefix to prepend to each directory type. A prefix is
useful when subject filenames are not rooted at "/". This could
happen, for example, when backups are restored to a temporary work
area or a subject's root file system is mounted on a forensic
workstation. Trailing slashes are automatically pruned from the
supplied prefix.

=head1 CAVEATS

Currently, this program is only useful for filtering UNIX filenames.

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-bind.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
