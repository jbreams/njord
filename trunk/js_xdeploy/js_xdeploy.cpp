/*
 * Copyright 2009 Drew University
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); 
 * you may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at 
 *
 * 		http://www.apache.org/licenses/LICENSE-2.0
 *	
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * See the License for the specific language governing permissions and 
 * limitations under the License. 
 */

// js_xdeploy.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include <lm.h>
#include <Sddl.h>
#include <comdef.h>
#include <wbemidl.h>
#pragma comment(lib, "wbemuuid.lib")

struct JSConstDoubleSpec xdeployConsts[] = {
	{ ComputerNamePhysicalDnsDomain, "ComputerNamePhysicalDnsDomain", 0, 0 },
	{ ComputerNamePhysicalDnsHostname, "ComputerNamePhysicalDnsHostname", 0, 0 },
	{ ComputerNamePhysicalNetBIOS, "ComputerNamePhysicalNetBIOS", 0, 0 },
	{ NETSETUP_JOIN_DOMAIN, "NETSETUP_JOIN_DOMAIN", 0, 0 },
	{ NETSETUP_ACCT_CREATE, "NETSETUP_ACCT_CREATE", 0, 0 },
	{ NETSETUP_WIN9X_UPGRADE, "NETSETUP_WIN9X_UPGRADE", 0, 0},
	{ NETSETUP_DOMAIN_JOIN_IF_JOINED, "NETSETUP_DOMAIN_JOIN_IF_JOINED", 0, 0 },
	{ NETSETUP_JOIN_UNSECURE, "NETSETUP_JOIN_UNSECURE", 0, 0 },
	{ NETSETUP_MACHINE_PWD_PASSED, "NETSETUP_MACHINE_PWD_PASSED", 0, 0 },
	{ NETSETUP_DEFER_SPN_SET, "NETSETUP_DEFER_SPN_SET", 0, 0 },
	{ NETSETUP_JOIN_WITH_NEW_NAME, "NETSETUP_JOIN_WITH_NEW_NAME", 0, 0 },
	{ NETSETUP_INSTALL_INVOCATION, "NETSETUP_INSTALL_INVOCATION", 0, 0 },
	{ SidTypeUser, "SidTypeUser", 0, 0 },
	{ SidTypeGroup, "SidTypeGroup", 0, 0 },
	{ SidTypeDomain, "SidTypeDomain", 0, 0 },
	{ SidTypeAlias, "SidTypeAlias", 0, 0 },
	{ SidTypeWellKnownGroup, "SidTypeWellKnownGroup", 0, 0 },
	{ SidTypeDeletedAccount, "SidTypeInvalid", 0, 0 },
	{ SidTypeUnknown, "SidTypeUnknown", 0, 0 },
	{ SidTypeComputer, "SidTypeComputer", 0, 0 },
	{ SidTypeLabel, "SidTypeLabel", 0, 0 },
	{ 0 }
};

IWbemServices * pSvc = NULL;
IWbemLocator * pLoc = NULL;

