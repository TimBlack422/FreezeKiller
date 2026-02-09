#include<ntddk.h>
#include"UnDocAPI&MyFunc.h"
#include"MajorFunc.h"


NTSTATUS
ObReferenceDriverObjectByName(
    IN WCHAR* ObjectName,
    OUT PDRIVER_OBJECT& Object
) 
{
    UNICODE_STRING driverName{ 0 };
    RtlInitUnicodeString(&driverName, ObjectName);

    auto status = ObReferenceObjectByName(
        &driverName,
        OBJ_CASE_INSENSITIVE,
        nullptr,
        0,
        *IoDriverObjectType,
        KernelMode,
        nullptr,
        (PVOID*)&Object);


    return status;
}

void LookUpTheLowDevice(DEVICE_STACK_INFOMATION *stack,PDRIVER_OBJECT enumingDriver) {

    //这里不想写注释了，将就看吧
    //这样写在执行效率上其实并不好
        for (auto stackDeviceObj = stack; stackDeviceObj;
            stackDeviceObj = stackDeviceObj->Next)
        {
            for (auto currectDevObjOfEnumingDriver = enumingDriver->DeviceObject
                ; currectDevObjOfEnumingDriver
                ;currectDevObjOfEnumingDriver = currectDevObjOfEnumingDriver->NextDevice)
            {
                for (auto currectDevice = currectDevObjOfEnumingDriver,
                    blink=currectDevObjOfEnumingDriver
                    ; currectDevice 
                    ; blink= currectDevice
                    ,currectDevice=currectDevice->AttachedDevice)
                {
                    if (stackDeviceObj->CurrectDevice == currectDevice)
                    {
                        stackDeviceObj->LowDevice = blink;

                        KdPrint(("a filter was found\n"));
                        //这样一写最外层的那个for已经成了修饰作用了
                        if (stackDeviceObj->Next)
                            stackDeviceObj = stackDeviceObj->Next;
                        else return;

                        break;
                    }
                }
            }

        }

}

void ReleaseDevStack(DEVICE_STACK_INFOMATION* dsi) {

    if (dsi)
        ReleaseDevStack(dsi->Next);
    else
        return;
    
    ExFreePool(dsi);
};