#include "stdafx.h"
#include "embed.h"
#include "Varstore.h"

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

WCHAR* Utf8ToWchar(const char* str)
{
    int len = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    WCHAR* result = new WCHAR[len];
    MultiByteToWideChar(CP_UTF8, 0, str, -1, result, len);
    return result;
}

char* WcharToUtf8(WCHAR* str)
{
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    char *result = new char[len];
    WideCharToMultiByte(CP_UTF8, 0, str, -1, result, len, NULL, NULL);
    return result;
}

void VarStore::SuckOutVars(void * MozViewPtr)
{
	MozView * view = (MozView*)MozViewPtr;
	nsIDOMWindow2 * DOMwnd = view->GetDOMWindow();
	nsIDOMDocument * document;
	DOMwnd->GetDocument(&document);
	nsIDOMNodeList * inputElements;
	document->GetElementsByTagName(NS_LITERAL_STRING("input"), &inputElements);
	PRUint32 nInputs = 0;
	inputElements->GetLength(&nInputs);
	for(PRUint32 i = 0; i < nInputs; i++)
	{
		nsIDOMNode * inputNode;
		nsIDOMHTMLInputElement * input;
		inputElements->Item(i, &inputNode);
		inputNode->QueryInterface(nsIDOMHTMLInputElement::GetIID(), (void**)&input);
		
		nsString type, value, name;
		BOOL checked = FALSE;

		input->GetType(type);
		input->GetName(name);

		if(type.Compare(NS_LITERAL_STRING("checkbox")) == 0)
		{
			input->GetChecked((PRBool*)&checked);
			AddVariable((LPWSTR)name.get(), checked);
		}
		else if(type.Compare(NS_LITERAL_STRING("radio")) == 0)
		{
			input->GetChecked((PRBool*)&checked);
			if(checked == TRUE)
			{
				input->GetValue(value);
				AddVariable((LPWSTR)name.get(), (LPWSTR)value.get());
			}
		}
		else
		{
			input->GetValue(value);
			AddVariable((LPWSTR)name.get(), (LPWSTR)value.get());
		}
		input->Release();
		inputNode->Release();
	}
	inputElements->Release();

	document->GetElementsByTagName(NS_LITERAL_STRING("textarea"), &inputElements);
	inputElements->GetLength(&nInputs);
	for(PRUint32 j = 0; j < nInputs; j++)
	{
		nsIDOMNode * inputNode;
		nsIDOMHTMLTextAreaElement * input;
		inputElements->Item(j, &inputNode);
		inputNode->QueryInterface(nsIDOMHTMLTextAreaElement::GetIID(), (void**)&input);
		
		nsString name, value;
		input->GetName(name);
		input->GetValue(value);

		AddVariable((LPWSTR)name.get(), (LPWSTR)value.get());
		input->Release();
		inputNode->Release();
	}
	inputElements->Release();

	document->GetElementsByTagName(NS_LITERAL_STRING("select"), &inputElements);
	inputElements->GetLength(&nInputs);
	for(PRUint32 j = 0; j < nInputs; j++)
	{
		nsString name, value;
		nsIDOMNode * inputNode;
		nsIDOMHTMLSelectElement * input;
		inputElements->Item(j, &inputNode);
		inputNode->QueryInterface(nsIDOMHTMLSelectElement::GetIID(), (void**)&input);
		input->GetName(name);
		PRInt32 selectedOption;
		input->GetSelectedIndex(&selectedOption);
		nsIDOMHTMLOptionsCollection * optionsCollection;
		input->GetOptions(&optionsCollection);
		
		nsIDOMNode * optionNode;
		optionsCollection->Item(selectedOption, &optionNode);
		nsIDOMHTMLOptionElement * option;
		optionNode->QueryInterface(nsIDOMHTMLOptionElement::GetIID(), (void**)&option);
		option->GetValue(value);
		option->Release();
		optionNode->Release();
		optionsCollection->Release();
		input->Release();
		inputNode->Release();
		AddVariable((LPWSTR)name.get(), (LPWSTR)value.get());
	}
	inputElements->Release();
	document->Release();
}

