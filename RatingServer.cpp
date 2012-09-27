/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// A simple Rating List Server for Games like OpenRA

#include <iostream>
#include <string>
#include <cstdio>
#include <fstream>
#include <sstream>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "RatingServer.h"
#include "Network.h"
#include "Games.h"

using namespace std;

int numberOfRegisteredAccounts = 0;
int idOfLastRegisteredAccount  = 0;

CAccountListElement * accountListStart;
CAccountListElement * accountListEnd;

CRatingListElement * ratingListStart;
CRatingListElement * ratingListEnd;

int main(int argc, char * argv[])
{
	cout << endl << "Tirili's Rating Server" << endl << endl;
	
	if (argc != 5)
	{
		cout << endl << "Usage: RatingServer FileToLoadFrom FileToSaveTo ListeningPortNr MaxConnections" << endl << endl;
		return 1;
	}
		
	setupAccountList();
	setupRatingList();
	
	loadFromFile(argv[1]);	// Load data from file AccountData.trs
	
/*
	// Account managing tests:
	
	addAccount(1, "Lux", "Eagle", "Lion", false, "Some cool dude! Visit http://content.open-ra.org !", "fghr", false);
	addAccount(2, "Boris", "", "", false, "yo I'm da Boris!", "kizhr4", false);
	CAccount * account = addAccount(4, "JoSI", "mayhem", "", true, "deluxe", "j2hz53rfgsw", false);
	
	printRatingList();
	
	updateRating(2, 1293.47);
	updateRating(1, 874.232);
	
	cout << "Games: " << account->getPublicNrOfEvaluatedTtrsGames() << endl << endl;
	
	account->setNrOfEvaluatedTtrsGames(21);
	
	cout << "Games: " << account->getPublicNrOfEvaluatedTtrsGames() << endl << endl;
	
	addAccount(5, "Pirc", "", "jatso", true, "_#*#_", "w345trgfhuxd", false);
*/
	
	printRatingList();
	printAccountList();
	
	setupConnectionsAndManageCommunications(argv[3], argv[4]);
	
//	saveToFile(argv[2]);
	
	return 0;
}

CAccount::CAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName,	// Trivial constructor
			bool paramPrivateNrOfEvaluatedTtrsGames, string paramDescription, string paramPassword)
{
	id = paramId;
	
	numberOfRegisteredAccounts++;
	
	if (paramId > idOfLastRegisteredAccount)
	{
		idOfLastRegisteredAccount = paramId;
	}
	
	firstName = paramFirstName; secondName = paramSecondName; thirdName = paramThirdName;
	
	description = paramDescription;
	
	password = paramPassword;
	
	ttrsv = 1000;					// Let 1000 be the future average rating value of all players
	
	nrOfEvaluatedTtrsGames = 0;
	
	privateNrOfEvaluatedTtrsGames = paramPrivateNrOfEvaluatedTtrsGames;
}

CAccount::~CAccount()
{
	numberOfRegisteredAccounts--;
}

int CAccount::getId()
{
	return id;
}

string CAccount::getFirstName()
{
	return firstName;
}

string CAccount::getSecondName()
{
	return secondName;
}

string CAccount::getThirdName()
{
	return thirdName;
}

float CAccount::getTtrsv()
{
	return ttrsv;
}

int CAccount::getNrOfEvaluatedTtrsGames()
{
	return nrOfEvaluatedTtrsGames;
}

int CAccount::getPublicNrOfEvaluatedTtrsGames()
{
	if (privateNrOfEvaluatedTtrsGames && (nrOfEvaluatedTtrsGames > 20))	return (-1);
	else									return nrOfEvaluatedTtrsGames;
}

void CAccount::setNrOfEvaluatedTtrsGames(int paramNrOfEvaluatedTtrsGames)
{
	nrOfEvaluatedTtrsGames = paramNrOfEvaluatedTtrsGames;
}

bool CAccount::getPrivateNrOfEvaluatedTtrsGames()
{
	return privateNrOfEvaluatedTtrsGames;
}

void CAccount::setPrivateNrOfEvaluatedTtrsGames(bool paramPrivateNrOfEvaluatedTtrsGames)
{
	privateNrOfEvaluatedTtrsGames = paramPrivateNrOfEvaluatedTtrsGames;
}

string CAccount::getDescription()
{
	return description;
}

void CAccount::incrementNrOfEvaluatedTtrsGames()
{
	nrOfEvaluatedTtrsGames++;
}

bool CAccount::passwordMatches(string paramPassword)
{
	return (password.compare(paramPassword) == 0);
}

