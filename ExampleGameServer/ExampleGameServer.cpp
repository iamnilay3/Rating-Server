/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This is the code for a simple example game server.
// To test some commands use the function commandTesting(int paramRatingServerSocketFd, int * paramGameClientSocketFd)
// defined at the end of this file!

#include <iostream>
#include <string>
#include <cstdio>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

void *get_in_addr(struct sockaddr *sa);
void sendCommand(int paramRatingServerSocketFd, const void *paramSendBuffer, size_t paramLength);
int fetchSequence(int paramRatingServerSocketFd, char * receiveBuffer);
void commandTesting(int paramRatingServerSocketFd, int * paramGameClientSocketFd);

int main(int argc, char *argv[])
{
	// Variables for the GamesServer-RatingServer-Connection being the client
	
	int ratingServerSocketFd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	
	// Variables for the GameServer-GameClient-Connection being the server
	
	int listener;     			// listening socket descriptor
	int gameClientSocketFd[6];		// newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr;	// client address
	socklen_t addrlen;
	char remoteIP[INET6_ADDRSTRLEN];
	int yes=1;        			// for setsockopt() SO_REUSEADDR, below
	struct addrinfo *ai;
	
	char buf[1000];    	// buffer for client data
	int nbytes;
	
	int i;
	
	// Welcome procedure
	
	cout << endl << "Simple example game server for Tirili's Rating Server" << endl << endl;
	
	if (argc != 5)
	{
		cout << "Usage: ExampleGameServer ListeningPortNr MaxConnections RatingServerHostname RatingServerPortNr" << endl << endl;
		return 1;
	}
	
	// Setup of the GamesServer-RatingServer-Connection being the client
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((rv = getaddrinfo(argv[3], argv[4], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	for(p = servinfo; p != NULL; p = p->ai_next)	// loop through all the results and connect to the first we can
	{
		if ((ratingServerSocketFd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}
		
		if (connect(ratingServerSocketFd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(ratingServerSocketFd);
			perror("client: connect");
			continue;
		}
		
		break;
	}
	
	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}
	
	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	printf("client: connecting to %s\n\n", s);
	
	freeaddrinfo(servinfo); // all done with this structure
	
	// Setup of the GameServer-GameClient-Connection being the server
	
	memset(&hints, 0, sizeof hints);	// get us a socket and bind it
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, argv[1], &hints, &ai)) != 0)
	{
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next)
	{
		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (listener < 0)
		{
			continue;
		}
		
		// lose the pesky "address already in use" error message
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
		
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0)
		{
			close(listener);
			continue;
		}
		
		break;
	}
	
	if (p == NULL)		// if we got here, it means we didn't get bound
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai);	// all done with this
	
	if (listen(listener, atoi(argv[2])) == -1)	// listen
	{
		perror("listen");
		exit(3);
	}
	
	for (i = 0; i < 6; i++)
	{
		addrlen = sizeof remoteaddr;	// handle new connections
		gameClientSocketFd[i] = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
		
		if (gameClientSocketFd[i] == -1)
		{
			perror("accept");
		}
		else
		{
			printf("Selectserver: New connection from %s on socket %d.\n\n",
				inet_ntop(remoteaddr.ss_family, get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN),
				gameClientSocketFd[i]);
		}
	}
	
	// Executing the main purpose of the program
	
	commandTesting(ratingServerSocketFd, gameClientSocketFd); // Test some commands
	
	// Disconnecting the GamesServer-RatingServer-Connection being the client
	
	for (i = 0; i < 6; i++)
	{
		close(gameClientSocketFd[i]);
	}
	
	close(ratingServerSocketFd);
	
	// Disconnecting the GameServer-GameClient-Connection being the server
	
	// Ending the GameServer execution
	
	return 0;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}
	
	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void print_dec_byte_content(char * pointer, int length)
{
	int i;
	
	for (i = 0; i < length; i++)
	{
		printf("Decimal (unsigned int) content of pointer[%d] is %d (%c).\n\n", i, pointer[i], pointer[i]);
	}
}

void sendCommand(int paramRatingServerSocketFd, const void *paramSendBuffer, size_t paramLength)
{
	if (send(paramRatingServerSocketFd, paramSendBuffer, paramLength, 0) == -1)
	{
		perror("send");
	}
}

