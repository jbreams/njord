#include "stdafx.h"
#include "njord.h"
#include <Wincrypt.h>
#pragma comment(lib, "crypt32.lib")

HCRYPTPROV hProv = NULL;
HCRYPTKEY hPubKey = NULL;
BOOL allowUnsignedCode = FALSE;
BOOL promptAfterFailedSigning = FALSE;
BOOL verifyCertificate = FALSE;

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

	LPWSTR returnThis = finalBuffer;
	if(!IsTextUnicode(finalBuffer, actualRead, NULL))
	{
		DWORD sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, (LPSTR)finalBuffer, -1, NULL, 0);
		LPWSTR actualReturn = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeNeeded * sizeof(WCHAR));
		MultiByteToWideChar(CP_UTF8, 0, (LPSTR)finalBuffer, -1, actualReturn, sizeNeeded * sizeof(WCHAR));
		HeapFree(GetProcessHeap(), 0, finalBuffer);
		returnThis = actualReturn;
	}
	if(hProv == NULL)
		return returnThis;

	LPWSTR signatureBlock = wcsstr(returnThis, TEXT("/* SIG # Begin signature block"));
	if(signatureBlock == NULL)
	{
		int messageBoxResult = MessageBox(NULL, TEXT("Signing error.\r\nnJord is configured to require code-signing on loaded nJord scripts. The file specified does not appear to have a signature block.\r\n\r\n Do you want to continue file loading?"), TEXT("nJord Platform"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONSTOP | MB_TOPMOST | MB_SETFOREGROUND);
		if(messageBoxResult == IDNO)
		{
			HeapFree(GetProcessHeap(), 0, returnThis);
			return NULL;
		}
	}

	signatureBlock = wcsstr(signatureBlock, TEXT("\r\n")) + 2;
	DWORD signatureTextLength = (DWORD)wcsstr(signatureBlock, TEXT("SIG # End signature block */"));
	if(signatureTextLength == 0)
		signatureTextLength = wcslen(signatureBlock);
	else
	{
		signatureTextLength -= (DWORD)signatureBlock;
		signatureTextLength /= sizeof(WCHAR);
	}
	DWORD signatureLength = 0;
	if(!CryptStringToBinary(signatureBlock, signatureTextLength, CRYPT_STRING_ANY, NULL, &signatureLength, NULL, NULL))
	{
		HeapFree(GetProcessHeap(), 0, returnThis);
		MessageBox(NULL, TEXT("Unable to parse signature block. Cannot continue with file loading."), TEXT("nJord Platform"), MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
		return NULL;
	}
	LPBYTE signature = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, signatureLength + 1);
	if(!CryptStringToBinary(signatureBlock, signatureTextLength, CRYPT_STRING_ANY, signature, &signatureLength, NULL, NULL))
	{
		HeapFree(GetProcessHeap(), 0, signature);
		HeapFree(GetProcessHeap(), 0, returnThis);
		MessageBox(NULL, TEXT("Unable to parse signature block. Cannot continue with file loading."), TEXT("nJord Platform"), MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
		return NULL;
	}

	HCRYPTHASH hHash;
	CryptCreateHash(hProv, CALG_SHA1, NULL, 0, &hHash);
	HashSomeStuff(returnThis, TEXT("\r\n/* SIG # Begin signature block\r\n"), hHash);
	BOOL ok = CryptVerifySignature(hHash, signature, signatureLength, hPubKey, NULL, 0);
	HeapFree(GetProcessHeap(), 0, signature);
	CryptDestroyHash(hHash);
	if(!ok)
	{
		int messageBoxResult = MessageBox(NULL, TEXT("Signing error.\r\nnJord is configured to require code-signing on loaded nJord scripts. The file specified has failed signature verification and may have been tampered with.\r\nIt is unwise to run code that has failed this test!\r\n\r\n Do you want to continue file loading?"), TEXT("nJord Platform"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONSTOP | MB_TOPMOST | MB_SETFOREGROUND);
		if(messageBoxResult == IDNO)
		{
			HeapFree(GetProcessHeap(), 0, returnThis);
			return NULL;
		}
	}
	return returnThis;
}

BOOL GetCertContextFromRegistry(HKEY securityKey, PCCERT_CONTEXT * pCert)
{
	LSTATUS status = ERROR_SUCCESS;
	*pCert = NULL;

	DWORD type, length;
	if(RegQueryValueEx(securityKey, TEXT("Certificate"), NULL, &type, NULL, &length) != ERROR_SUCCESS)
	{
		RegCloseKey(securityKey);
		return FALSE;
	}

	LPBYTE certContents = NULL;
	if(type == REG_SZ)
	{
		LPWSTR certString = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, length + sizeof(WCHAR));
		status = RegQueryValueExW(securityKey, TEXT("Certificate"), NULL, NULL, (LPBYTE)certString, &length);
		if(status != ERROR_SUCCESS)
		{
			SetLastError(status);
			HeapFree(GetProcessHeap(), 0, certString);
			return FALSE;
		}

		if(!CryptStringToBinary(certString, wcslen(certString), CRYPT_STRING_ANY, NULL, &length, NULL, NULL))
		{
			HeapFree(GetProcessHeap(), 0, certString);
			return FALSE;
		}
		certContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, length);
		if(!CryptStringToBinary(certString, wcslen(certString), CRYPT_STRING_ANY, NULL, &length, NULL, NULL))
		{
			HeapFree(GetProcessHeap(), 0, certString);
			HeapFree(GetProcessHeap(), 0, certContents);
			return FALSE;
		}
		HeapFree(GetProcessHeap(), 0, certString);
	}
	else if(type == REG_BINARY)
	{
		certContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, length + 1);
		status = RegQueryValueEx(securityKey, TEXT("Certificate"), NULL, NULL, certContents, &length);
		if(status != ERROR_SUCCESS)
		{
			SetLastError(status);
			HeapFree(GetProcessHeap(), 0, certContents);
			return FALSE;
		}
	}
	else
	{
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	PCCERT_CONTEXT acquiredCert = CertCreateCertificateContext(X509_ASN_ENCODING, certContents, length);
	HeapFree(GetProcessHeap(), 0, certContents);
	if(acquiredCert == NULL)
		return FALSE;
	*pCert = acquiredCert;
	return TRUE;
}

