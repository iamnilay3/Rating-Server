/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <stdio.h>
#include <string> 

using namespace std;

class CTask;
class CModificationInformation;

class CSchedule
{
public:
	CTask * firstTask;
	
	void addTask(int paramDelay, int paramSocketFd, bool paramCorrectPassword, int paramType, int paramId,
			CModificationInformation * paramModificationInformation, string paramString1);
	void removeTask(CTask * paramTask);
	void executeTask(CTask * paramTask);
	void checkForTasksToExecute();
	
	timeval * timeToNextTask(timeval * paramTimeToNextTask);
};

class CTask
{
public:
	time_t timeWhenToApply;
	
	int socketFd;
	
	bool correctPassword;
	
	int type;
	
	int id;
	
	CModificationInformation * modificationInformation;
	
	string string1;
	
	CTask * nextTask;
	
	CTask(time_t paramTimeWhenToApply, int paramSocketFd, bool paramCorrectPassword, int paramType, int paramId,
		CModificationInformation * paramModificationInformation, string paramString1, CTask * paramNextTask);	// Trivial constructor
};

class CModificationInformation
{
public:
	int socketFd;
	
	int id;
	
	int nrOfExpectedDescriptionLines;
	int nrOfReceivedDescriptionLines;
	
	string firstName;
	string secondName;
	string thirdName;
	
	string description;
	
	bool privateNrOfEvaluatedTtrsGames;
	
	CModificationInformation(int paramSocketFd, int paramId, int paramNrOfExpectedDescriptionLines,
		string paramFirstName, string paramSecondName, string paramThirdName,
		bool paramPrivateNrOfEvluatedTtrsGames);							// Trivial constructor
};

extern CSchedule schedule;

#endif