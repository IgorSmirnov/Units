#include "ms_xml.h"
#include <stdio.h>

// Helper function to create a DOM instance. 
IXMLDOMDocument * DomFromCOM()
{
   HRESULT hr;
   IXMLDOMDocument *pxmldoc = NULL;

    HRCALL( CoCreateInstance(__uuidof(DOMDocument40),
                      NULL,
                      CLSCTX_INPROC_SERVER,
                      __uuidof(IXMLDOMDocument),
                      (void**)&pxmldoc),
            "Create a new DOMDocument");

    HRCALL( pxmldoc->put_async(VARIANT_FALSE), "should never fail");
    HRCALL( pxmldoc->put_validateOnParse(VARIANT_FALSE), "should never fail");
    HRCALL( pxmldoc->put_resolveExternals(VARIANT_FALSE), "should never fail");
    HRCALL( pxmldoc->put_preserveWhiteSpace(VARIANT_TRUE), "should never fail");

   return pxmldoc;
clean:
   if (pxmldoc)
    {
      pxmldoc->Release();
    }
   return NULL;
}


// Helper function to append a child to a parent node:
void AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent)
{
   HRESULT hr;
   IXMLDOMNode *pNode=NULL;
    HRCALL(pParent->appendChild(pChild, &pNode), "");
clean:
    if (pNode) pNode->Release();
    pNode=NULL;
}

// Helper function that put output in stdout and debug window
// in Visual Studio:
void dprintf( char * format, ...)
{
    static char buf[1024];
    va_list args;
    va_start( args, format );
    vsprintf( buf, format, args );
    va_end( args);
    OutputDebugStringA( buf);
    printf("%s", buf);
}

// Helper function to append a whitespace text node to a 
// specified element:
void AddTextToNode(IXMLDOMDocument* pDom, IXMLDOMNode *pNode, BSTR Text)
{
   HRESULT hr;
   IXMLDOMText *pws=NULL;
   IXMLDOMNode *pBuf=NULL;
    HRCALL(pDom->createTextNode(Text,&pws), " ");
    HRCALL(pNode->appendChild(pws,&pBuf)," ");
clean:
    if (pws) pws->Release();
    pws=NULL;
    if (pBuf) pBuf->Release();
    pBuf=NULL;
}


VARIANT VariantString(BSTR str)
{
   VARIANT var;
   VariantInit(&var);
   V_BSTR(&var) = SysAllocString(str);
   V_VT(&var) = VT_BSTR;
   return var;
}

IXMLDOMElement * CreateRootElement(IXMLDOMDocument * pXMLDom, BSTR bstr) // Создать корневой элемент
{
   IXMLDOMElement * pRoot = NULL;
   HRESULT hr;
   HRCALL(pXMLDom->createElement(bstr, &pRoot), "");
   AppendChildToParent(pRoot, pXMLDom);
clean:
   return pRoot;
}

void CreateComment(IXMLDOMDocument * pXMLDom, BSTR Comment) // Добавить элемент комментария.
{
   Comment = SysAllocString(Comment);
   HRESULT hr;
   IXMLDOMComment * pc = NULL;
   HRCALL(pXMLDom->createComment(Comment, &pc), "");
   AppendChildToParent(pc, pXMLDom);
clean:
   SysFreeString(Comment);
   Comment = NULL;
   pc->Release();
   pc=NULL;
}

void CreateAttribute(IXMLDOMDocument * pXMLDom, IXMLDOMElement * pElement, BSTR Name, BSTR Value)// Создать атрибут для элемента, и присвоить ему значение
{
   IXMLDOMAttribute * pa = NULL;
   IXMLDOMAttribute * pa1 = NULL;
   HRESULT hr;
   BSTR bstr = SysAllocString(Name);
   VARIANT var = VariantString(Value);
   HRCALL(pXMLDom->createAttribute(bstr,&pa), "");
   HRCALL(pa->put_value(var), "");
   HRCALL(pElement->setAttributeNode(pa, &pa1), "");
clean:
   SysFreeString(bstr);
   bstr=NULL;
   if (pa1) {
      pa1->Release();
      pa1=NULL;
   }
   pa->Release();
   pa=NULL;
   VariantClear(&var);
}

