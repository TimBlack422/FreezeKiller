#include"MajorFunc.h"
#include"ControlCode.h"
#include"UnDocAPI&MyFunc.h"


NTSTATUS MajorCreateClose(PDEVICE_OBJECT obj, IRP* Irp) {
	UNREFERENCED_PARAMETER(obj);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
};


NTSTATUS MajorDeviceControl(PDEVICE_OBJECT obj, IRP* Irp) {
	UNREFERENCED_PARAMETER(obj);

	auto irpStack = IoGetCurrentIrpStackLocation(Irp);
	if (!irpStack) {			//这里有点怕
		Irp->IoStatus.Status = STATUS_INVALID_BUFFER_SIZE;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_INVALID_BUFFER_SIZE;
	}

	auto controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
	
	NTSTATUS status = NULL;
	switch (controlCode) 
	{
	case DISABLE_DEEPFRZ:
		status = EnableDeepFrz(false);
		break;
	case ENABLE_DEEPFRZ:
		status = EnableDeepFrz(true);
		break;
	case DELETE_DEEPFRZ_REGISTRY_KEY_1:
		status = DeleteDeepFrzRegisterKey1();
		break;
	case Restart_By_Return_To_Firmware:
		status = RestartByReturnToFirmware();
		break;
	default:
		Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
		Irp->IoStatus.Information = 0;
		IoCompleteRequest(Irp, IO_NO_INCREMENT);

		return STATUS_NOT_SUPPORTED;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return status;
}


//其他所有的实现函数都在这里写了
//懒得分文件了


//接下来是功能实现函数


//deepfrz有以下几个驱动
// DFRegMon(这个直接UnloadDriver卸载好了) DeepFrz DfDiskLo FarDisk FarSpace
//探索下来是DeepFrz这个drvobj会attach以下几个的drvobj的devobj
// disk volmgr i8042prt mouhid 


bool DisableDeepFrz = false;

//devstack描述
DEVICE_STACK_INFOMATION
* stackOfDeepFrz	= nullptr,
* stackOfDfDiskLo	= nullptr,
* stackOfFarDisk	= nullptr,
* stackOfFarSpace	= nullptr;

//用来存储deepfrz原本的函数
PDRIVER_DISPATCH   OldMajorFunction[3][IRP_MJ_MAXIMUM_FUNCTION + 1];

PDRIVER_OBJECT deepFrzDrvobj = nullptr, dfDiskLoDrvobj = nullptr,
farDiskDrvobj = nullptr, farSpaceDrvObj = nullptr;

NTSTATUS EnableDeepFrz(bool isEnable)
{

	auto status = ObReferenceDriverObjectByName(L"\\driver\\DeepFrz", deepFrzDrvobj);
	status = ObReferenceDriverObjectByName(L"\\driver\\DfDiskLo", dfDiskLoDrvobj);
	//status = ObReferenceDriverObjectByName(L"\\Driver\\FarDisk", &farDiskDrvobj);
	status = ObReferenceDriverObjectByName(L"\\driver\\FarSpace", farSpaceDrvObj);

	KdPrint(("%p  %p	%p	%p", deepFrzDrvobj, dfDiskLoDrvobj, farSpaceDrvObj));

	//farDisk这个驱动可以被直接unload
	if (!(deepFrzDrvobj && dfDiskLoDrvobj /*&& farDiskDrvobj*/ && farSpaceDrvObj))
	{
		return STATUS_NOT_FOUND;
	}

	if (isEnable)			//启用DeepFrz
	{
		if (!DisableDeepFrz)
			return STATUS_SUCCESS;
		// 取消挂钩
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			deepFrzDrvobj->MajorFunction[i] = OldMajorFunction[0][i];
			OldMajorFunction[0][i] = nullptr;
		}
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			dfDiskLoDrvobj->MajorFunction[i] = OldMajorFunction[1][i];
			OldMajorFunction[1][i] = nullptr;
		}
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			farSpaceDrvObj->MajorFunction[i] = OldMajorFunction[2][i];
			OldMajorFunction[2][i] = nullptr;
		}

		ReleaseDevStack(stackOfDeepFrz);
		ReleaseDevStack(stackOfDfDiskLo);
		ReleaseDevStack(stackOfFarDisk);
		ReleaseDevStack(stackOfFarSpace);

		DisableDeepFrz = false;


	}
	else					//停用DeepFrz
	{
		//这两个不做错误处理
		UNICODE_STRING NameOfSomeDriver{ 0 };
		RtlInitUnicodeString(&NameOfSomeDriver, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\FarDisk");
		ZwUnloadDriver(&NameOfSomeDriver);

		RtlInitUnicodeString(&NameOfSomeDriver, L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\DFRegMon");
		ZwUnloadDriver(&NameOfSomeDriver);

		
		if (DisableDeepFrz)
			return STATUS_SUCCESS;

		//这里真的不想写函数了，直接复制粘贴吧
		__try {
			//针对deepFrz设备对象的enum
			if (deepFrzDrvobj->DeviceObject)
			{
				stackOfDeepFrz =
					(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
				RtlZeroMemory(stackOfDeepFrz, sizeof(DEVICE_STACK_INFOMATION));
				stackOfDeepFrz->CurrectDevice = deepFrzDrvobj->DeviceObject;


				auto currectDevstackInfo = stackOfDeepFrz;
				for (auto currectDevobj = deepFrzDrvobj->DeviceObject;
					currectDevobj; currectDevobj = currectDevobj->NextDevice)
				{
					if (currectDevobj->NextDevice)
					{
						currectDevstackInfo->Next =
							(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
						RtlZeroMemory(currectDevstackInfo->Next, sizeof(DEVICE_STACK_INFOMATION));

						currectDevstackInfo->Next->CurrectDevice = currectDevobj->NextDevice;

						currectDevstackInfo = currectDevstackInfo->Next;
					}
				}
			}

			//针对dfDiskLo
			if (dfDiskLoDrvobj->DeviceObject)
			{
				stackOfDfDiskLo =
					(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
				RtlZeroMemory(stackOfDfDiskLo, sizeof(DEVICE_STACK_INFOMATION));
				stackOfDfDiskLo->CurrectDevice = dfDiskLoDrvobj->DeviceObject;


				auto currectDevstackInfo = stackOfDfDiskLo;
				for (auto currectDevobj = dfDiskLoDrvobj->DeviceObject;
					currectDevobj; currectDevobj = currectDevobj->NextDevice)
				{
					if (currectDevobj->NextDevice)
					{
						currectDevstackInfo->Next =
							(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
						RtlZeroMemory(currectDevstackInfo->Next, sizeof(DEVICE_STACK_INFOMATION));

						currectDevstackInfo->Next->CurrectDevice = currectDevobj->NextDevice;

						currectDevstackInfo = currectDevstackInfo->Next;
					}
				}
			}

			//针对FarDisk
			//FarDisk可以被直接卸载
			//这个放到最前

			/*if (farDiskDrvobj->DeviceObject)
			{
				stackOfFarDisk =
					(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
				RtlZeroMemory(stackOfFarDisk, sizeof(DEVICE_STACK_INFOMATION));
				stackOfFarDisk->CurrectDevice = farDiskDrvobj->DeviceObject;


				auto currectDevstackInfo = stackOfFarDisk;
				for (auto currectDevobj = farDiskDrvobj->DeviceObject;
					currectDevobj; currectDevobj = currectDevobj->NextDevice)
				{
					if (currectDevobj->NextDevice)
					{
						currectDevstackInfo->Next =
							(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
						RtlZeroMemory(currectDevstackInfo->Next, sizeof(DEVICE_STACK_INFOMATION));

						currectDevstackInfo->Next->CurrectDevice = currectDevobj->NextDevice;

						currectDevstackInfo = currectDevstackInfo->Next;
					}
				}
			}*/

			//针对FarSpace
			if (farSpaceDrvObj->DeviceObject)
			{
				stackOfFarSpace =
					(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
				RtlZeroMemory(stackOfFarSpace, sizeof(DEVICE_STACK_INFOMATION));
				stackOfFarSpace->CurrectDevice = farSpaceDrvObj->DeviceObject;


				auto currectDevstackInfo = stackOfFarSpace;
				for (auto currectDevobj = farSpaceDrvObj->DeviceObject;
					currectDevobj; currectDevobj = currectDevobj->NextDevice)
				{
					if (currectDevobj->NextDevice)
					{
						currectDevstackInfo->Next =
							(DEVICE_STACK_INFOMATION*)ExAllocatePool(NonPagedPool, sizeof(DEVICE_STACK_INFOMATION));
						RtlZeroMemory(currectDevstackInfo->Next, sizeof(DEVICE_STACK_INFOMATION));

						currectDevstackInfo->Next->CurrectDevice = currectDevobj->NextDevice;

						currectDevstackInfo = currectDevstackInfo->Next;
					}
				}
			}

		}
		__except (1) {
			ObDereferenceObject(deepFrzDrvobj);
			ObDereferenceObject(dfDiskLoDrvobj);
			ObDereferenceObject(farSpaceDrvObj);
			return STATUS_DRIVER_INTERNAL_ERROR;
		}

		//接下来是针对被附加函数的
		//这里的工作量有点大，要包括所有驱动的所有设备对象
		//遍历过程都是重复的，懒得写函数了（最终还是写了）


		PDRIVER_OBJECT enumingDriver = nullptr;

		{
		//先是disk
		status = ObReferenceDriverObjectByName(L"\\Driver\\Disk", enumingDriver);
		if (NT_SUCCESS(status))
		{
			LookUpTheLowDevice(stackOfDeepFrz, enumingDriver);
			//LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);
			LookUpTheLowDevice(stackOfFarSpace, enumingDriver);
		}

		//再是volmgr
		status = ObReferenceDriverObjectByName(L"\\Driver\\volmgr", enumingDriver);
		if (NT_SUCCESS(status))
		{

			LookUpTheLowDevice(stackOfDeepFrz, enumingDriver);
			//LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);
			LookUpTheLowDevice(stackOfFarSpace, enumingDriver);
		}

		//i8042prt
		status = ObReferenceDriverObjectByName(L"\\Driver\\i8042prt", enumingDriver);
		if (NT_SUCCESS(status))
		{
			LookUpTheLowDevice(stackOfDeepFrz, enumingDriver);
			//LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);
			LookUpTheLowDevice(stackOfFarSpace, enumingDriver);
		}

		//mouhid
		status = ObReferenceDriverObjectByName(L"\\Driver\\mouhid", enumingDriver);
		if (NT_SUCCESS(status))
		{
			LookUpTheLowDevice(stackOfDeepFrz, enumingDriver);
			//LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);
			LookUpTheLowDevice(stackOfFarSpace, enumingDriver);
		}


		//还有只针对DfDiskLode
		//先是NVME scsi sata
		status = ObReferenceDriverObjectByName(L"\\Driver\\stornvme", enumingDriver);
		if(NT_SUCCESS(status))
		LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);

		//再来\Driver\atapi ide
		status = ObReferenceDriverObjectByName(L"\\Driver\\atapi", enumingDriver);
		if (NT_SUCCESS(status))
		LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);

		//scsi
		status = ObReferenceDriverObjectByName(L"\\Driver\\LSI_SAS", enumingDriver);
		if (NT_SUCCESS(status))
		LookUpTheLowDevice(stackOfDfDiskLo, enumingDriver);

		//懒得用循环了
		}
		//开始挂钩
		// 
		//这里应该原子地读取和写入
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			OldMajorFunction[0][i] = deepFrzDrvobj->MajorFunction[i];
			deepFrzDrvobj->MajorFunction[i] = HookDispathMajorFunc;
		}
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			OldMajorFunction[1][i] = dfDiskLoDrvobj->MajorFunction[i];
			dfDiskLoDrvobj->MajorFunction[i] = HookDispathMajorFunc;
		}
		for (int i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++)
		{
			OldMajorFunction[2][i] = farSpaceDrvObj->MajorFunction[i];
			farSpaceDrvObj->MajorFunction[i] = HookDispathMajorFunc;
		}
		
		DisableDeepFrz = true;
	}

	ObDereferenceObject(deepFrzDrvobj);
	ObDereferenceObject(dfDiskLoDrvobj);
	ObDereferenceObject(farSpaceDrvObj);



	return STATUS_SUCCESS;
}


//用来hook的函数
NTSTATUS HookDispathMajorFunc(PDEVICE_OBJECT obj, IRP* Irp) {

	const DEVICE_STACK_INFOMATION* stack = nullptr;

	
	//这里没用switchcase了
	if (obj->DriverObject == deepFrzDrvobj)
		stack = stackOfDeepFrz;
	else if(obj->DriverObject == dfDiskLoDrvobj)
		stack = stackOfDfDiskLo;
	else if(obj->DriverObject == farSpaceDrvObj)
		stack = stackOfFarSpace;

	//其实可以直接替换掉devext里面的，但我想先试试效果
	for (auto& i = stack; stack; stack = stack->Next)
	{
		if (obj==(stack->CurrectDevice))
		{
			if (!stack->LowDevice)
			{
				Irp->IoStatus.Status = STATUS_SUCCESS;
				Irp->IoStatus.Information = 0;
				IoCompleteRequest(Irp, IO_NO_INCREMENT);

				//KdPrint(("Completed\n"));

				return STATUS_SUCCESS;
			}

			//KdPrint(("Skiped\n"));

			IoSkipCurrentIrpStackLocation(Irp);
			return IoCallDriver(stack->LowDevice, Irp);
		}
	}
	//在139k的vm下分配4核心不卡
	
	
	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;

}

//用户模式下这两个函数所实现的功能存在问题
//于是乎，上内核

NTSTATUS DeleteDeepFrzRegisterKey1() 
{
	// i can use a loop statement here
	// 不用循环或者函数了

	// ForDiskDrive
	// \Register\HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e967-e325-11ce-bfc1-08002be10318}
	// UpperFilters "DeepFrz partmgr" LowerFilters "DfDiskLo EhStorClass"

	KdPrint(("HelloRegister\n"));

	auto status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Control\\Class\\{4d36e967-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, L"partmgr", sizeof(L"partmgr"));
	KdPrint(("Status:%x", status));
	if (!NT_SUCCESS(status))
		return status;

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Control\\Class\\{4d36e967-e325-11ce-bfc1-08002be10318}",
		L"LowerFilters", REG_MULTI_SZ, L"EhStorClass", sizeof(L"EhStorClass"));
	if (!NT_SUCCESS(status))
		return status;
	KdPrint(("ForDiskDrive\n"));
	
	// ForKeyboard
	// \Register\HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e96b-e325-11ce-bfc1-08002be10318}
	// UpperFilters "DeepFrz kbdclass"

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Control\\Class\\{4d36e96b-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, L"kbdclass", sizeof(L"kbdclass"));
	if (!NT_SUCCESS(status))
		return status;

	KdPrint(("ForKeyBoard\n"));
	// ForMouse 
	// \Register\HKEY_LOCAL_MACHINE\SYSTEM\ControlSet001\Control\Class\{4d36e96f-e325-11ce-bfc1-08002be10318}
	// UpperFilters "DeepFrz mouclass"

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Control\\Class\\{4d36e96f-e325-11ce-bfc1-08002be10318}",
		L"UpperFilters", REG_MULTI_SZ, L"mouclass", sizeof(L"mouclass"));
	if (!NT_SUCCESS(status))
		return status;

	KdPrint(("ForMouse\n"));
	// ForVolume
	// \SYSTEM\ControlSet001\Control\Class\{71a27cdd-812a-11d0-bec7-08002be2092f}
	// UpperFilters "DeepFrz volsnap FarSpace"

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Control\\Class\\{71a27cdd-812a-11d0-bec7-08002be2092f}",
		L"UpperFilters", REG_MULTI_SZ, L"volsnap", sizeof(L"volsnap"));
	if (!NT_SUCCESS(status))
		return status;

	KdPrint(("ForVolume\n"));
	// Service
	// SYSTEM\ControlSet001\Services\DeepFrz
	// DfDiskLo  DFRegMon   FarDisk   FarSpace
	// Start 0  ->3
	// 让编译器来帮我们做循环吧

	DWORD32 Data = 3;
	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\DeepFrz",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\DfDiskLo",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\DFRegMon",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\FarDisk",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\FarSpace",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	Data = 4;
	status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE, L"\\Registry\\Machine\\SYSTEM\\ControlSet001\\Services\\DFServ",
		L"Start", REG_DWORD, &Data, sizeof(Data));
	if (!NT_SUCCESS(status))
		return status;

	return STATUS_SUCCESS;
}

NTSTATUS RestartByReturnToFirmware()
{
	//now,back to bios.

	HalReturnToFirmware(HalRestartRoutine);
	return STATUS_SUCCESS;
}