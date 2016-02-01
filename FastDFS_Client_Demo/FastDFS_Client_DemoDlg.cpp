
// FastDFS_Client_DemoDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "FastDFS_Client_Demo.h"
#include "FastDFS_Client_DemoDlg.h"

CRITICAL_SECTION m_dirOperation;

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CFastDFS_Client_DemoDlg 对话框




CFastDFS_Client_DemoDlg::CFastDFS_Client_DemoDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CFastDFS_Client_DemoDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFastDFS_Client_DemoDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_LOG, m_lstLog);
}

BEGIN_MESSAGE_MAP(CFastDFS_Client_DemoDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_INITIALIZE_LIB, &CFastDFS_Client_DemoDlg::OnBnClickedInitializeLib)
	ON_BN_CLICKED(IDC_CHOOSE_UPLOAD_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedChooseUploadFile)
	ON_BN_CLICKED(IDC_UPLOAD, &CFastDFS_Client_DemoDlg::OnBnClickedUpload)
	ON_BN_CLICKED(IDC_DOWNLOAD_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedDownloadFile)
	ON_BN_CLICKED(IDC_DELETE_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedDeleteFile)
	ON_BN_CLICKED(IDC_CHOOSE_SLAVE_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedChooseSlaveFile)
	ON_BN_CLICKED(IDC_UPLOAD_SLAVE_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedUploadSlaveFile)
	ON_BN_CLICKED(IDC_UNINITIALIZE, &CFastDFS_Client_DemoDlg::OnBnClickedUninitialize)
	ON_BN_CLICKED(IDC_CREATE_TEST_FILE, &CFastDFS_Client_DemoDlg::OnBnClickedCreateTestFile)
	ON_BN_CLICKED(IDC_UPLOAD_TEST, &CFastDFS_Client_DemoDlg::OnBnClickedUploadTest)
	ON_BN_CLICKED(IDC_DELETE_TEST, &CFastDFS_Client_DemoDlg::OnBnClickedDeleteTest)
	ON_BN_CLICKED(IDC_DOWNLOAD_TEST, &CFastDFS_Client_DemoDlg::OnBnClickedDownloadTest)
	ON_BN_CLICKED(IDC_SEND_REAL, &CFastDFS_Client_DemoDlg::OnBnClickedSendReal)
	ON_BN_CLICKED(IDC_STOP_SEND_REAL, &CFastDFS_Client_DemoDlg::OnBnClickedStopSendReal)
	ON_BN_CLICKED(IDC_QUERY_STAT, &CFastDFS_Client_DemoDlg::OnBnClickedQueryStat)
	ON_BN_CLICKED(IDC_DOWNLOAD_EX, &CFastDFS_Client_DemoDlg::OnBnClickedDownloadEx)
	ON_BN_CLICKED(IDC_DELETE_EX, &CFastDFS_Client_DemoDlg::OnBnClickedDeleteEx)
	ON_BN_CLICKED(IDC_CHECK_CONFIG, &CFastDFS_Client_DemoDlg::OnBnClickedCheckConfig)
END_MESSAGE_MAP()


// CFastDFS_Client_DemoDlg 消息处理程序

BOOL CFastDFS_Client_DemoDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
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

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	InitializeCriticalSection(&m_dirOperation);

	GetDlgItem(IDC_THREAD_NUM)->SetWindowTextA(_T("1"));
	GetDlgItem(IDC_FILE_NUM)->SetWindowTextA(_T("1"));
	GetDlgItem(IDC_FILE_SIZE)->SetWindowTextA(_T("512000"));
	GetDlgItem(IDC_TRACKER_IP)->SetWindowTextA(_T("192.168.114.200"));
	GetDlgItem(IDC_TRACKER_PORT)->SetWindowTextA(_T("22122"));

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CFastDFS_Client_DemoDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CFastDFS_Client_DemoDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CFastDFS_Client_DemoDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

