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

void print_dec_byte_content(char * pointer, int length)
{
	int i;
	
	for (i = 0; i < length; i++)
	{
		printf("Decimal (unsigned int) content of pointer[%d] is %d (%c).\n\n", i, pointer[i], pointer[i]);
	}
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
	sendCommand(paramSocketFd, sendBuffer, 5);
	
	// Protocol Send-Syntax:	00
	
	cout << "Ping command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax: 	01
	
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
	
	// Protocol Send-Syntax:	10:Nickname
	
	cout << "Info Requesting - Player Info command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	11:{s,f}FirstName'\0'SecondName'\0'ThirdName'\0'{t,f}NrOfDescriptionCommandsThatWillFollow{t,f}
	
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
		
		account->privateNrOfEvaluatedTtrsGames = (receiveBuffer[++i] == 't') ? "true" : "false";
		
		j = ((int) receiveBuffer[++i]) - 48;
		
		// Protocol Receive-Syntax:	12:DescriptionLine_i
		
		for (i = 0; i < j - 1; i++)
		{
			lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
			
			account->description.append(&receiveBuffer[5], 995);
		}
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->description.append(&receiveBuffer[5], lengthOfReceivedSequence - 5);
		
		// Protocol Receive-Syntax:	13:RatingValue1'\0'RatingValue2'\0'...RatingValue_m'\0'
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->ttrsv.append(&receiveBuffer[5], lengthOfReceivedSequence - 6);
		
		// Protocol Receive-Syntax:	14:PlayedGames1'\0'PlayedGames2'\0'...PlayedGames_m'\0'
		
		lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
		
		account->publicNumberOfRatedTtrsGamesPlayed.append(&receiveBuffer[5], lengthOfReceivedSequence - 6); 
		
		if (account->publicNumberOfRatedTtrsGamesPlayed.compare("-0001") == 0)
		{
			account->publicNumberOfRatedTtrsGamesPlayed = "> 20";
		}
		
		account->print(true, true);
		
		cout << "Info Requesting - Player Info seems to work." << endl << endl;
		
		delete account;
	}
	
	// Testing the Info Requesting - Rating List command (command-id = 15):
	
	cout << "Testing the Info Requesting - Rating List command ..." << endl << endl;
	
	sendString = "003151";
	strcpy(sendBuffer, sendString.c_str());
	sendCommand(paramSocketFd, sendBuffer, 6);
	
	// Protocol Send-Syntax:	15:RatingValueNr
	
	cout << "Info Requesting - Rating List command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	16:{s,f}NrOfAccountsInListThatWillFollow
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	if (receiveBuffer[5] == 'f')
	{
		cout << "Received error signal from server: The rating value you specified might not be available." << endl << endl;
	}
	else
	{
		*subsequence = "";
		subsequence->append(&receiveBuffer[6], lengthOfReceivedSequence - 5);
		numberOfAccounts = atoi(subsequence->c_str());
		
		element = accountListStart;
		
		for (k = 0; k < numberOfAccounts; k++, element = element->nextElement)
		{
			element->account = new CAccount;
			
			account = element->account;
			
			// Protocol Receive-Syntax:	17:FirstName'\0'SecondName'\0'ThirdName'\0'
			
			lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
			
			*subsequence = "";
			
			i = 5;
			
			while ((*cp = receiveBuffer[i]) != '\0')
			{
				subsequence->append(cp, 1);
				i++;
			}
			
			account->firstName = *subsequence;
			
			i++;
			
			*subsequence = "";
			
			while ((*cp = receiveBuffer[i]) != '\0')
			{
				subsequence->append(cp, 1);
				i++;
			}
			
			account->secondName = *subsequence;
			
			i++;
			
			*subsequence = "";
			
			while ((*cp = receiveBuffer[i]) != '\0')
			{
				subsequence->append(cp, 1);
				i++;
			}
			
			account->thirdName = *subsequence;
			
			// Protocol Receive-Syntax:	18:RatingValue1'\0'RatingValue2'\0'...RatingValue_m'\0'
			
			lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
			
			*subsequence = "";
			subsequence->append(&receiveBuffer[5], 4);
			
			account->ttrsv = *subsequence;
			
			// Protocol Receive-Syntax:	19:PlayedGames1'\0'PlayedGames2'\0'...PlayedGames_m'\0'
			
			lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
			
			*subsequence = "";
			subsequence->append(&receiveBuffer[5], 5);
			
			account->publicNumberOfRatedTtrsGamesPlayed = *subsequence;
			
			if (account->publicNumberOfRatedTtrsGamesPlayed.compare("-0001") == 0)
			{
				account->publicNumberOfRatedTtrsGamesPlayed = "> 20";
			}
			
			element->nextElement = new CAccountListElement;
			
			account->print(false, false);
		}
		
		cout << "Info Requesting - Rating List seems to work." << endl << endl;
	}
	
	// Testing the Account Modification - Registration command (command-id = 20):
	
	cout << "Testing the Account Modification - Registration command ..." << endl << endl;
	
	nickname = "Claudio Arrau";
	password = "$money100$!";
	
	sendString = "???20";
	sendString.append(nickname);
	sendString.append(1, '\0');
	sendString.append(password);
	sendString.append(1, '\0');
	copyToBuffer(sendBuffer, sendString, sendString.length());
	sprintf(sendBuffer, "%03d", sendString.length() - 3);
	sendBuffer[3] = '2';
	sendCommand(paramSocketFd, sendBuffer, sendString.length());
	
	// Protocol Send-Syntax:	20:Nickname'\0'Password'\0'
	
	cout << "Account Modification - Registration command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	21:{s,f}ErrorMessage
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	cout << "Account Modification - Registration seems to work." << endl << endl;
	
	// Testing the Account Modification - Account Details Modification command 1 (command-id = 22):
	
	cout << "Testing the Account Modification - Account Details Modification command 1 ..." << endl << endl;
	
	nickname = "Claudio Arrau";
	password = "$money100$!";
	
	sendString = "???22";
	sendString.append(nickname);
	sendString.append(1, '\0');
	sendString.append(password);
	sendString.append(1, '\0');
	sendString.append("Sebastian Vettel");
	sendString.append(1, '\0');
	sendString.append("Magnus Carlsen");
	sendString.append(1, '\0');
	sendString.append("David Hilbert");
	sendString.append(1, '\0');
	sendString.append("t");
	sendString.append("3");
	
	copyToBuffer(sendBuffer, sendString, sendString.length());
	sprintf(sendBuffer, "%03d", sendString.length() - 3);
	sendBuffer[3] = '2';
	sendCommand(paramSocketFd, sendBuffer, sendString.length());
	
	// Protocol Send-Syntax:
	//	22:OldNickname'\0'Password'\0'FirstName'\0'SecondName'\0'ThirdName'\0'{t,f}NrOfDescriptionCommandsThatWillFollow
	
	cout << "Account Modification - Account Details Modification command 1 (22) sent - command 2 (23) must be sent now." << endl << endl;
	
	// Testing the Account Modification - Account Details Modification command 2 (command-id = 23):
	
	cout << "Testing the Account Modification - Account Details Modification command 2 ..." << endl << endl;
	
	for (i = 0; i < 2; i++)
	{
		sendString = "99723";
		
		for (j = 0; j < 995; j++)
		{
			sendString.append("A");
		}
		
		copyToBuffer(sendBuffer, sendString, sendString.length());
		sendCommand(paramSocketFd, sendBuffer, sendString.length());
	}
	
	sendString = "04123And this is the end of the description.";
	
	copyToBuffer(sendBuffer, sendString, sendString.length());
	sendCommand(paramSocketFd, sendBuffer, sendString.length());
	
	// Protocol Send-Syntax:	23:DescriptionLine_n
	
	cout << "Account Modification - Account Details Modification command 2 (23) sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	24:{s,f}ErrorMessage'\0'
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	cout << "Account Modification - Account Details Modification seems to work." << endl << endl;
	
	// Testing the Account Modification - Password Modification command (command-id = 28):
	
	cout << "Testing the Account Modification - Password Modification command ..." << endl << endl;
	
	sendString = "???28";
	sendString.append("David Hilbert");
	sendString.append(1, '\0');
	sendString.append("$money100$!");
	sendString.append(1, '\0');
	sendString.append("uE(2lai9?s");
	sendString.append(1, '\0');
	
	copyToBuffer(sendBuffer, sendString, sendString.length());
	sprintf(sendBuffer, "%03d", sendString.length() - 3);
	sendBuffer[3] = '2';
	sendCommand(paramSocketFd, sendBuffer, sendString.length());
	
	// Protocol Send-Syntax:	28:Nickname'\0'OldPassword'\0'NewPassword'0'
	
	cout << "Account Modification - Password Modification command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	29:{s,f}ErrorMessage
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	cout << "Account Modification - Password Modification seems to work." << endl << endl;
	
	// Testing the Account Modification - Deletion command (command-id = 30):
	
	cout << "Testing the Account Modification - Deletion command ..." << endl << endl;
	
	sendString = "???30";
	sendString.append("David Hilbert");
	sendString.append(1, '\0');
	sendString.append("uE(2lai9?s");
	sendString.append(1, '\0');
	
	copyToBuffer(sendBuffer, sendString, sendString.length());
	sprintf(sendBuffer, "%03d", sendString.length() - 3);
	sendBuffer[3] = '3';
	sendCommand(paramSocketFd, sendBuffer, sendString.length());
	
	// Protocol Send-Syntax:	30:Nickname'\0'Password'\0'
	
	cout << "Account Modification - Deletion command sent - waiting for server answer..." << endl << endl;
	
	// Protocol Receive-Syntax:	31:{s,f}ErrorMessage
	
	lengthOfReceivedSequence = fetchSequence(paramSocketFd, receiveBuffer);
	
	receiveBuffer[lengthOfReceivedSequence] = '\0';
	
	printf("client: received '%s'\n\n", receiveBuffer);
	
	cout << "Account Modification - Deletion seems to work." << endl << endl;
}