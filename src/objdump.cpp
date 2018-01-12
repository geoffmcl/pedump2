//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: OBJDUMP.C
//==================================

#include <windows.h>
#include <stdio.h>
#include "common.h"
#include "symboltablesupport.h"
#include "coffsymboltable.h"
#include "extrnvar.h"

typedef struct _i386RelocTypes
{
    WORD type;
    PSTR name;
} i386RelocTypes;

// ASCII names for the various relocations used in i386 COFF OBJs
i386RelocTypes i386Relocations[] = 
{
{ IMAGE_REL_I386_ABSOLUTE, "ABSOLUTE" },
{ IMAGE_REL_I386_DIR16, "DIR16" },
{ IMAGE_REL_I386_REL16, "REL16" },
{ IMAGE_REL_I386_DIR32, "DIR32" },
{ IMAGE_REL_I386_DIR32NB, "DIR32NB" },
{ IMAGE_REL_I386_SEG12, "SEG12" },
{ IMAGE_REL_I386_SECTION, "SECTION" },
{ IMAGE_REL_I386_SECREL, "SECREL" },
{ IMAGE_REL_I386_REL32, "REL32" }
};
#define I386RELOCTYPECOUNT (sizeof(i386Relocations) / sizeof(i386RelocTypes))

//
// Given an i386 OBJ relocation type, return its ASCII name in a buffer
//
void GetObjRelocationName(WORD type, PSTR buffer, DWORD cBytes)
{
    DWORD i;
    
    for ( i=0; i < I386RELOCTYPECOUNT; i++ )
        if ( type == i386Relocations[i].type )
        {
            strncpy(buffer, i386Relocations[i].name, cBytes);
            return;
        }
        
    sprintf( buffer, "???_%X", type);
}

//
// Dump the relocation table for one COFF section
//
int DumpObjRelocations(PIMAGE_RELOCATION pRelocs, DWORD count)
{
    DWORD i;
    char szTypeName[32];
    
    for ( i=0; i < count; i++ )
    {
        if (!IsAddressInRange((BYTE *)pRelocs, (int)sizeof(IMAGE_RELOCATION))) {
            SPRTF("TODO: DumpObjRelocations %u of %u PIMAGE_RELOCATION out of range - %p\n", i, count, pRelocs);
            return 1;
        }
        GetObjRelocationName(pRelocs->Type, szTypeName, sizeof(szTypeName));
        SPRTF("  Address: %08X  SymIndex: %08X  Type: %s\n",
                pRelocs->VirtualAddress, pRelocs->SymbolTableIndex,
                szTypeName);
        pRelocs++;
    }
    return 0;
}

