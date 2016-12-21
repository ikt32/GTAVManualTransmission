
// GearsAppDlg.h : header file
//

#pragma once
#include <string>
#include "../Gears/ScriptSettings.hpp"
#include "../Gears/Input/ScriptControls.hpp"


// CGearsAppDlg dialog
class CGearsAppDlg : public CDialogEx
{
// Construction
public:
	CGearsAppDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GEARSAPP_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	Logger logger;
	ScriptControls controls;
	ScriptSettings settings;
};
