// signscript.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <Wincrypt.h>
#include <Cryptuiapi.h>

#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")

void HashSomeStuff(LPWSTR contents, LPWSTR stopString, HCRYPTHASH hHash)
{
	LPWSTR buffer = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 1024 * sizeof(WCHAR));
	WORD bufferUsed = 0;
	WORD inQuotes = 0;
	BOOL inWhitespace = FALSE;
	for(DWORD i = 0; i < wcslen(contents); i++)
	{
		if(wcscmp(contents + i, stopString) == 0)
			break;
		if(contents[i] == L'\"')
		{
			if(!inQuotes) inQuotes = 0x1001;
			else inQuotes = 0;
		}
		if(contents[i] == L'\'' && inQuotes != 0x1001)
		{
			if(!inQuotes) inQuotes = 0x0101;
			else inQuotes = 0;
		}
		if(inQuotes && i > 0 && contents[i - 1] == L'\\')
			inQuotes = 0;
	
		if(!inQuotes)
		{
			if(contents[i] == L'/' && contents[i + 1] == L'/')
			{
				while(contents[i] != L'\n' || contents[i] == L'\0')
					i++;
			}
			if(contents[i] == L'/' && contents[i + 1] == L'*')
			{
				while(contents[i] == '\0' || !(contents[i] == L'*' && contents[i + 1] == L'/'))
					i++;
				if(contents[i] != '\0')
					i += 2;
			}
		}

		if(iswgraph(contents[i]))
		{
			inWhitespace = FALSE;
			buffer[bufferUsed++] = contents[i];
		}
		else if(iswspace(contents[i]))
		{
			if(!inWhitespace)
			{
				buffer[bufferUsed++] = L' ';
				inWhitespace = TRUE;
			}
		}

		if(bufferUsed == 1024)
		{
			CryptHashData(hHash, (LPBYTE)buffer, 1024 * sizeof(WCHAR), 0);
			bufferUsed = 0;
			memset(buffer, 0, 1024 * sizeof(WCHAR));
		}
	}
	if(wcslen(buffer) > 0)
	{
		bufferUsed = wcslen(buffer);
		if(buffer[bufferUsed - 1] == L' ' && !inQuotes)
			bufferUsed --;
		CryptHashData(hHash, (LPBYTE)buffer, bufferUsed * sizeof(WCHAR), 0);
	}
	HeapFree(GetProcessHeap(), 0, buffer);
}

