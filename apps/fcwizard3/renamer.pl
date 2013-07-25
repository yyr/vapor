#!/usr/local/bin/perl
#
# usage: rename perlexpr [files ...]

($op = shift) || die "Usage: rename perlexpr [files ...]\n";

if (!@ARGV) {
	@ARGV = <STDIN>;
	chop(@ARGV);
}

for (@ARGV) {
	$was = $_;
	eval $op;
	die $@ if $@;
	rename($was, $_) unless $was eq $_;
}
