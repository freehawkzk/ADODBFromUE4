
// ADODBFromUE4.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CADODBFromUE4App: 
// �йش����ʵ�֣������ ADODBFromUE4.cpp
//

class CADODBFromUE4App : public CWinApp
{
public:
	CADODBFromUE4App();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CADODBFromUE4App theApp;