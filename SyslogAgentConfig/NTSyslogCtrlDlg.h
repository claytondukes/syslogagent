
// NTSyslogCtrlDlg.h : header file
//

#pragma once

#define COMPUTERS_SECTION	_T( "Computers")
#define LAST_COMPUTER_ENTRY	_T( "Last")
#define SYSLOG_AGENT_NAME	_T( "NTSyslog.exe")


// CNTSyslogCtrlDlg dialog
class CNTSyslogCtrlDlg : public CDialogEx
{
// Construction
public:
	CNTSyslogCtrlDlg(CWnd* pParent = nullptr);	// standard constructor
	void OnAppAbout();

// Dialog Data
	enum { IDD = IDD_NTSYSLOGCTRL_DIALOG };
	CStatic m_StatusIcon;

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

protected:
	void SetComputerName();

	//added to reduce redundant coding
	void SetMainDialogControls(int _i_);
	void ReadMachineEventLogSettings(int good_query);
	CStringArray m_csaEventlogSelect;
	DWORD m_csaEventlogSelectSize;

	BOOL DisplayStatus(UINT nIconID, DWORD dwServiceState = 0);
	BOOL QueryServiceStatus();
	HICON m_hIcon;
	CString m_csComputer;

	CString reg_primary;
	CString reg_secondary;

	bool m_deliveryMode; //0=udp, 1=tcp
	bool m_usePing;  //Require m_deliveryMode==0 for impact
	int reg_Port, reg_Port2;

	CString reg_FilterArray;

	bool m_RestartServiceQuestionPlanned;

	UINT_PTR daTimer;

	// Generated message map functions
	//{{AFX_MSG(CNTSyslogCtrlDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnSelectComputer();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//afx_msg void OnSyslogd();
	afx_msg void OnEventLog();
	afx_msg void OnStartService();
	afx_msg void OnStopService();
	//}}AFX_MSG

public:
	afx_msg void OnBnClickedInstall();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedForwardEvents();
	afx_msg void logger(int severity, wchar_t *text, ...);

	afx_msg void OnBnClickedForwardApplication();
	afx_msg void OnBnClickedAddApplication();
	afx_msg void OnBnClickedUseping();
	CListBox m_ApplicationList;
	afx_msg void OnLbnSelchangeApplicationList();
	void UpdateApplicationList(void);
	afx_msg void OnBnClickedRemoveapplication();
	afx_msg void OnBnClickedEditApplication();
	LRESULT OnHelpCommand(WPARAM wParam, LRESULT lParam);

private:
	void CheckRestartService(void);

public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedDelLogFiles();
	afx_msg void OnBnClickedDir();
	afx_msg void OnEnChangeFilterarray();
	afx_msg void OnBnClickedUseMirror();
	afx_msg void OnBnClickedRadioUdp();
	afx_msg void OnBnClickedRadioUdpPing();
	afx_msg void OnBnClickedRadioTcp();

// Implementation
protected:

	DECLARE_MESSAGE_MAP()
};
