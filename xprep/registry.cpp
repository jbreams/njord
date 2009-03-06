#include "stdafx.h"
#include "njord.h"

struct JSConstDoubleSpec regConsts[] = {
	{ (LONGLONG)HKEY_LOCAL_MACHINE, "HKEY_LOCAL_MACHINE", 0, 0},
	{ (LONGLONG)HKEY_USERS, "HKEY_USERS", 0, 0 },
	{ (LONGLONG)HKEY_CURRENT_USER, "HKEY_CURRENT_USER", 0, 0 },
	{ (LONGLONG)HKEY_CURRENT_CONFIG, "HKEY_CURRENT_CONFIG", 0, 0},
	{ (LONGLONG)HKEY_CLASSES_ROOT, "HKEY_CLASSES_ROOT", 0, 0 },
	{ REG_DWORD, "REG_DWORD", 0, 0 },
	{ REG_EXPAND_SZ, "REG_EXPAND_SZ", 0, 0},
	{ REG_MULTI_SZ, "REG_MULTI_SZ", 0, 0 },
	{ REG_SZ, "REG_SZ", 0, 0 },
	{ KEY_ALL_ACCESS, "KEY_ALL_ACCESS", 0, 0 },
	{ KEY_CREATE_SUB_KEY, "KEY_CREATE_SUB_KEY", 0, 0 },
	{ KEY_ENUMERATE_SUB_KEYS, "KEY_ENUMERATE_SUB_KEYS", 0, 0 },
	{ KEY_EXECUTE, "KEY_EXECUTE", 0, 0 },
	{ KEY_NOTIFY, "KEY_NOTIFY", 0, 0},
	{ KEY_SET_VALUE, "KEY_SET_VALUE", 0, 0},
	{ KEY_WOW64_32KEY, "KEY_WOW64_32KEY", 0, 0 },
	{ KEY_WOW64_64KEY, "KEY_WOW64_64KEY", 0, 0 },
	{ KEY_WRITE, "KEY_WRITE", 0, 0 },
	{ KEY_READ, "KEY_READ", 0, 0 },
	{ KEY_QUERY_VALUE, "KEY_QUERY_VALUE", 0, 0},
	{ REG_CREATED_NEW_KEY, "REG_CREATED_NEW_KEY", 0, 0 },
	{ REG_OPENED_EXISTING_KEY, "REG_OPENED_EXISTING_KEY", 0, 0 },
	{ 0 },
};

void reg_cleanup(JSContext * cx, JSObject * obj)
{
	HKEY myKey = (HKEY)JS_GetPrivate(cx, obj);
	if(myKey != NULL)
		RegCloseKey(myKey);
}

JSClass regKeyClass = {
    "RegKey",  /* name */
    JSCLASS_HAS_PRIVATE,  /* flags */
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, reg_cleanup,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * regKeyProto = NULL;

JSBool reg_create_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * subKeyName = NULL;
	HKEY hive = HKEY_LOCAL_MACHINE, result;
	REGSAM desiredAccess = KEY_READ | KEY_WRITE;

	if(!JS_ConvertArguments(cx, argc, argv, "u S /u", &hive, &subKeyName, &desiredAccess))
	{
		JS_ReportError(cx, "Unable to parse arguments in reg_create_key");
		return JS_FALSE;
	}

	LONG resultCode = RegCreateKeyExW(hive, (LPWSTR)JS_GetStringChars(subKeyName), 0, NULL, 0, desiredAccess, NULL, &result, NULL);
	if(resultCode != ERROR_SUCCESS)
	{
		SetLastError(resultCode);
		*rval = JS_FALSE;
		return JS_TRUE;
	}

	if(JS_InstanceOf(cx, obj, &regKeyClass, NULL))
	{
		HKEY prevKey = (HKEY)JS_GetPrivate(cx, obj);
		if(prevKey != NULL)
			RegCloseKey(prevKey);
		JS_SetPrivate(cx, obj, result);
		*rval = JS_TRUE;
	}
	else
	{
		JSObject * newRegKey = JS_NewObject(cx, &regKeyClass, regKeyProto, obj);
		JS_SetPrivate(cx, newRegKey, result);
		*rval = OBJECT_TO_JSVAL(newRegKey);
	}
	return JS_TRUE;	
}

