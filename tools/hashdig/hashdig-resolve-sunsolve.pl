#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-resolve-sunsolve.pl,v 1.19 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Resolve hashes against Sun's Solaris Fingerprint Database.
#
######################################################################

use strict;
use File::Basename;
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

  my ($sProgram);

  $sProgram = basename(__FILE__);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('ad:f:k:r:s:w:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The AutoRecover flag, '-a', is optional.
  #
  ####################################################################

  my $sAutoRecover = (exists($hOptions{'a'})) ? 1 : 0;

  ####################################################################
  #
  # An output directory, '-d', is optional.
  #
  ####################################################################

  my $sOutDir = (exists($hOptions{'d'})) ? $hOptions{'d'} : "sunsolve";

  ####################################################################
  #
  # A filename is required, and can be '-' or a regular file.
  #
  ####################################################################

  my ($sFileHandle, $sFilename);

  if (!exists($hOptions{'f'}) || !defined($hOptions{'f'}) || length($hOptions{'f'}) < 1)
  {
    Usage($sProgram);
  }
  $sFilename = $hOptions{'f'};

  if ($sFilename eq '-')
  {
    $sFileHandle = \*STDIN;
  }
  else
  {
    if (!-f $sFilename)
    {
      print STDERR "$sProgram: File='$sFilename' Error='File must exist and be regular.'\n";
      exit(2);
    }
    if (!open(FH, "< $sFilename"))
    {
      print STDERR "$sProgram: File='$sFilename' Error='$!'\n";
      exit(2);
    }
    $sFileHandle = \*FH;
  }

  ####################################################################
  #
  # A kidLimit, '-k', is optional.
  #
  ####################################################################

  my $sKidLimit = (exists($hOptions{'k'})) ? $hOptions{'k'} : 5;

  if ($sKidLimit !~ /^\d+$/)
  {
    print STDERR "$sProgram: KidLimit='$sKidLimit' Error='Limit must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A redoBlock, '-r', is optional.
  #
  ####################################################################

  my $sRedoBlock = (exists($hOptions{'r'})) ? $hOptions{'r'} : undef;

  if (defined($sRedoBlock) && $sRedoBlock !~ /^\d+$/)
  {
    print STDERR "$sProgram: GetBlock='$sRedoBlock' Error='Block must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A sleepInterval, '-s', is optional.
  #
  ####################################################################

  my $sSleepInterval = (exists($hOptions{'s'})) ? $hOptions{'s'} : 0;

  if ($sSleepInterval !~ /^\d+$/)
  {
    print STDERR "$sProgram: SleepInterval='$sSleepInterval' Error='Interval must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # A warningLimit, '-w', is optional.
  #
  ####################################################################

  my $sWarningLimit = (exists($hOptions{'w'})) ? $hOptions{'w'} : 1;

  if ($sWarningLimit !~ /^\d+$/)
  {
    print STDERR "$sProgram: WarningLimit='$sWarningLimit' Error='Limit must be a number.'\n";
    exit(2);
  }

  ####################################################################
  #
  # If there's any arguments left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # Verify existence of or create output directory.
  #
  ####################################################################

  if (defined($sRedoBlock) || $sAutoRecover)
  {
    if (!-d $sOutDir)
    {
      print STDERR "$sProgram: Directory='$sOutDir' Error='Directory does not exist.'\n";
      exit(2);
    }
  }
  else
  {
    if (-d $sOutDir || !mkdir($sOutDir, 0755))
    {
      print STDERR "$sProgram: Directory='$sOutDir' Error='Directory exists or could not be created.'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Resolve hashes in blocks of 256 (limit imposed by sun).
  #
  ####################################################################

  my ($sBlock, $sCount, $sErrorMessage, $sKidPid, @aKidPids, $sMD5Hashes, $sWarnings);

  $sBlock = (defined($sRedoBlock)) ? $sRedoBlock : 0;
  $sCount = 0;
  $sMD5Hashes = "";
  $sWarnings = 0;

  while (my $sRecord = <$sFileHandle>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if ($sRecord =~ /^([0-9a-fA-F]{32})(?:\|[KU])?$/)
    {
      $sMD5Hashes .= $1 . "\n";
      if (++$sCount % 256 == 0)
      {
        if (scalar(@aKidPids) >= $sKidLimit)
        {
          my $sPid = wait;
          for (my $sIndex = 0; $sIndex < scalar(@aKidPids); $sIndex++)
          {
            if ($aKidPids[$sIndex] == $sPid)
            {
              splice(@aKidPids, $sIndex, 1);
              last;
            }
          }
        }
        if ($sAutoRecover && BlockIsGood($sOutDir, $sBlock, $sMD5Hashes, \$sErrorMessage))
        {
          print STDERR "$sProgram: Block='$sBlock' Warning='Block skipped: file exists and is not empty.'\n";
          $sMD5Hashes = "";
          $sBlock++;
          next;
        }
        $sKidPid = SunFingerPrintLookup($sOutDir, $sBlock, $sMD5Hashes, $sRedoBlock, \$sErrorMessage);
        if (!defined($sKidPid))
        {
          print STDERR "$sProgram: $sErrorMessage\n";
        }
        else
        {
          push(@aKidPids, $sKidPid);
        }
        sleep($sSleepInterval) if ($sSleepInterval > 0);
        $sMD5Hashes = "";
        $sBlock++;
        last if (defined($sRedoBlock));
      }
    }
    else
    {
      print STDERR "$sProgram: File='$sFilename' Record='$sRecord' Error='Record did not parse properly.'\n";
      $sWarnings++;
      if ($sWarningLimit && $sWarnings >= $sWarningLimit)
      {
        print STDERR "$sProgram: WarningLimit='$sWarningLimit' Error='Limit exceeded. Program aborting.'\n";
        WaitLoop();
        exit(2);
      }
    }
  }

  ####################################################################
  #
  # Process any remaining input.
  #
  ####################################################################

  if ($sCount && length($sMD5Hashes))
  {
    if ($sAutoRecover && BlockIsGood($sOutDir, $sBlock, $sMD5Hashes, \$sErrorMessage))
    {
      print STDERR "$sProgram: Block='$sBlock' Warning='Block skipped: file exists and is not empty.'\n";
    }
    else
    {
      $sKidPid = SunFingerPrintLookup($sOutDir, $sBlock, $sMD5Hashes, $sRedoBlock, \$sErrorMessage);
      if (!defined($sKidPid))
      {
        print STDERR "$sProgram: $sErrorMessage\n";
      }
      else
      {
        push(@aKidPids, $sKidPid);
      }
    }
    $sMD5Hashes = "";
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  WaitLoop();

  1;


######################################################################
#
# BlockIsGood
#
######################################################################

sub BlockIsGood
{
  my ($sOutDir, $sBlock, $sMD5Hashes, $psErrorMessage) = @_;

  my $sGetFile = sprintf("%s/hashdig-getblock.%06d", $sOutDir, $sBlock);
  my $sOutFile = sprintf("%s/hashdig-sunsolve.%06d", $sOutDir, $sBlock);

  if (!-s $sGetFile || !-s $sOutFile)
  {
    return 0;
  }

  1;
}


######################################################################
#
# SunFingerPrintLookup
#
######################################################################

sub SunFingerPrintLookup
{
  my ($sOutDir, $sBlock, $sMD5Hashes, $sRedoBlock, $psErrorMessage) = @_;

  my $sSunSite = "sunsolve.sun.com";
  my $sGetFile = sprintf("%s/hashdig-getblock.%06d", $sOutDir, $sBlock);
  my $sOutFile = sprintf("%s/hashdig-sunsolve.%06d", $sOutDir, $sBlock);

  ####################################################################
  #
  # Spawn a child process to handle the lookup.
  #
  ####################################################################

  my $sKidPid = fork();
  if (!defined($sKidPid))
  {
    $$psErrorMessage = "SunFingerPrintLookup(): Block='$sBlock' Error='$!'";
    return undef;
  }

  if ($sKidPid == 0)
  {
    if (!defined($sRedoBlock) || !-f $sGetFile || !-s _)
    {
      if (!open(FH, "> $sGetFile"))
      {
        print STDERR "SunFingerPrintLookup(): Block='$sBlock' Error='$!'\n";
        exit(2);
      }
      print FH $sMD5Hashes;
      close(FH);
    }
    if (!open(FH, "> $sOutFile"))
    {
      print STDERR "SunFingerPrintLookup(): Block='$sBlock' Error='$!'\n";
      exit(2);
    }

    my $sHandle = IO::Socket::INET->new(Proto => "tcp", PeerAddr => $sSunSite, PeerPort => 80) or die "($!)\n";
    $sHandle->autoflush(1);

    ##################################################################
    #
    # Determine the size of the query string.
    #
    ##################################################################

    my $sMD5List = "md5list=";
    my $sSubmit = "&submit=submit";

    my $sSize = length($sMD5List) + length($sMD5Hashes) + length($sSubmit);

    ##################################################################
    #
    # Transmit the head.
    #
    ##################################################################

    my $sHeader = "POST /pub-cgi/fileFingerprints.pl HTTP/1.0\n";
    my $sCType = "Content-Type: application/x-www-form-urlencoded\nContent-Length:$sSize\n\n";

    if (!print $sHandle $sHeader . $sCType . $sMD5List)
    {
      print STDERR "SunFingerPrintLookup(): Block='$sBlock' Error='Socket write failed for header data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Push out the hash data.
    #
    ##################################################################

    if (!print $sHandle $sMD5Hashes)
    {
      print STDERR "SunFingerPrintLookup(): Block='$sBlock' Error='Socket write failed for hash data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Push out the submit data.
    #
    ##################################################################

    if (!print $sHandle $sSubmit)
    {
      print STDERR "SunFingerPrintLookup(): Block='$sBlock' Error='Socket write failed for submit data: ($!)'\n";
      exit(2);
    }

    ##################################################################
    #
    # Read the return data, and write it to the output file.
    #
    ##################################################################

    while (<$sHandle>)
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
    print STDOUT "KidPid='$sKidPid' File='$sOutFile'\n";
  }

  return $sKidPid;
}


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-a] [-d dir] [-k count] [-r block] [-s count] [-w count] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


######################################################################
#
# WaitLoop
#
######################################################################

sub WaitLoop
{
  while (wait() != -1) {}
}


=pod

=head1 NAME

hashdig-resolve-sunsolve.pl - Resolve hashes against Sun's Solaris Fingerprint Database

=head1 SYNOPSIS

B<hashdig-resolve-sunsolve.pl> B<[-a]> B<[-d dir]> B<[-k count]> B<[-w count]> B<[-r block]> B<[-s count]> B<-f {file|-}>

=head1 DESCRIPTION

This utility resolves a list of hashes against Sun's Solaris
Fingerprint Database. Input is expected to be plain text with one
hash per line. Each line must match the following regular expression:

    ^([0-9a-fA-F]{32})(?:\|[KU])?$

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

=item B<-a>

Enables auto-recover mode. Individual requests can fail from time
to time, or the job may have been aborted or killed prior to
completion. This option allows you to continue the resolution process
where the last job left off. Along the way, it'll redo any getblocks
that produced no output. The original output B<dir> must exist, and
be specified if not the default value. More importantly, the original
input must be used for this mode to work as intended.

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

=item B<-r block>

Specifies a particular request ID to redo. Individual requests can
fail from time to time; this option allows you to selectively redo
those requests as necessary. The original output B<dir> must exist,
and be specified if not the default value. The B<file> argument
should be the name of the original getblock file or '-' for stdin.
However, no more than 256 valid records from the specified B<file>
will be processed. Output will be written to the original sunsolve
file whether it exists or not. A corresponding getblock file will
be created only if it doesn't exist.

=item B<-s count>

Specifies the number of seconds to wait between subsequent requests.
The default value is 0, which means do not wait.

=item B<-w count>

Specifies the number of warnings that will be tolerated before the
program aborts. The default value is 1. If this option is set to
0, no limit will be imposed.

=back

=head1 BUGS

Under certain conditions (e.g., slow link, killed TCP sessions, too
many kid processes, etc.), requests can fail without generating an
error message. In these situations, output from the affected request
is often invalid or incomplete. Currently, there is no mechanism
to detect whether or not such is the case.

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-dump(1), hashdig-harvest-sunsolve(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
