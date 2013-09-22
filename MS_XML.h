#import <msxml4.dll> raw_interfaces_only
using namespace MSXML2;

// Macro that calls a COM method returning HRESULT value:
#define HRCALL(a, errmsg) \
do { \
    hr = (a); \
    if (FAILED(hr)) { \
        dprintf( "%s:%d  HRCALL Failed: %s\n  0x%.8x = %s\n", \
                __FILE__, __LINE__, errmsg, hr, #a ); \
        goto clean; \
    } \
} while (0)


// Helper function to create a DOM instance. 
IXMLDOMDocument * DomFromCOM();

// Helper function to append a child to a parent node:
void AppendChildToParent(IXMLDOMNode *pChild, IXMLDOMNode *pParent);

// Helper function that put output in stdout and debug window
// in Visual Studio:
void dprintf( char * format, ...);

// Helper function to append a whitespace text node to a 
// specified element:
void AddTextToNode(IXMLDOMDocument* pDom, IXMLDOMNode *pNode, BSTR Text);

VARIANT VariantString(BSTR str);

IXMLDOMElement * CreateRootElement(IXMLDOMDocument * pXMLDom, BSTR bstr); // ������� �������� �������

void CreateComment(IXMLDOMDocument * pXMLDom, BSTR bstr); // �������� ������� �����������.

// ������� ������� ��� �������� � ��������� ��� ��������
void CreateAttribute(IXMLDOMDocument * pXMLDom, IXMLDOMElement * pElement, BSTR Name, BSTR Value);

IXMLDOMElement * AppendElement(IXMLDOMDocument * pXMLDom, IXMLDOMNode * pNode, BSTR Name); // �������� �������

void AddTextContent(IXMLDOMElement * pElement, BSTR Text); // �������� ��������� ����������

BSTR GetAttribute(IXMLDOMNode * Node, BSTR Name);

IXMLDOMNode * GetChildByName(IXMLDOMNode * Node, BSTR Name);
