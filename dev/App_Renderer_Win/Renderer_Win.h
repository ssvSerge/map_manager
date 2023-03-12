#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CRendererWinApp : public CWinAppEx {
	public:
		CRendererWinApp() noexcept;

	public:
		virtual BOOL InitInstance();
		virtual int  ExitInstance();

		UINT  m_nAppLook;
		BOOL  m_bHiColorIcons;

		virtual void PreLoadState();
		virtual void LoadCustomState();
		virtual void SaveCustomState();

		afx_msg void OnAppAbout();
		afx_msg void OnFileOpen();

		DECLARE_MESSAGE_MAP()
};

extern CRendererWinApp theApp;
