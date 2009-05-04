#include "stdafx.h"
#include "nsXULAppApi.h"
#include "nsXPComGlue.h"
#include "nsIDOMWindow2.h"
#include "nsIProxyObjectManager.h"
#include "nsProxiedService.h"

#include "nsCOMPtr.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsWeakReference.h"

#include "nsStringAPI.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMWindow2.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIDOMNamedNodeMap.h"
#include "nsIDOMAttr.h"
#include "nsIWebNavigation.h"
#include "webbrowserchrome.h"
#include "privatedata.h"

void finalizeElement(JSContext * cx, JSObject * obj)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	mNode->Release();
}

JSClass lDOMNodeClass  = {
	"DOMNode",  
    JSCLASS_HAS_PRIVATE, 
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizeElement,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * lDOMNodeProto;

JSBool buildNodeList(nsIDOMNodeList * nodeList, JSContext * cx, JSObject * parent, jsval * rval)
{
	JSObject * nodeArray = JS_NewArrayObject(cx,0, NULL);
	if(!nodeArray)
		return JS_FALSE;
	*rval = OBJECT_TO_JSVAL(nodeArray);

	PRUint32 nNodes;
	nodeList->GetLength(&nNodes);
	for(PRUint32 i = 0; i < nNodes; i++)
	{
		nsIDOMNode * curNode;
		nodeList->Item(i, &curNode);

		JSObject * curNodeObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, parent);
		JS_SetPrivate(cx, curNodeObj, curNode);
		jsval curNodeVal = OBJECT_TO_JSVAL(curNodeObj);
		JS_DefineElement(cx, nodeArray, i, curNodeVal, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY);
	}
	return JS_TRUE;
}
JSBool setAttribute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMElement * mElement = NULL;
	mNode->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)&mElement);
	if(mElement == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	LPWSTR name, value;
	if(!JS_ConvertArguments(cx, argc, argv, "W W", &name, &value))
	{
		JS_ReportError(cx, "Error parsing arguments in set attribute");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	nsDependentString aName(name), aValue(value);
	mElement->SetAttribute(aName, aValue);
	return JS_TRUE;
}

JSBool removeAttribute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMElement * mElement = NULL;
	mNode->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)&mElement);
	if(mElement == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	LPWSTR name;
	if(!JS_ConvertArguments(cx, argc, argv, "W", &name))
	{
		JS_ReportError(cx, "Error parsing arguments in set attribute");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	nsDependentString aName(name);
	mElement->RemoveAttribute(aName);
	return JS_TRUE;
}

JSBool hasAttribute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMElement * mElement;
	mNode->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)&mElement);
	if(mElement == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	LPWSTR name;
	if(!JS_ConvertArguments(cx, argc, argv, "W", &name))
	{
		JS_ReportError(cx, "Error parsing arguments in set attribute");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	JS_EndRequest(cx);

	nsDependentString aName(name);
	PRBool hasAttribute;
	mElement->HasAttribute(aName, &hasAttribute);
	*rval = hasAttribute ? JSVAL_TRUE : JSVAL_FALSE;
	return JS_TRUE;
}

JSBool stringPropGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMElement * mElement = NULL;
	mNode->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)&mElement);
	JSString * idvalStr = JS_ValueToString(cx, idval);
	jschar * idvalChars = JS_GetStringChars(idvalStr);

	nsString value;
	if(wcscmp(idvalChars, TEXT("name")) == 0)
		mNode->GetNodeName(value);
	else if(wcscmp(idvalChars, TEXT("value")) == 0)
		mNode->GetNodeValue(value);
	if(wcscmp(idvalChars, TEXT("tagname")) == 0)
	{
		if(mElement)
			mElement->GetTagName(value);
	}
	if(mElement)
		mElement->Release();

	if(value.IsEmpty())
		*vp = JSVAL_NULL;
	else
	{
		JSString * retVal = JS_NewUCStringCopyZ(cx, (jschar*)value.get());
		*vp = STRING_TO_JSVAL(retVal);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool stringPropSetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	JSString * idvalStr = JS_ValueToString(cx, idval);
	jschar * idvalChars = JS_GetStringChars(idvalStr);
	JSString * valStr = JS_ValueToString(cx, *vp);
	jschar * valChars = JS_GetStringChars(valStr);

	nsString value;
	if(wcscmp(idvalChars, TEXT("value")) == 0)
		mNode->SetNodeValue(nsDependentString(valChars));
	JS_EndRequest(cx);

	return JS_TRUE;
}


