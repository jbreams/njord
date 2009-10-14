#include <windows.h>
#define XP_WIN
#define JS_THREADSAFE
#include <jsapi.h>
#include <jsstr.h>
#include <winldap.h>

void ldap_obj_finalize(JSContext * cx, JSObject * obj)
{
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess != NULL)
		ldap_unbind(sess);
	JS_EndRequest(cx);
}

JSClass ldap_class = { "LDAP", JSCLASS_HAS_PRIVATE, 
						JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
						JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ldap_obj_finalize,
						JSCLASS_NO_OPTIONAL_MEMBERS };
JSObject * ldap_proto;

JSBool js_ldap_connect(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR hostname;
	DWORD port;
	JSBool ssl = FALSE;
	
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W u /b", &hostname, &port, &ssl))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_connect");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	LDAP * sess = ldap_sslinit(hostname, port, (int)ssl);
	if(sess == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	
	JSObject * retObj = JS_NewObject(cx, &ldap_class, ldap_proto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, sess);
	JS_EndRequest(cx);
	return JS_TRUE;
}

void mod_obj_finalize(JSContext * cx, JSObject * obj)
{
	JS_BeginRequest(cx);
	LDAPMod * theMod = (LDAPMod*)JS_GetPrivate(cx, obj);
	if(theMod != NULL)
	{
		HeapFree(GetProcessHeap(), 0, theMod->mod_type);
		for(DWORD i = 0; i < (sizeof(theMod->mod_vals.modv_strvals) / sizeof(PWCHAR)) - 1; i++)
			HeapFree(GetProcessHeap(), 0, theMod->mod_vals.modv_strvals[i]);
		HeapFree(GetProcessHeap(), 0, theMod->mod_vals.modv_strvals);
		HeapFree(GetProcessHeap(), 0, theMod);
	}
	JS_EndRequest(cx);
}

JSClass mod_obj_class = { "LDAPMod", JSCLASS_HAS_PRIVATE, 
								JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
								JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, mod_obj_finalize,
								JSCLASS_NO_OPTIONAL_MEMBERS };
JSObject * mod_obj_proto;


JSBool js_ldap_alloc_mod(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LDAPMod * theMod;
	JS_BeginRequest(cx);

	if(argc < 3)
	{
		JS_ReportError(cx, "Must pass at least an attrib name, op type, and one value to ldap_alloc_mod");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	theMod = (LDAPMod*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LDAPMod));
	JS_ValueToECMAUint32(cx, argv[argc - 1], &theMod->mod_op);
	
	JSString * attribNameStr = JS_ValueToString(cx, argv[0]);
	DWORD structSize = (JS_GetStringLength(attribNameStr) + 1) * sizeof(WCHAR);
	theMod->mod_type = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, structSize);
	wcscpy_s(theMod->mod_type, structSize, (LPWSTR)JS_GetStringChars(attribNameStr));
	
	theMod->mod_vals.modv_strvals = (PWCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PWCHAR) * (argc - 1));
	DWORD x = 0;
	for(uintN i = 2; i < argc; i++)
	{
		JSString * attrValStr = JS_ValueToString(cx, argv[i]);
		JS_AddRoot(cx, attrValStr);
		DWORD bufferLen = (JS_GetStringLength(attrValStr) + 1) * sizeof(WCHAR);
		theMod->mod_vals.modv_strvals[x] = (PWCHAR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bufferLen);
		wcscpy_s(theMod->mod_vals.modv_strvals[x], bufferLen, (LPWSTR)JS_GetStringChars(attrValStr));
		JS_RemoveRoot(cx, attrValStr);
		x++;
	}
	
	JSObject * retObj = JS_NewObject(cx, &mod_obj_class, mod_obj_proto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, theMod);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_ldap_bind(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PWCHAR dn = NULL, cred = NULL;
	ULONG method = LDAP_AUTH_SIMPLE;
	
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_ConvertArguments(cx, argc, argv, "/W W u", &dn, &cred, &method))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_bind");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	JS_YieldRequest(cx);
	ULONG retCode = ldap_bind_s(sess, dn, cred, method);
	if(retCode == LDAP_SUCCESS)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		SetLastError(retCode);
	}
	return JS_TRUE;
}

