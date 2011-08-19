#!/usr/bin/perl -w
# -*- Mode: perl; indent-tabs-mode: nil -*-

#
#  MATE VFS
#
#  Copyright (C) 2000, 2001 Eazel, Inc.
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this library; if not, write to the Free Software
#  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#
#  Author: Darin Adler <darin@eazel.com>,
#

# check-mime.pl: 

# What we check:
#     types are lower-case in all files
#     types in mate-vfs.keys.in are in alphabetical order
#     types in mate-vfs.mime are in alphabetical order
#     types in mate-vfs.mime have descriptions in mate-vfs.keys.in
#     types in mate-vfs-mime-magic have descriptions in mate-vfs.keys.in
#     types in mate-vfs.applications have descriptions in mate-vfs.keys.in
#     (need to add what we check now)

# Other things to check later:
#     OAFIIDs are consistent (same UUID part for same prefix)
#     some way of detecting mate-vfs.keys.in entries for nonexistent MIME types

use diagnostics;
use strict;

my %seen;

my $previous_type = "";
my $type;
my %in_keys;
my %described;

my %seen_keys;

my @expected_keys;
my %expected_keys;

my $exit_status = 0;

sub complain ($)
  {
    my ($complaint) = @_;

    print STDERR "- ", $complaint, "\n";
    $exit_status = 1;
  }

my $check_icons = -d "./icons";
if ($check_icons)
  {
    print "Found Caja icon directory, checking icons.\n";
  }
else
  {
    print "Unable to locate Caja icon directory, not checking icons.\n";
  }
              
print "Reading mate-vfs.keys.in.\n";

@expected_keys =
  (
   "can_be_executable",
   "_category",
   "default_action_type",
   "default_application_id",
   "icon_filename",
   "open",
   "short_list_application_ids_for_advanced_user_level",
   "short_list_application_ids_for_intermediate_user_level",
   "short_list_application_ids_for_novice_user_level",
   "short_list_component_iids",
   "short_list_component_iids_for_advanced_user_level",
   "short_list_component_iids_for_intermediate_user_level",
   "short_list_component_iids_for_novice_user_level",
   "startup_notify",
   "use_category_default",
   "vfs_method",
  );
@expected_keys{@expected_keys} = (1) x @expected_keys;

open KEYS, "mate-vfs.keys.in" or die;
while (<KEYS>)
  {
    chomp;
    if (/^\s+$/)
      {
        complain "blank line with spaces on line $.";
      }
    else
      {
        if (/^ /)
          {
            complain "leading spaces used instead of a tab on line $.";
          }
        if (/\s$/)
          {
            complain "trailing whitespace on line $.";
          }
        if (/\t /)
          {
            complain "space after tab on line $.";
          }
        if (/  /)
          {
            complain "multiple spaces on line $.";
          }
      }
    next if /^\s*\#/;
    if (/\s=/)
      {
        complain "space before = on line $.";
      }
    if (/=\s/)
      {
        complain "space after = on line $.";
      }
    if (/^(\S.*)/)
      {
	$previous_type = $type if $type;
	$type = $1;
	if ($type ne lc $type && !$seen{$type})
	  {
	    complain "$type contains upper-case letters";
	  }
	if (lc $type le lc $previous_type)
	  {
	    complain "$type is after $previous_type";
	  }
	$seen{$type} = 1;
	$in_keys{$type} = 1;

        %seen_keys = ();
      }
    elsif (/^\s*$/)
      {
	$previous_type = $type if $type;
	$type = "";
      }
    elsif (/^\s*([_a-z]+)\s*=\s*(.*)/)
      {
        if ($seen_keys{$1})
          {
            complain "saw key $1 twice at line $.";
          }
        else
          {
            $seen_keys{$1} = 1;
          }
        if ($1 eq "_description")
          {
            if ($type eq "")
              {
                complain "description outside any type at line $.";
              }
            $described{$type} = 1;
          }
        elsif ($1 eq "icon_filename")
          {
            if ($2 eq "i-regular")
              {
                if ($type eq "")
                  {
                    complain "i-regular specified explicitly at line $.";
                  }
                else
                  {
                    complain "i-regular specified for $type at line $.";
                  }
              }
            elsif ($check_icons)
              {
                if (not -f "./icons/$2" and not -f "./icons/$2.png")
                  {
                    complain "$2 specified for $type, but not in ./icons directory";
                  }
              }
          }
        elsif (!$expected_keys{$1})
          {
            if ($type ne "application/mime-type-test")
              {
                complain "saw unexpected key $1 at line $.";
              }
          }
      }
    elsif (/^\s*(.+?)\s*=/)
      {
        complain "saw bad characters in key $1 at line $.";
      }
    else
      {
        complain "could not parse line $.";
      }
  }
close KEYS;

print "Reading mate-vfs-mime-magic.\n";

my %in_magic;

