//
//  Preferences.h
//  CloneKeys
//
//  Created by Robert Jenkins on 9/9/07.
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


#define kPreferencesKey CFSTR("com.rjenkins.clonekeys")

enum
{
	kPrefMoveUp22		= 'prf1',
	kPrefMoveUp0		= 'prf2',
	kPrefAutoAdd		= 'prf3',
	kPrefAutoAddName	= 'prf4',
	kPrefOverrideHotkey	= 'prf5'
};
static const ControlID kMoveUp22		= { 'PREF', 1};
static const ControlID kMoveUp0			= { 'PREF', 2};
static const ControlID kAutoAdd			= { 'PREF', 3};
static const ControlID kAutoAddName		= { 'PREF', 4};
static const ControlID kOverrideHotkey	= { 'PREF', 5};
static const ControlID kOverrideField	= { 'PREF', 6};

static Boolean		prefMoveUp22;
static Boolean		prefMoveUp0;
static Boolean		prefAutoAdd;
static CFStringRef	prefAutoAddName;
static UInt32		prefOverrideModifiers;
static UInt32		prefOverrideKey;

static WindowRef	grab_new_hotkey;
static UInt32		newOverrideModifiers;
static UInt32		newOverrideKey;

// Our Preferences window class
class Preferences : public TWindow
{
    public:
                            Preferences() : TWindow( CFSTR("Preferences") ) {}
        virtual             ~Preferences() {}
        
        static void         Create();
        
    protected:
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
		virtual OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent& inEvent);
};


