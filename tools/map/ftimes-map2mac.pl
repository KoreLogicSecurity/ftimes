#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-map2mac.pl,v 1.4 2003/08/12 18:17:31 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Create MAC/MACH timelines using FTimes map data.
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

  $program = "ftimes-map2mac.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options, $runMode);

  $runMode = shift || Usage($program);

  if ($runMode =~ /^(-e|--extsort)$/)
  {
    if (!getopts('df:rs:T:w', \%options))
    {
      Usage($program);
    }
    $runMode = "extsort";
  }
  elsif ($runMode =~ /^(-i|--intsort)$/)
  {
    if (!getopts('df:rw', \%options))
    {
      Usage($program);
    }
    $runMode = "intsort";
  }
  else
  {
    Usage($program);
  }

  ####################################################################
  #
  # The decode flag, '-d', is optional. Default value is 0.
  #
  ####################################################################

  my ($decode);

  $decode = (exists($options{'d'})) ? 1 : 0;

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
  # The reverse flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($reverse);

  $reverse = (exists($options{'r'})) ? 1 : 0;

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
  # The windows flag, '-w', is optional. Default value is 0.
  #
  ####################################################################

  my ($windows);

  $windows = (exists($options{'w'})) ? 1 : 0;

  ####################################################################
  #
  # If any arguments remain in the array, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($program);
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $header, $nIndex, $mIndex, $aIndex, $cIndex, $chIndex);

  if (!defined($header = <$fileHandle>))
  {
    print STDERR "$program: File='$filename' Error='Unable to read header.'\n";
    exit(2);
  }
  $header =~ s/[\r\n]+$//;
  @fields = split(/\|/, $header);
  for(my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] eq "name")
    {
      $nIndex = $i;
    }
    elsif ($fields[$i] eq "mtime")
    {
      $mIndex = $i;
    }
    elsif ($fields[$i] eq "atime")
    {
      $aIndex = $i;
    }
    elsif ($fields[$i] eq "ctime")
    {
      $cIndex = $i;
    }
    elsif ($fields[$i] eq "chtime")
    {
      $chIndex = $i;
    }
  }
  if ($windows)
  {
    if (!defined($nIndex) || !defined($mIndex) || !defined($aIndex) || !defined($cIndex) || !defined($chIndex))
    {
      print STDERR "$program: File='$filename' Error='Missing one or more required header fields.'\n";
      exit(2);
    }
  }
  else
  {
    if (!defined($nIndex) || !defined($mIndex) || !defined($aIndex) || !defined($cIndex))
    {
      print STDERR "$program: File='$filename' Error='Missing one or more required header fields.'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Open a pipe to the sort utility if in external sort mode.
  #
  ####################################################################

  if ($runMode =~ /^extsort$/)
  {
    my $command = "$sort -u -T $tmpDir";
    if ($reverse)
    {
      $command .= " -r";
    }
    if (!open(SH, "|$command"))
    {
      print STDERR "$program: Command='$command' Error='$!'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Process data.
  #
  ####################################################################

  my ($mac, $name, %unique);

  if ($windows)
  {
    while (my $line = <$fileHandle>)
    {
      $line =~ s/[\r\n]+$//;
      @fields = split(/\|/, $line);
      foreach my $time (@fields[$mIndex, $aIndex, $cIndex, $chIndex])
      {
        $mac  = ($time eq $fields[$mIndex]) ? "M" : ".";
        $mac .= ($time eq $fields[$aIndex]) ? "A" : ".";
        $mac .= ($time eq $fields[$cIndex]) ? "C" : ".";
        $mac .= ($time eq $fields[$chIndex]) ? "H" : ".";
        $name = ($decode) ? URLDecode($fields[$nIndex]) : $fields[$nIndex];
        $name =~ s/^"//;
        $name =~ s/"$//;
        if ($runMode =~ /^extsort$/)
        {
          print SH "$time|$mac|$name\n";
        }
        else
        {
          $unique{"$time|$mac|$name"}++;
        }
      }
    }
  }
  else
  {
    while (my $line = <$fileHandle>)
    {
      $line =~ s/[\r\n]+$//;
      @fields = split(/\|/, $line);
      foreach my $time (@fields[$mIndex, $aIndex, $cIndex])
      {
        $mac  = ($time eq $fields[$mIndex]) ? "M" : ".";
        $mac .= ($time eq $fields[$aIndex]) ? "A" : ".";
        $mac .= ($time eq $fields[$cIndex]) ? "C" : ".";
        $name = ($decode) ? URLDecode($fields[$nIndex]) : $fields[$nIndex];
        $name =~ s/^"//;
        $name =~ s/"$//;
        if ($runMode =~ /^extsort$/)
        {
          print SH "$time|$mac|$name\n";
        }
        else
        {
          $unique{"$time|$mac|$name"}++;
        }
      }
    }
  }

  ####################################################################
  #
  # Display sorted MAC list if in internal sort mode.
  #
  ####################################################################

  if ($runMode =~ /^intsort$/)
  {
    foreach my $key (($reverse) ? reverse(sort(keys(%unique))) : sort(keys(%unique)))
    {
      print $key,"\n";
    }
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  close($fileHandle);

  if ($runMode =~ /^extsort$/)
  {
    close(SH);
  }

  1;


######################################################################
#
# URLDecode
#
######################################################################

sub URLDecode
{
  my ($rawString) = @_;
  $rawString =~ s/\+/ /sg;
  $rawString =~ s/%([0-9a-fA-F]{2})/pack('C', hex($1))/seg;
  return $rawString;
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
  print STDERR "Usage: $program {-e|--extsort} [-d] [-r] [-s file] [-T dir] [-w] -f {file|-}\n";
  print STDERR "       $program {-i|--intsort} [-d] [-r] [-w] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-map2mac.pl - Create MAC/MACH timelines using FTimes map data

=head1 SYNOPSIS

B<ftimes-map2mac.pl> B<{-e|--extsort}> B<[-d]> B<[-r]> B<[-s file]> B<[-T dir]> B<[-w]> B<-f {file|-}>

B<ftimes-map2mac.pl> B<{-i|--intsort}> B<[-d]> B<[-r]> B<[-w]> B<-f {file|-}>

=head1 DESCRIPTION

This utility takes FTimes map data as input and produces a MAC or
MACH timeline as output. The letters M, A, and C, have the usual
meanings -- i.e. mtime, atime, and ctime. The H in MACH stands for
chtime which is NTFS specific. Output is written to stdout and has
the following format:

    datetime|mac/mach|name

Two sorting methods are supported: external and internal.  The
internal sorting method constructs timelines entirely within core
memory.  While this method is self-contained, its effectiveness
deteriorates as the amount of data increases.  The external sorting
method, on the other hand, requires an external sort utility, but
it is generally faster and can handle large data sets more effectively.
The external sorting method was designed to work with GNU sort.

=head1 OPTIONS

=over 4

=item B<-d>

Enables URL decoding of filenames.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin.

=item B<-r>

Causes timeline to be output in reverse time order.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH environment variable. Alternate sort utilities
must support the C<-r>, C<-T>, and C<-u> options. This program's
external sorting method was designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is /tmp.

=item B<-w>

Enables support for Windows NT/2K map data.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1)

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
