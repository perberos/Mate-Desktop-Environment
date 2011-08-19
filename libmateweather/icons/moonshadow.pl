#!/usr/bin/perl -w

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see
# <http://www.gnu.org/licenses/>.

use Cwd;
use Getopt::Long;
use File::Basename;

#
# This script is used to generate the shaded cresent or
# gibbous shapes in a moon icon.  After creating icon files
# weather-clear-night.svg and weather-few-clouds.svg or
# any other icon file which includes a moon image, find the path
# that represent the inner, lighter colored outline of the moon.
# The center and radius arguments that you pass to this script
# should be based on the location and the radius less half the path
# width after adjusting for the path's 'transform' attribute.
#
# Once you've determined that, go to the end of the layer object
# (the following '</g>').  Add the following, adjusting 'style'
# and 'id' attributes if desired:
#
#   <path
#       sodipodi:nodetypes="ccaaac"
#       style="opacity:0.25;fill:#000000;fill-opacity:1;stroke:#000000;stroke-width:0;stroke-miterlimit:4;stroke-linejoin:miter;stroke-opacity:1;"
#       id="shadow"
#       d="M __X0__,__Y0__
#          C __X1__,__Y0__ __X2__,__Y2__ __X2__,__Y3__
#          S __X1__,__Y4__ __X0__,__Y4__
#          C __X6__,__Y4__ __X7__,__Y7__ __X7__,__Y3__
#          S __X6__,__Y0__ __X0__,__Y0__
#          z" />
#
# The first three points define the arc on the outside edge from the
# the top center at <X0,Y0> through center left or right <X2,Y3> to bottom
# center <X0,Y4>.  The final two define the Bezier curve for terminator
# (the edge between light and dark portions of the moon's surface) from
# <X0,Y4> through <X7,Y3> and ending back at the start.
#
# The string __ANGLE__ will be replaced within each generated file
# with the three-digit angle value.
#
# An additional __OPACITY__ value can be included in the template.
# This controls the shading of an object that would depend on the
# amount of light being reflected off the moon; e.g.: a darker cloud
# image when the moon is new and a lighter one when full.  The resulting
# value will vary from 0.125 at the darkest to 0 for brightest
#
# Note that the file "*-night-180.svg" file is not generated.  The routine
# in libmateweather that returns icon names knows to not append '-180' to the
# return value.

GetOptions("center=s"=>\$center,
		   "radius=f"=>\$radius);

# Slurp the svg template
die "missing icon template" unless $#ARGV == 0;
my $src = $ARGV[0];
die "$src not found" unless -f $src;
{
  local ($/,undef);
  open(T, $src) || die "can't open $src: $!";
  $fmt=<T>;
}

# The "-c" option can be either an "x,y" value
# or just a single numeric ("x,x")

my $x;
my $y;
print "center=$center\n";
if ($center =~ /^(.+)\,(.+)$/) {
  $x = $1;
  $y = $2;
} elsif ($center =~ /^(.+)$/) {
  $y = $x = $1;
} else {
  die "can't parse '-c': should be <n> or <n,n>\n";
}

use constant PI => atan2(1,1)*4;
my $b = $radius * 4/3 * (sqrt(2) - 1.);

my $x0=$x;
my $x1=$x - $b;
my $x2=$x - $radius;

my $y0=$y+$radius;
my $y2=$y + $b;
my $y3=$y;
my $y4=$y-$radius;

$_ = $fmt;

eval s/__X0__/$x0/g;
eval s/__Y0__/$y0/g;
eval s/__Y2__/$y2/g;
eval s/__Y3__/$y3/g;
eval s/__Y4__/$y4/g;

my $pass1=$_;

my $out = basename($src,".svg");
for ($d=0; $d<360; $d+=10) {
  if ($d == 180) {
	$x1=$x+$b;
	$x2=$x+$radius;
	next;
  }

  my $angle = sprintf "%03d",$d;
  my $name = sprintf "%s-%03d.svg",$out,$d;

  open (D, ">$name") || die "can't open $name: $!";

  my $h= cos( ($d % 180) * PI / 180.);
  my $x6=$x+$h*$b;
  my $x7=$x+$h*$radius;
  my $y7=$y - $b;
  my $opacity = (1+ cos($d * PI / 180.)) / 16.;
  $_=$pass1;

  eval s/__X1__/$x1/g;
  eval s/__X2__/$x2/g;
  eval s/__X6__/$x6/g;
  eval s/__X7__/$x7/g;
  eval s/__Y7__/$y7/g;
  eval s/__OPACITY__/$opacity/g;
  eval s/__ANGLE__/$angle/g;
  print D;
  close (D);
}
