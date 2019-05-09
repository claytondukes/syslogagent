﻿// NTSyslog.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include "..\Syslogserver\common_stdafx.h" 

#include "pch.h"
#include <iostream>

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <tchar.h>
#include <time.h>
#include "NTSyslog.h"
#include "ntsl.h"
#include "RegistrySettings.h"
#include "errorHandling.h"
#include "error.h"

LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize);

__int64 LastWaitForSingleObjectTime;
bool ServiceStop;
DebugFlagsDef DebugFlags;

extern void ernoUnloadLoadLibrary(); //clean up..


/*------------------------------[ private data ]------------------------------*/
static SERVICE_STATUS			service_status;
static SERVICE_STATUS_HANDLE	service_status_handle;
static DWORD					service_error;
static char						service_error_msg[NTSL_SYS_LEN];
static HANDLE					service_stop_event;

/*----------------------------[ private functions ]---------------------------*/
static void WINAPI service_main(int argc, char **argv);
static void WINAPI service_ctrl(DWORD ctrlCode);
static bool        service_report_status(DWORD currentState, DWORD exitCode, DWORD waitHint);
static void        service_install();
static void        service_remove();
static void	       service_debug(int argc, char **argv);
static bool WINAPI service_control_handler(DWORD ctrlType);
static void        service_addEventSource(wchar_t* path);

void obtainDebugLogFileName() {
	char LocalOwnPath[256] = "";
	__time64_t long_time;
	char tmp[64];
	struct tm newtime;
	// Get full pathname of the exe-files
	DWORD dwLength = GetModuleFileName(NULL, (LPTSTR)&LocalOwnPath[0], (sizeof(LocalOwnPath) / sizeof(LocalOwnPath[0])));
	if (dwLength) {
		while (dwLength && LocalOwnPath[dwLength] != _T('\\')) {
			dwLength--;
		}
		if (dwLength)
			LocalOwnPath[dwLength + 1] = _T('\000');
	}
	strncpy(DebugFlags.DebugFilePath, LocalOwnPath, 255);
	strncpy(DebugFlags.DebugDumpFilePath, LocalOwnPath, 255);
	strncpy(DebugFlags.DebugOldDumpFilePath, LocalOwnPath, 255);
	strcat(DebugFlags.DebugFilePath, "SyslogAgentDebug");
	strcat(DebugFlags.DebugDumpFilePath, "SyslogAgentCrashInfo.dmp");
	strcat(DebugFlags.DebugOldDumpFilePath, "SyslogAgentCrashInfo_old.dmp");

	_time64(&long_time);
	_localtime64_s(&newtime, &long_time);
	sprintf(tmp, "%02d%02d%02d", newtime.tm_hour, newtime.tm_min, newtime.tm_sec);

	strcat(DebugFlags.DebugFilePath, tmp);
	strcat(DebugFlags.DebugFilePath, ".log");

}
//********************************************************
void logToFile(wchar_t* text) {
	//Write to file
	FILE *fp;
	struct tm newtime;
	wchar_t tmp[64];
	__time64_t long_time;
	_time64(&long_time);
	_localtime64_s(&newtime, &long_time);
	wsprintf(tmp, L"%02d%02d%02d", newtime.tm_hour, newtime.tm_min, newtime.tm_sec);

	fopen_s(&fp, DebugFlags.DebugFilePath, "a");
	if (fp != NULL) {
		fprintf(fp, "%02d%02d%02d %s\n", newtime.tm_hour, newtime.tm_min, newtime.tm_sec, text);
		fclose(fp);
	}
}

/********************************************************
myInvalidParameterHandler -  handle invalid parameter situation, as a dump file be default is *not* created due to security
http://msdn.microsoft.com/en-us/library/a9yf33zb.aspx
********************************************************/

