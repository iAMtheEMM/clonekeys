//
//  CloneKeys.h
//  CloneKeys
//
//  Clones Keystrokes to multiple applications at once.
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


#include <Carbon/Carbon.h>
#include <AvailabilityMacros.h>
#include <CoreFoundation/CoreFoundation.h>
#include <ApplicationServices/ApplicationServices.h>
#include "TApplication.h"
#include "TURLTextView.h"
#include "TWindow.h"
#include "AboutBox.h"
#include "Preferences.h"
#include "CKProcess.h"

#define kToolbarAddButton		CFSTR("com.rjenkins.clonekeys.add")
#define kToolbarRemoveButton	CFSTR("com.rjenkins.clonekeys.remove")
#define kToolbarOverrideButton	CFSTR("com.rjenkins.clonekeys.override")

#define kWindowUnifiedTitleAndToolbarAttribute 1 << 7

enum
{
	kCmdAddWindow		= 'awin',
	kCmdRemoveWindow	= 'rwin',
	kCmdOverride		= 'over',
	kCmdDataPanel		= 'wins'
};

enum
{
	kNameColumn			= 'pnam',
	kPSNColumn			= 'ppsd',
	kStatusColumn		= 'pstt'
};

enum
{
	/* modifier keys */
	kCKCapsLockKey = 0x039,
	kCKShiftKey = 0x038,
	kCKControlKey = 0x03B,
	kCKOptionKey = 0x03A,
	kCKCommandKey = 0x037
};

list<CKProcess>CloneKeysProcessList;

static const UInt32 Modifiers[5] = { shiftKey, cmdKey, optionKey, controlKey, alphaLock };
static const UInt32 CKModifiers[5] = { kCKShiftKey, kCKCommandKey, kCKOptionKey, kCKControlKey, kCKCapsLockKey };
static const CFStringRef ModifierNames[5] = { CFSTR("Shift"), CFSTR("Cmd"), CFSTR("Option"), CFSTR("Ctrl"), CFSTR("CAPS") };

static const ControlID kDataPanel = { 'DATA', 1 };
static const HICommandExtended OverrideCmd = { 0, kCmdOverride };

static HIToolbarRef		mainToolbar;
static ControlRef		DataBrowser;
static UInt32			currentModifiers = 0;

bool add_window_flag = false;
bool mouseBroadcast = false;
bool keyboardBroadcast = true;

void DeleteFromProcessList(ProcessSerialNumber *psn);

// Our custom application class
class CarbonApp : public TApplication
{
    public:
							
                            CarbonApp() {}
        virtual             ~CarbonApp() {}
        
    protected:
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
};

// Our main window class
class MainWindow : public TWindow
{
    public:
                            MainWindow() : TWindow( CFSTR("MainWindow") ) {}
        virtual             ~MainWindow() {}
        
        static void         Create();
		
		HIToolbarItemRef	overrideToolbarItem;
		static OSStatus		ToolbarDelegateHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData );
		
		static OSStatus	CloneKeysGetSetItemData(ControlRef browser, DataBrowserItemID itemID, DataBrowserPropertyID property, 
									DataBrowserItemDataRef itemData, Boolean changeValue);

		static Boolean	CloneKeysItemComparison(ControlRef browser, DataBrowserItemID itemOneID, 
									DataBrowserItemID itemTwoID, DataBrowserPropertyID sortProperty);

		static void		CloneKeysItemNotification(ControlRef browser, DataBrowserItemID itemID, 
									DataBrowserItemNotification message);
		
		static Boolean	CloneKeysAcceptDrag(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID);

		static Boolean	CloneKeysReceiveDrag(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID);

		static Boolean	CloneKeysAddDragItem(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID,
									ItemReference* itemRef);

		static void		CloneKeysItemHelpContent(ControlRef browser, DataBrowserItemID item, 
									DataBrowserPropertyID property, HMContentRequest inRequest, 
									HMContentProvidedType *outContentProvided, HMHelpContentPtr ioHelpContent);

		static void		CloneKeysGetContextualMenu(ControlRef browser, MenuRef* contextualMenu, UInt32 *helpType, 
									CFStringRef* helpItemString, AEDesc *selection);

		static void		CloneKeysSelectContextualMenu(ControlRef browser, MenuRef contextualMenu, 
									UInt32 selectionType, SInt16 menuID, MenuItemIndex menuItem);
									        
		virtual Boolean     HandleCommand( const HICommandExtended& inCommand );
    protected:

		virtual OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent& inEvent);
		
		void				GetToolbarDefaultItems( CFMutableArrayRef array );
		void				GetToolbarAllowedItems( CFMutableArrayRef array );
		HIToolbarItemRef	CreateToolbarItemForIdentifier( CFStringRef identifier, CFTypeRef configData );
		void				RegisterIcon( FourCharCode inCreator, FourCharCode inType, const char* inName );
		
		void				InstallDataBrowserCallbacks(ControlRef browser);
};

