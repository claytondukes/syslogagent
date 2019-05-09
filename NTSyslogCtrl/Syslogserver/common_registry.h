#pragma once

void  ReadRegKey(UINT *value, UINT DefaultValue, CString Name);
void  ReadRegKey(bool *value, bool DefaultValue, CString Name);
void  ReadRegKey(CString *value, CString DefaultValue, CString Name);

int  WriteRegKey(UINT *value, CString Name);
int  WriteRegKey(bool *value, CString Name);
int  WriteRegKey(CString *value, CString Name);

int GoToRegKey(CString name);
void DeleteKey(CString name);
int GetNextKey(CString *keyName);
int OpenRegistry(CString key);
void  CloseRegistry();

int SystemDSNtype();
void ReadPathFromRegistry(char *connStr,CString ownPath);
void writeServiceDescription();


//required to exist in project calling this file
void logger(int severity,wchar_t *text,...);


//---Registry -----

#define NTSYSLOG_SERVICE_NAME	L"SyslogAgent"
#define NTSYSLOG_DISPLAY_NAME	L"SyslogAgent"

#define EVENTIDFILTERARRAY				L"EventIDFilterList"

#define NTSYSLOG_SOFTWARE_KEY	 L"SOFTWARE\\Datagram\\SyslogAgent"
#define NTSYSLOG_SYSLOG_KEY		 L"SyslogAgent"
#define APPLICATIONLOGS			 L"ApplicationLogs"
#define APPLICATION_SECTION		 L"Application"
#define SECURITY_SECTION		 L"Security"
#define SYSTEM_SECTION			 L"System"

#define EVENTLOG_REG_PATH		_T( "System\\CurrentControlSet\\Services\\EventLog")


#define PRIMARY_SYSLOGD_ENTRY	 L"Syslog"
#define BACKUP_SYSLOGD_ENTRY	 L"Syslog1"

#define FORWARDEVENTLOGS		 L"ForwardEventLogs"
#define FORWARDAPPLICATIONLOGS	 L"ForwardApplicationLogs"
#define FORWARDTOMIRROR			 L"ForwardToMirror"

#define USE_PING_ENTRY			 L"UsePingBeforeSend"
#define TCP_DELIVERY			 L"TCPDelivery"
#define PORT_ENTRY				 L"SendToPort"
#define PORT_BACKUP_ENTRY		 L"SendToBackupPort"
#define EVENTLOG_POLL_INTERVAL	 L"EventLogPollInterval"
#define DATAGRAM_KEY			 L"SOFTWARE\\Datagram"

#define INFORMATION_ENTRY		 L"Information"
#define INFORMATION_PRIORITY	 L"Information Priority" 
#define WARNING_ENTRY			 L"Warning" 
#define WARNING_PRIORITY		 L"Warning Priority" 
#define ERROR_ENTRY				 L"Error"
#define ERROR_PRIORITY			 L"Error Priority" 
#define AUDIT_SUCCESS_ENTRY		 L"Audit Success"
#define AUDIT_SUCCESS_PRIORITY	 L"Audit Success Priority" 
#define AUDIT_FAILURE_ENTRY		 L"Audit Failure"
#define AUDIT_FAILURE_PRIORITY	 L"Audit Failure Priority" 
#define SUCCESS_ENTRY			 L"Success"
#define SUCCESS_PRIORITY		 L"Success Priority"

//erno
#define SEVERITY_INFORMATION	6
#define SEVERITY_WARNING		4
#define SEVERITY_NOTICE			5
//SEVERITY_ERROR defined in WinError.h!
#define mySEVERITY_ERROR			3
#define FACILITY_SECURITYAUTH	4
#define FACILITY_SYSTEM			3
#define FACILITY_LOCAL7			23