void myInvalidParameterHandler(const wchar_t* expression,
	const wchar_t* function,
	const wchar_t* file,
	unsigned int line,
	uintptr_t pReserved)
{
	wchar_t textOut[1024];
	wsprintf(textOut, L"Invalid parameter detected in function %s. File: %s Line: %d", function, file, line);
	logToFile(textOut);
	wsprintf(textOut, L"Expression: %s", expression);
	logToFile(textOut);
}
/********************************************************
DEBUGSERVICE  -  Log to file if argument -DSERVICE is set for service
********************************************************/
void DEBUGSERVICE(int indentLevel, wchar_t *a, ...) {
	va_list args;
	va_start(args, a);
	if (DebugFlags.Debug_Service) {

		if ((*a) != NULL) { //Perhaps just an OutLevel debugalarm function call
			wchar_t outText[1024] = L"Service Debug:                    "; //blanks in case of indentation
			wchar_t *outTextPtr = (wchar_t*)&outText + 14 + DebugFlags.DebugServiceIndentation;
			wsprintf((wchar_t*)outTextPtr, a, args);
			logToFile((wchar_t*)outText);
		}
		switch (indentLevel) {
		case EndHeader:
			DebugFlags.DebugServiceIndentation = max(0, DebugFlags.DebugServiceIndentation - 2);
			break;
		case Header:
			DebugFlags.DebugServiceIndentation = min(20, DebugFlags.DebugServiceIndentation + 2);
			break;
		case Title:
			DebugFlags.DebugServiceIndentation = 0;
			break;
		default: //Message etc
			break;
		}
	}
	va_end(args);
}
/********************************************************
DEBUGLOGGER  -  Log to file if argument -DLOGGER is set for service
********************************************************/
void DEBUGLOGGER(int indentLevel, wchar_t *a, ...) {
	va_list args;
	va_start(args, a);
	if (DebugFlags.Debug_Logger) {

		if ((*a) != NULL) { //Perhaps just an OutLevel debugalarm function call
			wchar_t outText[1024] = L"Service logger:                    "; //blanks in case of indentation
			wchar_t *outTextPtr = (wchar_t*)&outText + 15 + DebugFlags.DebugLoggerIndentation;
			wsprintf(outTextPtr, a, args);
			logToFile(outText);
		}
		switch (indentLevel) {
		case EndHeader:
			DebugFlags.DebugLoggerIndentation = max(0, DebugFlags.DebugLoggerIndentation - 2);
			break;
		case Header:
			DebugFlags.DebugLoggerIndentation = min(20, DebugFlags.DebugLoggerIndentation + 2);
			break;
		case Title:
			DebugFlags.DebugLoggerIndentation = 0;
			break;
		default: //Message etc
			break;
		}
	}
	va_end(args);
}
/********************************************************
DEBUGPARSE  -  Log to file if argument -DPARSE is set for service
********************************************************/
void DEBUGPARSE(int indentLevel, wchar_t *a, ...) {
	va_list args;
	va_start(args, a);
	if (DebugFlags.Debug_Parse) {

		if ((*a) != NULL) { //Perhaps just an OutLevel function call
			wchar_t outText[1024] = L"Parse Debug:                    "; //blanks in case of indentation
			wchar_t *outTextPtr = (wchar_t*)&outText + 14 + DebugFlags.DebugParseIndentation;
			wsprintf(outTextPtr, a, args);
			logToFile(outText);
		}
		switch (indentLevel) {
		case EndHeader:
			DebugFlags.DebugParseIndentation = max(0, DebugFlags.DebugParseIndentation - 2);
			break;
		case Header:
			DebugFlags.DebugParseIndentation = min(20, DebugFlags.DebugParseIndentation + 2);
			break;
		case Title:
			DebugFlags.DebugParseIndentation = 0;
			break;
		default: //Message etc
			break;
		}
	}
	va_end(args);
}
/********************************************************
DEBUGAPPLPARSE  -  Log to file if argument -DAPPLPARSE is set for service
********************************************************/
void DEBUGAPPLPARSE(int indentLevel, wchar_t *a, ...) {
	va_list args;
	va_start(args, a);
	if (DebugFlags.Debug_Appl) {

		if ((*a) != NULL) { //Perhaps just an OutLevel function call
			wchar_t outText[1024] = L"Applpar Debug:                    "; //blanks in case of indentation
			wchar_t *outTextPtr = (wchar_t*)&outText + 14 + DebugFlags.DebugApplIndentation;
			wsprintf(outTextPtr, a, args);
			logToFile(outText);
		}
		switch (indentLevel) {
		case EndHeader:
			DebugFlags.DebugApplIndentation = max(0, DebugFlags.DebugApplIndentation - 2);
			break;
		case Header:
			DebugFlags.DebugApplIndentation = min(20, DebugFlags.DebugApplIndentation + 2);
			break;
		case Title:
			DebugFlags.DebugApplIndentation = 0;
			break;
		default: //Message etc
			break;
		}
	}
	va_end(args);
}
/*----------------------------------[ main ]----------------------------------
	*  Service entrypoint.
	*
	*  Parameters:
	*		argc  -	 number of command line arguments
	*		argv  -  array of command line arguments
	*
	*  Return value:
	*		none
	*----------------------------------------------------------------------------*/
	//erno void _CRTAPI1 main(int argc, char **argv)
