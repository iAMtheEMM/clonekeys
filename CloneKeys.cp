//
//  CloneKeys.cpp
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


#include <syslog.h>
#include "CloneKeys.h"

MainWindow *main_wind;
void focusFirstWindowOfPid(pid_t pid);

//--------------------------------------------------------------------------------------------
void log_err(AXError err)
{
	if (err)
	{
		switch(err)
		{
			case kAXErrorIllegalArgument:
				syslog(LOG_NOTICE, "Error: [%s]", "kAXErrorIllegalArgument");
				break;
			case kAXErrorInvalidUIElement:
				syslog(LOG_NOTICE, "Error: [%s]", "kAXErrorInvalidUIElement");
				break;
			case kAXErrorFailure:
				syslog(LOG_NOTICE, "Error: [%s]", "kAXErrorFailure");
				break;
			case kAXErrorCannotComplete:
				syslog(LOG_NOTICE, "Error: [%s]", "kAXErrorCannotComplete");
				break;
			case kAXErrorNotImplemented:
				syslog(LOG_NOTICE, "Error: [%s]", "kAXErrorNotImplemented");
				break;
			default:
				syslog(LOG_NOTICE, "Error: [%s]", "UNKNOWN");
				break;
		} 
	}
}

//--------------------------------------------------------------------------------------------
void CreateOverrideString(UInt32 modifiers, UInt32 key, CFStringRef *theString)
{
	CFMutableStringRef	output;
	UInt32				cnt;

	output = CFStringCreateMutable(NULL, 0);
	cnt = 0;
	for(UInt32 k = 0; k < 5; k++)
	{
		// Check for keydowns
		if (modifiers & Modifiers[k])
		{
			if (cnt != 0)
				CFStringAppend(output, CFSTR("+"));
			CFStringAppend(output, ModifierNames[k]);
			cnt++;
		}
	}
	if (cnt != 0)
		CFStringAppend(output, CFSTR(" "));
	CFStringAppendFormat(output, NULL, CFSTR("key(%d)"), key);

	*theString = CFStringCreateCopy(NULL, output);
	CFRelease(output);
}