#define SYSLOG_SOFTWARE_KEY	_T( "SOFTWARE\\Datagram")
#define SYSLOG_SYSLOG_KEY		_T( "SyslogServer")
#define EVENTLOG_REG_PATH		_T( "System\\CurrentControlSet\\Services\\EventLog")
#define PORT					_T( "Port" )
#define PORTINACTIVE			_T( "PortInactive" )
#define PORTTCP					_T( "PortTCP" )
#define PORTTCPINACTIVE			_T( "PortTCPInactive" )
#define FILESIZE				_T( "Filesize" )
#define DAYSTOKEEPINDB			_T( "NumberOfDaysToKeepInDb" )
#define DAYSTODELETEDB			_T( "NumberOfDaysToDeleteInDb" )
#define DAYSTODELETEBACKUPDB	_T( "NumberOfDaysToDeleteInBackupDb" )
#define DELETEFILE				_T( "DeleteFile" )
#define PERFORMBACKUP			_T( "PerformBackup" )
#define PERFORMDELETEDB			_T( "PerformDeleteDb" )
#define PERFORMDELETEBACKUPDB	_T( "PerformDeleteBackupDb" )
#define LOGPATH					_T( "LogFilePath" )
#define GATEWAY					_T( "MailGateway" )
#define MAILSENDER				_T( "MailSenderAddress" )
#define ADMINISTRATOR			_T( "AdministratorEmail" )
#define DBUSER					_T( "DbUser" )
#define DBPASS					_T( "DbPass" )
#define LICENSEDTO				_T( "LicensedTo" )
#define LICENSE					_T( "License" )
#define LASTBACKUPPERFORMED		_T( "LastBackupPerformed" )
#define ALARMLOOPTIMER			_T( "AlarmLoopTimer" )
#define DNSPURGEINTERVALINMINUTES	_T( "DNSPurgeIntervalInMinutes" )
#define LOOKUPNAMES				_T( "LookupNames" )
#define NEXTOPTIMIZEDATE		_T( "NextOptimizeDate" )
#define OPTIMIZEINTERVAL		_T( "OptimizeInterval" )
#define ADMINREPORTINGLEVEL		_T( "AdminReportingLevel" )

//SyslogAgent Application logging
#define APP_PATH				_T( "Path" )
#define APP_FILENAME			_T( "FileName" )
#define APP_EXT					_T( "FileExtension" )
#define APP_ROTATEFILE			_T( "RotateFileName" )
#define APP_ROTATEDFILE			_T( "RotatedFileName" )
#define APP_PARSE_DATE			_T( "ParseDate" )
#define APP_PARSE_HOST			_T( "ParseHost" )
#define APP_PARSE_SEVERITY		_T( "ParseSeverity" )
#define APP_SEVERITY			_T( "Severity" )
#define APP_PARSE_PROCESS		_T( "ParseProcess" )
#define APP_PROCESS_NAME		_T( "ProcessName" )
#define APP_FACILITY			_T( "Facility" )
#define APP_IGNORE_PREFIX_LINES	_T( "IgnorePrefixLines" )
#define APP_PREFIX				_T( "Prefix" )
#define APP_IGNORE_FIRST_LINES  _T( "IgnoreFirstLines" )
#define APP_NBR_IGNORE_LINES	_T( "NbrIgnoreLines" )
#define APP_UNICODE				_T( "Unicode" )

typedef struct {
	CString DestionationHost;
	CString BackupDestionationHost;
	int DestinationPort;
	int BackupDestinationPort;
	bool ForwardToMirror;
	bool UsePing;
	bool TCPDelivery;
	CString ApplicationName;
	CString logPath;
	CString fileExtension;
	CString SpecificFile;
	CString RotatedFile;
	bool FileRotates;
	bool ParseDate;
	bool ParseHost;
	bool ParseSeverity;
	int SeverityLevel;
	bool ParseProcess;
	CString ProcessName;
	int Facility;
	bool ignorePrefixLines;
	CString prefix;
	bool ignoreFirstLines;
	int NbrIgnoreLines;
	char fieldCodes[32][16];  //32 fields ?16 chars
	bool fieldCodeProcessIsPresent;
	bool failedToParseDate;
	bool failedToParseTime;
	bool failedToParseHost;
	bool failedToParseProcess;
	bool failedToParseSeverity;
	bool ConfirmedUnicodeFormat;
} applSettings;

//RFC3164 messages
//Not defined in common_Syslogproject since syslogagent is c, not cpp.
#define Emergency 0       
#define Alert 1 
#define Critical 2       
#define Error 3       
#define Warning 4       
#define Notice 5 
#define Informational 6       
#define Debug 7       

//Not defined in common_Syslogproject since syslogagent is c, not cpp.
#define MAXBUFLEN 1200  //Max length (rfc3164) 1024 bytes msg plus other headers (time, host...)


//The following 2 also defined in ServiceInstaller project - did not want to connect the 2 projects with dependencies
#define SERVICESTRING			_T( "Description" )
#define SERVICE_REG_PATH		_T( "System\\CurrentControlSet\\Services\\Syslogserver")