void __cdecl main(int argc, char **argv) {
	char SyslogAddress[128];
	int i;

	memset(&DebugFlags, 0, sizeof(DebugFlags));

	SERVICE_TABLE_ENTRY dispatchTable[] = {
		{ (LPTSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)service_main },
		{ NULL, NULL }
	};

	obtainDebugLogFileName(); //Can log after this

//	char temp[256]="Antal argument ";
//	sprintf(temp,"argumentantal %d",argc);
//	logToFile(temp);

	//Check possible debug arguments
	for (i = 1; i < (int)argc; i++) {
		if (_stricmp(argv[i], "-install") == 0) {
			printf("%s Version %s.%s\n%s\n\n", APP_NAME, VERSION_MAJOR, VERSION_MINOR, COPYRIGHT);
			if (argc > 2) {
				strncpy_s(SyslogAddress, sizeof(SyslogAddress), argv[2], 128);
				service_install();
				initRegistry(&SyslogAddress[0]);
			}
			else {
				printf("\nMust enter IP address of Server to log to. Service not installed.\n\n");
			}
			exit(0);
		}
		else if (_stricmp(argv[i], "-remove") == 0) {
			printf("%s Version %s.%s\n%s\n\n", APP_NAME, VERSION_MAJOR, VERSION_MINOR, COPYRIGHT);
			service_remove();
			exit(0);
		}
		else if (_stricmp(argv[i], "-DSERVICE") == 0) {
			DebugFlags.Debug_Service = true;
		}
		else if (_stricmp(argv[i], "-DEBUG") == 0) {
			DebugFlags.Debugging = true;
		}
		else if (_stricmp(argv[i], "-debug") == 0) {
			DebugFlags.Debugging = true;
		}
		else if (_stricmp(argv[i], "-DLOGGER") == 0) {
			DebugFlags.Debug_Logger = true;
		}
		else if (_stricmp(argv[i], "-DPARSE") == 0) {
			DebugFlags.Debug_Parse = true;
		}
		else if (_stricmp(argv[i], "-DAPPLPARSE") == 0) {
			DebugFlags.Debug_Appl = true;
		}
		else {
			wchar_t temp[256] = L"Option ";
			wcscat(temp, (wchar_t*)T2W((LPTSTR)argv[i]));
			wcscat(temp, L" was not recognized.");
			logToFile(temp);
		}
		//add more options as needed
	}

	if (DebugFlags.Debug_Service) {
		DEBUGSERVICE(Message, (LPTSTR)L" Debugging service.");
	}

	if (DebugFlags.Debugging) {
		service_debug(argc, argv);
	}

	// if it doesn't match any of the above parameters
	// the service control manager may be starting the service
	// so we must call StartServiceCtrlDispatcher
	printf("%s Version %s.%s\n%s\n\n", APP_NAME, VERSION_MAJOR, VERSION_MINOR, COPYRIGHT);
	printf("%s -install SyslogServerIP         to install the service locally\n", APP_NAME);
	printf("%s -remove                         to remove the service\n", APP_NAME);
	printf("%s -debug                          to run as a console app for debugging\n\n", APP_NAME);

	if (!StartServiceCtrlDispatcher(dispatchTable)) {
		DWORD a;
		a = GetLastError();
		if (ERROR_FAILED_SERVICE_CONTROLLER_CONNECT != a)
			ntsl_log_error((LPTSTR)NTSL_ERROR_SERVICE_DISPATCH, GetLastError());
	}
}

