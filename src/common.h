//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: common.h
//==================================
#ifndef _COMMON_H_
#define _COMMON_H_
#include <algorithm> // for std::sort
#include <vector>
#include <string>
#include <set>

#include "sprtf.hxx"
#include "utils.hxx"

typedef std::vector<std::string> vSTR;

// MakePtr is a macro that allows you to easily add to values (including
// pointers) together without dealing with C's pointer arithmetic.  It
// essentially treats the last two parameters as DWORDs.  The first
// parameter is used to typecast the result to the appropriate pointer type.
// #define MakePtr( cast, ptr, addValue ) (cast)( (DWORD)(ptr) + (DWORD)(addValue))
#define MakePtr( cast, ptr, addValue ) (cast)( (BYTE *)(ptr) + (DWORD)(addValue))

void DumpHeader(PIMAGE_FILE_HEADER pImageFileHeader);
void DumpOptionalHeader32(PIMAGE_OPTIONAL_HEADER32 pImageOptionalHeader);
void DumpOptionalHeader64(PIMAGE_OPTIONAL_HEADER64 pImageOptionalHeader);
void DumpSectionTable(PIMAGE_SECTION_HEADER section, unsigned cSections, BOOL IsEXE, const char *caller);
LPVOID GetSectionPtr(PSTR name, PIMAGE_NT_HEADERS pNTHeader, BYTE *imageBase);
LPVOID GetPtrFromRVA( DWORD rva, PIMAGE_NT_HEADERS pNTHeader, BYTE *imageBase );
PIMAGE_SECTION_HEADER GetSectionHeader(PSTR name, PIMAGE_NT_HEADERS pNTHeader);
PIMAGE_SECTION_HEADER GetEnclosingSectionHeader(DWORD rva,
                                                PIMAGE_NT_HEADERS pNTHeader);
void DumpRawSectionData(PIMAGE_SECTION_HEADER section,
                        PVOID base,
                        unsigned cSections);
void DumpDebugDirectory(PIMAGE_DEBUG_DIRECTORY debugDir, DWORD size, BYTE *base);
void DumpCOFFHeader(PIMAGE_COFF_SYMBOLS_HEADER pDbgInfo);
extern void HexDump(PBYTE ptr, DWORD length, PBYTE pb = 0);

PSTR GetMachineTypeName( WORD wMachineType );

#if 0 // 000000000000000000000000000000000000000000000000000000000000000000
#define GetImgDirEntryRVA( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)

#define GetImgDirEntrySize( pNTHdr, IDE ) \
	(pNTHdr->OptionalHeader.DataDirectory[IDE].Size)
#endif // 000000000000000000000000000000000000000000000000000000000000000000

extern DWORD GetImgDirEntryRVA(PVOID pNTHdr, DWORD);
//    (Is32) ? ((PIMAGE_OPTIONAL_HEADER32)pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress) : \
//  	((PIMAGE_OPTIONAL_HEADER64)pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)

extern DWORD GetImgDirEntrySize(PVOID pNTHdr, DWORD IDE);
//    (Is32) ? ((PIMAGE_OPTIONAL_HEADER32)pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress) : \
//  	((PIMAGE_OPTIONAL_HEADER64)pNTHdr->OptionalHeader.DataDirectory[IDE].VirtualAddress)

extern BOOL DoesSectionExist(const char *sect);

extern void add_2_dll_list(char * pDLL);  // FIX20171017
extern void kill_dll_list();  // FIX20171017
extern void show_dll_list();  // FIX20171017

extern int IsAddressInRange(BYTE *bgn, int len);
extern int is_listed_machine_type(WORD wMachineType);

#endif // #ifndef _COMMON_H_
// eof