//--------------------------------------------------------------------------------------------
static OSStatus MonitorHandler( EventHandlerCallRef inRef, EventRef inEvent, void* inRefcon )
{
	OSStatus				result = eventNotHandledErr;
	UInt32					keyCode;
	char					charCode;
	ProcessSerialNumber		process_psn;
	CFStringRef				process_name;
	bool					keyDown;
	bool					keyPressed;
	bool					broadcasted;
	bool					autoAdd;
	list<CKProcess>::iterator index;

	process_name = NULL;
	keyPressed = false;
	broadcasted = false;
	autoAdd = false;

	switch (GetEventClass(inEvent))
	{
		case kEventClassKeyboard:
			switch(GetEventKind(inEvent))
			{
				case kEventRawKeyDown:
					GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
					//syslog(LOG_NOTICE, "Key Down [%c] [%d]", (char)charCode, (int)keyCode);
					keyPressed = true;
					keyDown = true;
					break;
				case kEventRawKeyUp:
					GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
					//syslog(LOG_NOTICE, "Key Up [%c] [%d]", (char)charCode, (int)keyCode);
					keyPressed = true;
					keyDown = false;
					break;
				case kEventRawKeyRepeat:
					GetEventParameter(inEvent, kEventParamKeyCode, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
					GetEventParameter(inEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(char), NULL, &charCode);
					//syslog(LOG_NOTICE, "Key Repeat [%c] [%d]", (char)charCode, (int)keyCode);
					// For key repeats, just sending another keyDown seems to work just fine.
					keyPressed = true;
					keyDown = true;
					break;
				case kEventRawKeyModifiersChanged:
					GetEventParameter(inEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(UInt32), NULL, &keyCode);
					//syslog(LOG_NOTICE, "Key Modifier [0x%x]", (int)keyCode);
					if (add_window_flag == false && keyboardBroadcast == true)
					{
						UInt32 DownKeys;
						UInt32 UpKeys;
						
						DownKeys = keyCode & ~currentModifiers;
						UpKeys = currentModifiers & ~keyCode;
						
						//syslog(LOG_NOTICE, "Key Downs [0x%x]", (int)DownKeys);
						//syslog(LOG_NOTICE, "Key Ups [0x%x]", (int)UpKeys);
						
						for(index = CloneKeysProcessList.begin(); index != CloneKeysProcessList.end(); ++index)
						{
							AXError err;
							// Don't clone to the current process, it'll double-up.
							if ((*index).currentProcess == false)
							{
								for(UInt32 k = 0; k < 5; k++)
								{
									// Check for keydowns
									if (DownKeys & Modifiers[k])
									{
										err = AXUIElementPostKeyboardEvent((*index).application, (CGCharCode)0, CKModifiers[k], true);
									}
									// Check for keyups
									if (UpKeys & Modifiers[k])
									{
										err = AXUIElementPostKeyboardEvent((*index).application, (CGCharCode)0, CKModifiers[k], false);
									}
								}
							}
						}
					}
					currentModifiers = keyCode;
					break;
				default:
					//syslog(LOG_NOTICE, "Unknown key event [%d]", GetEventKind(inEvent));
					break;
			}
			break;
			
		case kEventClassApplication:
			if (prefAutoAdd && GetEventKind(inEvent) == kEventAppLaunched)
			{
				UInt32			actualSize;
				EventParamType	actualType;
				
				GetEventParameter(inEvent, kEventParamProcessID,
					typeProcessSerialNumber, &actualType,
					sizeof(process_psn), &actualSize, &process_psn);
			
				CopyProcessName(&process_psn, &process_name);
				
				if (process_name != NULL && ( CFStringCompare( process_name, prefAutoAddName, kCFCompareBackwards) == kCFCompareEqualTo ))
				{
					autoAdd = true;
				}
			}
			if (autoAdd == true || (GetEventKind(inEvent) == kEventAppFrontSwitched && add_window_flag == true))
			{
				ProcessSerialNumber our_psn;
				Boolean sameProcess;
				OSStatus err;
				
				GetFrontProcess(&process_psn);
				GetCurrentProcess(&our_psn);
				
				err = SameProcess(&process_psn, &our_psn, &sameProcess); 
				if (!err && !sameProcess)
				{
					CopyProcessName(&process_psn, &process_name);
					if (process_name == NULL)
					{
						process_name = CFSTR("<Unknown>");
					}

					// See if we are already broadcasting to that process...
					index = find(CloneKeysProcessList.begin(), CloneKeysProcessList.end(), process_psn);
					if (autoAdd || index == CloneKeysProcessList.end())
					{
						pid_t process_pid;
						CKProcess newNode;
						DataBrowserItemID maxID;
						
						GetProcessPID(&process_psn, &process_pid);
					
						// Get rid of the old placeholder node and make a new complete node.
						if (add_window_flag)
						{
							index--;
							CloneKeysProcessList.erase(index);
						}
						else
						{
							maxID = (DataBrowserItemID)(CloneKeysProcessList.size()+1);
							AddDataBrowserItems(DataBrowser, kDataBrowserNoItem, 1, &maxID, kDataBrowserItemNoProperty);
						}

						newNode.psn = process_psn;
						newNode.application = AXUIElementCreateApplication(process_pid);
						newNode.name       = process_name;
						newNode.psn_string = CFStringCreateCopy(NULL, CFStringCreateWithFormat(NULL, NULL, CFSTR("%ld"), process_psn.lowLongOfPSN));
						newNode.status = CFSTR("broadcasting...");
						newNode.currentProcess = true;
				
						CloneKeysProcessList.push_back(newNode);
						
						UpdateDataBrowserItems(DataBrowser, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty, kDataBrowserNoItem);

						// If the main window is at x, 22, move it to x, 0
						if (prefMoveUp22 || prefMoveUp0)
						{
							AXUIElementRef focusWindow;
							AXValueRef value;
							CGPoint	xy;
							
							AXUIElementCopyAttributeValue(newNode.application, kAXFocusedWindowAttribute, (CFTypeRef *)&focusWindow);
							AXUIElementCopyAttributeValue(focusWindow, kAXPositionAttribute, (CFTypeRef *)&value);
							AXValueGetValue(value, kAXValueCGPointType, &xy);
							
							if ((prefMoveUp22 && xy.y == 22.0) || (prefMoveUp0 && xy.y == 0.0))
							{
								xy.y -= 22.0;
							}
							value = AXValueCreate(kAXValueCGPointType, &xy);
							AXUIElementSetAttributeValue(focusWindow, kAXPositionAttribute, (CFTypeRef)value);
						}

						// Pop back to the front after we aquire the target window.
						if (add_window_flag)
						{
							add_window_flag = false;
							SetFrontProcess(&our_psn);
						}
					}
				}
			}
			else if (GetEventKind(inEvent) == kEventAppFrontSwitched)
			{
				GetFrontProcess(&process_psn);
				
				for(index = CloneKeysProcessList.begin(); index != CloneKeysProcessList.end(); ++index)
				{
					Boolean sameProcess;
				
					SameProcess(&process_psn, &((*index).psn), &sameProcess);
				
					if (sameProcess == true)
					{
						(*index).currentProcess = true;
					}
					else
					{
						(*index).currentProcess = false;
					}
				}

				for(index = CloneKeysProcessList.begin(); index != CloneKeysProcessList.end(); ++index) {
					if (!(*index).currentProcess) {
				        // tell the first window of every background broacastee it has focus
						pid_t cur_pid;
						GetProcessPID(&((*index).psn), &cur_pid);
						focusFirstWindowOfPid(cur_pid);
					}
				}
			}
			if (GetEventKind(inEvent) == kEventAppTerminated)
			{
				UInt32			actualSize;
				EventParamType	actualType;
				
				GetEventParameter(inEvent, kEventParamProcessID,
					typeProcessSerialNumber, &actualType,
					sizeof(process_psn), &actualSize, &process_psn);
					
				index = find(CloneKeysProcessList.begin(), CloneKeysProcessList.end(), process_psn);
				if (index != CloneKeysProcessList.end())
				{
					DataBrowserItemID maxID;
					
					CloneKeysProcessList.erase(index);
					
					maxID = (DataBrowserItemID)CloneKeysProcessList.size();
					maxID++;
					RemoveDataBrowserItems(DataBrowser, kDataBrowserNoItem, 1, &maxID, kDataBrowserItemNoProperty);
					UpdateDataBrowserItems(DataBrowser, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty, kDataBrowserNoItem);
				}
			}
			break;
			
		default:
			break;
	}

	if (keyPressed && add_window_flag == false)
	{
		if (grab_new_hotkey == NULL && keyDown == true && prefOverrideModifiers == currentModifiers && prefOverrideKey == keyCode)
		{
			// Send keyboard broadcasting toggle event.
			main_wind->HandleCommand(OverrideCmd);
		}
		else if (keyboardBroadcast)
		{
			for(index = CloneKeysProcessList.begin(); index != CloneKeysProcessList.end(); ++index)
			{
				AXError err;

				// Don't clone to the current process, it'll double-up.
				if ((*index).currentProcess == false)
				{
					err = AXUIElementPostKeyboardEvent((*index).application, charCode, keyCode, keyDown);
					//err = AXUIElementPostKeyboardEvent((*index).application, charCode, keyCode, keyDown);
					broadcasted = true;
					//log_err(err);
				}
			}
		}
		if (grab_new_hotkey != NULL && keyDown == true)
		{
			ControlRef	control;
			HIViewRef rootRef;
			CFStringRef sOverride;
			
			newOverrideModifiers = currentModifiers;
			newOverrideKey = keyCode;
			
			rootRef = HIViewGetRoot(grab_new_hotkey);
			CreateOverrideString(newOverrideModifiers, newOverrideKey, &sOverride);
			HIViewFindByID(rootRef, kOverrideField, &control);
			SetControlData(control, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &sOverride);
			
			grab_new_hotkey = NULL;
		}
	}
	
	// Sleep for a split second, so we don't flood?
	/***
	if (broadcasted)
	{
		int serr;

		serr = usleep(2500);
		if (serr)
		{
			syslog(LOG_NOTICE, "usleep failed. [%d]", serr);
		}
	}
	**/
	//syslog(LOG_NOTICE, "raw monitor = [%d] [%d]", GetEventClass(inEvent), GetEventKind(inEvent)); 

	if (broadcasted)
		result = noErr;

	return result;
}

// focus on the first titled window of the specified PID
void focusFirstWindowOfPid(pid_t pid)
{
	AXUIElementRef appRef = AXUIElementCreateApplication(pid);
	
	CFArrayRef winRefs;
	AXUIElementCopyAttributeValues(appRef, kAXWindowsAttribute, 0, 255, &winRefs);
	if (!winRefs) return;
	
	for (int i = 0; i < CFArrayGetCount(winRefs); i++)
	{
		AXUIElementRef winRef = (AXUIElementRef)CFArrayGetValueAtIndex(winRefs, i);
		CFStringRef titleRef = NULL;
		AXUIElementCopyAttributeValue( winRef, kAXTitleAttribute, (const void**)&titleRef);
		
		char buf[1024];
		buf[0] = '\0';
		if (!titleRef)
		{
			strcpy(buf, "null");
		}
		if (!CFStringGetCString(titleRef, buf, 1023, kCFStringEncodingUTF8)) return;
		CFRelease(titleRef);
		
		if (strlen(buf) != 0)
		{
			AXError result = AXUIElementSetAttributeValue(winRef, kAXFocusedAttribute, kCFBooleanTrue);
			// CFRelease(winRef);
			// syslog(LOG_NOTICE, "result %d of setting window %s focus of pid %d", result, buf, pid);
			if (result != 0)
			{
				// syslog(LOG_NOTICE, "result %d of setting window %s focus of pid %d", result, buf, pid);
			}
			break;
		}
		else {
			// syslog(LOG_NOTICE, "Skipping setting window %s focus of pid %d", buf, pid);
		}
	}

	AXUIElementSetAttributeValue(appRef, kAXFocusedApplicationAttribute, kCFBooleanTrue);

	CFRelease(winRefs);
	CFRelease(appRef);
}

//--------------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	Boolean			test;
	HICommand		command = { 0, kHICommandNew };
    CarbonApp		app;
	EventTypeSpec   kEvents[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};
	EventTypeSpec	kAppEvents[] =
	{
		{ kEventClassApplication, kEventAppFrontSwitched },
		{ kEventClassApplication, kEventAppLaunched },
		{ kEventClassApplication, kEventAppTerminated }
	};
	
	if (AXAPIEnabled() == false)
	{
		AlertStdCFStringAlertParamRec	param;
		DialogRef						dialog;
		DialogItemIndex					itemHit;
	
		GetStandardAlertDefaultParams( &param, kStdCFStringAlertVersionOne );
	
		param.movable = true;
		
		CreateStandardAlert( kAlertNoteAlert, CFSTR( "WARNING: This application will not work properly if the accessibility API is not enabled. Users can enable the accessibility API by checking \"Enable access for assistive devices\" in Universal Access Preferences." ), NULL, &param, &dialog );
		
		SetWindowTitleWithCFString( GetDialogWindow( dialog ), CFSTR( "Assistive Devices Not Enabled" ) );

		RunStandardAlert( dialog, NULL, &itemHit );
	}
	
	EnableMenuCommand(NULL, kHICommandPreferences);
	
	// Grab the preferences...
	prefMoveUp22 = CFPreferencesGetAppBooleanValue( CFSTR( "MoveUp22" ), kPreferencesKey, &test );
	if ( !test ) prefMoveUp22 = false;

	prefMoveUp0 = CFPreferencesGetAppBooleanValue( CFSTR( "MoveUp0" ), kPreferencesKey, &test );
	if ( !test ) prefMoveUp0 = false;

	prefAutoAdd = CFPreferencesGetAppBooleanValue( CFSTR( "AutoAdd" ), kPreferencesKey, &test );
	if ( !test ) prefAutoAdd = false;

	prefAutoAddName = (CFStringRef)CFPreferencesCopyAppValue( CFSTR("AutoAddName"), kPreferencesKey);
	if (!prefAutoAddName)
	{
		prefAutoAddName = CFSTR("");
	}

	prefOverrideModifiers = CFPreferencesGetAppIntegerValue( CFSTR( "OverrideModifiers" ), kPreferencesKey, &test );
	if ( !test ) prefOverrideModifiers = 0;
	
	prefOverrideKey = CFPreferencesGetAppIntegerValue( CFSTR( "OverrideKey" ), kPreferencesKey, &test );
	if ( !test ) prefOverrideKey = 113;
		
	grab_new_hotkey = NULL;
	
	// Setup the main SystemEvent handler.
	InstallEventHandler( GetEventMonitorTarget(), MonitorHandler, GetEventTypeCount( kEvents ), kEvents, 0, NULL );
	InstallApplicationEventHandler( MonitorHandler, GetEventTypeCount(kAppEvents), kAppEvents, 0, NULL);
		
	// Create the main window.
	ProcessHICommand( &command );
		
    app.Run();
	
    return 0;
}