/*------------------------------[ service_main ]------------------------------
	*  Calls service initialization routines.
	*
	*  Parameters:
	*		argc  -	 number of command line arguments
	*		argv  -  array of command line arguments
	*
	*  Return value:
	*		none
	*----------------------------------------------------------------------------*/
static void WINAPI service_main(int argc, char **argv) {
	int i;
	//	char temp[256];
	//	for (i=0;i<(int)argc;i++) {
	//	sprintf(temp,"Option %d was %s",i,argv[i]);
	//	logToFile(temp);
	//	}

		//Check possible debug arguments
	for (i = 1; i < (int)argc; i++) {
		if (_stricmp(argv[i], "-DSERVICE") == 0) {
			DebugFlags.Debug_Service = true;
		}
#ifdef _DEBUG
		else if (_stricmp(argv[i], "-DEBUG") == 0) {
			DebugFlags.Debugging = true;
		}
		else if (_stricmp(argv[i], "-debug") == 0) {
			DebugFlags.Debugging = true;
		}
#endif
		else if (_stricmp(argv[i], "-DLOGGER") == 0) {
			DebugFlags.Debug_Logger = true;
		}
		else if (_stricmp(argv[i], "-DAPPLPARSE") == 0) {
			DebugFlags.Debug_Appl = true;
		}
		else if (_stricmp(argv[i], "-DPARSE") == 0) {
			DebugFlags.Debug_Parse = true;
		}
		else {
			wchar_t temp[256] = L"Option ";
			wcscat(temp, T2W((LPTSTR)argv[i]));
			wcscat(temp, L" was not recognized.");
			logToFile(temp);
		}
		//add more options as needed
	}

	DEBUGSERVICE(Header, (LPTSTR)L"Start service_main.");
	DEBUGLOGGER(Message, (LPTSTR)L"%s Version %s.%s\n\n", APP_NAME, VERSION_MAJOR, VERSION_MINOR);

	service_status_handle = RegisterServiceCtrlHandler(((LPCTSTR)SERVICE_NAME), service_ctrl);

	if (!service_status_handle) {
		DEBUGSERVICE(Message, (LPTSTR)L"Failed to register at service handler.");
		goto cleanup;
	}

	service_status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	service_status.dwServiceSpecificExitCode = 0;


	// report the status to the service control manager.
	if (!service_report_status(SERVICE_START_PENDING, NO_ERROR, 3000)) {
		DEBUGSERVICE(Message, (LPTSTR)L"Failed to report start pending to service handler.");
		goto cleanup;
	}

	service_start(argc, argv);

cleanup:

	// try to report the stopped status to the service control manager.
	if (service_status_handle) {
		DEBUGSERVICE(EndHeader, (LPTSTR)L"Leaving service_main, and reporting service stopped.");
		(VOID)service_report_status(SERVICE_STOPPED, service_error, 0);
	}

	return;
}



/*------------------------------[ service_ctrl ]------------------------------
	*  Called by the SCM whenever ControlService() is called for this service
	*
	*  Parameters:
	*		ctrlCode - type of control requested
	*
	*  Return value:
	*		none
	*----------------------------------------------------------------------------*/
