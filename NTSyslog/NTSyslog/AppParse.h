#pragma once

#include "..\Syslogserver\common_registry.h"

void GetOwnIP();


void parseFieldCodes(applSettings *,wchar_t *);
int getLine(FILE *fp,bool unicode, wchar_t *buf,int maxNbr);
int cleanInput(wchar_t *buf, wchar_t *buf2,int numbytes);
CString getLatestLogFileName(CString logPath,CString fileExtension, CString SpecificFile);
int parseMessage(wchar_t*, wchar_t*,applSettings*);
bool testUnicode(wchar_t *filename);
char ernogetc(FILE *fp,bool unicode);

