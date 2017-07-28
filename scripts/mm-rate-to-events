#!/usr/bin/perl

# converts a trace FROM varying rate (in kilobits per millisecond,
# sampled once per millisecond)
# TO packet delivery occurrences (millisecond timestamps when an
# MTU-sized datagram, i.e. 12,000 bits, can be delivered)
#
# The latter is the format that mm-link expects.
#
# KJW 7/28/2017

use strict;
use warnings;
use Scalar::Util qw[looks_like_number];
use POSIX qw[ceil];

my $time = 0;
my $reserve_bits = 0;
my $PACKET_LENGTH = 12000; # bits

# input trace gives a number of kilobits that can be delivered each millisecond
while (my $kilobits_this_millisecond = <>) {
  chomp $kilobits_this_millisecond;
  die qq{Not a number: "$kilobits_this_millisecond"}
    unless looks_like_number $kilobits_this_millisecond;

  # add current rate to "reserve"
  $reserve_bits += 1000 * $kilobits_this_millisecond;

  # if reserve has enough bits for a packet, deliver it now
  # output trace gives timestamps of packet delivery occurrences
  while ( $reserve_bits >= $PACKET_LENGTH ) {
    print qq{$time\n};
    $reserve_bits -= $PACKET_LENGTH;
  }

  # advance time by one millisecond
  $time++;
}
