// js_reg.cpp : Defines the exported functions for the DLL application.
//
#include "stdafx.h"
#include "jsapi.h"
#include "jsstr.h"

struct JSConstDoubleSpec regConsts[] = {
	{ 1, "HKEY_LOCAL_MACHINE", 0, 0},
	{ 2, "HKEY_USERS", 0, 0 },
	{ 3, "HKEY_CURRENT_USER", 0, 0 },
	{ 4, "HKEY_CURRENT_CONFIG", 0, 0 },
	{ 5, "HKEY_CLASSES_ROOT", 0, 0 },
	{ 6, "REG_DWORD", 0, 0 },
	{ 7, "REG_EXPAND_SZ", 0, 0 },
	{ 8, "REG_MULTI_SZ", 0, 0 },
	{ 9, "REG_SZ", 0, 0 },
	{ 0, 0, 0, 0 },
};

HKEY inline ResolveRegConst(jsdouble key)
{
	switch((WORD)key)
	{
	case 1:
		return HKEY_LOCAL_MACHINE;
	case 2:
		return HKEY_USERS;
	case 3:
		return HKEY_CURRENT_USER;
	case 4:
		return HKEY_CURRENT_CONFIG;
	case 5:
		return HKEY_CLASSES_ROOT;
	default:
		return NULL;
	}
}

JSBool xprep_js_set_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	jsdouble jsHive, jsRegType;
	JSString * jsSubKey, *jsValueName;
	DWORD sknLen = 0, vnLen = 0, valueType = 0, contentLength = 0;
	HKEY hSubKey = NULL, hive = NULL;
	JSBool retval = JS_TRUE;
	LPBYTE valueContent = NULL;
	
	if(argc < 5)
	{
		*rval = JS_FALSE;
		JS_ReportError(cx, "No value supplied, cannot set registry values to NULL.");
		return JS_FALSE;
	}

	if(JS_ConvertArguments(cx, argc, argv, "I S S I *", &jsHive, &jsSubKey, &jsValueName, &jsRegType) == JS_FALSE)
		return JS_FALSE;


	sknLen = JS_GetStringLength(jsSubKey);
	hive = ResolveRegConst(jsHive);
	switch((WORD)jsRegType)
	{
	case 6:
		valueType = REG_DWORD;
		{
			if(!JSVAL_IS_NUMBER(argv[4]))
			{
				JS_ReportError(cx, "Value supplied is not a number for a REG_DWORD value.");
				*rval = JS_FALSE;
				retval = JS_FALSE;
				goto cleanup;
			}
			DWORD dw_value = 0;
			JS_ValueToECMAUint32(cx, argv[4], &dw_value);
			valueContent = new BYTE[sizeof(DWORD)];
			contentLength = sizeof(DWORD);
			memcpy_s(valueContent, sizeof(DWORD), &dw_value, sizeof(DWORD));
		}
		break;
	case 7:
		valueType = REG_EXPAND_SZ;
		break;
	case 8:
		valueType = REG_MULTI_SZ;
		{
			JSObject * valueArray;
			JS_ValueToObject(cx, argv[4], &valueArray);
			if(!JS_IsArrayObject(cx, valueArray))
			{
				JS_ReportError(cx, "Value supplied is not an array for a REG_MULTI_SZ value.");
				*rval = JS_FALSE;
				goto cleanup;
			}
			jsuint arraylen;
			JS_GetArrayLength(cx, valueArray, &arraylen);
			HANDLE destHeap = HeapCreate(0, 0, 0);
			DWORD used = 0, totalSize = 4096;
			LPBYTE destination = (LPBYTE)HeapAlloc(destHeap, 0, 4096);
			for(jsuint i = 0; i < arraylen; i++)
			{
				jsval curStringVal = JSVAL_VOID;
				if(JS_GetElement(cx, valueArray, i, &curStringVal) == JS_FALSE || curStringVal == JSVAL_VOID)
					continue;
				JSString * curString = JS_ValueToString(cx, curStringVal);
				DWORD curStringLen = JS_GetStringLength(curString);
				if(totalSize - used - ((curStringLen + 1)* sizeof(WCHAR)) < 5)
				{
					totalSize += 4096;
					HeapReAlloc(destHeap, 0, destination, totalSize);
				}
				LPBYTE realDest = destination + used;
				memset(realDest, 0, totalSize - used);
				wcscpy_s((LPWSTR)realDest, (totalSize - used)/sizeof(WCHAR), (LPWSTR)JS_GetStringChars(curString));
				used += (curStringLen + 1) * sizeof(WCHAR);
			}
			valueContent = new BYTE[used + sizeof(WCHAR)];
			memset(valueContent, 0, used + sizeof(WCHAR));
			contentLength = sizeof(WCHAR) + used;
			memcpy_s(valueContent, contentLength, destination, used);
			HeapDestroy(destHeap);
		}
		break;
	case 9:
		valueType = REG_SZ;
		break;
	default:
		*rval = JS_FALSE;
		retval = JS_FALSE;
		JS_ReportError(cx, "Unsupported type for RegSetValue.");
		goto cleanup;
	}


	if(valueType == REG_SZ || valueType == REG_EXPAND_SZ)
	{
		if(!JSVAL_IS_STRING(argv[4]))
		{
			JS_ReportError(cx, "Value supplied is not a string for a REG_SZ or REG_EXPAND_SZ value.");
			*rval = JS_FALSE;
			retval = JS_FALSE;
			goto cleanup;
		}
		JSString * str_value = JS_ValueToString(cx, argv[4]);
		contentLength = (JS_GetStringLength(str_value) + 1) * sizeof(WCHAR);
		valueContent = new BYTE[contentLength];
		memcpy_s(valueContent, contentLength, (void*)JS_GetStringChars(str_value), contentLength);
	}
	if(RegCreateKeyW(hive, (LPWSTR)JS_GetStringChars(jsSubKey), &hSubKey) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Unable to open or create registry key to create value");
		*rval = JS_FALSE;
		retval = JS_FALSE;
		goto cleanup;
	}
	
	if(RegSetValueExW(hSubKey, (LPWSTR)JS_GetStringChars(jsValueName), 0, valueType, valueContent, contentLength) != ERROR_SUCCESS)
	{
		JS_ReportError(cx, "Unable to set value.");
		*rval = JS_FALSE;
		retval = JS_FALSE;
	}

	RegCloseKey(hSubKey);
	*rval = JS_TRUE;