int g_bInit = 0;
void CFastDFS_Client_DemoDlg::OnBnClickedInitializeLib()
{
	// TODO: 在此添加控件通知处理程序代码
	m_hDll = LoadLibrary(_T("FastDFS_Client_Win.dll"));

	m_func_Initialize = (func_Initialize)GetProcAddress(m_hDll, "FDFSC_Initialize");
	m_func_UnInitialize = (func_UnInitialize)GetProcAddress(m_hDll, "FDFSC_UnInitialize");
	m_func_UploadFile = (func_UploadFile)GetProcAddress(m_hDll, "FDFSC_UploadFile");
	m_func_DownloadFile = (func_DownloadFile)GetProcAddress(m_hDll, "FDFSC_DownloadFile");
	m_func_DeleteFile = (func_DeleteFile)GetProcAddress(m_hDll, "FDFSC_DeleteFile");
	m_func_UploadFileByID = (func_UploadFileByID)GetProcAddress(m_hDll, "FDFSC_UploadFileByID");
	m_func_DownloadFileByID = (func_DownloadFileByID)GetProcAddress(m_hDll, "FDFSC_DownloadFileByID");
	m_func_DeleteFileByID = (func_DeleteFileByID)GetProcAddress(m_hDll, "FDFSC_DeleteFileByID");
	m_func_UploadSlaveFileByID = (func_UploadSlaveFileByID)GetProcAddress(m_hDll, "FDFSC_UploadSlaveFileByID");
	m_func_TrackerListGroups = (func_TrackerListGroups)GetProcAddress(m_hDll, "FDFSC_TrackerListGroups");
	m_func_TrackerListStorages = (func_TrackerListStorages)GetProcAddress(m_hDll, "FDFSC_TrackerListStorages");
	m_func_DeleteFileByIDEx = (func_DeleteFileByIDEx)GetProcAddress(m_hDll, "FDFSC_DeleteFileByIDEx");
	m_func_DownloadFileByIDEx = (func_DownloadFileByIDEx)GetProcAddress(m_hDll, "FDFSC_DownloadFileByIDEx");
	m_func_CheckConfiguration = (func_CheckConfiguration)GetProcAddress(m_hDll, "FDFSC_CheckConfiguration");

	UINT32 nTrackerPort = GetDlgItemInt(IDC_TRACKER_PORT);
	TCHAR nTrackerIP[IP_ADDRESS_SIZE];
	GetDlgItem(IDC_TRACKER_IP)->GetWindowTextA(nTrackerIP, sizeof(nTrackerIP));
	ServerAddress addr[2];
	addr[0].nPort = nTrackerPort;
	memcpy(addr[0].szIP, nTrackerIP, strlen(nTrackerIP) + 1);
	UINT32 nRet;
	CString strMsg;
	nRet = m_func_Initialize(&addr[0], 1, 0);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("Initialize Failed"));
	}
	else
	{
		strMsg.Format(_T("Initialize Succeed"));
	}
	m_lstLog.AddString(strMsg);
	g_bInit = 1;
}

void CFastDFS_Client_DemoDlg::OnBnClickedChooseUploadFile()
{
	// TODO: 在此添加控件通知处理程序代码
	
	CString FilePathName;
    CFileDialog dlg(TRUE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
        NULL, 
        NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        (LPCTSTR)_TEXT("JPG Files (*.jpg)|*.jpg|All Files (*.*)|*.*||"),
        NULL);
    if (dlg.DoModal() == IDOK)
    {
        FilePathName = dlg.GetPathName(); //文件名保存在了FilePathName里
		GetDlgItem(IDC_UPLOAD_FILE_PATH)->SetWindowText(FilePathName.GetBuffer(0));
    }
    else
    {
         return;
    }
}