void WINAPI service_ctrl(uint32 ctrlCode) {

	wchar_t temp[64];
	wsprintf(temp, L"Service_ctrl received code %u.", ctrlCode);

	DEBUGSERVICE(Message, temp);
	switch (ctrlCode) {
		// stop the service.
		//
		// SERVICE_STOP_PENDING should be reported before
		// setting the Stop Event - hServerStopEvent - in
		// service_stop().  This avoids a race condition
		// which may result in a 1053 - The Service did not respond...
		// error.
	case SERVICE_CONTROL_SHUTDOWN:
	case SERVICE_CONTROL_STOP:
		service_stop();
		ernoUnloadLoadLibrary(); //clean up..
		return;

		// Update the service status.
		//
	case SERVICE_CONTROL_INTERROGATE:
		break;

		// invalid control code
		//
	default:
		break;
	}

	service_report_status(service_status.dwCurrentState, NO_ERROR, 0);
}


/*-----------------------------[ service_halting ]-----------------------------
	*  Service status
	*
	* //erno this is a *very* expensive call - avoid at all cost...
	*  Returns:
	*		running		0
	*		halting		1
	*----------------------------------------------------------------------------*/
int service_halting() {
	__int64	thisrun;

	time((time_t*)&thisrun);

	if (ServiceStop) return 1; //ServiceStop introducerat om service_stop_event 'f�rbrukas' inne i en loop och d�refter inte rapporteras att tj�nsten skall stoppa.
								// inf�r 3.0 stoppade inte tj�nsten p� win2003...

	if (thisrun >= (LastWaitForSingleObjectTime + 2)) {
		LastWaitForSingleObjectTime = thisrun;
		ServiceStop = (WaitForSingleObject(service_stop_event, NTSL_LOOP_WAIT) == WAIT_OBJECT_0);
		return ServiceStop;
	}
	else
		return 0;
}



/*--------------------------[ service_report_status ]--------------------------
	*  Sets the current status and reports it to the Service Control Manager
	*
	*  Parameters:
	*		currentState	-  the state of the service
	*		exitCode		-  error code to report
	*		waitHint		-  worst case estimate to next checkpoint
	*
	*  Return value:
	*		true			-  success
	*		false			-  failure
	*----------------------------------------------------------------------------*/
static bool service_report_status(uint32 currentState, uint32 exitCode, uint32 waitHint) {
	static uint32 checkPoint = 1;
	bool		  rc = true;

	if (!DebugFlags.Debugging) {// when debugging we don't report to the SCM
		if (currentState == SERVICE_START_PENDING)
			service_status.dwControlsAccepted = 0;
		else
			service_status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;

		service_status.dwCurrentState = currentState;
		service_status.dwWin32ExitCode = exitCode;
		service_status.dwWaitHint = waitHint;

		if ((currentState == SERVICE_RUNNING) ||
			(currentState == SERVICE_STOPPED))
			service_status.dwCheckPoint = 0;
		else
			service_status.dwCheckPoint = checkPoint++;

		// report the status of the service to the service control manager.
		if (!(rc = SetServiceStatus(service_status_handle, &service_status)))
			ntsl_log_error((LPTSTR)NTSL_ERROR_SERVICE_STATUS, GetLastError());
	}
	return(rc);
}



/*-----------------------------[ service_install ]-----------------------------
	*  Installs the service.
	*----------------------------------------------------------------------------*/
