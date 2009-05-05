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

enum { NAME_PROP, TAGNAME_PROP, VALUE_PROP, PARENTNODE_PROP, FIRSTCHILD_PROP, LASTCHILD_PROP, PREVIOUSSIBLING_PROP, NEXTSIBLING_PROP, CHILDREN_PROP, TEXT_PROP };

JSObject * lDOMNodeProto;
void finalizeElement(JSContext * cx, JSObject * obj)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	if(mNode != NULL)
		mNode->Release();
}

JSClass lDOMNodeClass  = {
	"DOMNode",  
    JSCLASS_HAS_PRIVATE, 
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, finalizeElement,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

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

	nsString value;
	if(idval == NAME_PROP)
		mNode->GetNodeName(value);
	else if(idval == VALUE_PROP)
		mNode->GetNodeValue(value);
	else if(idval == TAGNAME_PROP)
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
	JSString * valStr = JS_ValueToString(cx, *vp);
	jschar * valChars = JS_GetStringChars(valStr);

	nsString value;
	nsresult result;
	switch(JSVAL_TO_INT(idval))
	{
	case 3:
		result = mNode->SetNodeValue(nsDependentString(valChars));
		break;
	}
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
	idval = JSVAL_TO_INT(idval);
	nsIDOMNode * retNode = NULL;

	if(idval == PARENTNODE_PROP)
		mNode->GetParentNode(&retNode);
	else if(idval == FIRSTCHILD_PROP)
		mNode->GetFirstChild(&retNode);
	else if(idval == LASTCHILD_PROP)
		mNode->GetLastChild(&retNode);
	else if(idval == PREVIOUSSIBLING_PROP)
		mNode->GetPreviousSibling(&retNode);
	else if(idval == NEXTSIBLING_PROP)
		mNode->GetNextSibling(&retNode);

	if(retNode == NULL)
		*vp = JSVAL_NULL;
	else
	{
		retNode->AddRef();
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

JSBool getElementText(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMNodeList * children;
	mNode->GetChildNodes(&children);
	PRUint32 nChildren;
	children->GetLength(&nChildren);
	
	LPWSTR retString = (LPWSTR)JS_malloc(cx, (512 * sizeof(WCHAR)));
	memset(retString, 0, sizeof(WCHAR) * 512);
	DWORD retSCLength = 512;
	for(PRUint32 i = 0; i < nChildren; i++)
	{
		nsIDOMNode * curChild;
		children->Item(i, &curChild);
		
		PRUint16 type;
		curChild->GetNodeType(&type);
		if(type != nsIDOMNode::TEXT_NODE)
			continue;

		nsString curString;
		curChild->GetNodeValue(curString);
		if((wcslen(retString) + curString.Length() + 1) > retSCLength)
		{
			retSCLength += 128;
			retString = (LPWSTR)JS_realloc(cx, retString, retSCLength * sizeof(WCHAR));
		}
		wcscat_s(retString, retSCLength - wcslen(retString), curString.get());
		curChild->Release();
	}
	children->Release();

	JS_BeginRequest(cx);
	JSString * retJSString = JS_NewUCString(cx, retString, wcslen(retString));
	*vp = STRING_TO_JSVAL(retJSString);
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool setElementText(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	nsIDOMNode * mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsIDOMNodeList * children;
	mNode->GetChildNodes(&children);
	PRUint32 nChildren;
	children->GetLength(&nChildren);

	for(PRUint32 i = 0; i < nChildren; i++)
	{
		nsIDOMNode * curChild;
		children->Item(i, &curChild);
		PRUint16 curType;
		curChild->GetNodeType(&curType);
		if(curType == nsIDOMNode::TEXT_NODE)
			mNode->RemoveChild(curChild, &curChild);
		curChild->Release();
	}
	children->Release();

	JSString * newValue = JS_ValueToString(cx, *vp);
	nsCOMPtr<nsIDOMText> nNode;

	nsIDOMDocument *mDoc;
	mNode->GetOwnerDocument(&mDoc);
	mDoc->CreateTextNode(nsDependentString((LPWSTR)JS_GetStringChars(newValue)), getter_AddRefs(nNode));
	mDoc->Release();

	nsCOMPtr<nsIDOMNode> newNode = do_QueryInterface(nNode);
	nsCOMPtr<nsIDOMNode> outNode;
	if(mNode->AppendChild(newNode, getter_AddRefs(outNode)) != NS_OK)
		*vp = JSVAL_NULL;
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

	BYTE readOnlyProp = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

	JSPropertySpec nodeProps[] = {
		{ "name", NAME_PROP, readOnlyProp, stringPropGetter, NULL },
		{ "tagName", TAGNAME_PROP, readOnlyProp, stringPropGetter, NULL },
		{ "value", VALUE_PROP, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, stringPropGetter, stringPropSetter},
		{ "parentNode", PARENTNODE_PROP, readOnlyProp, nodePropGetter, NULL },
		{ "firstChild", FIRSTCHILD_PROP, readOnlyProp, nodePropGetter, NULL },
		{ "lastChild", LASTCHILD_PROP, readOnlyProp, nodePropGetter, NULL },
		{ "previousSibling", PREVIOUSSIBLING_PROP, readOnlyProp, nodePropGetter, NULL },
		{ "nextSibling", NEXTSIBLING_PROP, readOnlyProp, nodePropGetter, NULL },
		{ "children", CHILDREN_PROP, readOnlyProp, childGetter, NULL },
		{ "text", TEXT_PROP, JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED, getElementText, setElementText },
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