open MAGIC, "mate-vfs-mime-magic" or die;
while (<MAGIC>)
  {
    chomp;
    if (/^\s+$/)
      {
        complain "blank line with spaces on line $.";
      }
    else
      {
        if (/^ /)
          {
            complain "leading spaces used instead of a tab on line $.";
          }
        if (/\s$/)
          {
            complain "trailing whitespace on line $.";
          }
        if (/\t /)
          {
            complain "space after tab on line $.";
          }
      }
    next if /^\s*\#/;
    next if /^\s*$/;
    if (/^[0-9:]+\s+[a-z]+\s+([^\\\s]|\\.)+\s+(&\s*\S+\s+)?(\S+)\s*$/)
      {
	$type = $3;
	if ($type ne lc $type && !$seen{$type})
	  {
	    complain "$type contains upper-case letters";
	  }
	if (!$described{$type} && !$in_magic{$type})
	  {
	    if (!$in_keys{$type})
	      {
		complain "$type is in mate-vfs-mime-magic, but not mate-vfs.keys.in";
	      }
	    else
	      {
		complain "$type is in mate-vfs-mime-magic, but has no description in mate-vfs.keys.in";
	      }
	  }
	$seen{$type} = 1;
	$in_magic{$type} = 1;
      }
    else
      {
	complain "could not parse line $.";
      }
  }
close MAGIC;

print "Reading mate-vfs.mime.\n";

@expected_keys =
  (
   "ext",
   "regex",
  );
@expected_keys{@expected_keys} = (1) x @expected_keys;

$previous_type = "";
$type = "";
my %in_mime;

open MIME, "mate-vfs.mime" or die;
while (<MIME>)
  {
    chomp;
    if (/^\s+$/)
      {
        complain "blank line with spaces on line $.";
      }
    else
      {
        if (/^ /)
          {
            complain "leading spaces used instead of a tab on line $.";
          }
        if (/\s$/)
          {
            complain "trailing whitespace on line $.";
          }
        if (/\t /)
          {
            complain "space after tab on line $.";
          }
        if (/  /)
          {
            complain "multiple spaces on line $.";
          }
      }
    next if /^\s*\#/;
    if (/^(\S.*)/)
      {
	$previous_type = $type if $type;
	$type = $1;
	if ($type ne lc $type && !$seen{$type})
	  {
	    complain "$type contains upper-case letters";
	  }
	if (lc $type le lc $previous_type)
	  {
	    complain "$type is after $previous_type in mate-vfs.mime";
	  }
	if (!$described{$type} && !$in_mime{$type})
	  {
	    if (!$in_keys{$type})
	      {
		complain "$type is in mate-vfs.mime, but not mate-vfs.keys.in";
	      }
	    else
	      {
		complain "$type is in mate-vfs.mime, but has no description in mate-vfs.keys.in";
	      }
	  }
	$seen{$type} = 1;
	$in_mime{$type} = 1;
      }
    elsif (/^\s*$/)
      {
	$previous_type = $type if $type;
	$type = "";
      }
    elsif (/^\s+([a-z]+)(,\d+)?:/)
      {
        if (!$expected_keys{$1})
          {
            complain "saw unexpected key $1 at line $.";
          }
      }
    else
      {
        complain "could not parse line $.";
      }
  }
close MIME;

print "Reading mate-vfs.applications.\n";

@expected_keys =
  (
   "can_open_multiple_files",
   "command",
   "expects_uris",
   "name",
   "requires_terminal",
   "supported_uri_schemes",
   "uses_matevfs",
  );
@expected_keys{@expected_keys} = (1) x @expected_keys;

# special case for text/* for now
# could put in fancier handling later
my %in_applications = ( "text/*" => 1 );

my $previous_application = "";
my $application;

open MIME, "mate-vfs.applications" or die;
while (<MIME>)
  {
    chomp;
    if (/^\s+$/)
      {
        complain "blank line with spaces on line $.";
      }
    else
      {
        if (/^ /)
          {
            complain "leading spaces used instead of a tab on line $.";
          }
        if (/\s$/)
          {
            complain "trailing whitespace on line $.";
          }
        if (/\t /)
          {
            complain "space after tab on line $.";
          }
        if (/  /)
          {
            complain "multiple spaces on line $.";
          }
      }
    if (/\s=/)
      {
        complain "space before = on line $.";
      }
    if (/=\s/)
      {
        complain "space after = on line $.";
      }

    if (/^(\S.*)/)
      {
	$previous_application = $application if $application;
	$application = $1;
	if (lc $application le lc $previous_application)
	  {
	    complain "$application is after $previous_application in mate-vfs.applications";
	  }
        %seen_keys = ();
      }
    elsif (/^\s*([_a-z]+)\s*=\s*(.*)/)
      {
        if ($seen_keys{$1})
          {
            complain "saw key $1 twice at line $.";
          }
        else
          {
            $seen_keys{$1} = 1;
          }
        if ($1 eq "mime_types")
          {
            foreach my $type (split ",", $2)
              {
                next if $in_applications{$type};
                
                if ($type ne lc $type && !$seen{$type})
                  {
                    complain "$type contains upper-case letters";
                  }
                if (!$described{$type} && !$in_applications{$type})
                  {
                    if (!$in_keys{$type})
                      {
                        complain "$type is in mate-vfs.applications, but not mate-vfs.keys.in";
                      }
                    else
                      {
                        complain "$type is in mate-vfs.applications, but has no description in mate-vfs.keys.in";
                      }
                  }
                $seen{$type} = 1;
                $in_applications{$type} = 1;
              }
          }
        elsif (!$expected_keys{$1})
          {
            complain "saw unexpected key $1 at line $.";
          }
      }
    elsif (/^\s*(.+?)\s*=/)
      {
        complain "saw bad characters in key $1 at line $.";
      }
    elsif (!/^\s*$/)
      {
        complain "could not parse line $.";
      }
  }
close MIME;

exit $exit_status;