void CAccount::printDetails()					// Show account details
{
	cout << "ID: " << id << endl;
	cout << "First Name: " << firstName << "    Second Name: " << secondName << "    Third Name: " << thirdName << endl;
	cout << "TTRSv: " << ttrsv << endl;
	cout << "Number of evaluated TTRS games: " << nrOfEvaluatedTtrsGames << endl;
	cout << "Description: " << description << endl << endl;
}

CAccountListElement::CAccountListElement() { id = 0; account = 0; prevElement = 0; nextElement = 0; }			// Trivial constructor

CRatingListElement::CRatingListElement() { rank = 0; ttrsv = -1000000; account = 0; prevElement = 0; nextElement  = 0;}	// Trivial constructor

void setupAccountList()		// Create a first empty account-list
{
	accountListStart = new CAccountListElement;
	accountListEnd   = new CAccountListElement;
	
	accountListStart->id = -2;
	accountListStart->nextElement = accountListEnd;
	
	accountListEnd->id = -1;
	accountListEnd->prevElement = accountListStart;
}

void setupRatingList()		// Create a first empty rating-list
{
	ratingListStart = new CRatingListElement;
	ratingListEnd   = new CRatingListElement;
	
	ratingListStart->rank = -2;
	ratingListStart->nextElement = ratingListEnd;
	
	ratingListEnd->rank = -1;
	ratingListEnd->prevElement = ratingListStart;
}

void insertIntoAccountList(CAccountListElement * accountListElement)
{
	CAccountListElement * element = accountListStart;
	
	while ((element->id == -2) || ((element->id != -1) && (element->id < accountListElement->id)))
	{
		element = element->nextElement;
	}
	
	element->prevElement->nextElement = accountListElement;
	
	accountListElement->prevElement = element->prevElement;
	
	element->prevElement = accountListElement;
	
	accountListElement->nextElement = element;
}

void insertIntoRatingList(CRatingListElement * ratingListElement)
{
	CRatingListElement * element = ratingListStart;
	
	while ((element->rank == -2) || ((element->rank != -1) && (element->ttrsv >= ratingListElement->ttrsv)))
	{
		element = element->nextElement;
	}
	
	element->prevElement->nextElement = ratingListElement;
	
	ratingListElement->prevElement = element->prevElement;
	
	element->prevElement = ratingListElement;
	
	ratingListElement->nextElement = element;
}

CAccountListElement * extractAccountListElement(int paramId)
{
	CAccountListElement * element = accountListStart;
	
	while ((element->id == -2) || ((element->id != -1) && (element->account->getId() != paramId)))
	{
		element = element->nextElement;
	}
	
	element->prevElement->nextElement = element->nextElement;
	
	element->nextElement->prevElement = element->prevElement;
	
	return element;
}

CRatingListElement * extractRatingListElement(int paramId)
{
	CRatingListElement * element = ratingListStart;
	
	while ((element->rank == -2) || ((element->rank != -1) && (element->account->getId() != paramId)))
	{
		element = element->nextElement;
	}
	
	element->prevElement->nextElement = element->nextElement;
	
	element->nextElement->prevElement = element->prevElement;
	
	return element;
}

CAccount * addAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName,
		      bool paramPrivateNrOfEvaluatedTtrsGames, string paramDescription, string paramPassword, bool verbose)
{
	if ((getIdFromName(paramFirstName) > 0) || (getIdFromName(paramSecondName) > 0) || (getIdFromName(paramThirdName) > 0))
	{
		cout << "An account with one of the specified names already exists. Creation canceled!" << endl;
		
		return 0;
	}
	
	CAccount * account = new CAccount(paramId, paramFirstName, paramSecondName, paramThirdName,
					  paramPrivateNrOfEvaluatedTtrsGames, paramDescription, paramPassword);
	
	
	CAccountListElement * accountListElement = new CAccountListElement;
	
	accountListElement->id = account->getId();
	accountListElement->account = account;
	
	insertIntoAccountList(accountListElement);
	
	
	CRatingListElement  * ratingListElement  = new CRatingListElement;
	
	ratingListElement->ttrsv = account->getTtrsv();
	ratingListElement->account = account;
	
	insertIntoRatingList(ratingListElement);
	
	
	if (verbose)
	{
		cout << "Added account: " << endl;
		account->printDetails();
	}
	
	return account;
}

void removeAccount(int paramId)
{
	delete extractRatingListElement(paramId);
	
	CAccountListElement * element = extractAccountListElement(paramId);
	
	delete element->account;
	
	delete element;
	
	cout << "Removed account (id = " << paramId << ")." << endl << endl;
}