JSBool js_ldap_unbind(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	ldap_unbind_s(sess);
	JS_SetPrivate(cx, obj, NULL);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_ldap_add(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PWCHAR dn;
	JSObject * arrayOfMods = NULL;
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_ConvertArguments(cx, argc, argv, "W o", &dn, &arrayOfMods))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_bind");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_IsArrayObject(cx, arrayOfMods))
	{
		JS_ReportError(cx, "Did not pass an array of mod objects!");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	unsigned int nMods = 0;
	JS_GetArrayLength(cx, arrayOfMods, (jsuint*)&nMods);
	LDAPMod ** passedMods = (LDAPMod**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LDAPMod*) * nMods);
	for(unsigned int i = 0; i < nMods; i++)
	{
		jsval curModObjVal;
		JS_GetElement(cx, arrayOfMods, i, &curModObjVal);
		JSObject * curModObj = NULL;
		JS_ValueToObject(cx, curModObjVal, &curModObj);
		if(!JS_InstanceOf(cx, curModObj, &mod_obj_class, NULL))
			continue;
		passedMods[i] = (LDAPMod*)JS_GetPrivate(cx, curModObj);
	}

	JS_YieldRequest(cx);
	ULONG retCode = ldap_add_s(sess, dn, passedMods);
	HeapFree(GetProcessHeap(), 0, passedMods);
	if(retCode == LDAP_SUCCESS)
		*rval = JSVAL_TRUE;
	else
	{
		SetLastError(retCode);
		*rval = JSVAL_FALSE;
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_ldap_compare(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PWCHAR dn, attr, value;
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_ConvertArguments(cx, argc, argv, "W W W", &dn, &attr, &value))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_compare");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	ULONG retCode = ldap_compare_s(sess, dn, attr, value);
	if(retCode == LDAP_COMPARE_TRUE)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		SetLastError(retCode);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_ldap_delete(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PWCHAR dn;
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_ConvertArguments(cx, argc, argv, "W", &dn))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_bind");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	ULONG retCode = ldap_delete_s(sess, dn);
	if(retCode == LDAP_COMPARE_TRUE)
		*rval = JSVAL_TRUE;
	else
	{
		*rval = JSVAL_FALSE;
		SetLastError(retCode);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_ldap_modify(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	PWCHAR dn;
	JSObject * arrayOfMods = NULL;
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_ConvertArguments(cx, argc, argv, "W o", &dn, &arrayOfMods))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_bind");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	if(!JS_IsArrayObject(cx, arrayOfMods))
	{
		JS_ReportError(cx, "Did not pass an array of mod objects!");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	unsigned int nMods = 0;
	JS_GetArrayLength(cx, arrayOfMods, (jsuint*)&nMods);
	LDAPMod ** passedMods = (LDAPMod**)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LDAPMod*) * nMods);
	for(unsigned int i = 0; i < nMods; i++)
	{
		jsval curModObjVal;
		JS_GetElement(cx, arrayOfMods, i, &curModObjVal);
		JSObject * curModObj = NULL;
		JS_ValueToObject(cx, curModObjVal, &curModObj);
		if(!JS_InstanceOf(cx, curModObj, &mod_obj_class, NULL))
			continue;
		passedMods[i] = (LDAPMod*)JS_GetPrivate(cx, curModObj);
	}

	JS_YieldRequest(cx);
	ULONG retCode = ldap_modify_s(sess, dn, passedMods);
	HeapFree(GetProcessHeap(), 0, passedMods);
	if(retCode == LDAP_SUCCESS)
		*rval = JSVAL_TRUE;
	else
	{
		SetLastError(retCode);
		*rval = JSVAL_FALSE;
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

struct results_data
{
	LDAP * sess;
	LDAPMessage * resultHandle;
	LDAPMessage * curResult;
};

void results_obj_finalize(JSContext * cx, JSObject * obj)
{
	JS_BeginRequest(cx);
	struct results_data * rd = (struct results_data*)JS_GetPrivate(cx, obj);
	if(rd != NULL)
	{
		if(rd->resultHandle != NULL)
			ldap_msgfree(rd->resultHandle);
		JS_free(cx, rd);
	}
	JS_EndRequest(cx);
}

JSClass ldap_results_class = { "LDAPResult", JSCLASS_HAS_PRIVATE, 
								JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
								JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, results_obj_finalize,
								JSCLASS_NO_OPTIONAL_MEMBERS };
JSObject * ldap_results_proto;

JSBool js_ldap_search(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	LDAP * sess = (LDAP*)JS_GetPrivate(cx, obj);
	if(sess == NULL)
	{
		JS_ReportError(cx, "Binding to an unconnected session.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	PWCHAR base, filter;
	ULONG scope = LDAP_SCOPE_ONELEVEL;
	JSBool attrsOnly = JS_FALSE;
	if(!JS_ConvertArguments(cx, argc, argv, "W u W * /b", &base, &scope, &filter, &attrsOnly))
	{
		JS_ReportError(cx, "Error parsing arguments in ldap_bind");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	PWCHAR * attrs = NULL;
	JS_EnterLocalRootScope(cx);
	if(JSVAL_IS_OBJECT(argv[3]) && JS_IsArrayObject(cx, JSVAL_TO_OBJECT(argv[3])))
	{
		JSObject * attrArray;
		JS_ValueToObject(cx, argv[3], &attrArray);
		jsuint nAttribs;
		JS_GetArrayLength(cx, attrArray, &nAttribs);
		attrs = (PWCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PWCHAR) * (nAttribs + 1));
		for(jsuint i = 0; i < nAttribs; i++)
		{
			jsval curStringVal;
			JS_GetElement(cx, attrArray, i, &curStringVal);
			JSString * curAttrString = JS_ValueToString(cx, curStringVal);
			attrs[i] = (PWCHAR)JS_GetStringChars(curAttrString);
		}
	}
	else if(JSVAL_IS_NULL(argv[3]))
		attrs = NULL;
	else
	{
		attrs = (PWCHAR*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PWCHAR) * 2);
		JSString * singleAttr = JS_ValueToString(cx, argv[3]);
		attrs[0] = (PWCHAR)JS_GetStringChars(singleAttr);
	}

	LDAPMessage * searchResult = NULL;
	ULONG retCode = ldap_search_s(sess, base, scope, filter, attrs, attrsOnly, &searchResult);
	JS_LeaveLocalRootScope(cx);
	if(retCode != LDAP_SUCCESS)
	{
		SetLastError(retCode);
		if(searchResult != NULL)
			ldap_msgfree(searchResult);
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	JSObject * retObj = JS_NewObject(cx, &ldap_results_class, ldap_results_proto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	struct results_data * rd = (struct results_data*)JS_malloc(cx, sizeof(struct results_data));
	memset(rd, 0, sizeof(struct results_data));
	rd->resultHandle = searchResult;
	rd->sess = sess;
	JS_SetPrivate(cx, retObj, rd);
	JS_EndRequest(cx);
	return JS_TRUE;
}


JSBool js_get_next_entry(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	struct results_data * rd = (struct results_data*)JS_GetPrivate(cx, obj);
	if(rd == NULL)
	{
		JS_ReportError(cx, "Trying to advance search result on uninitialized structure.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	if(rd->curResult == NULL)
		rd->curResult = ldap_first_entry(rd->sess, rd->resultHandle);
	else
		rd->curResult = ldap_next_entry(rd->sess, rd->curResult);
	if(rd->curResult == NULL)
		*rval = JSVAL_FALSE;
	else
		*rval = JSVAL_TRUE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_get_values(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	struct results_data * rd = (struct results_data*)JS_GetPrivate(cx, obj);
	if(rd == NULL || rd->curResult == NULL)
	{
		JS_ReportError(cx, "Trying to advance search result on uninitialized structure.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PWCHAR attr;
	if(!JS_ConvertArguments(cx, argc, argv, "W", &attr))
	{
		JS_ReportError(cx, "Must provide the name of an attribute to retrieve.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PWCHAR * values = ldap_get_values(rd->sess, rd->curResult, attr);
	ULONG nValues = ldap_count_values(values);
	if(nValues == 0)
		*rval = JSVAL_FALSE;
	else if(nValues == 1)
	{
		JSString * retStr = JS_NewUCStringCopyZ(cx, (jschar*)values[0]);
		*rval = STRING_TO_JSVAL(retStr);
	}
	else
	{
		JSObject * retArray = JS_NewArrayObject(cx, 0, NULL);
		*rval = OBJECT_TO_JSVAL(retArray);
		for(ULONG i = 0; i < nValues; i++)
		{
			JSString * curStr = JS_NewUCStringCopyZ(cx, (jschar*)values[i]);
			JS_DefineElement(cx, retArray, i, STRING_TO_JSVAL(curStr), NULL, NULL, 0);
		}
	}
	ldap_value_free(values);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_get_dn(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	struct results_data * rd = (struct results_data*)JS_GetPrivate(cx, obj);
	if(rd == NULL || rd->curResult == NULL)
	{
		JS_ReportError(cx, "Trying to advance search result on uninitialized structure.");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PWCHAR dn = ldap_get_dn(rd->sess, rd->curResult);
	JSString * retStr = JS_NewUCStringCopyZ(cx, (jschar*)dn);
	*rval = STRING_TO_JSVAL(retStr);
	ldap_memfree(dn);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool js_explode_dn(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	PWCHAR dn;
	JSBool noTypes = JS_FALSE;
	if(!JS_ConvertArguments(cx, argc, argv, "W/ b", &dn, &noTypes))
	{
		JS_ReportError(cx, "Must pass a dn to explode!");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	PWCHAR * parts = ldap_explode_dn(dn, noTypes);
	PWCHAR curPart = *parts;
	JSObject * retArray = JS_NewArrayObject(cx, 0, NULL);
	*rval = OBJECT_TO_JSVAL(retArray);
	jsuint index = 0;
	while(curPart != NULL)
	{
		JSString * curPartStr = JS_NewUCStringCopyZ(cx, (jschar*)curPart);
		JS_DefineElement(cx, retArray, index++, STRING_TO_JSVAL(curPartStr), NULL, NULL, 0);
		curPart++;
	}
	ldap_value_free(parts);
	JS_EndRequest(cx);
	return JS_TRUE;
}

#ifdef __cplusplus
extern "C" {
#endif
BOOL __declspec(dllexport) InitExports(JSContext * cx, JSObject * global)
{
	JSFunctionSpec ldapGlobalFuncs[] = {
		{ "ldap_connect", js_ldap_connect, 3, 0, 0 },
		{ "ldap_explode_dn", js_explode_dn, 1, 0, 0 },
		{ "ldap_alloc_mod", js_ldap_alloc_mod, 3, 0, 0 },
		{ 0 }
	};

	JSFunctionSpec ldapObjFuncs[] = {
		{ "add", js_ldap_add, 2, 0, 0 },
		{ "modify", js_ldap_modify, 2, 0, 0 },
		{ "delete", js_ldap_delete, 1, 0, 0 },
		{ "bind", js_ldap_bind, 3, 0, 0 },
		{ "compare", js_ldap_compare, 3, 0, 0 },
		{ "unbind", js_ldap_unbind, 0, 0, 0 },
		{ "search", js_ldap_search, 5, 0, 0 },
		{ 0 }
	};

	JSFunctionSpec ldapResultsFuncs[] = {
		{ "next", js_get_next_entry, 0, 0, 0 },
		{ "dn", js_get_dn, 0, 0, 0 },
		{ "values", js_get_values, 1, 0, 0 },
		{ 0 }
	};

	ldap_proto = JS_InitClass(cx, global, NULL, &ldap_class, NULL, 0, NULL, ldapObjFuncs, NULL, NULL);
	ldap_results_proto = JS_InitClass(cx, global, NULL, &ldap_results_class, NULL,0, NULL, ldapResultsFuncs, NULL, NULL);
	mod_obj_proto = JS_InitClass(cx, global, NULL, &mod_obj_class, NULL, 0, NULL, NULL, NULL, NULL);
	JS_DefineFunctions(cx, global, ldapGlobalFuncs);
	return TRUE;
}

BOOL __declspec(dllexport) CleanupExports(JSContext * cx, JSObject * global)
{
	return TRUE;
}
#ifdef __cplusplus
}
#endif