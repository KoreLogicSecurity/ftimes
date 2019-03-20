#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-bind.pl,v 1.4 2003/03/25 21:15:33 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Bind HashDig hashes to filenames.
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

  my ($program, %properties);

  $program = $properties{'program'} = "hashdig-bind.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('f:qrt:', \%options))
  {
    Usage($program);
  }

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
  # The beQuiet flag, '-q', is optional. Default value is 0.
  #
  ####################################################################

  $properties{'beQuiet'} = (exists($options{'q'})) ? 1 : 0;

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
  # The fileType flag, '-t', is required.
  #
  ####################################################################

  my ($bindFile, $fileType);

  $fileType = (exists($options{'t'})) ? uc($options{'t'}) : undef;

  if (!defined($fileType))
  {
    Usage($program);
  }

  if ($fileType =~ /^FTIMES$/)
  {
    $bindFile = \&BindFTimesFile;
  }
  elsif ($fileType =~ /^(KG|KNOWNGOODS)$/)
  {
    $bindFile = \&BindKnownGoodsFile;
  }
  elsif ($fileType =~ /^MD5$/)
  {
    $bindFile = \&BindMD5File;
  }
  elsif ($fileType =~ /^MD5DEEP$/)
  {
    $bindFile = \&BindMD5SumFile;
  }
  elsif ($fileType =~ /^MD5SUM$/)
  {
    $bindFile = \&BindMD5SumFile;
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
  # Read hashes and create known and unknown hash lists.
  #
  ####################################################################

  my (%hashKList, %hashUList);

  while (my $record = <$fileHandle>)
  {
    $record =~ s/[\r\n]+$//;
    if (my @fields = $record =~ /$recordRegex/o)
    {
      $fields[$hIndex] = lc($fields[$hIndex]);
      if ($fields[$cIndex] eq "K")
      {
        $hashKList{$fields[$hIndex]}++;
      }
      else
      {
        $hashUList{$fields[$hIndex]}++;
      }
    }
    else
    {
      print STDERR "$program: File='$filename' Record='$record' Error='Record did not parse properly.'\n";
      exit(2);
    }
  }
  close($fileHandle);

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  foreach my $inputFile (@ARGV)
  {
    &$bindFile($inputFile, \%properties);
  }

  ####################################################################
  #
  # Cleanup and go home.
  #
  ####################################################################

  1;


######################################################################
#
# BindFTimesFile
#
######################################################################

sub BindFTimesFile
{
  my ($filename, $pProperties) = @_;

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

  my (@fields, $hashIndex, $header, $modeIndex, $nameIndex);

  $header = <FH>;
  $header =~ s/[\r\n]+$//;

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
    elsif ($fields[$i] =~ /^md5$/o)
    {
      $hashIndex = $i;
    }
    elsif ($fields[$i] =~ /^name$/o)
    {
      $nameIndex = $i;
    }
  }

  if (!defined($hashIndex) || !defined($nameIndex))
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
  # Open output files.
  #
  ####################################################################

  my (@handles, %handleList);

  @handles = ("a", "d", "i", "k", "s", "u");

  if (!defined(OpenFileHandles($filename, \@handles, \%handleList)))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='Unable to create output files.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($category, $categoryHandle, $combinedHandle, $hash);

  $combinedHandle = $handleList{'a'};

  while(my $record = <FH>)
  {
    $record =~ s/[\r\n]+$//;
    @fields = split(/\|/, $record, -1);

    if (defined($fields[$hashIndex]) && defined($fields[$nameIndex]))
    {
      $hash = lc($fields[$hashIndex]);

      if ($hashUList{$hash})
      {
        $category = "U";
        $categoryHandle = $handleList{'u'};
      }
      elsif ($hashKList{$hash})
      {
        $category = "K";
        $categoryHandle = $handleList{'k'};
      }
      elsif ($hash =~ /^DIRECTORY$/oi)
      {
        $category = "D";
        $categoryHandle = $handleList{'d'};
        $hash = "DIRECTORY";
      }
      elsif (defined($modeIndex) && $fields[$modeIndex] =~ /^12[0-7]{4}$/o)
      {
        $category = "S";
        $categoryHandle = $handleList{'s'};
      }
      elsif ($hash =~ /^SPECIAL$/oi)
      {
        $category = "S";
        $categoryHandle = $handleList{'s'};
        $hash = "SPECIAL";
      }
      else
      {
        $category = "I";
        $categoryHandle = $handleList{'i'};
      }

      $fields[$nameIndex] =~ s/^"(.*)"$/$1/; # Remove double quotes around the name.

      print $combinedHandle "$category|$hash|$fields[$nameIndex]\n";
      print $categoryHandle "$category|$hash|$fields[$nameIndex]\n";
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

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $handle (keys(%handleList))
  {
    close($handleList{$handle});
  }
  close(FH);

  return 1;
}


######################################################################
#
# BindKnownGoodsFile
#
######################################################################

sub BindKnownGoodsFile
{
  my ($filename, $pProperties) = @_;

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

  my ($fieldCount, @fields, $hashIndex, $header, $nameIndex);

  $header = "ID,FILENAME,MD5,SHA-1,SIZE,TYPE,PLATFORM,PACKAGE";

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
    if ($fields[$i] =~ /^FILENAME$/o)
    {
      $nameIndex = $i;
    }
    elsif ($fields[$i] =~ /^MD5$/o)
    {
      $hashIndex = $i;
    }
  }
  $fieldCount = scalar(@fields);

  if (!defined($hashIndex) || !defined($nameIndex))
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
  # Open output files.
  #
  ####################################################################

  my (@handles, %handleList);

  @handles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($filename, \@handles, \%handleList)))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='Unable to create output files.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($category, $categoryHandle, $combinedHandle, $count, $hash, $name);

  $combinedHandle = $handleList{'a'};

  while(my $record = <FH>)
  {
    @fields = split(/,/, $record, -1);
    $count = scalar(@fields);

    if (defined($fields[$hashIndex]) && defined($fields[$nameIndex]) && $count >= $fieldCount)
    {
      if ($count > $fieldCount)
      {
        my $lIndex = $nameIndex;
        my $hIndex = $count - $fieldCount + $hashIndex - 1;
        $name = join(',', @fields[$lIndex..$hIndex]);
        $hash = lc($fields[$hIndex + 1]);
      }
      else
      {
        $name = $fields[$nameIndex];
        $hash = lc($fields[$hashIndex]);
      }

      if ($hashKList{$hash})
      {
        $category = "K";
        $categoryHandle = $handleList{'k'};
      }
      elsif ($hashUList{$hash})
      {
        $category = "U";
        $categoryHandle = $handleList{'u'};
      }
      else
      {
        $category = "I";
        $categoryHandle = $handleList{'i'};
      }

      print $combinedHandle "$category|$hash|$name\n";
      print $categoryHandle "$category|$hash|$name\n";
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

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $handle (keys(%handleList))
  {
    close($handleList{$handle});
  }
  close(FH);

  return 1;
}


######################################################################
#
# BindMD5File
#
######################################################################

sub BindMD5File
{
  my ($filename, $pProperties) = @_;

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
  # Open output files.
  #
  ####################################################################

  my (@handles, %handleList);

  @handles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($filename, \@handles, \%handleList)))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='Unable to create output files.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($category, $categoryHandle, $combinedHandle, $hash, $name);

  $combinedHandle = $handleList{'a'};

  while(my $record = <FH>)
  {
    if (($name, $hash) = $record =~ /^MD5\s+\((.*)\)\s+=\s+([0-9a-fA-F]{32})$/o)
    {
      $hash = lc($hash);

      if ($hashKList{$hash})
      {
        $category = "K";
        $categoryHandle = $handleList{'k'};
      }
      elsif ($hashUList{$hash})
      {
        $category = "U";
        $categoryHandle = $handleList{'u'};
      }
      else
      {
        $category = "I";
        $categoryHandle = $handleList{'i'};
      }

      print $combinedHandle "$category|$hash|$name\n";
      print $categoryHandle "$category|$hash|$name\n";
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

  return 1;
}


######################################################################
#
# BindMD5SumFile
#
######################################################################

sub BindMD5SumFile
{
  my ($filename, $pProperties) = @_;

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
  # Open output files.
  #
  ####################################################################

  my (@handles, %handleList);

  @handles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($filename, \@handles, \%handleList)))
  {
    if (!$$pProperties{'beQuiet'})
    {
      print STDERR "$$pProperties{'program'}: File='$filename' Error='Unable to create output files.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($category, $categoryHandle, $combinedHandle, $hash, $name);

  $combinedHandle = $handleList{'a'};

  while(my $record = <FH>)
  {
    if (($hash, $name) = $record =~ /^([0-9a-fA-F]{32})\s+(.*)\s*$/o)
    {
      $hash = lc($hash);

      if ($hashKList{$hash})
      {
        $category = "K";
        $categoryHandle = $handleList{'k'};
      }
      elsif ($hashUList{$hash})
      {
        $category = "U";
        $categoryHandle = $handleList{'u'};
      }
      else
      {
        $category = "I";
        $categoryHandle = $handleList{'i'};
      }

      print $combinedHandle "$category|$hash|$name\n";
      print $categoryHandle "$category|$hash|$name\n";
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

  return 1;
}


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

  $outBase = $name . ".bound";

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
  print STDERR "Usage: $program [-q] [-r] -t type -f {file|-} file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-bind.pl - Bind HashDig hashes to filenames

=head1 SYNOPSIS

B<hashdig-bind.pl> B<[-q]> B<[-r]> B<-t type> B<-f {file|-}> B<file [file ...]>

=head1 DESCRIPTION

This utility binds HashDig hashes to filenames in subject data.
Depending on the file format (see B<-t> option), one or more of the
following output files will be created in the current working
directory: all, directory, indeterminate, known, special, and
unknown. These files will have the following format:

    <filename>.bound.{a|d|i|k|s|u}

The 'all' file is the sum of the other output files.

=head1 OPTIONS

=over 4

=item B<-f {file|-}>

Specifies the name of a HashDig hash file to use. A value of '-'
will cause the program to read from stdin.

=item B<-q>

Don't report errors (i.e. be quiet) while processing files.

=item B<-r>

Accept records in the reverse HashDig format (i.e. category|hash).

=item B<-t type>

Specifies the type of files that are to be processed. All files
processed in a given invocation must be of the same type. Currently,
the following types are supported: FTIMES, KG|KNOWNGOODS, MD5,
MD5DEEP, and MD5SUM. The value for this option is not case sensitive.

=back

=head1 CAVEATS

This script attempts to keep all hash/category information in memory.
When all available memory has been exhausted, Perl will probably
force the script to abort. In some cases, it can create a core file.

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

hashdig-dump.pl

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