//
// top level routine called from PEDUMP.C to dump the components of a
// COFF OBJ file.
//
void DumpObjFile( PIMAGE_FILE_HEADER pImageFileHeader )
{
    unsigned i;
    int iret = 0;
    PIMAGE_SECTION_HEADER pSections;
    
    DumpHeader(pImageFileHeader);
    SPRTF("\n");

    pSections = MakePtr(PIMAGE_SECTION_HEADER, (pImageFileHeader+1),
                            pImageFileHeader->SizeOfOptionalHeader);

    DumpSectionTable(pSections, pImageFileHeader->NumberOfSections, FALSE, "objdump");
    SPRTF("\n");

    if ( fShowRelocations )
    {
        unsigned max = pImageFileHeader->NumberOfSections;
        SPRTF("Show Relocations: count %u\n", max);
        if (max) {
            PIMAGE_SECTION_HEADER last = pSections + (max - 1);
            if (IsAddressInRange((BYTE *)last, (int)sizeof(IMAGE_SECTION_HEADER))) {
                for (i = 0; i < max; i++)
                {
                    if (pSections[i].PointerToRelocations == 0)
                        continue;
                    PIMAGE_RELOCATION pir = MakePtr(PIMAGE_RELOCATION, pImageFileHeader, pSections[i].PointerToRelocations);
                    WORD num = pSections[i].NumberOfRelocations;
                    if (IsAddressInRange((BYTE *)pir, (int)sizeof(IMAGE_RELOCATION))) {
                        SPRTF("Section %02X (%.8s) relocations\n", i, pSections[i].Name);
                        //iret = DumpObjRelocations( MakePtr(PIMAGE_RELOCATION, pImageFileHeader,
                        //                        pSections[i].PointerToRelocations),
                        //                    pSections[i].NumberOfRelocations );
                        iret = DumpObjRelocations(pir, num);
                        if (iret) {
                            SPRTF("TODO: Abandoned DumpObjRelocations() at %u of %u iterations...\n", i, max);
                            break;
                        }
                        SPRTF("\n");
                    }
                    else {
                        SPRTF("TODO: DUmpObjFile - Abandoned ShowRelocations at %u of %u iterations... PIMAGE_RELOCATION out of range - %p\n", i, max, pir);
                        break;

                    }
                }
            }
            else {
                SPRTF("TODO: last PIMAGE_SECTION_HEADER out of range - %p\n", last);
            }
        }
    }
     
    if ( fShowSymbolTable && pImageFileHeader->PointerToSymbolTable )
    {
        unsigned num = pImageFileHeader->NumberOfSymbols;
        PIMAGE_SYMBOL pv = MakePtr(PIMAGE_SYMBOL, pImageFileHeader, pImageFileHeader->PointerToSymbolTable);
        PSTR ps = (PSTR)(pv + num);
        if (IsAddressInRange((BYTE *)pv, (int)sizeof(IMAGE_SYMBOL))) {
            if (IsAddressInRange((BYTE *)ps, (int)16)) {

                if (g_pCOFFSymbolTable)
                    delete g_pCOFFSymbolTable;

                //g_pCOFFSymbolTable = new COFFSymbolTable(
                //			MakePtr(PVOID, pImageFileHeader, 
                //					pImageFileHeader->PointerToSymbolTable),
                //			pImageFileHeader->NumberOfSymbols );
                g_pCOFFSymbolTable = new COFFSymbolTable(pv, num);

                DumpSymbolTable(g_pCOFFSymbolTable);
            }
            else {
                SPRTF("TODO: DumpSymbolTable PSTR out of range - %p\n", ps);
            }
        }
        else {
            SPRTF("TODO: DumpSymbolTable PIMAGE_SYMBOL out of range - %p\n", pv);
        }
        SPRTF("\n");
    }

    if ( fShowLineNumbers )
    {
        WORD wcnt = pImageFileHeader->NumberOfSections;
        // Walk through the section table...
        SPRTF("Show Line Numbers: %u sections\n", wcnt);
        if (wcnt) {
            PIMAGE_SECTION_HEADER last = pSections + (wcnt - 1);
            if (IsAddressInRange((BYTE *)last, (int)sizeof(IMAGE_SECTION_HEADER))) {
                for (i = 0; i < wcnt; i++)
                {
                    // if there's any line numbers for this section, dump'em
                    if (pSections->NumberOfLinenumbers)
                    {
                        PIMAGE_LINENUMBER pln = MakePtr(PIMAGE_LINENUMBER, pImageFileHeader, pSections->PointerToLinenumbers);
                        WORD num = pSections->NumberOfLinenumbers;
                        if (IsAddressInRange((BYTE *)pln, (int)sizeof(IMAGE_LINENUMBER))) {
                            DumpLineNumbers(pln, num);
                            SPRTF("\n");
                        }
                        else {
                            SPRTF("TODO: Abandon ShowLineNumbers %u of %u - PIMAGE_LINENUMBER out of range - %p\n", i, wcnt, pln);
                            break;
                        }
                    }
                    pSections++;
                }
            }
            else {
                SPRTF("TODO: last  PIMAGE_SECTION_HEADER out of range - %p\n", last);
            }
        }
    }
    
    if ( fShowRawSectionData )
    {
        PIMAGE_SECTION_HEADER psh = (PIMAGE_SECTION_HEADER)(pImageFileHeader + 1);
        WORD nSects = pImageFileHeader->NumberOfSections;
        SPRTF("Show Raw Section Data: %u sections\n", nSects);
        if (IsAddressInRange((BYTE *)psh, (int)sizeof(IMAGE_SECTION_HEADER))) {
            if (nSects) {
                PIMAGE_SECTION_HEADER last = psh + (nSects - 1);
                if (IsAddressInRange((BYTE *)last, (int)sizeof(IMAGE_SECTION_HEADER))) {
                    DumpRawSectionData(psh, pImageFileHeader, nSects);
                }
                else {
                    SPRTF("TODO: last PIMAGE_SECTION_HEADER out of range - %p\n", last);
                }
            }
        }
        else {
            SPRTF("TODO: first PIMAGE_SECTION_HEADER out of range - %p\n", psh);
        }
    }

    if (g_pCOFFSymbolTable)
        delete g_pCOFFSymbolTable;
    g_pCOFFSymbolTable = 0;
}
