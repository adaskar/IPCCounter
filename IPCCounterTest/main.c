#include <Windows.h>
#include <stdio.h>
#include <conio.h>

#define DEVICE_DRIVERTEST0 0x8019

#define IOCTL_DRIVERTEST0_INC   \
    CTL_CODE(DEVICE_DRIVERTEST0, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_DRIVERTEST0_DEC	\
    CTL_CODE(DEVICE_DRIVERTEST0, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

int wmain(int argc, wchar_t* argv[])
{
	//LONG l = -5, g_Counter = -5;

	//while (InterlockedDecrement((LONG*)(&l)) > -1) {
	//	InterlockedDecrement(&g_Counter);
	//}
	//InterlockedIncrement((LONG*)(&l));

	//while (InterlockedIncrement((LONG*)(&l)) < 1) {
	//	InterlockedIncrement(&g_Counter);
	//}
	//InterlockedDecrement((LONG*)(&l));



	HANDLE hFile;
	DWORD dw;
	LONG lResult;

	hFile = CreateFileA("\\\\.\\DriverTest0", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"error: 0x%X", GetLastError());
		return 1;
	}

	while (1) {
		int ch = _getche();

		switch (ch) {
		case '+':
			DeviceIoControl(hFile, IOCTL_DRIVERTEST0_INC, NULL, 0, &lResult, sizeof(lResult), &dw, NULL);
			break;
		case '-':
			DeviceIoControl(hFile, IOCTL_DRIVERTEST0_DEC, NULL, 0, &lResult, sizeof(lResult), &dw, NULL);
			break;
		case 'q':
			goto exit;
		}

		wprintf(L"lResult: %d\n", lResult);
	}


exit:
	CloseHandle(hFile);
	return 0;
}