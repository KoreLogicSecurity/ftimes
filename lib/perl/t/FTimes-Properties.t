#!/usr/bin/perl -w
######################################################################
#
# $Id: FTimes-Properties.t,v 1.7 2019/03/14 16:07:42 klm Exp $
#
######################################################################
#
# Copyright 2011-2019 The FTimes Project, All Rights Reserved.
#
######################################################################
#
# Purpose: Tests for FTimes::Properties.
#
######################################################################

use 5.008;
use strict;
use Test;

BEGIN
{
  my %hPlan =
  (
    'tests' => 1 + 1 + 30 + 30 + 29 + 29 + 2 + 2 + 75,
  );
  plan(%hPlan);
};

######################################################################
#
# Tests
#
######################################################################

  ####################################################################
  #
  # Test: The target module must load without error.
  #
  ####################################################################

  use FTimes::Properties;
  ok(1);

  ####################################################################
  #
  # Test: The global regexes hash must be defined.
  #
  ####################################################################

  my $phGlobalRegexes = PropertiesGetGlobalRegexes();
  ok(defined($phGlobalRegexes));

  ####################################################################
  #
  # Common variables.
  #
  ####################################################################

  my (@aTuples, %hTargetValues, %hThrowAway);

  ####################################################################
  #
  # Test: Verify field mask combinations.
  #
  ####################################################################

  %hTargetValues =
  (
    'none'                     => 0x00000001,
    'none+dev'                 => 0x00000003,
    'none+inode'               => 0x00000005,
    'none+volume'              => 0x00000009,
    'none+findex'              => 0x00000011,
    'none+mode'                => 0x00000021,
    'none+attributes'          => 0x00000041,
    'none+nlink'               => 0x00000081,
    'none+uid'                 => 0x00000101,
    'none+gid'                 => 0x00000201,
    'none+rdev'                => 0x00000401,
    'none+atime'               => 0x00000801,
    'none+ams'                 => 0x00001001,
    'none+mtime'               => 0x00002001,
    'none+mms'                 => 0x00004001,
    'none+ctime'               => 0x00008001,
    'none+cms'                 => 0x00010001,
    'none+chtime'              => 0x00020001,
    'none+chms'                => 0x00040001,
    'none+size'                => 0x00080001,
    'none+altstreams'          => 0x00100001,
    'none+md5'                 => 0x00200001,
    'none+sha1'                => 0x00400001,
    'none+sha256'              => 0x00800001,
    'none+magic'               => 0x01000001,
    'none+owner'               => 0x02000001,
    'none+group'               => 0x04000001,
    'none+dacl'                => 0x08000001,
    'none+hashes'              => 0x00e00001,
    'none+times'               => 0x0007f801,
    'none+all'                 => undef,

    'all'                      => 0x0fffffff,
    'all-dev'                  => 0x0ffffffd,
    'all-inode'                => 0x0ffffffb,
    'all-volume'               => 0x0ffffff7,
    'all-findex'               => 0x0fffffef,
    'all-mode'                 => 0x0fffffdf,
    'all-attributes'           => 0x0fffffbf,
    'all-nlink'                => 0x0fffff7f,
    'all-uid'                  => 0x0ffffeff,
    'all-gid'                  => 0x0ffffdff,
    'all-rdev'                 => 0x0ffffbff,
    'all-atime'                => 0x0ffff7ff,
    'all-ams'                  => 0x0fffefff,
    'all-mtime'                => 0x0fffdfff,
    'all-mms'                  => 0x0fffbfff,
    'all-ctime'                => 0x0fff7fff,
    'all-cms'                  => 0x0ffeffff,
    'all-chtime'               => 0x0ffdffff,
    'all-chms'                 => 0x0ffbffff,
    'all-size'                 => 0x0ff7ffff,
    'all-altstreams'           => 0x0fefffff,
    'all-md5'                  => 0x0fdfffff,
    'all-sha1'                 => 0x0fbfffff,
    'all-sha256'               => 0x0f7fffff,
    'all-magic'                => 0x0effffff,
    'all-owner'                => 0x0dffffff,
    'all-group'                => 0x0bffffff,
    'all-dacl'                 => 0x07ffffff,
    'all-hashes'               => 0x0f1fffff,
    'all-times'                => 0x0ff807ff,

    'none+dev+dev'             => 0x00000003,
    'none+dev+inode'           => 0x00000007,
    'none+dev+volume'          => 0x0000000b,
    'none+dev+findex'          => 0x00000013,
    'none+dev+mode'            => 0x00000023,
    'none+dev+attributes'      => 0x00000043,
    'none+dev+nlink'           => 0x00000083,
    'none+dev+uid'             => 0x00000103,
    'none+dev+gid'             => 0x00000203,
    'none+dev+rdev'            => 0x00000403,
    'none+dev+atime'           => 0x00000803,
    'none+dev+ams'             => 0x00001003,
    'none+dev+mtime'           => 0x00002003,
    'none+dev+mms'             => 0x00004003,
    'none+dev+ctime'           => 0x00008003,
    'none+dev+cms'             => 0x00010003,
    'none+dev+chtime'          => 0x00020003,
    'none+dev+chms'            => 0x00040003,
    'none+dev+size'            => 0x00080003,
    'none+dev+altstreams'      => 0x00100003,
    'none+dev+md5'             => 0x00200003,
    'none+dev+sha1'            => 0x00400003,
    'none+dev+sha256'          => 0x00800003,
    'none+dev+magic'           => 0x01000003,
    'none+dev+owner'           => 0x02000003,
    'none+dev+group'           => 0x04000003,
    'none+dev+dacl'            => 0x08000003,
    'none+dev+hashes'          => 0x00e00003,
    'none+dev+times'           => 0x0007f803,

    'all-dev-dev'              => 0x0ffffffd,
    'all-dev-inode'            => 0x0ffffff9,
    'all-dev-volume'           => 0x0ffffff5,
    'all-dev-findex'           => 0x0fffffed,
    'all-dev-mode'             => 0x0fffffdd,
    'all-dev-attributes'       => 0x0fffffbd,
    'all-dev-nlink'            => 0x0fffff7d,
    'all-dev-uid'              => 0x0ffffefd,
    'all-dev-gid'              => 0x0ffffdfd,
    'all-dev-rdev'             => 0x0ffffbfd,
    'all-dev-atime'            => 0x0ffff7fd,
    'all-dev-ams'              => 0x0fffeffd,
    'all-dev-mtime'            => 0x0fffdffd,
    'all-dev-mms'              => 0x0fffbffd,
    'all-dev-ctime'            => 0x0fff7ffd,
    'all-dev-cms'              => 0x0ffefffd,
    'all-dev-chtime'           => 0x0ffdfffd,
    'all-dev-chms'             => 0x0ffbfffd,
    'all-dev-size'             => 0x0ff7fffd,
    'all-dev-altstreams'       => 0x0feffffd,
    'all-dev-md5'              => 0x0fdffffd,
    'all-dev-sha1'             => 0x0fbffffd,
    'all-dev-sha256'           => 0x0f7ffffd,
    'all-dev-magic'            => 0x0efffffd,
    'all-dev-owner'            => 0x0dfffffd,
    'all-dev-group'            => 0x0bfffffd,
    'all-dev-dacl'             => 0x07fffffd,
    'all-dev-hashes'           => 0x0f1ffffd,
    'all-dev-times'            => 0x0ff807fd,

    'all-hashes+hashes'        => 0x0fffffff,
    'all-hashes-dev+hashes'    => 0x0ffffffd,

    'none+hashes-hashes'       => 0x00000001,
    'none+hashes+dev-hashes'   => 0x00000003,
  );

   foreach my $sTest (keys %hTargetValues)
   {
     my $sResult = ParseFieldMask($sTest, %hThrowAway);
     ok((!defined($sResult) && !defined($hTargetValues{$sTest})) || ($sResult == $hTargetValues{$sTest}));
   }

  ####################################################################
  #
  # Test: Send Boolean through the gauntlet.
  #
  ####################################################################

  @aTuples =
  (
    "fail,",
    "pass,1",
    "pass,0",
    "pass,T",
    "pass,F",
    "pass,N",
    "pass,Y",
    "pass,t",
    "pass,f",
    "pass,n",
    "pass,y",
    "pass,TRUE",
    "pass,FALSE",
    "pass,YES",
    "pass,NO",
    "pass,ON",
    "pass,OFF",
    "pass,true",
    "pass,false",
    "pass,yes",
    "pass,no",
    "pass,on",
    "pass,off",
    "pass,TrUe",
    "pass,FaLsE",
    "pass,YeS",
    "pass,OfF",
    "fail,1_junk",
    "fail,0_junk",
    "fail,t_junk",
    "fail,f_junk",
    "fail,n_junk",
    "fail,y_junk",
    "fail,on_junk",
    "fail,no_junk",
    "fail,off_junk",
    "fail,yes_junk",
    "fail,true_junk",
    "fail,false_junk",
    "fail,junk_1",
    "fail,junk_0",
    "fail,junk_t",
    "fail,junk_f",
    "fail,junk_y",
    "fail,junk_n",
    "fail,junk_on",
    "fail,junk_no",
    "fail,junk_off",
    "fail,junk_yes",
    "fail,junk_true",
    "fail,junk_false",
    "fail,junk_1_junk",
    "fail,junk_0_junk",
    "fail,junk_t_junk",
    "fail,junk_f_junk",
    "fail,junk_y_junk",
    "fail,junk_n_junk",
    "fail,junk_on_junk",
    "fail,junk_no_junk",
    "fail,junk_yes_junk",
    "fail,junk_off_junk",
    "fail,junk_true_junk",
    "fail,junk_false_junk",
    "fail,rue",
    "fail,RUE",
    "fail,alse",
    "fail,ALSE",
    "fail,tru",
    "fail,TRU",
    "fail,fals",
    "fail,FALS",
    "fail,YE",
    "fail,OF",
    "fail,Of",
  );

  TestLoop(\@aTuples, $$phGlobalRegexes{'Boolean'});


######################################################################
#
# TestLoop
#
######################################################################

sub TestLoop
{
  my ($paTuples, $sRegex) = @_;

  foreach my $sTuple (@$paTuples)
  {
    my ($sExpectedResult, $sPath) = split(/,/, $sTuple);
    $sPath =~ s/%2c/,/g;
    my $sResult = ($sPath =~ /^$sRegex$/) ? "pass" : "fail";
    ok($sResult eq $sExpectedResult);
  }

  1;
}
