
// FastDFS_Client_DemoDlg.h : 头文件
//

#pragma once


#include "FastDFS_Client_Win.h"
#include "TestUpload.h"
#include "TestDelete.h"
#include "TestDownload.h"
#include "afxwin.h"
typedef UINT32			(__stdcall * func_Initialize)(ServerAddress *pAddr, UINT32 nAddrCount);
typedef UINT32			(__stdcall * func_UploadFile)(const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyFileExtName, BYTE *pbyGroupName, BYTE *pbyRemoteFileName);
typedef UINT32			(__stdcall * func_DownloadFile)(const BYTE *pbyGroupName, const BYTE *pbyRemoteFileName, BYTE *pbyFileBuff, UINT32 *nFileSize);
typedef UINT32			(__stdcall * func_DeleteFile)(const BYTE *pbyGroupName, const BYTE *pbyRemoteFileName);
typedef UINT32			(__stdcall * func_UploadFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyFileExtName, BYTE *pbyFileID);
typedef UINT32			(__stdcall * func_DownloadFileByID)(const BYTE *pbyFileID, BYTE *pbyFileBuff, UINT32 *nFileSize);
typedef UINT32			(__stdcall * func_DeleteFileByID)(const BYTE *pbyFileID);
typedef UINT32			(__stdcall * func_UploadSlaveFileByID)(const BYTE *pbyFileBuff, UINT32 nFileSize, const BYTE *pbyMasterGroupName, const BYTE *pbyMasterFileName, const BYTE *pbyPrefixName, const BYTE *pbyFileExtName, BYTE *pbyFileID);
typedef void			(__stdcall * func_UnInitialize)();
typedef UINT32			(__stdcall * func_TrackerListGroups)(ServerAddress *pTrackerAddr, FDFSGroupStat *pStat, UINT32 nLen, UINT32 *pnStatCount);
typedef UINT32			(__stdcall * func_TrackerListStorages)(ServerAddress *pTrackerAddr, TCHAR *pszGroupName, FDFSStorageStat *pStat, UINT32 nLen, UINT32 *pnStatCount);
typedef UINT32			(__stdcall * func_DeleteFileByIDEx)(const TCHAR *pszTrackerIPList, const TCHAR *pszFileID);
typedef UINT32			(__stdcall * func_DownloadFileByIDEx)(const TCHAR *pszTrackerIPList, const TCHAR *pszFileID, BYTE *pbyFileBuff, UINT32 *nFileSize);
typedef UINT32			(__stdcall * func_CheckConfiguration)(const TCHAR *pszTrackerIPList);
// CFastDFS_Client_DemoDlg 对话框
class CFastDFS_Client_DemoDlg : public CDialog
{
// 构造
public:
	CFastDFS_Client_DemoDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_FASTDFS_CLIENT_DEMO_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedInitializeLib();

public:
	HMODULE m_hDll;
	func_Initialize m_func_Initialize;
	func_UnInitialize m_func_UnInitialize;
	func_UploadFile m_func_UploadFile;
	func_DownloadFile m_func_DownloadFile;
	func_DeleteFile m_func_DeleteFile;
	func_UploadFileByID m_func_UploadFileByID;
	func_DownloadFileByID m_func_DownloadFileByID;
	func_DeleteFileByID m_func_DeleteFileByID;
	func_UploadSlaveFileByID m_func_UploadSlaveFileByID;
	func_TrackerListGroups m_func_TrackerListGroups;
	func_TrackerListStorages m_func_TrackerListStorages;
	func_DeleteFileByIDEx m_func_DeleteFileByIDEx;
	func_DownloadFileByIDEx m_func_DownloadFileByIDEx;
	func_CheckConfiguration m_func_CheckConfiguration;

	afx_msg void OnBnClickedChooseUploadFile();
	afx_msg void OnBnClickedUpload();
	CListBox m_lstLog;
	afx_msg void OnBnClickedDownloadFile();
	afx_msg void OnBnClickedDeleteFile();
	afx_msg void OnBnClickedChooseSlaveFile();
	afx_msg void OnBnClickedUploadSlaveFile();
	afx_msg void OnBnClickedUninitialize();
	afx_msg void OnBnClickedCreateTestFile();
	afx_msg void OnBnClickedUploadTest();

	afx_msg void OnBnClickedDeleteTest();
	static DWORD WINAPI TestUploadThread(void *pContext);
	static DWORD WINAPI TestDeleteThread(void *pContext);
	static DWORD WINAPI TestDownloadThread(void *pContext);
	static DWORD WINAPI SendRealThread(void *pContext);
	afx_msg void OnBnClickedDownloadTest();
	afx_msg void OnBnClickedSendReal();

	HANDLE m_hExitEvent;
	UINT32 m_nTotalUploadCount;
	UINT64 m_nStart;
	UINT64 m_nStop;
	afx_msg void OnBnClickedStopSendReal();
	afx_msg void OnBnClickedQueryStat();
	afx_msg void OnBnClickedDownloadEx();
	afx_msg void OnBnClickedDeleteEx();
	afx_msg void OnBnClickedCheckConfig();
};
