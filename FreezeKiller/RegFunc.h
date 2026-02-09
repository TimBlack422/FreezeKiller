#pragma once

//声明了一些快速操作注册表的函数

#include <Windows.h>

LSTATUS  WriteRegistryValue(
	 HKEY hKey,				//注册表项的句柄
	 PCWSTR Path,			//具体要编辑的子项名称
	 PCWSTR ValueName,		//值的名称
	 ULONG  ValueType,      //值的类型
	 PVOID  ValueData,		//要写入的数据
	 ULONG  ValueLength		//缓冲区大小
);