JSBool getElementsByTagName(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMElement * mElement;
	mNode->QueryInterface(NS_GET_IID(nsIDOMElement), (void**)&mElement);
	if(mElement == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}
	LPWSTR tagName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &tagName))
	{
		JS_ReportError(cx, "error parsing arguments in getelements by tagname");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsDependentString aTagName(tagName);
	nsIDOMNodeList * mNodeList;
	mElement->GetElementsByTagName(aTagName, &mNodeList);
	mElement->Release();

	buildNodeList(mNodeList, cx, obj, rval);
	mNodeList->Release();
	JS_EndRequest(cx);
	return JS_TRUE;
}


JSBool nodePropGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	JSString * idvalStr = JS_ValueToString(cx, idval);
	jschar * idvalChars = JS_GetStringChars(idvalStr);

	nsIDOMNode * retNode = NULL;

	if(_wcsicmp(idvalChars, TEXT("parentNode")) == 0)
		mNode->GetParentNode(&retNode);
	else if(_wcsicmp(idvalChars, TEXT("firstChild")) == 0)
		mNode->GetFirstChild(&retNode);
	else if(_wcsicmp(idvalChars, TEXT("lastChild")) == 0)
		mNode->GetLastChild(&retNode);
	else if(_wcsicmp(idvalChars, TEXT("previousSibling")) == 0)
		mNode->GetPreviousSibling(&retNode);
	else if(_wcsicmp(idvalChars, TEXT("nextSibling")) == 0)
		mNode->GetNextSibling(&retNode);

	if(retNode == NULL)
		*vp = JSVAL_NULL;
	else
	{
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*vp = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, retNode);
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool childGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	JSString * idvalStr = JS_ValueToString(cx, idval);
	jschar * idvalChars = JS_GetStringChars(idvalStr);
	if(_wcsicmp(idvalChars, TEXT("childNodes")) == 0)
	{
		nsIDOMNodeList * nodeList;
		mNode->GetChildNodes(&nodeList);
		buildNodeList(nodeList, cx, obj, vp);
		nodeList->Release();
	}
	else
		*vp = JSVAL_NULL;
	JS_EndRequest(cx);
	return JS_TRUE;
}
JSBool insertBefore(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * newChildObj, *refChildObj;
	nsIDOMNode * newChild, *refChild, *mNode, *outNode;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o o", &newChildObj, &refChildObj))
	{
		JS_ReportError(cx, "Error converting arguments in insertBefore");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	if(!JS_InstanceOf(cx, newChildObj, &lDOMNodeClass, NULL) || !JS_InstanceOf(cx, refChildObj, &lDOMNodeClass, NULL))
	{
		JS_ReportWarning(cx, "Arguments to insertBefore must both be DOMNode objects.");
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	newChild = (nsIDOMNode*)JS_GetPrivate(cx, newChildObj);
	refChild = (nsIDOMNode*)JS_GetPrivate(cx, refChildObj);
	mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);

	if(mNode->InsertBefore(newChild, refChild, &outNode) == NS_OK)
	{
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, outNode);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}
JSBool replaceChild(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject * newChildObj, *refChildObj;
	nsIDOMNode * newChild, *refChild, *mNode, *outNode;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o o", &newChildObj, &refChildObj))
	{
		JS_ReportError(cx, "Error converting arguments in replaceChild");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	if(!JS_InstanceOf(cx, newChildObj, &lDOMNodeClass, NULL) || !JS_InstanceOf(cx, refChildObj, &lDOMNodeClass, NULL))
	{
		JS_ReportWarning(cx, "Arguments to replaceChild must both be DOMNode objects.");
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	newChild = (nsIDOMNode*)JS_GetPrivate(cx, newChildObj);
	refChild = (nsIDOMNode*)JS_GetPrivate(cx, refChildObj);
	mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);

	if(mNode->ReplaceChild(newChild, refChild, &outNode) == NS_OK)
	{
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, outNode);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool removeChild(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject *refChildObj;
	nsIDOMNode *refChild, *mNode, *outNode;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o", &refChildObj))
	{
		JS_ReportError(cx, "Error converting arguments in replaceChild");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	if(!JS_InstanceOf(cx, refChildObj, &lDOMNodeClass, NULL))
	{
		JS_ReportWarning(cx, "Arguments to replaceChild must both be DOMNode objects.");
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	refChild = (nsIDOMNode*)JS_GetPrivate(cx, refChildObj);
	mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);

	if(mNode->RemoveChild(refChild, &outNode) == NS_OK)
	{
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, outNode);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool appendChild(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JSObject *refChildObj;
	nsIDOMNode *refChild, *mNode, *outNode;

	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "o", &refChildObj))
	{
		JS_ReportError(cx, "Error converting arguments in replaceChild");
		JS_EndRequest(cx);
		return JS_FALSE;
	}
	
	if(!JS_InstanceOf(cx, refChildObj, &lDOMNodeClass, NULL))
	{
		JS_ReportWarning(cx, "Arguments to replaceChild must both be DOMNode objects.");
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	refChild = (nsIDOMNode*)JS_GetPrivate(cx, refChildObj);
	mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);

	if(mNode->AppendChild(refChild, &outNode) == NS_OK)
	{
		JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
		*rval = OBJECT_TO_JSVAL(retObj);
		JS_SetPrivate(cx, retObj, outNode);
	}
	else
		*rval = JSVAL_FALSE;
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSClass lDOMDocClass = {
	"DOMDocument",  
    JSCLASS_HAS_PRIVATE, 
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizeElement,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
JSObject * lDOMDocProto;

JSBool createElement(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMDocument * mDoc;
	mNode->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**)&mDoc);
	if(mDoc == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	LPWSTR tagName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &tagName))
	{
		mDoc->Release();
		JS_ReportError(cx, "Error parsing arguments in createElement");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsIDOMElement * nElement;
	mDoc->CreateElement(nsDependentString(tagName), &nElement);
	mDoc->Release();
	if(nElement == NULL)
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	nsIDOMNode * nNode;
	nElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&nNode);
	nElement->Release();

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool createTextNode(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsIDOMNode* mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMDocument * mDoc;
	mNode->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**)&mDoc);
	if(mDoc == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	LPWSTR tagName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &tagName))
	{
		mDoc->Release();
		JS_ReportError(cx, "Error parsing arguments in createElement");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsIDOMText * nText;
	mDoc->CreateTextNode(nsDependentString(tagName), &nText);
	mDoc->Release();
	if(nText == NULL)
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	nsIDOMNode * nNode;
	nText->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&nNode);
	nText->Release();

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool getElementByID(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMDocument * mDoc;
	mNode->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**)&mDoc);

	LPWSTR elementID;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &elementID))
	{
		mDoc->Release();
		JS_ReportError(cx, "Error parsing arguments in getElementByID");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsIDOMElement * rElement;
	mDoc->GetElementById(nsDependentString(elementID), &rElement);
	mDoc->Release();
	if(rElement == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	nsIDOMNode * nNode;
	rElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&nNode);
	rElement->Release();

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool docNodeGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMDocument * mDoc;
	mNode->QueryInterface(NS_GET_IID(nsIDOMDocument), (void**)&mDoc);

	nsIDOMElement * docElement;
	mDoc->GetDocumentElement(&docElement);
	nsIDOMNode * nNode;
	docElement->QueryInterface(NS_GET_IID(nsIDOMNode), (void**)&nNode);
	docElement->Release();
	mDoc->Release();

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*vp = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;	
}

