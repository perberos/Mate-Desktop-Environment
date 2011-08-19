#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-

use diagnostics;
use strict;

my $DEBUG = 0;

my %public_headers;
my %module_headers;
my %private_headers;

my $current_hash;
my $current_hash_name;

my $current_header;
my $included_header;

my $exit_code = 0;

open MAKEFILE_AM, "<./Makefile.am";

while (<MAKEFILE_AM>) {
    if (/libmatevfsinclude_HEADERS/) {
        $current_hash = \%public_headers
    } elsif (/libmatevfsmoduleinclude_HEADERS/) {
        $current_hash = \%module_headers
    } elsif (/noinst_HEADERS/) {
        $current_hash = \%private_headers
    } elsif (/\$\(NULL\)/ || ! /\\/) {
        $current_hash = 0;
    }

    if (/.*\.h[ \t]/) {
        chomp;
        $current_header = $_;
        $current_header =~ s/[ \t]*(.*\.h)[ \t]*.*/$1/;
        
        if ($current_hash) {
            $$current_hash{$current_header} = 1;
        }
    }
}

close MAKEFILE_AM;

for my $public_header (keys %public_headers) {
    open HEADER, "<${public_header}";
    while (<HEADER>) {
        if (/\#include[ \t]+<libmatevfs\/.*\.h>.*/) {
            chomp;
            $included_header = $_;
            $included_header =~ s/\#include[ \t]+<libmatevfs\/(.*\.h)>.*/$1/;

            if ($private_headers{$included_header}) {
                print "Public header \"$public_header\" includes private header \"$included_header\"\n";
                $exit_code = 1;
            } elsif ($module_headers{$included_header}) {
                print "Public header \"$public_header\" includes module API header \"$included_header\"\n";
                $exit_code = 1;
            } elsif ($public_headers{$included_header}) {
                print "Public header \"$public_header\" includes public header \"$included_header\"\n" if $DEBUG;
            } else {
                print "Public header \"$public_header\" includes unknown header \"$included_header\"\n";
                $exit_code = 1;
            }
        }
    }
    close HEADER;
}


for my $module_header (keys %module_headers) {
    open HEADER, "<${module_header}";

    while (<HEADER>) {
        if (/\#include[ \t]+<libmatevfs\/.*\.h>.*/) {
            chomp;
            $included_header = $_;
            $included_header =~ s/\#include[ \t]+<libmatevfs\/(.*\.h)>.*/$1/;

            if ($private_headers{$included_header}) {
                print "Module API header \"$module_header\" includes private header \"$included_header\"\n";
                $exit_code = 1;
            } elsif ($module_headers{$included_header}) {
                print "Module API header \"$module_header\" includes public header \"$included_header\"\n" if $DEBUG;
            } elsif ($public_headers{$included_header}) {
                print "Module API header \"$module_header\" includes public header \"$included_header\"\n" if $DEBUG;
            } else {
                print "Module API header \"$module_header\" includes unknown header \"$included_header\"\n";
                $exit_code = 1;
            }
        }
    }
    close HEADER;
}


for my $private_header (keys %private_headers) {
    open HEADER, "<${private_header}";

    while (<HEADER>) {
        if (/\#include[ \t]+<libmatevfs\/.*\.h>.*/) {
            chomp;
            $included_header = $_;
            $included_header =~ s/\#include[ \t]+<libmatevfs\/(.*\.h)>.*/$1/;

            if ($private_headers{$included_header}) {
                print "Private header \"$private_header\" includes private header \"$included_header\"\n" if $DEBUG;
            } elsif ($module_headers{$included_header}) {
                print "Private header \"$private_header\" includes module API header \"$included_header\"\n" if $DEBUG;
            } elsif ($public_headers{$included_header}) {
                print "Private header \"$private_header\" includes public header \"$included_header\"\n" if $DEBUG;
            } else {
                print "Private header \"$private_header\" includes unknown header \"$included_header\"\n";
                $exit_code = 1;
            }
        }
    }
    close HEADER;
}

exit $exit_code;
