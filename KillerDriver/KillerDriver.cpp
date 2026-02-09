#include<ntddk.h>
#include"MajorFunc.h"
#include"UnDocAPI&MyFunc.h"

void DriverUnload(PDRIVER_OBJECT DriverObj);

//这个驱动的一些函数都不能分页
//因为deepfrz等软件对磁盘操作的过滤会导致文件映射内存找不到文件从而拥有蓝屏的可能性


extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING)
{
	DriverObject->DriverUnload = DriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE]	= MajorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]	= MajorCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = MajorDeviceControl;

	UNICODE_STRING deviceName, symbolicLinkName;
	RtlInitUnicodeString(&deviceName, L"\\Device\\FreezeKiller");
	RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\FreezeKiller");


	PDEVICE_OBJECT devObj{ 0 };
	auto status = IoCreateDevice(DriverObject, 0, &deviceName, FILE_DEVICE_UNKNOWN,
		FILE_DEVICE_SECURE_OPEN,FALSE, &devObj);
	if (!NT_SUCCESS(status))
		return STATUS_DEVICE_DOES_NOT_EXIST;

	status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
	if (!NT_SUCCESS(status))
		return STATUS_DEVICE_DOES_NOT_EXIST;


	return STATUS_SUCCESS;
}

void DriverUnload(PDRIVER_OBJECT DriverObj) {
	UNICODE_STRING symbolicLinkName{ 0 };
	RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\FreezeKiller");

	EnableDeepFrz(true);

	IoDeleteSymbolicLink(&symbolicLinkName);
	IoDeleteDevice(DriverObj->DeviceObject);

};


