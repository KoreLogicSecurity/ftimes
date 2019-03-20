#!/usr/bin/perl -w
######################################################################
#
# $Id: hashdig-harvest.pl,v 1.33 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2003-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Harvest hashes from a one or more files.
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

  my ($sProgram, %hProperties);

  $sProgram = $hProperties{'program'} = basename(__FILE__);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('a:c:d:h:o:qs:T:t:', \%hOptions))
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
  # The Category flag, '-c', is optional.
  #
  ####################################################################

  my ($sCategory);

  $sCategory = (exists($hOptions{'c'})) ? uc($hOptions{'c'}) : "U";

  if ($sCategory !~ /^[KU]$/)
  {
    print STDERR "$sProgram: Category='$sCategory' Error='Invalid category.'\n";
    exit(2);
  }
  $hProperties{'category'} = $sCategory;

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

  $hProperties{'BeQuiet'} = (exists($hOptions{'q'})) ? 1 : 0;

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
  # The FileType flag, '-t', is required.
  #
  ####################################################################

  my ($sFileType, $sProcessFile);

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
    $sProcessFile = \&ProcessFTimesFile;
  }
  elsif ($sFileType =~ /^FTK$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA Hash" : "MD5 Hash";
    $sProcessFile = \&ProcessFTKFile;
  }
  elsif ($sFileType =~ /^GENERIC$/)
  {
    $sProcessFile = \&ProcessGenericFile;
  }
  elsif ($sFileType =~ /^(HK|HASHKEEPER)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sProcessFile = \&ProcessHashKeeperFile;
  }
  elsif ($sFileType =~ /^(KG|KNOWNGOODS)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA-1" : "MD5";
    $sProcessFile = \&ProcessKnownGoodsFile;
  }
  elsif ($sFileType =~ /^MD5$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sProcessFile = \&ProcessMD5File;
  }
  elsif ($sFileType =~ /^(MD5SUM|MD5DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sProcessFile = \&ProcessMD5SumFile;
  }
  elsif ($sFileType =~ /^NSRL1$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA-1" : "MD5";
    $sProcessFile = \&ProcessNSRL1File;
  }
  elsif ($sFileType =~ /^NSRL2$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} =~ /^(MD5|SHA1)$/);
    $hProperties{'HashField'} = ($hProperties{'HashType'} eq "SHA1") ? "SHA-1" : "MD5";
    $sProcessFile = \&ProcessNSRL2File;
  }
  elsif ($sFileType =~ /^OPENSSL$/)
  {
    $sProcessFile = \&ProcessMD5File;
  }
  elsif ($sFileType =~ /^PLAIN$/)
  {
    $sProcessFile = \&ProcessPlainFile;
  }
  elsif ($sFileType =~ /^RPM$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "MD5");
    $sProcessFile = \&ProcessRPMFile;
  }
  elsif ($sFileType =~ /^SHA1$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA1");
    $sProcessFile = \&ProcessMD5File;
  }
  elsif ($sFileType =~ /^(SHA1SUM|SHA1DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA1");
    $sProcessFile = \&ProcessMD5SumFile;
  }
  elsif ($sFileType =~ /^SHA256$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA256");
    $sProcessFile = \&ProcessMD5File;
  }
  elsif ($sFileType =~ /^(SHA256SUM|SHA256DEEP)$/)
  {
    $hProperties{'HashTypeSupported'} = 0 unless ($hProperties{'HashType'} eq "SHA256");
    $sProcessFile = \&ProcessMD5SumFile;
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
  # Iterate over input files.
  #
  ####################################################################

  foreach my $sInputFile (@ARGV)
  {
    &$sProcessFile($sInputFile, \%hProperties, \*SH);
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

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
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/\|/, $sRecord, -1);
    $aFields[$sHashIndex] .= ""; # Convert undefined values into empty values.
    if (my ($sHash) = $aFields[$sHashIndex] =~ /^$sHashRegex$/o)
    {
      if (defined($sModeIndex) && (($aFields[$sModeIndex] =~ /^12[0-7]{4}$/o) || ($aFields[$sModeIndex] =~ /^4[0-7]{4}$/o)))
      {
        next; # Skip directories and symbolic links.
      }
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    elsif ($aFields[$sHashIndex] =~ /^(?:DIRECTORY|SPECIAL|SYMLINK)$/o)
    {
      next; # Skip directories, special files, and symbolic links.
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessFTKFile
#
######################################################################

sub ProcessFTKFile
{
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @aFields = split(/\t/, $sHeader, -1);

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
    {
      $sHashIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/\t/, $sRecord, -1);
    $aFields[$sHashIndex] .= "";
    if (my ($sHash) = $aFields[$sHashIndex] =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessGenericFile
#
######################################################################

sub ProcessGenericFile
{
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @aFields = split(/[$$phProperties{'Delimiter'}]/, $sHeader, -1);

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
    {
      $sHashIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = split(/[$$phProperties{'Delimiter'}]/, $sRecord, -1);
    $aFields[$sHashIndex] .= "";
    if (my ($sHash) = $aFields[$sHashIndex] =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
      }
    }
  }

  close(FH);

  return 1;
}


######################################################################
#
# ProcessHashKeeperFile
#
######################################################################

sub ProcessHashKeeperFile
{
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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
  # The header for this format is useless for field-index parsing
  # due to potential delimiter collision issues. The hash field is
  # bound to the left by the file_name and directory fields and to
  # the right by the comments field. All of these fields could
  # contain embedded commas, so using a comma as the delimiter will
  # not work without extra processing. Therefore, we'll impose the
  # following parse rules:
  #
  #  - The file_id and hashset_id fields must be defined and contain
  #    one or more decimal characters.
  #
  #  - The file_name field must be defined, contain one or more
  #    characters (except double quotes), and be enclosed by double
  #    quotes.
  #
  #  - The directory field may be empty, but if it is defined, then
  #    it must contain one or more characters (except double quotes)
  #    and be enclosed by double quotes.
  #
  #  - The hash field must be defined, contain 32 hex characters,
  #    and be enclosed by double quotes.
  #
  #  - All remaining fields are ignored.
  #
  # This amounts to the following regular expression:
  #
  #  ^\d+,\d+,"[^"]+",(?:|"[^"]+"),"([0-9A-Fa-f]{32})",.*$
  #
  ####################################################################

  ####################################################################
  #
  # Process header.
  #
  ####################################################################

  my ($sHeader);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  if ($sHeader !~ /^"file_id","hashset_id","file_name","directory","hash","file_size","date_modified","time_modified","time_zone","comments","date_accessed","time_accessed"$/)
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash) = $sRecord =~ /^\d+,\d+,"[^"]+",(?:|"[^"]+"),"([0-9A-Fa-f]{32})",.*$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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
  # Process header. This type doesn't carry its own header, so create
  # a fake one and parse it instead.
  #
  ####################################################################

  my (@aFields, $sHashIndex, $sHeader);

  $sHeader = "ID,FILENAME,MD5,SHA-1,SIZE,TYPE,PLATFORM,PACKAGE";

  @aFields = reverse(split(/,/, $sHeader, -1));

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^$$phProperties{'HashField'}$/o)
    {
      $sHashIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = (reverse(split(/,/, $sRecord, -1)))[$sHashIndex];
    $aFields[0] .= ""; # Convert undefined values into empty values.
    if (my ($sHash) = $aFields[0] =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(SHA1\\s*\\(.*\\)\\s*=\\s+([0-9a-fA-F]{40}));
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(SHA256\\s*\\(.*\\)\\s*=\\s+([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(MD5\\s*\\(.*\\)\\s*=\\s+([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash) = $sRecord =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40})\\s+.*\\s*);
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64})\\s+.*\\s*);
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32})\\s+.*\\s*);
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash) = $sRecord =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    @aFields = split(/,/, $sHeader, -1);
  }
  else # MD5
  {
    @aFields = reverse(split(/,/, $sHeader, -1));
  }

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^"$$phProperties{'HashField'}"$/o)
    {
      $sHashIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq("([0-9a-fA-F]{40})");
  }
  else # MD5
  {
    $sHashRegex = qq("([0-9a-fA-F]{32})");
  }

  while (my $sRecord = <FH>)
  {
    my ($sNsrlHash);
    $sRecord =~ s/[\r\n]+$//;
    if ($$phProperties{'HashType'} =~ /^SHA1$/)
    {
      $sNsrlHash = "" . (split(/,/, $sRecord, -1))[$sHashIndex];
    }
    else # MD5
    {
      $sNsrlHash = "" . (reverse(split(/,/, $sRecord, -1)))[$sHashIndex];
    }
    if (my ($sHash) = $sNsrlHash =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = <FH>;
  $sHeader =~ s/[\r\n]+$//;

  if (!defined($sHeader))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  @aFields = split(/,/, $sHeader, -1);

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^"$$phProperties{'HashField'}"$/o)
    {
      $sHashIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq("([0-9a-fA-F]{40})");
  }
  else # MD5
  {
    $sHashRegex = qq("([0-9a-fA-F]{32})");
  }

  while (my $sRecord = <FH>)
  {
    my ($sNsrlHash);
    $sRecord =~ s/[\r\n]+$//;
    $sNsrlHash = "" . (split(/,/, $sRecord, -1))[$sHashIndex];
    if (my ($sHash) = $sNsrlHash =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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
  # Process records.
  #
  ####################################################################

  my ($sHashRegex);

  if ($$phProperties{'HashType'} =~ /^SHA1$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{40}));
  }
  elsif ($$phProperties{'HashType'} =~ /^SHA256$/)
  {
    $sHashRegex = qq(([0-9a-fA-F]{64}));
  }
  else # MD5
  {
    $sHashRegex = qq(([0-9a-fA-F]{32}));
  }

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    if (my ($sHash) = $sRecord =~ /^$sHashRegex$/o)
    {
      print $sSortHandle lc($sHash), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sFilename, $phProperties, $sSortHandle) = @_;

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

  my (@aFields, $sHashIndex, $sHeader, $sModeIndex);

  $sHeader = "path size mtime md5sum mode owner group isconfig isdoc rdev symlink";

  @aFields = reverse(split(/ /, $sHeader, -1));

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] =~ /^md5sum$/o)
    {
      $sHashIndex = $sIndex;
    }
    if ($aFields[$sIndex] =~ /^mode$/o)
    {
      $sModeIndex = $sIndex;
    }
  }

  if (!defined($sHashIndex) || !defined($sModeIndex))
  {
    if (!$$phProperties{'BeQuiet'})
    {
      print STDERR "$$phProperties{'program'}: File='$sFilename' Header='$sHeader' Error='Header did not parse properly.'\n";
    }
    return undef;
  }

  ####################################################################
  #
  # Process records.
  #
  ####################################################################

  while (my $sRecord = <FH>)
  {
    $sRecord =~ s/[\r\n]+$//;
    @aFields = reverse(split(/ /, $sRecord, -1));
    $aFields[$sHashIndex] .= "";
    if ($aFields[$sHashIndex] =~ /^[0-9a-fA-F]{32}$/o)
    {
      print $sSortHandle lc($aFields[$sHashIndex]), "|", $$phProperties{'category'}, "\n";
    }
    else
    {
      $aFields[$sModeIndex] .= "";
      if ($aFields[$sModeIndex] !~ /^010[0-7]{4}$/o)
      {
        next; # Skip anything that's not a regular file
      }
      if (!$$phProperties{'BeQuiet'})
      {
        print STDERR "$$phProperties{'program'}: Record='$sRecord' Error='Record did not parse properly.'\n";
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
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-a hash-type] [-c {K|U}] [-d delimiter] [-h hash-field] [-q] [-s file] [-T dir] -t file-type -o {file|-} file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hashdig-harvest.pl - Harvest hashes from a one or more files

=head1 SYNOPSIS

B<hashdig-harvest.pl> B<[-a hash-type]> B<[-c {K|U}]> B<[-d delimiter]> B<[-h hash-field]> B<[-q]> B<[-s file]> B<[-T dir]> B<-t file-type> B<-o {file|-}> B<file [file ...]>

=head1 DESCRIPTION

This utility extracts hashes of the specified B<hash-type> from
one or more input files of the specified B<file-type>, tags them (see
B<-c>), and writes them to the specified output file (see B<-o>).
Output is a sorted list of hash/category pairs having the following
format:

    hash|category

=head1 OPTIONS

=over 4

=item B<-a hash-type>

Specifies the type of hashes that are to be harvested. Currently, the
following hash types (or algorithms) are supported: 'MD5', 'SHA1', and
'SHA256'. The default hash type is that specified by the HASH_TYPE
environment variable or 'MD5' if HASH_TYPE is not set. The value for
this option is not case sensitive.

=item B<-c category>

Specifies the category, {K|U}, that is to be assigned to each hash.
Currently, the following categories are supported: known (K) and
unknown (U). The default category is unknown. The value for this
option is not case sensitive.

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

=item B<-o {file|-}>

Specifies the name of the output file. A value of '-' will cause
the program to write to stdout.

=item B<-q>

Don't report errors (i.e., be quiet) while processing files.

=item B<-s file>

Specifies the name of an alternate sort utility. Relative paths are
affected by your PATH environment variable. Alternate sort utilities
must support the C<-o>, C<-T> and C<-u> options. This program was
designed to work with GNU sort.

=item B<-T dir>

Specifies the directory sort should use as a temporary work area.
The default directory is that specified by the TMPDIR environment
variable or /tmp if TMPDIR is not set.

=item B<-t file-type>

Specifies the type of files that are to be processed. All files
processed in a given invocation must be of the same type. Currently,
the following types are supported: FTIMES, FTK, GENERIC,
HK|HASHKEEPER, KG|KNOWNGOODS, MD5, MD5DEEP, MD5SUM, NSRL1, NSRL2,
OPENSSL, PLAIN, RPM, SHA1, SHA1DEEP, SHA1SUM, SHA256, SHA256DEEP, and
SHA256SUM. The value for this option is not case sensitive.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), hashdig-make(1), md5(1), md5sum(1), md5deep(1), openssl(1), rpm(8), sha1(1), sha1sum(1), sha1deep(1)

=head1 LICENSE

All HashDig documentation and code is distributed under same terms
and conditions as FTimes.

=cut