cleanup:

	if(valueContent != NULL)
		delete [] valueContent;

	return retval;
}
JSBool xprep_js_delete_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * keyName, *valueName = NULL;
	jsdouble hiveVal = 0;
	HKEY hive, curKey;

	if(!JS_ConvertArguments(cx, argc, argv, "d S /S", &hiveVal, &keyName, &valueName))
		return JS_FALSE;

	if(JS_GetStringLength(keyName) == 0)
		return JS_FALSE;

	hive = ResolveRegConst(hiveVal);

	if(RegOpenKeyEx(hive, (LPWSTR)JS_GetStringChars(keyName), 0, KEY_WRITE, &curKey) != ERROR_SUCCESS)
		return JS_FALSE;
	LONG result = RegDeleteValue(curKey, (LPWSTR)JS_GetStringChars(valueName));
	RegCloseKey(curKey);
	if(result != ERROR_SUCCESS)
		return JS_FALSE;
	return JS_TRUE;
}

JSBool xprep_js_create_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * keyName = NULL;
	jsdouble jsHive = 0;
	HKEY hive, curKey;

	if(!JS_ConvertArguments(cx, argc, argv, "d S", &jsHive, &keyName))
		return JS_FALSE;

	if(JS_GetStringLength(keyName) == 0)
		return JS_FALSE;
	hive = ResolveRegConst(jsHive);
	if(hive == NULL)
		return JS_FALSE;

	if(RegCreateKeyEx(hive, (LPWSTR)JS_GetStringChars(keyName), 0, NULL, 0, KEY_WRITE, NULL, &curKey, NULL) != ERROR_SUCCESS)
		return JS_FALSE;
	RegCloseKey(curKey);
	*rval = JS_TRUE;
	return JS_TRUE;
}