BOOL GetCertContextFromFile(LPWSTR filePath, PCCERT_CONTEXT * pCert)
{
	DWORD fileLength;
	*pCert = NULL;

	HANDLE certFile = CreateFile(filePath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	HeapFree(GetProcessHeap(), 0, filePath);
	if(certFile == INVALID_HANDLE_VALUE)
		return FALSE;
	LARGE_INTEGER fileSize;
	GetFileSizeEx(certFile, &fileSize);
	LPWSTR certFileContents = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, fileSize.LowPart);
	BOOL ok = ReadFile(certFile, certFileContents, fileSize.LowPart, &fileLength, NULL);
	CloseHandle(certFile);
	if(!ok)
	{
		HeapFree(GetProcessHeap(), 0, certFileContents);
		return FALSE;
	}

	if(!IsTextUnicode(certFileContents, fileLength, NULL))
	{
		LPSTR oldText = (LPSTR)certFileContents;
		DWORD newSize = MultiByteToWideChar(CP_UTF8, 0, oldText, fileLength, NULL, 0);
		certFileContents = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, (newSize + 1) * sizeof(WCHAR));
		fileLength = MultiByteToWideChar(CP_UTF8, 0, oldText, fileLength, certFileContents, newSize);
		HeapFree(GetProcessHeap(), 0, oldText);
	}

	LPBYTE certContents = NULL;
	DWORD certLength = 0;
	if(!CryptStringToBinary(certFileContents, fileLength, CRYPT_STRING_ANY, NULL, &certLength, NULL, NULL))
	{
		HeapFree(GetProcessHeap(), 0, certFileContents);
		return FALSE;
	}
	certContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, certLength + 1);
	ok = CryptStringToBinary(certFileContents, fileLength, CRYPT_STRING_ANY, certContents, &certLength, NULL, NULL);
	HeapFree(GetProcessHeap(), 0, certFileContents);
	if(!ok)
	{
		HeapFree(GetProcessHeap(), 0, certContents);
		return FALSE;
	}

	PCCERT_CONTEXT myCert = CertCreateCertificateContext(X509_ASN_ENCODING, certContents, certLength);
	HeapFree(GetProcessHeap(), 0, certContents);
	if(myCert == NULL)
		return FALSE;
	*pCert = myCert;
	return TRUE;
}

