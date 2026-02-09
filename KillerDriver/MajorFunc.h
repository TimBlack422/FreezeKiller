#pragma once

#include<ntddk.h>

NTSTATUS MajorCreateClose(PDEVICE_OBJECT obj, IRP* Irp);

NTSTATUS MajorDeviceControl(PDEVICE_OBJECT obj, IRP* Irp);

NTSTATUS EnableDeepFrz(bool isEnable);//参数true:取消屏蔽(既启用deepfrz)

NTSTATUS HookDispathMajorFunc(PDEVICE_OBJECT obj, IRP* Irp);

NTSTATUS DeleteDeepFrzRegisterKey1();

NTSTATUS RestartByReturnToFirmware();