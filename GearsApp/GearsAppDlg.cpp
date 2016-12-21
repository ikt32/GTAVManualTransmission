
// GearsAppDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GearsApp.h"
#include "GearsAppDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif



std::string GUID2Str(GUID g) {
	wchar_t szGuidW[40] = { 0 };
	StringFromGUID2(g, szGuidW, 40);
	std::wstring wGuid = szGuidW;
	return std::string(wGuid.begin(), wGuid.end());
}


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGearsAppDlg dialog



CGearsAppDlg::CGearsAppDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_GEARSAPP_DIALOG, pParent),
	logger("./Settings.log"),
	controls(logger),
	settings("./settings_general.ini", "./settings_wheel.ini", logger)
{

	logger.Clear();
	logger.Write("Manual Transmission v4.2.0 - Config utility");
	settings.Read(&controls);

	GUID steerGuid = controls.WheelAxesGUIDs[static_cast<int>(ScriptControls::WheelAxisType::Steer)];

	controls.InitWheel();

	logger.Write("Registered GUIDs: ");

	std::vector<GUID> guids;
	for (auto g : settings.reggdGuids) {
		logger.Write("GUID:   " + GUID2Str(g));
		guids.push_back(g);
	}

	int activeGuids = static_cast<int>(controls.WheelDI.GetGuids().size());
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGearsAppDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CGearsAppDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDOK, &CGearsAppDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// CGearsAppDlg message handlers

BOOL CGearsAppDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGearsAppDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGearsAppDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGearsAppDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}
void CGearsAppDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	
	CDialogEx::OnOK(); // This exits the program. Neat-o.
}
