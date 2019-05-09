#include "..\Syslogserver\common_stdafx.h"
#include "AppWatch.h"
#include "output.h"

#include "winsock2.h"
#include "RegistrySettings.h"
#include "afxtempl.h"
#include <queue>
#include "NTSyslog.h"
#include "errorHandling.h"


using namespace std ;
extern queue<aMess> messQueue;

int GMTimeDiffInHours=0;

extern volatile int LastTransmitOK;

//unicode
//    int _convert; (_convert); UINT _acp = ATL::_AtlGetConversionACP() /*CP_THREAD_ACP*/; (_acp); LPCWSTR _lpw; (_lpw); LPCSTR _lpa; (_lpa);
extern int _convert; 
//(_convert); 
extern UINT _acp; 
//(_acp); 
extern LPCWSTR _lpw; 
//(_lpw); 
extern LPCSTR _lpa; 
//(_lpa);

/********************************************************
initFile --	Init input file	and	ensure access file exists -	otherwise copy
********************************************************/
void initLogFile(_threadData *threadData,applSettings *SettingsPtr) {
	struct __stat64 sb;
	wchar_t buf[1024];
	int i, charNr;

	try {
	//Input file
	//--------------------------------------------------------
//By recommendation Kristopher Heijari	if ((fopen_s(&(threadData->fp),threadData->filename.GetBuffer(),"rb"))!= 0 ||_stat64(threadData->filename.GetBuffer(),&sb)) {

		if (SettingsPtr->ConfirmedUnicodeFormat) {
			threadData->fp= _wfsopen(T2W(threadData->filename.GetBuffer()),L"rb", _SH_DENYNO);
		} else {
			threadData->fp= _fsopen((char*)threadData->filename.GetBuffer(),"rb", _SH_DENYNO);
		}
	if(threadData->fp==NULL ){
		logger(Error, (LPTSTR)L"Failed to open input file, %s. Parsing of this application log failed.",threadData->filename.GetBuffer());
		logger(Error, (LPTSTR)L"Error code %d.",errno);
		AfxEndThread(-1,true);
	}

	DEBUGAPPLPARSE(Informational, (LPTSTR)L"Opened log file %s without errors.",threadData->filename.GetBuffer());

	if(_wstat64(threadData->filename.GetBuffer(),&sb)){
		logger(Error, (LPTSTR)L"Failed to get file info using _stat64, on input file, %s. Parsing of this application log failed.",threadData->filename.GetBuffer());
		logger(Error, (LPTSTR)L"Error code %d.",errno);
		AfxEndThread(-1,true);
	}
	
	if (SettingsPtr->ConfirmedUnicodeFormat) {
		ernogetc(threadData->fp,true); //Eat the FF FE
	}
	if (sb.st_size>(_int64)2147483648) {
		logger(Error, (LPTSTR)L"Error. Input file size larger than 2Gb, which SyslogAgent cannot handle.");
		AfxEndThread(-2,true);
	}

	//Read format, if any
	SettingsPtr->fieldCodes[0][0]='\0'; //reset any existing codes
	for (i=0;i<5;i++) {
		charNr=getLine(threadData->fp,SettingsPtr->ConfirmedUnicodeFormat,buf,sizeof(buf));
		buf[sizeof(buf)-1]='\0'; //getLine returns the full 1024 in case of EOF. WTF!? No harm done as long as a memory walk will not exceed the buffer size
		if(charNr==0)//no more lines?
			break;
		if (buf==wcsstr(buf,L"#Fields:")) { //buf begins with #Fields:
			parseFieldCodes(SettingsPtr,buf);
			break;
		}
	}

	if (fseek(threadData->fp, (long)(threadData->filePosition*(1+(int)SettingsPtr->ConfirmedUnicodeFormat)), SEEK_SET)!= 0) {  //if unicode double since character !=byte
		char daStr[256];
		strerror_s(daStr,sizeof(daStr),errno);
		logger(Error, (LPTSTR)L"Failed to obtain file stats with fseek. %s",daStr);
		AfxEndThread(-2,true);
	}
	if (SettingsPtr->ConfirmedUnicodeFormat) {
		ernogetc(threadData->fp,true); //Read and ignore the FF FE
	}

	if ((threadData->filePosition==0)&(SettingsPtr->ignoreFirstLines==true)) {
		for (i=0;i<SettingsPtr->NbrIgnoreLines;i++) {
			threadData->filePosition+=getLine(threadData->fp,SettingsPtr->ConfirmedUnicodeFormat,buf,sizeof(buf));
		}
	}

	// Keep track of file's previous incarnation.
	threadData->createdTime=sb.st_ctime;

	}
	catch (...) {
		CString mess;
		logger(Error, (LPTSTR)L"Exception in initLogFile. Application logging of %s cannot continue due to exception.",threadData->filename );
		CreateMiniDump(NULL);
		AfxEndThread(0,true);
	}


}

