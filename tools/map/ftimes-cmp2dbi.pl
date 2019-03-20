#!/usr/bin/perl -w
######################################################################
#
# $Id: ftimes-cmp2dbi.pl,v 1.6 2007/02/23 00:22:36 mavrik Exp $
#
######################################################################
#
# Copyright 2006-2007 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Preprocess FTimes compare data for MySQL db import.
#
######################################################################

use strict;
use Cwd;
use Digest::MD5 qw(md5_hex);
use File::Basename;
use Getopt::Std;

######################################################################
#
# Main Routine
#
######################################################################

  my $sProgram = basename(__FILE__);

  ####################################################################
  #
  # Validation expressions.
  #
  ####################################################################

  my $sDBRegex       = qq(^[A-Za-z][A-Za-z0-9_]*\$);
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
    'hostname'      => "varbinary(64) not null",
    'category'      => "enum('C', 'M', 'N', 'U', 'X') not null",
    'name'          => "blob not null, index name_index (name(255))",
    'changed'       => "set ('dev', 'inode', 'volume', 'findex', 'mode', 'attributes', 'nlink', 'uid', 'gid', 'rdev', 'atime', 'ams', 'mtime', 'mms', 'ctime', 'cms', 'chtime', 'chms', 'size', 'altstreams', 'md5', 'sha1', 'sha256', 'magic')",
    'unknown'       => "set ('dev', 'inode', 'volume', 'findex', 'mode', 'attributes', 'nlink', 'uid', 'gid', 'rdev', 'atime', 'ams', 'mtime', 'mms', 'ctime', 'cms', 'chtime', 'chms', 'size', 'altstreams', 'md5', 'sha1', 'sha256', 'magic')",
    'namemd5'       => "varbinary(32) not null",
  );

  ####################################################################
  #
  # Get Options.
  #
  ####################################################################

  my %hOptions;

  if (!getopts('d:Ff:h:m:t:', \%hOptions))
  {
    Usage($sProgram);
  }

  ####################################################################
  #
  # A Database, '-d', is optional.
  #
  ####################################################################

  my $sDB = (exists $hOptions{'d'}) ? $hOptions{'d'} : "ftimes";

  if ($sDB !~ /$sDBRegex/)
  {
    print STDERR "$sProgram: Database='$hOptions{'d'}' Regex='$sDBRegex' Error='Invalid db name.'\n";
    exit(2);
  }

  ####################################################################
  #
  # The ForceWrite, '-F', flag is optional.
  #
  ####################################################################

  my $sForceWrite = (exists $hOptions{'F'}) ? 1 : 0;

  ####################################################################
  #
  # A Filename, '-f', is required. It can be '-' or a regular file.
  #
  ####################################################################

  my ($sFileHandle, $sFilename, $sOutFile, $sSQLFile);

  if (!exists $hOptions{'f'})
  {
    Usage($sProgram);
    exit(2);
  }
  else
  {
    $sFilename = $hOptions{'f'};
    if (-f $sFilename)
    {
      if (!open(IN, "< $sFilename"))
      {
        print STDERR "$sProgram: Filename='$sFilename' Error='$!'\n";
        exit(2);
      }
      $sFileHandle = \*IN;
    }
    else
    {
      if ($sFilename ne '-')
      {
        print STDERR "$sProgram: Filename='$sFilename' Error='File not found.'\n";
        exit(2);
      }
      $sFilename = "stdin";
      $sFileHandle = \*STDIN;
    }
  }
  $sOutFile = (($sFilename !~ /^\//) ? cwd() . "/" : "") . $sFilename . ".dbi";
  $sSQLFile = (($sFilename !~ /^\//) ? cwd() . "/" : "") . $sFilename . ".sql";

  ####################################################################
  #
  # A Hostname, '-h', is optional. It affects the primary key.
  #
  ####################################################################

  my ($sHostname, $sPrimaryKey);

  if (exists $hOptions{'h'})
  {
    if ($hOptions{'h'} =~ /$sHostnameRegex/)
    {
      $sHostname = $hOptions{'h'};
      $sPrimaryKey = "primary key (hostname, namemd5)";
    }
    else
    {
      print STDERR "$sProgram: Hostname='$sHostname' Regex='$sHostnameRegex' Error='Invalid hostname.'\n";
      exit(2);
    }
  }
  else
  {
    $sPrimaryKey = "primary key (namemd5)";
  }

  ####################################################################
  #
  # A MaxRows, '-m', is optional.
  #
  ####################################################################

  my $sMaxRows = (exists $hOptions{'m'}) ? $hOptions{'m'} : 0;

  if ($sMaxRows !~ /$sMaxRowsRegex/)
  {
    print STDERR "$sProgram: MaxRows='$sMaxRows' Regex='$sMaxRowsRegex' Error='Invalid number.\n";
    exit(2);
  }

  ####################################################################
  #
  # A Table, '-t', is optional.
  #
  ####################################################################

  my $sTable = (exists $hOptions{'t'}) ? $hOptions{'t'} : "map";

  if ($sTable !~ /$sTableRegex/)
  {
    print STDERR "$sProgram: Table='$sTable' Regex='$sTableRegex' Error='Invalid table name.'\n";
    exit(2);
  }

  ##################################################################
  #
  # Check file existence if force write is disabled.
  #
  ##################################################################

  if (!$sForceWrite && -f $sSQLFile)
  {
    print STDERR "$sProgram: Filename='$sSQLFile' Error='Output file already exists.'\n";
    exit(2);
  }

  if (!$sForceWrite && -f $sOutFile)
  {
    print STDERR "$sProgram: Filename='$sOutFile' Error='Output file already exists.'\n";
    exit(2);
  }

  ##################################################################
  #
  # Process the header.
  #
  ##################################################################

  my ($sHeader, $sHeaderFieldCount, @aHeaderFields, $sNameIndex);

  if (!defined ($sHeader = <$sFileHandle>))
  {
    print STDERR "$sProgram: Error='Header not defined.'\n";
    exit(2);
  }
  $sHeader =~ s/[\r\n]+$//;
  @aHeaderFields = split(/\|/, $sHeader);
  $sHeaderFieldCount = scalar(@aHeaderFields);
  for (my $sIndex = 0; $sIndex < $sHeaderFieldCount; $sIndex++)
  {
    if (!exists $hTableLayout{$aHeaderFields[$sIndex]})
    {
      print STDERR "$sProgram: Field='$aHeaderFields[$sIndex]' Error='Field not recognized.'\n";
      exit(2);
    }
    if ($aHeaderFields[$sIndex] =~ /^name$/i)
    {
      $sNameIndex = $sIndex;
    }
  }
  if (!defined $sNameIndex && $sNameIndex != 0)
  {
    print STDERR "$sProgram: Header='$sHeader' Error='Invalid header or unable to locate the name field.'\n";
    exit(2);
  }
  push(@aHeaderFields, "namemd5");
  unshift(@aHeaderFields, "hostname") if (defined $sHostname);

  ##################################################################
  #
  # Open output files.
  #
  ##################################################################

  umask(022);

  if (!open(OUT, "> $sOutFile"))
  {
    print STDERR "$sProgram: Filename='$sOutFile' Error='$!'\n";
    exit(2);
  }

  if (!open(SQL, "> $sSQLFile"))
  {
    print STDERR "$sProgram: Filename='$sSQLFile' Error='$!'\n";
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
      print STDERR "$sProgram: Line='$sLine' HeaderFieldCount='$sHeaderFieldCount' DataFieldCount='$sDataFieldCount' Error='FieldCounts don't match.'\n";
      close($sFileHandle);
      close(OUT);
      exit(2);
    }
    if (defined $sHostname)
    {
      $sLine = join('|', ($sHostname, @aDataFields, md5_hex($aDataFields[$sNameIndex])));
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

  my (@aColumns, $sCreateOptions, $sDate);

  $sDate = localtime(time);
  $sDate =~ s/\s+/ /g;
  print SQL <<EOF;
######################################################################
#
# Created by $sProgram on $sDate.
#
######################################################################

######################################################################
#
# Create database, if necessary.
#
######################################################################

CREATE DATABASE IF NOT EXISTS $sDB;

######################################################################
#
# Set active database.
#
######################################################################

USE $sDB;

######################################################################
#
# Create table, if necessary.
#
######################################################################

EOF

  if (!defined $sHostname)
  {
    print SQL "DROP TABLE IF EXISTS $sTable;\n";
  }
  foreach my $sField (@aHeaderFields)
  {
    push(@aColumns, ($sField . " " . $hTableLayout{$sField}));
  }
  push(@aColumns, $sPrimaryKey);
  $sCreateOptions = ($sMaxRows > 0) ? "MAX_ROWS = $sMaxRows" : "";

  my $sCreateSpec = join(', ', @aColumns);
  my $sInsertSpec = join(', ', @aHeaderFields);

  print SQL <<EOF;
CREATE TABLE IF NOT EXISTS $sTable ($sCreateSpec) $sCreateOptions;

######################################################################
#
# Bulk load the data.
#
######################################################################

LOAD DATA INFILE '$sOutFile' INTO TABLE $sTable FIELDS TERMINATED BY '|' IGNORE 1 LINES ($sInsertSpec);

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
  print STDERR "Usage: $sProgram [-F] [-d db] [-h host] [-m max-rows] [-t table] -f {file|-}\n";
  print STDERR "\n";
  exit(1);
}


=pod

=head1 NAME

ftimes-cmp2dbi.pl - Preprocess FTimes compare data for MySQL DB import

=head1 SYNOPSIS

B<ftimes-cmp2dbi.pl> B<[-F]> B<[-d db]> B<[-h host]> B<[-m max-rows]> B<[-t table]> B<-f {file|-}>

=head1 DESCRIPTION

This utility takes FTimes compare data as input, processes it, and
produces two output files having the extensions .sql and .dbi. The SQL
statements in the .sql file may be used to import the .dbi data into
MySQL -- dbi is short for DB Import. An extra field, namemd5, is added
to the .dbi file. This field serves as the primary key -- either alone
or in conjunction with the hostname field.

=head1 OPTIONS

=over 4

=item B<-d db>

Specifies the name of the database to create/use. This value is passed
directly into the .sql file. The default value is 'ftimes'.

=item B<-F>

Force existing .sql and .dbi files to be truncated on open.

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

=item B<-t table>

Specifies the name of the table to create/use. This value is passed
directly into the .sql file. The default value is 'compare'.

=back

=head1 CAVEATS

Don't mix host and non host .dbi files as they use different schemas.

=head1 AUTHORS

Klayton Monroe and Jason Smith

=head1 SEE ALSO

ftimes(1)

=head1 LICENSE

All documentation and code is distributed under same terms and
conditions as FTimes.

=cut
