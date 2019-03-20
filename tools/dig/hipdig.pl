#!/usr/bin/perl -w
######################################################################
#
# $Id: hipdig.pl,v 1.57 2019/03/14 16:07:43 klm Exp $
#
######################################################################
#
# Copyright 2001-2019 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Dig for hosts, IPs, passwords, and more...
#
######################################################################

use strict;
use File::Basename;
use FindBin qw($Bin $RealBin); use lib ("$Bin/../lib/perl5/site_perl", "$RealBin/../lib/perl5/site_perl", "/usr/local/ftimes/lib/perl5/site_perl");
use FTimes::EadRoutines 1.025;
use Getopt::Std;
use vars qw($sDigStringRegExp);

BEGIN
{
  ####################################################################
  #
  # The Properties hash is essentially private. Those parts of the
  # program that wish to access or modify the data in this hash need
  # to call GetProperties() to obtain a reference.
  #
  ####################################################################

  my (%hProperties);

  sub GetProperties
  {
    return \%hProperties;
  }
}

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

  my ($phProperties);

  $phProperties = GetProperties();

  $$phProperties{'Program'} = basename(__FILE__);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('D:l:o:r:s:T:t:', \%hOptions))
  {
    Usage($$phProperties{'Program'});
  }

  ####################################################################
  #
  # The dump type information command, '-D', is optional.
  #
  ####################################################################

  $$phProperties{'DumpType'} = (exists($hOptions{'D'})) ? uc($hOptions{'D'}) : undef;

  if (defined($$phProperties{'DumpType'}))
  {
    if ($$phProperties{'DumpType'} =~ /^(?:DOMAIN|HOST)$/o)
    {
      DumpDomainInformation(0);
      exit(0);
    }
    elsif ($$phProperties{'DumpType'} =~ /^DOMAIN_REGEX$/o)
    {
      DumpDomainInformation(1);
      exit(0);
    }
    elsif ($$phProperties{'DumpType'} =~ /^(SSN|SOCIAL)$/o)
    {
      DumpSSNInformation();
      exit(0);
    }
    elsif ($$phProperties{'DumpType'} =~ /^(STATE)$/o)
    {
      DumpStateInformation();
      exit(0);
    }
    elsif ($$phProperties{'DumpType'} =~ /^(EIN|TIN)$/o)
    {
      DumpEinInformation();
      exit(0);
    }
    else
    {
      print STDERR "$$phProperties{'Program'}: Error='Invalid dump type ($$phProperties{'DumpType'}).'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # The stdin label, '-l', is optional.
  #
  ####################################################################

  $$phProperties{'StdinLabel'} = (exists($hOptions{'l'})) ? $hOptions{'l'} : "-";

  ####################################################################
  #
  # The option list, '-o', is optional.
  #
  ####################################################################

  my @sSupportedOptions =
  (
    'BeQuiet',
    'MadMode',
    'NoHeader',
    'RegularFilesOnly',
  );
  foreach my $sOption (@sSupportedOptions)
  {
    $$phProperties{$sOption} = 0;
  }

  $$phProperties{'Options'} = (exists($hOptions{'o'})) ? $hOptions{'o'} : undef;

  if (defined($$phProperties{'Options'}))
  {
    foreach my $sOption (split(/,/, $$phProperties{'Options'}))
    {
      foreach my $sSupportedOption (@sSupportedOptions)
      {
        $sOption = $sSupportedOption if ($sOption =~ /^$sSupportedOption$/i);
      }
      if (!exists($$phProperties{$sOption}))
      {
        print STDERR "$$phProperties{'Program'}: Error='Unknown or unsupported option ($sOption).'\n";
        exit(2);
      }
      $$phProperties{$sOption} = 1;
    }
  }

  ####################################################################
  #
  # A read buffer size, '-r', is optional.
  #
  ####################################################################

  $$phProperties{'ReadBufferSize'} = (exists($hOptions{'r'})) ? $hOptions{'r'} : 32768;

  if ($$phProperties{'ReadBufferSize'} !~ /^\d{1,10}$/ || $$phProperties{'ReadBufferSize'} <= 0)
  {
    print STDERR "$$phProperties{'Program'}: Error='Invalid read buffer size ($$phProperties{'ReadBufferSize'}).'\n";
    exit(2);
  }

  ####################################################################
  #
  # A save buffer size, '-s', is optional.
  #
  ####################################################################

  $$phProperties{'SaveBufferSize'} = (exists($hOptions{'s'})) ? $hOptions{'s'} : 64;

  if ($$phProperties{'SaveBufferSize'} !~ /^\d{1,10}$/ || $$phProperties{'SaveBufferSize'} <= 0)
  {
    print STDERR "$$phProperties{'Program'}: Error='Invalid save buffer size ($$phProperties{'SaveBufferSize'}).'\n";
    exit(2);
  }

  ####################################################################
  #
  # A dig tag, '-T', is optional.
  #
  ####################################################################

  $$phProperties{'DigTag'} = (exists($hOptions{'T'})) ? $hOptions{'T'} : undef;

  ####################################################################
  #
  # A dig type, '-t', is optional.
  #
  ####################################################################

  my ($sDigType, $sDigRoutine);

  $sDigType = (exists($hOptions{'t'})) ? $hOptions{'t'} : "IP";

  if ($sDigType =~ /^\s*CUSTOM\s*([=~])\s*(.+)\s*$/i)
  {
    $sDigStringRegExp = ($1 eq "=") ? "($2)" : "$2";
    $sDigRoutine = \&Dig4Custom;
    $$phProperties{'DigTag'} = "" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^HOST$/i)
  {
    $sDigRoutine = \&Dig4Domains;
    $$phProperties{'DigTag'} = "domain" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^IP$/i)
  {
    $sDigRoutine = \&Dig4IPs;
    $$phProperties{'DigTag'} = "ip" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(PASS|PASSWORD)$/i)
  {
    $sDigRoutine = \&Dig4Passwords;
    $$phProperties{'DigTag'} = "password" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(SSN|SOCIAL)$/i)
  {
    $sDigRoutine = \&Dig4SSN;
    $$phProperties{'DigTag'} = "ssn" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(T1|TRACK1)$/i)
  {
    $sDigRoutine = \&Dig4Track1;
    $$phProperties{'DigTag'} = "track1" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(T1S|TRACK1-STRICT)$/i)
  {
    $sDigRoutine = \&Dig4Track1Strict;
    $$phProperties{'DigTag'} = "track1strict" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(T2|TRACK2)$/i)
  {
    $sDigRoutine = \&Dig4Track2;
    $$phProperties{'DigTag'} = "track2" if (!defined($$phProperties{'DigTag'}));
  }
  elsif ($sDigType =~ /^(T2S|TRACK2-STRICT)$/i)
  {
    $sDigRoutine = \&Dig4Track2Strict;
    $$phProperties{'DigTag'} = "track2strict" if (!defined($$phProperties{'DigTag'}));
  }
  else
  {
    print STDERR "$$phProperties{'Program'}: Error='Invalid dig type ($sDigType).'\n";
    exit(2);
  }

  ####################################################################
  #
  # If there isn't at least one argument left, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) < 1)
  {
    Usage($$phProperties{'Program'});
  }

  ####################################################################
  #
  # Do a sanity check on the read/save buffer sizes.
  #
  ####################################################################

  if ($$phProperties{'SaveBufferSize'} * 10 > $$phProperties{'ReadBufferSize'})
  {
    print STDERR "$$phProperties{'Program'}: Error='The save buffer size is more than 1/10th the read buffer size. Either increase the read buffer or decrease the save buffer.'\n";
    exit(2);
  }

  ####################################################################
  #
  # Check custom dig string expression to ensure that Perl groks it.
  #
  ####################################################################

  if (defined($sDigStringRegExp))
  {
    eval { "This is a test." =~ m/$sDigStringRegExp/gso; };
    if ($@)
    {
      my $sMessage = $@; $sMessage =~ s/[\r\n]+/ /g; $sMessage =~ s/\s+$//;
      print STDERR "$$phProperties{'Program'}: Error='Expression check failed ($sMessage).'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Set header/record print formats.
  #
  ####################################################################

  my ($sHeaderFormat, $sRecordFormat);

  if ($$phProperties{'MadMode'})
  {
    $sHeaderFormat = "dig|name|type|tag|offset|string\n";
    $sRecordFormat = "dig|\"%s\"|regexp|$$phProperties{'DigTag'}|%d|%s\n";
  }
  else
  {
    $sHeaderFormat = "name|type|tag|offset|string\n";
    $sRecordFormat = "\"%s\"|regexp|$$phProperties{'DigTag'}|%d|%s\n";
  }

  ####################################################################
  #
  # Conditionally print out a header line.
  #
  ####################################################################

  print $sHeaderFormat unless ($$phProperties{'NoHeader'});

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  my ($sHandle, $sStdinExhausted);

  $sStdinExhausted = 0;

  foreach my $sCandidateFile (@ARGV)
  {
    my $sFile = ($sCandidateFile =~ m{^file://(.+)$}o) ? EadFTimesUrlDecode($1) : $sCandidateFile;

    if ($sFile eq "-")
    {
      if ($sStdinExhausted)
      {
        next;
      }
      $sHandle = \*STDIN;
      $sStdinExhausted = 1;
      $sFile = $$phProperties{'StdinLabel'};
    }
    else
    {
      if ($$phProperties{'RegularFilesOnly'} && !-f $sFile)
      {
        print STDERR "$$phProperties{'Program'}: Warning='File \"$sFile\" is not regular, and was skipped.'\n" unless ($$phProperties{'BeQuiet'});
        next;
      }
      if (!open(FH, "< $sFile"))
      {
        print STDERR "$$phProperties{'Program'}: Error='File \"$sFile\" could not be opened ($!).'\n";
        next;
      }
      $sHandle = \*FH;
    }

    binmode($sHandle); # Enable binmode for WIN32 platforms.

    ##################################################################
    #
    # Unsearched data from previous reads is saved by copying it to
    # the beginning of the input buffer. Newly read data is then
    # appended to the saved data. This technique is used to simulate
    # a continuous input stream.
    #
    #                               +----------------+
    #                         +---> |xxxxx SAVE xxxxx|    +---> ...
    #   +----------------+    |     +----------------+    |
    #   |                |    |     |                |    |
    #   |    1st READ    |    |     |    2nd READ    |    |
    #   |                |    |     |                |    |
    #   |xxxxx SAVE xxxxx| ---+     |xxxxx SAVE xxxxx| ---+
    #   +----------------+          +----------------+
    #
    ##################################################################

    my ($sData, $sDataOffset, $sLastOffset, $sNRead, $sReadOffset, $sDataLength);

    $sReadOffset = $sDataOffset = 0;

    while ($sNRead = read($sHandle, $sData, $$phProperties{'ReadBufferSize'}, $sDataOffset))
    {
      $sDataLength = $sDataOffset + $sNRead;
      $sLastOffset = &$sDigRoutine(\$sData, $sDataOffset, $sReadOffset, $sFile, $sRecordFormat);
      $sDataOffset = ($sDataLength - $sLastOffset > $$phProperties{'SaveBufferSize'}) ? $$phProperties{'SaveBufferSize'} : $sDataLength - $sLastOffset;
      $sData = substr($sData, ($sDataLength - $sDataOffset), $sDataOffset);
      $sReadOffset += $sNRead;
    }

    if (!defined($sNRead))
    {
      print STDERR "$$phProperties{'Program'}: Error='File \"$sFile\" could not be read ($!).'\n";
    }

    close($sHandle);
  }

  ####################################################################
  #
  # Clean up and go home.
  #
  ####################################################################

  1;


######################################################################
#
# Dig4Custom
#
######################################################################

sub Dig4Custom
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # Select user-defined strings. Set the g flag to match all values
  # in the buffer. Set the s flag to ignore newline boundaries. Set
  # the o flag to compile the expression once. Note: The expression
  # is expected to have at least one set of capturing parentheses,
  # but in case it doesn't, the special arrays @- and @+ are used to
  # construct the appropriate value for $1.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required -- this is a custom dig.

  $sMatchOffset = 0;

  while ($$psData =~ m/$sDigStringRegExp/gso)
  {
    my $sValue = (scalar(@-) < 2) ? substr($$psData, $-[0], $+[0] - $-[0]) : $1;
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($sValue) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($sValue));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Domains
#
######################################################################

sub Dig4Domains
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # Select candidate hostnames that are bound to the right by a
  # character that is not an alphanumeric or a dot. Set the g flag
  # to match all hostnames in the buffer. Set the s flag to ignore
  # newline boundaries. Set the x flag to allow white space in the
  # expression. Set the i flag to go case insensitive. Set the o
  # flag to compile the expression once.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required (i.e., a trailing lookahead is used).

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (?:^|[^\w.]) # The match must be preceded by nothing or anything but an alphanumeric or a dot.
                        (
                          (?:[\w-]+\.)+ # Match one or more of (one or more (alphanumeric or hyphen) followed by a dot).
                          (?:
                            A[CDEFGILMNOQRSTUWZ]|
                            B[ABDEFGHIJMNORSTVWYZ]|
                            C[ACDFGHIKLMNORSUVXYZ]|
                            D[EJKMOZ]|
                            E[CEGHRST]|
                            F[IJKMORX]|
                            G[ABDEFGHILMNPQRSTUWY]|
                            H[KMNRTU]|
                            I[DELMNOQRST]|
                            J[EMOP]|
                            K[EGHIMNPRWYZ]|
                            L[ABCIKRSTUVY]|
                            M[ACDGHKLMNOPQRSTUVWXYZ]|
                            N[ACEFGILOPRTUZ]|
                            O[M]|
                            P[AEFGHKLMNRSTWY]|
                            Q[A]|
                            R[EOUW]|
                            S[ABCDEGHIJKLMNORTUVYZ]|
                            T[CDFGHJKMNOPRTVWZ]|
                            U[AGKMSYZ]|
                            V[ACEGINU]|
                            W[FS]|
                            Y[ETU]|
                            Z[AMRW]|
                            COM|EDU|GOV|INT|MIL|NET|ORG|
                            ARPA|NATO
                          )\.? # Fully qualified names will end with a dot.
                        ) # Save the match in $1.
                        (?![\w.]) # The match must be followed by nothing or anything but an alphanumeric or a dot.
                      }gsxio
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4IPs
#
######################################################################

sub Dig4IPs
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # Select candidate IPs that are bound to the right by a character
  # that is not a dot or a digit. Set the g flag to match all IPs
  # in the buffer. Set the s flag to ignore newline boundaries. Set
  # the x flag to allow white space in the expression. Set the o
  # flag to compile the expression once.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required (i.e., a trailing lookahead is used).

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (?:^|[^0-9.]) # The match must be preceded by nothing or anything but a digit or a dot.
                        (
                          (?:[1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])
                        ) # Save the match in $1.
                        (?![0-9.]) # The match must be followed by nothing or anything but a digit or a dot.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Passwords
#
######################################################################

sub Dig4Passwords
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # Select candidate passwords that are bound by a colon on either
  # side. Set the g flag to match all passwords in the buffer. Set
  # the s flag to ignore newline boundaries. Set the x flag to allow
  # white space in the expression. Set the o flag to compile the
  # expression once.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 1; # Adjustment of one is required (i.e., to account for trailing ':').

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        : # The match must be preceded by a colon.
                        (
                          \$[1-9]\$[\.\/0-9a-zA-Z]{8}\$[\.\/0-9a-zA-Z]{22}| # Modular crypt
                          [\.\/0-9a-zA-Z]{13}(?:,[\.\/0-9a-zA-Z]{4})? # Old crypt
                        ) # Save the match in $1.
                        : # The match must be followed by a colon.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Track1
#
######################################################################

sub Dig4Track1
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # The Track1 format, given here, is based on information located at
  # the following URL: http://www.exeba.com/comm40/track.htm
  #
  # Field  1) Start Sentinel always '%'
  # Field  2) Format Code always 'B'
  # Field  3) Account Code 13 or 16 Characters
  # Field  4) Separator always '^'
  # Field  5) Cardholder Name Variable Length
  # Field  6) Separator always '^'
  # Field  7) Expiration Date 4 Digits, YYMM Format
  # Field  8) Service Code 3 Digits
  # Field  9) PIN Verification Number 0 To 5 Digits
  # Field 10) Optional Discretionary Data Variable
  # Field 11) End Sentinel always '?'
  # Field 12) Check Character 1 Check Character
  #
  # This routine matches fields 1 through 6.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required.

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (
                          %                                  # Field 1
                          B                                  # Field 2
                          [0-9A-Za-z]{13}(?:[0-9A-Za-z]{3})? # Field 3
                          \^                                 # Field 4
                          [^\^]*                             # Field 5
                          \^                                 # Field 6
                        ) # Save the match in $1.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Track1Strict
#
######################################################################

sub Dig4Track1Strict
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # The Track1 format is described in the Dig4Track1 routine. This
  # routine matches fields 1 through 11. To reduce false positives,
  # the expression for field 10 does not allow separators or sentinels
  # (i.e., '^', '%', or '?') -- this is based on the assumption that
  # they are reserved characters.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required.

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (
                          %                                  # Field 1
                          B                                  # Field 2
                          [0-9A-Za-z]{13}(?:[0-9A-Za-z]{3})? # Field 3
                          \^                                 # Field 4
                          [^\^]*                             # Field 5
                          \^                                 # Field 6
                          [0-9]{4}                           # Field 7
                          [0-9]{3}                           # Field 8
                          [0-9]{0,5}                         # Field 9
                          [^%\^\?]*                          # Field 10
                          \?                                 # Field 11
                        ) # Save the match in $1.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Track2
#
######################################################################

sub Dig4Track2
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # The Track2 format, given here, is based on information located at
  # the following URL: http://www.exeba.com/comm40/track.htm
  #
  # Field  1) Start Sentinel always ';'
  # Field  2) Account Code 13 or 16 Characters
  # Field  3) Separator always '='
  # Field  4) Expiration Date 4 Digits, YYMM Format
  # Field  5) Service Code 3 Digits
  # Field  6) PIN Verification Number 0 To 5 Digits
  # Field  7) Optional Discretionary Data Variable
  # Field  8) End Sentinel always '?'
  # Field  9) Check Character 1 Check Character
  #
  # This routine matches fields 1 through 3.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required.

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (
                          ;                                  # Field 1
                          [0-9A-Za-z]{13}(?:[0-9A-Za-z]{3})? # Field 2
                          =                                  # Field 3
                        ) # Save the match in $1.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# Dig4Track2Strict
#
######################################################################

sub Dig4Track2Strict
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # The Track2 format is described in the Dig4Track2 routine. This
  # routine matches fields 1 through 8. To reduce false positives,
  # the expression for field 7 does not allow separators or sentinels
  # (i.e., '=', ';', or '?') -- this is based on the assumption that
  # they are reserved characters.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required.

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (
                          ;                                  # Field 1
                          [0-9A-Za-z]{13}(?:[0-9A-Za-z]{3})? # Field 2
                          =                                  # Field 3
                          [0-9]{4}                           # Field 4
                          [0-9]{3}                           # Field 5
                          [0-9]{0,5}                         # Field 6
                          [^;=\?]*                           # Field 7
                          \?                                 # Field 8
                        ) # Save the match in $1.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}

######################################################################
#
# Dig4SSN
#
######################################################################

sub Dig4SSN
{
  my ($psData, $sDataOffset, $sReadOffset, $sFilename, $sFormat) = @_;

  ####################################################################
  #
  # A Social Security Number (SSN) consists of nine digits having the
  # following format: AAA-GG-SSSS. The first three digits (AAA) denote
  # the area (or State) where the application for the original SSN was
  # filed. Within each area, group numbers (GG) range from 01 to 99,
  # but they are not assigned in consecutive order. Within each group,
  # serial numbers (SSSS) run consecutively from 0001 through 9999.
  # This information was obtained from following references:
  #
  #   http://www.ssa.gov/employer/ssnweb.htm
  #   http://www.ssa.gov/employer/stateweb.htm
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required (i.e., a trailing lookahead is used).

  $sMatchOffset = 0;

  while (
          $$psData =~ m{
                        (?:^|[^0-9]) # The match must be preceded by nothing or anything but a digit.
                        (?!0{3}-?\d{2}-?\d{4}) # The match must not contain all zeros in the AAA field.
                        (?!\d{3}-?0{2}-?\d{4}) # The match must not contain all zeros in the GG field.
                        (?!\d{3}-?\d{2}-?0{4}) # The match must not contain all zeros in the SSSS field.
                        (\d{3}(?:-\d{2}-|\d{2})\d{4}) # Save the match in $1.
                        (?![0-9]) # The match must be followed by nothing or anything but a digit.
                      }gsxo
        )
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", EadFTimesUrlEncode($sFilename), $sAbsoluteOffset, EadFTimesUrlEncode($1));
  }

  return $sMatchOffset;
}


######################################################################
#
# DumpDomainInformation
#
######################################################################

sub DumpDomainInformation
{
  my ($sDumpRegex) = @_;

  ####################################################################
  #
  # Two letter domains: http://www.iana.org/cctld/cctld-whois.htm
  #
  ####################################################################

  my %hDomainList =
  (
    'AC'   => 'Ascension Island',
    'AD'   => 'Andorra',
    'AE'   => 'United Arab Emirates',
    'AF'   => 'Afghanistan',
    'AG'   => 'Antigua and Barbuda',
    'AI'   => 'Anguilla',
    'AL'   => 'Albania',
    'AM'   => 'Armenia',
    'AN'   => 'Netherlands Antilles',
    'AO'   => 'Angola',
    'AQ'   => 'Antarctica',
    'AR'   => 'Argentina',
    'AS'   => 'American Samoa',
    'AT'   => 'Austria',
    'AU'   => 'Australia',
    'AW'   => 'Aruba',
    'AZ'   => 'Azerbaijan',
    'BA'   => 'Bosnia and Herzegovina',
    'BB'   => 'Barbados',
    'BD'   => 'Bangladesh',
    'BE'   => 'Belgium',
    'BF'   => 'Burkina Faso',
    'BG'   => 'Bulgaria',
    'BH'   => 'Bahrain',
    'BI'   => 'Burundi',
    'BJ'   => 'Benin',
    'BM'   => 'Bermuda',
    'BN'   => 'Brunei Darussalam',
    'BO'   => 'Bolivia',
    'BR'   => 'Brazil',
    'BS'   => 'Bahamas',
    'BT'   => 'Bhutan',
    'BV'   => 'Bouvet Island',
    'BW'   => 'Botswana',
    'BY'   => 'Belarus',
    'BZ'   => 'Belize',
    'CA'   => 'Canada',
    'CC'   => 'Cocos (Keeling) Islands',
    'CF'   => 'Central African Republic',
    'CD'   => 'Congo, Democratic Republic of',
    'CG'   => 'Congo, Republic of',
    'CH'   => 'Switzerland',
    'CI'   => 'Cote d\'Ivoire (Ivory Coast)',
    'CK'   => 'Cook Islands',
    'CL'   => 'Chile',
    'CM'   => 'Cameroon',
    'CN'   => 'China',
    'CO'   => 'Colombia',
    'CR'   => 'Costa Rica',
    'CS'   => 'Czechoslovakia (former)',
    'CU'   => 'Cuba',
    'CV'   => 'Cap Verde',
    'CX'   => 'Christmas Island',
    'CY'   => 'Cyprus',
    'CZ'   => 'Czech Republic',
    'DE'   => 'Germany',
    'DJ'   => 'Djibouti',
    'DK'   => 'Denmark',
    'DM'   => 'Dominica',
    'DO'   => 'Dominican Republic',
    'DZ'   => 'Algeria',
    'EC'   => 'Ecuador',
    'EE'   => 'Estonia',
    'EG'   => 'Egypt',
    'EH'   => 'Western Sahara',
    'ER'   => 'Eritrea',
    'ES'   => 'Spain',
    'ET'   => 'Ethiopia',
    'FI'   => 'Finland',
    'FJ'   => 'Fiji',
    'FK'   => 'Falkland Islands (Malvina)',
    'FM'   => 'Micronesia, Federal State of',
    'FO'   => 'Faroe Islands',
    'FR'   => 'France',
    'FX'   => 'France, Metropolitan',
    'GA'   => 'Gabon',
    'GB'   => 'Great Britain (UK)',
    'GD'   => 'Grenada',
    'GE'   => 'Georgia',
    'GF'   => 'French Guiana',
    'GG'   => 'Guernsey',
    'GH'   => 'Ghana',
    'GI'   => 'Gibraltar',
    'GL'   => 'Greenland',
    'GM'   => 'Gambia',
    'GN'   => 'Guinea',
    'GP'   => 'Guadeloupe',
    'GQ'   => 'Equatorial Guinea',
    'GR'   => 'Greece',
    'GS'   => 'South Georgia and the South Sandwich Islands',
    'GT'   => 'Guatemala',
    'GU'   => 'Guam',
    'GW'   => 'Guinea-Bissau',
    'GY'   => 'Guyana',
    'HK'   => 'Hong Kong',
    'HM'   => 'Heard and McDonald Islands',
    'HN'   => 'Honduras',
    'HR'   => 'Croatia/Hrvatska',
    'HT'   => 'Haiti',
    'HU'   => 'Hungary',
    'ID'   => 'Indonesia',
    'IE'   => 'Ireland',
    'IL'   => 'Israel',
    'IM'   => 'Isle of Man',
    'IN'   => 'India',
    'IO'   => 'British Indian Ocean Territory',
    'IQ'   => 'Iraq',
    'IR'   => 'Iran (Islamic Republic of)',
    'IS'   => 'Iceland',
    'IT'   => 'Italy',
    'JE'   => 'Jersey',
    'JM'   => 'Jamaica',
    'JO'   => 'Jordan',
    'JP'   => 'Japan',
    'KE'   => 'Kenya',
    'KG'   => 'Kyrgyzstan',
    'KH'   => 'Cambodia',
    'KI'   => 'Kiribati',
    'KM'   => 'Comoros',
    'KN'   => 'Saint Kitts and Nevis',
    'KP'   => 'Korea, Democratic People\'s Republic',
    'KR'   => 'Korea, Republic of',
    'KW'   => 'Kuwait',
    'KY'   => 'Cayman Islands',
    'KZ'   => 'Kazakhstan',
    'LA'   => 'Lao People\'s Democratic Republic',
    'LB'   => 'Lebanon',
    'LC'   => 'Saint Lucia',
    'LI'   => 'Liechtenstein',
    'LK'   => 'Sri Lanka',
    'LR'   => 'Liberia',
    'LS'   => 'Lesotho',
    'LT'   => 'Lithuania',
    'LU'   => 'Luxembourg',
    'LV'   => 'Latvia',
    'LY'   => 'Libyan Arab Jamahiriya',
    'MA'   => 'Morocco',
    'MC'   => 'Monaco',
    'MD'   => 'Moldova, Republic of',
    'MG'   => 'Madagascar',
    'MH'   => 'Marshall Islands',
    'MK'   => 'Macedonia, Former Yugoslav Republic',
    'ML'   => 'Mali',
    'MM'   => 'Myanmar',
    'MN'   => 'Mongolia',
    'MO'   => 'Macau',
    'MP'   => 'Northern Mariana Islands',
    'MQ'   => 'Martinique',
    'MR'   => 'Mauritania',
    'MS'   => 'Montserrat',
    'MT'   => 'Malta',
    'MU'   => 'Mauritius',
    'MV'   => 'Maldives',
    'MW'   => 'Malawi',
    'MX'   => 'Mexico',
    'MY'   => 'Malaysia',
    'MZ'   => 'Mozambique',
    'NA'   => 'Namibia',
    'NC'   => 'New Caledonia',
    'NE'   => 'Niger',
    'NF'   => 'Norfolk Island',
    'NG'   => 'Nigeria',
    'NI'   => 'Nicaragua',
    'NL'   => 'Netherlands',
    'NO'   => 'Norway',
    'NP'   => 'Nepal',
    'NR'   => 'Nauru',
    'NT'   => 'Neutral Zone',
    'NU'   => 'Niue',
    'NZ'   => 'New Zealand',
    'OM'   => 'Oman',
    'PA'   => 'Panama',
    'PE'   => 'Peru',
    'PF'   => 'French Polynesia',
    'PG'   => 'Papua New Guinea',
    'PH'   => 'Philippines',
    'PK'   => 'Pakistan',
    'PL'   => 'Poland',
    'PM'   => 'St. Pierre and Miquelon',
    'PN'   => 'Pitcairn Island',
    'PR'   => 'Puerto Rico',
    'PS'   => 'Palestinian Territories',
    'PT'   => 'Portugal',
    'PW'   => 'Palau',
    'PY'   => 'Paraguay',
    'QA'   => 'Qatar',
    'RE'   => 'Reunion Island',
    'RO'   => 'Romania',
    'RU'   => 'Russian Federation',
    'RW'   => 'Rwanda',
    'SA'   => 'Saudi Arabia',
    'SB'   => 'Solomon Islands',
    'SC'   => 'Seychelles',
    'SD'   => 'Sudan',
    'SE'   => 'Sweden',
    'SG'   => 'Singapore',
    'SH'   => 'St. Helena',
    'SI'   => 'Slovenia',
    'SJ'   => 'Svalbard and Jan Mayen Islands',
    'SK'   => 'Slovak Republic',
    'SL'   => 'Sierra Leone',
    'SM'   => 'San Marino',
    'SN'   => 'Senegal',
    'SO'   => 'Somalia',
    'SR'   => 'Suriname',
    'ST'   => 'Sao Tome and Principe',
    'SU'   => 'USSR (former)',
    'SV'   => 'El Salvador',
    'SY'   => 'Syrian Arab Republic',
    'SZ'   => 'Swaziland',
    'TC'   => 'Turks and Caicos Islands',
    'TD'   => 'Chad',
    'TF'   => 'French Southern Territories',
    'TG'   => 'Togo',
    'TH'   => 'Thailand',
    'TJ'   => 'Tajikistan',
    'TK'   => 'Tokelau',
    'TM'   => 'Turkmenistan',
    'TN'   => 'Tunisia',
    'TO'   => 'Tonga',
    'TP'   => 'East Timor',
    'TR'   => 'Turkey',
    'TT'   => 'Trinidad and Tobago',
    'TV'   => 'Tuvalu',
    'TW'   => 'Taiwan',
    'TZ'   => 'Tanzania',
    'UA'   => 'Ukraine',
    'UG'   => 'Uganda',
    'UK'   => 'United Kingdom',
    'UM'   => 'US Minor Outlying Islands',
    'US'   => 'United States',
    'UY'   => 'Uruguay',
    'UZ'   => 'Uzbekistan',
    'VA'   => 'Holy See (City Vatican State)',
    'VC'   => 'Saint Vincent and the Grenadines',
    'VE'   => 'Venezuela',
    'VG'   => 'Virgin Islands (British)',
    'VI'   => 'Virgin Islands (USA)',
    'VN'   => 'Vietnam',
    'VU'   => 'Vanuatu',
    'WF'   => 'Wallis and Futuna Islands',
    'WS'   => 'Western Samoa',
    'YE'   => 'Yemen',
    'YT'   => 'Mayotte',
    'YU'   => 'Yugoslavia',
    'ZA'   => 'South Africa',
    'ZM'   => 'Zambia',
    'ZR'   => 'Zaire',
    'ZW'   => 'Zimbabwe',
    'COM'  => 'US Commercial',
    'EDU'  => 'US Educational',
    'GOV'  => 'US Government',
    'INT'  => 'International',
    'MIL'  => 'US Military',
    'NET'  => 'Network',
    'ORG'  => 'Non-Profit Organization',
    'ARPA' => 'Old style Arpanet',
    'NATO' => 'NATO'
  );

  if ($sDumpRegex)
  {
    foreach my $sLetter ("A" .. "W", "Y", "Z")
    {
      print $sLetter, "[";
      foreach my $sDomain (sort(keys(%hDomainList)))
      {
        if (length($sDomain) == 2 && $sDomain =~ /$sLetter(.)/)
        {
          print "$1";
        }
      }
      print "]|\n";
    }
    foreach my $sLength ("3", "4")
    {
      my (@aList);

      foreach my $sDomain (sort(keys(%hDomainList)))
      {
        if (length($sDomain) == $sLength)
        {
          push(@aList, $sDomain);
        }
      }
      print join("|", @aList);
      print (($sLength == 3) ? "|\n" : "\n");
    }
  }
  else
  {
    foreach my $sDomain (sort(keys(%hDomainList)))
    {
      print "$sDomain|$hDomainList{$sDomain}\n";
    }
  }

  return 1;
}


######################################################################
#
# DumpSSNInformation
#
######################################################################

sub DumpSSNInformation
{

  ####################################################################
  #
  # Social Security Area List.
  #
  # Reference:
  #
  #  http://www.ssa.gov/employer/stateweb.htm
  #
  ####################################################################

  my %hSSNAreaList =
  (
    '001-003' => 'New Hampshire',
    '004-007' => 'Maine',
    '008-009' => 'Vermont',
    '010-034' => 'Massachusetts',
    '035-039' => 'Rhode Island',
    '040-049' => 'Connecticut',
    '050-134' => 'New York',
    '135-158' => 'New Jersey',
    '159-211' => 'Pennsylvania',
    '212-220' => 'Maryland',
    '221-222' => 'Delaware',
    '223-231,691-699' => 'Virginia',
    '232,237-246,681-690' => 'North Carolina',
    '232-236' => 'West Virginia',
    '247-251,654-658' => 'South Carolina',
    '252-260,667-675' => 'Georgia',
    '261-267,589-595,766-772' => 'Florida',
    '268-302' => 'Ohio',
    '303-317' => 'Indiana',
    '318-361' => 'Illinois',
    '362-386' => 'Michigan',
    '387-399' => 'Wisconsin',
    '400-407' => 'Kentucky',
    '408-415,756-763' => 'Tennessee',
    '416-424' => 'Alabama',
    '425-428,587-588,752-755' => 'Mississippi',
    '429-432,676-679' => 'Arkansas',
    '433-439,659-665' => 'Louisiana',
    '440-448' => 'Oklahoma',
    '449-467,627-645' => 'Texas',
    '468-477' => 'Minnesota',
    '478-485' => 'Iowa',
    '486-500' => 'Missouri',
    '501-502' => 'North Dakota',
    '503-504' => 'South Dakota',
    '505-508' => 'Nebraska',
    '509-515' => 'Kansas',
    '516-517' => 'Montana',
    '518-519' => 'Idaho',
    '520'     => 'Wyoming',
    '521-524,650-653' => 'Colorado',
    '525,585,648-649' => 'New Mexico',
    '526-527,600-601,764-765' => 'Arizona',
    '528-529,646-647' => 'Utah',
    '530,680' => 'Nevada',
    '531-539' => 'Washington',
    '540-544' => 'Oregon',
    '545-573,602-626' => 'California',
    '574'     => 'Alaska',
    '575-576,750-751' => 'Hawaii',
    '577-579' => 'District of Columbia',
    '580'     => 'Virgin Islands',
    '580-584,596-599' => 'Puerto Rico',
    '586'     => 'American Samoa, Guam, and Philippine Islands',
    '700-728' => 'Railroad Board',
    '729-733' => 'Enumeration at Entry',
  );

  foreach my $sSSNArea (sort(keys(%hSSNAreaList)))
  {
    print "$sSSNArea|$hSSNAreaList{$sSSNArea}\n";
  }

  return 1;
}


######################################################################
#
# DumpStateInformation
#
######################################################################

sub DumpStateInformation
{

  ####################################################################
  #
  # State two-letter abbreviations.
  #
  ####################################################################

  my %hStateAbbreviationList =
  (
    'AK' => 'Alaska',
    'AL' => 'Alabama',
    'AR' => 'Arkansas',
    'AZ' => 'Arizona',
    'CA' => 'California',
    'CO' => 'Colorado',
    'CT' => 'Connecticut',
    'DC' => 'District of Columbia',
    'DE' => 'Delaware',
    'FL' => 'Florida',
    'GA' => 'Georgia',
    'HI' => 'Hawaii',
    'IA' => 'Iowa',
    'ID' => 'Idaho',
    'IL' => 'Illinois',
    'IN' => 'Indiana',
    'KS' => 'Kansas',
    'KY' => 'Kentucky',
    'LA' => 'Louisiana',
    'MA' => 'Massachusetts',
    'MD' => 'Maryland',
    'ME' => 'Maine',
    'MI' => 'Michigan',
    'MN' => 'Minnesota',
    'MO' => 'Missouri',
    'MS' => 'Mississippi',
    'MT' => 'Montana',
    'NC' => 'North Carolina',
    'ND' => 'North Dakota',
    'NE' => 'Nebraska',
    'NH' => 'New Hampshire',
    'NJ' => 'New Jersey',
    'NM' => 'New Mexico',
    'NV' => 'Nevada',
    'NY' => 'New York',
    'OH' => 'Ohio',
    'OK' => 'Oklahoma',
    'OR' => 'Oregon',
    'PA' => 'Pennsylvania',
    'RI' => 'Rhode Island',
    'SC' => 'South Carolina',
    'SD' => 'South Dakota',
    'TN' => 'Tennessee',
    'TX' => 'Texas',
    'UT' => 'Utah',
    'VA' => 'Virginia',
    'VT' => 'Vermont',
    'WA' => 'Washington',
    'WI' => 'Wisconsin',
    'WV' => 'West Virginia',
    'WY' => 'Wyoming',
  );

  foreach my $sStateAbbreviation (sort(keys(%hStateAbbreviationList)))
  {
    print "$sStateAbbreviation|$hStateAbbreviationList{$sStateAbbreviation}\n";
  }

  return 1;
}


######################################################################
#
# DumpEinInformation
#
######################################################################

sub DumpEinInformation
{

  ####################################################################
  #
  # Employer Identification Number Campus List.
  #
  # Reference:
  #
  #   http://www.irs.gov/businesses/small/article/0,,id=169067,00.html
  #
  # According to HUD, "EINs never begin with the following two-digit
  # prefixes: 00, 07, 08, 09, 17, 18, 19, 28, 29, 49, 78, or 79. All
  # other two-digit prefixes are valid."
  #
  # Reference:
  #
  #   http://www.hud.gov/offices/cpo/foia/tin.cfm
  #
  ####################################################################

  my %hEinCampusList =
  (
#   '00'    => 'N/V',
    '01-06,11,13-14,16,21-23,25,34,51-52,54-59,65' => 'Brookhaven',
#   '07-09' => 'N/V',
    '10,12' => 'Andover',
    '15,24' => 'Fresno',
#   '17-19' => 'N/V',
    '20,26-27' => 'Internet', # Note: Prefixes 26 and 27 were previously assigned by the Philadelphia campus.
#   '28-29' => 'N/V',
    '30,32,35-38,61' => 'Cincinnati',
    '31'    => 'Small Business Administration (SBS)',
    '33,39,41-43,45-48,62-64,66,68,71-77,81-88,91-93,98-99' => 'Philadelphia',
    '40,44' => 'Kansas City',
#   '49'    => 'N/V',
    '50,53' => 'Austin',
    '60,67' => 'Atlanta',
#   '69-70' => 'N/A',
#   '78-79' => 'N/V',
    '80,90' => 'Ogden',
#   '89'    => 'N/A',
    '94-95' => 'Memphis',
#   '96-97' => 'N/A',
  );

  foreach my $sEinCampus (sort(keys(%hEinCampusList)))
  {
    print "$sEinCampus|$hEinCampusList{$sEinCampus}\n";
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
  print STDERR "Usage: $sProgram [-D type] [-l stdin-label] [-o option[,option[,...]]] [-r read-buffer-size] [-s save-buffer-size] [-T dig-tag] [-t {type|custom[=~]regexp}] -- {-|file} [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hipdig.pl - Dig for hosts, IPs, passwords, and more...

=head1 SYNOPSIS

B<hipdig.pl> B<[-D type]> B<-l stdin-label> B<[-o option[,option[,...]]]> B<[-r read-buffer-size]> B<[-s save-buffer-size]> B<[-T dig-tag]> B<[-t {type|custom[=~]regexp}]> -- B<{-|file} [file ...]>

=head1 DESCRIPTION

This utility performs regular expression searches across one or
more files.  Output is written to stdout in FTimes dig format which
has the following fields:

    name|type|tag|offset|string

where name and string are the FTimes-encoded form of the raw data.

Feeding the output of this utility to ftimes-dig2ctx(1) allows you
to extract a variable amount of context surrounding each hit.

Feeding the output of this utility to ftimes-xformer(1) allows you
to isolate and/or manipulate field data.  Note that previous versions
of this script would print offsets in hexadecimal when the B<-H>
option was set.  Since that option is no longer supported, below is
an example of how you may achieve the equivalent result:

    hipdig.pl ... | ftimes-xformer -l name,type,tag,hex_offset,string \
        -o ParseOffset -f -

Filenames supplied as arguments may be expressed as either a
native or FTimes-encoded path/name.  If the latter form is used,
the path/name must be prefixed with 'file://' as shown in the
example below.

    file://some/path/that+has+been/neutered%25.txt

=head1 OPTIONS

=over 4

=item B<-D>

Dump the specified type information to stdout and exit.  Currently,
the following types are supported: {DOMAIN|HOST}, DOMAIN_REGEX,
{EIN|TIN}, {SSN|SOCIAL}, and STATE.

=item B<-l stdin-label>

Specifies an alternate label to use instead of "-" when digging
on stdin.

=item B<-o option,[option[,...]]>

Specifies the list of options to apply.  Currently, the following
options are supported:

=over 4

=item BeQuiet

Don't report warnings (i.e., be quiet) while processing files.

=item MadMode

Alter the output format to match FTimes made mode output.

=item NoHeader

Don't print an output header.

=item RegularFilesOnly

Operate on regular files only (i.e., no directories, specials, etc.).
Note that a symbolic link that resolve to a regular file is allowed.

=back

=item B<-r read-buffer-size>

Specifies the read buffer size.  The default value is 32,768 bytes.

=item B<-s save-buffer-size>

Specifies the save buffer size.  This is the maximum number of bytes
to carry over from one search buffer to the next.  The default value
is 64 bytes.  This value is limited to 1/10th the read buffer size.

=item B<-T dig-tag>

Specifies a tag that is assigned to each dig string.  This option is
intended for use with the CUSTOM search type since internally-defined
search types have a default tag value.  Note however, that the default
tag value is trumped by this value, if specified.

=item B<-t {type|custom[=~]regexp}>

Specifies the type of search that is to be conducted.  Currently,
the following types are supported: CUSTOM, HOST, IP, {PASS|PASSWORD},
{SSN|SOCIAL}, {T1|TRACK1}, {T1S|TRACK1-STRICT}, {T2|TRACK2}, and
{T2S|TRACK2-STRICT}.  The default value is IP.  The value for this
option is not case sensitive.

If the specified type is CUSTOM, then it must be accompanied by
a valid Perl regular expression.  The required format for this
argument is:

    custom=<regex>

or

    custom~<regex>

where B<custom> is the literal string 'custom' and B<regex> is Perl
regular expression.  Note that if the '=' operator is specified,
then the expression is automatically wrapped in a set of capturing
parentheses such that $1 will be populated upon a successful match.
If you wish to control what constitutes $1 (i.e., either the entire
match or a particular submatch), you must use the '~' operator and
explicitly place at least one set of capturing parentheses in the
expression.

Any whitespace surrounding these tokens is ignored, but whitespace
within <regexp> is not.  Proper quoting is essential when specifying
custom expressions.  When in doubt, use single quotes like so:

    'custom=(?i)abc 123'

or

    'custom~(?i)abc (123)'

To control $1 when more than one set of parentheses is required
for grouping, use '?:' as demonstrated in the following example,
which only returns '123' in $1 upon a successful match.

    'custom~(?i)(?:abc|def) (123) (?:pdq|xyz)'

Since searches are block-oriented, the use of begin/end anchors
(i.e., '^' and '$') are of little value unless the pattern you seek
is know to begin or end on a block boundary.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), ftimes-dig2ctx(1), ftimes-xformer(1)

=head1 LICENSE

All documentation and code are distributed under same terms and
conditions as FTimes.

=cut
