#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-bind.pl,v 1.27 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Bind resolved hashes to filenames.
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

  my ($sProgram, %hProperties);

  $sProgram = $hProperties{'program'} = basename(__FILE__);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('a:d:f:h:n:qrt:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # A HashType, '-a', is optional. The 'a' is short for algorithm.
  #
  ####################################################################

  my ($sHashType);

  $sHashType = (exists($hOptions{'a'})) ? uc($hOptions{'a'}) : (defined($ENV{'HASH_TYPE'})) ? uc($ENV{'HASH_TYPE'}) : "MD5";

  if ($sHashType !~ /^(MD5|SHA1|SHA256)$/)
  {
    print STDERR "$sProgram: HashType='$sHashType' Error='Invalid hash type.'\n";
    exit(2);
  }
  $hProperties{'HashType'} = $sHashType;
  $hProperties{'HashTypeSupported'} = 1; # Be optimistic.

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
  # A Delimiter, '-d', optional. To make the delimiter be a tab, the
  # user must literally specify "\t" on the command line.
  #
  ####################################################################

  my ($sDelimiter);

  $sDelimiter = (exists($hOptions{'d'})) ? $hOptions{'d'} : "|";

  if ($sDelimiter !~ /^(\\t|[ ,;|])$/)
  {
    print STDERR "$sProgram: Delimiter='$sDelimiter' Error='Invalid delimiter.'\n";
    exit(2);
  }
  $hProperties{'Delimiter'} = $sDelimiter;

  ####################################################################
  #
  # A HashField, '-h', is optional.
  #
  ####################################################################

  $hProperties{'HashField'} = (exists($hOptions{'h'})) ? $hOptions{'h'} : "hash";

  ####################################################################
  #
  # A NameField, '-n', is optional.
  #
  ####################################################################

  $hProperties{'NameField'} = (exists($hOptions{'n'})) ? $hOptions{'n'} : "name";

  ####################################################################
  #
  # The BeQuiet flag, '-q', is optional.
  #
  ####################################################################

  $hProperties{'BeQuiet'} = (exists($hOptions{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The ReverseFormat flag, '-r', is optional.
  #
  ####################################################################

  my ($sCIndex, $sHIndex, $sRecordRegex, $sReverseFormat);

  $sReverseFormat = (exists($hOptions{'r'})) ? 1 : 0;

  if ($sReverseFormat)
  {
    if ($sHashType =~ /^SHA1$/)
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{40}));
    }
    elsif ($sHashType =~ /^SHA256$/)
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{64}));
    }
    else # MD5
    {
      $sRecordRegex = qq(([KU])\\|([0-9a-fA-F]{32}));
    }
    $sCIndex = 0;
    $sHIndex = 1;
  }
  else
  {
    if ($sHashType =~ /^SHA1$/)
    {
      $sRecordRegex = qq(([0-9a-fA-F]{40})\\|([KU]));
    }
    elsif ($sHashType =~ /^SHA256$/)
    {
      $sRecordRegex = qq(([0-9a-fA-F]{64})\\|([KU]));
    }
    else # MD5
    {
      $sRecordRegex = qq(([0-9a-fA-F]{32})\\|([KU]));
    }
    $sCIndex = 1;
    $sHIndex = 0;
  }

  ####################################################################
  #
  # The FileType flag, '-t', is required.
  #
  ####################################################################

  my ($sBindFile, $sFileType);

  $sFileType = (exists($hOptions{'t'})) ? uc($hOptions{'t'}) : undef;

  if (!defined($sFileType))
  {
    Usage($sProgram);
  }

  if ($sFileType =~ /^FTIMES$/)
  {
    if ($hProperties{'HashType'} eq "SHA1")
    {
      $hProperties{'HashField'} = "sha1";
    }
    elsif ($hProperties{'HashType'} eq "SHA256")
    {
      $hProperties{'HashField'} = "sha256";
    }
    else
    {
      $hProperties{'HashField'} = "md5";
    }
    $sBindFile = \&BindFTimesFile;
  }
  elsif ($sFileType =~ /^FTK$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA Hash" : "MD5 Hash";
    $sBindFile = \&BindFTKFile;
  }
  elsif ($sFileType =~ /^GENERIC$/)
  {
    $sBindFile = \&BindGenericFile;
  }
  elsif ($sFileType =~ /^(KG|KNOWNGOODS)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA-1" : "MD5";
    $sBindFile = \&BindKnownGoodsFile;
  }
  elsif ($sFileType =~ /^MD5$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sBindFile = \&BindMD5File;
  }
  elsif ($sFileType =~ /^(MD5SUM|MD5DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sBindFile = \&BindMD5SumFile;
  }
  elsif ($sFileType =~ /^OPENSSL$/)
  {
    $sBindFile = \&BindMD5File;
  }
  elsif ($sFileType =~ /^SHA1$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA1");
    $sBindFile = \&BindMD5File;
  }
  elsif ($sFileType =~ /^(SHA1SUM|SHA1DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA1");
    $sBindFile = \&BindMD5SumFile;
  }
  elsif ($sFileType =~ /^SHA256$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA256");
    $sBindFile = \&BindMD5File;
  }
  elsif ($sFileType =~ /^(SHA256SUM|SHA256DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA256");
    $sBindFile = \&BindMD5SumFile;
  }
  else
  {
    print STDERR "$sProgram: FileType='$sFileType' Error='Invalid file type.'\n";
    exit(2);
  }

  if (!$hProperties{'HashTypeSupported'})
  {
    print STDERR "$sProgram: FileType='$sFileType' Error='The specified hash type ($hProperties{'HashType'}) is not supported for this file type.'\n";
    exit(2);
  }

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
  # Read hashes and create known and unknown hash lists.
  #
  ####################################################################

  my (%hHashKList, %hHashUList);

  while (my $sRecord = <$sFileHandle>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my @aFields = $sRecord =~ /^$sRecordRegex$/o)
    {
      $aFields[$sHIndex] = lc($aFields[$sHIndex]);
      if ($aFields[$sCIndex] eq "K")
      {
        $hHashKList{$aFields[$sHIndex]}++;
      }
      else
      {
        $hHashUList{$aFields[$sHIndex]}++;
      }
    }
    else
    {
      print STDERR "$sProgram: File='$sFilename' Record='$sRecord' Error='Record did not parse properly.'\n";
      exit(2);
    }
  }
  close($sFileHandle);

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  foreach my $sInputFile (@ARGV)
  {
    &$sBindFile($sInputFile, \%hProperties);
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
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex, $sNameIndex);

  $sHeader = <FH>;
  if (defined($sHeader))
  {
    $sHeader =~ s/[\r\n]+$//;
    @aFields = split(/\|/, $sHeader, -1);
    for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
    {
      if ($aFields[$sIndex] =~ /^mode$/o)
      {
        $sModeIndex = $sIndex;
      }
      elsif ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
      {
        $sHashIndex = $sIndex;
      }
      elsif ($aFields[$sIndex] =~ /^name$/o)
      {
        $sNameIndex = $sIndex;
      }
    }

    if (!defined($sHashIndex) || !defined($sNameIndex))
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
      }
      close(FH);
      return undef;
    }
  }
  else
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='' Error='Header did not parse properly.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Open output files.
  #
  ####################################################################

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "d", "i", "k", "l", "s", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sHash);

  $sCombinedHandle = $hHandleList{'a'};

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/\|/, $sRecord, -1);
    if (defined($aFields[$sHashIndex]) && defined($aFields[$sNameIndex]))
    {
      $sHash = lc($aFields[$sHashIndex]);

      if ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      elsif ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      elsif ($sHash =~ /^directory$/o)
      {
        $sCategory = "D";
        $sCategoryHandle = $hHandleList{'d'};
        $sHash = uc($sHash);
      }
      elsif ($sHash =~ /^symlink$/o)
      {
        $sCategory = "L";
        $sCategoryHandle = $hHandleList{'l'};
        $sHash = uc($sHash);
      }
      elsif (defined($sModeIndex) && $aFields[$sModeIndex] =~ /^12[0-7]{4}$/o)
      {
        $sCategory = "L";
        $sCategoryHandle = $hHandleList{'l'};
      }
      elsif ($sHash =~ /^special$/o)
      {
        $sCategory = "S";
        $sCategoryHandle = $hHandleList{'s'};
        $sHash = uc($sHash);
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      $aFields[$sNameIndex] =~ s/^"(.*)"$/$1/; # Remove double quotes around the name.

      print $sCombinedHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
      print $sCategoryHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
  }
  close(FH);

  return 1;
}