void CFastDFS_Client_DemoDlg::OnBnClickedUpload()
{
	// TODO: 在此添加控件通知处理程序代码
	HANDLE			hFile				= NULL;
	UINT32			nRet;
	TCHAR			szFile1[260]		= {0};
	BOOL			bRet				= FALSE;
	DWORD			dwRead				= 0;
	BYTE			*pbyFile			= NULL;
	int				nFileSize			= 0;
	CString			strMsg;

	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}

	GetDlgItem(IDC_UPLOAD_FILE_PATH)->GetWindowText(szFile1, sizeof(szFile1));
	
	hFile	= CreateFile(szFile1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		strMsg.Format(_T("文件打开失败, 错误号：%d"), GetLastError());
		//m_listBox.AddString((LPCTSTR)strMsg);
		MessageBox(strMsg);
		return;
	}
	nFileSize	= GetFileSize(hFile, NULL);
	if(nFileSize == 0)
	{
		MessageBox(_T("文件大小错误"));
		return;
	}
	pbyFile	= new BYTE[nFileSize];
	bRet	= ReadFile(hFile, pbyFile, nFileSize, &dwRead, NULL);
	if(!bRet || dwRead != nFileSize)
	{
		strMsg.Format(_T("读取文件失败, 错误号：%d"), GetLastError());
		MessageBox(strMsg);
		delete[] pbyFile;
		return;
	}
	CloseHandle(hFile);
	hFile	= NULL;

	BYTE byGroupName[FDFS_GROUP_NAME_MAX_LEN + 1];
	BYTE byRemoteFileName[FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	BYTE byFileExtName[6];
	memcpy(byFileExtName, "jpg", 4);
	nRet = m_func_UploadFile(pbyFile, nFileSize, byFileExtName, byGroupName, byRemoteFileName);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("上传文件失败"));
	}
	else
	{
		strMsg.Format(_T("上传文件成功，获得文件id：%s/%s"), byGroupName, byRemoteFileName);
	}
	m_lstLog.AddString(strMsg);
	delete[] pbyFile;
}

BYTE			byFileBuff[5*1024*1024];
void CFastDFS_Client_DemoDlg::OnBnClickedDownloadFile()
{
	// TODO: 在此添加控件通知处理程序代码
	UINT32			nRet;
	TCHAR			szFileID[260]		= {0};
	BYTE			*pbyFile			= NULL;
	UINT32			nFileSize			= 0;
	CString			strMsg;
	
	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}

	GetDlgItem(IDC_FILE_ID)->GetWindowText(szFileID, sizeof(szFileID));
	
	nRet = m_func_DownloadFileByID((BYTE*)szFileID, byFileBuff, &nFileSize);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("下载文件失败."));
		m_lstLog.AddString(strMsg);
		return;
	}
	
	HANDLE hFile = CreateFile(_TEXT("c:\\12.jpg"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return;
	DWORD dwBytes;
	WriteFile(hFile, byFileBuff, nFileSize, &dwBytes, NULL);
	strMsg.Format(_T("照片写入到c:\12.jpeg文件中."));
	m_lstLog.AddString(strMsg);
	CloseHandle(hFile);
}

void CFastDFS_Client_DemoDlg::OnBnClickedDeleteFile()
{
	// TODO: 在此添加控件通知处理程序代码
	UINT32			nRet;
	TCHAR			szFileID[260]		= {0};
	BYTE			*pbyFile			= NULL;
	UINT32			nFileSize			= 0;
	CString			strMsg;
	
	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}

	GetDlgItem(IDC_FILE_ID)->GetWindowText(szFileID, sizeof(szFileID));
	
	nRet = m_func_DeleteFileByID((BYTE*)szFileID);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("删除文件失败"));
	}
	else
	{
		strMsg.Format(_T("删除文件成功"));
	}
	m_lstLog.AddString(strMsg);
}

void CFastDFS_Client_DemoDlg::OnBnClickedChooseSlaveFile()
{
	// TODO: 在此添加控件通知处理程序代码
	CString FilePathName;
    CFileDialog dlg(TRUE, //TRUE为OPEN对话框，FALSE为SAVE AS对话框
        NULL, 
        NULL,
        OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
        (LPCTSTR)_TEXT("JPG Files (*.jpg)|*.jpg|All Files (*.*)|*.*||"),
        NULL);
    if (dlg.DoModal() == IDOK)
    {
        FilePathName = dlg.GetPathName(); //文件名保存在了FilePathName里
		GetDlgItem(IDC_SLAVE_FILE_PATH)->SetWindowText(FilePathName.GetBuffer(0));
    }
    else
    {
         return;
    }
}

