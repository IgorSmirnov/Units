#include "stdafx.h"
#include "perecords.h"
#include "binary.h"
#include <windows.h>

#define PreDefRec(r)
#define StartRec(r)		TFieldDesc FD_##r[] = {
#define EndRec(r)		TFieldDesc()};

#define f(t, n)			TFieldDesc(#n,     sizeof(t)),
#define fd(t, n, d)		TFieldDesc( d,     sizeof(t)),
#define fdt(t, n, d, o)	TFieldDesc( d,     sizeof(t), o),
#define a(t, n, c)		TArrFieldDesc<t>(#n, c * sizeof(t)),
#define ah(t, n, c)		TArrFieldDesc<t>(#n, c * sizeof(t), DT_HEX | DT_CHAR),
#define r(n, c)			TFieldDesc(c),
#define p(n)			f(void *, n)
#define pd(n, d)		fd(void *, n, d)
#define pc(t, n)		TPCharFieldDesc<t>(#n),
#define pcd(t, n, d)	TPCharFieldDesc<t>(d),
#define psd(s, n, d)	TFieldDesc( d, FD_##s),
#define psc(s, n)		TFieldDesc(#n, FD_##s, DT_PRECORD | DT_COUNTER),

/*
#define PreDefRec(r)
#define StartRec(r) FIELDDESC FD_##r[] = {
#define EndRec(r) {0, 0, 0}};
#define f(t, n)			{#n,      sizeof(t),     DT_HEX, 0, (get_String<t>)},
#define fd(t, n, d)		{ d,      sizeof(t),     DT_HEX, 0, (get_String<t>)},
#define a(t, n, c)		{#n,  sizeof(t) * c,     DT_HEX, 0, (get_AString<t>)},
#define r(n, c)			{ 0,	          c,          0, 0, 0},
#define p(n)			{#n, sizeof(void *),     DT_HEX, 0, get_String<void *>},
#define pd(n, d)		{ d, sizeof(void *),     DT_HEX, 0, get_String<void *>},
#define psd(s, n, d)	{ d,    sizeof(s *), DT_PRECORD, FD_##s, get_String<void *>},
#define psc(s, n)		{#n, sizeof(s *) + sizeof(int), DT_PRECORD, FD_##s, get_String<void *>},
*/
#include "PERecords.inc"

#undef PreDefRec
#undef StartRec
#undef EndRec
#undef f
#undef fd
#undef fdt
#undef a
#undef ah
#undef r
#undef p
#undef pd
#undef psd
#undef pc
#undef pcd















