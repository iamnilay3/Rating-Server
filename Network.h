/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

#ifndef NETWORK_H
#define NETWORK_H

#include "RatingServer.h"

using namespace std;

class CCircularBuffer
{
public:
	char content[2000];
	int dataStart;
	int nextInsert;
	bool restOfSequenceLengthDetermined;
	int restOfSequenceLength;
	bool commandTypeDetermined;
	int commandType;
	
	CCircularBuffer();
	
	void renew();
	int dataLoad();
};

void *get_in_addr(struct sockaddr *sa);
void sendCommand(int paramSocketFd, const void *paramSendBuffer, size_t paramLength);
void setupConnectionsAndManageCommunications(char * paramListeningPortNr, char * paramMaxConnections);
void handleIncomingData(int paramSocketFd);

#endif
