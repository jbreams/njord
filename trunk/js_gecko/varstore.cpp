#include "StdAfx.h"
#import <msxml6.dll>
#include "jsapi.h"
#include "jsstr.h"
#define VARSTORE_OBJ_DEFINED
#include "VarStore.h"

VarStore::VarStore(void)
{
	head = NULL;
	varCount = 0;
}

VarStore::~VarStore(void)
{
	struct storage * cur = head;
	while(cur != NULL)
	{
		if(cur->type == STRING)
			delete [] cur->string;
		delete [] cur->name;
		struct storage * curNext = cur->next;
		delete cur;
		cur = curNext;
	}
}

BOOL VarStore::AddVariable(LPTSTR name, LPTSTR string)
{
	DWORD len = 0;
	struct storage * newHead;
	BOOL existing = FALSE;
	newHead = GetVarObj(name, TRUE);
	if(newHead == NULL)
	{
		newHead = new struct storage;
		if(newHead == NULL)
			goto failure;
		memset(newHead, 0, sizeof(struct storage));
	}
	else
		existing = TRUE;
	if(newHead->name == NULL)
	{
		len = ::_tcslen(name);
		newHead->name = new TCHAR[len + 1];
		if(newHead->name == NULL)
			goto failure;
		::_tcscpy_s(newHead->name, len + 1, name);
	}
	len = ::_tcslen(string);
	newHead->string = new TCHAR[len + 1];
	if(newHead->string == NULL)
		goto failure;
	::_tcscpy_s(newHead->string, len + 1, string);
	if(existing == FALSE)
	{
		varCount++;
		newHead->next = head;
		head = newHead;
	}
	return TRUE;

failure:
	if(newHead == NULL || existing == TRUE)
		return FALSE;
	if(newHead->name != NULL)
		delete [] newHead->name;
	if(newHead->string != NULL)
		delete [] newHead->string;
	delete newHead;
	return FALSE;
}

BOOL VarStore::AddVariable(LPTSTR name, BOOL boolean)
{
	struct storage * newHead;
	BOOL existing = FALSE;
	newHead = GetVarObj(name, TRUE);
	if(newHead == NULL)
	{
		newHead = new struct storage;
		if(newHead == NULL)
			goto failure;
		memset(newHead, 0, sizeof(struct storage));
	}
	else
		existing = TRUE;
	if(newHead->name == NULL)
	{
		DWORD len = ::_tcslen(name);
		newHead->name = new TCHAR[len + 1];
		if(newHead->name == NULL)
			goto failure;
		::_tcscpy_s(newHead->name, len + 1, name);
	}
	newHead->type = BOOLEAN;
	newHead->boolean = boolean;
	if(existing == FALSE)
	{
		varCount++;
		newHead->next = head;
		head = newHead;
	}
	return TRUE;

failure:
	if(newHead == NULL || existing == TRUE)
		return FALSE;
	if(newHead->name != NULL)
		delete [] newHead->name;
	delete newHead;
	return FALSE;
}

BOOL VarStore::AddVariable(LPTSTR name, DWORD number)
{
	struct storage * newHead;
	BOOL existing = FALSE;
	newHead = GetVarObj(name, TRUE);
	if(newHead == NULL)
	{
		newHead = new struct storage;
		if(newHead == NULL)
			goto failure;
		memset(newHead, 0, sizeof(struct storage));
	}
	else
		existing = TRUE;
	if(newHead->name == NULL)
	{
		DWORD len = ::_tcslen(name);
		newHead->name = new TCHAR[len + 1];
		if(newHead->name == NULL)
			goto failure;
		::_tcscpy_s(newHead->name, len + 1, name);
	}
	newHead->type = NUMBER;
	newHead->number = number;
	if(existing == FALSE)
	{
		newHead->next = head;
		head = newHead;
		varCount++;
	}
	return TRUE;

failure:
	if(newHead == NULL || existing == TRUE)
		return FALSE;
	if(newHead->name != NULL)
		delete [] newHead->name;
	delete newHead;
	return FALSE;
}

VarStore::storage * VarStore::GetVarObj(LPTSTR name, BOOL wipe = FALSE)
{
	struct storage * cur = head;
	while(cur != NULL)
	{
		if(::_tcscmp(name, cur->name) == 0)
		{
			if(wipe == TRUE)
			{
				if(cur->string != NULL)
					delete [] cur->string;
				cur->boolean = FALSE;
				cur->number = 0;
			}
			return cur;
		}
		cur = cur->next;
	}
	return NULL;
}