/**********************************************************************
**********************************************************************/
void AppWatch(applSettings *SettingsPtr) {

	aMess data,data2;
	wchar_t * datap=&(data.text[0]);
	wchar_t * dataendp=&(data.text[MAXBUFLEN-1]);
	int ch,i;
	struct __stat64 statbuf;
	//off_t lastsize = 0;
	CString	errorText, newfileName;
	_threadData	threadData;
	bool rotate=SettingsPtr->FileRotates; //just shorter name
	bool RotationJustOccured=false;

	applSettings loggerInformation;

	_set_se_translator(&trans_func);

	try {
	GetOwnIP();

	// Give	logger function prerequisites to contact the server to report errors...
	loggerInformation=*SettingsPtr;

	if (SettingsPtr->SpecificFile!="") {
		if (rotate) {
			DEBUGAPPLPARSE(Informational, (LPTSTR)L"Initiating application logging for %s, using file rotation.",SettingsPtr->ApplicationName);
		} else {
			DEBUGAPPLPARSE(Warning, (LPTSTR)L"Initiating application logging for %s, using specific file %s.",SettingsPtr->ApplicationName,SettingsPtr->SpecificFile);
		}
	}
	else {
		DEBUGAPPLPARSE(Informational, (LPTSTR)L"Initiating application logging for %s, using file time stamps to identify current log file in directory %s.",SettingsPtr->ApplicationName, SettingsPtr->logPath);
	}

RestartLocation:

	while(true) { //Loop until at least one file in directory
		threadData.filename=getLatestLogFileName(SettingsPtr->logPath,SettingsPtr->fileExtension,SettingsPtr->SpecificFile);
		if (threadData.filename!=L"")
			break;
		DEBUGAPPLPARSE(Warning, (LPTSTR)L"No log file identified for logging %s.",SettingsPtr->ApplicationName);
		if (service_halting())
			AfxEndThread(0,true);
		Sleep(2000);
	}
	
	//Go to 65k before the file end - needed since IIS logs do block allocation. Then read until EOF or ascii(0)
	_wstat64(threadData.filename.GetBuffer(),&statbuf);
	threadData.filePosition=statbuf.st_size-65536;
	threadData.filePosition=max(0,threadData.filePosition);
	threadData.filename.ReleaseBuffer();
	initLogFile(&threadData,SettingsPtr);
	if (SettingsPtr->ConfirmedUnicodeFormat) {
		threadData.filePosition++;
	}
	while ((ch = ernogetc(threadData.fp,SettingsPtr->ConfirmedUnicodeFormat)) > 0)  {  //identify eof (-1) or a ascii(0)
		threadData.filePosition++;
	}
	DEBUGAPPLPARSE(Informational, (LPTSTR)L"Initial file position in log file %s is %I64d.",threadData.filename,threadData.filePosition);

	if(rotate) { 
		if (threadData.fp) {
			DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
			fclose(threadData.fp);
			threadData.fp=NULL;
		}
	}

	for (;;) {  //until terminate program

		if (LastTransmitOK==false) {
			logger(Warning, (LPTSTR)L"Last transmit not OK detected in application logging '%s'. Waiting to receive OK status from network.",SettingsPtr->ApplicationName);
			if (threadData.fp) {
				DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
				fclose(threadData.fp);
				threadData.fp=NULL;
			}
			while (LastTransmitOK==false) {  //wait forever until status ok - release locks meanwhile
				if (service_halting())
					AfxEndThread(0,true);
				Sleep(2000);
			}
			//Now connection is ok again, try to resume...
			logger(Informational, (LPTSTR)L"Last transmit status now detected OK in application logging '%s'. Restarting logging...",SettingsPtr->ApplicationName);
			goto RestartLocation;
		}

		//File mode
		//-----------------------------------
		if (rotate) { 
			if (_wstat64(SettingsPtr->SpecificFile.GetBuffer(), &(statbuf)) == 0)  {
				if ((statbuf.st_ctime!= threadData.createdTime||statbuf.st_size<threadData.filePosition)) {
					DEBUGAPPLPARSE(Informational, (LPTSTR)L"Identified log file rotation for application %s.",SettingsPtr->ApplicationName);

					//rotation has occured. Read end of old file then start with the new file
					threadData.filename=SettingsPtr->RotatedFile;
					RotationJustOccured=true;

				}
			}
			SettingsPtr->SpecificFile.ReleaseBuffer();
		} 

		//Open file
		if(rotate) { //Give file rotation a change
			initLogFile(&threadData,SettingsPtr);
		}

		fseek(threadData.fp, (long)(threadData.filePosition*(1+(int)SettingsPtr->ConfirmedUnicodeFormat)), SEEK_SET); //Needed for IIS, due to 65k blocks. Needed regardless of rotation/directory/file mode?
		while ((ch = ernogetc(threadData.fp,SettingsPtr->ConfirmedUnicodeFormat)) != EOF)  {
			
			//IIS logs are allocated by 65k blocks, and filled with zeros, ascii(0)
			if (ch==L'\0') {
				
				break;//ungetc is *not* the solution, since it writes info to the stream. use fseek so reset back to original state.

			}

			threadData.filePosition++;
			*datap++=ch;
			if ((*(datap-1)	== (wchar_t)10)|(datap>=dataendp))	{
				
				if (datap>&(data.text[1])) //must be at least tv?characters written...
					if ((*(datap-2)	== (wchar_t)13)&&(*(datap-1) == (wchar_t)10)) //CR-LF
						*(datap-2)=(wchar_t)0;

				if (!((SettingsPtr->ignorePrefixLines==true)&(data.text[0]==SettingsPtr->prefix))) { //If prefix true, ignore
					cleanInput(&(data.text[0]),&(data2.text[0]),MAXBUFLEN); //fills data2
					parseMessage(&(data2.text[0]),&(data.text[0]),SettingsPtr); //fills data
					DEBUGAPPLPARSE(Informational, (LPTSTR)L"Sending log entry:%s",data.text);

					//send to queue
					insertIntoOutputQueue(&data);

				} else { //found a prefixed line. Is it #Fields: ?
					DEBUGAPPLPARSE(Informational, (LPTSTR)L"Identified prefixed line. Not sending it. Line is %s",data.text);
					if (&(data.text[0])==wcsstr(&(data.text[0]),L"#Fields: "))
						parseFieldCodes(SettingsPtr,&(data.text[0]));
				}


				datap=&(data.text[0]);
			}
		} //end while

		if (ferror(threadData.fp)) {
			char daStr[265];
			strerror_s(daStr,265,errno);
			logger(Debug, (LPTSTR)L"File error in Application logging code. %s",daStr);
			if (threadData.fp!=NULL) {
				DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
				fclose(threadData.fp);
				threadData.fp=NULL;
				Sleep(500); //shutdown in progress?
				initLogFile(&threadData,SettingsPtr);
			}
		}

		if (RotationJustOccured) {
			DEBUGAPPLPARSE(Informational, (LPTSTR)L"Finished reading from rotated log file. Will start reading from new log file.");
			threadData.filename=SettingsPtr->SpecificFile; //now start to read the current file (just read the rotated file)
			if (SettingsPtr->ConfirmedUnicodeFormat) {
				threadData.filePosition=1;
			} else {
				threadData.filePosition=0;
			}
			threadData.createdTime=statbuf.st_ctime;
			RotationJustOccured=false;
			continue; //do not sleep, instead start to read the new file immediately
		}


		if(rotate) { 
			if (threadData.fp) {
				DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
				fclose(threadData.fp);
				threadData.fp=NULL;
			}
		}

		//Directory mode
		if (SettingsPtr->SpecificFile==L"") { //Directory search for new log file

			//Check for possible new file
			newfileName=getLatestLogFileName(SettingsPtr->logPath,SettingsPtr->fileExtension,SettingsPtr->SpecificFile);
			if (newfileName!=threadData.filename) {
				DEBUGAPPLPARSE(Informational, (LPTSTR)L"Identified new log file for application %s. New file is %s.",SettingsPtr->ApplicationName, newfileName);
				if (SettingsPtr->ConfirmedUnicodeFormat) {
					threadData.filePosition=1;
				} else {
					threadData.filePosition=0;
				}
				threadData.createdTime=statbuf.st_ctime;
				DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
				fclose(threadData.fp);
				threadData.filename=newfileName;
				initLogFile(&threadData,SettingsPtr);
			}
		}

		//sleep...
		for (i=0;i<3;i++) {
			if (service_halting()) {
				if(!rotate) { 
					if (threadData.fp) {
						DEBUGAPPLPARSE(Informational, (LPTSTR)L"Closing log file %s.",threadData.filename);
						fclose(threadData.fp);
						threadData.fp=NULL;
						}		
					}
				AfxEndThread(0,true);
			}
			Sleep(1000);
		}

	}

	}
	catch(SE_Exception e) {
		logger(Error, (LPTSTR)L"SEH Exception in AppWatch. Application logging of %s cannot continue due to exception. File being parsed: %s. File position: %d. Error code %u. File dump should be written.",SettingsPtr->ApplicationName,threadData.filename,threadData.filePosition,e.getSeNumber());
		CreateMiniDump(NULL);
		AfxEndThread(0,true);
	}
	catch (...) {
		logger(Error, (LPTSTR)L"Exception in AppWatch. Application logging of %s cannot continue due to exception. File being parsed: %s. File position: %d.",SettingsPtr->ApplicationName,threadData.filename,threadData.filePosition);
		CreateMiniDump(NULL);
		AfxEndThread(0,true);
	}
}

