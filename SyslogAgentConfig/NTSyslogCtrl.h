
// NTSyslogCtrl.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

#define CHECK_NOT_ENABLED		0
#define CHECK_INFORMATION		1
#define CHECK_SUCCESS			32  //In windows EVENTLOG_SUCCESS is zero, but that works out bad for filtering purposes. Bastard solution.
#define CHECK_WARNING			2
#define CHECK_ERROR				4
#define CHECK_AUDIT_SUCCESS		8
#define CHECK_AUDIT_FAILURE		16

#define DEFAULT_CHECKS (CHECK_WARNING + CHECK_ERROR + CHECK_AUDIT_FAILURE)
#define ALL_CHECKS (CHECK_INFORMATION + CHECK_WARNING + CHECK_SUCCESS + CHECK_ERROR \
						+ CHECK_AUDIT_SUCCESS + CHECK_AUDIT_FAILURE)

#define DEFAULT_PRIORITY		9

extern "C" {
	void __cdecl initRegistry(char * SyslogAddress);
}

// CNTSyslogCtrlApp:
// See NTSyslogCtrl.cpp for the implementation of this class
//

class CNTSyslogCtrlApp : public CWinApp
{
public:
	CNTSyslogCtrlApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CNTSyslogCtrlApp theApp;