BOOL GetCertContextFromRegFileValue(HKEY securityKey, PCCERT_CONTEXT * pCert)
{
	LPWSTR filePath;
	DWORD fileLength;
	*pCert = NULL;
	if(RegQueryValueEx(securityKey, TEXT("CertificatePath"), NULL, NULL, NULL, &fileLength) != ERROR_SUCCESS)
		return FALSE;
	filePath = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, fileLength + sizeof(WCHAR));
	if(RegQueryValueEx(securityKey, TEXT("CertificatePath"), NULL, NULL, (LPBYTE)filePath, &fileLength) != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, filePath);
		return FALSE;
	}
	return GetCertContextFromFile(filePath, pCert);
}

BOOL GetCertContextFromStore(HKEY securityKey, PCCERT_CONTEXT * pCert)
{
	DWORD bufferLen = (MAX_PATH + 1) * sizeof(WCHAR);
	LPWSTR storeName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, bufferLen);
	DWORD issuerFind = 0;
	DWORD outSize = sizeof(DWORD);
	DWORD storeLocation = 0;
	BOOL ok = TRUE;

	if(RegQueryValueEx(securityKey, TEXT("StoreName"), NULL, NULL, (LPBYTE)storeName, &bufferLen) != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, storeName);
		return FALSE;
	}
	if(RegQueryValueEx(securityKey, TEXT("StoreLocation"), NULL, NULL, (LPBYTE)&storeLocation, &outSize) == ERROR_SUCCESS)
	{
		switch(storeLocation)
		{
		case 0:
		default:
			storeLocation = CERT_SYSTEM_STORE_LOCAL_MACHINE;
			break;
		case 1:
			storeLocation = CERT_SYSTEM_STORE_CURRENT_USER;
			break;
		case 2:
			storeLocation = CERT_SYSTEM_STORE_USERS;
			break;
		case 3:
			storeLocation = CERT_SYSTEM_STORE_SERVICES;
			break;
		}
	}
	else
		storeLocation = CERT_SYSTEM_STORE_LOCAL_MACHINE;
	LPWSTR subjectName = (LPWSTR)HeapAlloc(GetProcessHeap(), 0, bufferLen);
	bufferLen = (MAX_PATH + 1) * sizeof(WCHAR);
	RegQueryValueEx(securityKey, TEXT("MatchIssuer"), NULL, NULL, (LPBYTE)&issuerFind, &outSize);
	if(issuerFind > 0)
	{
		ok = RegQueryValueEx(securityKey, TEXT("IssuerName"), NULL, NULL, (LPBYTE)subjectName, &bufferLen) == ERROR_SUCCESS;
		issuerFind = CERT_FIND_ISSUER_STR;
	}
	else
	{
		ok = RegQueryValueEx(securityKey, TEXT("SubjectName"), NULL, NULL, (LPBYTE)subjectName, &bufferLen) == ERROR_SUCCESS;
		issuerFind = CERT_FIND_SUBJECT_STR;
	}
	if(!ok)
	{
		HeapFree(GetProcessHeap(), 0, storeName);
		HeapFree(GetProcessHeap(), 0, subjectName);
		return FALSE;
	}

	HCERTSTORE myStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, X509_ASN_ENCODING, NULL, storeLocation, storeName);
	HeapFree(GetProcessHeap(), 0, storeName);
	if(myStore == NULL)
	{
		HeapFree(GetProcessHeap(), 0, subjectName);
		return FALSE;
	}

	*pCert = CertFindCertificateInStore(myStore, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, issuerFind, subjectName, NULL);
	CertCloseStore(myStore, 0);
	HeapFree(GetProcessHeap(), 0, subjectName);
	return TRUE;
}

