#include <Windows.h>
#include <stdio.h>
#include <conio.h>

#define DEVICE_IPCCOUNTER 0x8019

#define IOCTL_IPCCOUNTER_INC \
    CTL_CODE(DEVICE_IPCCOUNTER, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_IPCCOUNTER_DEC \
    CTL_CODE(DEVICE_IPCCOUNTER, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

int wmain(int argc, wchar_t* argv[])
{
	HANDLE hFile;
	DWORD dw;
	LONG lResult;

	hFile = CreateFileA("\\\\.\\IPCCounter", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		wprintf(L"error: 0x%X", GetLastError());
		return 1;
	}

	while (1) {
		int ch = _getche();

		switch (ch) {
		case '+':
			DeviceIoControl(hFile, IOCTL_IPCCOUNTER_INC, NULL, 0, &lResult, sizeof(lResult), &dw, NULL);
			break;
		case '-':
			DeviceIoControl(hFile, IOCTL_IPCCOUNTER_DEC, NULL, 0, &lResult, sizeof(lResult), &dw, NULL);
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