/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This is the code for a simple example client.
// To test some commands use the function commandTesting(int paramSocketFd) defined at the end of this file!

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
void sendCommand(int paramSocketFd, const void *paramSendBuffer, size_t paramLength);
int fetchSequence(int paramSocketFd, char * receiveBuffer);
void commandTesting(int paramSocketFd);

int main(int argc, char *argv[])
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	
	cout << endl << "Simple example client for Tirili's Rating Server" << endl << endl;
	
	if (argc != 3)
	{
		cout << endl << "Usage: ExampleClient Hostname PortNr" << endl << endl;
		return 1;
	}
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	
	if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}
	
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			perror("client: socket");
			continue;
		}
		
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
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
	
	commandTesting(sockfd); // Test some commands
	
	close(sockfd);
	
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

void sendCommand(int paramSocketFd, const void *paramSendBuffer, size_t paramLength)
{
	if (send(paramSocketFd, paramSendBuffer, paramLength, 0) == -1)
	{
		perror("send");
	}
}

int fetchSequence(int paramSocketFd, char * receiveBuffer)
{
	int restOfSequenceLength;
	char tempString[4];
	
	if (recv(paramSocketFd, receiveBuffer, 5, 0) == -1)
	{
		perror("recv");
		exit(1);
	}
	
	tempString[0] = receiveBuffer[0];
	tempString[1] = receiveBuffer[1];
	tempString[2] = receiveBuffer[2];
	tempString[3] = '\0';
	restOfSequenceLength = atoi(tempString);
	
	if ((restOfSequenceLength - 2 > 0) && (recv(paramSocketFd, receiveBuffer + 5, restOfSequenceLength - 2, 0) == -1))
	{
		perror("recv");
		exit(1);
	}
	
	return (restOfSequenceLength + 3);
}

class CAccount
{
public:
	string firstName, secondName, thirdName;
	
	string ttrsv;
	
	string publicNumberOfRatedTtrsGamesPlayed;
	
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
	
	void print()
	{
		cout << "First Name: " << firstName << "   Second Name: " << secondName << "   Third Name: " << thirdName << endl << endl;
		cout << "Ttrsv: " << ttrsv << " (" << publicNumberOfRatedTtrsGamesPlayed << ")" << endl << endl;
		cout << "Description: " << description  << endl << endl;
	}
};

void commandTesting(int paramSocketFd)
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
	
	// Testing the Ping command (command-id = 00):
	
	cout << "Testing the Ping command ..." << endl << endl;
	
	sendString = "00200";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramSocketFd, sendBuffer, 5);
	
	cout << "Ping command sent - waiting for server answer..." << endl << endl;
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	subsequence = new string();
	
	subsequence->append(receiveBuffer, 5);
	
	if (subsequence->compare("00201") == 0)
	{
		cout << "Pong received - ping works!" << endl << endl;
	}
	
	// Testing the Info Requesting - Player Info command (command-id = 10):
	
	cout << "Testing the Info Requesting - Player Info command ..." << endl << endl;
	
	sendString = "00610Lion";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramSocketFd, sendBuffer, 9);
	
	cout << "Info Requesting - Player Info command sent - waiting for server answer..." << endl << endl;
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	if (receiveBuffer[5] == 'f')
	{
		cout << "Received error signal from server: The name you specified might not be available." << endl << endl;
	}
	else
	{
		CAccount * account = new CAccount();
		
		*subsequence = "";
		
		i = 6;
		
		while ((*cp = receiveBuffer[i]) != '\0')
		{
			subsequence->append(cp, 1);
			i++;
		}
		
		account->firstName = *subsequence;
		
		*subsequence = "";
		
		i++;
		
		while ((*cp = receiveBuffer[i]) != '\0')
		{
			subsequence->append(cp, 1);
			i++;
		}
		
		account->secondName = *subsequence;
		
		*subsequence = "";
		
		i++;
		
		while ((*cp = receiveBuffer[i]) != '\0')
		{
			subsequence->append(cp, 1);
			i++;
		}
		
		account->thirdName = *subsequence;
		
		j = ((int) receiveBuffer[++i]) - 48;
		
		for (i = 0; i < j - 1; i++)
		{
			lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
			
			account->description.append(&receiveBuffer[5], 995);
		}
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->description.append(&receiveBuffer[5], lengthOfReceivedSequence - 5);
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->ttrsv.append(&receiveBuffer[5], lengthOfReceivedSequence - 6); 
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->publicNumberOfRatedTtrsGamesPlayed.append(&receiveBuffer[5], lengthOfReceivedSequence - 6); 
		
		if (account->publicNumberOfRatedTtrsGamesPlayed.compare("-0001") == 0)
		{
			account->publicNumberOfRatedTtrsGamesPlayed = "> 20";
		}
		
		account->print();
		
		delete account;
	}
	


	
	for (i = 0; i < 3; i++)
	{
		if ((lengthOfReceivedSequence = recv(paramSocketFd, receiveBuffer, 1000, 0)) == -1)
		{
			perror("recv");
			exit(1);
		}
		
		receiveBuffer[lengthOfReceivedSequence] = '\0';
		
		printf("client: received '%s'\n\n",receiveBuffer);
	}
}