IXMLDOMElement * AppendElement(IXMLDOMDocument * pXMLDom, IXMLDOMNode * pNode, BSTR Name) // Добавить элемент
{
   IXMLDOMElement * pe = NULL;
   //bstr = SysAllocString(Name);
   HRESULT hr;
   HRCALL(pXMLDom->createElement(Name, &pe),"");
   //SysFreeString(bstr);
   //bstr=NULL;
   AppendChildToParent(pe, pNode);
   //pe->Release();
   //pe=NULL;
clean:
   return pe;
}

void AddTextContent(IXMLDOMElement * pElement, BSTR Text) // Добавить текстовое содержимое
{
   HRESULT hr;
   HRCALL(pElement->put_text(Text), "");
clean:
   return;
}

char * WideToChar(BSTR s)
{
	int l = lstrlenW(s);
	char * r = new char[l + 1];
	r[l] = 0;
	if(!WideCharToMultiByte(CP_ACP, 0, s, l, r, l, 0, 0))
		dprintf("WideToChar error!");
	return r;
}

BSTR CharToWide(char * s)
{
	int l = lstrlen(s);
	BSTR r = new OLECHAR[2*l + 1];
	r[l] = 0;
	for(int x = 0; x < l; x++)
		r[x] = ((unsigned char)(s[x]) <= 0xB0) ? s[x] : ((unsigned char)(s[x]) + 0x350);
//	if(!MultiByteToWideChar(0, 0, s, l, r, 2*l + 1))
//		dprintf("CharToWide error!");
	return r;
}

   // Create a <node2> to hold a CDATA section.
   /*bstr = SysAllocString(L"node2");
   HRCALL(pXMLDom->createElement(bstr,&pe),"create <node2> ");
   SysFreeString(bstr);
   bstr=NULL;

   bstr = SysAllocString(L"<some mark-up text>");
   HRCALL(pXMLDom->createCDATASection(bstr,&pcd),"");
   SysFreeString(bstr);
   bstr=NULL;
   if (!pcd) goto clean;

   AppendChildToParent(pcd, pe);
   pcd->Release();
   pcd=NULL;
   
   // Append <node2> to <root>.
   AppendChildToParent(pe, pRoot);
   pe->Release();
   pe=NULL;*/

BSTR GetAttribute(IXMLDOMNode * Node, BSTR Name)
{
	IXMLDOMNamedNodeMap * Attributes;
	Node->get_attributes(&Attributes);
	IXMLDOMNode * Item;
	Attributes->getNamedItem(Name, &Item);
	Attributes->Release();
	if(!Item) return 0;
	VARIANT Result;
	Item->get_nodeValue(&Result);
	Item->Release();
	if(Result.bstrVal && !*(Result.bstrVal)) {SysFreeString(Result.bstrVal); return 0;}
	return Result.bstrVal;
}

IXMLDOMNode * GetChildByName(IXMLDOMNode * Node, BSTR Name)
{
	IXMLDOMNodeList * List;
	IXMLDOMNode * Item;
	if(FAILED(Node->get_childNodes(&List))) return 0;
	long l;
	if(FAILED(List->get_length(&l))) {List->Release(); return 0;};
	for(long x = 0; x < l; x++)
	{
		BSTR NodeName;
		if(FAILED(List->get_item(x, &Item))) {List->Release(); return 0;};
		if(FAILED(Item->get_nodeName(&NodeName))) {Item->Release(); List->Release(); return 0;};
		if(!wcscmp(NodeName, Name))
		{
			List->Release();
			SysFreeString(NodeName);
			return Item;
		}
		SysFreeString(NodeName);
		Item->Release();
	}
	List->Release();
	return 0;
}