/*












RECFIELD RF_MZTITLE[] = {
	{"Signature", 2, DT_CHAR | DT_HEX},
	{"PartPag", 2, DT_HEX},
    {"Page count", 2, DT_HEX},
    {"Relo count", 2, DT_HEX},
    {"Header size", 2, DT_HEX},
    {"Minimum memory", 2, DT_HEX},
    {"Maximum memory", 2, DT_HEX},
	{"ReloSS", 2, DT_HEX},
    {"ExeSP", 2, DT_HEX},
    {"Check sum", 2, DT_HEX},
    {"ExeIP", 2, DT_HEX},
    {"ReloCS", 2, DT_HEX},
    {"Table offset", 2, DT_HEX},
    {"Overlay", 2, DT_HEX},
    {0, 4 * 8, 0},
	{"PE offset", 4, DT_HEX},
{0, 0, 0}};

RECFIELD RF_PETITLE[] = {
    {"Signature", 4, DT_CHAR | DT_HEX},
    {"CPU type", 2, DT_HEX},
    {"Number of objects", 2, DT_HEX},
    {"Date time", 4, DT_HEX},
    {0, 4 * 2, 0},
    {"NT length", 2, DT_HEX},
    {"Flags", 2, DT_HEX},
    {0, 2, 0},
    {"Linker version", 2, DT_HEX},
    {0, 4 * 3, 0},
    {"Entrypoint RVA", 4, DT_HEX},
    {0, 4 * 2, 0},
    {"Image base", 4, DT_HEX},
    {"Object align", 4, DT_HEX},
    {"File align", 4, DT_HEX},
    {"OS version", 4, DT_HEX},
    {"User version", 4, DT_HEX},
    {"NT version", 4, DT_HEX},
    {0, 4 * 1, 0},
    {"Program size", 4, DT_HEX},
    {"Full title size", 4, DT_HEX},
    {"Check sum", 4, DT_HEX},
    {"NT sub system", 2, DT_HEX},
    {"DLL flags", 2, DT_HEX},
    {"Stack size", 4, DT_HEX},
    {"Minimal stack size", 4, DT_HEX},
    {"Heap size", 4, DT_HEX},
    {"Minimal heap size", 4, DT_HEX},
    {0, 4 * 1, 0},
    {"Array size", 4, DT_HEX},
    {"Export table offset", 4, DT_HEX},
    {"Export table size", 4, DT_HEX},
    {"Import table offset", 4, DT_HEX},
    {"Import table size", 4, DT_HEX},
    {"Resource table offset", 4, DT_HEX},
    {"Resource table size", 4, DT_HEX},
    {"Exception table offset", 4, DT_HEX},
    {"Exception table size", 4, DT_HEX},
    {"WarrTabOffset", 4, DT_HEX},
    {"WarrTabSize", 4, DT_HEX},
    {"AddrTabOffset", 4, DT_HEX},
    {"AddrTabSize", 4, DT_HEX},
    {"Debug table offset", 4, DT_HEX},
    {"Debug table size", 4, DT_HEX},
    {"DescrTabOffset", 4, DT_HEX},
    {"DescrTabSize", 4, DT_HEX},
    {"MachSpecOffset", 4, DT_HEX},
    {"MachSpecSize", 4, DT_HEX},
    {"TLSOffset", 4, DT_HEX},
    {"TLSSize", 4, DT_HEX},
    {"LoadCFGOffset", 4, DT_HEX},
    {"LoadCFGSize", 4, DT_HEX},
    {0, 4 * 2, 0},
    {"IATOffset", 4, DT_HEX},
    {"IATSize", 4, DT_HEX},
	{0, 4 * 6, 0},
{0, 0, 0}};

RECFIELD RF_OBJECTTABLE[] = {
    {"Name", 8, DT_CHAR},
    {"Virtual size", 4, DT_HEX},
    {"RVA", 4, DT_HEX},
    {"Phisical size", 4, DT_HEX},
    {"Offset", 4, DT_HEX},
    {0, 4 * 3, 0},
    {"Flags", 4, DT_HEX},
{0, 0, 0}};

RECFIELD RF_IMPORTDIRECTORYTABLE[] = {
    {"Lookup offset", 4, DT_HEX},
    {"Time date stamp", 4, DT_HEX},
    {"Forward chain", 4, DT_HEX},
    {"Library name offset", 4, DT_HEX},
    {"Address table offset", 4, DT_HEX},
{0, 0, 0}};

RECFIELD RF_EXPORTDIRECTORYTABLE[] = {
    {"Flags", 4, DT_HEX},
    {"Time date stamp", 4, DT_HEX},
    {"Version", 4, DT_HEX},
    {"Name RVA", 4, DT_HEX},
    {"Ordinal base", 4, DT_HEX},
    {"Number of funcs", 4, DT_HEX},
    {"Number of names", 4, DT_HEX},
    {"Address table offset", 4, DT_HEX},
    {"Name pointers RVA", 4, DT_HEX},
    {"Ordinal table RVA", 4, DT_HEX},
{0, 0, 0}};

RECFIELD RF_EXPORTEDENTRY[] = {
	{"Имя", 4, DT_PCHAR},
	{"Ординал", 4, DT_UNSIGNED},
	{"Адрес входа", 4, DT_HEX},
{0, 0, 0}};



RECFIELD RF_IMPORTEDENTRY[] = {
	{"Имя", 4, DT_PCHAR},
	{"Ординал", 4, DT_UNSIGNED},
	{"Адрес связи", 4, DT_HEX},
	{"Library", 4, DT_PRECORD, RF_IMPORTEDLIB},
{0, 0, 0}};

RECFIELD RF_IMPORTEDLIB[] = {
	{"Библиотека", 4, DT_PCHAR},
	{"Entries", 8, DT_PRECORD, RF_IMPORTEDENTRY},
{0, 0, 0}};

















*/