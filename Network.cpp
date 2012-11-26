/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This file contains the network functions.

#include <iostream>
#include <string>
#include <cstdio>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "RatingServer.h"
#include "Network.h"
#include "Games.h"

using namespace std;

CCircularBuffer * socketBuffer;

CCircularBuffer::CCircularBuffer()	// Trivial constructor
{
	renew();
}

void CCircularBuffer::renew()
{
	dataStart = 0; nextInsert = 0; restOfSequenceLengthDetermined = false; commandTypeDetermined = false;
}

int CCircularBuffer::dataLoad()
{
	return ((nextInsert - dataStart) % 2000);
}

void print_dec_byte_content(char * pointer, int length)
{
	int i;
	
	for (i = 0; i < length; i++)
	{
		printf("\nDecimal (unsigned int) content of pointer[%d] is %d\n", i, (unsigned int) pointer[i]);
	}
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

void copyToBuffer(char * paramBuffer, string paramSendString, size_t paramSendStringLength)
{
	for (int i = 0; i < paramSendStringLength; i++)
	{
		paramBuffer[i] = paramSendString.at(i);
	}
}

void sendCommand(int paramSocketFd, const void *paramSendBuffer, size_t paramLength)
{
	if (send(paramSocketFd, paramSendBuffer, paramLength, 0) == -1)
	{
		perror("send");
	}
}

void setupConnectionsAndManageCommunications(char * paramListeningPortNr, char * paramMaxConnections)
{
	fd_set master;    // master file descriptor list
	fd_set read_fds;  // temp file descriptor list for select()
	int fdmax;        // maximum file descriptor number
	
	int listener;     // listening socket descriptor
	int newfd;        // newly accept()ed socket descriptor
	struct sockaddr_storage remoteaddr; // client address
	socklen_t addrlen;
	
	char buf[1001];    // buffer for client data
	int nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
	
	CCircularBuffer * cicularBuffer;
	
	struct addrinfo hints, *ai, *p;
	
	socketBuffer = new CCircularBuffer[atoi(paramMaxConnections) + 4];
	
	int i, k, j, rv;
	
	char tempString[4];

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&read_fds);
	
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, paramListeningPortNr, &hints, &ai)) != 0)
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
	
	// if we got here, it means we didn't get bound
	if (p == NULL)
	{
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	
	freeaddrinfo(ai); // all done with this
	
	// listen
	if (listen(listener, atoi(paramMaxConnections)) == -1)
	{
		perror("listen");
		exit(3);
	}
	
	// add the listener to the master set
	FD_SET(listener, &master);
	
	// keep track of the biggest file descriptor
	fdmax = listener; // so far, it's this one
	
	// main loop
	for(;;)
	{
		read_fds = master; // copy it
		if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
		{
			perror("select");
			exit(4);
		}
		
		// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++)
		{
			if (FD_ISSET(i, &read_fds))
			{ // we got one!!
				if (i == listener)
				{
					// handle new connections
					addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
					
					if (newfd == -1)
					{
						perror("accept");
					}
					else
					{
						FD_SET(newfd, &master); // add to master set
						if (newfd > fdmax)
						{    // keep track of the max
							fdmax = newfd;
						}
						printf("selectserver: new connection from %s on socket %d\n", inet_ntop(remoteaddr.ss_family,
							get_in_addr((struct sockaddr*)&remoteaddr), remoteIP, INET6_ADDRSTRLEN), newfd);
						
						socketBuffer[newfd].renew();	// Prepare the circular buffer!
					}
				}
				else
				{
					// handle data from a client
					if ((nbytes = recv(i, buf, sizeof buf, 0)) <= 0)
					{
						// got error or connection closed by client
						if (nbytes == 0)
						{
							// connection closed
							printf("selectserver: socket %d hung up\n", i);
						}
						else
						{
							perror("recv");
						}
						close(i); // bye!
						FD_CLR(i, &master); // remove from master set
					}
					else
					{
						// we got some data from a client
						
						cicularBuffer = &socketBuffer[i];
						
//nbytes -= 2; // Fix (Remove this line, when not testing with telnet!)
						for (j = 0, k = cicularBuffer->nextInsert; j < nbytes; j++, k++)	// Copy into circular buffer!
						{
							cicularBuffer->content[k % 2000] = buf[j];
						}
						cicularBuffer->nextInsert = k % 2000;
						
cout << endl << "1) cicularBuffer->dataStart = " << cicularBuffer->dataStart << endl << endl;
print_dec_byte_content(&cicularBuffer->content[cicularBuffer->dataStart], 10);
cout << endl << "1) cicularBuffer->dataStart = " << cicularBuffer->dataStart << endl << endl;
cout << endl << "1) cicularBuffer->nextInsert = " << cicularBuffer->nextInsert << endl << endl;
						for (;;)
						{
							if (!cicularBuffer->restOfSequenceLengthDetermined)
							{
								if (cicularBuffer->dataLoad() >= 3)			// If enough data received,
								{							// get command rest length!
									tempString[0] = cicularBuffer->content[cicularBuffer->dataStart];
									tempString[1] = cicularBuffer->content[(cicularBuffer->dataStart + 1) % 2000];
									tempString[2] = cicularBuffer->content[(cicularBuffer->dataStart + 2) % 2000];
									
									cicularBuffer->dataStart = (cicularBuffer->dataStart + 3) % 2000;
									
									tempString[3] = '\0';
									
									cicularBuffer->restOfSequenceLength = atoi(tempString);
									
									cicularBuffer->restOfSequenceLengthDetermined = true;
								}
								else
								{
									break;
								}
							}
/*print_dec_byte_content(&cicularBuffer->content[cicularBuffer->dataStart], 10);
cout << endl << "2) cicularBuffer->dataStart = " << cicularBuffer->dataStart << endl << endl;
cout << endl << "2) cicularBuffer->nextInsert = " << cicularBuffer->nextInsert << endl << endl;
cout << endl << "2) cicularBuffer->restOfSequenceLength = " << cicularBuffer->restOfSequenceLength << endl << endl;*/
							
							if (!cicularBuffer->commandTypeDetermined)
							{
								if (cicularBuffer->dataLoad() >= 2)			// If enough data received,
								{							// get command type!
									tempString[0] = cicularBuffer->content[cicularBuffer->dataStart];
									tempString[1] = cicularBuffer->content[(cicularBuffer->dataStart + 1) % 2000];
									
									cicularBuffer->dataStart = (cicularBuffer->dataStart + 2) % 2000;
									
									tempString[2] = '\0';
									
									cicularBuffer->commandType = atoi(tempString);
									
									cicularBuffer->commandTypeDetermined = true;
								}
								else
								{
									break;
								}
							}
print_dec_byte_content(&cicularBuffer->content[cicularBuffer->dataStart], 10);
cout << endl << "3) cicularBuffer->dataStart = " << cicularBuffer->dataStart << endl << endl;
cout << endl << "3) cicularBuffer->nextInsert = " << cicularBuffer->nextInsert << endl << endl;
cout << endl << "3) cicularBuffer->restOfSequenceLength = " << cicularBuffer->restOfSequenceLength << endl << endl;
cout << endl << "3) cicularBuffer->commandType = " << cicularBuffer->commandType << endl << endl;
cout << endl << "3) cicularBuffer->dataLoad() = " << cicularBuffer->dataLoad() << endl << endl;
//cin >> k;
							if (cicularBuffer->dataLoad() >= cicularBuffer->restOfSequenceLength - 2)
							{
							// If enough data received, get and handle command!
								handleIncomingData(i);
cout << "Handled incoming data! :)" << endl << endl;
							}
							else
							{
								break;
							}
						}
					}
				} // END handle data from client
			} // END got new incoming connection
		} // END looping through file descriptors
	} // END for(;;)
}