int fetchSequence(int paramRatingServerSocketFd, char * receiveBuffer)
{
	int restOfSequenceLength;
	char tempString[4];
	
	if (recv(paramRatingServerSocketFd, receiveBuffer, 5, 0) == -1)
	{
		perror("recv");
		exit(1);
	}
	
	tempString[0] = receiveBuffer[0];
	tempString[1] = receiveBuffer[1];
	tempString[2] = receiveBuffer[2];
	tempString[3] = '\0';
	restOfSequenceLength = atoi(tempString);
	
	if ((restOfSequenceLength - 2 > 0) && (recv(paramRatingServerSocketFd, receiveBuffer + 5, restOfSequenceLength - 2, 0) == -1))
	{
		perror("recv");
		exit(1);
	}
	
	return (restOfSequenceLength + 3);
}

void copyToBuffer(char * paramBuffer, string paramSendString, size_t paramSendStringLength)
{
	for (int i = 0; i < paramSendStringLength; i++)
	{
		paramBuffer[i] = paramSendString.at(i);
	}
}

class CAccount
{
public:
	string firstName, secondName, thirdName;
	
	string ttrsv;
	
	string publicNumberOfRatedTtrsGamesPlayed;
	
	string privateNrOfEvaluatedTtrsGames;
	
	string description;
	
	CAccount()
	{
	}
	
	CAccount(string paramFirstName, string paramSecondName, string paramThirdName,
		 int paramTtrsv, int paramPublicNumberOfRatedTtrsGamesPlayed, string paramDescription)
	{
		firstName = paramFirstName; secondName = paramSecondName; thirdName = paramThirdName;
		ttrsv = paramTtrsv; publicNumberOfRatedTtrsGamesPlayed = paramPublicNumberOfRatedTtrsGamesPlayed; description = paramDescription;
	}
	
	void print(bool paramPrintDescription, bool paramPrintPrivateNrOfEvaluatedTtrsGames)
	{
		cout << "First Name: " << firstName << "   Second Name: " << secondName << "   Third Name: " << thirdName << endl << endl;
		cout << "Ttrsv: " << ttrsv << " (" << publicNumberOfRatedTtrsGamesPlayed << ")" << endl << endl;
		if (paramPrintDescription)
		{
			cout << "Description: " << description  << endl << endl;
		}
		if (paramPrintPrivateNrOfEvaluatedTtrsGames)
		{
			cout << "Private Number Of Rated Ttrs Games Played: " << privateNrOfEvaluatedTtrsGames  << endl << endl;
		}
	}
};

class CAccountListElement
{
public:
	CAccount * account;
	
	CAccountListElement * nextElement;
};

void commandTesting(int paramRatingServerSocketFd, int * paramGameClientSocketFd)
{
	// This function contains the command tests.
	
	int i, j, k, l;
	
	char receiveBuffer[1001];
	int lengthOfReceivedSequence;
	
	string sendString;
	char sendBuffer[1000];
	
	string * subsequence;
	char compareCharArray[1000];
	char cp[6];
	
	CAccount * account;
	CAccountListElement * accountListStart = new CAccountListElement;
	CAccountListElement * element;
	
	int numberOfAccounts;
	
	string nickname;
	string password;
	
	// Testing the Ping command (command-id = 00):
	
	cout << "Testing the Ping command ..." << endl << endl;
	
	sendString = "00200";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramRatingServerSocketFd, sendBuffer, 5);
	
	// Protocol Send-Syntax:	00
	
	cout << "Ping command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax: 	01
	
	lengthOfReceivedSequence = fetchSequence(paramRatingServerSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	subsequence = new string();
	
	subsequence->append(receiveBuffer, 5);
	
	if (subsequence->compare("00201") == 0)
	{
		cout << "Pong received - ping works!" << endl << endl;
	}
	
	// Answering expected ping command from ExampleGameClient with RoleNr = 1:
	
	lengthOfReceivedSequence = fetchSequence(paramGameClientSocketFd[1], receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	// Protocol Receive-Syntax:	00
	
	sendString = "00201";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramGameClientSocketFd[1], sendBuffer, 5);
	
	// Protocol Send-Syntax: 	01
	
	cout << "Handled command: Ping" << endl << endl;
}