static void service_install()
{
	SC_HANDLE   service;
	SC_HANDLE   manager;
	wchar_t		path[NTSL_PATH_LEN];
	LPCTSTR lpDependencies = __TEXT("EventLog\0");
	HKEY hk;
	//    DWORD dwData; 
	wchar_t  key[256];
	wchar_t serviceDescription[] = L"Forwards Event logs to the Syslog server";

	if (GetModuleFileName(NULL, path + 1, NTSL_PATH_LEN - 1) == 0)
	{
		_tprintf(TEXT("Unable to install %s - %s\n"), SERVICE_NAME,
			GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));
		return;
	}

	// quote path
	path[0] = '"';
	wcscat(path, L"\"");

	service_addEventSource(path);

	manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (manager)
	{
		service = CreateService(
			manager,                    // SCManager database
			SERVICE_NAME,         // name of service
			SERVICE_NAME,			// name to display
			SERVICE_ALL_ACCESS,         // desired access
			SERVICE_WIN32_OWN_PROCESS,  // service type
			SERVICE_AUTO_START,			// start type
			SERVICE_ERROR_NORMAL,       // error control type
			path,                       // service's binary
			NULL,                       // no load ordering group
			NULL,                       // no tag identifier
			lpDependencies,             // dependencies
			NULL,                       // LocalSystem account
			NULL);                      // no password

		if (service)
		{
			_tprintf(TEXT("%s installed.\n"), SERVICE_NAME);
			CloseServiceHandle(service);
		}
		else
		{
			_tprintf(TEXT("CreateService failed - %s\n"), GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));
		}

		CloseServiceHandle(manager);
	}
	else
		_tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));

	wsprintf(key, L"SYSTEM\\CurrentControlSet\\Services\\Syslog Agent");
	if (RegOpenKey(HKEY_LOCAL_MACHINE, L"SYSTEM\\CurrentControlSet\\Services\\Syslog Agent", &hk) != ERROR_SUCCESS)
		return;

	// Add the name to the EventMessageFile subkey. 
	if (RegSetValueEx(hk,             // subkey handle 
		(LPCTSTR)"Description",		       // value name 
		0,                        // must be zero 
		REG_EXPAND_SZ,            // value type 
		(LPBYTE)serviceDescription,           // pointer to value data 
		wcslen(serviceDescription) + 1))       // length of value data 
		return; //error - no big deal

	RegCloseKey(hk);

}

/*-----------------------------[ service_remove ]-----------------------------
	*  Stops and removes the service
	*----------------------------------------------------------------------------*/
static void service_remove()
{
	SC_HANDLE   service;
	SC_HANDLE   manager;

	manager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (manager)
	{
		service = OpenService(manager, SERVICE_NAME, SERVICE_ALL_ACCESS);

		if (service)
		{
			// try to stop the service
			if (ControlService(service, SERVICE_CONTROL_STOP, &service_status))
			{
				_tprintf(TEXT("Stopping %s."), SERVICE_NAME);
				Sleep(1000);

				while (QueryServiceStatus(service, &service_status))
				{
					if (service_status.dwCurrentState == SERVICE_STOP_PENDING)
					{
						_tprintf(TEXT("."));
						Sleep(1000);
					}
					else
						break;
				}

				if (service_status.dwCurrentState == SERVICE_STOPPED)
					_tprintf(TEXT("\n%s stopped.\n"), SERVICE_NAME);
				else
					_tprintf(TEXT("\n%s failed to stop.\n"), SERVICE_NAME);
			}

			// now remove the service
			if (DeleteService(service))
				_tprintf(TEXT("%s removed.\n"), SERVICE_NAME);
			else
				_tprintf(TEXT("DeleteService failed - %s\n"),
					GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));


			CloseServiceHandle(service);
		}
		else
			_tprintf(TEXT("OpenService failed - %s\n"),
				GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));

		CloseServiceHandle(manager);
	}
	else
		_tprintf(TEXT("OpenSCManager failed - %s\n"),
			GetLastErrorText((LPTSTR)service_error_msg, NTSL_SYS_LEN));
}



/*------------------------------[ service_debug ]------------------------------
	*  Runs the service as a console application
	*
	*  Parameters:
	*		argc  -  number of command line arguments
	*		argv  -  array of command line arguments
	*----------------------------------------------------------------------------*/
static void service_debug(int argc, char **argv) {
	SetConsoleCtrlHandler(service_control_handler, TRUE);
	service_start(argc, argv);
}


/*-------------------------[ service_control_handler ]-------------------------
	*  Handles console control events.
	*
	*  Parameters:
	*		crtlType  -  type of control event
	*
	*  Return value:
	*		true  - handled
	*      false - not handled
	*----------------------------------------------------------------------------*/
