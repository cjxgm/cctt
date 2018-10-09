#!/usr/bin/env perl
use v5.26;
use utf8;
use strict;
use warnings;
use autodie;
use Cwd qw[abs_path];

my @includes;

for (<library/*/include>) {
    push @includes, "-isystem " . abs_path($_);
}

print join(" ", @includes), "\n";