######################################################################
#
# BindFTKFile
#
######################################################################

sub BindFTKFile
{
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex, $sNameIndex);

  $sHeader = <FH>;
  if (defined($sHeader))
  {
    $sHeader =~ s/[\r\n]+$//;
    @aFields = split(/\t/, $sHeader, -1);
    for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
    {
      if ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
      {
        $sHashIndex = $sIndex;
      }
      elsif ($aFields[$sIndex] =~ /^Full Path$/o)
      {
        $sNameIndex = $sIndex;
      }
    }

    if (!defined($sHashIndex) || !defined($sNameIndex))
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
      }
      close(FH);
      return undef;
    }
  }
  else
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='' Error='Header did not parse properly.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Open output files.
  #
  ####################################################################

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sHash);

  $sCombinedHandle = $hHandleList{'a'};

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/\t/, $sRecord, -1);
    if (defined($aFields[$sHashIndex]) && defined($aFields[$sNameIndex]))
    {
      $sHash = lc($aFields[$sHashIndex]);

      if ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      elsif ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      print $sCombinedHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
      print $sCategoryHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
  }
  close(FH);

  return 1;
}


######################################################################
#
# BindGenericFile
#
######################################################################

sub BindGenericFile
{
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex, $sNameIndex);

  $sHeader = <FH>;
  if (defined($sHeader))
  {
    $sHeader =~ s/[\r\n]+$//;
    @aFields = split(/[$$phProperties{'Delimiter'}]/, $sHeader, -1);
    for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
    {
      if ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
      {
        $sHashIndex = $sIndex;
      }
      elsif ($aFields[$sIndex] =~ /^$$phProperties{'NameField'}$/o)
      {
        $sNameIndex = $sIndex;
      }
    }

    if (!defined($sHashIndex) || !defined($sNameIndex))
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
      }
      close(FH);
      return undef;
    }
  }
  else
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='' Error='Header did not parse properly.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Open output files.
  #
  ####################################################################

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sHash);

  $sCombinedHandle = $hHandleList{'a'};

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/[$$phProperties{'Delimiter'}]/, $sRecord, -1);
    if (defined($aFields[$sHashIndex]) && defined($aFields[$sNameIndex]))
    {
      $sHash = lc($aFields[$sHashIndex]);

      if ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      elsif ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      print $sCombinedHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
      print $sCategoryHandle "$sCategory|$sHash|$aFields[$sNameIndex]\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
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
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my ($sFieldCount, @aFields, $sHashIndex, $sHeader, $sNameIndex);

  $sHeader = "ID,FILENAME,MD5,SHA-1,SIZE,TYPE,PLATFORM,PACKAGE";

  if (defined($sHeader))
  {
    $sHeader =~ s/[\r\n]+$//;
    @aFields = split(/,/, $sHeader, -1);
    for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
    {
      if ($aFields[$sIndex] =~ /^FILENAME$/o)
      {
        $sNameIndex = $sIndex;
      }
      elsif ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
      {
        $sHashIndex = $sIndex;
      }
    }
    $sFieldCount = scalar(@aFields);

    if (!defined($sHashIndex) || !defined($sNameIndex))
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
      }
      close(FH);
      return undef;
    }
  }
  else
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='' Error='Header did not parse properly.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Open output files.
  #
  ####################################################################

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sCount, $sHash, $sName);

  $sCombinedHandle = $hHandleList{'a'};

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/,/, $sRecord, -1);
    $sCount = scalar(@aFields);
    if (defined($aFields[$sHashIndex]) && defined($aFields[$sNameIndex]) && $sCount >= $sFieldCount)
    {
      if ($sCount > $sFieldCount)
      {
        my $sLIndex = $sNameIndex;
        my $sHIndex = $sCount - $sFieldCount + $sHashIndex - 1;
        $sName = join(',', @aFields[$sLIndex..$sHIndex]);
        $sHash = lc($aFields[$sHIndex + 1]);
      }
      else
      {
        $sName = $aFields[$sNameIndex];
        $sHash = lc($aFields[$sHashIndex]);
      }

      if ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      elsif ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      print $sCombinedHandle "$sCategory|$sHash|$sName\n";
      print $sCategoryHandle "$sCategory|$sHash|$sName\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
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
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
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

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sHashRegex);

  $sCombinedHandle = $hHandleList{'a'};

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(SHA1\\s*\\((.*)\\)\\s*=\\s+([0-9a-fA-F]{40}));
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(SHA256\\s*\\((.*)\\)\\s*=\\s+([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(MD5\\s*\\((.*)\\)\\s*=\\s+([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sName, $sHash) = $sRecord =~ /^$sHashRegex$/o)
    {
      $sHash = lc($sHash);

      if ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      elsif ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      print $sCombinedHandle "$sCategory|$sHash|$sName\n";
      print $sCategoryHandle "$sCategory|$sHash|$sName\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
  }
  close(FH);

  return 1;
}


######################################################################
#
# BindMD5SumFile
#
######################################################################

sub BindMD5SumFile
{
  my ($sFilename, $phProperties) = @_;

  ####################################################################
  #
  # Open input file.
  #
  ####################################################################

  if (!open(FH, "< $sFilename"))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='$!'\n";
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

  my (@aHandles, %hHandleList);

  @aHandles = ("a", "i", "k", "u");

  if (!defined(OpenFileHandles($sFilename, \@aHandles, \%hHandleList)))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Error='Unable to create one or more output files.'\n";
    }
    close(FH);
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sCategory, $sCategoryHandle, $sCombinedHandle, $sHashRegex);

  $sCombinedHandle = $hHandleList{'a'};

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40})\\s+(.*)\\s*);
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64})\\s+(.*)\\s*);
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32})\\s+(.*)\\s*);
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash, $sName) = $sRecord =~ /^$sHashRegex$/o)
    {
      $sHash = lc($sHash);

      if ($hHashKList{$sHash})
      {
        $sCategory = "K";
        $sCategoryHandle = $hHandleList{'k'};
      }
      elsif ($hHashUList{$sHash})
      {
        $sCategory = "U";
        $sCategoryHandle = $hHandleList{'u'};
      }
      else
      {
        $sCategory = "I";
        $sCategoryHandle = $hHandleList{'i'};
      }

      print $sCombinedHandle "$sCategory|$sHash|$sName\n";
      print $sCategoryHandle "$sCategory|$sHash|$sName\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  ####################################################################
  #
  # Cleanup.
  #
  ####################################################################

  foreach my $sHandle (keys(%hHandleList))
  {
    close($hHandleList{$sHandle});
  }
  close(FH);

  return 1;
}


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

  $sOutBase = $sName . ".bound";

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
      if (exists($$phHandleList{$sExtension}) && defined($$phHandleList{$sExtension}))
      {
        close($$phHandleList{$sExtension});
      }
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
  print STDERR "Usage: $sProgram [-qr] [-a hash-type] [-d delimiter] [-h hash-field] [-n name-field] -t file-type -f {hashdig-file|-} file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-bind.pl - Bind resolved hashes to filenames

