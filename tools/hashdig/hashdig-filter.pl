#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-filter.pl,v 1.17 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
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

  my ($sProgram);

  $sProgram = basename(__FILE__);

  ####################################################################
  #
  # Initialize filters.
  #
  ####################################################################

  my @aBinDirs =
     (
       "/bin/",
       "/kvm/",
       "/opt/bin/",
       "/opt/local/bin/",
       "/opt/local/sbin/",
       "/opt/sbin/",
       "/platform/",
       "/sbin/",
       "/usr/bin/",
       "/usr/ccs/bin/",
       "/usr/kvm/",
       "/usr/local/bin/",
       "/usr/local/sbin/",
       "/usr/platform/",
       "/usr/sbin/",
       "/usr/ucb/"
     );

  my @aDevDirs =
     (
       "/dev/",
       "/devices/"
     );

  my @aEtcDirs =
     (
       "/etc/",
       "/opt/etc/",
       "/opt/local/etc/",
       "/usr/local/etc/"
     );

  my @aLibDirs =
     (
       "/lib/",
       "/libexec/",
       "/opt/lib/",
       "/opt/libexec/",
       "/opt/local/lib/",
       "/opt/local/libexec/",
       "/usr/ccs/lib/",
       "/usr/lib/",
       "/usr/libexec/",
       "/usr/local/lib/",
       "/usr/local/libexec/",
       "/usr/ucblib/"
     );

  my @aKernelDirs =
     (
       "/kernel/",
       "/modules/",
       "/usr/kernel/",
       "/usr/modules/"
     );

  my @aManDirs =
     (
       "/opt/man/",
       "/opt/local/man/",
       "/opt/local/share/man/",
       "/opt/share/man/",
       "/usr/local/man/",
       "/usr/local/share/man/",
       "/usr/man/",
       "/usr/share/man/"
     );

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('p:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The Prefix flag, '-p', is optional.
  #
  ####################################################################

  my ($sPrefix);

  $sPrefix = (exists($hOptions{'p'})) ? $hOptions{'p'} : "";
  $sPrefix =~ s/\/+$//;

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
  # Finalize regular expressions.
  #
  ####################################################################

  my $sBinRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aBinDirs) . ')';
  my $sDevRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aDevDirs) . ')';
  my $sEtcRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aEtcDirs) . ')';
  my $sLibRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aLibDirs) . ')';
  my $sKernelRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aKernelDirs) . ')';
  my $sManRegex = '\|(?:' . $sPrefix . join('|' . $sPrefix, @aManDirs) . ')';

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  my (@aHandles, %hHandleList);

  @aHandles = ("bin", "dev", "etc", "lib", "kernel", "man", "other");

  foreach my $sInputFile (@ARGV)
  {
    ##################################################################
    #
    # Open file handles.
    #
    ##################################################################

    if (!open(FH, "< $sInputFile"))
    {
      print STDERR "$sProgram: File='$sInputFile' Error='$!'\n";
      next;
    }

    if (!defined(OpenFileHandles($sInputFile, \@aHandles, \%hHandleList)))
    {
      print STDERR "$sProgram: File='$sInputFile' Error='Unable to create output files.'\n";
      close(FH);
      next;
    }

    ##################################################################
    #
    # Filter input keeping a tally of matches.
    #
    ##################################################################

    my $sHandle;
    my $sBinCounter = 0;
    my $sDevCounter = 0;
    my $sEtcCounter = 0;
    my $sLibCounter = 0;
    my $sKernelCounter = 0;
    my $sManCounter = 0;
    my $sOtherCounter = 0;
    my $sAllCounter = 0;

    while (my $sRecord = <FH>)
    {
      $sRecord =~ s/[\r\n]+$//;
      if ($sRecord =~ /$sBinRegex/o)
      {
        $sHandle = $hHandleList{'bin'};
        print $sHandle "$sRecord\n";
        $sBinCounter++;
      }
      elsif ($sRecord =~ /$sDevRegex/o)
      {
        $sHandle = $hHandleList{'dev'};
        print $sHandle "$sRecord\n";
        $sDevCounter++;
      }
      elsif ($sRecord =~ /$sEtcRegex/o)
      {
        $sHandle = $hHandleList{'etc'};
        print $sHandle "$sRecord\n";
        $sEtcCounter++;
      }
      elsif ($sRecord =~ /$sLibRegex/o)
      {
        $sHandle = $hHandleList{'lib'};
        print $sHandle "$sRecord\n";
        $sLibCounter++;
      }
      elsif ($sRecord =~ /$sKernelRegex/o)
      {
        $sHandle = $hHandleList{'kernel'};
        print $sHandle "$sRecord\n";
        $sKernelCounter++;
      }
      elsif ($sRecord =~ /$sManRegex/o)
      {
        $sHandle = $hHandleList{'man'};
        print $sHandle "$sRecord\n";
        $sManCounter++;
      }
      else
      {
        $sHandle = $hHandleList{'other'};
        print $sHandle "$sRecord\n";
        $sOtherCounter++;
      }
      $sAllCounter++;
    }

    ##################################################################
    #
    # Close file handles.
    #
    ##################################################################

    foreach my $sHandle (keys(%hHandleList))
    {
      close($hHandleList{$sHandle});
    }
    close(FH);

    ##################################################################
    #
    # Print tally report.
    #
    ##################################################################

    my (@aCounts);

    push(@aCounts, "all='$sAllCounter'");
    push(@aCounts, "bin='$sBinCounter'");
    push(@aCounts, "dev='$sDevCounter'");
    push(@aCounts, "etc='$sEtcCounter'");
    push(@aCounts, "lib='$sLibCounter'");
    push(@aCounts, "kernel='$sKernelCounter'");
    push(@aCounts, "man='$sManCounter'");
    push(@aCounts, "other='$sOtherCounter'");
    print "File='$sInputFile' ", join(' ', @aCounts), "\n";
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
  my ($sInputFile, $paHandles, $phHandleList) = @_;

  my ($sFailures, $sOutBase);

  my ($sName, $sPath, $sSuffix) = fileparse($sInputFile);

  $sOutBase = $sName;

  $sFailures = 0;

  foreach my $sExtension (@$paHandles)
  {
    $$phHandleList{$sExtension} = new FileHandle(">$sOutBase.$sExtension");
    if (!defined($$phHandleList{$sExtension}))
    {
      $sFailures++;
    }
  }

  if ($sFailures)
  {
    foreach my $sExtension (@$paHandles)
    {
      unlink("$sOutBase.$sExtension");
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
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-p prefix] file [file ...]\n";
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
to be one or more files created by hashdig-bind(1). Output is written
to a set of files, in the current working directory, having the
following format:

    category|hash|name

The naming convention for output files is:

    <filename>.{bin|dev|etc|lib|kernel|man|other}

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

=back

=head1 CAVEATS

Currently, this program is only useful for filtering UNIX filenames.

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

hashdig-bind(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