JSBool reg_open_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * subKeyName = NULL;
	HKEY hive = HKEY_LOCAL_MACHINE, result;
	REGSAM desiredAccess = KEY_READ | KEY_WRITE;

	if(!JS_ConvertArguments(cx, argc, argv, "u S /u", &hive, &subKeyName, &desiredAccess))
	{
		JS_ReportError(cx, "Unable to parse arguments in reg_create_key");
		return JS_FALSE;
	}

	LONG resultCode = RegOpenKeyEx(hive, (LPWSTR)JS_GetStringChars(subKeyName), 0, desiredAccess, &result);
	if(resultCode != ERROR_SUCCESS)
	{
		SetLastError(resultCode);
		*rval = JS_FALSE;
		return JS_TRUE;
	}

	if(JS_InstanceOf(cx, obj, &regKeyClass, NULL))
	{
		HKEY prevKey = (HKEY)JS_GetPrivate(cx, obj);
		if(prevKey != NULL)
			RegCloseKey(prevKey);
		JS_SetPrivate(cx, obj, result);
		*rval = JS_TRUE;
	}
	else
	{
		JSObject * newRegKey = JS_NewObject(cx, &regKeyClass, regKeyProto, obj);
		JS_SetPrivate(cx, newRegKey, result);
		*rval = OBJECT_TO_JSVAL(newRegKey);
	}
	return JS_TRUE;	
}

JSBool reg_set_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY thisKey = (HKEY)JS_GetPrivate(cx, obj);
	if(thisKey == NULL)
	{
		JS_ReportError(cx, "reg_set_value called on an uninitialized RegKey. Call RegOpenKey or RegCreateKey first!");
		return JS_FALSE;
	}

	if(argc < 2)
	{
		JS_ReportError(cx, "Not enough arguments for reg_set_value, must provide value name and the value itself.");
		return JS_FALSE;
	}

	JSString * valueName;
	DWORD valueType;
	if(JSVAL_IS_NUMBER(argv[1]))
		valueType = REG_DWORD;
	else if(JSVAL_IS_OBJECT(argv[1]) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[1])))
		valueType = REG_MULTI_SZ;
	else
		valueType = REG_SZ;

	if(!JS_ConvertArguments(cx, argc, argv, "S * /u", &valueName, &valueType))
	{
		JS_ReportError(cx, "unable to parse arguments in reg_set_value.");
		return JS_FALSE;
	}

	LPVOID data = NULL;
	DWORD dataSize;
	switch(valueType)
	{
	case REG_DWORD:
		data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD));
		JS_ValueToECMAUint32(cx, argv[1], (DWORD*)data);
		dataSize = sizeof(DWORD);
		break;
	case REG_MULTI_SZ:
		{
			JSObject * arrayObj;
			JS_ValueToObject(cx, argv[1], &arrayObj);
			DWORD nStrings = 0, curPos = 0, maxSize = sizeof(jschar) * 4096;
			LPWSTR retStr = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(jschar) * 4096);
			JS_GetArrayLength(cx, arrayObj, (jsuint*)&nStrings);
			for(DWORD i = 0; i < nStrings; i++)
			{
				jsval curVal;
				JS_GetElement(cx, arrayObj, i, &curVal);
				JSString * curStr = JS_ValueToString(cx, curVal);
				DWORD curStrLen = JS_GetStringLength(curStr);
				if(curPos + (curStrLen * sizeof(jschar)) >= maxSize)
				{
					maxSize += (1024 * sizeof(jschar));
					retStr = (LPWSTR)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, retStr, maxSize);
				}
				memcpy_s(retStr, maxSize - curPos, JS_GetStringChars(curStr), curStrLen * sizeof(jschar));
				curPos += (curStrLen + 1) * sizeof(jschar);
			}

			data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, curPos + (sizeof(jschar) * 2));
			memcpy_s(data, curPos + (sizeof(jschar) * 2), retStr, curPos);
			HeapFree(GetProcessHeap(), 0, retStr);
			dataSize = curPos + (sizeof(jschar) * 2);
		}
		break;
	default:
		{
			JSString * dataStr = JS_ValueToString(cx, argv[1]);
			DWORD strLen = (JS_GetStringLength(dataStr) + 1) * sizeof(jschar);
			data = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, strLen);
			memcpy_s(data, strLen, JS_GetStringChars(dataStr), JS_GetStringLength(dataStr));
			dataSize = strLen;
		}
		break;
	}

	LSTATUS result = RegSetValueExW(thisKey, (LPWSTR)JS_GetStringChars(valueName), 0, valueType, (BYTE*)data, dataSize);
	HeapFree(GetProcessHeap(), 0, data);
	if(result != ERROR_SUCCESS)
	{
		SetLastError(result);
		*rval = JS_FALSE;
	}
	else
		*rval = JS_TRUE;
	return JS_TRUE;
}

