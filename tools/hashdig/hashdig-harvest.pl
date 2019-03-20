#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-harvest.pl,v 1.6 2003/03/27 02:57:56 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Harvest hashes from a one or more files.
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

  my ($program, %properties);

  $program = $properties{'program'} = "hashdig-harvest.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('c:o:qs:T:t:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The category flag, '-c', is optional. Default value is "U".
  #
  ####################################################################

  my ($category);

  $category = (exists($options{'c'})) ? uc($options{'c'}) : "U";

  if ($category !~ /^[KU]$/)
  {
    print STDERR "$program: Category='$category' Error='Invalid category.'\n";
    exit(2);
  }
  $properties{'category'} = $category;

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

  $properties{'beQuiet'} = (exists($options{'q'})) ? 1 : 0;

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
  # The fileType flag, '-t', is required.
  #
  ####################################################################

  my ($fileType, $processFile);

  $fileType = (exists($options{'t'})) ? uc($options{'t'}) : undef;

  if (!defined($fileType))
  {
    Usage($program);
  }

  if ($fileType =~ /^FTIMES$/)
  {
    $processFile = \&ProcessFTimesFile;
  }
  elsif ($fileType =~ /^(KG|KNOWNGOODS)$/)
  {
    $processFile = \&ProcessKnownGoodsFile;
  }
  elsif ($fileType =~ /^MD5$/)
  {
    $processFile = \&ProcessMD5File;
  }
  elsif ($fileType =~ /^MD5SUM|MD5DEEP$/)
  {
    $processFile = \&ProcessMD5SumFile;
  }
  elsif ($fileType =~ /^NSRL1$/)
  {
    $processFile = \&ProcessNSRL1File;
  }
  elsif ($fileType =~ /^NSRL2$/)
  {
    $processFile = \&ProcessNSRL2File;
  }
  elsif ($fileType =~ /^PLAIN$/)
  {
    $processFile = \&ProcessPlainFile;
  }
  elsif ($fileType =~ /^RPM$/)
  {
    $processFile = \&ProcessRPMFile;
  }
  else
  {
    print STDERR "$program: FileType='$fileType' Error='Invalid file type.'\n";
    exit(2);
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
  # Iterate over input files.
  #
  ####################################################################

  foreach my $inputFile (@ARGV)
  {
    &$processFile($inputFile, \%properties, \*SH);
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
# ProcessFTimesFile
#
######################################################################

sub ProcessFTimesFile
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $hashIndex, $header, $modeIndex);

  $header = <FH>;

  if (!defined($header))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @fields = split(/\|/, $header, -1);

  for (my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] =~ /^mode$/o)
    {
      $modeIndex = $i;
    }
    elsif ($fields[$i] =~ /^md5[\r\n]*$/o)
    {
      $hashIndex = $i;
    }
  }

  if (!defined($hashIndex))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    @fields = split(/\|/, $record, -1);

    $fields[$hashIndex] .= "";
    if ($fields[$hashIndex] =~ /^[0-9a-fA-F]{32}[\r\n]*$/o)
    {
      if (defined($modeIndex) && $fields[$modeIndex] =~ /^12[0-7]{4}$/o)
      {
        next; # Skip symlink hashes
      }
      print $sortHandle lc(substr($fields[$hashIndex],0,32)), "|", $$pProperties{'category'}, "\n";
    }
    elsif ($fields[$hashIndex] =~ /^(?:DIRECTORY|SPECIAL)[\r\n]*$/o)
    {
      next; # Skip directories and special files
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessKnownGoodsFile
#
######################################################################

sub ProcessKnownGoodsFile
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $hashIndex, $header);

  $header = "ID,FILENAME,MD5,SHA-1,SIZE,TYPE,PLATFORM,PACKAGE";

  @fields = reverse(split(/,/, $header, -1));

  for (my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] =~ /^MD5$/i)
    {
      $hashIndex = $i;
    }
  }

  if (!defined($hashIndex))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    @fields = (reverse(split(/,/, $record, -1)))[$hashIndex];

    $fields[0] .= "";
    if ($fields[0] =~ /^[0-9a-fA-F]{32}$/o)
    {
      print $sortHandle lc($fields[0]), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessMD5File
#
######################################################################

sub ProcessMD5File
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  # This format has no header.

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    if (my ($hash) = $record =~ /^MD5\s+\(.*\)\s+=\s+([0-9a-fA-F]{32})$/o)
    {
      print $sortHandle lc($hash), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessMD5SumFile
#
######################################################################

sub ProcessMD5SumFile
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  # This format has no header.

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    if (my ($hash) = $record =~ /^([0-9a-fA-F]{32})\s+.*\s*$/o)
    {
      print $sortHandle lc($hash), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessNSRL1File
#
######################################################################

sub ProcessNSRL1File
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $hashIndex, $header, $modeIndex);

  $header = <FH>;

  if (!defined($header))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @fields = reverse(split(/,/, $header, -1));

  for (my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] =~ /^"MD5"$/o)
    {
      $hashIndex = $i;
    }
  }

  if (!defined($hashIndex))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    my $hash = "" . (reverse(split(/,/, $record, -1)))[$hashIndex];

    if ($hash =~ /^"[0-9a-fA-F]{32}"$/o)
    {
      print $sortHandle lc(substr($hash,1,32)), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessNSRL2File
#
######################################################################

sub ProcessNSRL2File
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $hashIndex, $header, $modeIndex);

  $header = <FH>;

  if (!defined($header))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @fields = split(/,/, $header, -1);

  for (my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] =~ /^"MD5"$/o)
    {
      $hashIndex = $i;
    }
  }

  if (!defined($hashIndex))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    my $hash = "" . (split(/,/, $record, -1))[$hashIndex];

    if ($hash =~ /^"[0-9a-fA-F]{32}"$/o)
    {
      print $sortHandle lc(substr($hash,1,32)), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessPlainFile
#
######################################################################

sub ProcessPlainFile
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  # This format has no header.

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    if (my ($hash) = $record =~ /^([0-9a-fA-F]{32})$/o)
    {
      print $sortHandle lc($hash), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessRPMFile
#
######################################################################

sub ProcessRPMFile
{
  my ($filename, $pProperties, $sortHandle) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "<$filename"))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@fields, $hashIndex, $header, $modeIndex);

  $header = "path size mtime md5sum mode owner group isconfig isdoc rdev symlink";

  @fields = reverse(split(/ /, $header, -1));

  for (my $i = 0; $i < scalar(@fields); $i++)
  {
    if ($fields[$i] =~ /^md5sum$/o)
    {
      $hashIndex = $i;
    }
    if ($fields[$i] =~ /^mode$/o)
    {
      $modeIndex = $i;
    }
  }

  if (!defined($hashIndex) || !defined($modeIndex))
  {
    if (!$$pProperties{'beQuiet'})
    {
      $header =~ s/[\r\n]+$//;
      print STDERR "$$pProperties{'program'}: File='$filename' Header='$header' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while(my $record = <FH>)
  {
    @fields = reverse(split(/ /, $record, -1));

    $fields[$hashIndex] .= "";
    if ($fields[$hashIndex] =~ /^[0-9a-fA-F]{32}$/o)
    {
      print $sortHandle lc($fields[$hashIndex]), "|", $$pProperties{'category'}, "\n";
    }
    else
    {
      $fields[$modeIndex] .= "";
      if ($fields[$modeIndex] !~ /^010[0-7]{4}$/o)
      {
        next; # Skip anything that's not a regular file
      }
      if (!$$pProperties{'beQuiet'})
      {
        $record =~ s/[\r\n]+$//;
        print STDERR "$$pProperties{'program'}: Record='$record' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

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
  print STDERR "Usage: $program [-c {K|U}] [-q] [-s file] [-T dir] -t type -o {file|-} file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-harvest.pl - Harvest hashes from a one or more files

=head1 SYNOPSIS

B<hashdig-harvest.pl> B<[-c {K|U}]> B<[-q]> B<[-s file]> B<[-T dir]> B<-t type> B<-o {file|-}> B<file [file ...]>

=head1 DESCRIPTION

This utility extracts MD5 hashes from one or more input files of
the specified B<type>, tags them (see B<-c>), and writes them to
the specified output file (see B<-o>). Output is a sorted list of
hash/category pairs having the following format:

    hash|category

=head1 OPTIONS

=over 4

=item B<-c category>

Specifies the category, {K|U}, that is to be assigned to each hash.
Currently, the following categories are supported: known (K) and
unknown (U). The default category is unknown. The value for this
option is not case sensitive.

=item B<-o {file|-}>

Specifies the name of the output file. A value of '-' will cause
the program to write to stdout.

=item B<-q>

Don't report errors (i.e. be quiet) while processing files.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH environment variable. Alternate sort utilities
must support the C<-o>, C<-T> and C<-u> options. This program was
designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is /tmp.

=item B<-t type>

Specifies the type of files that are to be processed. All files
processed in a given invocation must be of the same type. Currently,
the following types are supported: FTIMES, KG|KNOWNGOODS, MD5,
MD5DEEP, MD5SUM, NSRL1, NSRL2, PLAIN, and RPM. The value for this
option is not case sensitive.

=back

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

ftimes(1), hashdig-make.pl, md5(1), md5sum(1), md5deep, rpm(8)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
