#include "pch.h"

#include "framework.h"
#include "App.h"
#include "Dlg.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(CAppRenderer, CWinApp)
	ON_COMMAND(ID_HELP, &CWinApp::OnHelp)
END_MESSAGE_MAP()

CAppRenderer theApp;

CAppRenderer::CAppRenderer() {
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

BOOL CAppRenderer::InitInstance() {

	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);

	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	CShellManager *pShellManager = new CShellManager;

	CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));

	SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	CAppRendererDlg dlg;
	m_pMainWnd = &dlg;
	dlg.DoModal();

	if (pShellManager != nullptr) {
		delete pShellManager;
	}

	#if !defined(_AFXDLL) && !defined(_AFX_NO_MFC_CONTROLS_IN_DIALOGS)
		ControlBarCleanUp();
	#endif

	return FALSE;
}