int _tmain(int argc, _TCHAR* argv[])
{
	if(argc < 2)
	{
		printf("Must specify certificate store\r\n");
		return 1;
	}
	HCERTSTORE myStore = CertOpenSystemStore(NULL, argv[1]);
	if(myStore == NULL)
	{
		printf("Unable to open certificate store. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}
	PCCERT_CONTEXT pCert = CryptUIDlgSelectCertificateFromStore(myStore, NULL, NULL, NULL, CRYPTUI_SELECT_LOCATION_COLUMN, NULL, NULL);
	CertCloseStore(myStore, 0);
	HCRYPTPROV hProv;
	DWORD keySpec = AT_SIGNATURE;
	BOOL mustFree = TRUE;
	if(!CryptAcquireCertificatePrivateKey(pCert, 0, NULL, &hProv, &keySpec, &mustFree))
	{
		printf("Unable to acquire private key. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}
	CertFreeCertificateContext(pCert);
	printf("Private key encryption context acquired!\r\n");

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hInstance = GetModuleHandle(NULL);
	ofn.lpstrFilter = TEXT("JavaScripts\0*.js\0All Files\0*.*\0\0");
	ofn.lpstrFile = (LPTSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH + 1) * sizeof(TCHAR));
	ofn.nMaxFile = MAX_PATH + 1;
	ofn.lpstrTitle = TEXT("Select script to sign");
	ofn.lpstrDefExt = TEXT("js");
	if(!GetOpenFileName(&ofn))
	{
		printf("Unable to show open file dialog. Error code: %u\r\n", CommDlgExtendedError());
		return CommDlgExtendedError();
	}

	HANDLE scriptFile = CreateFile(ofn.lpstrFile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(scriptFile == INVALID_HANDLE_VALUE)
	{
		printf("Unable to open script for signing. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}
	LARGE_INTEGER scriptFileLength;
	GetFileSizeEx(scriptFile, &scriptFileLength);
	LPWSTR scriptContents = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, scriptFileLength.LowPart + sizeof(WCHAR));
	if(scriptContents == NULL)
	{
		printf("Unable to allocate memory for reading script. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}
	DWORD scriptLength = 0;
	if(!ReadFile(scriptFile, scriptContents, scriptFileLength.LowPart, &scriptLength, NULL))
	{
		printf("Unable to read script for signing. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}
	CloseHandle(scriptFile);

	HCRYPTHASH hHash;
	if(CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash))
		printf("Created hash...\r\n");
	else
	{
		printf("Error creating hash. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}

	BOOL mustUnicodeIt = FALSE;
	if(!IsTextUnicode(scriptContents, scriptLength, NULL))
	{
		LPSTR oldStuff = (LPSTR)scriptContents;
		DWORD fileLength = MultiByteToWideChar(CP_UTF8, 0, oldStuff, scriptLength, NULL, 0);
		scriptContents = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (fileLength + 1)  * sizeof(WCHAR));
		scriptLength = MultiByteToWideChar(CP_UTF8, 0, oldStuff, scriptLength, scriptContents, fileLength);
		HeapFree(GetProcessHeap(), 0, oldStuff);
		mustUnicodeIt = TRUE;
	}

	HashSomeStuff(scriptContents, TEXT("\r\n/* SIG # Begin signature block\r\n"), hHash);
	HeapFree(GetProcessHeap(), 0, scriptContents);

	DWORD sigLength = 0;
	CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &sigLength);
	LPBYTE sigContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, sigLength);
	CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, sigContents, &sigLength);

	DWORD sigStringLength = 0;
	CryptBinaryToStringA(sigContents, sigLength, CRYPT_STRING_BASE64, NULL, &sigStringLength);
	LPSTR sigString = (LPSTR)HeapAlloc(GetProcessHeap(), 0, sigStringLength + 1);
	CryptBinaryToStringA(sigContents, sigLength, CRYPT_STRING_BASE64, sigString, &sigStringLength);
	HeapFree(GetProcessHeap(), 0, sigContents);
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	scriptFile = CreateFile(ofn.lpstrFile, FILE_APPEND_DATA, 0, NULL, OPEN_EXISTING, 0, NULL);
	HeapFree(GetProcessHeap(), 0, ofn.lpstrFile);
	if(scriptFile == INVALID_HANDLE_VALUE)
	{
		printf("Unable to open file for signing. Error code: %u\r\n", GetLastError());
		return GetLastError();
	}

	printf("Signing file...\r\n");
	GetFileSizeEx(scriptFile, &scriptFileLength);
	SetFilePointerEx(scriptFile, scriptFileLength, NULL, FILE_BEGIN);

	LPSTR output = (LPSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sigStringLength + 80);
	sprintf_s(output, sigStringLength + 70, "\r\n/* SIG # Begin signature block\r\n%sSIG # End signature block */\r\n", sigString);

	if(!mustUnicodeIt)
	{
		LPWSTR unicodeOutput = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (sigStringLength + 80) * sizeof(WCHAR));
		MultiByteToWideChar(CP_UTF8, 0, output, -1, unicodeOutput, sigStringLength + 80);
		WriteFile(scriptFile, unicodeOutput, wcslen(unicodeOutput) * sizeof(WCHAR), &sigStringLength, NULL);
		HeapFree(GetProcessHeap(), 0, unicodeOutput);
	}
	else
		WriteFile(scriptFile, output, strlen(output), &sigStringLength, NULL);
	CloseHandle(scriptFile);
	HeapFree(GetProcessHeap(), 0, output);
	HeapFree(GetProcessHeap(), 0, sigString);

	return 0;
}