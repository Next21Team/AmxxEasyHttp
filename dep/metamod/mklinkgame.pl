#!/usr/bin/perl

$build = 0;

#if file is 0 byte or DNE, build it
if (!(-s "linkgame.cpp"))
{
	$build = 1;
} else {
    #otherwise see if it needs to be updated
	opendir(DIR, "ents/");
	@files = readdir(DIR);
	for ($i=0; $i<=$#files; $i++)
	{
		if (newer("ents/$files[$i]", "linkgame.cpp"))
		{
			$build = 1;
		}
	}
	closedir(DIR);
}

die("No build necessary\n") unless $build;

print "Building linkgame.cpp...\n";

($s,$m,$h,$d,$m,$y,$wd,$doy,$dst) = localtime(time());

$lastyear = $y;

open(OUT, ">linkgame.cpp");

$hdr = <<EOC;
// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// linkgame.cpp - export entities from mod "games" back to the HL engine

/*
 * Copyright (c) 2001-$lastyear Will Day <willday@hpgx.net>
 *
 *    This file is part of Metamod.
 *
 *    Metamod is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    Metamod is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Metamod; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */

#include <extdll.h>		// always

#include "linkent.h"	// LINK_ENTITY_TO_GAME

// Entity list for gamedlls adapted from adminmod linkfunc.cpp.

// NOTE: This list of entities is automatically generated via the script
// "mklinkgame.sh".  See the files in the directory "ents" for the actual
// list of entities.
EOC

print OUT $hdr;

my @list;

opendir(DIR, "ents/");
@files = readdir(DIR);
closedir(DIR);

for ($i=0; $i<=$#files; $i++)
{
	open(FILE, "ents/".$files[$i]);
	while(<FILE>)
	{
		chomp;
		if (!($_ =~/^\s+$/) and !/^\/\//)
		{
			push(@list,$_);
		}
	}
	close(FILE);
}

@list = sort(@list);
$prev = "\\";
@out = grep($_ ne $prev && ($prev = $_), @list);

for($i=0;$i<=$#out;$i++)
{
	print OUT "LINK_ENTITY_TO_GAME($out[$i]);\n";
}

close(OUT);

sub newer
{
  	my($file1,$file2)=(@_);
  	if ((stat($file1))[9] > (stat($file2))[9])
  	{
  		return 1;
	}
  	return 0;
}
