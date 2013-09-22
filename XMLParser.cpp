//#include "stdafx.h"
#include "XMLParser.h"
//#include <string.h>
//#include <wchar.h>

///////////////// Символьные сущности //////////////////////////////////////////

namespace TXML
{

char Symbols[] = {"&<>'\""};

int SSizes[] = {3, 2, 2, 4, 4};

TCHAR InvChar[] = _T("A name contains an invalid character or whitespace at start.");
TCHAR Ueof[] = _T("Unexpected end of file found.");
TCHAR SFileOpenError[] = _T("TXML::TDocument: File open error.");

char    * SNames[] = { "amp", "lt", "gt", "apos", "quot", 0};
wchar_t * WNames[] = {L"amp",L"lt",L"gt",L"apos",L"quot", 0};

}