JSBool reg_delete_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY thisKey = (HKEY)JS_GetPrivate(cx, obj);
	if(thisKey == NULL)
	{
		JS_ReportError(cx, "reg_delete_value called on an uninitialized RegKey. Call RegOpenKey or RegCreateKey first!");
		return JS_FALSE;
	}
	if(argc == 0)
	{
		JS_ReportError(cx, "Insufficient arguments, must provide a value name.");
		return JS_FALSE;
	}
	JSString * valueName = JS_ValueToString(cx, argv[0]);
	LSTATUS result = RegDeleteValue(thisKey, (LPWSTR)JS_GetStringChars(valueName));
	if(result != ERROR_SUCCESS)
	{
		SetLastError(result);
		*rval = JS_FALSE;
	}
	else
		*rval = JS_TRUE;
	return JS_TRUE;
}

JSBool reg_query_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY thisKey = (HKEY)JS_GetPrivate(cx, obj);
	if(thisKey == NULL)
	{
		JS_ReportError(cx, "reg_delete_value called on an uninitialized RegKey. Call RegOpenKey or RegCreateKey first!");
		return JS_FALSE;
	}
	if(argc == 0)
	{
		JS_ReportError(cx, "Insufficient arguments, must provide a value name.");
		return JS_FALSE;
	}
	JSString * valueNameStr = JS_ValueToString(cx, argv[0]);
	
	DWORD valueType, valueSize;
	LPWSTR valueName = (LPWSTR)JS_GetStringChars(valueNameStr);
	RegQueryValueEx(thisKey, valueName, NULL, NULL, NULL, &valueSize);
	valueSize += 10;
	LPVOID valueData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, valueSize);
	LSTATUS status = RegQueryValueEx(thisKey, valueName, NULL, &valueType, (LPBYTE)valueData, &valueSize);
	if(status != ERROR_SUCCESS)
	{
		HeapFree(GetProcessHeap(), 0, valueData);
		SetLastError(status);
		*rval = JS_FALSE;
		return JS_TRUE;
	}

	*rval = JSVAL_NULL;
	switch(valueType)
	{
	case REG_DWORD:
		{
			DWORD numberValue;
			memcpy_s(&numberValue, sizeof(DWORD), valueData, sizeof(DWORD));
			JS_NewNumberValue(cx, numberValue, rval);
		}
		break;
	case REG_QWORD:
		{
			LONGLONG numberValue;
			memcpy_s(&numberValue, sizeof(LONGLONG), valueData, sizeof(LONGLONG));
			JS_NewNumberValue(cx, numberValue, rval);
		}
		break;
	case REG_MULTI_SZ:
		{
			JSObject * retArray = JS_NewArrayObject(cx, 0, NULL);
			JS_AddRoot(cx, retArray);
			LPWSTR curPos = (LPWSTR)valueData;
			DWORD i = 0;
			while(curPos + 1 != TEXT('\0'))
			{
				DWORD curStrLen = (wcslen(curPos) + 1) * sizeof(jschar);
				LPWSTR curStr = (LPWSTR)JS_malloc(cx, curStrLen);
				wcscpy_s(curStr, curStrLen, curPos);
				JSString * curStrObj = JS_NewUCString(cx, (jschar*)curStr, wcslen(curStr));
				jsval curStrVal = STRING_TO_JSVAL(curStrObj);
				JS_SetElement(cx, retArray, i++, &curStrVal);
				curPos += curStrLen;				
			}
			JS_RemoveRoot(cx, retArray);
			*rval = OBJECT_TO_JSVAL(retArray);
		}
		break;
	case REG_SZ:
	case REG_EXPAND_SZ:
	case REG_LINK:
		{
			JSString * newStr = JS_NewUCStringCopyZ(cx, (jschar*)valueData);
			*rval = STRING_TO_JSVAL(newStr);
		}
	}
	HeapFree(GetProcessHeap(), 0, valueData);
	return JS_TRUE;
}

JSBool reg_enum_values(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY curKey = (HKEY)JS_GetPrivate(cx, obj);
	DWORD nValues = 0, bufLen = 0;
	if(curKey == NULL)
	{
		JS_ReportError(cx, "reg_delete_value called on an uninitialized RegKey. Call RegOpenKey or RegCreateKey first!");
		return JS_FALSE;
	}

	JSObject * arrayObj = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(arrayObj);
	RegQueryInfoKey(curKey, NULL, NULL, NULL, NULL, NULL, NULL, &nValues, &bufLen, NULL, NULL, NULL);
	if(nValues == 0)
		return JS_TRUE;

	bufLen += 1;
	LPWSTR valueName = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufLen * sizeof(WCHAR));
	for(DWORD i = 0; i < nValues; i++)
	{
		memset(valueName, 0, bufLen);
		DWORD nameLen = bufLen;
		RegEnumValue(curKey, i, valueName, &nameLen, NULL, NULL, NULL, NULL);
		if(wcslen(valueName) == 0)
			continue;
		JSString * copiedName = JS_NewUCStringCopyN(cx, (jschar*)valueName, nameLen);
		jsval copiedNameVal = STRING_TO_JSVAL(copiedName);
		JS_SetElement(cx, arrayObj, i, &copiedNameVal);
	}
	HeapFree(GetProcessHeap(), 0, valueName);
	return JS_TRUE;
}