//--------------------------------------------------------------------------------------------
Boolean
CarbonApp::HandleCommand( const HICommandExtended& inCommand )
{
	Boolean result = false;
	
    switch ( inCommand.commandID )
    {
        case kHICommandNew:
            MainWindow::Create();
            result = true;
			break;
			
		case kHICommandAbout:
			AboutBox::Create();
			result = true;
			break;
			
        case kHICommandPreferences:
			Preferences::Create();
			result = true;
			break;
			
        default:
            return false;
    }
	
	return result;
}

//--------------------------------------------------------------------------------------------
void
MainWindow::Create()
{
    main_wind = new MainWindow();

    // Position new windows in a staggered arrangement on the main screen
    //RepositionWindow( *wind, NULL, kWindowCascadeOnMainScreen );
    
	// Toolbar events.
	EventTypeSpec kToolbarEvents[] =
	{
		{ kEventClassToolbar, kEventToolbarGetDefaultIdentifiers },
		{ kEventClassToolbar, kEventToolbarGetAllowedIdentifiers },
		{ kEventClassToolbar, kEventToolbarGetSelectableIdentifiers },
		{ kEventClassToolbar, kEventToolbarCreateItemWithIdentifier },
		{ kEventClassToolbar, kEventToolbarCreateItemFromDrag }
	};

	// Add some additional events so that we can pass them on to the "global" event handler.
	EventTypeSpec   kEvents[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};
	
	main_wind->RegisterForEvents( GetEventTypeCount( kEvents ), kEvents);
	
	// Find the DataBrowser Control
	GetControlByID( *main_wind, &kDataPanel, &DataBrowser );
	main_wind->InstallDataBrowserCallbacks(DataBrowser);
	
	//SetDataBrowserTarget(DataBrowser, 50);
	
	HIToolbarCreate(kPreferencesKey, kHIToolbarNoAttributes, &mainToolbar);
	
	InstallEventHandler( HIObjectGetEventTarget( mainToolbar ), NewEventHandlerUPP(ToolbarDelegateHandler),
				GetEventTypeCount( kToolbarEvents ), kToolbarEvents, main_wind, NULL );

	SetWindowToolbar( *main_wind, mainToolbar);
	ShowHideWindowToolbar( *main_wind, true, false);
	
	ChangeWindowAttributes( *main_wind, kWindowToolbarButtonAttribute | kWindowUnifiedTitleAndToolbarAttribute, 0 );	
	SetAutomaticControlDragTrackingEnabledForWindow(*main_wind, true);

	add_window_flag = false;
	
    // The window was created hidden, so show it
    main_wind->Show();
	
	SetKeyboardFocus(*main_wind, DataBrowser, kControlDataBrowserPart);
}

