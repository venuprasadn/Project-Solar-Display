
// SolarDisplayManager.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CSolarDisplayManagerApp:
// See SolarDisplayManager.cpp for the implementation of this class
//

class CSolarDisplayManagerApp : public CWinApp
{
public:
	CSolarDisplayManagerApp();

// Overrides
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

private:
	ULONG_PTR m_gdiplusToken;

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CSolarDisplayManagerApp theApp;
