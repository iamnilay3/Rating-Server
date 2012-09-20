/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This file contains the network functions.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;

void setupConnection(char * listeningPortNr)
{
	int status;
	
	struct addrinfo hints;
	struct addrinfo *servinfo;	// will point to the results
	
	int sockFd;
	
	int socketReuse = 1;
	
	struct sockaddr_storage their_addr;
	socklen_t addr_size;

	int newFd;
	
	int messageFd;
}
