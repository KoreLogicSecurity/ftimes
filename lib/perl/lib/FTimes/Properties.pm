######################################################################
#
# $Id: Properties.pm,v 1.4 2012/01/04 03:12:27 mavrik Exp $
#
######################################################################
#
# Copyright 2011-2012 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Global FTimes Properties.
#
######################################################################

package FTimes::Properties;

require Exporter;

use 5.008;
use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);

use Config;

@EXPORT = qw(ParseFieldMask PropertiesGetGlobalExitCodes PropertiesGetGlobalFieldMasks PropertiesGetGlobalKvps PropertiesGetGlobalRegexes PropertiesGetGlobalTemplates PropertiesGetOfficialKeyName PropertiesGetOfficialKeys);
@EXPORT_OK = ();
@ISA = qw(Exporter);
$VERSION = do { my @r = (q$Revision: 1.4 $ =~ /(\d+)/g); sprintf("%d."."%03d" x $#r, @r); };

######################################################################
#
# FTimes-specific variables.
#
######################################################################

my %hGlobalExitCodes =
(
  'ftimes' =>
  {
     '1' => "XER_Abort",
     '2' => "XER_Usage",
     '3' => "XER_BootStrap",
     '4' => "XER_ProcessArguments",
     '5' => "XER_Initialize",
     '6' => "XER_CheckDependencies",
     '7' => "XER_Finalize",
     '8' => "XER_WorkHorse",
     '9' => "XER_FinishUp",
    '10' => "XER_FinalStage",
  },

  'ftimes-cat' =>
  {
     '1' => "XER_Usage",
     '2' => "XER_Abort",
  },

  'ftimes-xpa' =>
  {
     '1' => "XER_Usage",
     '2' => "XER_Abort",
     '3' => "XER_BootStrap",
     '4' => "XER_ProcessArguments",
  },

  'hashcp' =>
  {
     '1' => "XER_Usage",
     '2' => "XER_Abort",
  },

  'rtimes' =>
  {
     '1' => "XER_Usage",
     '2' => "XER_Abort",
     '3' => "XER_BootStrap",
     '4' => "XER_ProcessArguments",
     '5' => "XER_WorkHorse",
  },

  'tarmap' =>
  {
     '1' => "XER_Usage",
     '2' => "XER_BootStrap",
     '3' => "XER_ProcessArguments",
     '4' => "XER_WorkHorse",
     '5' => "XER_RunMode",
  },
);

my %hGlobalFieldMasks =
(
  'cmp' =>
  {
    'name'        => 0x00000001,
    'dev'         => 0x00000002,
    'inode'       => 0x00000004,
    'volume'      => 0x00000008,
    'findex'      => 0x00000010,
    'mode'        => 0x00000020,
    'attributes'  => 0x00000040,
    'nlink'       => 0x00000080,
    'uid'         => 0x00000100,
    'gid'         => 0x00000200,
    'rdev'        => 0x00000400,
    'atime'       => 0x00000800,
    'ams'         => 0x00001000,
    'mtime'       => 0x00002000,
    'mms'         => 0x00004000,
    'ctime'       => 0x00008000,
    'cms'         => 0x00010000,
    'chtime'      => 0x00020000,
    'chms'        => 0x00040000,
    'size'        => 0x00080000,
    'altstreams'  => 0x00100000,
    'md5'         => 0x00200000,
    'sha1'        => 0x00400000,
    'sha256'      => 0x00800000,
    'magic'       => 0x01000000,
    'owner'       => 0x02000000,
    'group'       => 0x04000000,
    'dacl'        => 0x08000000,
    # Combo fields
    'hashes'      => 0x00e00000, # This field is a combination of the 'md5', 'sha1', and 'sha256' fields.
    'times'       => 0x0007f800, # This field is a combination of the 'atime, 'mtime', 'ctime', and 'chtime' fields.
  },

  'map.unix' =>
  {
    'dev'         => 0x00000001,
    'inode'       => 0x00000002,
    'mode'        => 0x00000004,
    'nlink'       => 0x00000008,
    'uid'         => 0x00000010,
    'gid'         => 0x00000020,
    'rdev'        => 0x00000040,
    'atime'       => 0x00000080,
    'mtime'       => 0x00000100,
    'ctime'       => 0x00000200,
    'size'        => 0x00000400,
    'md5'         => 0x00000800,
    'sha1'        => 0x00001000,
    'sha256'      => 0x00002000,
    'magic'       => 0x00004000,
    # Combo fields
    'hashes'      => 0x00003800, # This field is a combination of the 'md5', 'sha1', and 'sha256' fields.
    'times'       => 0x00000380, # This field is a combination of the 'atime, 'mtime', and 'ctime' fields.
  },

  'map.winx' =>
  {
    'volume'      => 0x00000001,
    'findex'      => 0x00000002,
    'attributes'  => 0x00000004,
    'atime'       => 0x00000008,
    'mtime'       => 0x00000010,
    'ctime'       => 0x00000020,
    'chtime'      => 0x00000040,
    'size'        => 0x00000080,
    'altstreams'  => 0x00000100,
    'md5'         => 0x00000200,
    'sha1'        => 0x00000400,
    'sha256'      => 0x00000800,
    'magic'       => 0x00001000,
    'owner'       => 0x00002000,
    'group'       => 0x00004000,
    'dacl'        => 0x00008000,
    # Combo fields
    'hashes'      => 0x00000e00, # This field is a combination of the 'md5', 'sha1', and 'sha256' fields.
    'times'       => 0x00000078, # This field is a combination of the 'atime, 'mtime', 'ctime', and 'chtime' fields.
  },
);

my %hGlobalKvps =
(
  'CmpHeaderFields'         => [ qw(category name changed unknown records) ],
  'CmpMaskFields'           => [ qw(name dev inode volume findex mode attributes nlink uid gid rdev atime ams mtime mms ctime cms chtime chms size altstreams md5 sha1 sha256 magic owner group dacl) ],
  'DigHeaderFields'         => [ qw(name type tag offset string) ],
  'DigMaskFields'           => [ qw(type tag offset string) ],
  'FTimesLogPrefixDebug'    => '@@@  DEBUGGER  @@@',
  'FTimesLogPrefixWayPoint' => '---  WAYPOINT  ---',
  'FTimesLogPrefixLandmark' => '+++  LANDMARK  +++',
  'FTimesLogPrefixProperty' => '<<<  PROPERTY  >>>',
  'FTimesLogPrefixWarning'  => '***  LOG_WARN  ***',
  'FTimesLogPrefixFailure'  => '***  LOG_FAIL  ***',
  'FTimesLogPrefixCritical' => '***  LOG_CRIT  ***',
  'MapHeaderFieldsUnix'     => [ qw(name dev inode mode nlink uid gid rdev atime mtime ctime size md5 sha1 sha256 magic) ],
  'MapMaskFieldsUnix'       => [ qw(dev inode mode nlink uid gid rdev atime mtime ctime size md5 sha1 sha256 magic) ],
  'MapHeaderFieldsWinx'     => [ qw(name volume findex attributes atime ams mtime mms ctime cms chtime chms size altstreams md5 sha1 sha256 magic owner group dacl) ],
  'MapMaskFieldsWinx'       => [ qw(volume findex attributes atime mtime ctime chtime size altstreams md5 sha1 sha256 magic owner group dacl) ],
);

my %hGlobalRegexes =
(
  'AnyValue'               => qq(.*),
  'AnyValueExceptNothing'  => qq(.+),
  'AuthType'               => qq((?:basic|none)),
  'BaseDirectory'          => qq((?:[A-Za-z]:)?/[\\w./-]+),
  'BaseNameSuffix'         => qq((?:datetime|none|pid)),
  'Boolean'                => qq((?i)(?:[10]|y(es)?|n(o)?|t(rue)?|f(alse)?|on|off)),
  'CrlfOrLf'               => qq((?:CR)?LF),
  'DateTime'               => qq([0-9]{4}-[0-9]{2}-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}),
  'Decimal16Bit'           => qq(\\d{1,5}), # 65535
  'Decimal32Bit'           => qq(\\d{1,10}), # 4294967295
  'Decimal64Bit'           => qq(\\d{1,20}), # 18446744073709551615
  'CmpFieldMask'           => "^(all|none)([+-](?:" . join("|", @{$hGlobalKvps{'CmpMaskFields'}}, "times", "hashes") . ")+)*\$",
  'FileMode'               => qq(0[0-7]{1,6}),
  'Filename'               => qq([\\w+.:-]{1,1024}),
  'Hash128Bit'             => qq([0-9A-Fa-f]{32}),
  'Hash160Bit'             => qq([0-9A-Fa-f]{40}),
  'Hash256Bit'             => qq([0-9A-Fa-f]{64}),
  'OsClass'                => qq((?:UNIX|WINX)),
  'Priority'               => qq((?:low|below_normal|normal|above_normal|high)),
  'RunMode'                => qq((?:cfgtest|compare|decode|dig(?:auto|full|lean)|get|mad|map(?:auto|full|lean))),
  'RunType'                => qq((?:baseline|linktest|snapshot)),
  'MapFieldMaskUnix'       => "^(all|none)([+-](?:" . join("|", @{$hGlobalKvps{'MapHeaderFieldsUnix'}}, "times", "hashes") . ")+)*\$",
  'Username'               => qq([\\w]{1,32}),
  'UrlGetRequest'          => qq((?i)dig(?:full|lean)config|map(?:full|lean)config),
  'MapFieldMaskWinx'       => "^(all|none)([+-](?:" . join("|", @{$hGlobalKvps{'MapHeaderFieldsWinx'}}, "times", "hashes") . ")+)*\$",
  'YesNo'                  => qq([YyNn]),
);

my %hGlobalTemplates =
(
  'generic' =>
  {
    qq([A-Za-z][\\w.-]*)                 => $hGlobalRegexes{'AnyValue'},
  },

  'ftimes.dig' =>
  {
    'AnalyzeBlockSize'                   => $hGlobalRegexes{'Decimal32Bit'},
    'AnalyzeByteCount'                   => $hGlobalRegexes{'Decimal64Bit'},
    'AnalyzeCarrySize'                   => $hGlobalRegexes{'Decimal32Bit'},
    'AnalyzeDeviceFiles'                 => $hGlobalRegexes{'YesNo'},
    'AnalyzeMaxDps'                      => $hGlobalRegexes{'Decimal32Bit'},
    'AnalyzeRemoteFiles'                 => $hGlobalRegexes{'YesNo'},
    'AnalyzeStartOffset'                 => $hGlobalRegexes{'Decimal64Bit'},
    'AnalyzeStepSize'                    => $hGlobalRegexes{'Decimal32Bit'},
    'BaseName'                           => $hGlobalRegexes{'Filename'},
    'BaseNameSuffix'                     => $hGlobalRegexes{'BaseNameSuffix'},
    'DigStringNoCase'                    => $hGlobalRegexes{'AnyValueExceptNothing'},
    'DigStringNormal'                    => $hGlobalRegexes{'AnyValueExceptNothing'},
    'DigStringRegExp'                    => $hGlobalRegexes{'AnyValueExceptNothing'},
    'DigStringXMagic'                    => $hGlobalRegexes{'AnyValueExceptNothing'},
    'EnableRecursion'                    => $hGlobalRegexes{'YesNo'},
    'Exclude'                            => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludeFilter'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludesMustExist'                  => $hGlobalRegexes{'YesNo'},
    'FileSizeLimit'                      => $hGlobalRegexes{'Decimal32Bit'},
    'Import'                             => $hGlobalRegexes{'AnyValueExceptNothing'},
    'Include'                            => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludeFilter'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludesMustExist'                  => $hGlobalRegexes{'YesNo'},
    'LogDir'                             => $hGlobalRegexes{'BaseDirectory'},
    'MatchLimit'                         => $hGlobalRegexes{'Decimal32Bit'},
    'NewLine'                            => $hGlobalRegexes{'CrlfOrLf'},
    'OutDir'                             => $hGlobalRegexes{'BaseDirectory'},
    'Priority'                           => $hGlobalRegexes{'Priority'},
    'RequirePrivilege'                   => $hGlobalRegexes{'YesNo'},
    'RunType'                            => $hGlobalRegexes{'RunType'},
    'SslBundledCAsFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslExpectedPeerCN'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslMaxChainLength'                  => $hGlobalRegexes{'Decimal16Bit'},
    'SslPassPhrase'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPrivateKeyFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPublicCertFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslUseCertificate'                  => $hGlobalRegexes{'YesNo'},
    'SslVerifyPeerCert'                  => $hGlobalRegexes{'YesNo'},
    'UrlAuthType'                        => $hGlobalRegexes{'AuthType'},
    'UrlPassword'                        => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlPutSnapshot'                     => $hGlobalRegexes{'YesNo'},
    'UrlPutUrl'                          => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlUnlinkOutput'                    => $hGlobalRegexes{'YesNo'},
    'UrlUsername'                        => $hGlobalRegexes{'Username'},
  },

  'ftimes.get' =>
  {
    'BaseName'                           => $hGlobalRegexes{'Filename'},
    'GetAndExec'                         => $hGlobalRegexes{'YesNo'},
    'GetFileName'                        => $hGlobalRegexes{'Filename'},
    'Import'                             => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslBundledCAsFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslExpectedPeerCN'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslMaxChainLength'                  => $hGlobalRegexes{'Decimal16Bit'},
    'SslPassPhrase'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPrivateKeyFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPublicCertFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslUseCertificate'                  => $hGlobalRegexes{'YesNo'},
    'SslVerifyPeerCert'                  => $hGlobalRegexes{'YesNo'},
    'UrlGetRequest'                      => $hGlobalRegexes{'UrlGetRequest'},
    'UrlAuthType'                        => $hGlobalRegexes{'AuthType'},
    'UrlGetUrl'                          => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlPassword'                        => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlUsername'                        => $hGlobalRegexes{'Username'},
  },

  'ftimes.map' =>
  {
    'AnalyzeBlockSize'                   => $hGlobalRegexes{'Decimal32Bit'},
    'AnalyzeByteCount'                   => $hGlobalRegexes{'Decimal64Bit'},
    'AnalyzeDeviceFiles'                 => $hGlobalRegexes{'YesNo'},
    'AnalyzeMaxDps'                      => $hGlobalRegexes{'Decimal32Bit'},
    'AnalyzeRemoteFiles'                 => $hGlobalRegexes{'YesNo'},
    'AnalyzeStartOffset'                 => $hGlobalRegexes{'Decimal64Bit'},
    'AnalyzeStepSize'                    => $hGlobalRegexes{'Decimal32Bit'},
    'BaseName'                           => $hGlobalRegexes{'Filename'},
    'BaseNameSuffix'                     => $hGlobalRegexes{'BaseNameSuffix'},
    'Compress'                           => $hGlobalRegexes{'YesNo'},
    'EnableRecursion'                    => $hGlobalRegexes{'YesNo'},
    'Exclude'                            => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludeFilter'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludeFilterMd5'                   => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludeFilterSha1'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludeFilterSha256'                => $hGlobalRegexes{'AnyValueExceptNothing'},
    'ExcludesMustExist'                  => $hGlobalRegexes{'YesNo'},
    'FieldMask'                          => $hGlobalRegexes{'FieldMask'},
    'FileSizeLimit'                      => $hGlobalRegexes{'Decimal32Bit'},
    'HashDirectories'                    => $hGlobalRegexes{'YesNo'},
    'HashSymbolicLinks'                  => $hGlobalRegexes{'YesNo'},
    'Import'                             => $hGlobalRegexes{'AnyValueExceptNothing'},
    'Include'                            => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludeFilter'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludeFilterMd5'                   => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludeFilterSha1'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludeFilterSha256'                => $hGlobalRegexes{'AnyValueExceptNothing'},
    'IncludesMustExist'                  => $hGlobalRegexes{'YesNo'},
    'LogDir'                             => $hGlobalRegexes{'BaseDirectory'},
    'MagicFile'                          => $hGlobalRegexes{'Filename'},
    'NewLine'                            => $hGlobalRegexes{'CrlfOrLf'},
    'OutDir'                             => $hGlobalRegexes{'BaseDirectory'},
    'Priority'                           => $hGlobalRegexes{'Priority'},
    'RequirePrivilege'                   => $hGlobalRegexes{'YesNo'},
    'RunType'                            => $hGlobalRegexes{'RunType'},
    'SslBundledCAsFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslExpectedPeerCN'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslMaxChainLength'                  => $hGlobalRegexes{'Decimal16Bit'},
    'SslPassPhrase'                      => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPrivateKeyFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslPublicCertFile'                  => $hGlobalRegexes{'AnyValueExceptNothing'},
    'SslUseCertificate'                  => $hGlobalRegexes{'YesNo'},
    'SslVerifyPeerCert'                  => $hGlobalRegexes{'YesNo'},
    'UrlAuthType'                        => $hGlobalRegexes{'AuthType'},
    'UrlPassword'                        => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlPutSnapshot'                     => $hGlobalRegexes{'YesNo'},
    'UrlPutUrl'                          => $hGlobalRegexes{'AnyValueExceptNothing'},
    'UrlUnlinkOutput'                    => $hGlobalRegexes{'YesNo'},
    'UrlUsername'                        => $hGlobalRegexes{'Username'},
  },

  'nph-ftimes.global' =>
  {
    'BaseDirectory'                      => $hGlobalRegexes{'BaseDirectory'},
    'CapContentLength'                   => $hGlobalRegexes{'YesNo'},
    'EnableLogging'                      => $hGlobalRegexes{'YesNo'},
    'MaxContentLength'                   => $hGlobalRegexes{'Decimal64Bit'},
    'RequireMask'                        => $hGlobalRegexes{'YesNo'},
    'RequireMatch'                       => $hGlobalRegexes{'YesNo'},
    'RequireUser'                        => $hGlobalRegexes{'YesNo'},
    'RequiredDigMask'                    => $hGlobalRegexes{'FieldMask'},
    'RequiredMapMask'                    => $hGlobalRegexes{'FieldMask'},
    'UseGmt'                             => $hGlobalRegexes{'YesNo'},
  },
);

######################################################################
#
# ParseFieldMask
#
######################################################################

sub ParseFieldMask
{
  my ($sFieldMask, $phFieldMask) = @_;

  ####################################################################
  #
  # Squash input and check syntax.
  #
  ####################################################################

  $sFieldMask =~ tr/A-Z/a-z/;

  my $sFieldMaskRegex = PropertiesGetGlobalRegexes()->{'CmpFieldMask'};
  if ($sFieldMask !~ /^$sFieldMaskRegex$/)
  {
    return undef;
  }

  ####################################################################
  #
  # Split input once for fields and then for tokens.
  #
  ####################################################################

  my @aFields = split(/[+-]/, $sFieldMask);
  my @aTokens = split(/[^+-]+/, $sFieldMask);

  ####################################################################
  #
  # Remove base element from the arrays.
  #
  ####################################################################

  my $sBaseField = $aFields[0];
  shift(@aFields);
  shift(@aTokens);

  ####################################################################
  #
  # Initialize the field mask.
  #
  ####################################################################

  if ($sBaseField eq "all")
  {
    foreach my $sField (@{$hGlobalKvps{'CmpMaskFields'}})
    {
      $$phFieldMask{$sField} = 1;
    }
  }
  else
  {
    foreach my $sField (@{$hGlobalKvps{'CmpMaskFields'}})
    {
      $$phFieldMask{$sField} = 0;
    }
  }
  $$phFieldMask{'name'} = 1; # This field must be set in all cases.

  ####################################################################
  #
  # Build the BitMask.
  #
  ####################################################################

  my $sBitMask;

  for (my $sIndex = 0; $sIndex < scalar(@aFields); $sIndex++)
  {
    if ($aFields[$sIndex] eq "hashes")
    {
      $$phFieldMask{'md5'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'sha1'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'sha256'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
    }
    elsif ($aFields[$sIndex] eq "times")
    {
      $$phFieldMask{'ams'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'atime'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'chms'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'chtime'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'cms'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'ctime'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'mms'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
      $$phFieldMask{'mtime'} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
    }
    else
    {
      $$phFieldMask{$aFields[$sIndex]} = ($aTokens[$sIndex] eq "+") ? 1 : 0;
    }
  }

  foreach my $sField (@{$hGlobalKvps{'CmpMaskFields'}})
  {
    $sBitMask |= $hGlobalFieldMasks{'cmp'}{$sField} if ($$phFieldMask{$sField});
  }

  return $sBitMask;
}


######################################################################
#
# PropertiesGetGlobalExitCodes
#
######################################################################

sub PropertiesGetGlobalExitCodes
{
  return \%hGlobalExitCodes;
}


######################################################################
#
# PropertiesGetGlobalFieldMasks
#
######################################################################

sub PropertiesGetGlobalFieldMasks
{
  return \%hGlobalFieldMasks;
}


######################################################################
#
# PropertiesGetGlobalKvps
#
######################################################################

sub PropertiesGetGlobalKvps
{
  return \%hGlobalKvps;
}


######################################################################
#
# PropertiesGetGlobalRegexes
#
######################################################################

sub PropertiesGetGlobalRegexes
{
  return \%hGlobalRegexes;
}


######################################################################
#
# PropertiesGetGlobalTemplates
#
######################################################################

sub PropertiesGetGlobalTemplates
{
  return \%hGlobalTemplates;
}


######################################################################
#
# PropertiesGetOfficialKeyName
#
######################################################################

sub PropertiesGetOfficialKeyName
{
  my ($sType, $sKey) = @_;

  ####################################################################
  #
  # If the keys in the template of the specified type are regular
  # expressions, then there is no official name, so simply return the
  # name that was supplied by the caller -- assuming it passes as a
  # valid key.
  #
  ####################################################################

  if ($sType =~ /^generic$/)
  {
    foreach my $sOfficialKey (keys(%{$hGlobalTemplates{$sType}}))
    {
      if ($sKey =~ /^$sOfficialKey$/)
      {
        return $sKey;
      }
    }
  }
  else
  {
    foreach my $sOfficialKey (keys(%{$hGlobalTemplates{$sType}}))
    {
      if (lc($sKey) eq lc($sOfficialKey))
      {
        return $sOfficialKey;
      }
    }
  }

  return undef;
}


######################################################################
#
# PropertiesGetOfficialKeys
#
######################################################################

sub PropertiesGetOfficialKeys
{
  my ($sType) = @_;

  if (exists($hGlobalTemplates{$sType}))
  {
    return sort(keys(%{$hGlobalTemplates{$sType}}));
  }

  return undef;
}

1;

__END__

=head1 NAME

FTimes::Properties - Global FTimes Properties

=head1 SYNOPSIS

    use FTimes::Properties;

    my $phGlobalRegexes = PropertiesGetGlobalRegexes();

=head1 DESCRIPTION

This module contains a collection of properties commonly used by
various FTimes utilities (e.g., regular expressions).

=head1 AUTHOR

Klayton Monroe and Jason Smith

=head1 LICENSE

All documentation and code are distributed under same terms and
conditions as B<FTimes>.

=cut
