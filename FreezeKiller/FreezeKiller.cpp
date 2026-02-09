
// FreezeKiller.cpp: 定义应用程序的类行为。
//

#include "framework.h"
#include "FreezeKiller.h"
#include "FreezeKillerDlg.h"


#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// CFreezeKillerApp 构造

CFreezeKillerApp::CFreezeKillerApp()
{

}


CFreezeKillerApp theApp;


bool LoadDriver();

void UnloadDriver();


// CFreezeKillerApp 初始化
BOOL CFreezeKillerApp::InitInstance()
{
	CWinApp::InitInstance();


	//由这里开始初始化
#ifdef _DEBUG
	CFile driverFile;
	driverFile.Open(L"KillerDriver.sys", CFile::modeRead | CFile::modeNoInherit);
	if (driverFile.m_hFile == CFile::hFileNull)
	{
		AfxMessageBox(L"Could not find the driver", MB_ICONERROR);
		return FALSE;
	}
	driverFile.Close();

#else	//release

#endif
	for (int i = 0; i < 10; i++)		//不用这个For会很玄学
	{
		if (!LoadDriver()) {
			if (i == 9)
			{
				AfxMessageBox(L"加载驱动失败", MB_ICONERROR);
				return FALSE;
			}
			UnloadDriver();
			
		}
		else
		{
			break;
		}
	}


	CFreezeKillerDlg dlg;
	m_pMainWnd = &dlg;
	INT_PTR nResponse = dlg.DoModal();


	UnloadDriver();
	return FALSE;
}



#include <winsvc.h>
#include <string>			//都用cpp了就奢侈一把吧

SC_HANDLE g_h_Driver = nullptr;
bool LoadDriver() 
{
	auto h_ScMang = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (h_ScMang == NULL)
		return false;

	std::wstring lpBinaryPathName;

	DWORD sizeOfBuffer = GetCurrentDirectory(0, NULL);
	LPWSTR CurrectDirection = new WCHAR[sizeOfBuffer + 1];

	GetCurrentDirectory(sizeOfBuffer, CurrectDirection);	//不想错误处理了

	lpBinaryPathName = std::wstring(L"\\??\\") + CurrectDirection + L"\\KillerDriver.sys";

	g_h_Driver = OpenService(h_ScMang,L"KillerDrv", GENERIC_ALL);
	if (!g_h_Driver)
	{
		g_h_Driver=
			CreateService(h_ScMang,
			L"KillerDrv",
			L"KillerDrv",
			SERVICE_ALL_ACCESS,
			SERVICE_KERNEL_DRIVER,
			SERVICE_DEMAND_START,
			SERVICE_ERROR_IGNORE,	//SERVICE_ERROR_CRITICAL
			lpBinaryPathName.c_str(),
			NULL,
			NULL,
			NULL,
			NULL,
			NULL);
	}
	
	if (!g_h_Driver)
		return false;


	delete[] CurrectDirection;

	CloseServiceHandle(h_ScMang);

	if (!StartService(g_h_Driver, NULL, NULL))
		return false;

	
	return true;		
}

void UnloadDriver() 
{
	if (!g_h_Driver)
		return;

	SERVICE_STATUS status{ 0 };
	ControlService(g_h_Driver, SERVICE_CONTROL_STOP, &status);


	DeleteService(g_h_Driver);

	CloseServiceHandle(g_h_Driver);

	return;
}
