
#include <windows.h>
#include <newdev.h>

extern BOOL RescanDevices();

const WCHAR helpMsg[] = TEXT("diinstalldriver <-i inf path> [-y] [-a] [-f]\r\n-i - Specifies the INF file to install\r\n-y - Allows the system to reboot if the driver installation requires it.\r\n-a - Allows the system to ask the user to reboot if the driver installation requires it.\r\n-f - Forces the driver installed, even if there is a better match already installed.");

int WINAPI WinMain(      
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR unused,
    int nCmdShow
)
{
	LPWSTR lpCmdLine = GetCommandLine();
	LPWSTR infPath = NULL, infLock = lpCmdLine;
	DWORD flags = 0;
	int rebootAction = 0;
	while(*infLock != 0)
	{
		if(*infLock == TEXT('-'))
		{
			infLock++;
			switch(*infLock)
			{
			case TEXT('y'):
				rebootAction = 1;
				break;
			case TEXT('a'):
				rebootAction = 2;
				break;
			case TEXT('f'):
				flags |= DIIRFLAG_FORCE_INF;
				break;
			case TEXT('i'):
				{
					if(*(infLock + 1) == TEXT(' '))
						infLock += 2;
					else
						infLock += 1;
					if(*infLock == 0)
						break;
					infPath = infLock;
					DWORD attribs;
					do
					{
						infLock++;
						WCHAR lastChar = *infLock;
						*infLock = 0;
						attribs = GetFileAttributes(infPath);
						*infLock = lastChar;
					} while(*infLock != 0 && (attribs == INVALID_FILE_ATTRIBUTES || attribs & FILE_ATTRIBUTE_DIRECTORY));
					if(attribs == INVALID_FILE_ATTRIBUTES || attribs & FILE_ATTRIBUTE_DIRECTORY)
						infPath = NULL;
					else
					{
						if(*infLock != 0)
						{
							*infLock = 0;
							infLock++;
						}
					}
				}
				break;
			}
		}
		infLock++;
	}

	if(infPath == NULL)
	{
		MessageBox(NULL, helpMsg, TEXT("Invalid Parameters"), MB_OK);
		return 1;
	}
	BOOL rebootDisposition = FALSE;
	BOOL success = DiInstallDriver(NULL, infPath, flags, (rebootAction == 2 ? &rebootDisposition : NULL));
	if(!success)
		return GetLastError();
	RescanDevices();
	if(rebootAction == 1 && rebootDisposition)
		InitiateSystemShutdownEx(NULL, TEXT("Rebooting to complete driver installation."), 0, TRUE, TRUE, SHTDN_REASON_MAJOR_HARDWARE | SHTDN_REASON_MINOR_INSTALLATION);

	return 0;
}