//--------------------------------------------------------------------------------------------
Boolean
MainWindow::HandleCommand( const HICommandExtended& inCommand )
{
	Boolean				result = false;
	IconRef				icon;
	
    switch ( inCommand.commandID )
    {
		case kCmdAddWindow:
			if (add_window_flag == false)
			{
				CKProcess newNode;
				
				add_window_flag = true;
				DataBrowserItemID maxID;
				//syslog(LOG_NOTICE, "ADD BUTTON [%d]", inCommand.commandID);
				
				newNode.name = CFSTR("Click target window");
				newNode.psn_string = CFStringCreateCopy(NULL, CFStringCreateWithFormat(NULL, NULL, CFSTR("%ld"), 0));
				newNode.status = CFSTR("WAITING...");
				newNode.currentProcess = true;
				
				CloneKeysProcessList.push_back(newNode);

				maxID = (DataBrowserItemID)CloneKeysProcessList.size();
				AddDataBrowserItems(DataBrowser, kDataBrowserNoItem, 1, &maxID, kDataBrowserItemNoProperty);
				UpdateDataBrowserItems(DataBrowser, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty, kDataBrowserNoItem);
			}
			result = true;
			break;
			
		case kCmdRemoveWindow:
			{
				Handle itemsHandle;
				DataBrowserItemID *items;
				DataBrowserItemID cnt;
				list<CKProcess>::iterator index;
				
				itemsHandle = NewHandle(0);
				//syslog(LOG_NOTICE, "REMOVE BUTTON [%d]", inCommand.commandID);
				GetDataBrowserItems(DataBrowser, kDataBrowserNoItem, false, kDataBrowserItemIsSelected, itemsHandle);
				
				HLockHi(itemsHandle);  // since we're about to dereference this, we need it to stay put
				items = (DataBrowserItemID *)*itemsHandle;

				// See if there was anything selected.
				if(GetHandleSize(itemsHandle))
				{
					for(index=CloneKeysProcessList.begin(), cnt = 1; index != CloneKeysProcessList.end(); ++index, ++cnt)
					{
						if (cnt == items[0])
						{
							DataBrowserItemID maxID;
					
							CloneKeysProcessList.erase(index);
					
							maxID = (DataBrowserItemID)CloneKeysProcessList.size();
							maxID++;
							RemoveDataBrowserItems(DataBrowser, kDataBrowserNoItem, 1, &maxID, kDataBrowserItemNoProperty);
							UpdateDataBrowserItems(DataBrowser, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty, kDataBrowserNoItem);
							break;
						}
					}
				}
				
				DisposeHandle(itemsHandle);
				result = true;
			}
			break;
			
		case kCmdOverride:
			if (keyboardBroadcast)
			{
				keyboardBroadcast = false;
				GetIconRef( kOnSystemDisk, 'CKCB', 'ovr2', &icon);
				HIToolbarItemSetLabel( this->overrideToolbarItem, CFSTR("Start"));
				HIToolbarItemSetIconRef( this->overrideToolbarItem, icon );
				ReleaseIconRef( icon );
			}
			else
			{
				keyboardBroadcast = true;
				GetIconRef( kOnSystemDisk, 'CKCB', 'ovr1', &icon);
				HIToolbarItemSetLabel( this->overrideToolbarItem, CFSTR("Stop"));
				HIToolbarItemSetIconRef( this->overrideToolbarItem, icon );
				ReleaseIconRef( icon );
			}
			//syslog(LOG_NOTICE, "OVERRIDE BUTTON [%d]", inCommand.commandID);
			result = true;
			break;
		
		case kCmdDataPanel:
			//syslog(LOG_NOTICE, "DATA PANEL [%d]", inCommand.commandID);
			SetKeyboardFocus(*this, DataBrowser, kControlDataBrowserPart);
			result = true;
			break;
		
		case kCmdBlacklist:
			BlackList::Create();
			break;
		default:
            result = false;
    }
	
	//syslog(LOG_NOTICE, "window command = [%d]", inCommand.commandID);
	
	return result;
}

