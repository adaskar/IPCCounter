#include <ntddk.h>

#define BAIL_NT(status) \
	do { \
		if (!NT_SUCCESS(status)) { \
			KdPrint(("Failed with status: (0x%08X). File: %s. Line: %d\n", status, __FILE__, __LINE__)); \
			goto exit; \
		} \
	} while (0)

#define BAIL_MY(s, st) \
	do { \
		if (!(s)) { \
			KdPrint(("Failed with status: (0x%08X). File: %s. Line: %d\n", st, __FILE__, __LINE__)); \
			status = st; \
			goto exit; \
		} \
	} while (0)

#define CompleteIRP(status, info) \
	do { \
		Irp->IoStatus.Status = status; \
		Irp->IoStatus.Information = info; \
		IoCompleteRequest(Irp, IO_NO_INCREMENT); \
		return status; \
	} while (0)

#define DEVICE_IPCCOUNTER 0x8019

#define IOCTL_IPCCOUNTER_INC \
    CTL_CODE(DEVICE_IPCCOUNTER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IPCCOUNTER_DEC \
    CTL_CODE(DEVICE_IPCCOUNTER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

UNICODE_STRING g_DeviceName = RTL_CONSTANT_STRING(L"\\Device\\IPCCounter");
UNICODE_STRING g_DeviceLink = RTL_CONSTANT_STRING(L"\\??\\IPCCounter");

LONG g_Counter;

NTSTATUS
IPCCounterCreate(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	KdPrint(("IPCCounter IPCCounterCreate\n"));

	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(Irp);

	InterlockedExchange((LONG*)(&irpSp->FileObject->FsContext2), 0);

	CompleteIRP(STATUS_SUCCESS, 0);
}

NTSTATUS
IPCCounterClose(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	KdPrint(("IPCCounter IPCCounterClose\n"));

	PIO_STACK_LOCATION irpSp;

	irpSp = IoGetCurrentIrpStackLocation(Irp);
	while (InterlockedDecrement((LONG*)(&irpSp->FileObject->FsContext2)) > -1) {
		InterlockedDecrement(&g_Counter);
	}
	InterlockedIncrement((LONG*)(&irpSp->FileObject->FsContext2));

	while (InterlockedIncrement((LONG*)(&irpSp->FileObject->FsContext2)) < 1) {
		InterlockedIncrement(&g_Counter);
	}
	InterlockedDecrement((LONG*)(&irpSp->FileObject->FsContext2));

	CompleteIRP(STATUS_SUCCESS, 0);
}

NTSTATUS
IPCCounterDeviceControl(
	_In_ struct _DEVICE_OBJECT* DeviceObject,
	_Inout_ struct _IRP* Irp
)
{
	UNREFERENCED_PARAMETER(DeviceObject);
	KdPrint(("IPCCounter IPCCounterDeviceControl\n"));

	NTSTATUS status = STATUS_SUCCESS;
	PIO_STACK_LOCATION irpSp;
	LONG operationResult = 0;

	irpSp = IoGetCurrentIrpStackLocation(Irp);
	BAIL_MY(irpSp->Parameters.DeviceIoControl.OutputBufferLength >= sizeof(LONG), STATUS_BUFFER_TOO_SMALL);

	switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
	case IOCTL_IPCCOUNTER_INC:
		InterlockedIncrement((LONG*)(&irpSp->FileObject->FsContext2));
		operationResult = InterlockedIncrement(&g_Counter);
		break;
	case IOCTL_IPCCOUNTER_DEC:
		InterlockedDecrement((LONG*)(&irpSp->FileObject->FsContext2));
		operationResult = InterlockedDecrement(&g_Counter);
		break;
	}

	*((LONG*)Irp->AssociatedIrp.SystemBuffer) = operationResult;

exit:
	if (!NT_SUCCESS(status)) {
		CompleteIRP(status, 0);
	}

	CompleteIRP(STATUS_SUCCESS, sizeof(LONG));
}

void DriverUnload(
	_In_ PDRIVER_OBJECT DriverObject
)
{
	KdPrint(("IPCCounter DriverUnload\n"));

	IoDeleteSymbolicLink(&g_DeviceLink);
	IoDeleteDevice(DriverObject->DeviceObject);
}

NTSTATUS
DriverEntry(
	_In_ PDRIVER_OBJECT  DriverObject,
	_In_ PUNICODE_STRING RegistryPath
)
{
	UNREFERENCED_PARAMETER(RegistryPath);
	KdPrint(("IPCCounter DriverEntry\n"));

	NTSTATUS status;
	PDEVICE_OBJECT deviceObject = NULL;

	status = IoCreateDevice(
		DriverObject,
		0,
		&g_DeviceName,
		FILE_DEVICE_UNKNOWN,
		0,
		FALSE,
		&deviceObject);
	BAIL_NT(status);

	deviceObject->Flags |= DO_DIRECT_IO;

	status = IoCreateSymbolicLink(&g_DeviceLink, &g_DeviceName);
	BAIL_NT(status);

	DriverObject->DriverUnload = DriverUnload;

	DriverObject->MajorFunction[IRP_MJ_CREATE] = IPCCounterCreate;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = IPCCounterClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IPCCounterDeviceControl;

exit:
	if (!NT_SUCCESS(status)) {
		if (deviceObject) {
			IoDeleteDevice(deviceObject);
		}
	}

	return status;
}