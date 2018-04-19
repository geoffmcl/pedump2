//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: extrnvar.h
//==================================
#ifndef _EXTRNVAR_H_
#define _EXTRNVAR_H_

// EXTRNVAR.H holds the "extern" definitions for global variables used
// through the program.

extern BOOL fShowRelocations;
extern BOOL fShowRawSectionData;
extern BOOL fShowSymbolTable;
extern BOOL fShowLineNumbers;
extern BOOL fShowIATentries;
extern BOOL fShowPDATA;
extern BOOL fShowResources;
extern BOOL fShowMachineType;

extern PIMAGE_COFF_SYMBOLS_HEADER g_pCOFFHeader;
extern PIMAGE_DEBUG_MISC g_pMiscDebugInfo;
extern PDWORD g_pCVHeader;

class COFFSymbolTable;
extern COFFSymbolTable * g_pCOFFSymbolTable;

extern char cMachineType[]; // [256] = { 0 };
extern int Is32;

#endif // #ifndef _EXTRNVAR_H_
/* eof */
