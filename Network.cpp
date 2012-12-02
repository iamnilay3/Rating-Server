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

void *get_in_addr(struct sockaddr *sa)	// get sockaddr, IPv4 or IPv6:
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

void bufferToLineArray(CCircularBuffer * circularBuffer, char * target, int length)
{
	int i,k;
	
	if (circularBuffer->dataStart + length > 2000)
	{
		for (i = circularBuffer->dataStart, k = 0; i < 2000; i++, k++)
		{
			target[k] = circularBuffer->content[i];
		}
		
		for (i = 0; i < (circularBuffer->dataStart + length) % 2000; i++, k++)
		{
			target[k] = circularBuffer->content[i];
		}
	}
	else
	{
		for (i = circularBuffer->dataStart, k = 0; i < circularBuffer->dataStart + length; i++, k++)
		{
			target[k] = circularBuffer->content[i];
		}
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
	
	CCircularBuffer * circularBuffer;
	
	struct addrinfo hints, *ai, *p;
	
	socketBuffer = new CCircularBuffer[atoi(paramMaxConnections) + 4];
	
	int i, k, j, rv;
	
	char tempString[4];
	
	cout << "Listening for connections ..." << endl << endl;
	
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
						printf("Selectserver: New connection from %s on socket %d.\n\n", inet_ntop(remoteaddr.ss_family,
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
							printf("Selectserver: Socket %d hung up.\n\n", i);
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
						
						circularBuffer = &socketBuffer[i];
						
//nbytes -= 2; // Fix (Remove this line, when not testing with telnet!)
						for (j = 0, k = circularBuffer->nextInsert; j < nbytes; j++, k++)	// Copy into circular buffer!
						{
							circularBuffer->content[k % 2000] = buf[j];
						}
						circularBuffer->nextInsert = k % 2000;
						
/*cout << endl << "1) circularBuffer->dataStart = " << circularBuffer->dataStart << endl << endl;
print_dec_byte_content(&circularBuffer->content[circularBuffer->dataStart], 10);
cout << endl << "1) circularBuffer->dataStart = " << circularBuffer->dataStart << endl << endl;
cout << endl << "1) circularBuffer->nextInsert = " << circularBuffer->nextInsert << endl << endl;
*/						for (;;)
						{
							if (!circularBuffer->restOfSequenceLengthDetermined)
							{
								if (circularBuffer->dataLoad() >= 3)			// If enough data received,
								{							// get command rest length!
									tempString[0] = circularBuffer->content[circularBuffer->dataStart];
									tempString[1] =
									 circularBuffer->content[(circularBuffer->dataStart + 1) % 2000];
									tempString[2] =
									 circularBuffer->content[(circularBuffer->dataStart + 2) % 2000];
									
									circularBuffer->dataStart = (circularBuffer->dataStart + 3) % 2000;
									
									tempString[3] = '\0';
									
									circularBuffer->restOfSequenceLength = atoi(tempString);
									
									if (circularBuffer->restOfSequenceLength > 997)	// Watch out!
									{
										circularBuffer->dataStart = circularBuffer->nextInsert;
										
										break;
									}
									
									circularBuffer->restOfSequenceLengthDetermined = true;
								}
								else
								{
									break;
								}
							}
/*print_dec_byte_content(&circularBuffer->content[circularBuffer->dataStart], 10);
cout << endl << "2) circularBuffer->dataStart = " << circularBuffer->dataStart << endl << endl;
cout << endl << "2) circularBuffer->nextInsert = " << circularBuffer->nextInsert << endl << endl;
cout << endl << "2) circularBuffer->restOfSequenceLength = " << circularBuffer->restOfSequenceLength << endl << endl;*/
							
							if (!circularBuffer->commandTypeDetermined)
							{
								if (circularBuffer->dataLoad() >= 2)			// If enough data received,
								{							// get command type!
									tempString[0] = circularBuffer->content[circularBuffer->dataStart];
									tempString[1] =
									 circularBuffer->content[(circularBuffer->dataStart + 1) % 2000];
									
									circularBuffer->dataStart = (circularBuffer->dataStart + 2) % 2000;
									
									tempString[2] = '\0';
									
									circularBuffer->commandType = atoi(tempString);
									
									circularBuffer->commandTypeDetermined = true;
								}
								else
								{
									break;
								}
							}
/*print_dec_byte_content(&circularBuffer->content[circularBuffer->dataStart], 10);
cout << endl << "3) circularBuffer->dataStart = " << circularBuffer->dataStart << endl << endl;
cout << endl << "3) circularBuffer->nextInsert = " << circularBuffer->nextInsert << endl << endl;
cout << endl << "3) circularBuffer->restOfSequenceLength = " << circularBuffer->restOfSequenceLength << endl << endl;
cout << endl << "3) circularBuffer->commandType = " << circularBuffer->commandType << endl << endl;
cout << endl << "3) circularBuffer->dataLoad() = " << circularBuffer->dataLoad() << endl << endl;*/
//cin >> k;
							if (circularBuffer->dataLoad() >= circularBuffer->restOfSequenceLength - 2)
							{
							// If enough data received, get and handle command!
								handleIncomingData(i);
								
								circularBuffer->restOfSequenceLengthDetermined = false;
								circularBuffer->commandTypeDetermined = false;
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
	
	char sequenceStore[1000];
	string * subsequence;
	string s;
	char c;
	char cp[6];
	
	CAccount * account;
	CRatingListElement * element;
	
	string nickname;
	string password;
	
	switch (cid)
	{
		case 00:	// Ping
			
			sendString = "00201";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, 5);
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			cout << "Handled command: Ping" << endl << endl;
			
			break;
			
		case 10:	// Info Requesting - Player Info
			
			subsequence = new string();
			
			bufferToLineArray(circularBuffer, sequenceStore, rest);
			subsequence->append(sequenceStore, rest);
			
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
			sprintf(sendBuffer, "%03d", sendString.length() - 3);
			sendBuffer[3] = '1';
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
			
			delete subsequence;
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			cout << "Handled command: Info Requesting - Player Info" << endl << endl;
			
			break;
			
		case 15:	// Info Requesting - Rating List
			
			subsequence = new string();
			
			bufferToLineArray(circularBuffer, sequenceStore, rest);
			subsequence->append(sequenceStore, rest);
			
			if (subsequence->compare("1") != 0)
			{
				sendString = "00316f";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 6);
				
				circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
				
				break;
			}
			
			sendString = "???16s" + to_string(numberOfRegisteredAccounts);
			copyToBuffer(sendBuffer, sendString, sendString.length());
			sprintf(sendBuffer, "%03d", sendString.length() - 3);
			sendBuffer[3] = '1';
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			element = ratingListStart->nextElement;
			
			while(element->rank != -1)
			{
				sendString = "???17";
				sendString.append(element->account->getFirstName());
				sendString.append(1, '\0');
				sendString.append(element->account->getSecondName());
				sendString.append(1, '\0');
				sendString.append(element->account->getThirdName());
				sendString.append(1, '\0');
				
				copyToBuffer(sendBuffer, sendString, sendString.length());
				sprintf(sendBuffer, "%03d", sendString.length() - 3);
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
				
				sendString = "00718";
				sprintf(cp, "%04.0f", element->account->getTtrsv());
				sendString.append(cp, 4);
				sendString.append(1, '\0');
				
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
				
				sendString = "00819";
				sprintf(cp, "%05d", element->account->getPublicNrOfEvaluatedTtrsGames());
				sendString.append(cp, 5);
				sendString.append(1, '\0');
				
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
				
				element = element->nextElement;
			}
			
			delete subsequence;
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			cout << "Handled command: Info Requesting - Rating List" << endl << endl;
			
			break;
			
		case 20:	// Account Modification - Registration
			
			bufferToLineArray(circularBuffer, sequenceStore, rest);
			
			nickname = "";
			
			i = 0;
			
			while ((*cp = sequenceStore[i]) != '\0')
			{
				nickname.append(cp, 1);
				i++;
			}
			
			password = "";
			
			i++;
			
			while ((*cp = sequenceStore[i]) != '\0')
			{
				password.append(cp, 1);
				i++;
			}
			
			if (addAccount(idOfLastRegisteredAccount + 1, nickname, "", "", false, "", password, true) == 0)
			{
				sendString = "05721fCould not create account. This name is already in use.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 60);
			}
			else
			{
				sendString = "00321s";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 6);
			}
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			cout << "Handled command: Account Modification - Registration" << endl << endl;
			
			break;
/*		case 22:
			break;
		case 25:
			break;
		case 26:
			break;
		case 28:
			break;
		case 30:
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
			
			circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
			
			break;
	}
}