BOOL RegDelKeyRecurse(HKEY parent, LPTSTR child)
{
	if(RegDeleteKey(parent, child) == ERROR_SUCCESS)
		return TRUE;
	HKEY thisKey;
	RegOpenKeyEx(parent, child, 0, KEY_WRITE | KEY_ENUMERATE_SUB_KEYS, &thisKey);
	if(thisKey == NULL)
		return FALSE;
	DWORD index = 0, curNameSize = 255;
	LONG result = 0;
	LPTSTR curName = new TCHAR[255];
	BOOL retval = TRUE;

	result = RegEnumKeyEx(thisKey, index, curName, &curNameSize, NULL, NULL, NULL, NULL);
	while(result != ERROR_NO_MORE_ITEMS)
	{
		if(RegDelKeyRecurse(thisKey, curName) == FALSE)
		{
			retval = FALSE;
			goto functionEnd;
		}

		curNameSize = 255;
		memset(curName, 0, 255);
		result = RegEnumKeyEx(thisKey, ++index, curName, &curNameSize, NULL, NULL, NULL, NULL);
	}
functionEnd:
	delete [] curName;
	RegCloseKey(thisKey);
	if(retval == TRUE)
		RegDeleteKey(parent, child);
	return retval;
}

JSBool xprep_js_delete_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * subKeyName;
	jsdouble jsHive;
	HKEY hive;

	if(!JS_ConvertArguments(cx, argc, argv, "d S", &jsHive, &subKeyName))
		return JS_FALSE;

	hive = ResolveRegConst(jsHive);
	if(RegDelKeyRecurse(hive, (LPWSTR)JS_GetStringChars(subKeyName)) == FALSE)
		return JS_FALSE;
	return JS_TRUE;
}
JSBool xprep_js_enum_keys(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * jsKeyName;
	jsdouble jsHive = 0;
	HKEY hive = NULL, curKey = NULL;
	DWORD nSubKeys = 0, bufLen = 0;
	LPWSTR keyName = NULL;
	jsval * retvals = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "d S", &jsHive, &jsKeyName))
		return JS_FALSE;

	hive = ResolveRegConst(jsHive);

	RegOpenKeyEx(hive, (LPWSTR)JS_GetStringChars(jsKeyName), 0, KEY_READ, &curKey);
	if(curKey == NULL)
	{
		JS_ReportError(cx, "Unable to open registry key for subkey enumeration.");
		return JS_FALSE;
	}

	RegQueryInfoKey(curKey, NULL, NULL, 0, &nSubKeys, &bufLen, NULL, NULL, NULL, NULL, NULL, NULL);
	if(nSubKeys == 0)
	{
		RegCloseKey(curKey);
		return JS_TRUE;
	}

	bufLen += 1;
	keyName = new WCHAR[bufLen];

	retvals = new jsval[nSubKeys];
	memset(retvals, JSVAL_VOID, sizeof(jsval) * nSubKeys);
	for(DWORD i = 0; i < nSubKeys; i++)
	{
		memset(keyName, 0, bufLen);
		DWORD nameLen = bufLen;
		RegEnumKeyEx(curKey, i, keyName, &nameLen, 0, NULL, NULL, NULL);
		if(wcslen(keyName) == 0)
			continue;
		JSString * copiedName = JS_NewUCString(cx, (jschar*)keyName, nameLen + 1);
		retvals[i] = STRING_TO_JSVAL(copiedName);
	}
	delete [] keyName;
	RegCloseKey(curKey);

	JSObject * retArray = JS_NewArrayObject(cx, nSubKeys, retvals);
	if(retArray == NULL)
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(retArray);
	return JS_TRUE;
}

