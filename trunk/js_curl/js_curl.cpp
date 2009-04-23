// js_curl.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "jsapi.h"
#include "jsstr.h"
#include "curl\curl.h"

struct curlopt_assoc {
	DWORD optnumber;
	JSType typeassociated;
};


const struct curlopt_assoc curlOptAssociations[] = {
	{CURLOPT_VERBOSE, JSTYPE_BOOLEAN},
	{CURLOPT_UPLOAD, JSTYPE_BOOLEAN},
	{CURLOPT_UNRESTRICTED_AUTH, JSTYPE_BOOLEAN},
	{CURLOPT_TRANSFERTEXT, JSTYPE_BOOLEAN},
	{CURLOPT_SSL_VERIFYPEER, JSTYPE_BOOLEAN},
	{CURLOPT_PUT, JSTYPE_BOOLEAN},
	{CURLOPT_POST, JSTYPE_BOOLEAN},
	{CURLOPT_NOSIGNAL, JSTYPE_BOOLEAN},
	{CURLOPT_NOPROGRESS, JSTYPE_BOOLEAN},
	{CURLOPT_NOBODY, JSTYPE_BOOLEAN},
	{CURLOPT_NETRC, JSTYPE_BOOLEAN},
	{CURLOPT_HTTPPROXYTUNNEL, JSTYPE_BOOLEAN},
	{CURLOPT_HTTPGET, JSTYPE_BOOLEAN},
	{CURLOPT_HEADER, JSTYPE_BOOLEAN},
	{CURLOPT_FTPLISTONLY, JSTYPE_BOOLEAN},
	{CURLOPT_FTPAPPEND, JSTYPE_BOOLEAN},
	{CURLOPT_FTP_USE_EPSV, JSTYPE_BOOLEAN},
	{CURLOPT_FTP_USE_EPRT, JSTYPE_BOOLEAN},
	{CURLOPT_FRESH_CONNECT, JSTYPE_BOOLEAN},
	{CURLOPT_FORBID_REUSE, JSTYPE_BOOLEAN},
	{CURLOPT_FOLLOWLOCATION, JSTYPE_BOOLEAN},
	{CURLOPT_FILETIME, JSTYPE_BOOLEAN},
	{CURLOPT_FAILONERROR, JSTYPE_BOOLEAN},
	{CURLOPT_DNS_USE_GLOBAL_CACHE, JSTYPE_BOOLEAN},
	{CURLOPT_CRLF, JSTYPE_BOOLEAN},
	{CURLOPT_COOKIESESSION, JSTYPE_BOOLEAN},
	{CURLOPT_AUTOREFERER, JSTYPE_BOOLEAN},
	{CURLOPT_BUFFERSIZE, JSTYPE_NUMBER},
	{CURLOPT_CLOSEPOLICY, JSTYPE_NUMBER},
	{CURLOPT_CONNECTTIMEOUT, JSTYPE_NUMBER},
	{CURLOPT_DNS_CACHE_TIMEOUT, JSTYPE_NUMBER},
	{CURLOPT_FTPSSLAUTH, JSTYPE_NUMBER},
	{CURLOPT_HTTP_VERSION, JSTYPE_NUMBER},
	{CURLOPT_HTTPAUTH, JSTYPE_NUMBER},
	{CURLOPT_INFILESIZE, JSTYPE_NUMBER},
	{CURLOPT_LOW_SPEED_LIMIT, JSTYPE_NUMBER},
	{CURLOPT_LOW_SPEED_TIME, JSTYPE_NUMBER},
	{CURLOPT_MAXCONNECTS, JSTYPE_NUMBER},
	{CURLOPT_MAXREDIRS, JSTYPE_NUMBER},
	{CURLOPT_PORT, JSTYPE_NUMBER},
	{CURLOPT_PROXYAUTH, JSTYPE_NUMBER},
	{CURLOPT_PROXYPORT, JSTYPE_NUMBER},
	{CURLOPT_PROXYTYPE, JSTYPE_NUMBER},
	{CURLOPT_RESUME_FROM, JSTYPE_NUMBER},
	{CURLOPT_SSL_VERIFYHOST, JSTYPE_NUMBER},
	{CURLOPT_SSLVERSION, JSTYPE_NUMBER},
	{CURLOPT_TIMECONDITION, JSTYPE_NUMBER},
	{CURLOPT_TIMEOUT, JSTYPE_NUMBER},
	{CURLOPT_TIMEVALUE, JSTYPE_NUMBER},
	{CURLOPT_CAINFO, JSTYPE_STRING},
	{CURLOPT_CAPATH, JSTYPE_STRING},
	{CURLOPT_COOKIE, JSTYPE_STRING},
	{CURLOPT_COOKIEFILE, JSTYPE_STRING},
	{CURLOPT_COOKIEJAR, JSTYPE_STRING},
	{CURLOPT_CUSTOMREQUEST, JSTYPE_STRING},
	{CURLOPT_EGDSOCKET, JSTYPE_STRING},
	{CURLOPT_ENCODING, JSTYPE_STRING},
	{CURLOPT_FTPPORT, JSTYPE_STRING},
	{CURLOPT_INTERFACE, JSTYPE_STRING},
	{CURLOPT_KRB4LEVEL, JSTYPE_STRING},
	{CURLOPT_POSTFIELDS, JSTYPE_STRING},
	{CURLOPT_PROXY, JSTYPE_STRING},
	{CURLOPT_PROXYUSERPWD, JSTYPE_STRING},
	{CURLOPT_RANDOM_FILE, JSTYPE_STRING},
	{CURLOPT_RANGE, JSTYPE_STRING},
	{CURLOPT_REFERER, JSTYPE_STRING},
	{CURLOPT_SSL_CIPHER_LIST, JSTYPE_STRING},
	{CURLOPT_SSLCERT, JSTYPE_STRING},
	{CURLOPT_SSLCERTPASSWD, JSTYPE_STRING},
	{CURLOPT_SSLCERTTYPE, JSTYPE_STRING},
	{CURLOPT_SSLENGINE, JSTYPE_STRING},
	{CURLOPT_SSLENGINE_DEFAULT, JSTYPE_STRING},
	{CURLOPT_SSLKEY, JSTYPE_STRING},
	{CURLOPT_SSLKEYPASSWD, JSTYPE_STRING},
	{CURLOPT_SSLKEYTYPE, JSTYPE_STRING},
	{CURLOPT_URL, JSTYPE_STRING},
	{CURLOPT_USERAGENT, JSTYPE_STRING},
	{CURLOPT_USERPWD, JSTYPE_STRING}
};