//--------------------------------------------------------------------------------------------
OSStatus
MainWindow::ToolbarDelegateHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* inUserData )
{
	MainWindow *wind = (MainWindow *)inUserData;
	OSStatus				result = eventNotHandledErr;
	CFMutableArrayRef		array;
	CFStringRef				identifier;

	switch(GetEventKind(inEvent))
	{
		case kEventToolbarGetDefaultIdentifiers:
			result = noErr;
			//syslog(LOG_NOTICE, "kEventToolbarGetDefaultIdentifiers");
			GetEventParameter(inEvent, kEventParamMutableArray, typeCFMutableArrayRef, NULL, sizeof(CFMutableArrayRef), NULL, &array);
			wind->GetToolbarDefaultItems( array );
			break;
		case kEventToolbarGetAllowedIdentifiers:
			result = noErr;
			//syslog(LOG_NOTICE, "kEventToolbarGetAllowedIdentifiers");
			GetEventParameter(inEvent, kEventParamMutableArray, typeCFMutableArrayRef, NULL, sizeof(CFMutableArrayRef), NULL, &array);
			wind->GetToolbarAllowedItems( array );
			break;
		case kEventToolbarCreateItemWithIdentifier:
			{
				HIToolbarItemRef		item;
				CFTypeRef				data = NULL;
				
				//syslog(LOG_NOTICE, "kEventToolbarCreateItemWithIdentifier");
				
				GetEventParameter( inEvent, kEventParamToolbarItemIdentifier, typeCFStringRef, NULL,
						sizeof( CFStringRef ), NULL, &identifier );
				
				GetEventParameter( inEvent, kEventParamToolbarItemConfigData, typeCFTypeRef, NULL,
						sizeof( CFTypeRef ), NULL, &data );
					
				item = wind->CreateToolbarItemForIdentifier( identifier, data );
				
				if ( item )
				{
					SetEventParameter( inEvent, kEventParamToolbarItem, typeHIToolbarItemRef,
						sizeof( HIToolbarItemRef ), &item );
					result = noErr;
				}
			}
			break;
		/****
		case kEventToolbarCreateItemFromDrag:
			syslog(LOG_NOTICE, "kEventToolbarCreateItemFromDrag");
			break;
		***/
	}

	return result;
}

//--------------------------------------------------------------------------------------------
OSStatus
MainWindow::HandleEvent(EventHandlerCallRef inRef, TCarbonEvent& inEvent)
{
	OSStatus	result = eventNotHandledErr;
	
	result = MonitorHandler(inRef, inEvent.GetEventRef(), NULL);
	result = TWindow::HandleEvent(inRef, inEvent);
	
	//syslog(LOG_NOTICE, "raw command = [%d] [%d]", inEvent.GetClass(), inEvent.GetKind()); 

	return result;
}

//--------------------------------------------------------------------------------------------
void
MainWindow::GetToolbarDefaultItems( CFMutableArrayRef array )
{
	CFArrayAppendValue( array, kToolbarAddButton );
	CFArrayAppendValue( array, kToolbarRemoveButton );
	CFArrayAppendValue( array, kHIToolbarSpaceIdentifier );
	CFArrayAppendValue( array, kToolbarOverrideButton );
	CFArrayAppendValue( array, kHIToolbarSpaceIdentifier );
	CFArrayAppendValue( array, kToolbarBlacklistButton);
	CFArrayAppendValue( array, kHIToolbarFlexibleSpaceIdentifier );
	//CFArrayAppendValue( array, kToolbarConnection );
}

//--------------------------------------------------------------------------------------------
void
MainWindow::GetToolbarAllowedItems( CFMutableArrayRef array )
{
	GetToolbarDefaultItems( array );
}

//--------------------------------------------------------------------------------------------
HIToolbarItemRef
MainWindow::CreateToolbarItemForIdentifier( CFStringRef identifier, CFTypeRef configData )
{
	HIToolbarItemRef		item = NULL;

	if (CFStringCompare( kToolbarAddButton, identifier, kCFCompareBackwards) == kCFCompareEqualTo )
	{
		if (HIToolbarItemCreate(identifier, kHIToolbarItemCantBeRemoved, &item) == noErr)
		{
			IconRef icon;
			static bool sRegisteredIcon;
			
			if (!sRegisteredIcon)
			{
				RegisterIcon('CKCB', 'awin', "add.icns");
				sRegisteredIcon = true;
			}
			GetIconRef( kOnSystemDisk, 'CKCB', 'awin', &icon);
			HIToolbarItemSetLabel( item, CFSTR("Add"));
			HIToolbarItemSetIconRef( item, icon );
			HIToolbarItemSetCommandID( item, 'awin');
			ReleaseIconRef( icon );
		}
	}
	
	if (CFStringCompare( kToolbarRemoveButton, identifier, kCFCompareBackwards) == kCFCompareEqualTo )
	{
		if (HIToolbarItemCreate(identifier, kHIToolbarItemCantBeRemoved, &item) == noErr)
		{
			IconRef icon;
			static bool sRegisteredIcon;
			
			if (!sRegisteredIcon)
			{
				RegisterIcon('CKCB', 'rwin', "remove.icns");
				sRegisteredIcon = true;
			}
			GetIconRef( kOnSystemDisk, 'CKCB', 'rwin', &icon);
			HIToolbarItemSetLabel( item, CFSTR("Remove"));
			HIToolbarItemSetIconRef( item, icon );
			HIToolbarItemSetCommandID( item, 'rwin');
			ReleaseIconRef( icon );
		}
	}

	if (CFStringCompare( kToolbarOverrideButton, identifier, kCFCompareBackwards) == kCFCompareEqualTo )
	{
		if (HIToolbarItemCreate(identifier, kHIToolbarItemCantBeRemoved, &item) == noErr)
		{
			IconRef icon;
			static bool sRegisteredIcon;
			
			if (!sRegisteredIcon)
			{
				RegisterIcon('CKCB', 'ovr1', "override_inactive.icns");
				RegisterIcon('CKCB', 'ovr2', "override_active.icns");
				sRegisteredIcon = true;
			}
			GetIconRef( kOnSystemDisk, 'CKCB', 'ovr1', &icon);
			HIToolbarItemSetLabel( item, CFSTR("Stop"));
			HIToolbarItemSetIconRef( item, icon );
			HIToolbarItemSetCommandID( item, 'over');
			ReleaseIconRef( icon );
			this->overrideToolbarItem = item;
		}
	}
	if (CFStringCompare( kToolbarBlacklistButton, identifier, kCFCompareBackwards) == kCFCompareEqualTo )
	{
		if (HIToolbarItemCreate(identifier, kHIToolbarItemCantBeRemoved, &item) == noErr)
		{
			IconRef icon;
			static bool sRegisteredIcon;
			
			if (!sRegisteredIcon)
			{
				RegisterIcon('CKCB', 'blak', "blacklist.ico");
				sRegisteredIcon = true;
			}
			GetIconRef( kOnSystemDisk, 'CKCB', 'blak', &icon);
			HIToolbarItemSetLabel( item, CFSTR("Blacklist"));
			HIToolbarItemSetIconRef( item, icon );
			HIToolbarItemSetCommandID( item, 'blak');
			ReleaseIconRef( icon );
		}
	}
	
	return item;
}

