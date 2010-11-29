/*
 *  BlackList.h
 *  CloneKeys
 *
 *  Created by Kirk Allen on 11/15/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */


enum 
{	
	kBlackButtonEnterKey = 'blk1',
	kBlackTextKey = 'blk2',
	kBlackButtonAdd = 'blk3',
	kBlackData = 'blk4'
};


static const ControlID kButtonEnterKey =	{'BLAK',1};
static const ControlID kTextKey	=			{'BLAK',2};
static const ControlID kButtonAdd =			{'BLAK',3};
static const ControlID kDataKeys =			{'BLAK',4};

class BlackList : public TWindow
{
public:
	BlackList() : TWindow( CFSTR("BlackList") ) {}
	virtual ~BlackList() {}
	
	static void Create();
	
protected:
	virtual Boolean HandleCommand(const HICommandExtended& inCommand);
	virtual OSStatus HandleEvent(EventHandlerRef inRef, TCarbonEvent& inEvent);
};