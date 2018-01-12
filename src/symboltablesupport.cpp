//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: COMMON.C
//==================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "common.h"
#include "coffsymboltable.h"
#include "extrnvar.h"
#include "cv_dbg.h"

//
// Dump the COFF debug information header
//
void DumpCOFFHeader(PIMAGE_COFF_SYMBOLS_HEADER pDbgInfo)
{
    SPRTF("COFF Debug Info Header\n");
    SPRTF("  NumberOfSymbols:      %08X\n", pDbgInfo->NumberOfSymbols);
    SPRTF("  LvaToFirstSymbol:     %08X\n", pDbgInfo->LvaToFirstSymbol);
    SPRTF("  NumberOfLinenumbers:  %08X\n", pDbgInfo->NumberOfLinenumbers);
    SPRTF("  LvaToFirstLinenumber: %08X\n", pDbgInfo->LvaToFirstLinenumber);
    SPRTF("  RvaToFirstByteOfCode: %08X\n", pDbgInfo->RvaToFirstByteOfCode);
    SPRTF("  RvaToLastByteOfCode:  %08X\n", pDbgInfo->RvaToLastByteOfCode);
    SPRTF("  RvaToFirstByteOfData: %08X\n", pDbgInfo->RvaToFirstByteOfData);
    SPRTF("  RvaToLastByteOfData:  %08X\n", pDbgInfo->RvaToLastByteOfData);
}


//
// Given a COFF symbol table index, look up its name.  This function assumes
// that the COFFSymbolCount and PCOFFSymbolTable variables have been already
// set up.
//
BOOL LookupSymbolName(DWORD index, PSTR buffer, UINT length)
{
	if ( !g_pCOFFSymbolTable )
		return FALSE;

	PCOFFSymbol pSymbol = g_pCOFFSymbolTable->GetSymbolFromIndex( index );

	if ( !pSymbol )
		return FALSE;

	lstrcpyn( buffer, pSymbol->GetName(), length );

	delete pSymbol;

    return TRUE;
}

//
// Dump a range of line numbers from the COFF debug information
//
void DumpLineNumbers(PIMAGE_LINENUMBER pln, DWORD count)
{
    char buffer[64];
    DWORD i;
    
    SPRTF("Line Numbers\n");
    
    for (i=0; i < count; i++)
    {
        if ( pln->Linenumber == 0 ) // A symbol table index
        {
            buffer[0] = 0;
            LookupSymbolName(pln->Type.SymbolTableIndex, buffer,
                            sizeof(buffer));
            SPRTF("SymIndex: %X (%s)\n", pln->Type.SymbolTableIndex,
                                             buffer);
        }
        else        // A regular line number
            SPRTF(" Addr: %05X  Line: %04u\n",
                pln->Type.VirtualAddress, pln->Linenumber);
        pln++;
    }
}


//
// Used by the DumpSymbolTable() routine.  It purpose is to filter out
// the non-normal section numbers and give them meaningful names.
//
void GetSectionName(WORD section, PSTR buffer, unsigned cbBuffer)
{
    char tempbuffer[10];
    
    switch ( (SHORT)section )
    {
        case IMAGE_SYM_UNDEFINED: strcpy(tempbuffer, "UNDEF"); break;
        case IMAGE_SYM_ABSOLUTE:  strcpy(tempbuffer, "ABS"); break;
        case IMAGE_SYM_DEBUG:     strcpy(tempbuffer, "DEBUG"); break;
        default: sprintf(tempbuffer, "%X", section);
    }
    
    strncpy(buffer, tempbuffer, cbBuffer-1);
}

//
// Dumps a COFF symbol table from an EXE or OBJ
//
void DumpSymbolTable( COFFSymbolTable *pSymTab )
{
    SPRTF( "Symbol Table - %X entries  (* = auxillary symbol)\n",
    		pSymTab->GetNumberOfSymbols() );


    SPRTF(
    "Indx Sectn Value    Type  Storage  Name\n"
    "---- ----- -------- ----- -------  --------\n");

	PCOFFSymbol pSymbol = pSymTab->GetNextSymbol( 0 );

	while ( pSymbol )
	{
	    char szSection[10];
        GetSectionName(pSymbol->GetSectionNumber(),szSection,sizeof(szSection));

        SPRTF( "%04X %5s %08X  %s %-8s %s\n",
        		pSymbol->GetIndex(), szSection, pSymbol->GetValue(),
        		pSymbol->GetTypeName(), pSymbol->GetStorageClassName(),
        		pSymbol->GetName() );

		if ( pSymbol->GetNumberOfAuxSymbols() )
		{
			char szAuxSymbol[1024];
			if (pSymbol->GetAuxSymbolAsString(szAuxSymbol,sizeof(szAuxSymbol)))
				SPRTF( "     * %s\n", szAuxSymbol );			
		}
		
		pSymbol = pSymTab->GetNextSymbol( pSymbol );

	}
}

void DumpMiscDebugInfo( PIMAGE_DEBUG_MISC pMiscDebugInfo )
{
	if ( IMAGE_DEBUG_MISC_EXENAME != pMiscDebugInfo->DataType )
	{
		SPRTF( "Unknown Miscellaneous Debug Information type: %u\n", 
				pMiscDebugInfo->DataType );
		return;
	}

	SPRTF( "Miscellaneous Debug Information\n" );
	SPRTF( "  %s\n", pMiscDebugInfo->Data );
}

// see: http://www.godevtool.com/Other/pdb.htm
// from the above page...
typedef struct tagmySDSR {
    DWORD sig;  // SDSR
    BYTE  guid[16]; // 16-byte Globally Unique Identifier
    DWORD age;
    BYTE  name[1];  // zero terminated string
}mySDSR, *pmySDSR;

void DumpCVDebugInfo( PDWORD pCVHeader )
{
	PPDB_INFO pPDBInfo;
    int i;
    BYTE b;
	SPRTF( "CodeView Signature: %08X\n", *pCVHeader );

    if ('01BN' == *pCVHeader)
    {
        // NB10 == PDB file info
        pPDBInfo = (PPDB_INFO)pCVHeader;

        SPRTF("  Offset: %08X  Signature: %08X  Age: %08X\n",
            pPDBInfo->Offset, pPDBInfo->sig, pPDBInfo->age);
        SPRTF("  File: %s\n", pPDBInfo->PdbName);

    } else {
        // other discovered/researched
        if ('SDSR' == (UINT32)*pCVHeader)
        {
            pmySDSR pd = (pmySDSR)pCVHeader;
            SPRTF("Visual Studio .NET debug info %.4s\n", pCVHeader);
            SPRTF("Age: %u, GUID: ", pd->age);
            for (i = 0; i < 16; i++) {
                b = pd->guid[i];
                if (i && (i < 9)) {
                    if (((i % 4) == 0) ||
                        ((i % 6) == 0) ||
                        ((i % 8) == 0)) {
                        SPRTF("-"); // add spacer at these points - not sure why!?
                    }
                }
                SPRTF("%02X", b);   // print the HEX
            }
            SPRTF("\n");    // terminate line
            SPRTF("File: %s\n", pd->name);  // should be the .PDB file name
        }
        else {
            // Could be NB09 or NB11 - See DUMP4 source
            // DumpCVSymbolTable((PBYTE)pCVHeader, g_pMappedFileBase);
            SPRTF("TODO: Unhandled CodeView Information format %.4s\n", pCVHeader);
        }
	}

}