JSBool initDOMNode(JSContext * cx, JSObject * global)
{
	JSFunctionSpec nodeFuncs[] = {
		{ "SetAttribute", setAttribute, 2, 0 },
		{ "RemoveAttribute", removeAttribute, 1, 0 },
		{ "HasAttribute", hasAttribute, 1, 0 },
		{ "GetElementsByTagName", getElementsByTagName, 1, 0 },
		{ "InsertBefore", insertBefore, 2, 0 },
		{ "ReplaceChild", replaceChild, 2, 0 },
		{ "RemoveChild", removeChild, 1, 0 },
		{ "AppendChild", appendChild, 1, 0 },
		{ 0 }
	};

	JSPropertySpec nodeProps[] = {
		{ "name", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, stringPropGetter, NULL },
		{ "tagName", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, stringPropGetter, NULL },
		{ "value", 0, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, stringPropGetter, stringPropSetter},
		{ "parentNode", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, nodePropGetter, NULL },
		{ "firstChild", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, nodePropGetter, NULL },
		{ "lastChild", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, nodePropGetter, NULL },
		{ "previousSibling", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, nodePropGetter, NULL },
		{ "nextSibling", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, nodePropGetter, NULL },
		{ "children", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, childGetter, NULL },
		{ 0 }
	};

	lDOMNodeProto = JS_InitClass(cx, global, NULL, &lDOMNodeClass, NULL, 0, nodeProps, nodeFuncs, NULL, NULL);

	JSFunctionSpec docFuncs[] = {
		{ "GetElementByID", getElementByID, 1, 0 },
		{ "CreateTextNode", createTextNode, 1, 0 },
		{ "CreateElement", createElement, 1, 0 },
		{ 0 }
	};

	JSPropertySpec docProps[] = {
		{ "documentElement", 0, JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, docNodeGetter, NULL },
		{ 0 }
	};

	lDOMDocProto = JS_InitClass(cx, global, lDOMNodeProto, &lDOMDocClass, NULL, 0, docProps, docFuncs, NULL, NULL);
	return JS_TRUE;
}