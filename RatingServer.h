/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

#ifndef RATINGSERVER_H
#define RATINGSERVER_H

#include <string>

using namespace std;

class CAccount
{
private:
	int id;							// Primary key in AccountData.trs file
	
	string firstName, secondName, thirdName;		// The first name is supposed to be shown in rating lists; the rest is aliases
	
	float ttrsv;						// Tirili's Team Rating System ( =: ttrs ) Value
	
	int nrOfEvaluatedTtrsGames;				// Number of evaluated ttrs games
	
	bool privateNrOfEvaluatedTtrsGames;			// Wheter to show "> 20" instead of the real number of games in public (if > 20)
	
	string description;					// E-mail address, website, player info and other voluntary stuff goes here
	
	string password;					// Account password
	
public:
	CAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName,	// Trivial constructor
		 bool paramPrivateNrOfEvaluatedTtrsGames, string paramDescription, string paramPassword);
	
	~CAccount();
	
	int getId();
	
	string getFirstName();
	
	string getSecondName();
	
	string getThirdName();
	
	float getTtrsv();
	
	int getNrOfEvaluatedTtrsGames();
	
	int getPublicNrOfEvaluatedTtrsGames();
	
	void setNrOfEvaluatedTtrsGames(int paramNrOfEvaluatedTtrsGames);
	
	bool getPrivateNrOfEvaluatedTtrsGames();
	
	void setPrivateNrOfEvaluatedTtrsGames(bool paramPrivateNrOfEvaluatedTtrsGames);
	
	string getDescription();
	
	void incrementNrOfEvaluatedTtrsGames();
	
	bool passwordMatches(string paramPassword);
	
	void printDetails();					// Show account details
	
	friend void updateRating(int paramId, float paramTtrsv, bool verbose);
	
	friend int saveToFile(const char * paramPathToFileToSaveTo);
};

class CAccountListElement	// Elements of the account list (will be sorted by account-id)
{
public:
	int id;
	
	CAccount * account;
	
	CAccountListElement * prevElement;
	CAccountListElement * nextElement;
	
	CAccountListElement();			// Trivial constructor
};

class CRatingListElement	// Elements of the sorted linked list of (playername,rating-value)-pairs (will be sorted by primary rating value)
{
public:
	int rank;
	
	float ttrsv;
	
	CAccount * account;
	
	CRatingListElement * prevElement;
	CRatingListElement * nextElement;
	
	CRatingListElement();			// Trivial constructor
};

void setupAccountList();
void setupRatingList();
void insertIntoAccountList(CAccountListElement accountListElement);
void insertIntoRatingList(CRatingListElement ratingListElement);
CAccountListElement * extractAccountListElement(int paramId);
CRatingListElement * extractRatingListElement(int paramId);
CAccount * addAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName,
		      bool paramPrivateNrOfEvaluatedTtrsGames, string paramDescription, string paramPassword, bool verbose = false);
void removeAccount(int paramID);
int getIdFromName(string paramName);
CAccount * getAccountFromId(int paramId);
CAccount * getAccountFromName(string paramName);
void updateRating(int paramId, float paramTtrsv, bool verbose = false);
void printAccountList();
void printRatingList();
int loadFromFile(const char * paramPathToFileToLoadFrom);
int saveToFile(const char * paramPathToFileToSaveTo);

extern int numberOfRegisteredAccounts;
extern int idOfLastRegisteredAccount;

extern CAccountListElement * accountListStart;
extern CAccountListElement * accountListEnd;

extern CRatingListElement * ratingListStart;
extern CRatingListElement * ratingListEnd;

#endif