static bool WINAPI service_control_handler(uint32 ctrlType)
{
	switch (ctrlType)
	{
		// value of CTRL_BREAK_EVENT is same as CTRL_BREAK_EVENT...
		case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
			DEBUGSERVICE(Message, (LPTSTR)L"Stopping service after received ctrl-c.");
		case SERVICE_CONTROL_STOP: //==CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
		case SERVICE_CONTROL_SHUTDOWN:
			service_stop();
			return true;
			break;
	}

	return false;
}


/*------------------------------[ RemoveOldDumpFile ]------------------------------
	* Dump file cannot be overwritten in exception handling (--> overwriting the most interesting information).
	At service start the existing one is renamed to _old.
	*----------------------------------------------------------------------------*/
void RemoveOldDumpFile() {
	int retCode;

	//remove(DebugFlags.DebugOldDumpFilePath);

	retCode = rename(DebugFlags.DebugDumpFilePath, DebugFlags.DebugOldDumpFilePath);

	if (retCode == 0) {
		DEBUGSERVICE(Informational, (LPTSTR)L"Existing dump file SyslogAgentCrashInfo.dmp in SyslogAgent directory renamed to SyslogAgentCrashInfo_old.dmp.");
	}
}
/*------------------------------[ service_start ]------------------------------
	* Starts and runs the service
	*----------------------------------------------------------------------------*/
void service_start(int argc, char **argv) {

	bool forwardEvents = 0; //ehh, implemented as int in C?
	int EventLogPollInterval = 10;  //seconds between checking the event log
	ServiceStop = false;

	int syslogPort, backupSyslogPort;

	SetUnhandledExceptionFilter(top_level_exception_filter);



	//	We would like invalid parameter to not just crash, but rather involve error handling.
	//	_set_invalid_parameter_handler(...
	_invalid_parameter_handler oldHandler, newHandler;
	newHandler = myInvalidParameterHandler;
	oldHandler = _set_invalid_parameter_handler(newHandler);

	__try
	{
		DEBUGSERVICE(Header, (LPTSTR)L"Preparing service start.");

		RemoveOldDumpFile();

		// report the status to the service control manager.
		if (!service_report_status(SERVICE_START_PENDING, NO_ERROR, 3000)) {
			DEBUGSERVICE(Message, (LPTSTR)L"Failed to report start pending to service handler from service_start.");
			return;
		}

		// create the event object. The control handler function signals
		// this event when it receives the "stop" control code.
		service_stop_event = CreateEvent(NULL, true, false, NULL);

		if (NULL == service_stop_event) {
			DEBUGSERVICE(Message, (LPTSTR)L"Failed to create service_stop_event.");
			return;
		}
		// report the status to the service control manager.
		if (!service_report_status(SERVICE_START_PENDING, NO_ERROR, 3000)) {
			DEBUGSERVICE(Message, (LPTSTR)L"Failed to report second start pending to service handler from service_start.");
			return;
		}

		//erno Ensure we have a syslog host adress
		if (!SyslogHostInRegistryOK()) {
			DEBUGSERVICE(Message, (LPTSTR)L"Syslog host info in registry not deemed ok. Will not start without this info.");
			(VOID)service_report_status(SERVICE_STOPPED, ERROR_BAD_NET_NAME, 0);
			return;
		}

		DEBUGSERVICE(Message, (LPTSTR)L"Start reading registry information.");

		//erno: init registry - does not overwrite values. NULL for no syslog IP
		initRegistry(NULL);

		DEBUGSERVICE(Message, (LPTSTR)L"Done initialising registry. Will now read registry information.");
		//erno Check UsePingBeforeSend setting
		ReadSettings(&syslogPort, &backupSyslogPort, &forwardEvents, &EventLogPollInterval);

		DEBUGSERVICE(Message, (LPTSTR)L"Done reading registry information.");

		DEBUGSERVICE(Message, (LPTSTR)L"Calling ntsl initiation.");

		ntsl_init();

		// report the status to the service control manager.
		if (!service_report_status(SERVICE_RUNNING, NO_ERROR, 0))
			return;

		DEBUGSERVICE(Message, (LPTSTR)L"Calling ntsl run.");

		ntsl_run(forwardEvents, EventLogPollInterval);
	}
	__except (CreateMiniDump(GetExceptionInformation()), EXCEPTION_EXECUTE_HANDLER)
	{
	}
}


