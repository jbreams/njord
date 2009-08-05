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
#include "nsIDOM3Node.h"
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
		nsCOMPtr<nsIDOMNode> curNode;
		nodeList->Item(i, getter_AddRefs(curNode));

		JSObject * curNodeObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, parent);
		jsval curNodeVal = OBJECT_TO_JSVAL(curNodeObj);
		JS_DefineElement(cx, nodeArray, i, curNodeVal, NULL, NULL, JSPROP_ENUMERATE | JSPROP_READONLY);
		JS_SetPrivate(cx, curNodeObj, curNode);
	}
	return JS_TRUE;
}
JSBool setAttribute(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMElement> mElement = do_QueryInterface(mNode);
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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMElement> mElement = do_QueryInterface(mNode);
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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMElement> mElement = do_QueryInterface(mNode);
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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMElement> mElement = do_QueryInterface(mNode);

	nsString value;
	switch(JSVAL_TO_INT(idval))
	{
	case NAME_PROP:
		mNode->GetNodeName(value);
		break;
	case VALUE_PROP:
		mNode->GetNodeValue(value);
		break;
	case TAGNAME_PROP:
		if(mElement)
			mElement->GetTagName(value);
		break;
	case TEXT_PROP:
		{
			nsCOMPtr<nsIDOM3Node> l3Node = do_QueryInterface(mNode);
			if(l3Node != NULL)
				l3Node->GetTextContent(value);
		}
		break;
	}

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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	JSString * valStr = JS_ValueToString(cx, *vp);
	*vp = STRING_TO_JSVAL(valStr);
	jschar * valChars = JS_GetStringChars(valStr);

	nsString value(valChars);
	nsresult result;
	switch(JSVAL_TO_INT(idval))
	{
	case VALUE_PROP:
		result = mNode->SetNodeValue(value);
		break;
	case TEXT_PROP:
		{
			nsCOMPtr<nsIDOM3Node> l3Node = do_QueryInterface(mNode);
			if(l3Node != NULL)
				l3Node->SetTextContent(value);
		}
		break;
	}
	JS_EndRequest(cx);

	return JS_TRUE;
}