void CFastDFS_Client_DemoDlg::OnBnClickedUploadSlaveFile()
{
	// TODO: 在此添加控件通知处理程序代码
	HANDLE			hFile				= NULL;
	UINT32			nRet;
	TCHAR			szFile1[260]		= {0};
	TCHAR			szMasterGroupName[16 + 1];
	TCHAR			szMasterFileID[128];
	TCHAR			szPrefixName[16];
	BOOL			bRet				= FALSE;
	DWORD			dwRead				= 0;
	BYTE			*pbyFile			= NULL;
	int				nFileSize			= 0;
	CString			strMsg;

	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}

	GetDlgItem(IDC_SLAVE_FILE_PATH)->GetWindowText(szFile1, sizeof(szFile1));
	GetDlgItem(IDC_MASTER_GROUP_NAME)->GetWindowText(szMasterGroupName, sizeof(szMasterGroupName));
	GetDlgItem(IDC_MASTER_FILE_ID)->GetWindowText(szMasterFileID, sizeof(szMasterFileID));
	GetDlgItem(IDC_PREFIX_NAME)->GetWindowText(szPrefixName, sizeof(szPrefixName));
	
	hFile	= CreateFile(szFile1, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		strMsg.Format(_T("从文件打开失败, 错误号：%d"), GetLastError());
		//m_listBox.AddString((LPCTSTR)strMsg);
		MessageBox(strMsg);
		return;
	}
	nFileSize	= GetFileSize(hFile, NULL);
	if(nFileSize == 0)
	{
		MessageBox(_T("从文件大小错误."));
		return;
	}
	pbyFile	= new BYTE[nFileSize];
	bRet	= ReadFile(hFile, pbyFile, nFileSize, &dwRead, NULL);
	if(!bRet || dwRead != nFileSize)
	{
		strMsg.Format(_T("读取从文件失败, 错误号：%d"), GetLastError());
		MessageBox(strMsg);
		delete[] pbyFile;
		return;
	}
	CloseHandle(hFile);
	hFile	= NULL;

	BYTE byFileExtName[6];
	BYTE byFileID[FDFS_GROUP_NAME_MAX_LEN + FDFS_REMOTE_FILE_NAME_MAX_LEN + 1];
	memcpy(byFileExtName, "jpg", 4);
	nRet = m_func_UploadSlaveFileByID(pbyFile, nFileSize, (BYTE*)szMasterGroupName, (BYTE*)szMasterFileID, (BYTE*)szPrefixName, byFileExtName, byFileID);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("上传文件失败"));
	}
	else
	{
		strMsg.Format(_T("上传文件成功，获得文件id：%s"), byFileID);
	}
	m_lstLog.AddString(strMsg);
	delete[] pbyFile;
}

void CFastDFS_Client_DemoDlg::OnBnClickedUninitialize()
{
	// TODO: 在此添加控件通知处理程序代码
	m_func_UnInitialize();
	CString strMsg;
	strMsg.Format(_T("反初始化成功"));
	m_lstLog.AddString(strMsg);
}

void CFastDFS_Client_DemoDlg::OnBnClickedCreateTestFile()
{
	// TODO: 在此添加控件通知处理程序代码
}

typedef struct
{
	int proccess_index;
	int file_num;
	int file_size;
	HANDLE hStart;
	void *pFun;
	void *pFun1;
} ThreadContext;

void CFastDFS_Client_DemoDlg::OnBnClickedUploadTest()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}
	UINT32 nThreadNum = GetDlgItemInt(IDC_THREAD_NUM);
	UINT32 nFileNum = GetDlgItemInt(IDC_FILE_NUM);
	UINT32 nFileSize = GetDlgItemInt(IDC_FILE_SIZE);
	if(nThreadNum <= 0 || nThreadNum > 64)
	{
		MessageBox(_T("线程数不合理"));
		return;
	}
	HANDLE *pThread = new HANDLE[nThreadNum];
	memset(pThread, 0, sizeof(HANDLE) * nThreadNum);
	HANDLE hStart = CreateEvent(NULL, TRUE, FALSE, NULL);

	for(int i = 0; i < nThreadNum; i++)
	{
		ThreadContext *pThreadContext = new ThreadContext();
		pThreadContext->hStart = hStart;
		pThreadContext->proccess_index = i;
		pThreadContext->file_num = nFileNum;
		pThreadContext->file_size = nFileSize;
		pThreadContext->pFun = m_func_UploadFileByID;
		pThreadContext->pFun1 = m_func_UploadSlaveFileByID;
		pThread[i] = CreateThread(NULL, 0, TestUploadThread, pThreadContext, 0, NULL);
		if(pThread[i] == NULL)
		{
			MessageBox(_T("线程数不合理"));
			goto ErrLine;
		}
	}
	Sleep(1000);
	SetEvent(hStart);
	WaitForMultipleObjects(nThreadNum, pThread, TRUE, INFINITE);
