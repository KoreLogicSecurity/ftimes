#!/usr/bin/perl -w
######################################################################
#
# $Id: hipdig.pl,v 1.36 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Dig for hosts, IPs, passwords, and more...
#
######################################################################

use strict;
use File::Basename;
use Getopt::Std;
use vars qw($sDigStringRegExp);

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

  if (!getopts('D:HhqRrs:T:t:x', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The DumpType flag, '-D', is optional.
  #
  ####################################################################

  my ($sDumpType);

  $sDumpType = (exists($hOptions{'D'})) ? $hOptions{'D'} : undef;

  if (defined($sDumpType))
  {
    if ($sDumpType =~ /^DOMAIN|HOST$/i)
    {
      DumpDomainInformation(0);
      exit(0);
    }
    elsif ($sDumpType =~ /^(SSN|SOCIAL)$/i)
    {
      DumpSSNInformation();
      exit(0);
    }
    else
    {
      print STDERR "$sProgram: DumpType='$sDumpType' Error='Invalid dump type.'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # The PrintHex flag, '-H', is optional.
  #
  ####################################################################

  my ($sPrintHex);

  $sPrintHex = (exists($hOptions{'H'})) ? 1 : 0;

  ####################################################################
  #
  # The PrintHeader flag, '-h', is optional.
  #
  ####################################################################

  my ($sPrintHeader);

  $sPrintHeader = (exists($hOptions{'h'})) ? 1 : 0;

  ####################################################################
  #
  # The BeQuiet flag, '-q', is optional.
  #
  ####################################################################

  my ($sBeQuiet);

  $sBeQuiet = (exists($hOptions{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The DumpDomainRegexInformation flag, '-R', is optional.
  #
  ####################################################################

  if (exists($hOptions{'R'}))
  {
    DumpDomainInformation(1);
    exit(0);
  }

  ####################################################################
  #
  # The RegularOnly flag, '-r', is optional.
  #
  ####################################################################

  my ($sRegularOnly);

  $sRegularOnly = (exists($hOptions{'r'})) ? 1 : 0;

  ####################################################################
  #
  # The SaveLength flag, '-s', is optional.
  #
  ####################################################################

  my ($sSaveLength);

  $sSaveLength = (exists($hOptions{'s'})) ? $hOptions{'s'} : 64;

  if ($sSaveLength !~ /^\d{1,10}$/ || $sSaveLength <= 0)
  {
    print STDERR "$sProgram: SaveLength='$sSaveLength' Error='Invalid save length.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The DigTag flag, '-T', is optional.
  #
  ####################################################################

  my ($sDigTag);

  $sDigTag = (exists($hOptions{'T'})) ? $hOptions{'T'} : undef;

  ####################################################################
  #
  # The DigType flag, '-t', is optional.
  #
  ####################################################################

  my ($sDigType, $sDigRoutine);

  $sDigType = (exists($hOptions{'t'})) ? $hOptions{'t'} : "IP";

  if ($sDigType =~ /^\s*CUSTOM\s*=\s*(.+)\s*$/i)
  {
    $sDigStringRegExp = $1;
    $sDigRoutine = \&Dig4Custom;
    $sDigTag = "" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^HOST$/i)
  {
    $sDigRoutine = \&Dig4Domains;
    $sDigTag = "domain" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^IP$/i)
  {
    $sDigRoutine = \&Dig4IPs;
    $sDigTag = "ip" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(PASS|PASSWORD)$/i)
  {
    $sDigRoutine = \&Dig4Passwords;
    $sDigTag = "password" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(SSN|SOCIAL)$/i)
  {
    $sDigRoutine = \&Dig4SSN;
    $sDigTag = "ssn" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(T1|TRACK1)$/i)
  {
    $sDigRoutine = \&Dig4Track1;
    $sDigTag = "track1" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(T1S|TRACK1-STRICT)$/i)
  {
    $sDigRoutine = \&Dig4Track1Strict;
    $sDigTag = "track1strict" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(T2|TRACK2)$/i)
  {
    $sDigRoutine = \&Dig4Track2;
    $sDigTag = "track2" if (!defined($sDigTag));
  }
  elsif ($sDigType =~ /^(T2S|TRACK2-STRICT)$/i)
  {
    $sDigRoutine = \&Dig4Track2Strict;
    $sDigTag = "track2strict" if (!defined($sDigTag));
  }
  else
  {
    print STDERR "$sProgram: DigType='$sDigType' Error='Invalid dig type.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The Expert flag, '-x', is optional.
  #
  ####################################################################

  my ($sExpert);

  $sExpert = (exists($hOptions{'x'})) ? 1 : 0;

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
  # Check/Finalize custom dig string expression. If expert mode is
  # not enabled, wrap the expression in '()'s to preserve $1. Then,
  # eval the expression to ensure that Perl groks it.
  #
  ####################################################################

  if (defined($sDigStringRegExp))
  {
    $sDigStringRegExp = "($sDigStringRegExp)" if (!$sExpert);
    eval { "This is a test." =~ m/$sDigStringRegExp/gso; };
    if ($@)
    {
      chomp($@);
      print STDERR "$sProgram: Error='Expression check failed: $@'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # Set print format.
  #
  ####################################################################

  my ($sFormat);

  if ($sPrintHex)
  {
    $sFormat = "\"%s\"|regexp|$sDigTag|0x%x|%s\n";
  }
  else
  {
    $sFormat = "\"%s\"|regexp|$sDigTag|%d|%s\n";
  }

  ####################################################################
  #
  # Print out a header line.
  #
  ####################################################################

  if ($sPrintHeader)
  {
    print "name|type|tag|offset|string\n";
  }

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  foreach my $sFile (@ARGV)
  {
    if ($sRegularOnly && !-f $sFile)
    {
      if (!$sBeQuiet)
      {
        print STDERR "$sProgram: File='$sFile' Error='File is not regular.'\n";
      }
      next;
    }

    if (!open(FH, "< $sFile"))
    {
      if (!$sBeQuiet)
      {
        print STDERR "$sProgram: File='$sFile' Error='$!'\n";
      }
      next;
    }

    binmode(FH); # Enable binmode for WIN32 platforms.

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

    while ($sNRead = read(FH, $sData, 0x8000, $sDataOffset))
    {
      $sDataLength = $sDataOffset + $sNRead;
      $sLastOffset = &$sDigRoutine(\$sData, $sDataOffset, $sReadOffset, $sFile, $sFormat);
      $sDataOffset = ($sDataLength - $sLastOffset > $sSaveLength) ? $sSaveLength : $sDataLength - $sLastOffset;
      $sData = substr($sData, ($sDataLength - $sDataOffset), $sDataOffset);
      $sReadOffset += $sNRead;
    }

    if (!defined($sNRead))
    {
      if (!$sBeQuiet)
      {
        print STDERR "$sProgram: File='$sFile' Error='$!'\n";
      }
    }

    close(FH);
  }

  ####################################################################
  #
  # Cleanup and go home.
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
  # is expected to have at least one set of capturing parentheses.
  #
  ####################################################################

  my ($sAbsoluteOffset, $sAdjustment, $sMatchOffset);

  $sAdjustment = 0; # No adjustment is required -- this is a custom dig.

  $sMatchOffset = 0;

  while ($$psData =~ m/$sDigStringRegExp/gso)
  {
    $sMatchOffset = pos($$psData);
    $sAbsoluteOffset = ($sReadOffset - $sDataOffset) + ($sMatchOffset - (length($1) + $sAdjustment));
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, UrlEncode($1));
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
    '223-231' => 'Virginia',
    '232'     => 'North Carolina',
    '232-236' => 'West Virginia',
    '237-246' => '',
    '247-251' => 'South Carolina',
    '252-260' => 'Georgia',
    '261-267' => 'Florida',
    '268-302' => 'Ohio',
    '303-317' => 'Indiana',
    '318-361' => 'Illinois',
    '362-386' => 'Michigan',
    '387-399' => 'Wisconsin',
    '400-407' => 'Kentucky',
    '408-415' => 'Tennessee',
    '416-424' => 'Alabama',
    '425-428' => 'Mississippi',
    '429-432' => 'Arkansas',
    '433-439' => 'Louisiana',
    '440-448' => 'Oklahoma',
    '449-467' => 'Texas',
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
    '521-524' => 'Colorado',
    '525,585' => 'New Mexico',
    '526-527' => 'Arizona',
    '528-529' => 'Utah',
    '530'     => 'Nevada',
    '531-539' => 'Washington',
    '540-544' => 'Oregon',
    '545-573' => 'California',
    '574'     => 'Alaska',
    '575-576' => 'Hawaii',
    '577-579' => 'District of Columbia',
    '580'     => 'Virgin Islands',
    '580-584' => 'Puerto Rico',
    '586'     => 'American Samoa, Guam, and Philippine Islands',
    '587-588' => '',
    '589-595' => '',
    '596-599' => '',
    '600-601' => '',
    '602-626' => '',
    '627-645' => '',
    '646-647' => '',
    '648-649' => '',
    '650-653' => '',
    '654-658' => '',
    '659-665' => '',
    '667-675' => '',
    '676-679' => '',
    '680'     => '',
    '681-690' => '',
    '691-699' => '',
    '700-728' => 'Railroad Board',
    '729-733' => 'Enumeration at Entry',
    '750-751' => '',
    '752-755' => '',
    '756-763' => '',
    '764-765' => '',
    '766-772' => '',
  );

  foreach my $sSSNArea (sort(keys(%hSSNAreaList)))
  {
    print "$sSSNArea|$hSSNAreaList{$sSSNArea}\n";
  }

  return 1;
}


######################################################################
#
# UrlEncode
#
######################################################################

sub UrlEncode
{
  my ($sData) = @_;

  $sData =~ s/([^ -\$&-*,-{}~])/sprintf("%%%02x",unpack('C',$1))/seg;
  $sData =~ s/ /+/sg;
  return $sData;
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
  print STDERR "Usage: $sProgram [-HhqRrx] [-D type] [-s length] [-T tag] [-t {type|custom=regexp}] file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hipdig.pl - Dig for hosts, IPs, passwords, and more...

=head1 SYNOPSIS

B<hipdig.pl> B<[-HhqRrx]> B<[-D type]> B<[-s length]> B<[-T tag]> B<[-t {type|custom=regexp}]> B<file [file ...]>

=head1 DESCRIPTION

This utility performs regular expression searches across one or
more files. Output is written to stdout in FTimes dig format which
has the following fields:

    name|type|offset|string

where string is the URL encoded form of the raw data.

Feeding the output of this utility to ftimes-dig2ctx(1) allows you
to extract a variable amount of context surrounding each hit.

=head1 OPTIONS

=over 4

=item B<-D>

Dump the specified type information to stdout and exit. Currently,
the following types are supported: DOMAIN|HOST and SSN|SOCIAL.

=item B<-H>

Print offsets in hex.  If not set, offsets will be printed in
decimal.

=item B<-h>

Print a header line.

=item B<-q>

Don't report errors (i.e., be quiet) while processing files.

=item B<-R>

Dump domain regex information to stdout and exit.

=item B<-r>

Operate on regular files only.

=item B<-s length>

Specifies the save length. This is the maximum number of bytes to
carry over from one search buffer to the next.

=item B<-T tag>

Specifies a tag that is used to identify the dig string.  Each
internally defined search type has a default tag value.  This option
would typically be used to assign a tag to a CUSTOM search type.

Note: The default tag, if any, is trumped by this value.

=item B<-t {type|custom=regexp}>

Specifies the type of search that is to be conducted. Currently,
the following types are supported: CUSTOM, HOST, IP, PASS|PASSWORD,
SSN|SOCIAL, T1|TRACK1, T1S|TRACK1-STRICT, T2|TRACK2, and
T2S|TRACK2-STRICT.  The default value is IP.  The value for this
option is not case sensitive.

If the specified type is CUSTOM, then it must be accompanied by a
valid regular expression. The required format for this argument is:

    custom = <regexp>

Any whitespace surrounding these tokens is ignored, but whitespace
within <regexp> is not.  Proper quoting is essential when specifying
custom expressions.  When in doubt, use single quotes like so:

    'custom=(?i)abc123'

Custom expressions are automatically wrapped in a single set of
capturing parentheses.  Therefore, the value of $1 (i.e., the entire
pattern) is copied directly to the output stream.  You can control
which subpattern constitutes $1 by enabling expert mode (see B<-x>).

=item B<-x>

Enable expert mode.  When this mode is active, custom expressions
are not automatically wrapped in a single set of capturing parentheses.
However, since $1 is still required, you must specify at least one
set of capturing parentheses in your expression.  For example, the
following expression allows you to match on the string '123' when
it is prefixed by any form of 'abc' or 'def':

    'custom=(?i)(?:abc|def)(123)'

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), ftimes-dig2ctx(1)

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
