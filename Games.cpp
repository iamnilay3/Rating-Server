/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This file contains the game management functions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Games.h"

using namespace std;

class CGameListElement
{
	int gameServerSocketFD;
	char * gameKey[4];
	
	CPlayerListElement * player;
	
	CGameListElement * nextGame;
};

class CPlayerListElement
{
	char * gameKey[4];
	char * playerKey[4];
	
	CPlayerListElement * nextPlayer;
};