JSBool xprep_varstore_getter(JSContext *cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	VarStore * curVarStore = (VarStore*)JS_GetPrivate(cx, obj);
	if(argc < 1 || !JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "No or invalid variabled name specified in GetVar");
		return JS_FALSE;
	}

	JSString * varName = JSVAL_TO_STRING(argv[0]);
	struct VarStore::storage * varObj = curVarStore->GetVarObj((LPWSTR)JS_GetStringChars(varName), FALSE);
	if(varObj == NULL)
		*rval = JSVAL_NULL;
	else
	{
		if(varObj->type == VarStore::BOOLEAN)
			*rval = BOOLEAN_TO_JSVAL(varObj->boolean);
		else if(varObj->type == VarStore::NUMBER)
			JS_NewNumberValue(cx, (jsdouble)varObj->number, rval);
		else if(varObj->type == VarStore::STRING)
		{
			JSString * copiedString = JS_NewUCStringCopyZ(cx, (jschar*)varObj->string);
			*rval = STRING_TO_JSVAL(copiedString);
		}
	}
	return JS_TRUE;
}

JSBool xprep_varstore_setter(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	VarStore * curVarStore = (VarStore*)JS_GetPrivate(cx, obj);
	if(argc < 2 || !JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "No or invalid variabled name specified in SetVar");
		return JS_FALSE;
	}
	
	JSString * varname = JSVAL_TO_STRING(argv[0]);
	BOOL addOK = FALSE;
	if(JSVAL_IS_STRING(argv[1]))
	{
		JSString * curVal = JS_ValueToString(cx, argv[1]);
		addOK = curVarStore->AddVariable((LPWSTR)JS_GetStringChars(varname), (LPWSTR)JS_GetStringChars(curVal));
	}
	else if(JSVAL_IS_NUMBER(argv[1]))
	{
		DWORD numVal = 0;
		JS_ValueToECMAUint32(cx, argv[1], &numVal);
		addOK = curVarStore->AddVariable((LPWSTR)JS_GetStringChars(varname), numVal);
	}
	else if(JSVAL_IS_BOOLEAN(argv[1]))
	{
		JSBool boolVal = JS_FALSE;
		JS_ValueToBoolean(cx, argv[1], &boolVal);
		addOK = curVarStore->AddVariable((LPWSTR)JS_GetStringChars(varname), boolVal);
	}
	else
	{
		*rval = JSVAL_NULL;
		JS_ReportError(cx, "Invalid variable type for varstore.");
		return JS_FALSE;
	}
	*rval = (JSBool)addOK;
	return JS_TRUE;
}

JSBool xprep_varstore_haser(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	VarStore * curVarStore = (VarStore*)JS_GetPrivate(cx, obj);
	if(argc < 1 || !JSVAL_IS_STRING(argv[0]))
	{
		JS_ReportError(cx, "No or invalid variabled name specified in HasVar");
		return JS_FALSE;
	}

	JSString * varName = JSVAL_TO_STRING(argv[0]);
	if(curVarStore->GetVarObj((LPWSTR)JS_GetStringChars(varName), FALSE) == NULL)
		*rval = JS_FALSE;
	else
		*rval = JS_TRUE;
	return JS_TRUE;

}

JSBool xprep_varstore_enum(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	VarStore * curVarStore = (VarStore *)JS_GetPrivate(cx, obj);
	if(curVarStore->varCount == 0)
	{
		*rval = JSVAL_NULL;
		return JS_TRUE;
	}
	jsval * arrayVals = (jsval*)JS_malloc(cx, sizeof(jsval)*curVarStore->varCount);
	struct VarStore::storage * curItem = curVarStore->head;
	DWORD j = 0;
	while(curItem != NULL)
	{
		JSString * curItemName = JS_NewUCStringCopyZ(cx, (jschar*)curItem->name);
		arrayVals[j++] = STRING_TO_JSVAL(curItemName);
		curItem = curItem->next;
	}

	JSObject * retArray = JS_NewArrayObject(cx, curVarStore->varCount, arrayVals);
	*rval = OBJECT_TO_JSVAL(retArray);
	return JS_TRUE;
}

void xprep_varstore_finalize(JSContext * cx, JSObject * obj)
{
	VarStore * curVarStore = (VarStore*)JS_GetPrivate(cx, obj);
	delete curVarStore;
}

JSBool xprep_varstore_construct(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	VarStore * curVarStore = new VarStore();
	JS_SetPrivate(cx, obj, (void*)curVarStore);
	return JS_TRUE;
}

JSClass varStoreClass = {
	"VarStore",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, xprep_varstore_finalize,
	JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * varStoreProto;

JSBool InitVarStore(JSContext * cx, JSObject * global)
{
	struct JSFunctionSpec varStoreClassMethods[] = {
		{"Get", xprep_varstore_getter, 1, 0, 0 },
		{"Set", xprep_varstore_setter, 2, 0, 0 },
		{"Has", xprep_varstore_haser, 1, 0, 0 },
		{"Enum", xprep_varstore_enum, 0, 0, 0 },
		{ 0, 0, 0, 0, 0 },
	};
	varStoreProto = JS_InitClass(cx, global, NULL, &varStoreClass, xprep_varstore_construct, 0, NULL, varStoreClassMethods, NULL, NULL);
	if(varStoreProto == NULL)
		return JS_FALSE;
	return JS_TRUE;
}