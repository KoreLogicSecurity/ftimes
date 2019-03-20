#!/usr/bin/perl -w
######################################################################
#
# $Id: hipdig.pl,v 1.20 2004/04/26 20:12:46 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2004 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Dig for hostnames, IPs, or encrypted passwords.
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

  my ($sProgram);

  $sProgram = basename(__FILE__);

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%hOptions);

  if (!getopts('DHhqRrs:t:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # The DumpDomainInformation flag, '-D', is optional.
  #
  ####################################################################

  if (exists($hOptions{'D'}))
  {
    DumpDomainInformation(0);
    exit(0);
  }

  ####################################################################
  #
  # The PrintHex flag, '-H', is optional. Default value is 0.
  #
  ####################################################################

  my ($sPrintHex);

  $sPrintHex = (exists($hOptions{'H'})) ? 1 : 0;

  ####################################################################
  #
  # The PrintHeader flag, '-h', is optional. Default value is 0.
  #
  ####################################################################

  my ($sPrintHeader);

  $sPrintHeader = (exists($hOptions{'h'})) ? 1 : 0;

  ####################################################################
  #
  # The BeQuiet flag, '-q', is optional. Default value is 0.
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
  # The RegularOnly flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($sRegularOnly);

  $sRegularOnly = (exists($hOptions{'r'})) ? 1 : 0;

  ####################################################################
  #
  # The SaveLength flag, '-s', is optional. Default value is 64.
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
  # The DigType flag, '-t', is optional. Default value is "IP".
  #
  ####################################################################

  my ($sDigType, $sDigRoutine);

  $sDigType = (exists($hOptions{'t'})) ? uc($hOptions{'t'}) : "IP";

  if ($sDigType =~ /^IP$/)
  {
    $sDigRoutine = \&Dig4IPs;
  }
  elsif ($sDigType =~ /^HOST$/)
  {
    $sDigRoutine = \&Dig4Domains;
  }
  elsif ($sDigType =~ /^(PASS|PASSWORD)$/)
  {
    $sDigRoutine = \&Dig4Passwords;
  }
  elsif ($sDigType =~ /^(T1|TRACK1)$/)
  {
    $sDigRoutine = \&Dig4Track1;
  }
  elsif ($sDigType =~ /^(T1S|TRACK1-STRICT)$/)
  {
    $sDigRoutine = \&Dig4Track1Strict;
  }
  elsif ($sDigType =~ /^(T2|TRACK2)$/)
  {
    $sDigRoutine = \&Dig4Track2;
  }
  elsif ($sDigType =~ /^(T2S|TRACK2-STRICT)$/)
  {
    $sDigRoutine = \&Dig4Track2Strict;
  }
  elsif ($sDigType =~ /^(SSN|SOCIAL)$/)
  {
    $sDigRoutine = \&Dig4SSN;
  }
  else
  {
    print STDERR "$sProgram: DigType='$sDigType' Error='Invalid dig type.'\n";
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
  # Set print format.
  #
  ####################################################################

  my ($sFormat);

  if ($sPrintHex)
  {
    $sFormat = "\"%s\"|0x%x|%s\n";
  }
  else
  {
    $sFormat = "\"%s\"|%d|%s\n";
  }

  ####################################################################
  #
  # Print out a header line.
  #
  ####################################################################

  if ($sPrintHeader)
  {
    print "name|offset|string\n";
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

    if (!open(FH, "<$sFile"))
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
  # (i.e. '^', '%', or '?') -- this is based on the assumption that
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
  # (i.e. '=', ';', or '?') -- this is based on the assumption that
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
    printf("$sFormat", $sFilename, $sAbsoluteOffset, $1);
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
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-D] [-H] [-h] [-q] [-R] [-r] [-s length] [-t type] file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hipdig.pl - Dig for hostnames, IPs, or encrypted passwords

=head1 SYNOPSIS

B<hipdig.pl> B<[-D]> B<[-H]> B<[-h]> B<[-q]> B<[-R]> B<[-r]> B<[-s length]> B<[-t type]> B<file [file ...]>

=head1 DESCRIPTION

This utility performs regular expression searches across one or
more files. Output is written to stdout in FTimes dig format which
has the following fields:

    name|offset|string

Feeding the output of this utility to ftimes-dig2ctx.pl allows you
to extract a variable amount of context surrounding each hit.

=head1 OPTIONS

=over 4

=item B<-D>

Dump known domain information to stdout and exit.

=item B<-H>

Print offsets in hex.  If not set, offsets will be printed in
decimal.

=item B<-h>

Print a header line.

=item B<-q>

Don't report errors (i.e. be quiet) while processing files.

=item B<-R>

Dump domain regex information to stdout and exit.

=item B<-r>

Operate on regular files only.

=item B<-s length>

Specifies the save length. This is the maximum number of bytes to
carry over from one search buffer to the next.

=item B<-t type>

Specifies the type of search that is to be conducted. Currently,
the following types are supported: HOST, IP, PASS|PASSWORD, SSN|SOCIAL,
T1|TRACK1, T1S|TRACK1-STRICT, T2|TRACK2, and T2S|TRACK2-STRICT.
The default value is IP.  The value for this option is not case
sensitive.

=back

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1), ftimes-dig2ctx.pl

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
