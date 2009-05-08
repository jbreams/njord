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
#include "nsIDOMHTMLInputElement.h"
#include "nsIDOMHTMLTextAreaElement.h"
#include "nsIDOMHTMLSelectElement.h"
#include "nsIDOMHTMLOptionElement.h"
#include "nsIDOMHTMLOptionsCollection.h"
#include "nsIDOMText.h"
#include "nsIWebNavigation.h"
#include "webbrowserchrome.h"
#include "privatedata.h"

JSBool g2_get_input_value(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval)
{
	LPWSTR jsNameStr;
	JS_BeginRequest(cx);
	if(!JS_ConvertArguments(cx, argc, argv, "W", &jsNameStr))
	{
		JS_ReportError(cx, "Error parsing arguments in g2_get_input_value");
		JS_EndRequest(cx);
		return JS_TRUE;
	}
	nsString nsNameStr;
	nsNameStr.Assign(jsNameStr);

	PrivateData * mPrivate = (PrivateData*)JS_GetPrivate(cx, obj);
	nsIDOMDocument * document = NULL;
	mPrivate->mDOMWindow->GetDocument(&document);
	nsIDOMNodeList * nodes;
	document->GetElementsByTagName(NS_LITERAL_STRING("input"), &nodes);
	PRUint32 nodesLen;
	nodes->GetLength(&nodesLen);
	*rval = JSVAL_NULL;
	for(PRUint32 i = 0; i < nodesLen; i++)
	{
		nsString type, name, value;
		PRBool checked;
		nsIDOMNode * curNode;
		nodes->Item(i, &curNode);
		nsIDOMHTMLInputElement * curInput;
		curNode->QueryInterface(nsIDOMHTMLInputElement::GetIID(), (void**)&curInput);
		curInput->GetName(name);
		if(name.Compare(nsDependentString(jsNameStr)) != 0)
		{
			curInput->Release();
			curNode->Release();
			continue;
		}
		
		curInput->GetType(type);
		if(type.Compare(NS_LITERAL_STRING("checkbox")) == 0)
		{
			curInput->GetChecked(&checked);
			*rval = checked ? JSVAL_TRUE : JSVAL_FALSE;
			break;
		}
		else if(type.Compare(NS_LITERAL_STRING("radio")) == 0)
		{
			curInput->GetChecked(&checked);
			if(checked)
				curInput->GetValue(value);
			else
			{
				curInput->Release();
				curNode->Release();
				continue;
			}
		}
		else
			curInput->GetValue(value);
		curInput->Release();
		curNode->Release();
		
		if(*rval == JSVAL_NULL)
		{
			JSString * valueStr = JS_NewUCStringCopyN(cx, (jschar*)value.get(), value.Length());
			*rval = STRING_TO_JSVAL(valueStr);
		}
		break;
	}
	nodes->Release();
	if(*rval != JSVAL_NULL)
	{
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	document->GetElementsByTagName(NS_LITERAL_STRING("textarea"), &nodes);
	nodes->GetLength(&nodesLen);
	for(PRUint32 i = 0; i < nodesLen; i++)
	{
		nsIDOMNode * curNode;
		nsIDOMHTMLTextAreaElement * curInput;
		nodes->Item(i, &curNode);
		curNode->QueryInterface(nsIDOMHTMLTextAreaElement::GetIID(), (void**)&curInput);
		nsString name, value;
		curInput->GetName(name);
		if(name.Compare(nsNameStr) != 0)
		{
			curInput->Release();
			curNode->Release();
			continue;
		}

		curInput->GetValue(value);
		curInput->Release();
		curNode->Release();
		JSString * valueStr = JS_NewUCStringCopyN(cx, (jschar*)value.get(), value.Length());
		*rval = STRING_TO_JSVAL(valueStr);
		break;
	}
	nodes->Release();
	if(*rval != JSVAL_NULL)
	{
		JS_EndRequest(cx);
		return JS_TRUE;
	}

	document->GetElementsByTagName(NS_LITERAL_STRING("select"), &nodes);
	nodes->GetLength(&nodesLen);
	for(PRUint32 i = 0; i < nodesLen; i++)
	{
		nsString name, value;
		nsIDOMNode * curNode;
		nsIDOMHTMLSelectElement * curInput;
		nodes->Item(i, &curNode);
		curNode->QueryInterface(nsIDOMHTMLSelectElement::GetIID(), (void**)&curInput);
		curInput->GetName(name);
		if(name.Compare(nsNameStr) != 0)
		{
			curInput->Release();
			curNode->Release();
			continue;
		}

		nsIDOMHTMLOptionsCollection * options;
		curInput->GetOptions(&options);
		PRInt32 selectedIndex;
		curInput->GetSelectedIndex(&selectedIndex);
		nsIDOMNode * optionNode;
		nsIDOMHTMLOptionElement * option;
		options->Item(selectedIndex, &optionNode);
		optionNode->QueryInterface(nsIDOMHTMLOptionElement::GetIID(), (void**)&option);
		option->GetValue(value);
		option->Release();
		optionNode->Release();
		options->Release();
		curInput->Release();
		curNode->Release();

		JSString * valueStr = JS_NewUCStringCopyN(cx, (jschar*)value.get(), value.Length());
		*rval = STRING_TO_JSVAL(valueStr);
		break;		
	}
	JS_EndRequest(cx);
	return JS_TRUE;
}