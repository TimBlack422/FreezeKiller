#include "RegFunc.h"

LSTATUS WriteRegistryValue(
	HKEY hKey,				//注册表项的句柄
	PCWSTR Path,			//具体要编辑的子项名称
	PCWSTR ValueName,		//值的名称
	ULONG  ValueType,       //值的类型
	PVOID  ValueData,		//要写入的数据
	ULONG  ValueLength		//缓冲区大小
)
{
	HKEY h_OpenedKey{ 0 };

	auto status = RegOpenKeyEx(hKey, Path, 0, KEY_ALL_ACCESS, &h_OpenedKey);
	if (status != ERROR_SUCCESS)
		return status;

	status = RegSetValueEx(h_OpenedKey, ValueName, 0, ValueType, (PBYTE)ValueData, ValueLength);
	if (status != ERROR_SUCCESS)
		return status;

	RegCloseKey(h_OpenedKey);
	return status;
}