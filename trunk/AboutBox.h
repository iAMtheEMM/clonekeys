//
//  AboutBox.h
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

static const ControlID kICONid		= { 'ICON', 0 };
static const ControlID kVERSid		= { 'VERS', 0 };
static const ControlID kCKURLid		= { 'URLV', 0 };
static const ControlID kKCURLid		= { 'URLV', 1 };

// Our AboutBox window class
class AboutBox : public TWindow
{
    public:
                            AboutBox() : TWindow( CFSTR("AboutBox") ) {}
        virtual             ~AboutBox() {}
        
        static void         Create();
        
    protected:
        virtual Boolean     HandleCommand( const HICommandExtended& inCommand );

};


