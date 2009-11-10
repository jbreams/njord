#include "stdafx.h"
#include <Wincrypt.h>
#pragma comment(lib, "crypt32.lib")

JSBool verify_file_signature(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR fileToVerify = NULL, hashToCheck = NULL, certToUse = NULL;
	JSBool useSHA = JS_FALSE;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W b/ W", &fileToVerify, &hashToCheck, &useSHA, &certToUse))
	{
		JS_ReportError(cx, "Error parsing arguments in VerifyFileHash");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	*rval = JSVAL_FALSE;
	HCRYPTHASH hHash;
	HCRYPTPROV hProv;
	HANDLE fileToCheck = CreateFile(fileToVerify, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(fileToCheck == INVALID_HANDLE_VALUE)
		return JS_TRUE;
	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
	{
		CloseHandle(fileToCheck);
		return JS_TRUE;
	}
	if(!CryptCreateHash(hProv, (useSHA ? CALG_SHA : CALG_MD5), NULL, 0, &hHash))
	{
		CloseHandle(fileToCheck);
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}
	LPBYTE buffer = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, 4096);
	DWORD curReadSize = 0;
	do
	{
		if(!ReadFile(fileToCheck, buffer, 4096, &curReadSize, NULL))
			curReadSize = 0;
		if(curReadSize > 0)
		{
			if(!CryptHashData(hHash, buffer, curReadSize, 0))
				MessageBoxA(NULL, "Error hashing data!", "", MB_OK);
			memset(buffer, 0, 4096);
		}
	} while(curReadSize > 0);
	HeapFree(GetProcessHeap(), 0, buffer);
	CloseHandle(fileToCheck);

	LPBYTE sigContents = NULL;
	DWORD sigLength = 0;
	DWORD flags = (certToUse != NULL ? CRYPT_STRING_ANY : CRYPT_STRING_HEX_ANY);
	CryptStringToBinary(hashToCheck, wcslen(hashToCheck), flags, sigContents, &sigLength, NULL, NULL);
	sigContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, sigLength);
	CryptStringToBinary(hashToCheck, wcslen(hashToCheck), flags, sigContents, &sigLength, NULL, NULL);
	if(certToUse != NULL)
	{
		HANDLE certFile = CreateFile(certToUse, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if(certFile == INVALID_HANDLE_VALUE)
		{
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			return JS_TRUE;
		}
		DWORD fileSize = GetFileSize(certFile, NULL);
		LPBYTE certFileContents = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fileSize + sizeof(WCHAR));
		if(!ReadFile(certFile, certFileContents, fileSize, &fileSize, NULL))
		{
			HeapFree(GetProcessHeap(), 0, certFileContents);
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			return JS_TRUE;
		}
		CloseHandle(certFile);

		LPBYTE certContents;
		DWORD certLength;
		if(IsTextUnicode(certFileContents, fileSize, NULL))
		{
			CryptStringToBinaryW((LPWSTR)certFileContents, wcslen((LPWSTR)certFileContents), CRYPT_STRING_ANY, NULL, &certLength, NULL, NULL);
			certContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, certLength + 1);
			CryptStringToBinaryW((LPWSTR)certFileContents, wcslen((LPWSTR)certFileContents), CRYPT_STRING_ANY, certContents, &certLength, NULL, NULL);
		}
		else
		{
			CryptStringToBinaryA((LPSTR)certFileContents, strlen((LPSTR)certFileContents), CRYPT_STRING_ANY, NULL, &certLength, NULL, NULL);
			certContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, certLength + 1);
			CryptStringToBinaryA((LPSTR)certFileContents, strlen((LPSTR)certFileContents), CRYPT_STRING_ANY, certContents, &certLength, NULL, NULL);
		}
		HeapFree(GetProcessHeap(), 0, certFileContents);
		PCCERT_CONTEXT pCert = CertCreateCertificateContext(X509_ASN_ENCODING, certContents, certLength);
		HeapFree(GetProcessHeap(), 0, certContents);
		HCRYPTKEY hPublicKey = NULL;
		BOOL ok = CryptImportPublicKeyInfo(hProv, pCert->dwCertEncodingType, &pCert->pCertInfo->SubjectPublicKeyInfo, &hPublicKey);
		CertFreeCertificateContext(pCert);
		if(!ok)
		{
			CryptDestroyHash(hHash);
			CryptReleaseContext(hProv, 0);
			return JS_TRUE;
		}
		*rval = CryptVerifySignature(hHash, sigContents, sigLength, hPublicKey, NULL, 0) ? JSVAL_TRUE : JSVAL_FALSE;
		CryptDestroyKey(hPublicKey);
	}
	else
	{
		DWORD hashSize = 4, dataSize = sizeof(DWORD);
		LPBYTE hashData = NULL;
		CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE*)&hashSize, &dataSize, 0);
		hashData = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, hashSize);
		CryptGetHashParam(hHash, HP_HASHVAL, hashData, &hashSize, 0);
		if(hashSize == sigLength)
			*rval = memcmp(hashData, sigContents, hashSize) == 0 ? JSVAL_TRUE : JSVAL_FALSE;
		HeapFree(GetProcessHeap(), 0, hashData);
	}
	HeapFree(GetProcessHeap(), 0, sigContents);
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	return JS_TRUE;
}