ErrLine:
	for(int i = 0; i < nThreadNum; i++)
	{
		if(pThread[i])
		{
			CloseHandle(pThread[i]);
			pThread[i] = 0;
		}
	}
	delete[] pThread;
	m_lstLog.AddString(_T("测试结束，结果保存在c:\\upload文件夹下"));
	return;
}

void CFastDFS_Client_DemoDlg::OnBnClickedDeleteTest()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}
	UINT32 nThreadNum = GetDlgItemInt(IDC_THREAD_NUM);
	UINT32 nFileNum = GetDlgItemInt(IDC_FILE_NUM);
	if(nThreadNum <= 0 || nThreadNum > 64)
	{
		MessageBox(_T("线程数不合理"));
		return;
	}
	HANDLE *pThread = new HANDLE[nThreadNum];
	memset(pThread, 0, sizeof(HANDLE) * nThreadNum);
	HANDLE hStart = CreateEvent(NULL, TRUE, FALSE, NULL);

	for(int i = 0; i < nThreadNum; i++)
	{
		ThreadContext *pThreadContext = new ThreadContext();
		pThreadContext->hStart = hStart;
		pThreadContext->proccess_index = i;
		pThreadContext->file_num = nFileNum;
		pThreadContext->pFun = m_func_DeleteFileByID;
		pThread[i] = CreateThread(NULL, 0, TestDeleteThread, pThreadContext, 0, NULL);
		if(pThread[i] == NULL)
		{
			MessageBox(_T("线程数不合理"));
			goto ErrLine;
		}
	}
	Sleep(1000);
	SetEvent(hStart);
ErrLine:
	WaitForMultipleObjects(nThreadNum, pThread, TRUE, INFINITE);
	for(int i = 0; i < nThreadNum; i++)
	{
		if(pThread[i])
		{
			CloseHandle(pThread[i]);
			pThread[i] = 0;
		}
	}
	m_lstLog.AddString(_T("测试结束，结果保存在c:\\delete文件夹下"));
	delete[] pThread;
	return;
}

DWORD WINAPI CFastDFS_Client_DemoDlg::TestUploadThread(void *pContext)
{
	ThreadContext *pCtx = (ThreadContext*)pContext;
	WaitForSingleObject(pCtx->hStart, INFINITE);
	TestUpload *pUpload = new TestUpload(pCtx->proccess_index, pCtx->file_num, pCtx->file_size);
	pUpload->Run((func_UploadFileByID)pCtx->pFun, (func_UploadSlaveFileByID)pCtx->pFun1);
	delete pUpload;
	delete pCtx;
	return 0;
}

