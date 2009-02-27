#include "stdafx.h"
#include "xprep.h"

LPWSTR LoadFile(LPTSTR fileName)
{
	HANDLE fileHandle;
	LPWSTR finalBuffer = NULL;
	DWORD bufferSize = 0, bufferUsed = 0, actualRead = 0;

	fileHandle = CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fileHandle == NULL)
		return NULL;

	LARGE_INTEGER fsize;
	GetFileSizeEx(fileHandle, &fsize);
	if(fsize.HighPart > 0)
	{
		CloseHandle(fileHandle);
		return NULL;
	}

	finalBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fsize.LowPart + sizeof(WCHAR));
	if(finalBuffer == NULL)
		return NULL;

	ReadFile(fileHandle, finalBuffer, fsize.LowPart, &actualRead, NULL);
	CloseHandle(fileHandle);

	if(!IsTextUnicode(finalBuffer, actualRead, NULL))
	{
		DWORD sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, (LPSTR)finalBuffer, -1, NULL, 0);
		LPWSTR actualReturn = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeNeeded * sizeof(WCHAR));
		MultiByteToWideChar(CP_UTF8, 0, (LPSTR)finalBuffer, -1, actualReturn, sizeNeeded * sizeof(WCHAR));
		HeapFree(GetProcessHeap(), 0, finalBuffer);
		return actualReturn;
	}
	else
		return finalBuffer;
}