//--------------------------------------------------------------------------------------------
void
MainWindow::RegisterIcon( FourCharCode inCreator, FourCharCode inType, const char* inName )
{
	IconRef result = 0;
	OSErr err = fnfErr;
	
	if ( inName != NULL && *inName != 0 )
	{
		CFBundleRef appBundle = CFBundleGetMainBundle();
		if ( appBundle )
		{
			CFStringRef fileName = CFStringCreateWithCString( NULL, inName, kCFStringEncodingASCII );
			if (fileName != NULL)
			{
				CFURLRef iconFileURL = CFBundleCopyResourceURL( appBundle, fileName, NULL, NULL );
				if (iconFileURL != NULL)
				{
					FSRef		fileRef;
					
					if ( CFURLGetFSRef( iconFileURL, &fileRef ) )
					{
						FSSpec		iconFileSpec;
						
						err = FSGetCatalogInfo( &fileRef, kFSCatInfoNone, NULL, NULL, &iconFileSpec, NULL );
						if ( err == noErr )
							err = RegisterIconRefFromIconFile( inCreator, inType, &iconFileSpec, &result );
					}
					CFRelease( iconFileURL );
				}
				CFRelease( fileName );
			}
		}
	}
}

// --------------------------------------------------------------------
void
MainWindow::InstallDataBrowserCallbacks(ControlRef browser)
{
    DataBrowserCallbacks myCallbacks;
    
    //Use latest layout and callback signatures
    myCallbacks.version = kDataBrowserLatestCallbacks;
    verify_noerr(InitDataBrowserCallbacks(&myCallbacks));
    
    myCallbacks.u.v1.itemDataCallback = 
        NewDataBrowserItemDataUPP(CloneKeysGetSetItemData);
	
	myCallbacks.u.v1.itemCompareCallback = 
		NewDataBrowserItemCompareUPP(CloneKeysItemComparison);

    myCallbacks.u.v1.itemNotificationCallback = 
        NewDataBrowserItemNotificationUPP(CloneKeysItemNotification);
     
 	myCallbacks.u.v1.acceptDragCallback =
 		NewDataBrowserAcceptDragUPP(CloneKeysAcceptDrag);
	myCallbacks.u.v1.receiveDragCallback = 
		NewDataBrowserReceiveDragUPP(CloneKeysReceiveDrag);
	myCallbacks.u.v1.addDragItemCallback = 
		NewDataBrowserAddDragItemUPP(CloneKeysAddDragItem);
	myCallbacks.u.v1.itemHelpContentCallback = 
		NewDataBrowserItemHelpContentUPP(CloneKeysItemHelpContent);

	myCallbacks.u.v1.getContextualMenuCallback = 
		NewDataBrowserGetContextualMenuUPP(CloneKeysGetContextualMenu);
	myCallbacks.u.v1.selectContextualMenuCallback = 
		NewDataBrowserSelectContextualMenuUPP(CloneKeysSelectContextualMenu);

	verify_noerr(SetDataBrowserCallbacks(browser, &myCallbacks));
	
	//AddDataBrowserItems(browser, kDataBrowserNoItem, 0, NULL, kDataBrowserItemNoProperty);
	SetDataBrowserTableViewHiliteStyle(browser, kDataBrowserTableViewFillHilite);
	DataBrowserChangeAttributes(browser, kDataBrowserAttributeListViewDrawColumnDividers | kDataBrowserAttributeListViewAlternatingRowColors, kDataBrowserAttributeNone);
}

//--------------------------------------------------------------------------------------------
OSStatus 
MainWindow::CloneKeysGetSetItemData(ControlRef browser, DataBrowserItemID itemID, DataBrowserPropertyID property, 
    DataBrowserItemDataRef itemData, Boolean changeValue)
{
	OSStatus status = noErr;
	list<CKProcess>::iterator index;
	DataBrowserItemID cnt;
	
	for(index=CloneKeysProcessList.begin(), cnt = 1; index != CloneKeysProcessList.end(); ++index, ++cnt)
	{
		if (cnt == itemID)
			break;
	}
	
	switch(property)
	{
		case kNameColumn:
			//syslog(LOG_NOTICE, "name = [%s]", CFStringGetCStringPtr((*index).name, kCFStringEncodingMacRoman));
			status = SetDataBrowserItemDataText(itemData, (*index).name);
			break;
		case kPSNColumn:
			//syslog(LOG_NOTICE, "psn = [%s]", CFStringGetCStringPtr((*index).psn_string, kCFStringEncodingMacRoman));
			status = SetDataBrowserItemDataText(itemData, (*index).psn_string);
			break;
		case kStatusColumn:
			//syslog(LOG_NOTICE, "status = [%s]", CFStringGetCStringPtr((*index).status, kCFStringEncodingMacRoman));
			status = SetDataBrowserItemDataText(itemData, (*index).status);
			break;
		case kDataBrowserItemIsContainerProperty:
			status = SetDataBrowserItemDataBooleanValue( itemData, false );
			break;
		case kDataBrowserItemIsEditableProperty:
			status = SetDataBrowserItemDataBooleanValue( itemData, false );
			break;
		default:
			status = errDataBrowserPropertyNotSupported;
			break;
	}
	
	return status;
}

// ---------------------------------------------------------------------------------
Boolean
MainWindow::CloneKeysItemComparison(ControlRef browser, DataBrowserItemID itemOneID, 
	DataBrowserItemID itemTwoID, DataBrowserPropertyID sortProperty)
{
	list<CKProcess>::iterator index;
	list<CKProcess>::iterator index1;
	list<CKProcess>::iterator index2;
	DataBrowserItemID cnt;
	
	for(index=CloneKeysProcessList.begin(), cnt = 1; index != CloneKeysProcessList.end(); ++index, ++cnt)
	{
		if (cnt == itemOneID)
		{
			index1 = index;
		}
		if (cnt == itemTwoID)
		{
			index2 = index;
		}
	}
	
	switch(sortProperty)
	{
		case kNameColumn:
			if (CFStringCompare((*index1).name, (*index2).name, kCFCompareBackwards) == kCFCompareLessThan)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		case kPSNColumn:
			if (CFStringCompare((*index1).psn_string, (*index2).psn_string, kCFCompareNumerically) == kCFCompareLessThan)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		case kStatusColumn:
			if (CFStringCompare((*index1).status, (*index2).status, kCFCompareBackwards) == kCFCompareLessThan)
			{
				return true;
			}
			else
			{
				return false;
			}
			break;
		default:
			return itemOneID < itemTwoID;
			break;
	}

	return false;
}

