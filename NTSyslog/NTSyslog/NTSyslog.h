#ifndef _ERNOSRV_H_
#define _ERNOSRV_H_

#pragma once

void service_start(int argc, char **argv);

void service_stop();
void service_stop_with_error(DWORD errorCode, LPTSTR comment);

int  service_halting();


typedef struct {
	char DebugFilePath[256];
	char DebugDumpFilePath[256];
	char DebugOldDumpFilePath[256];
	bool Debugging;
	bool Debug_Service;
	int DebugServiceIndentation;
	bool Debug_Parse;
	int DebugParseIndentation;
	bool Debug_Appl;
	int DebugApplIndentation;
	bool Debug_Logger;
	int DebugLoggerIndentation;
} DebugFlagsDef;

//Debug indentation
#define EndHeader 1
#define Header 2
#define Title 3
#define Message 4

void DEBUGSERVICE(int indentLevel, wchar_t *a, ...);
void DEBUGPARSE(int indentLevel, wchar_t *a, ...);
void DEBUGAPPLPARSE(int indentLevel, wchar_t *a, ...);
void DEBUGLOGGER(int indentLevel, wchar_t *a, ...);

#endif