int getIdFromName(string paramName)
{
	if (paramName == "") return -1;
	
	CAccountListElement * element = accountListStart;
	
	while((element->id == -2) || ((element->id != -1) && (element->account->getFirstName() != paramName)
					&& (element->account->getSecondName() != paramName)
					&& (element->account->getThirdName() != paramName)))
	{
		element = element->nextElement;
	}
	
	return (element->id);
}

CAccount * getAccountFromName(string paramName)
{
	if (paramName == "") return 0;
	
	CAccountListElement * element = accountListStart;
	
	while((element->id == -2) || ((element->id != -1) && (element->account->getFirstName() != paramName)
					&& (element->account->getSecondName() != paramName)
					&& (element->account->getThirdName() != paramName)))
	{
		element = element->nextElement;
	}
	
	return (element->account);
}

void updateRating(int paramId, float paramTtrsv, bool verbose)
{
	CRatingListElement * element = extractRatingListElement(paramId);
	
	element->account->ttrsv = paramTtrsv;
	element->ttrsv = paramTtrsv;
	
	insertIntoRatingList(element);
	
	if (verbose)
		cout << "Updated rating (id = " << paramId << ", new ttrsv = " << paramTtrsv << ")." << endl << endl;
}

void printAccountList()
{
	cout << "Account-List:" << endl << "=============================================================================" << endl << endl;
	
	cout << "Number of registered accounts: " << numberOfRegisteredAccounts << endl;
	cout << "Id of last registered account: " << idOfLastRegisteredAccount << endl << endl;
	
	CAccountListElement * element = accountListStart->nextElement;
	
	while(element->id != -1)
	{
		element->account->printDetails();
		
		element = element->nextElement;
	}
}

void printRatingList()
{
	int rank = 1;
	
	CRatingListElement * element = ratingListStart->nextElement;
	
	cout << "Rating-List" << endl << "=============================================================================" << endl;
	
	while(element->rank != -1)
	{
		printf("%5.0i) %4.0f ", rank++,  element->account->getTtrsv());
		cout << element->account->getFirstName();
		
		if (element->account->getSecondName() != "")
			cout << " aka " << element->account->getSecondName();
		if (element->account->getThirdName() != "")
			cout << " aka " << element->account->getThirdName();
		cout << endl;
		
		element = element->nextElement;
	}
	
	cout << endl;
}

int loadFromFile(const char * paramPathToFileToLoadFrom)
{
	string line[9];
	
	int id;
	float ttrsv;
	int nrOfEvaluatedTtrsGames;
	bool privateNrOfEvaluatedTtrsGames;
	
	ifstream file;
	file.open(paramPathToFileToLoadFrom, ios::in);
	
	if (!file.is_open())
	{
		cout << "ERROR: The file to read could not be opened!" << endl;
		
		return 1;
	}
	
	while (file.good())
	{
		for (int i = 0; i < 9; i++)
		{
			getline(file,line[i]);
		}
		
		if (line[0] == "")
			break;
		
		stringstream(line[0]) >> id;
		stringstream(line[4]) >> ttrsv;
		stringstream(line[5]) >> nrOfEvaluatedTtrsGames;
		privateNrOfEvaluatedTtrsGames = (line[6].compare("true") == 0);
		
		CAccount * account = addAccount(id, line[1], line[2], line[3], privateNrOfEvaluatedTtrsGames, line[7], line[8]);
		
		account->setNrOfEvaluatedTtrsGames(nrOfEvaluatedTtrsGames);
		
		updateRating(id, ttrsv);
	}
	
	file.close();
	
	cout << "Loaded data from file " << paramPathToFileToLoadFrom << "." << endl << endl;
	
	return 0;
}

int saveToFile(const char * paramPathToFileToSaveTo)
{
	ofstream file;
	file.open(paramPathToFileToSaveTo, ios::out | ios::trunc);
	
	CAccountListElement * element = accountListStart->nextElement;
	
	if (!file.is_open())
	{
		cout << "ERROR: The file to write could not be opened!" << endl;
		
		return 1;
	}
	
	while (element->id != -1)
	{
		file << element->account->getId() << endl;
		file << element->account->getFirstName() << endl;
		file << element->account->getSecondName() << endl;
		file << element->account->getThirdName() << endl;
		file << element->account->getTtrsv() << endl;
		file << element->account->getNrOfEvaluatedTtrsGames() << endl;
		file << (element->account->getPrivateNrOfEvaluatedTtrsGames() ? "true" : "false") << endl;
		file << element->account->getDescription() << endl;
		file << element->account->password << endl;
		
		element = element->nextElement;
	}
	
	file.close();
	
	cout << "Written data to file " << paramPathToFileToSaveTo << "." << endl << endl;
	
	return 0;
}