JSBool setcomputername(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * newName;
	COMPUTER_NAME_FORMAT nameType = ComputerNamePhysicalNetBIOS;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S /u", &newName, (DWORD*)&nameType))
	{
		JS_ReportError(cx, "Unable to parse arguments in setcomputername.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	*rval = SetComputerNameEx(nameType, (LPWSTR)JS_GetStringChars(newName)) != 0 ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool netgetjoinableous(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR domain, account, password;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W W W", &domain, &account, &password))
	{
		JS_ReportError(cx, "Error parsing arguments in NetGetJoinableOUs");
		JS_EndRequest(cx);
	}

	JS_YieldRequest(cx);
	DWORD ouCount = 0;
	LPWSTR * ous = NULL;
	DWORD status = NetGetJoinableOUs(NULL, domain, account, password, &ouCount, &ous);
	if(status != NERR_Success)
	{
		JS_NewNumberValue(cx, status, rval);
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * arrayObj = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(arrayObj);
	for(DWORD i = 0; i < ouCount; i++)
	{
		JSString * curOu = JS_NewUCStringCopyZ(cx, (jschar*)ous[i]);
		JS_DefineElement(cx, arrayObj, i, STRING_TO_JSVAL(curOu), NULL, NULL, 0);
	}
	NetApiBufferFree(ous);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool netjoindomain(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * serverStr = NULL, * domainStr = NULL, *accountOUStr = NULL, *accountStr = NULL, *passwordStr = NULL;
	DWORD joinOptions = 0;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S S S S u", &domainStr, &accountOUStr, &accountStr, &passwordStr, &joinOptions))
	{
		JS_ReportError(cx, "Unable to parse arguments in netjoindomain");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	JS_YieldRequest(cx);
	DWORD result = NetJoinDomain(NULL, (LPWSTR)JS_GetStringChars(domainStr), (LPWSTR)JS_GetStringChars(accountOUStr),
		(LPWSTR)JS_GetStringChars(accountStr), (LPWSTR)JS_GetStringChars(passwordStr), joinOptions);
	if(result == NERR_Success)
	{
		*rval = JSVAL_TRUE;
		return JS_TRUE;
	}
	JSBool retBool = JS_NewNumberValue(cx, result, rval);
	JS_EndRequest(cx);
	return retBool;
}

JSBool netlocalgroupaddsid(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * groupName, *sidString;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S S", &groupName, &sidString))
	{
		JS_ReportError(cx, "Unable to parse argument in netlocalgroupaddsid");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PSID realSid;
	ConvertStringSidToSid((LPWSTR)JS_GetStringChars(sidString), &realSid);

	LOCALGROUP_MEMBERS_INFO_0 passed;
	passed.lgrmi0_sid = realSid;

	JS_YieldRequest(cx);
	NET_API_STATUS status = NetLocalGroupAddMembers(NULL, (LPWSTR)JS_GetStringChars(groupName), 0, (LPBYTE)&passed, 1);
	LocalFree(realSid);
	
	if(status == NERR_Success)
		*rval = JSVAL_TRUE;
	else
		JS_NewNumberValue(cx, status, rval);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool netlocalgroupaddmembers(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * groupName;
	JSObject * membersArray = NULL;
	JSString * singleMember = NULL;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S *", &groupName) || argc < 2)
	{
		JS_ReportError(cx, "Unable to parse argument in netlocalgroupaddmembers");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(JSVAL_IS_OBJECT(argv[1]))
	{
		JS_ValueToObject(cx, argv[1], &membersArray);
		if(!JS_IsArrayObject(cx, membersArray))
		{
			JS_ReportError(cx, "netlocalgroupaddmembers takes either a string or an array of strings as an argument. Invalid type specified.");
			JS_EndRequest(cx);
			return JS_FALSE;
		}
	}
	else if(JSVAL_IS_STRING(argv[1]))
		singleMember = JS_ValueToString(cx, argv[1]);
	else
	{
		JS_ReportError(cx, "netlocalgroupaddmembers takes either a string or an array of strings as an argument. Invalid type specified.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	LOCALGROUP_MEMBERS_INFO_3 * members = NULL;
	DWORD memberCount = 0;
	if(singleMember != NULL)
	{
		members = new LOCALGROUP_MEMBERS_INFO_3;
		members->lgrmi3_domainandname = (LPWSTR)JS_GetStringChars(singleMember);
		memberCount = 1;
	}
	else if(membersArray != NULL)
	{
		JS_GetArrayLength(cx, membersArray, &memberCount);
		members = new LOCALGROUP_MEMBERS_INFO_3[memberCount];
		for(DWORD i = 0; i < memberCount; i++)
		{
			jsval curMemberVal;
			JS_GetElement(cx, membersArray, i, &curMemberVal);
			JSString * curMemberStr = JS_ValueToString(cx, curMemberVal);
			members[i].lgrmi3_domainandname = (LPWSTR)JS_GetStringChars(curMemberStr);
		}
	}

	DWORD result = NetLocalGroupAddMembers(NULL, (LPWSTR)JS_GetStringChars(groupName), 3, (LPBYTE)members, memberCount);
	delete [] members;

	JSBool retBool = JS_NewNumberValue(cx, result, rval);
	JS_EndRequest(cx);
	return retBool;
}

JSBool lookupAccountName(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * accountName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "S", &accountName))
	{
		JS_ReportError(cx, "Error parsing arguments in lookupAccountName");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PSID sid;
	LPWSTR domainName;
	DWORD sidSize = 0, rdSize = 0;
	SID_NAME_USE peUse;

	LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(accountName), NULL, &sidSize, NULL, &rdSize, &peUse);
	domainName = (LPWSTR)JS_malloc(cx, (rdSize + 1) * sizeof(TCHAR));
	sid = (PSID)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sidSize);

	JS_YieldRequest(cx);
	if(!LookupAccountName(NULL, (LPWSTR)JS_GetStringChars(accountName), sid, &sidSize, domainName, &rdSize, &peUse))
	{
		HeapFree(GetProcessHeap(), 0, sid);
		JS_free(cx, domainName);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, NULL, NULL, obj);
	*rval = OBJECT_TO_JSVAL(retObj);

	LPTSTR stringSid = NULL;

	ConvertSidToStringSid(sid, &stringSid);
	HeapFree(GetProcessHeap(), 0, sid);

	JS_DefineProperty(cx, retObj, "sid", STRING_TO_JSVAL(JS_NewUCStringCopyZ(cx, (jschar*)stringSid)), NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY);
	LocalFree(stringSid);
	JS_DefineProperty(cx, retObj, "domain", STRING_TO_JSVAL(JS_NewUCString(cx, (jschar*)domainName, rdSize)), NULL, NULL,  JSPROP_ENUMERATE | JSPROP_READONLY);
	jsval peUseVal;
	JS_NewNumberValue(cx, peUse, &peUseVal);
	JS_DefineProperty(cx, retObj, "type", peUseVal, NULL, NULL,  JSPROP_ENUMERATE | JSPROP_READONLY);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool GetLastNetErrorMessage(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	DWORD dwLastError = NERR_Success;
	HANDLE hModule = GetModuleHandle(NULL);
	if(argc < 1 || !JSVAL_IS_NUMBER(*argv))
	{
		JS_ReportError(cx, "Must provide valid error code to GetLastNetErrorMessage");
		return JS_FALSE;
	}

	JS_BeginRequest(cx);
	JS_ValueToECMAUint32(cx, *argv, &dwLastError);
	DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM ;

	if(dwLastError >= NERR_BASE && dwLastError <= MAX_NERR) 
	{
        hModule = LoadLibraryEx(TEXT("netmsg.dll"), NULL, LOAD_LIBRARY_AS_DATAFILE);
        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

	LPWSTR buffer = NULL;
	DWORD size = FormatMessage(dwFormatFlags, hModule, dwLastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL);
	if(size == 0)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	JSString * rString = JS_NewUCStringCopyN(cx, (jschar*)buffer, size);
	*rval = STRING_TO_JSVAL(rString);
	JS_EndRequest(cx);
	LocalFree(buffer);
	return JS_TRUE;
}

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
		DWORD unLen = UNLEN + 1;
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
#ifdef __cplusplus
extern "C" {
#endif

BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec xdeployFunctions[] = {
		{ "SetComputerName", setcomputername, 2, 0 },
		{ "NetJoinDomain", netjoindomain, 6, 0 },
		{ "NetLocalGroupAddMembers", netlocalgroupaddmembers, 2, 0 },
		{ "NetLocalGroupAddSid", netlocalgroupaddsid, 2, 0 },
		{ "NetGetLastErrorMessage", GetLastNetErrorMessage, 1, 0 },
		{ "LookupAccountName", lookupAccountName, 1, 0 },
		{ "NetGetJoinableOUs", netgetjoinableous, 3, 0 },
		{ "CryptVerifyFileSignature", verify_file_signature, 4, 0 },
		{ "CryptSignFile", sign_file, 2, 0 },
		{ 0 },
	};

	JS_BeginRequest(cx);
	JS_DefineFunctions(cx, global, xdeployFunctions);
	JS_DefineConstDoubles(cx, global, xdeployConsts);
	JS_EndRequest(cx);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	pSvc->Release();
	pLoc->Release();
	return TRUE;
}

#ifdef __cplusplus
}
#endif