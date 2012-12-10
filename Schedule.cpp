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

#include "Schedule.h"

using namespace std;

CSchedule schedule;

void CSchedule::addTask(int paramDelay, int paramSocketFd, int paramType, int paramId, CModificationInformation * paramModificationInformation,
			string paramString1, string paramString2, string paramString3)
{
	CTask * task = firstTask;
	
	CTask * tempTask;
	
	while ((task != 0) && (task->timeWhenToApply < time(0) + paramDelay))
	{
		task = task->nextTask;
	}
	
	tempTask = task;
	
	task = new CTask(time(0) + paramDelay, paramSocketFd, paramType, paramId, paramModificationInformation,
				paramString1,  paramString2,  paramString3, tempTask);
}

void CSchedule::removeTask(CTask * paramTask)
{
	CTask * task = firstTask;
	
	if (task == paramTask)
	{
		task = paramTask->nextTask;
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
	switch(paramTask->type)
	{
		case 0:
			break;
			
		case 1:
			break;
			
		case 2:
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

struct timeval * CSchedule::timeToNextTask()
{
	struct timeval * result;
	
	if (firstTask != 0)
	{
		result->tv_sec = firstTask->timeWhenToApply - time(0) + 1;
		result->tv_usec = 0;
		
		return result;
	}
	else
	{
		return 0;
	}
}

CTask::CTask(time_t paramTimeWhenToApply, int paramSocketFd, int paramType, int paramId, CModificationInformation * paramModificationInformation,
	string paramString1, string paramString2, string paramString3, CTask * paramNextTask)
{													// Trivial constructor
	timeWhenToApply = paramTimeWhenToApply;
	socketFd = paramSocketFd;
	type = paramType;
	id = paramId;
	modificationInformation = paramModificationInformation;
	string1 = paramString1;
	string2 = paramString2;
	string3 = paramString3;
	
	nextTask = paramNextTask;
}

CModificationInformation::CModificationInformation(int paramSocketFd, int paramId, int paramNrOfExpectedDescriptionLines, string paramPasswordAttempt,
	bool paramPasswordCanStillBeCorrect, string paramFirstName, string paramSecondName, string paramThirdName,
	bool paramPrivateNrOfEvluatedTtrsGames)
{													// Trivial constructor
	socketFd = paramSocketFd;
	
	id = paramId;
	
	nrOfExpectedDescriptionLines = paramNrOfExpectedDescriptionLines;
	nrOfReceivedDescriptionLines = 0;
	
	passwordAttempt = paramPasswordAttempt;
	
	passwordCanStillBeCorrect = paramPasswordCanStillBeCorrect;
	
	firstName = paramFirstName;
	secondName = paramSecondName;
	thirdName = paramThirdName;
	
	description = "";
	
	privateNrOfEvaluatedTtrsGames = paramPrivateNrOfEvluatedTtrsGames;
}