void handleIncomingData(int paramSocketFd)
{
	int i, j, k, l;
	
	CCircularBuffer * circularBuffer = &socketBuffer[paramSocketFd];
	
	int start = circularBuffer->dataStart;				// Data start
	int rest = circularBuffer->restOfSequenceLength - 2;		// Length of the rest of the sequence after the command-id
	int cid = circularBuffer->commandType;				// Command-id
	
	string sendString;
	char sendBuffer[1000];
	
	string * subsequence;
	string s;
	char cp[6];
	
	CAccount * account;
	
	switch (cid)
	{
		case 00:	// Ping
			sendString = "00201";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, 5);
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			break;
		case 10:	// Info Requesting	- Player info
			subsequence = new string();
			
			subsequence->append(&(circularBuffer->content[start]), rest);
			
			account = getAccountFromName(*subsequence);
			
			if (account == 0)
			{
				sendString = "00311f";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 6);
				
				circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
				
				break;
			}
			
			i = account->getDescription().length();
			j = i / 995;
			k = i % 995;
			l = j + (k != 0);
			
			sendString = "???11s";
			sendString.append(account->getFirstName());
			sendString.append(1, '\0');
			sendString.append(account->getSecondName());
			sendString.append(1, '\0');
			sendString.append(account->getThirdName());
			sendString.append(1, '\0');
			
			sprintf(cp, "%d", l);
			sendString.append(1, *cp);
			
			copyToBuffer(sendBuffer, sendString, sendString.length());
			sprintf(sendBuffer, "%03d", sendString.length());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			sendString = "";
			
			for (i = 0; i < j; i++)
			{
				sendString = "99712";
				sendString.append(account->getDescription().substr(i * 995, 995));
				
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
			}
			
			sendString = "";
			sprintf(cp, "%03d", 2 + k);
			sendString.append(cp, 3);
			sendString.append("12");
			sendString.append(account->getDescription().substr(j * 995, k));
			
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			sendString = "00713";
			sprintf(cp, "%04.0f", account->getTtrsv());
			sendString.append(cp, 4);
			sendString.append(1, '\0');
			
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			sendString = "00814";
			sprintf(cp, "%05d", account->getPublicNrOfEvaluatedTtrsGames());
			sendString.append(cp, 5);
			sendString.append(1, '\0');
			
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			break;
/*		case 15:
			break;
		case 20:
			break;
		case 22:
			break;
		case 25:
			break;
		case 26:
			break;
		case 28:
			break;
		case 50:
			break;
		case 52:
			break;
		case 54:
			break;
		case 60:
			break;
		case 61:
			break;
		case 62:
			break;
		case 63:
			break;
		case 64:
			break;
		case 66:
			break;
		case 70:
			break;
		case 71:
			break;
		case 72:
			break;
		case 74:
			break;
		case 85:
			break;
*/		default:
			cout << "This command is not yet implemented." << endl << endl;
			break;
	}
	
	circularBuffer->restOfSequenceLengthDetermined = false;
	circularBuffer->commandTypeDetermined = false;
}