=head1 SYNOPSIS

B<hashdig-bind.pl> B<[-qr]> B<[-a hash-type]> B<[-d delimiter]> B<[-h hash-field]> B<[-n name-field]> B<-t file-type> B<-f {hashdig-file|-}> B<file [file ...]>

=head1 DESCRIPTION

This utility binds resolved hashes to filenames. The source of
resolved hashes is a HashDig file or stdin, and the source of
filenames is one or more subject files. Depending on the type of
subject files (see B<-t> option), one or more of the following
output files will be created in the current working directory:
(a)ll, (d)irectory, (i)ndeterminate, (k)nown, symbolic (l)ink,
(s)pecial, and (u)nknown. These files will have the following
format:

    <filename>.bound.{a|d|i|k|l|s|u}

The 'all' file is the sum of the other output files.

=head1 OPTIONS

=over 4

=item B<-a hash-type>

Specifies the type of hashes that are to be bound. Currently, the
following hash types (or algorithms) are supported: 'MD5', 'SHA1', and
'SHA256'. The default hash type is that specified by the HASH_TYPE
environment variable or 'MD5' if HASH_TYPE is not set. The value for
this option is not case sensitive.

=item B<-d delimiter>

Specifies the input field delimiter. This option is ignored unless
used in conjunction with the GENERIC data type. Valid delimiters
include the following characters: tab '\t', space ' ', comma ',',
semi-colon ';', and pipe '|'. The default delimiter is a pipe. Note
that parse errors are likely to occur if the specified delimiter
appears in any of the field values.

