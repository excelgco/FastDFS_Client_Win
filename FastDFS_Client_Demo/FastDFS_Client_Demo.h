
// FastDFS_Client_Demo.h : PROJECT_NAME 应用程序的主头文件
//

#pragma once

#ifndef __AFXWIN_H__
	#error "在包含此文件之前包含“stdafx.h”以生成 PCH 文件"
#endif

#include "resource.h"		// 主符号


// CFastDFS_Client_DemoApp:
// 有关此类的实现，请参阅 FastDFS_Client_Demo.cpp
//

class CFastDFS_Client_DemoApp : public CWinAppEx
{
public:
	CFastDFS_Client_DemoApp();

// 重写
	public:
	virtual BOOL InitInstance();

// 实现

	DECLARE_MESSAGE_MAP()
};

extern CFastDFS_Client_DemoApp theApp;