// --------------------------------------------------------------------
void
MainWindow::CloneKeysItemNotification(ControlRef browser, DataBrowserItemID itemID, 
	DataBrowserItemNotification message)
{

}

// --------------------------------------------------------------------
Boolean
MainWindow::CloneKeysAcceptDrag(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID)
{
	return false;
}

// --------------------------------------------------------------------
Boolean
MainWindow::CloneKeysReceiveDrag(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID)
{
	return false;
}

// --------------------------------------------------------------------
Boolean
MainWindow::CloneKeysAddDragItem(ControlRef browser, DragReference theDrag, DataBrowserItemID itemID, ItemReference* itemRef)
{
	return false;
}

// --------------------------------------------------------------------
void 
MainWindow::CloneKeysItemHelpContent(ControlRef browser, DataBrowserItemID item, 
	DataBrowserPropertyID property, HMContentRequest inRequest, 
	HMContentProvidedType *outContentProvided, HMHelpContentPtr ioHelpContent)
{


}

// --------------------------------------------------------------------
void 
MainWindow::CloneKeysGetContextualMenu(ControlRef browser, MenuRef* contextualMenu, UInt32 *helpType, 
	CFStringRef* helpItemString, AEDesc *selection)
{


}

// --------------------------------------------------------------------
void
MainWindow::CloneKeysSelectContextualMenu(ControlRef browser, MenuRef contextualMenu, 
	UInt32 selectionType, SInt16 menuID, MenuItemIndex menuItem)
{
#pragma unused (browser, selectionType, menuID, menuItem)
}


//--------------------------------------------------------------------------------------------
void
Preferences::Create()
{
	ControlRef	control;
	HIViewRef rootRef;
	CFStringRef sOverride;
	
    Preferences* wind = new Preferences();

	// Add some additional events so that we can pass them on to the "global" event handler.
	EventTypeSpec   kEvents[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};
	
	wind->RegisterForEvents( GetEventTypeCount( kEvents ), kEvents);

    // Position new windows in a staggered arrangement on the main screen
    //RepositionWindow( *wind, NULL, kWindowCascadeOnMainScreen );
    
	rootRef = HIViewGetRoot(*wind);

	HIViewFindByID(rootRef, kMoveUp22, &control);
	SetControlValue(control, prefMoveUp22);
	
	HIViewFindByID(rootRef, kMoveUp0, &control);
	SetControlValue(control, prefMoveUp0);
	
	HIViewFindByID(rootRef, kAutoAdd, &control);
	SetControlValue(control, prefAutoAdd);
	
	HIViewFindByID(rootRef, kAutoAddName, &control);
	SetControlData(control, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &prefAutoAddName);
	
	CreateOverrideString(prefOverrideModifiers, prefOverrideKey, &sOverride);
	HIViewFindByID(rootRef, kOverrideField, &control);
	SetControlData(control, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &sOverride);
	
	newOverrideModifiers = prefOverrideModifiers;
	newOverrideKey = prefOverrideKey;
	
    // The window was created hidden, so show it
    wind->Show();
}

//--------------------------------------------------------------------------------------------
OSStatus
Preferences::HandleEvent(EventHandlerCallRef inRef, TCarbonEvent& inEvent)
{
	OSStatus	result = eventNotHandledErr;
	
	if (grab_new_hotkey != NULL)
	{
		result = MonitorHandler(inRef, inEvent.GetEventRef(), NULL);
	}
	else
	{
		result = TWindow::HandleEvent(inRef, inEvent);
	}
	
	//syslog(LOG_NOTICE, "raw command = [%d] [%d]", inEvent.GetClass(), inEvent.GetKind()); 

	return result;
}


//--------------------------------------------------------------------------------------------
Boolean
Preferences::HandleCommand( const HICommandExtended& inCommand )
{
	Boolean result = false;
	Boolean	test;
	CFNumberRef modifiers;
	CFNumberRef key;
	ControlRef	control;
	CFStringRef sOverride;
	HIViewRef rootRef = HIViewGetRoot(*this);
	
    switch ( inCommand.commandID )
    {
		case kHICommandOK:
		
			HIViewFindByID(rootRef, kMoveUp22, &control);
			prefMoveUp22 = GetControlValue( control );
			HIViewFindByID(rootRef, kMoveUp0, &control);
			prefMoveUp0 = GetControlValue( control );
			HIViewFindByID(rootRef, kAutoAdd, &control);
			prefAutoAdd = GetControlValue( control );
			HIViewFindByID(rootRef, kAutoAddName, &control);
			GetControlData(control, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &prefAutoAddName, NULL);
			
			prefOverrideModifiers = newOverrideModifiers;
			prefOverrideKey = newOverrideKey;
			
			modifiers = CFNumberCreate( NULL, kCFNumberSInt32Type, &prefOverrideModifiers );
			key = CFNumberCreate( NULL, kCFNumberSInt32Type, &prefOverrideKey );
			
			CFPreferencesSetAppValue( CFSTR( "MoveUp22" ), prefMoveUp22 ? kCFBooleanTrue : kCFBooleanFalse, kPreferencesKey );
			CFPreferencesSetAppValue( CFSTR( "MoveUp0" ), prefMoveUp0 ? kCFBooleanTrue : kCFBooleanFalse, kPreferencesKey );
			CFPreferencesSetAppValue( CFSTR( "AutoAdd" ), prefAutoAdd ? kCFBooleanTrue : kCFBooleanFalse, kPreferencesKey );
			CFPreferencesSetAppValue( CFSTR( "AutoAddName" ), prefAutoAddName, kPreferencesKey );
			CFPreferencesSetAppValue( CFSTR( "OverrideModifiers" ), modifiers, kPreferencesKey );
			CFPreferencesSetAppValue( CFSTR( "OverrideKey" ), key, kPreferencesKey );
			
			CFPreferencesAppSynchronize( kPreferencesKey );
			grab_new_hotkey = NULL;
			delete(this);
			result = true;
			break;
			
		case kHICommandCancel:
			grab_new_hotkey = NULL;
			delete(this);
			result = true;
			break;
		
		case kPrefMoveUp22:
		case kPrefMoveUp0:
			if (inCommand.commandID == kPrefMoveUp22)
			{
				HIViewFindByID(rootRef, kMoveUp22, &control);
			}
			else
			{
				HIViewFindByID(rootRef, kMoveUp0, &control);
			}
			test = GetControlValue( control );
			if (test == true)
			{
				HIViewFindByID(rootRef, kAutoAdd, &control);
				SetControlValue(control, false);
			}
			result = true;
			break;
		case kPrefAutoAdd:
			HIViewFindByID(rootRef, kAutoAdd, &control);
			test = GetControlValue( control );
			if (test == true)
			{
				HIViewFindByID(rootRef, kMoveUp22, &control);
				SetControlValue(control, false);
				HIViewFindByID(rootRef, kMoveUp0, &control);
				SetControlValue(control, false);
			}
			result = true;
			break;
		case kPrefAutoAddName:
			result = true;
			break;
		
		case kPrefOverrideHotkey:
			HIViewFindByID(rootRef, kOverrideField, &control);
			sOverride = CFSTR("<press new key combo>");
			SetControlData(control, kControlEntireControl, kControlEditTextCFStringTag, sizeof(CFStringRef), &sOverride);
			grab_new_hotkey = *this;
			result = true;
			break;
			
		default:
            return false;
    }
	
	//syslog(LOG_NOTICE, "sOverride = [%s]", CFStringGetCStringPtr(sOverride, kCFStringEncodingMacRoman));
	
	return result;
}

