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

void commandTesting(int paramSocketFd)
{
	// This function contains the command tests.
	
	int i;
	
	char receiveBuffer[1001];
	int lengthOfReceivedSequence;
	
	string sendString;
	char sendBuffer[1000];
	
	// Testing the Ping command (command-id = 00):
	
	cout << "Testing the Ping command ..." << endl << endl;
	
	sendString = "00200";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramSocketFd, sendBuffer, 5);
	
	cout << "Ping command sent - waiting for server answer..." << endl << endl;
	
	if ((lengthOfReceivedSequence = recv(paramSocketFd, receiveBuffer, 1000, 0)) == -1)
	{
		perror("recv");
		exit(1);
	}
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n",receiveBuffer);
	
	// Testing the Info Requesting - Player Info command (command-id = 10):
	
	cout << "Testing the Info Requesting - Player Info command ..." << endl << endl;
	
	sendString = "00610Lion";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramSocketFd, sendBuffer, 9);
	
	cout << "Info Requesting - Player Info command sent - waiting for server answer..." << endl << endl;
	
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