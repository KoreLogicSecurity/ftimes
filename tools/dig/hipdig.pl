#!/usr/bin/perl -w
######################################################################
#
# $Id: hipdig.pl,v 1.3 2003/03/26 21:39:17 mavrik Exp $
#
######################################################################
#
# Copyright 2001-2003 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Dig for hostnames, IPs, or encrypted passwords.
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

  $program = "hipdig.pl";

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my (%options);

  if (!getopts('DhqRrs:t:', \%options))
  {
    Usage($program);
  }

  ####################################################################
  #
  # The dump domain information flag, '-D', is optional.
  #
  ####################################################################

  if (exists($options{'D'}))
  {
    DumpDomainInformation(0);
    exit(0);
  }

  ####################################################################
  #
  # The printHeader flag, '-h', is optional. Default value is 0.
  #
  ####################################################################

  my ($printHeader);

  $printHeader = (exists($options{'h'})) ? 1 : 0;

  ####################################################################
  #
  # The beQuiet flag, '-q', is optional. Default value is 0.
  #
  ####################################################################

  my ($beQuiet);

  $beQuiet = (exists($options{'q'})) ? 1 : 0;

  ####################################################################
  #
  # The dump domain regex information flag, '-R', is optional.
  #
  ####################################################################

  if (exists($options{'R'}))
  {
    DumpDomainInformation(1);
    exit(0);
  }

  ####################################################################
  #
  # The regularOnly flag, '-r', is optional. Default value is 0.
  #
  ####################################################################

  my ($regularOnly);

  $regularOnly = (exists($options{'r'})) ? 1 : 0;

  ####################################################################
  #
  # The saveLength flag, '-s', is optional. Default value is 64.
  #
  ####################################################################

  my ($saveLength);

  $saveLength = (exists($options{'s'})) ? $options{'s'} : 64;

  if ($saveLength !~ /^\d{1,10}$/ || $saveLength <= 0)
  {
    print STDERR "$program: SaveLength='$saveLength' Error='Invalid save length.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The digType flag, '-t', is optional. Default value is "IP".
  #
  ####################################################################

  my ($digType, $digRoutine);

  $digType = (exists($options{'t'})) ? uc($options{'t'}) : "IP";

  if ($digType =~ /^IP$/)
  {
    $digRoutine = \&Dig4IPs;
  }
  elsif ($digType =~ /^HOST$/)
  {
    $digRoutine = \&Dig4Domains;
  }
  elsif ($digType =~ /^(PASS|PASSWORD)$/)
  {
    $digRoutine = \&Dig4Passwords;
  }
  else
  {
    print STDERR "$program: DigType='$digType' Error='Invalid dig type.'\n";
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
  # Print out a header line.
  #
  ####################################################################

  if ($printHeader)
  {
    print "name|offset|string\n";
  }

  ####################################################################
  #
  # Iterate over input files.
  #
  ####################################################################

  foreach my $file (@ARGV)
  {
    if ($regularOnly && !-f $file)
    {
      if (!$beQuiet)
      {
        print STDERR "$program: File='$file' Error='File is not regular.'\n";
      }
      next;
    }

    if (!open(FH, "<$file"))
    {
      if (!$beQuiet)
      {
        print STDERR "$program: File='$file' Error='$!'\n";
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

    my ($data, $dataOffset, $lastOffset, $nRead, $readOffset, $dataLength);

    $readOffset = $dataOffset = 0;

    while ($nRead = read(FH, $data, 0x8000, $dataOffset))
    {
      $dataLength = $dataOffset + $nRead;
      $lastOffset = &$digRoutine(\$data, $dataOffset, $readOffset, $file);
      $dataOffset = ($dataLength - $lastOffset > $saveLength) ? $saveLength : $dataLength - $lastOffset;
      $data = substr($data, ($dataLength - $dataOffset), $dataOffset);
      $readOffset += $nRead;
    }

    if (!defined($nRead))
    {
      if (!$beQuiet)
      {
        print STDERR "$program: File='$file' Error='$!'\n";
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
  my ($pData, $dataOffset, $readOffset, $filename) = @_;

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

  my ($absoluteOffset, $matchOffset);

  $matchOffset = 0;

  while (
          $$pData =~ m{
                        ( # Remember the matched string as $1.
                          (?:[\w-]{1,}\.)+ # One or more of (one or more (alphanumeric or hyphen) followed by a dot)
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
                          )\.? # Fully qualified domain names will end with a dot
                        )[^\w\.] # The match must not be followed by an alphanumeric or a dot
                      }gsxio
        )
  {
    $matchOffset = pos($$pData);
    $absoluteOffset = ($readOffset - $dataOffset) + ($matchOffset - (length($1) + 1));
    print "\"$filename\"|$absoluteOffset|$1\n";
  }

  return $matchOffset;
}


######################################################################
#
# Dig4IPs
#
######################################################################

sub Dig4IPs
{
  my ($pData, $dataOffset, $readOffset, $filename) = @_;

  ####################################################################
  #
  # Select candidate IPs that are bound to the right by a character
  # that is not a dot or a digit. Set the g flag to match all IPs
  # in the buffer. Set the s flag to ignore newline boundaries. Set
  # the x flag to allow white space in the expression. Set the o
  # flag to compile the expression once.
  #
  ####################################################################

  my ($absoluteOffset, $matchOffset);

  $matchOffset = 0;

  while (
          $$pData =~ m{
                        ( # Remember the matched string as $1.
                          (?:[1-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])\.
                          (?:[0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-5][0-9])
                        )[^0-9\.] # The match must not be followed by a digit or a dot
                      }gsxo
        )
  {
    $matchOffset = pos($$pData);
    $absoluteOffset = ($readOffset - $dataOffset) + ($matchOffset - (length($1) + 1));
    print "\"$filename\"|$absoluteOffset|$1\n";
  }

  return $matchOffset;
}


######################################################################
#
# Dig4Passwords
#
######################################################################

sub Dig4Passwords
{
  my ($pData, $dataOffset, $readOffset, $filename) = @_;

  ####################################################################
  #
  # Select candidate passwords that are bound by a colon on either
  # side. Set the g flag to match all passwords in the buffer. Set
  # the s flag to ignore newline boundaries. Set the x flag to allow
  # white space in the expression. Set the o flag to compile the
  # expression once.
  #
  ####################################################################

  my ($absoluteOffset, $matchOffset);

  $matchOffset = 0;

  while (
          $$pData =~ m{
                        : # The match must be preceded by a colon
                        (
                          \$[1-9]\$[\.\/0-9a-zA-Z]{8}\$[\.\/0-9a-zA-Z]{22}| # Modular crypt
                          [\.\/0-9a-zA-Z]{13}(?:,[\.\/0-9a-zA-Z]{4})? # Old crypt
                        )
                        : # The match must be followed by a colon
                      }gsxo
        )
  {
    $matchOffset = pos($$pData);
    $absoluteOffset = ($readOffset - $dataOffset) + ($matchOffset - (length($1) + 1));
    print "\"$filename\"|$absoluteOffset|$1\n";
  }

  return $matchOffset;
}


######################################################################
#
# DumpDomainInformation
#
######################################################################

sub DumpDomainInformation
{
  my ($dumpRegex) = @_;

  ####################################################################
  #
  # Two letter domains: http://www.iana.org/cctld/cctld-whois.htm
  #
  ####################################################################

  my %domainList =
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

  if ($dumpRegex)
  {
    foreach my $letter ("A" .. "W", "Y", "Z")
    {
      print $letter, "[";
      foreach my $domain (sort(keys(%domainList)))
      {
        if (length($domain) == 2 && $domain =~ /$letter(.)/)
        {
          print "$1";
        }
      }
      print "]|\n";
    }
    foreach my $length ("3", "4")
    {
      my (@list);

      foreach my $domain (sort(keys(%domainList)))
      {
        if (length($domain) == $length)
        {
          push(@list, $domain);
        }
      }
      print join("|", @list);
      print (($length == 3) ? "|\n" : "\n");
    }
  }
  else
  {
    foreach my $domain (sort(keys(%domainList)))
    {
      print "$domain|$domainList{$domain}\n";
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
  my ($program) = @_;
  print STDERR "\n";
  print STDERR "Usage: $program [-D] [-h] [-q] [-R] [-r] [-s length] [-t type] file [file ...]\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

hipdig.pl - Dig for hostnames, IPs, or encrypted passwords

=head1 SYNOPSIS

B<hipdig.pl> B<[-D]> B<[-h]> B<[-q]> B<[-R]> B<[-r]> B<[-s length]> B<[-t type]> B<file [file ...]>

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
the following types are supported: HOST, IP, and PASS|PASSWORD.
The default value is IP. The value for this option is not case
sensitive.

=back

=head1 AUTHOR

Klayton Monroe, klm@ir.exodus.net

=head1 SEE ALSO

ftimes(1), ftimes-dig2ctx.pl

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