BOOL StartupSigning()
{
	HKEY securityKey;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\nJord\\Security"), 0, KEY_QUERY_VALUE, &securityKey) != ERROR_SUCCESS)
		return TRUE; // If there is no security key, then signing isn't configured.

	LPWSTR nameBuffer = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (MAX_PATH + 1) * sizeof(WCHAR));
	DWORD nameLength = MAX_PATH + 1;
	DWORD index = 0;
	PCCERT_CONTEXT pCert = NULL;
	BOOL ok = TRUE;
	while(RegEnumValue(securityKey, index++, nameBuffer, &nameLength, NULL, NULL, NULL, NULL) != ERROR_NO_MORE_ITEMS)
	{
		if(_wcsicmp(nameBuffer, TEXT("Certificate")) == 0)
		{
			ok = GetCertContextFromRegistry(securityKey, &pCert);
			break;
		}
		else if(_wcsicmp(nameBuffer, TEXT("CertificatePath")) == 0)
		{
			ok = GetCertContextFromRegFileValue(securityKey, &pCert);
			break;
		}
		else if(_wcsicmp(nameBuffer, TEXT("StoreName")) == 0)
		{
			ok = GetCertContextFromStore(securityKey, &pCert);
			break;
		}
		else
		{
			memset(nameBuffer, 0, (MAX_PATH + 1) * sizeof(WCHAR));
			nameLength = MAX_PATH + 1;
		}
	}
	HeapFree(GetProcessHeap(), 0, nameBuffer);
	if(!ok)
	{
		RegCloseKey(securityKey);
		return FALSE;
	}

	nameLength = sizeof(DWORD);
	index = 0;
	if(RegQueryValueEx(securityKey, TEXT("AllowUnsignedCode"), NULL, NULL, (LPBYTE)&index, &nameLength) == ERROR_SUCCESS)
		allowUnsignedCode = index > 0 ? TRUE : FALSE;
	else
		allowUnsignedCode = TRUE;

	if(RegQueryValueEx(securityKey, TEXT("PromptAfterBadSigning"), NULL, NULL, (LPBYTE)&index, &nameLength) == ERROR_SUCCESS)
		promptAfterFailedSigning = index > 0 ? TRUE : FALSE;
	else
		promptAfterFailedSigning = TRUE;

	if(RegQueryValueEx(securityKey, TEXT("VerifyCertificate"), NULL, NULL, (LPBYTE)&index, &nameLength) == ERROR_SUCCESS)
		verifyCertificate = index;
	else
		verifyCertificate = 0;

	RegCloseKey(securityKey);
	if(pCert == NULL)
		return TRUE; // There was a security key, but signing was not configured.

	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		CertFreeCertificateContext(pCert);
		return FALSE;
	}
	ok = CryptImportPublicKeyInfo(hProv, pCert->dwCertEncodingType, &pCert->pCertInfo->SubjectPublicKeyInfo, &hPubKey);
	CertFreeCertificateContext(pCert);
	if(!ok)
	{
		CryptReleaseContext(hProv, 0);
		return FALSE;
	}
	return TRUE;
}
BOOL ShutdownSigning()
{
	CryptDestroyKey(hPubKey);
	CryptReleaseContext(hProv, 0);
	return TRUE;
}