DWORD WINAPI CFastDFS_Client_DemoDlg::SendRealThread(void *pContext)
{
	UINT32 nRet;
	DWORD dwRead;
	HANDLE hFile = CreateFile(_T("c:\\face.jpg"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		printf("文件打开失败");
		return -1;
	}
	UINT32 nFaceSize = GetFileSize(hFile, NULL);
	if(nFaceSize <= 0)
	{
		printf("文件大小错误");
		return -1;
	}
	BYTE *pbyFace = new BYTE[nFaceSize];
	BOOL bRet = ReadFile(hFile, pbyFace, nFaceSize, &dwRead, NULL);
	if(!bRet || dwRead != nFaceSize)
	{
		printf("读取文件失败");
		return -1;
	}
	CloseHandle(hFile);
	hFile = NULL;
	
	hFile = CreateFile(_T("c:\\scene.jpg"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		printf("文件打开失败");
		return -1;
	}
	UINT32 nSceneSize = GetFileSize(hFile, NULL);
	if(nSceneSize <= 0)
	{
		printf("文件大小错误");
		return -1;
	}
	BYTE *pbyScene = new BYTE[nSceneSize];
	bRet = ReadFile(hFile, pbyScene, nSceneSize, &dwRead, NULL);
	if(!bRet || dwRead != nSceneSize)
	{
		printf("读取文件失败");
		return -1;
	}
	CloseHandle(hFile);
	hFile = NULL;
	
	hFile = CreateFile(_T("c:\\compress.jpg"), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		printf("文件打开失败");
		return -1;
	}
	UINT32 nCompressSize = GetFileSize(hFile, NULL);
	if(nCompressSize <= 0)
	{
		printf("文件大小错误");
		return -1;
	}
	BYTE *pbyCompress = new BYTE[nCompressSize];
	bRet = ReadFile(hFile, pbyCompress, nCompressSize, &dwRead, NULL);
	if(!bRet || dwRead != nCompressSize)
	{
		printf("读取文件失败");
		return -1;
	}
	CloseHandle(hFile);
	hFile = NULL;

	BYTE pbyFaceID[256];
	BYTE pbySceneID[256];
	BYTE pbyCompressID[256];
	CFastDFS_Client_DemoDlg *pDlg = (CFastDFS_Client_DemoDlg*)pContext;
	while(1)
	{
		DWORD dwRet = WaitForSingleObject(pDlg->m_hExitEvent, 0);
		if(dwRet == WAIT_OBJECT_0)
			break;
		nRet = pDlg->m_func_UploadFileByID(pbyFace, nFaceSize, (const BYTE*)"jpg", pbyFaceID);
		nRet = pDlg->m_func_UploadFileByID(pbyScene, nSceneSize, (const BYTE*)"jpg", pbySceneID);
		nRet = pDlg->m_func_UploadFileByID(pbyCompress, nCompressSize, (const BYTE*)"jpg", pbyCompressID);
		InterlockedIncrement((long*)&pDlg->m_nTotalUploadCount);
	}

	delete[] pbyFace;
	delete[] pbyScene;
	delete[] pbyCompress;
	return 0;
}

DWORD WINAPI CFastDFS_Client_DemoDlg::TestDeleteThread(void *pContext)
{
	ThreadContext *pCtx = (ThreadContext*)pContext;
	WaitForSingleObject(pCtx->hStart, INFINITE);
	TestDelete *pDelete = new TestDelete(pCtx->proccess_index, pCtx->file_num);
	pDelete->Run((func_DeleteFileByID)pCtx->pFun);
	delete pDelete;
	delete pCtx;
	return 0;
}

DWORD WINAPI CFastDFS_Client_DemoDlg::TestDownloadThread(void *pContext)
{
	ThreadContext *pCtx = (ThreadContext*)pContext;
	WaitForSingleObject(pCtx->hStart, INFINITE);
	TestDownload *pDownload = new TestDownload(pCtx->proccess_index, pCtx->file_num);
	pDownload->Run((func_DownloadFileByID)pCtx->pFun);
	delete pDownload;
	delete pCtx;
	return 0;
}

void CFastDFS_Client_DemoDlg::OnBnClickedDownloadTest()
{
	// TODO: 在此添加控件通知处理程序代码
	if(g_bInit == 0)
	{
		MessageBox(_T("请初始化"));
		return;
	}
	UINT32 nThreadNum = GetDlgItemInt(IDC_THREAD_NUM);
	UINT32 nFileNum = GetDlgItemInt(IDC_FILE_NUM);
	if(nThreadNum <= 0 || nThreadNum > 64)
	{
		MessageBox(_T("线程数不合理"));
		return;
	}
	HANDLE *pThread = new HANDLE[nThreadNum];
	memset(pThread, 0, sizeof(HANDLE) * nThreadNum);
	HANDLE hStart = CreateEvent(NULL, TRUE, FALSE, NULL);

	for(int i = 0; i < nThreadNum; i++)
	{
		ThreadContext *pThreadContext = new ThreadContext();
		pThreadContext->hStart = hStart;
		pThreadContext->proccess_index = i;
		pThreadContext->file_num = nFileNum;
		pThreadContext->pFun = m_func_DownloadFileByID;
		pThread[i] = CreateThread(NULL, 0, TestDownloadThread, pThreadContext, 0, NULL);
		if(pThread[i] == NULL)
		{
			MessageBox(_T("线程数不合理"));
			goto ErrLine;
		}
	}
	Sleep(1000);
	SetEvent(hStart);
ErrLine:
	WaitForMultipleObjects(nThreadNum, pThread, TRUE, INFINITE);
	for(int i = 0; i < nThreadNum; i++)
	{
		if(pThread[i])
		{
			CloseHandle(pThread[i]);
			pThread[i] = 0;
		}
	}
	m_lstLog.AddString(_T("测试结束，结果保存在c:\\download文件夹下"));
	delete[] pThread;
	return;
}

UINT64 now_microseconds(void)
{
	FILETIME ft;
	GetSystemTimeAsFileTime(&ft);
	return (((UINT64)ft.dwHighDateTime << 32) | (UINT64)ft.dwLowDateTime)
		 / 10;
}

void CFastDFS_Client_DemoDlg::OnBnClickedSendReal()
{
	// TODO: 在此添加控件通知处理程序代码
	m_hExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	UINT32 nThreadNum = GetDlgItemInt(IDC_THREAD_NUM);
	m_nTotalUploadCount = 0;
	m_nStart = now_microseconds();
	for(int i = 0; i < nThreadNum; i++)
	{
		CreateThread(NULL, 0, SendRealThread, this, 0, NULL);
	}
}

void CFastDFS_Client_DemoDlg::OnBnClickedStopSendReal()
{
	// TODO: 在此添加控件通知处理程序代码
	SetEvent(m_hExitEvent);
	m_nStop = now_microseconds();
	TCHAR	*pStr	= new TCHAR[256];
	_stprintf_s(pStr, 256, _T("本次上传总量 %d 平均上传速度 %f 张每秒."), m_nTotalUploadCount, (m_nTotalUploadCount) / ((float)(m_nStop - m_nStart) / 1000000));
	MessageBox(pStr);
}

void CFastDFS_Client_DemoDlg::OnBnClickedQueryStat()
{
	// TODO: 在此添加控件通知处理程序代码
	ServerAddress addr;
	GetDlgItem(IDC_QUERY_TRACKER)->GetWindowText(addr.szIP, sizeof(addr.szIP));
	addr.nPort = 22122;
	FDFSGroupStat stat[16];
	UINT32 nCount = 0;
	UINT32 nRet = m_func_TrackerListGroups(&addr, stat, 16, &nCount);
	if(nRet == enumSuccess_FDFS)
	{
		MessageBox(_T("OK"));
	}
	FDFSStorageStat stat1[16];
	nRet = m_func_TrackerListStorages(&addr, _T("g1"), stat1, 16, &nCount);
	if(nRet == enumSuccess_FDFS)
	{
		MessageBox(_T("OK"));
	}
}

void CFastDFS_Client_DemoDlg::OnBnClickedDownloadEx()
{
	// TODO: 在此添加控件通知处理程序代码
	UINT32			nRet;
	TCHAR			szFileID[260]		= {0};
	BYTE			*pbyFile			= NULL;
	UINT32			nFileSize			= 0;
	CString			strMsg;
	m_hDll = LoadLibrary(_T("FastDFS_Client_Win.dll"));

	m_func_Initialize = (func_Initialize)GetProcAddress(m_hDll, "FDFSC_Initialize");
	m_func_UnInitialize = (func_UnInitialize)GetProcAddress(m_hDll, "FDFSC_UnInitialize");
	m_func_UploadFile = (func_UploadFile)GetProcAddress(m_hDll, "FDFSC_UploadFile");
	m_func_DownloadFile = (func_DownloadFile)GetProcAddress(m_hDll, "FDFSC_DownloadFile");
	m_func_DeleteFile = (func_DeleteFile)GetProcAddress(m_hDll, "FDFSC_DeleteFile");
	m_func_UploadFileByID = (func_UploadFileByID)GetProcAddress(m_hDll, "FDFSC_UploadFileByID");
	m_func_DownloadFileByID = (func_DownloadFileByID)GetProcAddress(m_hDll, "FDFSC_DownloadFileByID");
	m_func_DeleteFileByID = (func_DeleteFileByID)GetProcAddress(m_hDll, "FDFSC_DeleteFileByID");
	m_func_UploadSlaveFileByID = (func_UploadSlaveFileByID)GetProcAddress(m_hDll, "FDFSC_UploadSlaveFileByID");
	m_func_TrackerListGroups = (func_TrackerListGroups)GetProcAddress(m_hDll, "FDFSC_TrackerListGroups");
	m_func_TrackerListStorages = (func_TrackerListStorages)GetProcAddress(m_hDll, "FDFSC_TrackerListStorages");
	m_func_DeleteFileByIDEx = (func_DeleteFileByIDEx)GetProcAddress(m_hDll, "FDFSC_DeleteFileByIDEx");
	m_func_DownloadFileByIDEx = (func_DownloadFileByIDEx)GetProcAddress(m_hDll, "FDFSC_DownloadFileByIDEx");

	nRet = m_func_Initialize(NULL, 0, 1);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("初始化失败."));
		m_lstLog.AddString(strMsg);
		return;
	}
	

	GetDlgItem(IDC_FILE_ID)->GetWindowText(szFileID, sizeof(szFileID));
	TCHAR szTrackerIP[IP_ADDRESS_SIZE];
	GetDlgItem(IDC_TRACKER_IP)->GetWindowTextA(szTrackerIP, sizeof(szTrackerIP));
	
	nRet = m_func_DownloadFileByIDEx(szTrackerIP, szFileID, byFileBuff, &nFileSize);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("下载文件失败."));
		m_lstLog.AddString(strMsg);
		return;
	}
	
	HANDLE hFile = CreateFile(_TEXT("c:\\12.jpg"), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return;
	DWORD dwBytes;
	WriteFile(hFile, byFileBuff, nFileSize, &dwBytes, NULL);
	strMsg.Format(_T("照片写入到c:\12.jpeg文件中."));
	m_lstLog.AddString(strMsg);
	CloseHandle(hFile);
	m_func_UnInitialize();
}

