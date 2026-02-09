#pragma once

using NTSTATUS = long;

typedef enum _SHUTDOWN_ACTION
{
	ShutdownNoReboot,
	ShutdownReboot,
	ShutdownPowerOff
} SHUTDOWN_ACTION, * PSHUTDOWN_ACTION;

//直通hal，保存数据巴拉巴拉，快速关机
typedef NTSTATUS(__stdcall* NtShutdownSystem1)(SHUTDOWN_ACTION Action);
NtShutdownSystem1 NtShutdownSystem;