void VarStore::RecursePushIn(void * documentptr, void * parentPtr, void * listPtr)
{
	nsIDOMDocument * document = (nsIDOMDocument*)documentptr;
	nsIDOMNodeList * list = (nsIDOMNodeList*)listPtr;
	nsIDOMNode * parent = (nsIDOMNode*)parentPtr;

	DWORD count = 0;
	list->GetLength((PRUint32*)&count);
	for(DWORD i = 0; i < count; i++)
	{
		nsIDOMNode * curNode = NULL;
		nsString value, varName, tagName;
		list->Item(i, &curNode);
		PRBool hasChildren = FALSE;
		PRUint16 nodeType = 0;
		curNode->GetNodeType(&nodeType);
		if(nodeType != nsIDOMNode::ELEMENT_NODE)
			goto nextNode;
		
		nsIDOMElement * curElement = NULL;
		curNode->QueryInterface(nsIDOMElement::GetIID(), (void**)&curElement);
		curElement->GetNodeName(tagName);
		LPWSTR tagNameStr = (LPWSTR)tagName.get();
		curElement->GetAttribute(NS_LITERAL_STRING("name"), varName);
		if(varName.Length() == 0)
			curElement->GetAttribute(NS_LITERAL_STRING("id"), varName);
		if(varName.Length() == 0)
			curElement->GetAttribute(NS_LITERAL_STRING("varname"), varName);
		if(varName.Length() == 0)
		{
			curElement->Release();
			goto nextNode;
		}

		struct storage * varObj = GetVarObj((LPWSTR)varName.get(), FALSE);
		if(varObj != NULL)
		{
			if(varObj->type == STRING)
				value.Assign(varObj->string);
			else if(varObj->type == BOOLEAN)
				value.AssignLiteral(varObj->boolean == TRUE ? "true" : "false");
			else if(varObj->type == NUMBER)
			{
				LPWSTR tempStr = (LPWSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 20 * sizeof(WCHAR));
				_ltow_s(varObj->number, tempStr, 20, 10);
				value.Assign(tempStr);
				HeapFree(GetProcessHeap(), 0, tempStr);
			}
		}

		if(tagName.Compare(NS_LITERAL_STRING("INPUT")) == 0)
		{
			nsIDOMHTMLInputElement * inputElement;
			curElement->QueryInterface(nsIDOMHTMLInputElement::GetIID(), (void**)&inputElement);
			nsString inputType;
			inputElement->GetType(inputType);
			if(inputType.Compare(NS_LITERAL_STRING("checkbox")) == 0)
			{
				if(varObj->type == BOOLEAN && varObj->boolean == TRUE)
					inputElement->SetChecked(TRUE);
				else
					inputElement->SetChecked(FALSE);
			}
			if(inputType.Compare(NS_LITERAL_STRING("radio")) == 0)
			{
				nsString inputValue;
				inputElement->GetValue(inputValue);
				if(inputValue.Compare(value) == 0)
					inputElement->SetChecked(TRUE);
				else
					inputElement->SetChecked(FALSE);
			}
			else
				inputElement->SetValue(value);
			inputElement->Release();
		}
		else if(tagName.Compare(NS_LITERAL_STRING("TEXTAREA")) == 0)
		{
			nsIDOMHTMLTextAreaElement * textAreaElement;
			curElement->QueryInterface(nsIDOMHTMLTextAreaElement::GetIID(), (void**)&textAreaElement);
			textAreaElement->SetValue(value);
		}
		else if(tagName.Compare(NS_LITERAL_STRING("SELECT")) == 0)
		{
			nsIDOMHTMLSelectElement * selectElement;
			nsIDOMHTMLOptionsCollection * options;
			curElement->QueryInterface(nsIDOMHTMLSelectElement::GetIID(), (void**)&selectElement);
			selectElement->GetOptions(&options);
			DWORD nOptions;
			options->GetLength((PRUint32*)&nOptions);
			BOOL foundInSet = FALSE;
			for(DWORD x = 0; x < nOptions; x++)
			{
				nsString curOptionValue;
				nsIDOMNode * curOptionNode;
				options->Item(x, &curOptionNode);
				nsIDOMHTMLOptionElement * curOption;
				curOptionNode->QueryInterface(nsIDOMHTMLOptionElement::GetIID(), (void**)&curOption);
				curOption->GetValue(curOptionValue);
				if(curOptionValue == value)
				{
					selectElement->SetSelectedIndex(x);
					foundInSet = TRUE;
					curOption->Release();
					break;
				}
				curOption->Release();
			}
			if(!foundInSet)
			{
				nOptions++;
				options->SetLength(nOptions);
				
				nsIDOMNode * newOptionNode;
				nsIDOMHTMLOptionElement * newOptionElement;
				options->Item(nOptions - 1, &newOptionNode);
				newOptionNode->QueryInterface(nsIDOMHTMLOptionElement::GetIID(), (void**)&newOptionElement);
				newOptionElement->SetValue(value);
				newOptionElement->Release();
				newOptionNode->Release();
				selectElement->SetSelectedIndex(nOptions - 1);
			}
			options->Release();
			selectElement->Release();
		}
		else if(tagName.Compare(NS_LITERAL_STRING("VARSTORE")) == 0)
		{
			nsIDOMText * textNode;
			nsIDOMNode * oldNode;
			document->CreateTextNode(value, &textNode);
			parent->ReplaceChild(textNode, curNode, &oldNode);
			textNode->Release();
			oldNode->Release();
		}
		else
		{
			nsIDOMText * textNode;
			nsIDOMNode * oldNode;
			document->CreateTextNode(value, &textNode);
			curElement->AppendChild(textNode, &oldNode);
			textNode->Release();
			oldNode->Release();
		}
	
nextNode:

		curNode->HasChildNodes(&hasChildren);
		if(hasChildren)
		{
			nsIDOMNodeList * nextList = NULL;
			curNode->GetChildNodes(&nextList);
			RecursePushIn(document, curNode, nextList);
		}
		curNode->Release();
	}
	list->Release();
}

void VarStore::ParseIntoXML(void * mozViewPtr)
{
	MozView * view = (MozView*)mozViewPtr;
	nsIDOMWindow2 * DOMwnd = view->GetDOMWindow();
	nsIDOMDocument * document;
	DOMwnd->GetDocument(&document);

	nsIDOMNodeList * topList;
	nsIDOMElement * topElement;
	nsIDOMNode * topNode;

	document->GetChildNodes(&topList);
	document->GetDocumentElement(&topElement);
	topElement->QueryInterface(nsIDOMNode::GetIID(), (void**)&topNode);

	RecursePushIn(document, topNode, topList);
	topList->Release();
	topNode->Release();
	topElement->Release();
	document->Release();
}