=item B<-h hash-field>

Specifies the name of the field that contains the hash value. This
option is ignored unless used in conjunction with the GENERIC data
type. The default value for this option is "hash".

=item B<-n name-field>

Specifies the name of the field that contains the name value. This
option is ignored unless used in conjunction with the GENERIC data
type. The default value for this option is "name".

=item B<-f {hashdig-file|-}>

Specifies the name of a HashDig file to use as the source of hashes.
A value of '-' will cause the program to read from stdin. HashDig
files have the following format:

    hash|category

=item B<-q>

Don't report errors (i.e., be quiet) while processing files.

=item B<-r>

Accept HashDig records in reverse format (i.e., category|hash).

=item B<-t file-type>

Specifies the type of subject files that are to be processed. All
files processed in a given invocation must be of the same type.
Currently, the following types are supported: FTIMES, FTK, GENERIC,
KG|KNOWNGOODS, MD5, MD5DEEP, MD5SUM, OPENSSL, SHA1, SHA1DEEP, SHA1SUM,
SHA256, SHA256DEEP, and SHA256SUM. The value for this option is not
case sensitive.

=back

=head1 CAVEATS

This utility attempts to load all hash/category information into a
pair associative arrays. When all available memory has been exhausted,
Perl will probably force the script to abort. In extreme cases,
this can produce a core file.

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), hashdig-dump(1), hashdig-harvest(1), hashdig-harvest-sunsolve(1), md5(1), md5sum(1), md5deep(1), openssl(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
