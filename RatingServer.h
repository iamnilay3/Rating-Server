/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

using namespace std;

class CAccount;
class CAccountListElement;
class CRatingListElement;

void setupAccountList();
void setupRatingList();
void insertIntoAccountList(CAccountListElement accountListElement);
void insertIntoRatingList(CRatingListElement ratingListElement);
CAccountListElement * extractAccountListElement(int paramId);
CRatingListElement * extractRatingListElement(int paramId);
CAccount * addAccount(int paramId, string paramFirstName, string paramSecondName, string paramThirdName, string paramDescription,
		      bool verbose = false);
void removeAccount(int paramID);
int getIdFromName(string paramName);
void updateRating(int paramId, float paramTtrsv, bool verbose = false);
void printAccountList();
void printRatingList();
int loadFromFile(const char * paramPathToFileToLoadFrom);
int saveToFile(const char * paramPathToFileToSaveTo);
