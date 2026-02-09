#pragma once

#include<ntddk.h>


extern "C"
NTKERNELAPI
NTSTATUS
ObReferenceObjectByName(
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID* Object
);
extern "C" POBJECT_TYPE *IoDeviceObjectType, *IoDriverObjectType;

typedef enum _FIRMWARE_REENTRY
{
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
}FIRMWARE_REENTRY,* PFIRMWARE_REENTRY;

extern "C"
void
NTAPI
HalReturnToFirmware(FIRMWARE_REENTRY Reentry);
//a example
//NTSTATUS status = ObReferenceObjectByName(
//    &uKbdDriverName,
//    OBJ_CASE_INSENSITIVE,
//    NULL,
//    FILE_ALL_ACCESS,
//    *IoDriverObjectType,
//    KernelMode,
//    NULL,
//    (PVOID*)&g_pKdbDriverObj
//);

//以下这两个函数是自己写的函数

//NTSTATUS
//ObReferenceDeviceObjectByName(
//    IN WCHAR* ObjectName,
//    OUT PDEVICE_OBJECT* Object
//);

NTSTATUS
ObReferenceDriverObjectByName(
    IN WCHAR* ObjectName,
    OUT PDRIVER_OBJECT& Object
);

//这里直接用一个链表来记录设备栈
//用hashmap会快点，但是我懒得写了

using DEVICE_STACK_INFOMATION =
struct DEVICE_STACK_INFOMATION
{
    PDEVICE_OBJECT CurrectDevice    = nullptr;
    PDEVICE_OBJECT LowDevice        = nullptr;
    
    PVOID          DeviceExtension  = nullptr;


    DEVICE_STACK_INFOMATION* Next   = nullptr;
};


//查找较低一层的deviceobj
void
LookUpTheLowDevice(DEVICE_STACK_INFOMATION *stack, PDRIVER_OBJECT enumingDriver);

//释放DEVICE_STACK_INFOMATION的内存
void
ReleaseDevStack(DEVICE_STACK_INFOMATION* dsi);