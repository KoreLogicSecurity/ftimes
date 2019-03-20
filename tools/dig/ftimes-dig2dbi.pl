#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-dig2dbi.pl,v 1.28 2014/07/18 06:40:44 mavrik Exp $
#
######################################################################
#
# Copyright 2006-2014 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Preprocess FTimes dig data for MySQL DB import.
#
######################################################################

use strict;
use Cwd;
use Digest::MD5 qw(md5_hex);
use File::Basename;
use Getopt::Std;

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

  ####################################################################
  #
  # Define helper routines.
  #
  ####################################################################

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

  my $phProperties = GetProperties();

  $$phProperties{'Program'} = basename(__FILE__);

  ####################################################################
  #
  # Validation expressions.
  #
  ####################################################################

  my $sDbRegex       = qq(^[A-Za-z][A-Za-z0-9_]*\$);
  my $sHostnameRegex = qq(^[\\w\\.-]{1,64}\$);
  my $sMaxRowsRegex  = qq(^\\d+\$);
  my $sTableRegex    = qq(^[A-Za-z][A-Za-z0-9_]*\$);

  ####################################################################
  #
  # SQL table creation parameters.
  #
  ####################################################################

  my %hTableLayout =
  (
    'hostname'      => "varbinary(64) not null, index hostname_index (hostname)",
    'name'          => "blob not null, index name_index (name(255))",
    'type'          => "varbinary(16) not null, index type_index (type)",
    'tag'           => "varbinary(64) null, index tag_index (tag)",
    'offset'        => "bigint unsigned not null, index offset_index (offset)",
    'string'        => "blob null, index string_index (string(255))",
    'joiner'        => "varbinary(32) not null, index joiner_index (joiner), index primary_index (joiner, offset)",
  );

  ####################################################################
  #
  # Note: The primary index defined above in not a primary key -- we
  # can't guarantee that the primary index will be unique across all
  # records since more than one dig string could have a match on the
  # same host in the same file at the same offset.
  #
  ####################################################################

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my %hOptions;

  if (!getopts('D:d:f:h:m:o:t:', \%hOptions))
  {
    Usage($$phProperties{'Program'});
  }

  ####################################################################
  #
  # The duplicates setting, '-D', is optional.
  #
  ####################################################################

  $$phProperties{'Duplicates'} = (exists($hOptions{'D'})) ? $hOptions{'D'} : "default";

  if ($$phProperties{'Duplicates'} !~ /^(default|ignore|replace)$/i)
  {
    print STDERR "$$phProperties{'Program'}: Error='Invalid duplicates setting ($$phProperties{'Duplicates'}). Use one of \"default\", \"ignore\", or \"replace\".'\n";
    exit(2);
  }

  ####################################################################
  #
  # A database, '-d', is optional.
  #
  ####################################################################

  $$phProperties{'Database'} = (exists($hOptions{'d'})) ? $hOptions{'d'} : "ftimes";

  if ($$phProperties{'Database'} !~ /$sDbRegex/)
  {
    print STDERR "$$phProperties{'Program'}: Database='$hOptions{'d'}' Regex='$sDbRegex' Error='Invalid db name.'\n";
    exit(2);
  }

  ####################################################################
  #
  # An input file, '-f', is required. It can be '-' or a regular file.
  #
  ####################################################################

  my ($sFileHandle);

  if (!exists($hOptions{'f'}))
  {
    Usage($$phProperties{'Program'});
  }
  else
  {
    my $sFile = $hOptions{'f'};
    if ($sFile eq '-')
    {
      $sFile = "stdin";
      $sFileHandle = \*STDIN;
    }
    else
    {
      if (!open(FH, "< $sFile"))
      {
        print STDERR "$$phProperties{'Program'}: Error='Unable to open $sFile ($!).'\n";
        exit(2);
      }
      $sFileHandle = \*FH;
    }
    $$phProperties{'OutFile'} = (($sFile !~ /^\//) ? cwd() . "/" : "") . $sFile . ".dbi";
    $$phProperties{'SqlFile'} = (($sFile !~ /^\//) ? cwd() . "/" : "") . $sFile . ".sql";
  }

  ####################################################################
  #
  # A Hostname, '-h', is optional.
  #
  ####################################################################

  $$phProperties{'Hostname'} = (exists($hOptions{'h'})) ? $hOptions{'h'} : undef;

  if (defined($$phProperties{'Hostname'}))
  {
    if ($$phProperties{'Hostname'} !~ /$sHostnameRegex/)
    {
      print STDERR "$$phProperties{'Program'}: Hostname='$$phProperties{'Hostname'}' Regex='$sHostnameRegex' Error='Invalid hostname.'\n";
      exit(2);
    }
  }

  ####################################################################
  #
  # A maximum row count, '-m', is optional.
  #
  ####################################################################

  $$phProperties{'MaxRows'} = (exists($hOptions{'m'})) ? $hOptions{'m'} : 0;

  if ($$phProperties{'MaxRows'} !~ /$sMaxRowsRegex/)
  {
    print STDERR "$$phProperties{'Program'}: MaxRows='$$phProperties{'MaxRows'}' Regex='$sMaxRowsRegex' Error='Invalid number.\n";
    exit(2);
  }

  ####################################################################
  #
  # The option list, '-o', is optional.
  #
  ####################################################################

  $$phProperties{'ForceWrite'} = 0;
  $$phProperties{'LocalInFile'} = 0;
  $$phProperties{'UseMergeTables'} = 0;

  $$phProperties{'Options'} = (exists($hOptions{'o'})) ? $hOptions{'o'} : undef;

  if (exists($hOptions{'o'}) && defined($hOptions{'o'}))
  {
    foreach my $sActualOption (split(/,/, $$phProperties{'Options'}))
    {
      foreach my $sTargetOption ('ForceWrite', 'LocalInFile', 'UseMergeTables')
      {
        if ($sActualOption =~ /^$sTargetOption$/i)
        {
          $$phProperties{$sTargetOption} = 1;
        }
      }
    }
  }

  ####################################################################
  #
  # A table, '-t', is optional.
  #
  ####################################################################

  $$phProperties{'Table'} = (exists($hOptions{'t'})) ? $hOptions{'t'} : "dig";

  if ($$phProperties{'Table'} !~ /$sTableRegex/)
  {
    print STDERR "$$phProperties{'Program'}: Table='$$phProperties{'Table'}' Regex='$sTableRegex' Error='Invalid table name.'\n";
    exit(2);
  }

  ####################################################################
  #
  # If any arguments remain, it's an error.
  #
  ####################################################################

  if (scalar(@ARGV) > 0)
  {
    Usage($$phProperties{'Program'});
  }

  ##################################################################
  #
  # Update the table name if MERGE tables are in play.
  #
  ##################################################################

  if ($$phProperties{'UseMergeTables'})
  {
    if (!defined($$phProperties{'Hostname'}))
    {
      print STDERR "$$phProperties{'Program'}: Error='MERGE tables have been requested, but no hostname was defined.'\n";
      exit(2);
    }
    $$phProperties{'Table'} = $$phProperties{'Table'} . "_" . $$phProperties{'Hostname'};
  }

  ##################################################################
  #
  # Check file existence if force write is disabled.
  #
  ##################################################################

  if (!$$phProperties{'ForceWrite'} && -f $$phProperties{'SqlFile'})
  {
    print STDERR "$$phProperties{'Program'}: Filename='$$phProperties{'SqlFile'}' Error='Output file already exists.'\n";
    exit(2);
  }

  if (!$$phProperties{'ForceWrite'} && -f $$phProperties{'OutFile'})
  {
    print STDERR "$$phProperties{'Program'}: Filename='$$phProperties{'OutFile'}' Error='Output file already exists.'\n";
    exit(2);
  }

  ##################################################################
  #
  # Process the header.
  #
  ##################################################################

  my ($sHeader, $sHeaderFieldCount, @aHeaderFields, $sNameIndex);

  if (!defined($sHeader = <$sFileHandle>))
  {
    print STDERR "$$phProperties{'Program'}: Error='Header not defined.'\n";
    exit(2);
  }
  $sHeader =~ s/[\r\n]+$//;
  @aHeaderFields = split(/\|/, $sHeader);
  $sHeaderFieldCount = scalar(@aHeaderFields);
  for (my $sIndex = 0; $sIndex < $sHeaderFieldCount; $sIndex++)
  {
    if (!exists($hTableLayout{$aHeaderFields[$sIndex]}))
    {
      print STDERR "$$phProperties{'Program'}: Field='$aHeaderFields[$sIndex]' Error='Field not recognized.'\n";
      exit(2);
    }
    if ($aHeaderFields[$sIndex] =~ /^name$/i)
    {
      $sNameIndex = $sIndex;
    }
  }
  if (!defined($sNameIndex) && $sNameIndex != 0)
  {
    print STDERR "$$phProperties{'Program'}: Header='$sHeader' Error='Invalid header or unable to locate the name field.'\n";
    exit(2);
  }
  push(@aHeaderFields, "joiner");
  unshift(@aHeaderFields, "hostname") if (defined($$phProperties{'Hostname'}));

  ##################################################################
  #
  # Open output files.
  #
  ##################################################################

  umask(022);

  if (!open(OUT, "> $$phProperties{'OutFile'}"))
  {
    print STDERR "$$phProperties{'Program'}: Filename='$$phProperties{'OutFile'}' Error='$!'\n";
    exit(2);
  }

  if (!open(SQL, "> $$phProperties{'SqlFile'}"))
  {
    print STDERR "$$phProperties{'Program'}: Filename='$$phProperties{'SqlFile'}' Error='$!'\n";
    exit(2);
  }

  ##################################################################
  #
  # Print the header.
  #
  ##################################################################

  print OUT join('|', @aHeaderFields) . "\n";

  ##################################################################
  #
  # Process the data.
  #
  ##################################################################

  my ($sDataFieldCount, @aDataFields);

  while (my $sLine = <$sFileHandle>)
  {
    $sLine =~ s/[\r\n]+$//;
    @aDataFields = split(/\|/, $sLine, -1); # Use large chunk size to preserve trailing NULL fields.
    $sDataFieldCount = scalar(@aDataFields);
    if ($sDataFieldCount != $sHeaderFieldCount)
    {
      print STDERR "$$phProperties{'Program'}: Line='$sLine' HeaderFieldCount='$sHeaderFieldCount' DataFieldCount='$sDataFieldCount' Error='FieldCounts don't match.'\n";
      close($sFileHandle);
      close(OUT);
      exit(2);
    }
    if (defined($$phProperties{'Hostname'}))
    {
      $sLine = join('|', ($$phProperties{'Hostname'}, @aDataFields, md5_hex($$phProperties{'Hostname'} . $aDataFields[$sNameIndex])));
    }
    else
    {
      $sLine = join('|', (@aDataFields, md5_hex($aDataFields[$sNameIndex])));
    }
    $sLine =~ s/\\/\\\\/g; # Escape backslashes.
    $sLine =~ s/\|\|/\|\\N\|/g; # Add embedded NULLs.
    $sLine =~ s/\|\|/\|\\N\|/g; # Add embedded NULLs again to catch adjacent offenders.
    $sLine =~ s/\|$/\|\\N/g; # Add end-of-line NULLs.
    print OUT $sLine . "\n";
  }
  close($sFileHandle);
  close(OUT);

  ##################################################################
  #
  # Build the SQL file.
  #
  ##################################################################

  my (@aColumns, $sCreateOptions, $sDate, $sLoad);

  if ($$phProperties{'LocalInFile'})
  {
    $sLoad = "LOAD DATA LOCAL INFILE '" . $$phProperties{'OutFile'} . "'";
  }
  else
  {
    $sLoad = "LOAD DATA INFILE '" . $$phProperties{'OutFile'} . "'";
  }

  if ($$phProperties{'Duplicates'} =~ /^(ignore|replace)$/i)
  {
    $sLoad .= " " . uc($1);
  }

  $sDate = localtime(time);
  $sDate =~ s/\s+/ /g;
  print SQL <<EOF;
######################################################################
#
# Created by $$phProperties{'Program'} on $sDate.
#
######################################################################

######################################################################
#
# Create database, if necessary.
#
######################################################################

CREATE DATABASE IF NOT EXISTS $$phProperties{'Database'};

######################################################################
#
# Set active database.
#
######################################################################

USE $$phProperties{'Database'};

######################################################################
#
# Create table, if necessary.
#
######################################################################

EOF

  if (!defined($$phProperties{'Hostname'}))
  {
    print SQL "DROP TABLE IF EXISTS \`$$phProperties{'Table'}\`;\n";
  }
  foreach my $sField (@aHeaderFields)
  {
    push(@aColumns, ($sField . " " . $hTableLayout{$sField}));
  }
  $sCreateOptions = ($$phProperties{'MaxRows'} > 0) ? "MAX_ROWS = $$phProperties{'MaxRows'}" : "";

  my $sCreateSpec = join(', ', @aColumns);
  my $sInsertSpec = join(', ', @aHeaderFields);

  print SQL <<EOF;
CREATE TABLE IF NOT EXISTS \`$$phProperties{'Table'}\` ($sCreateSpec) $sCreateOptions;

######################################################################
#
# Bulk load the data.
#
######################################################################

$sLoad INTO TABLE \`$$phProperties{'Table'}\` FIELDS TERMINATED BY '|' IGNORE 1 LINES ($sInsertSpec);

######################################################################
#
# End of auto-generated SQL.
#
######################################################################
EOF

  close(SQL);

  1;


######################################################################
#
# Usage
#
######################################################################

sub Usage
{
  my ($sProgram) = @_;
  print STDERR "\n";
  print STDERR "Usage: $sProgram [-D {default|ignore|replace}] [-d db] [-h host] [-m max-rows] [-o option[,option[,...]]] [-t table] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-dig2dbi.pl - Preprocess FTimes dig data for MySQL DB import

=head1 SYNOPSIS

B<ftimes-dig2dbi.pl> B<[-D {default|ignore|replace}]> B<[-d db]> B<[-h host]> B<[-m max-rows]> B<[-o option[,option[,...]]]> B<[-t table]> B<-f {file|-}>

=head1 DESCRIPTION

This utility takes FTimes dig data as input, processes it, and
produces two output files having the extensions .sql and .dbi. The SQL
statements in the .sql file may be used to import the .dbi data into
MySQL -- dbi is short for DB Import. An extra field, joiner, is added
to the .dbi file. This field serves as the primary index -- either
alone or in conjunction with the hostname field. It can also be used
as a joining key to tie map and dig tables together.


=head1 OPTIONS

=over 4

=item B<-D {default|ignore|replace}>

Add the IGNORE or REPLACE keyword to the LOAD statement. The default
value is 'default', which means don't alter the LOAD statement.

=item B<-d db>

Specifies the name of the database to create/use. This value is passed
directly into the .sql file. The default value is 'ftimes'.

=item B<-f {file|-}>

Specifies the name of the input file. A value of '-' will cause the
program to read from stdin. If input is read from stdin, the output
files will be placed in the current directory and have a basename of
'stdin'.

=item B<-h host>

Specifies the name of the host to bind to the imported data. If
specified, an additional field, hostname, is inserted in the .dbi file
and filled with the appropriate value. By default, the hostname field
is not used.

=item B<-m max-rows>

Limits the number of records that may be inserted into the specified
database. This value is passed directly into the .sql file as
MAX_ROWS.

=item B<-o option,[option[,...]]>

Specifies the list of options to apply.  Currently, the following
options are supported:

=over 4

=item ForceWrite

Force existing .sql and .dbi files to be truncated on open.

=item LocalInFile

Add the LOCAL keyword to the LOAD statement. When this keyword is
specified, the .dbi file is read by the local MySQL client and sent to
the remote MySQL server.

=item UseMergeTables

Append the hostname, if defined, as a suffix on the table name. This
allows data for each unique host to be loaded into separate tables
that can be linked together using MySQL MERGE tables.

=back

=item B<-t table>

Specifies the name of the table to create/use. This value is passed
directly into the .sql file. The default value is 'dig'.

=back

=head1 CAVEATS

Don't mix host and non host .dbi files as they use different schemas.

=head1 AUTHOR

Klayton Monroe

=head1 SEE ALSO

ftimes(1)

=head1 LICENSE

All documentation and code are distributed under same terms and
conditions as FTimes.

=cut
