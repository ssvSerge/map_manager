
// App_Remderer.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CAppRenderer:
// See App_Remderer.cpp for the implementation of this class
//

class CAppRenderer : public CWinApp
{
public:
	CAppRenderer();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CAppRenderer theApp;