/*------------------------------[ service_stop ]------------------------------
	* Stops the service.
	*
	* NOTE: If this service will take longer than 3 seconds,
	* spawn a thread to execute the stop code and return.
	* Otherwise the SCM will think the service has stopped responding.
	*----------------------------------------------------------------------------*/
void service_stop()
{
	if (service_stop_event) {
		DEBUGSERVICE(Message, (LPTSTR)L"Registered service_stop_event.");
		service_report_status(SERVICE_STOP_PENDING, NO_ERROR, 2500);
		SetEvent(service_stop_event);
	}
}

/*------------------------------[ service_stop_with_error ]------------------------------
	* Stops the service.
	*
	* NOTE: If this service will take longer than 3 seconds,
	* spawn a thread to execute the stop code and return.
	* Otherwise the SCM will think the service has stopped responding.
	*----------------------------------------------------------------------------*/
void service_stop_with_error(DWORD errorCode, LPTSTR comment) {
	DEBUGSERVICE(Message, (LPTSTR)L"Registered service_stop_with_error. Error code %i. Comment %s.", errorCode, comment);
	service_report_status(SERVICE_STOP_PENDING, errorCode, 2900);
	SetEvent(service_stop_event);
}


LPTSTR GetLastErrorText(LPTSTR lpszBuf, DWORD dwSize)
{
	DWORD dwRet;
	LPTSTR lpszTemp = NULL;

	dwRet = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ARGUMENT_ARRAY,
		NULL,
		GetLastError(),
		LANG_NEUTRAL,
		(LPTSTR)&lpszTemp,
		0,
		NULL);

	// supplied buffer is not long enough
	if (!dwRet || ((long)dwSize < (long)dwRet + 14))
		lpszBuf[0] = TEXT('\0');
	else
	{
		lpszTemp[lstrlen(lpszTemp) - 2] = TEXT('\0');  //remove cr and newline character
		_stprintf_s(lpszBuf, 256, TEXT("%s (0x%x)"), lpszTemp, GetLastError());
	}

	if (lpszTemp)
		LocalFree((HLOCAL)lpszTemp);

	return lpszBuf;
}

void service_addEventSource(wchar_t* path)
{
	HKEY hk;
	DWORD dwData;
	wchar_t  key[256];

	if (path == NULL) return;

	// Add your source name as a subkey under the Application 
	// key in the EventLog registry key. 

	wsprintf(key, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\%s", APP_NAME);
	if (RegCreateKey(HKEY_LOCAL_MACHINE, (LPCTSTR)key, &hk))
		ntsl_die((LPTSTR)L"Could not create the registry key.");


	// Add the name to the EventMessageFile subkey. 
	if (RegSetValueEx(hk,             // subkey handle 
		(LPCTSTR)"EventMessageFile",       // value name 
		0,                        // must be zero 
		REG_EXPAND_SZ,            // value type 
		(LPBYTE)path,           // pointer to value data 
		wcslen(path) + 1))       // length of value data 
		ntsl_die((LPTSTR)L"Could not set the event message file.");

	// Set the supported event types in the TypesSupported subkey. 

	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE |
		EVENTLOG_INFORMATION_TYPE;

	if (RegSetValueEx(hk,      // subkey handle 
		(LPCTSTR)"TypesSupported",  // value name 
		0,                 // must be zero 
		REG_DWORD,         // value type 
		(LPBYTE)&dwData,  // pointer to value data 
		sizeof(DWORD)))    // length of value data 
		ntsl_die((LPTSTR)L"Could not set the supported types.");

	RegCloseKey(hk);
}