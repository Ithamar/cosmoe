/*
 *  The Cosmoe application server
 *  Copyright (C) 1999 - 2001 Kurt Skauen
 *  Copyright (C) 2002 Bill Hayden
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __F_CONFIG_H__
#define __F_CONFIG_H__

#include <stdio.h>
#include <string>
#include <map>

#include <SupportDefs.h>
#include <View.h>
#include <Font.h>
#include <Message.h>

class AppserverConfig
{
public:
	AppserverConfig();
	~AppserverConfig();
	static AppserverConfig* GetInstance();
	
	int        LoadConfig( FILE* hFile, bool bActivateConfig );
	int SaveConfig();
	bool        IsDirty() const;
	void        SetConfig( const BMessage* pcConfig );
	void        GetConfig( BMessage* pcConfig );

	// Mouse configurations:
	
	void        SetDoubleClickTime( bigtime_t nDelay );
	bigtime_t        GetDoubleClickTime() const;

	// Font configuration:

	const font_properties* GetFontConfig( const std::string& cName );
	status_t                       SetFontConfig( const std::string& cName, const font_properties& cProps );
	status_t                       AddFontConfig( const std::string& cName, const font_properties& cProps );
	status_t                       DeleteFontConfig( const std::string& cName );
	
	// Apperance/screen mode:
	int                SetWindowDecoratorPath( const std::string& cPath );
	std::string        GetWindowDecoratorPath() const;

	void        SetDefaultColor( default_color_t eIndex, const rgb_color& sColor );

	// Misc configurations:

private:
	static AppserverConfig* s_pcInstance;
	bool        m_bDirty;
	std::string        m_cWindowDecoratorPath;
	bigtime_t        m_nDoubleClickDelay;
	std::map<std::string,font_properties> m_cFontSettings;
};


#endif // __F_CONFIG_H__
