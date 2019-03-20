#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-resolve-sunsolve.pl,v 1.4 2003/03/26 19:10:58 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Resolve hashes against Sun's Solaris Fingerprint Database.
#
######################################################################

use strict;
use Getopt::Std;
use IO::Socket;

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

  $program = "hashdig-resolve-sunsolve.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('d:f:k:s:w:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # An output directory, '-d', is optional. Default value is "sunsolve".
  #
  ####################################################################

  my $outDir = (exists($options{'d'})) ? $options{'d'} : "sunsolve";

  ####################################################################
  #
  # A filename is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($fileHandle, $filename);

  if (!exists($options{'f'}))
  {
    Usage($program);
  }
  else
  {
    $filename = $options{'f'};
    if (!defined($filename) || length($filename) < 1)
    {
      Usage($program);
    }
    if (-f $filename)
    {
      if (!open(FH, $filename))
      {
        print STDERR "$program: File='$filename' Error='$!'\n";
        exit(2);
      }
      $fileHandle = \*FH;
    }
    else
    {
      if ($filename ne '-')
      {
        print STDERR "$program: File='$filename' Error='File must be regular.'\n";
        exit(2);
      }
      $fileHandle = \*STDIN;
    }
  }

  ####################################################################
  #
  # A kidLimit, '-k', is optional. Default value is 5.
  #
  ####################################################################

  my $kidLimit = (exists($options{'k'})) ? $options{'k'} : 5;

  if ($kidLimit !~ /^\d+$/)
  {
    print STDERR "$program: KidLimit='$kidLimit' Error='Limit must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A sleepInterval, '-s', is optional. Default value is 0.
  #
  ####################################################################

  my $sleepInterval = (exists($options{'s'})) ? $options{'s'} : 0;

  if ($sleepInterval !~ /^\d+$/)
  {
    print STDERR "$program: SleepInterval='$sleepInterval' Error='Interval must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A warningLimit, '-w', is optional. Default value is 1.
  #
  ####################################################################

  my $warningLimit = (exists($options{'w'})) ? $options{'w'} : 1;

  if ($warningLimit !~ /^\d+$/)
  {
    print STDERR "$program: WarningLimit='$warningLimit' Error='Limit must be a number.'\n";
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
  # Create output directory.
  #
  ####################################################################

  if (-d $outDir || !mkdir($outDir, 0755))
  {
    print STDERR "$program: Directory='$outDir' Error='Directory exists or could not be created.'\n";
    exit(2);
  }

  ####################################################################
  #
  # Resolve hashes in blocks of 256 (limit imposed by sun).
  #
  ####################################################################

  my ($block, $count, $errorMessage, $kidPid, @kidPids, $md5Hashes, $warnings);

  for ($block = $count = 1, $md5Hashes = "", $warnings = 0; my $record = <$fileHandle>; $count++)
  {
    if (scalar(@kidPids) >= $kidLimit)
    {
      for (my $pid = wait, my $i = 0; $i < scalar(@kidPids); $i++)
      {
        if ($kidPids[$i] == $pid)
        {
          splice(@kidPids, $i, 1);
          last;
        }
      }
    }
    $record =~ s/[\r\n]+$//;
    if ($record !~ /^[0-9a-fA-F]{32}[\r\n]*$/)
    {
      print STDERR "$program: File='$filename' Record='$record' Error='Record did not parse properly.'\n";
      $warnings++;
      if ($warningLimit && $warnings >= $warningLimit)
      {
        print STDERR "$program: WarningLimit='$warningLimit' Error='Limit exceeded. Program aborting.'\n";
        exit(2);
      }
      next;
    }
    $md5Hashes .= $record . "\n";
    if ($count % 256 == 0)
    {
      $kidPid = SunFingerPrintLookup($outDir, $block, $md5Hashes, \$errorMessage);
      if (!defined($kidPid))
      {
        print STDERR "$program: $errorMessage\n";
      }
      else
      {
        push(@kidPids, $kidPid);
      }
      sleep($sleepInterval) if ($sleepInterval > 0);
      $md5Hashes = "";
      $block++;
    }
  }

  ####################################################################
  #
  # Process any remaining input.
  #
  ####################################################################

  if ($count && length($md5Hashes))
  {
    $kidPid = SunFingerPrintLookup($outDir, $block, $md5Hashes, \$errorMessage);
    if (!defined($kidPid))
    {
      print STDERR "$program: $errorMessage\n";
    }
    else
    {
      push(@kidPids, $kidPid);
    }
    $md5Hashes = "";
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  while (wait() != -1) {}

  1;


######################################################################
#
# SunFingerPrintLookup
#
######################################################################

sub SunFingerPrintLookup
{
  my ($outDir, $block, $md5Hashes, $pErrorMessage) = @_;

  my $sunSite = "sunsolve.sun.com";
  my $getFile = sprintf("%s/hashdig-getblock.%06d", $outDir, $block);
  my $outFile = sprintf("%s/hashdig-sunsolve.%06d", $outDir, $block);

  ####################################################################
  #
  # Spawn a child process to handle the lookup.
  #
  ####################################################################

  my $kidPid = fork();
  if (!defined($kidPid))
  {
    $$pErrorMessage = "SunFingerPrintLookup(): Block='$block' Error='$!'";
    return undef;
  }

  if ($kidPid == 0)
  {
    if (!open(FH, ">$getFile"))
    {
      print STDERR "SunFingerPrintLookup(): Block='$block' Error='$!'\n";
      exit(2);
    }
    print FH $md5Hashes;
    close(FH);
    if (!open(FH, ">$outFile"))
    {
      print STDERR "SunFingerPrintLookup(): Block='$block' Error='$!'\n";
      exit(2);
    }

    my $handle = IO::Socket::INET->new(Proto => "tcp", PeerAddr => $sunSite, PeerPort => 80) or die "($!)\n";
    $handle->autoflush(1);

    ##################################################################
    #
    # Determine the size of the query string.
    #
    ##################################################################

    my $md5List = "md5list=";
    my $submit = "&submit=submit";

    my $size = length($md5List) + length($md5Hashes) + length($submit);

    ##################################################################
    #
    # Transmit the head.
    #
    ##################################################################

    my $header = "POST /pub-cgi/fileFingerprints.pl HTTP/1.0\n";
    my $ctype = "Content-type: application/x-www-form-urlencoded\nContent-Length:$size\n\n";

    if (!print $handle $header . $ctype . $md5List)
    {
      print STDERR "SunFingerPrintLookup(): Block='$block' Error='Socket write failed for header data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Push out the hash data.
    #
    ##################################################################

    if (!print $handle $md5Hashes)
    {
      print STDERR "SunFingerPrintLookup(): Block='$block' Error='Socket write failed for hash data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Push out the submit data.
    #
    ##################################################################

    if (!print $handle $submit)
    {
      print STDERR "SunFingerPrintLookup(): Block='$block' Error='Socket write failed for submit data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Read the return data, and write it to the output file.
    #
    ##################################################################

    while (<$handle>)
    {
      print FH;
    }
    close(FH);

    ##################################################################
    #
    # Exit the child process.
    #
    ##################################################################

    exit(0);
  }
  else
  {
    print STDOUT "KidPid='$kidPid' File='$outFile'\n";
  }

  return $kidPid;
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
  print STDERR "Usage: $program [-d dir] [-k count] [-s count] [-w count] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-resolve-sunsolve.pl - Resolve hashes against Sun's Solaris Fingerprint Database

=head1 SYNOPSIS

B<hashdig-resolve-sunsolve.pl> B<[-d dir]> B<[-k count]> B<[-w count]> B<[-s count]> B<-f {file|-}>

=head1 DESCRIPTION

This utility resolves a list of hashes against Sun's Solaris
Fingerprint Database. Input is expected to be plain text with one
hash per line. Each line must match the following regular expression:

    ^[0-9a-fA-F]{32}$

Input that does not match this expression will cause the program
to generate a warning. When the warning limit (see B<-w> option)
has been exceeded, the program will abort.

Output for each block of 256 hashes is written to a pair of files
in B<dir>. These files have the following naming convention:

    hashdig-{getblock,sunsolve}.dddddd

where 'dddddd' is a decimal number that represents the request ID.
The first file, hashdig-getblock.dddddd, contains the list of hashes
submitted. The second file, hashdig-sunsolve.dddddd, contains the
raw HTML output returned by Sun's website.

=head1 OPTIONS

=over 4

=item B<-d dir>

Specifies the name of the output directory. By default the output
directory is called sunsolve. If the output directory exists, the
program will abort.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-k count>

Specifies the number of simultaneous kid processes to create -- one
kid per request. The default value is 5.

=item B<-s count>

Specifies the number of seconds to wait between subsequent requests.
The default value is 0, which means do not wait.

=item B<-w count>

Specifies the number of warnings that will be tolerated before the
program aborts. The default value is 1. If this option is set to
0, no limit will be imposed.

=back

=head1 BUGS

Under certain conditions (e.g., slow link, killed TCP sessions,
etc.), requests can fail without an error message being generated.
In these situations, output from the affected request is often
invalid or incomplete. Currently, there is no mechanism to detect
whether or not such is the case.

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-dump.pl, hashdig-harvest-sunsolve.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
