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
	
	setupAccountList();
	setupRatingList();
	
	loadFromFile(argv[1]);	// Load data from file AccountData.trs
	
/*	
	addAccount(1, "Nocturne", "Tirili", "BitchBot", "Some cool dude! Visit http://content.open-ra.org !", false);
	addAccount(2, "Boris", "", "", "yo I'm da Boris!", false);
	addAccount(4, "JoSI", "mayhem", "", "deluxe", false);
	
	printRatingList();
	
	updateRating(2, 1293.47);
	updateRating(1, 874.232);

	addAccount(5, "Pirc", "", "jatso", "_#*#_", false);
*/	
	printRatingList();
	printAccountList();
	
	saveToFile(argv[2]);
	
	return 0;
}

class CAccount
{
private:
	int id;							// Primary key in AccountData.trs file
	
	string firstName, secondName, thirdName;		// The first name is supposed to be shown in rating lists; the rest is aliases
	
	float ttrsv;						// Tirili's Team Rating System Value
	
	string description;					// E-mail address, website, player info and other voluntary stuff goes here
	
public:
	CAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName, string paramDescription)	// Trivial constructor
	{
		id = paramId;
		
		numberOfRegisteredAccounts++;
		
		if (paramId > idOfLastRegisteredAccount)
		{
			idOfLastRegisteredAccount = paramId;
		}
		
		firstName = paramFirstName; secondName = paramSecondName; thirdName = paramThirdName;
		
		description = paramDescription;
		
		ttrsv = 1000;					// Let 1000 be the future average rating value of all players
	}
	
	~CAccount()
	{
		numberOfRegisteredAccounts--;
	}
	
	int getId()
	{
		return id;
	}
	
	string getFirstName()
	{
		return firstName;
	}
	
	string getSecondName()
	{
		return secondName;
	}
	
	string getThirdName()
	{
		return thirdName;
	}
	
	float getTtrsv()
	{
		return ttrsv;
	}
	
	string getDescription()
	{
		return description;
	}
	
	void printDetails()					// Show account details
	{
		cout << "ID: " << id << endl;
		cout << "First Name: " << firstName << "    Second Name: " << secondName << "    Third Name: " << thirdName << endl;
		cout << "TTRSv: " << ttrsv << endl;
		cout << "Description: " << description << endl << endl;
	}
	
	friend void updateRating(int paramId, float paramTtrsv, bool verbose);
};

class CAccountListElement	// Elements of the account list (will be sorted by account-id)
{
public:
	int id;
	
	CAccount * account;
	
	CAccountListElement * prevElement;
	CAccountListElement * nextElement;
	
	CAccountListElement() { id = 0; account = 0; prevElement = 0; nextElement = 0; }	// Trivial constructor
};

class CRatingListElement	// Elements of the sorted linked list of (playername,rating-value)-pairs (will be sorted by primary rating value)
{
public:
	int rank;
	
	float ttrsv;
	
	CAccount * account;
	
	CRatingListElement * prevElement;
	CRatingListElement * nextElement;
	
	CRatingListElement() { rank = 0; ttrsv = -1000000; account = 0; prevElement = 0; nextElement  = 0;}	// Trivial constructor
};

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

CAccount * addAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName, string paramDescription, bool verbose)
{
	if ((getIdFromName(paramFirstName) > 0) || (getIdFromName(paramSecondName) > 0) || (getIdFromName(paramThirdName) > 0))
	{
		cout << "An account with one of the specified names already exists. Creation canceled!" << endl;
		
		return 0;
	}	
	
	CAccount * account = new CAccount(paramId, paramFirstName, paramSecondName, paramThirdName, paramDescription);
	
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
	CAccountListElement * element = accountListStart;
	
	while((element->id == -2) || ((element->account->getFirstName() != paramName)
					&& (element->account->getSecondName() != paramName)
					&& (element->account->getThirdName() != paramName)))
	{
		element = element->nextElement;
	}
	
	return (element->id);
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
	string line[6];
	int id;
	float ttrsv;
	
	ifstream file;
	file.open(paramPathToFileToLoadFrom, ios::in);
	
	if (!file.is_open())
	{
		cout << "ERROR: The file to read could not be opened!" << endl;
		
		return 1;
	}
	
	while (file.good())
	{
		for (int i = 0; i < 6; i++)
		{
			getline(file,line[i]);
		}
		
		if (line[0] == "")
			break;
		
		stringstream(line[0]) >> id;
		stringstream(line[4]) >> ttrsv;
		
		addAccount(id, line[1], line[2], line[3], line[5]);
		
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
		file << element->account->getDescription() << endl;
		
		element = element->nextElement;
	}
	
	file.close();
	
	cout << "Written data to file " << paramPathToFileToSaveTo << "." << endl << endl;
	
	return 0;
}