JSBool xprep_js_enum_values(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * jsKeyName;
	jsdouble jsHive = 0;
	HKEY hive = NULL, curKey = NULL;
	DWORD nValues = 0, bufLen = 0;
	LPWSTR valueName = NULL;
	jsval * retvals = NULL;

	if(!JS_ConvertArguments(cx, argc, argv, "d S", &jsHive, &jsKeyName))
		return JS_FALSE;

	hive = ResolveRegConst(jsHive);

	RegOpenKeyEx(hive, (LPWSTR)JS_GetStringChars(jsKeyName), 0, KEY_READ, &curKey);
	if(curKey == NULL)
	{
		JS_ReportError(cx, "Error opening registry key for value enumeration.");
		return JS_FALSE;
	}
	
	RegQueryInfoKey(curKey,NULL, NULL, 0, NULL, NULL, NULL, &nValues, &bufLen, NULL, NULL, NULL);
	if(nValues == 0)
	{
		RegCloseKey(curKey);
		return JS_TRUE;
	}

	bufLen += 1;
	valueName = new WCHAR[bufLen];

	retvals = new jsval[nValues];
	memset(retvals, JSVAL_VOID, sizeof(jsval) * nValues);
	for(DWORD i = 0; i < nValues; i++)
	{
		memset(valueName, 0, bufLen);
		DWORD nameLen = bufLen;
		RegEnumValueW(curKey, i, valueName, &nameLen, 0, NULL, NULL, NULL);
		if(wcslen(valueName) == 0)
			continue;
		JSString * copiedName = JS_NewUCString(cx, (jschar*)valueName, nameLen + 1);
		retvals[i] = STRING_TO_JSVAL(copiedName);
	}
	delete [] valueName;
	RegCloseKey(curKey);

	JSObject * retArray = JS_NewArrayObject(cx, nValues, retvals);
	if(retArray == NULL)
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(retArray);
	return JS_TRUE;
}
JSBool xprep_js_query_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * subKeyName, *valueName;
	jsdouble jsHive;
	HKEY hive, curKey;

	LPBYTE data;
	DWORD dataLen = 0, dataType = 0;

	if(!JS_ConvertArguments(cx, argc, argv, "d S S", &jsHive, &subKeyName, &valueName))
		return JS_FALSE;

	hive = ResolveRegConst(jsHive);
	RegOpenKeyEx(hive, (LPWSTR)JS_GetStringChars(subKeyName), 0, KEY_READ, &curKey);
	if(curKey == NULL)
	{
		JS_ReportError(cx, "Unable to open registry key for querying a value.");
		return JS_FALSE;
	}

	jschar * jscValueName = JS_GetStringChars(valueName);
	RegQueryValueEx(curKey, (LPWSTR)jscValueName, 0, &dataType, NULL, &dataLen);

	switch(dataType)
	{
	case REG_DWORD:
		data = (LPBYTE)new DWORD;
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_MULTI_SZ:
		data = (LPBYTE)new WCHAR[dataLen + 1];
		break;
	default:
		RegCloseKey(curKey);
		JS_ReportError(cx, "Specified value is an unsupported type and cannot be queried.");
		return JS_FALSE;
	}
	
	RegQueryValueExW(curKey, (LPWSTR)jscValueName, 0, NULL, data, &dataLen);
	RegCloseKey(curKey);

	JSBool returnCode = JS_FALSE;
	JSString * strValue = NULL;
	switch(dataType)
	{
	case REG_DWORD:
		returnCode = JS_NewNumberValue(cx, (DWORD)*data, rval);
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:
		strValue = JS_NewUCString(cx, (jschar*)data, dataLen);
		if(strValue == NULL)
			break;
		returnCode = JS_TRUE;
		*rval = STRING_TO_JSVAL(strValue);
		break;
	case REG_MULTI_SZ:
		{
			WORD strCount = 0;
			LPWSTR lock = (LPWSTR)data;
			for(DWORD i = 0; i < dataLen; i++)
			{
				DWORD lockLength = wcslen(lock);
				if(lockLength != 0)
					strCount++;
				else
					break;
				lock += lockLength + 1;
			}
			jsval * strArrayVals = new jsval[strCount];
			lock = (LPWSTR)data;
			WORD j = 0;
			while(j < strCount)
			{
				DWORD curStrLen = wcslen(lock);
				JSString * newString = JS_NewUCString(cx, (jschar*)lock, curStrLen);
				strArrayVals[j++] = STRING_TO_JSVAL(newString);
				lock += curStrLen + 1;
			}
			JSObject * arrayObj = JS_NewArrayObject(cx, strCount, strArrayVals);
			if(arrayObj == NULL)
			{
				returnCode = JS_FALSE;
				break;
			}
			*rval = OBJECT_TO_JSVAL(arrayObj);
			returnCode = JS_TRUE;
		}
		break;
	}

	delete [] data;
	return returnCode;
}