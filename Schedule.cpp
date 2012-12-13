/*
 * Copyright 2012 "TiriliPiitPiit" ( tirili_@fastmail.fm , http://github.com/TiriliPiitPiit )
 * 
 * This file is part of Tirili's Rating Server, which is free software. It is made
 * available to you under the terms of the GNU General Public License
 * as published by the Free Software Foundation. For more information,
 * see COPYING.
 */

// This file contains the schedule management functions for tasks which are delayed to
// improve safety regarding brute force attacks.

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "RatingServer.h"
#include "Network.h"
#include "Games.h"
#include "Schedule.h"

using namespace std;

CSchedule schedule;

void CSchedule::addTask(int paramDelay, int paramSocketFd, bool paramCorrectPassword, int paramType, int paramId,
			CModificationInformation * paramModificationInformation, string paramString1)
{
	time_t executionTime = time(0) + paramDelay;
	
	CTask * tempTask = 0;
	
	CTask * tempNextTask = firstTask;
	
	while ((tempNextTask != 0) && (tempNextTask->timeWhenToApply < executionTime))
	{
		tempTask = tempNextTask;
		tempNextTask = tempNextTask->nextTask;
	}
	
	if (tempTask == 0)
	{
		firstTask = new CTask(executionTime, paramSocketFd, paramCorrectPassword, paramType, paramId, paramModificationInformation,
					paramString1, tempNextTask);
	}
	else
	{
		tempTask->nextTask = new CTask(executionTime, paramSocketFd, paramCorrectPassword, paramType, paramId, paramModificationInformation,
						paramString1, tempNextTask);
	}
}

void CSchedule::removeTask(CTask * paramTask)
{
	CTask * task = firstTask;
	
	if (firstTask == paramTask)
	{
		firstTask = firstTask->nextTask;
	}
	else
	{
		while (task->nextTask !=  paramTask)
		{
			task = task->nextTask;
		}
		
		task->nextTask = paramTask->nextTask;
	}
	
	delete paramTask;
}

void CSchedule::executeTask(CTask * paramTask)
{
	string sendString;
	char sendBuffer[1000];
	
	CModificationInformation * modificationInformation;
	CAccount * account;
	
	int i;
	
	switch(paramTask->type)
	{
		case 1:
			modificationInformation = paramTask->modificationInformation;
			
			if (!paramTask->correctPassword)
			{
				sendString = "02524fThe password is wrong.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 28);
				
				break;
			}
			
			if (modificationInformation->nrOfExpectedDescriptionLines != modificationInformation->nrOfReceivedDescriptionLines)
			{
				sendString = "05024fCould not receive all lines of the description.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 53);
				
				break;
			}
			
			account = getAccountFromId(paramTask->id);
			
			if (((i = getIdFromName(modificationInformation->firstName)) > 0) && (i != modificationInformation->id))
			{
				sendString = "04924fAn account with the first Name already exists.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 52);
				
				break;
			}
			
			if (((i = getIdFromName(modificationInformation->secondName)) > 0) && (i != modificationInformation->id))
			{
				sendString = "05024fAn account with the second Name already exists.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 53);
				
				break;
			}
			
			if (((i = getIdFromName(modificationInformation->thirdName)) > 0) && (i != modificationInformation->id))
			{
				sendString = "04924fAn account with the third Name already exists.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 52);
				
				break;
			}
			
			account->setFirstName(modificationInformation->firstName);
			account->setSecondName(modificationInformation->secondName);
			account->setThirdName(modificationInformation->thirdName);
			account->setPrivateNrOfEvaluatedTtrsGames(modificationInformation->privateNrOfEvaluatedTtrsGames);
			account->setDescription(modificationInformation->description);
			
			sendString = "00324s";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramTask->socketFd, sendBuffer, 6);
			
			// Protocol Send-Syntax:	24:{s,f}ErrorMessage
			
			cout << "Modified account:" << endl;
			account->printDetails();
			
			break;
			
		case 2:
			if (!paramTask->correctPassword)
			{
				sendString = "02529fThe password is wrong.";
				strcpy(sendBuffer, sendString.c_str());
				sendCommand(paramTask->socketFd, sendBuffer, 28);
				
				break;
			}
			
			account = getAccountFromId(paramTask->id);
			
			account->setPassword(paramTask->string1);
			
			sendString = "00329s";
			strcpy(sendBuffer, sendString.c_str());
			sendCommand(paramTask->socketFd, sendBuffer, 6);
			
			// Protocol Send-Syntax:	29:{s,f}ErrorMessage
			
			cout << "Changed password of account #" << paramTask->id << "." << endl << endl;
			
			break;
			
		case 3:
			break;
			
		default:
			break;
	}
}

void CSchedule::checkForTasksToExecute()
{
	time_t currentTime = time(0);
	
	while ((firstTask != 0) && (firstTask->timeWhenToApply <= currentTime))
	{
		executeTask(firstTask);
		removeTask(firstTask);
	}
}

timeval * CSchedule::timeToNextTask(timeval * paramTimeToNextTask)
{
	if (firstTask != 0)
	{
		paramTimeToNextTask->tv_sec = firstTask->timeWhenToApply - time(0) + 1;
		paramTimeToNextTask->tv_usec = 0;
		
		return paramTimeToNextTask;
	}
	else
	{
		return 0;
	}
}

CTask::CTask(time_t paramTimeWhenToApply, int paramSocketFd, bool paramCorrectPassword, int paramType, int paramId,
		CModificationInformation * paramModificationInformation, string paramString1, CTask * paramNextTask)
{													// Trivial constructor
	timeWhenToApply = paramTimeWhenToApply;
	socketFd = paramSocketFd;
	correctPassword = paramCorrectPassword;
	type = paramType;
	id = paramId;
	modificationInformation = paramModificationInformation;
	string1 = paramString1;
	
	nextTask = paramNextTask;
}

CModificationInformation::CModificationInformation(int paramSocketFd, int paramId, int paramNrOfExpectedDescriptionLines,
							string paramFirstName, string paramSecondName, string paramThirdName,
							bool paramPrivateNrOfEvluatedTtrsGames)
{													// Trivial constructor
	socketFd = paramSocketFd;
	
	id = paramId;
	
	nrOfExpectedDescriptionLines = paramNrOfExpectedDescriptionLines;
	nrOfReceivedDescriptionLines = 0;
	
	firstName = paramFirstName;
	secondName = paramSecondName;
	thirdName = paramThirdName;
	
	description = "";
	
	privateNrOfEvaluatedTtrsGames = paramPrivateNrOfEvluatedTtrsGames;
}
