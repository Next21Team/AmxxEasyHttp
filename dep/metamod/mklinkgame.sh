#!/bin/sh
PATH=/bin:/usr/bin:/usr/sbin

build=0
if [ ! -s linkgame.cpp ]; then
	build=1
else
	for i in ents/*; do
		if [ $i -nt linkgame.cpp ]; then
			build=1	
		fi
	done
fi

if [ "$build" = 0 ]; then
	exit
fi
echo "Building linkgame.cpp" >& 2
lastyear=`date +%Y`

cat > linkgame.cpp <<EOC
// vi: set ts=4 sw=4 :
// vim: set tw=75 :

// linkgame.cpp - export entities from mod "games" back to the HL engine

/*
 * Copyright (c) 2001-${lastyear} Will Day <willday@hpgx.net>
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

egrep -hv '^#|^//|^ *$' ents/* | sort -u | sed "s/.*/LINK_ENTITY_TO_GAME(&);/" >> linkgame.cpp
