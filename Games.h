/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

#ifndef GAMES_H
#define GAMES_H

#include <stdio.h>
#include <string>

#include "RatingServer.h"

using namespace std;

class CPlayerListElement
{
public:
	int playerNr;
	
	char * gameKey[4];
	char * playerKey[4];
	
	CAccount * account;
	string chosenNickname;
	
	float efficiency;
	
	CPlayerListElement * nextPlayer;
};

class CGameListElement
{
public:
	int gameServerSocketFD;
	char * gameKey[4];
	
	CPlayerListElement * playerOfTeam1;
	CPlayerListElement * playerOfTeam2;
	
	CGameListElement * nextElement;
};

class CResultListElement : public CGameListElement
{
public:
	int frequency;
	
	char winnerTeam;		// 1,2 or 0 (for a draw)
	
	bool isEqualTo(CResultListElement paramCResultListElement);
	bool isSimilarTo(CResultListElement paramCResultListElement);
};

#endif