JSBool curl_init(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	CURL * sessionHandle = curl_easy_init();
	if(sessionHandle == NULL)
	{
		JS_ReportError(cx, "Unable to initialize cURL session.");
		return JS_FALSE;
	}
	if(JS_SetPrivate(cx, obj, sessionHandle) == JS_FALSE)
	{
		curl_easy_cleanup(sessionHandle);
		return JS_FALSE;
	}
	curl_easy_setopt(sessionHandle, CURLOPT_NOPROGRESS, 1L);
	return JS_TRUE;
}

void curl_cleanup(JSContext * cx, JSObject * obj)
{
	CURL * sessionHandle = JS_GetPrivate(cx, obj);
	if(sessionHandle != NULL)
		curl_easy_cleanup(sessionHandle);
}

JSBool curl_setopt(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	if(argc < 2)
	{
		JS_ReportError(cx, "Not enough arguments for curl_setopt");
		return JS_FALSE;
	}
	CURL * sessionHandle = JS_GetPrivate(cx, obj);
	CURLoption curlOption;
	JS_BeginRequest(cx);
	if(JS_ValueToInt32(cx, argv[0], (int32*)&curlOption) == JS_FALSE)
	{
		JS_ReportError(cx, "Unable to parse curl option code.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}


	JSType optionType = JSTYPE_NULL;
	for(DWORD i = 0; i < sizeof(curlOptAssociations) / sizeof(struct curlopt_assoc); i++)
	{
		if(curlOptAssociations[i].optnumber == curlOption)
		{
			optionType = curlOptAssociations[i].typeassociated;
			break;
		}
	}

	if(optionType == JSTYPE_NULL)
	{
		JS_ReportError(cx, "No curl option specified or invalid type.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	CURLcode result = CURLE_CONV_FAILED;
	if(optionType == JSTYPE_STRING)
	{
		JSString * strValue = JS_ValueToString(cx, argv[1]);
		if(strValue == NULL)
		{
			JS_ReportError(cx, "Unable to parse string option.");
			JS_EndRequest(cx);
			return JS_FALSE;
		}
		result = curl_easy_setopt(sessionHandle, curlOption, JS_GetStringBytes(strValue));
	}
	else if(optionType == JSTYPE_NUMBER)
	{
		DWORD numberValue = 0;
		if(!JS_ValueToECMAUint32(cx, argv[1], &numberValue))
		{
			JS_ReportError(cx, "Unable to parse number option.");
			JS_EndRequest(cx);
			return JS_FALSE;
		}
		result = curl_easy_setopt(sessionHandle, curlOption, numberValue);
	}
	else if(optionType == JSTYPE_BOOLEAN)
	{
		JSBool booleanValue = JS_FALSE;
		if(!JS_ValueToBoolean(cx, argv[1], &booleanValue))
		{
			JS_ReportError(cx, "Unable to parse boolean option.");
			JS_EndRequest(cx);
			return JS_FALSE;
		}
		result = curl_easy_setopt(sessionHandle, curlOption, (DWORD)booleanValue);
	}
	if(result != CURLE_OK)
	{
		JS_EndRequest(cx);
		return JS_NewNumberValue(cx, result, rval);
	}
	*rval = JSVAL_TRUE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

struct curl_passed_buffer
{
	BYTE * buffer;
	DWORD totalLength;
	DWORD used;
	size_t nmemb;
	HANDLE heap;
};

size_t write_to_buffer(void * buffer, size_t size, size_t nmemb, void * userp)
{
	struct curl_passed_buffer * pBuf = (struct curl_passed_buffer *)userp;
	DWORD totalRead = size * nmemb;
	if(pBuf->used + totalRead >= pBuf->totalLength)
	{
		pBuf->totalLength += (totalRead > 4096 ? totalRead + 2048 : 4096);
		LPVOID result = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pBuf->buffer, pBuf->totalLength);
		if(result == NULL)
			return 0;
		else
			pBuf->buffer = (LPBYTE)result;
	}
	memcpy_s(pBuf->buffer + pBuf->used, pBuf->totalLength - pBuf->used, buffer, totalRead);
	pBuf->used += totalRead;
	return totalRead;
}


size_t write_to_file(void * buffer, size_t size, size_t nmemb, void * userp)
{
	HANDLE hFile = (HANDLE)userp;
	DWORD written = 0;
	if(WriteFile(hFile, buffer, size * nmemb, &written, NULL) == FALSE)
		return 0;
	return written;
}

JSBool curl_saveas(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	if(argc < 1)
	{
		JS_ReportError(cx, "Must provide a path to saveas.");
		return JS_FALSE;
	}
	JS_BeginRequest(cx);
	CURL * sessionHandle = JS_GetPrivate(cx, obj);
	JSString * path = JS_ValueToString(cx, argv[0]);

	jsrefcount rCount = JS_SuspendRequest(cx);
	HANDLE openFile = CreateFile((LPWSTR)JS_GetStringChars(path), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if(openFile == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	curl_easy_setopt(sessionHandle, CURLOPT_WRITEFUNCTION, write_to_file);
	curl_easy_setopt(sessionHandle, CURLOPT_WRITEDATA, openFile);
	CURLcode result = curl_easy_perform(sessionHandle);
	CloseHandle(openFile);
	JS_ResumeRequest(cx, rCount);
	if(result != 0)
		JS_NewNumberValue(cx, result, rval);
	*rval = JSVAL_TRUE;
	JS_EndRequest(cx);
	return JS_TRUE;	
}

JSBool curl_perform(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	CURL * sessionHandle = JS_GetPrivate(cx, obj);
	struct curl_passed_buffer pBuf;
	memset(&pBuf, 0, sizeof(struct curl_passed_buffer));
	pBuf.buffer = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 4096);
	pBuf.totalLength = 4096;
	curl_easy_setopt(sessionHandle, CURLOPT_WRITEFUNCTION, write_to_buffer);
	curl_easy_setopt(sessionHandle, CURLOPT_WRITEDATA, &pBuf);

	CURLcode result = curl_easy_perform(sessionHandle);
	if(result != 0)
	{
		HeapFree(GetProcessHeap(), 0, pBuf.buffer);
		JS_BeginRequest(cx);
		JSBool retBool = JS_NewNumberValue(cx, result, rval);
		JS_EndRequest(cx);
		return retBool;
	}

	LPBYTE actualBuffer = (LPBYTE)JS_malloc(cx, pBuf.used + 1);
	memset(actualBuffer, 0, pBuf.used + 1);
	memcpy_s(actualBuffer, pBuf.used + 1, pBuf.buffer, pBuf.used);
	HeapFree(GetProcessHeap(), 0, pBuf.buffer);
	JS_BeginRequest(cx);
	JSString * retString = JS_NewString(cx, (char*)actualBuffer, pBuf.used);
	if(retString == NULL)
	{
		JS_ReportError(cx, "Unable to allocate buffer for returned content.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	*rval = STRING_TO_JSVAL(retString);
	JS_EndRequest(cx);
	return JS_TRUE;
}


struct JSFunctionSpec curlClassMethods[] = {
	{ "SetOpt", curl_setopt, 2, 0, 0 },
	{ "Perform", curl_perform, 0, 0, 0 },
	{ "SaveAs", curl_saveas, 1, 0, 0 },
	{ 0, 0, 0, 0, 0 }
};