JSBool reg_enum_keys(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY curKey = (HKEY)JS_GetPrivate(cx, obj);
	DWORD nSubKeys = 0, bufLen = 0;
	if(curKey == NULL)
	{
		JS_ReportError(cx, "reg_delete_value called on an uninitialized RegKey. Call RegOpenKey or RegCreateKey first!");
		return JS_FALSE;
	}

	JSObject * arrayObj = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(arrayObj);
	RegQueryInfoKey(curKey, NULL, NULL, 0, &nSubKeys, &bufLen, NULL, NULL, NULL, NULL, NULL, NULL);
	if(nSubKeys == 0)
		return JS_TRUE;

	bufLen += 1;
	LPWSTR keyName = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufLen * sizeof(WCHAR));
	for(DWORD i = 0; i < nSubKeys; i++)
	{
		memset(keyName, 0, bufLen);
		DWORD nameLen = bufLen;
		RegEnumKeyEx(curKey, i, keyName, &nameLen, 0, NULL, NULL, NULL);
		if(wcslen(keyName) == 0)
			continue;
		JSString * copiedName = JS_NewUCStringCopyN(cx, (jschar*)keyName, nameLen);
		jsval copiedNameVal = STRING_TO_JSVAL(copiedName);
		JS_SetElement(cx, arrayObj, i, &copiedNameVal);
	}
	HeapFree(GetProcessHeap(), 0, keyName);
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

JSBool reg_delete_key(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSString * subKeyName;
	HKEY hive;

	if(!JS_ConvertArguments(cx, argc, argv, "u S", &hive, &subKeyName))
		return JS_FALSE;

	if(RegDelKeyRecurse(hive, (LPWSTR)JS_GetStringChars(subKeyName)) == FALSE)
		*rval = JS_FALSE;
	else
		*rval = JS_TRUE;
	return JS_TRUE;
}


JSBool reg_load_hive(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY hive;
	JSString * subKeyNameStr = NULL, * fileName;

	LPWSTR subKeyName = NULL;
	if(!JS_ConvertArguments(cx, argc, argv, "u S /S", &hive, &fileName, &subKeyNameStr))
	{
		JS_ReportError(cx, "Error parsing arguments in reg_load_hive");
		return JS_FALSE;
	}

	if(subKeyNameStr != NULL)
		subKeyName = (LPWSTR)JS_GetStringChars(subKeyNameStr); 

	*rval = (JSBool)RegLoadKey(hive, subKeyName, (LPWSTR)JS_GetStringChars(fileName));
	return JS_TRUE;
}

JSBool reg_unload_hive(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	HKEY hive;
	JSString * subKeyNameStr;

	LPWSTR subKeyName = NULL;
	if(!JS_ConvertArguments(cx, argc, argv, "u S", &hive, &subKeyNameStr))
	{
		JS_ReportError(cx, "Error parsing arguments in reg_load_hive");
		return JS_FALSE;
	}

	*rval = (JSBool)RegUnLoadKey(hive, (LPWSTR)JS_GetStringChars(subKeyNameStr));
	return JS_TRUE;
}

BOOL InitRegistry(JSContext * cx, JSObject * global)
{
	struct JSFunctionSpec regKeyMethods[] = {
		{ "Create", reg_create_key, 2, 0, 0 },
		{ "Open", reg_open_key, 2, 0, 0 },
		{ "SetValue", reg_set_value, 2, 0, 0 },
		{ "DeleteValue", reg_delete_value, 1, 0, 0 },
		{ "QueryValue", reg_query_value, 1, 0, 0 },
		{ "EnumValues", reg_enum_values, 0, 0, 0 },
		{ "EnumKeys", reg_enum_keys, 0, 0, 0 },
		{ 0 },
	};

	struct JSFunctionSpec regKeyGlobalFunctions[] = {
		{ "RegCreateKey", reg_create_key, 2, 0, 0 },
		{ "RegOpenKey", reg_open_key, 2, 0, 0 },
		{ "RegDeleteKey", reg_delete_key, 2, 0, 0 },
		{ "RegLoadHive", reg_load_hive, 2, 0, 0 },
		{ "RegUnloadHive", reg_unload_hive, 2, 0, 0 },
		{ 0 },
	};

	JS_DefineConstDoubles(cx, global, regConsts);
	regKeyProto = JS_InitClass(cx, global, NULL, &regKeyClass, NULL, 0, NULL, regKeyMethods, NULL, NULL);
	if(regKeyProto == NULL)
		return FALSE;
	return JS_DefineFunctions(cx, global, regKeyGlobalFunctions);
}