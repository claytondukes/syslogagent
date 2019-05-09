//extern "C" by erno aug04
extern "C" {

/*-----------------------------------------------------------------------------
 *
 *  event.h - Event type definition
 *
 *    Copyright (c) 1998, SaberNet.net - All rights reserved
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public License
 *    as published by the Free Software Foundation; either version 2
 *    of the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307
 *
 *  $Id: event.h,v 1.12 2002/09/20 06:12:47 jason Exp $
 *
 *----------------------------------------------------------------------------*/
#ifndef _EVENT_H_
#define _EVENT_H_

#include "ntsl.h"
#include <time.h>


/*-------------------------------[ static data ]------------------------------*/
#define NTSL_EVENT_ERROR		 L"[error]"
#define NTSL_EVENT_WARNING  	 L"[warning]"
#define NTSL_EVENT_INFORMATION   L"[info]"
#define NTSL_EVENT_SUCCESS       L"[success]"
#define NTSL_EVENT_FAILURE       L"[failure]"
#define NTSL_EVENT_FORMAT_LEN    NTSL_EVENT_LEN
#define NTSL_DEFAULT_PRIORITY	9

/*-------------------------------[ ntsl_event ]-------------------------------*/
typedef struct 
{
	wchar_t date[NTSL_DATE_LEN];
	//erno2005
	wchar_t    facilityName[256];
	DWORD   time1970format;

	wchar_t    host[NTSL_SYS_LEN];
	wchar_t	source[NTSL_SYS_LEN];
	wchar_t	etype[NTSL_SYS_LEN];
    wchar_t    msg[NTSL_EVENT_LEN];
    int     priority;
	uint32	id;
	wchar_t	user[NTSL_SYS_LEN];
	wchar_t	domain[NTSL_SYS_LEN];
} ntsl_event;


//typedef struct {
//	char text[1200];  //MAXBUFLEN not used due to c/c++ problems if including appwatch.h
//} aMess;

//erno2005
int _event_output(ntsl_event *event);

//void ping_syslog_server();

#endif

} //end extern "C" by erno aug04