//--------------------------------------------------------------------------------------------
void
AboutBox::Create()
{
	OSStatus					err;
	ControlRef					control;
	ControlRef					root;
	HIViewRef					rootRef;
	Rect						bounds;
	ControlImageContentInfo		content = { kControlContentIconRef };
	ProcessSerialNumber			psn = { 0, kCurrentProcess };
	FSRef						appLocation;
	HIRect						hiBounds;
	
    AboutBox* wind = new AboutBox();

	rootRef = HIViewGetRoot(*wind);

	GetProcessBundleLocation( &psn, &appLocation);
	GetIconRefFromFileInfo( &appLocation, 0, NULL, 0, NULL, kIconServicesNormalUsageFlag, &content.u.iconRef, NULL );
	
	HIViewFindByID(rootRef, kICONid, &control);
	GetControlBounds(control, &bounds);
	DisposeControl(control);
	
	CreateIconControl( *wind, &bounds, &content, true, &control);
	
	//HIViewFindByID(rootRef, kVERSid, &control);
	//SetControlData(control, kControlEntireControl, 
	
	HIViewFindByID(rootRef, kCKURLid, &control);
	GetControlBounds( control, &bounds );
	DisposeControl( control );
	
	hiBounds.origin.x = bounds.left;
	hiBounds.origin.y = bounds.top;
	hiBounds.size.width = bounds.right - bounds.left;
	hiBounds.size.height = bounds.bottom - bounds.top;
	
	err = HIURLTextViewCreate( &hiBounds, CFURLCreateWithString( NULL, CFSTR("http://code.google.com/p/clonekeys/"), NULL ),
							   CFSTR("http://code.google.com/p/clonekeys/") , &control );

	GetRootControl( *wind, &root );
	HIViewAddSubview( root, control );

	if ( err == noErr )
		ShowControl( control );

	HIViewFindByID(rootRef, kKCURLid, &control);
	GetControlBounds( control, &bounds );
	DisposeControl( control );
	
	hiBounds.origin.x = bounds.left;
	hiBounds.origin.y = bounds.top;
	hiBounds.size.width = bounds.right - bounds.left;
	hiBounds.size.height = bounds.bottom - bounds.top;
	
	err = HIURLTextViewCreate( &hiBounds, CFURLCreateWithString( NULL, CFSTR("https://solidice.com/keyclone/index.html"), NULL ),
							   CFSTR("keyclone") , &control );

	HIViewAddSubview( root, control );

	if ( err == noErr )
		ShowControl( control );
		
    // The window was created hidden, so show it
    wind->Show();
}

//--------------------------------------------------------------------------------------------
Boolean
AboutBox::HandleCommand( const HICommandExtended& inCommand )
{
	Boolean result = false;
	
    switch ( inCommand.commandID )
    {
		case kHICommandOK:
			delete(this);
			result = true;
			break;
			
		default:
            return false;
    }
	
	//syslog(LOG_NOTICE, "sOverride = [%s]", CFStringGetCStringPtr(sOverride, kCFStringEncodingMacRoman));
	
	return result;
}

void BlackList::Create()
{
	//look at preferences::create() to get control!
	ControlRef	control;
	HIViewRef rootRef;
	CFStringRef sOverride;
	
	BlackList *wind = new BlackList();

	// Add some additional events so that we can pass them on to the "global" event handler.
	EventTypeSpec   kEvents[] =
	{
		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyUp },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};
	
	wind->RegisterForEvents(GetEventTypeCount(kEvents), kEvents);
	
	rootRef = HIViewGetRoot(*wind);
	
	HIViewFindByID(rootRef, kButtonEnterKey, &control);
	SetControlValue(control, kBlackButtonEnterKey);
	
	HIViewFindByID(rootRef, kTextKey, &control);
	SetControlValue(control, kBlackTextKey);
	
	HIViewFindByID(rootRef, kButtonAdd, &control);
	SetControlValue(control, kBlackButtonAdd);
	
	HIViewFindByID(rootRef, kDataKeys, &control);
	SetControlValue(control, kBlackData);
	
	
	wind->Show();
}

Boolean BlackList::HandleCommand(const HICommandExtended &inCommand)
{
	return true;
}

OSStatus BlackList::HandleEvent(EventHandlerRef inRef, TCarbonEvent& inEvent)
{
	OSStatus	result = eventNotHandledErr;
	
	return result;
	
}

