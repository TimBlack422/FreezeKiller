
// FreezeKillerDlg.cpp: 实现文件
//
#include "framework.h"
#include "FreezeKiller.h"
#include "FreezeKillerDlg.h"
#include "afxdialogex.h"
#include <format>
#include <string>
#include "RegFunc.h"
#include "NTAPI.h"
#include "..\KillerDriver\ControlCode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//描述DeepFrz之状态
enum class DeepFrzStatus
{
	Uninstall,		//未安装
	Running,		//正在运行
	Stopped			//(已安装DeepFrz)已经禁用
};

//用户模式下检测DeepFrz是否运行
DeepFrzStatus IsDeepFrzRunningByUser(std::wstring& pathOfDpf);



CFreezeKillerDlg::CFreezeKillerDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_FREEZEKILLER_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

BEGIN_MESSAGE_MAP(CFreezeKillerDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()

	ON_BN_CLICKED(IDC_CHECKTOPMOST, OnClickTopMost)
	ON_BN_CLICKED(IDC_DELETEDEEPFRZ, OnClickDeleteDeepFrz)
	ON_BN_CLICKED(IDC_SETTINGDEEPFRZ, OnClickSettingDeepFrzClick)
	
	ON_WM_CLOSE()
END_MESSAGE_MAP()


// CFreezeKillerDlg 消息处理程序


//这一段函数必须放前面，不然会和后面的串起来，内存管理出现崩坏，导致系统注册表损坏
afx_msg void CFreezeKillerDlg::OnClickDeleteDeepFrz()
{
	if (isDFRuning)
	{
		AfxMessageBox(L"你必须先禁用DeepFreeze，才能彻底删除DeepFreeze", MB_ICONINFORMATION);
		return;
	}

	if (AfxMessageBox(L"事实上，你现在可以直接使用DeepFreeze安装程序卸载DeepFreeze.\n如果使用本程序彻底删除DeepFreeze，这台电脑将再也无法安装DeepFreeze\n你确定要继续吗？\n按下确定按钮，电脑将重启，DeepFreeze将随之被删除",
		MB_ICONINFORMATION | MB_OKCANCEL) == IDCANCEL)
		return;

	//先删除注册表,不做错误处理了，也不用循环了

	REGSAM samDesired = KEY_WOW64_32KEY;

#ifdef _WIN64
	samDesired = KEY_WOW64_64KEY;
#endif // _X86
	// i can use a loop statement here also,but i dont want
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DeepFrz", samDesired, 0);
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DfDiskLo", samDesired, 0);
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DFRegMon", samDesired, 0);
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\FarDisk", samDesired, 0);
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\FarSpace", samDesired, 0);
	RegDeleteKeyEx(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DFServ", samDesired, 0);

	//现在来删除文件
	WCHAR WindowsDirectory[MAX_PATH]{ 0 };

	GetWindowsDirectory(WindowsDirectory, sizeof(WindowsDirectory));

	//先删驱动
	DeleteFile((std::wstring(WindowsDirectory) + L"\\System32\\Drivers\\DeepFrz.sys").c_str());
	DeleteFile((std::wstring(WindowsDirectory) + L"\\System32\\Drivers\\DfDiskLo.sys").c_str());
	DeleteFile((std::wstring(WindowsDirectory) + L"\\System32\\Drivers\\DFRegMon.sys").c_str());
	DeleteFile((std::wstring(WindowsDirectory) + L"\\System32\\Drivers\\FarDisk.sys").c_str());
	DeleteFile((std::wstring(WindowsDirectory) + L"\\System32\\Drivers\\FarSpace.sys").c_str());


	WindowsDirectory[2] = L'\0';

	//Persi0.sys
	std::wstring Directory = (std::wstring(WindowsDirectory) + L"\\Persi0.sys");
	DeleteFile(Directory.c_str());

	ExitWindowsEx(EWX_REBOOT, 0);
}

BOOL CFreezeKillerDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	//MFC用不了RTTI，不愧是成年老物
	m_buttonSettingDeepFrz	= (CButton*) (GetDlgItem(IDC_SETTINGDEEPFRZ));
	m_deleteDeepFrz			= (CButton*)(GetDlgItem(IDC_DELETEDEEPFRZ));
	m_TopMost				= (CButton*)(GetDlgItem(IDC_CHECKTOPMOST));

	m_Edit					= (CEdit*)(GetDlgItem(IDC_EDIT1));

	//检测DeepFrz开启状态
	DeepFrzStatus StatusOfDF{ 0 };
	std::wstring DepFrzVer;
	StatusOfDF = IsDeepFrzRunningByUser(DepFrzVer);
	switch(StatusOfDF){
	case DeepFrzStatus::Running:
		m_ContentOfEdit += L"用户模式下:DeepFreeze服务已经安装且正在运行.\r\n路径为：";
		m_ContentOfEdit += DepFrzVer;
		m_Edit->SetWindowText(m_ContentOfEdit.c_str());

		m_buttonSettingDeepFrz->EnableWindow(1);
		m_deleteDeepFrz->EnableWindow(1);

		isDFRuning = true;
		break;
	case DeepFrzStatus::Uninstall:
		m_ContentOfEdit += L"用户模式下:未检测到DeepFreeze服务\r\n";
		m_Edit->SetWindowText(m_ContentOfEdit.c_str());

		isDFRuning = false;
		break;
	case DeepFrzStatus::Stopped:
		m_ContentOfEdit += L"用户模式下:DeepFreeze(服务)已经被禁用.\r\n路径为：";
		m_ContentOfEdit += DepFrzVer;
		m_Edit->SetWindowText(m_ContentOfEdit.c_str());

		m_buttonSettingDeepFrz->EnableWindow(1);
		m_buttonSettingDeepFrz->SetWindowText(L"启用DeepFreeze");

		m_deleteDeepFrz->EnableWindow(1);

		isDFRuning = false;
	}


	  m_h_MyDriver =
		CreateFile(L"\\\\.\\FreezeKiller", GENERIC_READ | GENERIC_WRITE, 0,
			NULL, OPEN_EXISTING, 0, nullptr);

	  if (m_h_MyDriver == INVALID_HANDLE_VALUE)
	  {
		  AfxMessageBox(L"打开设备失败", MB_ICONERROR);
		  return FALSE;
	  }

	  //获取SE_SHUTDOWN_NAME
	  HANDLE   hToken;
	  if (OpenProcessToken(GetCurrentProcess(),
		  TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
	  {
		  TOKEN_PRIVILEGES   tkp{ 0 };

		  LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);

		  tkp.PrivilegeCount = 1;
		  tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		  AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0);
		  CloseHandle(hToken);
	  }
	  else
	  {
		  AfxMessageBox(L"获取权限失败", MB_ICONERROR);
		  return FALSE;
	  }

	return TRUE;  
}




void CFreezeKillerDlg::OnPaint()
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

		
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


HCURSOR CFreezeKillerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

//用来执行NtShutdownSystem来保存数据的函数
DWORD WINAPI ThreadProc(
	_In_ LPVOID lpParameter
) {
	NtShutdownSystem(ShutdownReboot);
	return 0;
}


afx_msg void CFreezeKillerDlg::OnClickTopMost()
{
	if (m_TopMost->GetCheck() == BST_CHECKED)
		SetWindowPos(&wndTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
	else
		SetWindowPos(&wndNoTopMost, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);;

}

afx_msg void CFreezeKillerDlg::OnClose() {

	if (m_h_MyDriver)
		DeviceIoControl(m_h_MyDriver, ENABLE_DEEPFRZ, nullptr, 0,
			nullptr, 0, nullptr, nullptr);

	CDialog::OnClose();
}


#include <format>
#include <winsvc.h>

#pragma comment(lib,"Version.lib")
DeepFrzStatus IsDeepFrzRunningByUser(std::wstring& pathOfDpf)
{
	pathOfDpf = L"";
	auto h_ScMang = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (h_ScMang == NULL)
		return DeepFrzStatus::Uninstall;

	auto h_DFServ = OpenService(h_ScMang, L"DFServ", SERVICE_ALL_ACCESS);
	if (h_DFServ == NULL)
		return DeepFrzStatus::Uninstall;

	QUERY_SERVICE_CONFIG* lpServiceConfig = nullptr;
	DWORD pcbBytesNeeded = 0;
	auto status =
		QueryServiceConfig(h_DFServ, lpServiceConfig, sizeof(QUERY_SERVICE_CONFIG), &pcbBytesNeeded);
	lpServiceConfig = (QUERY_SERVICE_CONFIG*)new BYTE[pcbBytesNeeded];
	status = QueryServiceConfig(h_DFServ, lpServiceConfig, pcbBytesNeeded, &pcbBytesNeeded);
	if (!status){
		CloseServiceHandle(h_DFServ);
		CloseServiceHandle(h_ScMang);
		return DeepFrzStatus::Uninstall;
	}

	//开始返回路径
	pathOfDpf = lpServiceConfig->lpBinaryPathName;
	CloseServiceHandle(h_DFServ);
	CloseServiceHandle(h_ScMang);


	//检索服务之状态
	if (lpServiceConfig->dwStartType== SERVICE_DISABLED)
	{
		return DeepFrzStatus::Stopped;
	}

	return DeepFrzStatus::Running;
}



//这个也必须放在最后，不然内存管理会失常，注册表会损坏
afx_msg void CFreezeKillerDlg::OnClickSettingDeepFrzClick()
{
	bool status = false;
	if (isDFRuning) {

		if (AfxMessageBox(L"按下确定按钮，你的电脑会在6s内进行第一次重启。\n你保存你在这台电脑上正在编辑的文件，并关闭其他的应用程序\n重启之后，DeepFreeze冻结解除,托盘图标消失.\n你仍然可以使用FreezeKiller重新启用DeepFreeze.\n确定继续吗?", MB_OKCANCEL | MB_ICONQUESTION) == IDCANCEL)
			return;

		status = DeviceIoControl(m_h_MyDriver, DISABLE_DEEPFRZ, nullptr, 0,
			nullptr, 0, nullptr, nullptr);

		if (!status)
		{
			auto lastError = GetLastError();

			AfxMessageBox(std::format(L"操作驱动失败 错误代码 {}", lastError).c_str(), MB_ICONERROR);
			return;
		}

		status = DeviceIoControl(m_h_MyDriver, DELETE_DEEPFRZ_REGISTRY_KEY_1, nullptr, 0,
			nullptr, 0, nullptr, nullptr);
		if (!status)
		{
			auto lastError = GetLastError();

			AfxMessageBox(std::format(L"修改注册表失败，错误代码 {}", lastError).c_str(), MB_ICONERROR);
			status = DeviceIoControl(m_h_MyDriver, ENABLE_DEEPFRZ, nullptr, 0,
				nullptr, 0, nullptr, nullptr);

			return;
		}

		this->SetWindowText(L"正在解除DeepFreeze冻结，你的电脑在此之间会卡顿，这是正常现象。之后你的电脑会重启");
		m_buttonSettingDeepFrz->SetWindowText(L"正在解除冻结......请勿操作电脑");
		//status = DeviceIoControl(m_h_MyDriver, ENABLE_DEEPFRZ, nullptr, 0,
		//	nullptr, 0, nullptr, nullptr);

		NtShutdownSystem = (NtShutdownSystem1)GetProcAddress(GetModuleHandle(L"ntdll.dll"), "NtShutdownSystem");

#ifdef DEBUG
		if (NtShutdownSystem)
			AfxMessageBox(L"已获得函数");
#endif // DEBUG

		//都要重启了，再进行错误检查意义就不大了
		CreateThread(NULL, NULL, ThreadProc, NtShutdownSystem, 0, nullptr);//想用lambda

		Sleep(6 * 1000);
		DeviceIoControl(m_h_MyDriver, Restart_By_Return_To_Firmware, nullptr, 0,
			nullptr, 0, nullptr, nullptr);

		return;

	}

	if (AfxMessageBox(L"你确定要启用DeepFreeze吗？\n按下确定按钮，电脑将重启，DeepFreeze将随之被启用",
		MB_ICONQUESTION | MB_OKCANCEL) == IDCANCEL)
		return;
	//也不用循环了
	// i can use a loop statement here,but i dont want
	// 

	//设备类方面
	WriteRegistryValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\ControlSet001\\Control\\Class\\{4d36e967-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, (PVOID)L"DeepFrz\0partmgr\0", sizeof(L"DeepFrz\0partmgr\0"));

	WriteRegistryValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\ControlSet001\\Control\\Class\\{4d36e967-e325-11ce-bfc1-08002be10318}",
		L"LowerFilters", REG_MULTI_SZ, (PVOID)L"DfDiskLo\0EhStorClass\0", sizeof(L"DfDiskLo\0EhStorClass\0"));


	WriteRegistryValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\ControlSet001\\Control\\Class\\{4d36e96b-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, (PVOID)L"DeepFrz\0kbdclass\0", sizeof(L"DeepFrz\0kbdclass\0"));

	WriteRegistryValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\ControlSet001\\Control\\Class\\{4d36e96f-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, (PVOID)L"DeepFrz\0mouclass\0", sizeof(L"DeepFrz\0mouclass\0"));

	WriteRegistryValue(HKEY_LOCAL_MACHINE,
		L"SYSTEM\\ControlSet001\\Control\\Class\\{71a27cdd-812a-11d0-bec7-08002be2092f}",
		L"UpperFilters", REG_MULTI_SZ, (PVOID)L"DeepFrz\0volsnap\0FarSpace\0", sizeof(L"DeepFrz\0volsnap\0FarSpace\0"));

	//服务方面
	DWORD Data = 0;
	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DeepFrz",
		L"Start", REG_DWORD, &Data, sizeof(Data));

	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DfDiskLo",
		L"Start", REG_DWORD, &Data, sizeof(Data));

	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\FarDisk",
		L"Start", REG_DWORD, &Data, sizeof(Data));

	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\FarSpace",
		L"Start", REG_DWORD, &Data, sizeof(Data));

	Data = 1;
	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DFRegMon",
		L"Start", REG_DWORD, &Data, sizeof(Data));

	Data = 2;
	WriteRegistryValue(HKEY_LOCAL_MACHINE, L"SYSTEM\\ControlSet001\\Services\\DFServ",
		L"Start", REG_DWORD, &Data, sizeof(Data));

#ifdef DEBUG

	AfxMessageBox(L"修改成功注册表");
#endif // DEBUG


	ExitWindowsEx(EWX_REBOOT, 0);

}

