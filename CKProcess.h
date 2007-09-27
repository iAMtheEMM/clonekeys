//
//  CKProcess.h
//  CloneKeys
//
//  Created by Robert Jenkins on 9/16/07.
//  Copyright (C) 2007 Robert Jenkins <rjenkins8142@gmail.com>
//
//  This file is part of CloneKeys.
//
//  CloneKeys is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version.
//
//  CloneKeys is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//  The License should be in the file COPYING distributed with this program.


#include <Carbon/Carbon.h>
#include <iostream>
#include <list>
using namespace std;

/***
typedef struct {
	ProcessSerialNumber		psn;
	AXUIElementRef			application;
	CFStringRef				name;
	CFStringRef				psn_string;
	CFStringRef				status;
} ProcessStruct;

ProcessStruct CloneKeyProcessList[50];

**/

class CKProcess
{
	public:
		ProcessSerialNumber		psn;
		AXUIElementRef			application;
		CFStringRef				name;
		CFStringRef				psn_string;
		CFStringRef				status;
		Boolean					currentProcess;

		CKProcess();
		CKProcess(const CKProcess &);
		~CKProcess();
		CKProcess &operator=(const CKProcess &rhs);
		int operator==(const CKProcess &rhs) const;
		int operator==(const CFStringRef &name) const;
		int operator==(const ProcessSerialNumber &name) const;
		int operator<(const CKProcess &rhs) const;
};

CKProcess::CKProcess()
{
	psn.highLongOfPSN = 0;
	psn.lowLongOfPSN  = 0;
	application		  = NULL;
	name			  = NULL;
	psn_string		  = NULL;
	status			  = NULL;
	currentProcess	  = false;
}

CKProcess::CKProcess(const CKProcess &copyin)
{                             
	psn				= copyin.psn;
	application		= copyin.application;
	name			= copyin.name;
	psn_string		= copyin.psn_string;
	status			= copyin.status;
	currentProcess	= copyin.currentProcess;
}

CKProcess::~CKProcess()
{
	//CFRelease(name);
	//CFRelease(psn_string);
	//CFRelease(status);
}

CKProcess& CKProcess::operator=(const CKProcess &rhs)
{
	this->psn			 = rhs.psn;
	this->application	 = rhs.application;
	this->name			 = rhs.name;
	this->psn_string	 = rhs.psn_string;
	this->status		 = rhs.status;
	this->currentProcess = rhs.currentProcess;
	return *this;
}

int CKProcess::operator==(const CKProcess &rhs) const
{
	Boolean sameProcess;
	
	SameProcess(&(this->psn), &(rhs.psn), &sameProcess);
	return sameProcess;
}

int CKProcess::operator==(const ProcessSerialNumber &thepsn) const
{
	Boolean sameProcess;
	
	SameProcess(&(this->psn), &(thepsn), &sameProcess);
	return sameProcess;
}

int CKProcess::operator==(const CFStringRef &name) const
{
	if (CFStringCompare( this->name, name, kCFCompareBackwards) == kCFCompareEqualTo)
	{
		return 1;
	}
	return 0;
}

// This function is required for built-in STL list functions like sort
int CKProcess::operator<(const CKProcess &rhs) const
{
	if (CFStringCompare( this->name, rhs.name, kCFCompareBackwards) == kCFCompareLessThan)
	{
		return 1;
	}
	return 0;
}