JSBool setNodeTextContent(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	JS_BeginRequest(cx);
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	if(argc == 0)
	{
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	JSString * contentStr = JS_ValueToString(cx, *argv);
	*argv = STRING_TO_JSVAL(contentStr);

	nsDependentString content(contentStr->chars, contentStr->length);
	PRBool hasChildren = PR_FALSE;
	mNode->HasChildNodes(&hasChildren);
	if(hasChildren)
	{
		nsCOMPtr<nsIDOMNodeList> children;
		mNode->GetChildNodes(getter_AddRefs(children));
		PRUint32 nChildren;
		children->GetLength(&nChildren);
		for(PRUint32 i = 0; i < nChildren; i++)
		{
			nsCOMPtr<nsIDOMNode> curChild, oldChild;
			if(NS_SUCCEEDED(children->Item(i, getter_AddRefs(curChild))))
				mNode->RemoveChild(curChild, getter_AddRefs(oldChild));
		}
	}

	nsCOMPtr<nsIDOMDocument> domDoc;
	mNode->GetOwnerDocument(getter_AddRefs(domDoc));
	nsCOMPtr<nsIDOMText> newText;
	domDoc->CreateTextNode(content, getter_AddRefs(newText));
	nsCOMPtr<nsIDOMNode> textNode = do_QueryInterface(newText);
	nsCOMPtr<nsIDOMNode> oldNode;
	mNode->AppendChild(textNode, getter_AddRefs(oldNode));
	JS_EndRequest(cx);
	return JS_TRUE;
}

JSBool getElementsByTagName(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsresult rv;
	nsCOMPtr<nsIDOMElement> mElement = do_QueryInterface(mNode, &rv);
	if(NS_FAILED(rv))
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
	buildNodeList(mNodeList, cx, obj, rval);
	mNodeList->Release();
	JS_EndRequest(cx);
	return JS_TRUE;
}


JSBool nodePropGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	JS_BeginRequest(cx);
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	idval = JSVAL_TO_INT(idval);
	nsCOMPtr<nsIDOMNode> retNode = NULL;

	if(idval == PARENTNODE_PROP)
		mNode->GetParentNode(getter_AddRefs(retNode));
	else if(idval == FIRSTCHILD_PROP)
		mNode->GetFirstChild(getter_AddRefs(retNode));
	else if(idval == LASTCHILD_PROP)
		mNode->GetLastChild(getter_AddRefs(retNode));
	else if(idval == PREVIOUSSIBLING_PROP)
		mNode->GetPreviousSibling(getter_AddRefs(retNode));
	else if(idval == NEXTSIBLING_PROP)
		mNode->GetNextSibling(getter_AddRefs(retNode));

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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
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
	nsCOMPtr<nsIDOMNode> newChild, refChild, mNode, outNode;

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

	if(mNode->InsertBefore(newChild, refChild, getter_AddRefs(outNode)) == NS_OK)
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
	nsCOMPtr<nsIDOMNode> newChild, refChild, mNode, outNode;

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

	if(mNode->ReplaceChild(newChild, refChild, getter_AddRefs(outNode)) == NS_OK)
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
	nsCOMPtr<nsIDOMNode> refChild, mNode, outNode;

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

	if(mNode->RemoveChild(refChild, getter_AddRefs(outNode)) == NS_OK)
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
	nsCOMPtr<nsIDOMNode> refChild, mNode, outNode;

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

	if(mNode->AppendChild(refChild, getter_AddRefs(outNode)) == NS_OK)
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
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMDocument>mDoc = do_QueryInterface(mNode);
	if(mDoc == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	LPWSTR tagName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &tagName))
	{
		JS_ReportError(cx, "Error parsing arguments in createElement");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsCOMPtr<nsIDOMElement> nElement;
	mDoc->CreateElement(nsDependentString(tagName), getter_AddRefs(nElement));
	if(nElement == NULL)
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	nsCOMPtr<nsIDOMNode>nNode = do_QueryInterface(nElement);

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool createTextNode(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMDocument>mDoc = do_QueryInterface(mNode);
	if(mDoc == NULL)
	{
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	LPWSTR tagName;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &tagName))
	{
		JS_ReportError(cx, "Error parsing arguments in createElement");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsCOMPtr<nsIDOMText> nText;
	mDoc->CreateTextNode(nsDependentString(tagName), getter_AddRefs(nText));
	if(nText == NULL)
	{
		JS_EndRequest(cx);
		*rval = JSVAL_FALSE;
		return JS_TRUE;
	}

	nsCOMPtr<nsIDOMNode> nNode = do_QueryInterface(nText);
	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool getElementByID(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval *rval)
{
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMDocument>mDoc = do_QueryInterface(mNode);

	LPWSTR elementID;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &elementID))
	{
		JS_ReportError(cx, "Error parsing arguments in getElementByID");
		JS_EndRequest(cx);
		return JS_FALSE;
	}

	nsCOMPtr<nsIDOMElement> rElement;
	mDoc->GetElementById(nsDependentString(elementID), getter_AddRefs(rElement));
	if(rElement == NULL)
	{
		*rval = JSVAL_FALSE;
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	nsCOMPtr<nsIDOMNode> nNode = do_QueryInterface(rElement);

	JSObject * retObj = JS_NewObject(cx, &lDOMNodeClass, lDOMNodeProto, obj);
	*rval = OBJECT_TO_JSVAL(retObj);
	JS_SetPrivate(cx, retObj, nNode);
	return JS_TRUE;
}

JSBool docNodeGetter(JSContext * cx, JSObject * obj, jsval idval, jsval * vp)
{
	nsCOMPtr<nsIDOMNode> mNode = (nsIDOMNode*)JS_GetPrivate(cx, obj);
	nsCOMPtr<nsIDOMDocument> mDoc = do_QueryInterface(mNode);
	nsCOMPtr<nsIDOMElement> docElement;
	mDoc->GetDocumentElement(getter_AddRefs(docElement));
	nsCOMPtr<nsIDOMNode> nNode = do_QueryInterface(docElement);

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
		{ "SetTextContent", setNodeTextContent, 1, 0 },
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
		{ "text", TEXT_PROP, JSPROP_ENUMERATE | JSPROP_PERMANENT, stringPropGetter, stringPropSetter },
		{ "textContent", 0, JSPROP_PERMANENT, NULL, NULL },
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