/**********************************************************************
**********************************************************************/
void AppWatchMain()	{
	int i;
	bool forwardApplications;
	applSettings *SettingsPtr;
	applSettings settingsTemplate;
	HANDLE ThreadId;
	CString	keyName;
	CList<CString,CString&>	applList(8);

	_set_se_translator(&trans_func);

	try {
	
	//Read registry main settings, and names of	application logs
	OpenRegistry(NTSYSLOG_SYSLOG_KEY);
	ReadRegKey(&forwardApplications,true,FORWARDAPPLICATIONLOGS);
	ReadRegKey(&(settingsTemplate.TCPDelivery),false,TCP_DELIVERY);
	ReadRegKey(&(settingsTemplate.UsePing),false,USE_PING_ENTRY);
	ReadRegKey((unsigned int*)&(settingsTemplate.DestinationPort),514,PORT_ENTRY);
	ReadRegKey((unsigned int*)&(settingsTemplate.BackupDestinationPort),514,PORT_BACKUP_ENTRY);
	ReadRegKey(&(settingsTemplate.DestionationHost),L"127.0.0.1",PRIMARY_SYSLOGD_ENTRY);
	ReadRegKey(&(settingsTemplate.BackupDestionationHost),L"",BACKUP_SYSLOGD_ENTRY);
	ReadRegKey(&(settingsTemplate.ForwardToMirror),false,FORWARDTOMIRROR);
	if (!(settingsTemplate.ForwardToMirror))
		settingsTemplate.BackupDestionationHost="";
	

	if(GoToRegKey(APPLICATIONLOGS)!=0) {
		logger(Error, (LPTSTR)L"Failed to go to sub registry key.");
		CString mess= CString("Registry access error!");
		applList.AddTail(mess);
	} else {
		while (GetNextKey(&keyName)) {
			applList.AddTail(keyName);
		}
	}
	CloseRegistry();

	//Prepare new ApplicationSettings struct for outputMain
	SettingsPtr= new applSettings;
	*SettingsPtr=settingsTemplate;//contains host and port information

	//Find out time difference between local and GMT
	CTime anyTime=CTime::GetCurrentTime();
	tm timeA,timeB;
	anyTime.GetLocalTm(&timeA);
	anyTime.GetGmtTm(&timeB);
	GMTimeDiffInHours=timeA.tm_hour-timeB.tm_hour;
		
	DEBUGSERVICE(Message, (LPTSTR)L"Application handling init done. Starting output thread....");

	//Start output thread
	ThreadId=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&outputMain,SettingsPtr,0,NULL);	 //Exists if failure
	if (ThreadId==NULL) 
		DEBUGSERVICE(Message, (LPTSTR)L"Failed to start outputMain thread....");

	}
	catch(SE_Exception e) {
		logger(Error, (LPTSTR)L"SEH Exception in AppWatch. Failed initiation. Basic functionality not available - terminating. Error code %u.",e.getSeNumber());
		CreateMiniDump(NULL);
		service_stop_with_error(e.getSeNumber(), (LPTSTR)"SEH Exception in AppWatch. Failed initiation. Basic functionality not available - terminating.");
		AfxEndThread(0,true);
	}
	catch(...) {
		logger(Error, (LPTSTR)L"Exception in AppWatch. Failed initiation. Basic functionality not available - terminating.");
		CreateMiniDump(NULL);
		service_stop_with_error(-1,(LPTSTR)"SEH Exception in AppWatch. Failed initiation. Basic functionality not available - terminating.");
		AfxEndThread(0,true);
	}

	try {
	//Make sure	application log	forwarding isn't disabled
	if (!forwardApplications) {	
		DEBUGSERVICE(Message, (LPTSTR)L"No application logging active - terminating this thread without errors.");
		AfxEndThread(0,true);
	}

	//Launch a thread per application watch
	POSITION pos = applList.GetHeadPosition();
	DEBUGSERVICE(Message, (LPTSTR)L"Application handling starting %d threads...",applList.GetCount());

	for (i=0;i < applList.GetCount();i++){

		//Prepare new ApplicationSettings struct
		SettingsPtr= new applSettings;
		*SettingsPtr=settingsTemplate;//contains host and port information

		SettingsPtr->ApplicationName=applList.GetNext(pos);

		//Read application log settings from registry
		OpenRegistry(NTSYSLOG_SYSLOG_KEY);
		GoToRegKey(APPLICATIONLOGS);
		GoToRegKey(SettingsPtr->ApplicationName);
		
		//Read file source
		ReadRegKey(&(SettingsPtr->logPath),L"",APP_PATH);
		ReadRegKey(&(SettingsPtr->fileExtension),L"",APP_EXT);
		ReadRegKey(&(SettingsPtr->SpecificFile),L"",APP_ROTATEFILE);
		if (SettingsPtr->SpecificFile!="") { //rotation files
			SettingsPtr->FileRotates=true;
			ReadRegKey(&(SettingsPtr->RotatedFile),L"",APP_ROTATEDFILE);
		} else {
			ReadRegKey(&(SettingsPtr->SpecificFile),L"",APP_FILENAME);
			SettingsPtr->FileRotates=false;
		}
		
		ReadRegKey(&(SettingsPtr->ParseDate),false,APP_PARSE_DATE);
		ReadRegKey(&(SettingsPtr->ParseHost),false,APP_PARSE_HOST);
		ReadRegKey(&(SettingsPtr->ConfirmedUnicodeFormat),false,APP_UNICODE);
		ReadRegKey(&(SettingsPtr->ParseSeverity),false,APP_PARSE_SEVERITY);
		ReadRegKey((unsigned int*)&(SettingsPtr->SeverityLevel),6,APP_SEVERITY);
		ReadRegKey(&(SettingsPtr->ParseProcess),false,APP_PARSE_PROCESS);
		ReadRegKey(&(SettingsPtr->ProcessName),L"Process Name",APP_PROCESS_NAME);
		ReadRegKey((unsigned int*)&(SettingsPtr->Facility),22,APP_FACILITY);
		ReadRegKey(&(SettingsPtr->ignorePrefixLines),false,APP_IGNORE_PREFIX_LINES);
		ReadRegKey(&(SettingsPtr->prefix),L"#",APP_PREFIX);	//suitable?
		ReadRegKey(&(SettingsPtr->ignoreFirstLines),false,APP_IGNORE_FIRST_LINES);
		ReadRegKey((unsigned int*)&(SettingsPtr->NbrIgnoreLines),0,APP_NBR_IGNORE_LINES);
		CloseRegistry();

		DEBUGSERVICE(Message, (LPTSTR)L"Application handling: Start thread for %s",SettingsPtr->ApplicationName);
		//Start thread
		ThreadId=CreateThread(NULL,0,(LPTHREAD_START_ROUTINE)&AppWatch,SettingsPtr,0,NULL);	 //Exists if failure
	}
	DEBUGSERVICE(Message, (LPTSTR)L"Application handling: Done starting threads.");
	Sleep(100);

	}
	catch(SE_Exception e) {
		logger(Error, (LPTSTR)L"SEH Exception in AppWatch application logging initiation. Application loggning not functional. Error code %u.",e.getSeNumber());
		CreateMiniDump(NULL);
	}

	catch(...) {
		logger(Message, (LPTSTR)L"Exception in AppWatch application logging initiation. Application loggning not functional.");
		CreateMiniDump(NULL);
	}

	
	}
