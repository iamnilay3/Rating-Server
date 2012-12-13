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
#include "Schedule.h"

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
	int result = ((nextInsert - dataStart) % 2000);
	
	if (result < 0)
	{
		result += 2000;
	}
	
	return result;
}

void print_dec_byte_content(char * pointer, int length)
{
	int i;
	
	for (i = 0; i < length; i++)
	{
		printf("Decimal (unsigned int) content of pointer[%d] is %d (%c).\n\n", i, pointer[i], pointer[i]);
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
	
	char buf[1000];    // buffer for client data
	int nbytes;
	
	char remoteIP[INET6_ADDRSTRLEN];
	
	int yes=1;        // for setsockopt() SO_REUSEADDR, below
	
	timeval timeToNextTask;
	
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
		
		if (select(fdmax+1, &read_fds, NULL, NULL, schedule.timeToNextTask(&timeToNextTask)) == -1)
		{
			perror("select");
			exit(4);
		}
		
		schedule.checkForTasksToExecute();
		
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
						
						for (j = 0, k = circularBuffer->nextInsert; j < nbytes; j++, k++)	// Copy into circular buffer!
						{
							circularBuffer->content[k % 2000] = buf[j];
						}
						circularBuffer->nextInsert = k % 2000;
						
						for (;;)
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
	CCircularBuffer * circularBuffer = &socketBuffer[paramSocketFd];
	
	int rest = circularBuffer->restOfSequenceLength - 2;		// Length of the rest of the sequence after the command-id
	
	string sendString;
	char sendBuffer[1000];
	char sequenceStore[1000];
	
	int i, j, k, l;
	char c;
	char cp[6];
	string * subsequence;
	string s;
	
	CAccount * account;
	CRatingListElement * element;
	
	string nickname;
	string password;
	string newPassword;
	
	string firstName, secondName, thirdName;
	bool privateNrOfEvaluatedTtrsGames;
	int nrOfExpectedDescriptionLines;
	
	int id;
	
	CTask * task;
	CModificationInformation * modificationInformation;
	
	bufferToLineArray(circularBuffer, sequenceStore, rest);
	
	switch (circularBuffer->commandType)
	{
		case 00:	// Ping
			
			// Protocol Receive-Syntax:	00
			
			sendString = "00201";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, 5);
			
			// Protocol Send-Syntax: 	01
			
			cout << "Handled command: Ping" << endl << endl;
			
			break;
			
		case 10:	// Info Requesting - Player Info
			
			// Protocol Receive-Syntax:	10:Nickname
			
			subsequence = new string();
			
			subsequence->append(sequenceStore, rest);
			
			account = getAccountFromName(*subsequence);
			
			if (account == 0)
			{
				sendString = "00311f";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 6);
				
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
			
			sendString.append(account->getPrivateNrOfEvaluatedTtrsGames() ? "t" : "f");
			
			sprintf(cp, "%d", l);
			sendString.append(1, *cp);
			
			copyToBuffer(sendBuffer, sendString, sendString.length());
			sprintf(sendBuffer, "%03d", sendString.length() - 3);
			sendBuffer[3] = '1';
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			// Protocol Send-Syntax: 
			//	11:{s,f}FirstName'\0'SecondName'\0'ThirdName'\0'{t,f}NrOfDescriptionCommandsThatWillFollow{t,f}
			
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
			
			// Protocol Send-Syntax:	12:DescriptionLine_i
			
			sendString = "00713";
			sprintf(cp, "%04.0f", account->getTtrsv());
			sendString.append(cp, 4);
			sendString.append(1, '\0');
			
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			// Protocol Send-Syntax:	13:RatingValue1'\0'RatingValue2'\0'...RatingValue_m'\0'
			
			sendString = "00814";
			sprintf(cp, "%05d", account->getPublicNrOfEvaluatedTtrsGames());
			sendString.append(cp, 5);
			sendString.append(1, '\0');
			
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			// Protocol Send-Syntax:	14:PlayedGames1'\0'PlayedGames2'\0'...PlayedGames_m'\0'
			
			delete subsequence;
			
			cout << "Handled command: Info Requesting - Player Info" << endl << endl;
			
			break;
			
		case 15:	// Info Requesting - Rating List
			
			// Protocol Receive-Syntax:	15:RatingValueNr
			
			subsequence = new string();
			subsequence->append(sequenceStore, rest);
			
			if (subsequence->compare("1") != 0)
			{
				sendString = "00316f";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 6);
				
				break;
			}
			
			sendString = "???16s" + to_string(numberOfRegisteredAccounts);
			copyToBuffer(sendBuffer, sendString, sendString.length());
			sprintf(sendBuffer, "%03d", sendString.length() - 3);
			sendBuffer[3] = '1';
			sendCommand(paramSocketFd, sendBuffer, sendString.length());
			
			// Protocol Send-Syntax:	16:{s,f}NrOfAccountsInListThatWillFollow
			
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
				
				// Protocol Send-Syntax:	17:FirstName'\0'SecondName'\0'ThirdName'\0'
				
				sendString = "00718";
				sprintf(cp, "%04.0f", element->account->getTtrsv());
				sendString.append(cp, 4);
				sendString.append(1, '\0');
				
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
				
				// Protocol Send-Syntax:	18:RatingValue1'\0'RatingValue2'\0'...RatingValue_m'\0'
				
				sendString = "00819";
				sprintf(cp, "%05d", element->account->getPublicNrOfEvaluatedTtrsGames());
				sendString.append(cp, 5);
				sendString.append(1, '\0');
				
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, sendString.length());
				
				// Protocol Send-Syntax:	19:PlayedGames1'\0'PlayedGames2'\0'...PlayedGames_m'\0'
				
				element = element->nextElement;
			}
			
			delete subsequence;
			
			cout << "Handled command: Info Requesting - Rating List" << endl << endl;
			
			break;
			
		case 20:	// Account Modification - Registration
			
			// Protocol Receive-Syntax:	20:Nickname'\0'Password'\0'
			
			nickname = "";
			
			i = 0;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				nickname.append(cp, 1);
				i++;
			}
			
			password = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
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
			
			// Protocol Send-Syntax:	21:{s,f}ErrorMessage
			
			cout << "Handled command: Account Modification - Registration" << endl << endl;
			
			break;
			
		case 22:	// Account Modification - Account Details Modification 1
			
			// Protocol Receive-Syntax:
			//	22:OldNickname'\0'Password'\0'FirstName'\0'SecondName'\0'ThirdName'\0'{t,f}NrOfDescriptionCommandsThatWillFollow
			
			nickname = "";
			
			i = 0;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				nickname.append(cp, 1);
				i++;
			}
			
			id = getIdFromName(nickname);
			
			if (id < 0)
			{
				sendString = "05024fThere is no account with that name you entered.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 53);
				
				break;
			}
			
			task = schedule.firstTask;
			
			while (task != 0)
			{
				if ((task->socketFd == paramSocketFd) || (task->id == id))
				{
					sendString = "03324fAnother change is in progress.";
					strcpy(sendBuffer, sendString.c_str());
					sendCommand(paramSocketFd, sendBuffer, 36);
					
					// Protocol Send-Syntax:	24:{s,f}ErrorMessage
					
					break;
				}
				
				task = task->nextTask;
			}
			
			if ((task != 0) && (task->socketFd == paramSocketFd) && (task->type == 1))
			{
				break;
			}
			
			password = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				password.append(cp, 1);
				i++;
			}
			
			firstName = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				firstName.append(cp, 1);
				i++;
			}
			
			secondName = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				secondName.append(cp, 1);
				i++;
			}
			
			thirdName = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				thirdName.append(cp, 1);
				i++;
			}
			
			privateNrOfEvaluatedTtrsGames = (sequenceStore[++i] == 't') ? true : false;
			nrOfExpectedDescriptionLines = ((int) sequenceStore[++i]) - 48;
			
			account = getAccountFromId(id);
			
			modificationInformation =
				new CModificationInformation(paramSocketFd, id, nrOfExpectedDescriptionLines,
								firstName, secondName, thirdName, privateNrOfEvaluatedTtrsGames);
			
			schedule.addTask(5, paramSocketFd, account->passwordMatches(password), 1, id, modificationInformation, "");
			
			cout << "Handled command: Account Modification - Account Details Modification 1" << endl << endl;
			
			break;
			
		case 23:	// Account Modification - Account Details Modification 2
			
			// Protocol Receive-Syntax:	23:DescriptionLine_n
			
			task = schedule.firstTask;
			
			while (task != 0)
			{
				if ((task->socketFd == paramSocketFd) && (task->type == 1))
				{
					break;
				}
				
				task = task->nextTask;
			}
			
			if (!((task != 0) && (task->socketFd == paramSocketFd) && (task->type == 1)))
			{
				cout << "Rejected unexpected command: Account Modification - Account Details Modification 2" << endl << endl;
				
				break;
			}
			
			modificationInformation = task->modificationInformation;
			
			modificationInformation->description.append(sequenceStore, rest);
			modificationInformation->nrOfReceivedDescriptionLines += 1;
			
			cout << "Handled command: Account Modification - Account Details Modification 2" << endl << endl;
			
			break;
			
		case 28:	// Account Modification - Password Modification
			
			// Protocol Receive-Syntax:	28:Nickname'\0'OldPassword'\0'NewPassword'0'
			
			nickname = "";
			
			i = 0;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				nickname.append(cp, 1);
				i++;
			}
			
			id = getIdFromName(nickname);
			
			if (id < 0)
			{
				sendString = "05029fThere is no account with that name you entered.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 53);
				
				break;
			}
			
			task = schedule.firstTask;
			
			while (task != 0)
			{
				if ((task->socketFd == paramSocketFd) || (task->id == id))
				{
					sendString = "03329fAnother change is in progress.";
					strcpy(sendBuffer, sendString.c_str());
					sendCommand(paramSocketFd, sendBuffer, 36);
					
					// Protocol Send-Syntax:	29:{s,f}ErrorMessage
					
					break;
				}
				
				task = task->nextTask;
			}
			
			if ((task != 0) && (task->socketFd == paramSocketFd) && (task->type == 1))
			{
				break;
			}
			
			password = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				password.append(cp, 1);
				i++;
			}
			
			newPassword = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				newPassword.append(cp, 1);
				i++;
			}
			
			account = getAccountFromId(id);
			
			schedule.addTask(5, paramSocketFd, account->passwordMatches(password), 2, id, 0, newPassword);
			
			cout << "Handled command: Account Modification - Password Modification" << endl << endl;
			
			break;
			
		case 30:	// Account Modification - Deletion
			
			// Protocol Receive-Syntax:	30:Nickname'\0'Password'\0'
			
			nickname = "";
			
			i = 0;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				nickname.append(cp, 1);
				i++;
			}
			
			id = getIdFromName(nickname);
			
			if (id < 0)
			{
				sendString = "05031fThere is no account with that name you entered.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramSocketFd, sendBuffer, 53);
				
				break;
			}
			
			task = schedule.firstTask;
			
			while (task != 0)
			{
				if ((task->socketFd == paramSocketFd) || (task->id == id))
				{
					sendString = "03331fAnother change is in progress.";
					strcpy(sendBuffer, sendString.c_str());
					sendCommand(paramSocketFd, sendBuffer, 36);
					
					// Protocol Send-Syntax:	31:{s,f}ErrorMessage
					
					break;
				}
				
				task = task->nextTask;
			}
			
			if ((task != 0) && (task->socketFd == paramSocketFd) && (task->type == 1))
			{
				break;
			}
			
			password = "";
			
			i++;
			
			while (((*cp = sequenceStore[i]) != '\0') && (i < rest))
			{
				password.append(cp, 1);
				i++;
			}
			
			account = getAccountFromId(id);
			
			schedule.addTask(5, paramSocketFd, account->passwordMatches(password), 3, id, 0, "");
			
			cout << "Handled command: Account Modification - Deletion" << endl << endl;
			
			break;
			
/*		case 50:
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
			
			sendString = "06599fThere is no command with this id or it is not yet implemented.";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramSocketFd, sendBuffer, 68);
			
			break;
	}
	
	circularBuffer->dataStart = (circularBuffer->dataStart + rest) % 2000;
}
