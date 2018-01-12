
#ifndef _SYNBOLTABLESUPPORT_H_
#define _SYNBOLTABLESUPPORT_H_

class COFFSymbolTable;

BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length);
void DumpSymbolTable( COFFSymbolTable *pSymTab );
void DumpMiscDebugInfo( PIMAGE_DEBUG_MISC PMiscDebugInfo );
void DumpCVDebugInfo( PDWORD pCVHeader );
void DumpLineNumbers(PIMAGE_LINENUMBER pln, DWORD count);

#endif
/* eof */