JSBool sign_file(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR fileName = NULL, subjectName = NULL;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W/ W", &fileName, &subjectName))
	{
		JS_ReportError(cx, "Error parsing arguments in SignFile");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);
	*rval = JSVAL_FALSE;

	BOOL ok = FALSE;
	HCRYPTPROV hProv;
	HCERTSTORE hCurrentUserStore = CertOpenStore(CERT_STORE_PROV_SYSTEM, 0, NULL, CERT_SYSTEM_STORE_CURRENT_USER, L"my");
	if(hCurrentUserStore == NULL)
		return JS_TRUE;

	if(subjectName == NULL)
	{
		DWORD unLen = 15 + 1;
		subjectName = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, unLen);
		GetUserName(subjectName, &unLen);
		ok = TRUE;
	}
	PCCERT_CONTEXT pCert = CertFindCertificateInStore(hCurrentUserStore, PKCS_7_ASN_ENCODING | X509_ASN_ENCODING, 0, CERT_FIND_SUBJECT_STR, subjectName, NULL);
	if(ok)
		HeapFree(GetProcessHeap(), 0, subjectName);
	CertCloseStore(hCurrentUserStore, 0);
	if(pCert == NULL)
		return JS_TRUE;
	DWORD keySpec = AT_SIGNATURE;
	ok = CryptAcquireCertificatePrivateKey(pCert, 0, NULL, &hProv, &keySpec, &ok);
	CertFreeCertificateContext(pCert);
	if(!ok)
		return JS_TRUE;

	HCRYPTHASH hHash;
	if(!CryptCreateHash(hProv, CALG_SHA, NULL, 0, &hHash))
	{
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}

	HANDLE fileToSign = CreateFile(fileName, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	if(fileToSign == INVALID_HANDLE_VALUE)
	{
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}

	LPBYTE buffer = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, 4096);
	DWORD curReadSize = 0;
	do
	{
		if(!ReadFile(fileToSign, buffer, 4096, &curReadSize, NULL))
			curReadSize = 0;
		if(curReadSize > 0)
		{
			if(!CryptHashData(hHash, buffer, curReadSize, 0))
				MessageBoxA(NULL, "Error hashing data!", "", MB_OK);
			memset(buffer, 0, 4096);
		}
	} while(curReadSize > 0);
	HeapFree(GetProcessHeap(), 0, buffer);
	CloseHandle(fileToSign);
	
	DWORD sigLength = 0;
	if(!CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, NULL, &sigLength))
	{
		CryptDestroyHash(hHash);
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}
	LPBYTE sigContents = (LPBYTE)HeapAlloc(GetProcessHeap(), 0, sigLength + 1);
	ok = CryptSignHash(hHash, AT_SIGNATURE, NULL, 0, sigContents, &sigLength);
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);
	if(!ok)
	{
		HeapFree(GetProcessHeap(), 0, sigContents);
		return JS_TRUE;
	}

	DWORD sigStringLength = 0;
	if(!CryptBinaryToString(sigContents, sigLength, CRYPT_STRING_BASE64, NULL, &sigStringLength))
	{
		HeapFree(GetProcessHeap(), 0, sigContents);
		return JS_TRUE;
	}
	LPWSTR sigString = (LPWSTR)JS_malloc(cx, sigStringLength * sizeof(jschar));
	ok = CryptBinaryToString(sigContents, sigLength, CRYPT_STRING_BASE64, sigString, &sigStringLength);
	HeapFree(GetProcessHeap(), 0, sigContents);
	if(!ok)
	{
		JS_free(cx, sigString);
		return JS_TRUE;
	}

	JS_BeginRequest(cx);
	JSString * retString = JS_NewUCString(cx, (jschar*)sigString, sigStringLength);
	*rval = STRING_TO_JSVAL(retString);
	JS_EndRequest(cx);
	return JS_TRUE;
}