void CFastDFS_Client_DemoDlg::OnBnClickedDeleteEx()
{
	// TODO: 在此添加控件通知处理程序代码
	m_hDll = LoadLibrary(_T("FastDFS_Client_Win.dll"));

	m_func_Initialize = (func_Initialize)GetProcAddress(m_hDll, "FDFSC_Initialize");
	m_func_UnInitialize = (func_UnInitialize)GetProcAddress(m_hDll, "FDFSC_UnInitialize");
	m_func_DeleteFileByIDEx = (func_DeleteFileByIDEx)GetProcAddress(m_hDll, "FDFSC_DeleteFileByIDEx");
	UINT32			nRet;
	TCHAR			szFileID[260]		= {0};
	BYTE			*pbyFile			= NULL;
	UINT32			nFileSize			= 0;
	CString			strMsg;
	
	nRet = m_func_Initialize(NULL, 0, 1);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("初始化失败."));
		m_lstLog.AddString(strMsg);
		return;
	}

	GetDlgItem(IDC_FILE_ID)->GetWindowText(szFileID, sizeof(szFileID));
	TCHAR szTrackerIP[IP_ADDRESS_SIZE];
	GetDlgItem(IDC_TRACKER_IP)->GetWindowTextA(szTrackerIP, sizeof(szTrackerIP));
	
	nRet = m_func_DeleteFileByIDEx(szTrackerIP, szFileID);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("删除文件失败"));
	}
	else
	{
		strMsg.Format(_T("删除文件成功"));
	}
	m_lstLog.AddString(strMsg);
}

void CFastDFS_Client_DemoDlg::OnBnClickedCheckConfig()
{
	// TODO: 在此添加控件通知处理程序代码
	UINT32			nRet;
	TCHAR			szTrackerIP[260]	= {0};
	BYTE			*pbyFile			= NULL;
	UINT32			nFileSize			= 0;
	CString			strMsg;

	GetDlgItem(IDC_QUERY_TRACKER)->GetWindowText(szTrackerIP, sizeof(szTrackerIP));
	
	nRet = m_func_CheckConfiguration(szTrackerIP);
	if(nRet != enumSuccess_FDFS)
	{
		strMsg.Format(_T("校验失败"));
		m_lstLog.AddString(strMsg);
	}
	else
	{
		strMsg.Format(_T("校验成功"));
		m_lstLog.AddString(strMsg);
	}
}