char bytetochar(BYTE c)
{
	if(c >= 0x00 && c <= 0x09)
		return '0' + c;
	if(c >= 0x0a && c <= 0x0f)
		return 'a' + (c - 0x0a);
	return 'f';
}

JSBool value_to_md5(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	*rval = JSVAL_FALSE;
	if(argc == 0)
		return JS_TRUE;

	HCRYPTPROV hProv; 
	if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
		return JS_TRUE;

	HCRYPTHASH hHash;
	if(!CryptCreateHash(hProv, CALG_MD5, NULL, 0, &hHash))
	{
		CryptReleaseContext(hProv, 0);
		return JS_TRUE;
	}

	JS_BeginRequest(cx);
	JSString * inString = JS_ValueToString(cx, *argv);
	*argv = STRING_TO_JSVAL(inString);
	CryptHashData(hHash, (LPBYTE)JS_GetStringChars(inString), JS_GetStringLength(inString) * sizeof(jschar), 0);

	DWORD hashLen;
	DWORD bufSize = sizeof(DWORD);
	CryptGetHashParam(hHash, HP_HASHSIZE, (LPBYTE)&hashLen, &bufSize, 0);
	LPBYTE theHash = (LPBYTE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, hashLen);
	CryptGetHashParam(hHash, HP_HASHVAL, theHash, &hashLen, 0);
	CryptDestroyHash(hHash);
	CryptReleaseContext(hProv, 0);

	bufSize = (hashLen * 2) + 1;
	LPSTR hashString = (LPSTR)JS_malloc(cx, bufSize);
	memset(hashString, 0, bufSize);
	DWORD c = 0;
	for(DWORD i = 0; i < hashLen; i++)
	{
		hashString[c++] = bytetochar(theHash[i] >> 4);
		hashString[c++] = bytetochar(theHash[i] & 0x0f);
	}
	HeapFree(GetProcessHeap(), 0, theHash);
	JSString * outString = JS_NewString(cx, hashString, bufSize);
	*rval = STRING_TO_JSVAL(outString);
	JS_EndRequest(cx);
	return JS_TRUE;	
}

void InitCrypto(JSContext * cx, JSObject * obj)
{
	struct JSFunctionSpec crypto_funcs[] = {
		{ "CryptVerifyFileSignature", verify_file_signature, 4, 0 },
		{ "CryptSignFile", sign_file, 2, 0 },
		{ "MD5", value_to_md5, 1, 0 },
		{ 0 }
	};

	JS_DefineFunctions(cx, obj, crypto_funcs);
}