//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: EXEDUMP.C
//==================================

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <Dbghelp.h>

#pragma hdrstop
#include "common.h"
#include "symboltablesupport.h"
#include "coffsymboltable.h"
#include "resdump.h"
#include "extrnvar.h"

/*
    from: https://msdn.microsoft.com/en-us/library/ms809762.aspx
    Table 10. IMAGE_EXPORT_DIRECTORY Format
    DWORD Characteristics - This field appears to be unused and is always set to 0.
    DWORD TimeDateStamp - The time/date stamp indicating when this file was created.
    WORD MajorVersion
    WORD MinorVersion - These fields appear to be unused and are set to 0.
    DWORD Name - The RVA of an ASCIIZ string with the name of this DLL.
    DWORD Base - The starting ordinal number for exported functions. For example, if the file exports functions with ordinal values of 
        10, 11, and 12, this field contains 10. To obtain the exported ordinal for a function, you need to add this value to the appropriate 
        element of the AddressOfNameOrdinals array.
    DWORD NumberOfFunctions - The number of elements in the AddressOfFunctions array. This value is also the number of functions 
        exported by this module. Theoretically, this value could be different than the NumberOfNames field (next), but actually they're always the same.
    DWORD NumberOfNames - The number of elements in the AddressOfNames array. This value seems always to be identical to the NumberOfFunctions field, 
        and so is the number of exported functions.
    PDWORD *AddressOfFunctions - This field is an RVA and points to an array of function addresses. The function addresses are the entry points (RVAs) 
        for each exported function in this module.
    PDWORD *AddressOfNames - This field is an RVA and points to an array of string pointers. The strings are the names of the exported functions in this module.
    PWORD *AddressOfNameOrdinals - This field is an RVA and points to an array of WORDs. The WORDs are the export ordinals of all the exported 
        functions in this module. However, don't forget to add in the starting ordinal number specified in the Base field.
    
    The layout of the export table is somewhat odd (see Figure 4 and Table 10). As I mentioned earlier, the requirements for exporting a function are 
    a name, an address, and an export ordinal. You'd think that the designers of the PE format would have put all three of these items into a structure, 
    and then have an array of these structures. Instead, each component of an exported entry is an element in an array. There are three of these 
    arrays (AddressOfFunctions, AddressOfNames, AddressOfNameOrdinals), and they are all parallel to one another. To find all the information about 
    the fourth function, you need to look up the fourth element in each array.

*/

int Is32 = 0;
PIMAGE_OPTIONAL_HEADER64 opt64 = 0;
PIMAGE_OPTIONAL_HEADER32 opt32 = 0;

void DumpExeDebugDirectory(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_DEBUG_DIRECTORY debugDir;
    PIMAGE_SECTION_HEADER header;
    DWORD va_debug_dir;
    DWORD size;
    
    va_debug_dir = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG);

    if ( va_debug_dir == 0 )
        return;

    // If we found a .debug section, and the debug directory is at the
    // beginning of this section, it looks like a Borland file
    header = GetSectionHeader(".debug", pNTHeader);
    if ( header && (header->VirtualAddress == va_debug_dir) )
    {
        debugDir = (PIMAGE_DEBUG_DIRECTORY)(header->PointerToRawData+base);
        size = GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG)
                * sizeof(IMAGE_DEBUG_DIRECTORY);
    }
    else    // Look for the debug directory
    {
        header = GetEnclosingSectionHeader( va_debug_dir, pNTHeader );
        if ( !header )
            return;

        size = GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_DEBUG );
    
        debugDir = MakePtr(PIMAGE_DEBUG_DIRECTORY, base,
                            header->PointerToRawData
							+ (va_debug_dir - header->VirtualAddress) );
    }

    DumpDebugDirectory( debugDir, size, base );
}


//
// Dump the imports table (the .idata section) of a PE file
//
/*
    Must account for the different structures - 32 vs 64 bit
    #ifdef _WIN64
    #define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG64
    #define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL64(Ordinal)
    typedef IMAGE_THUNK_DATA64              IMAGE_THUNK_DATA;
    typedef PIMAGE_THUNK_DATA64             PIMAGE_THUNK_DATA;
    #define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL64(Ordinal)
    typedef IMAGE_TLS_DIRECTORY64           IMAGE_TLS_DIRECTORY;
    typedef PIMAGE_TLS_DIRECTORY64          PIMAGE_TLS_DIRECTORY;
    #else
    #define IMAGE_ORDINAL_FLAG              IMAGE_ORDINAL_FLAG32
    #define IMAGE_ORDINAL(Ordinal)          IMAGE_ORDINAL32(Ordinal)
    typedef IMAGE_THUNK_DATA32              IMAGE_THUNK_DATA;
    typedef PIMAGE_THUNK_DATA32             PIMAGE_THUNK_DATA;
    #define IMAGE_SNAP_BY_ORDINAL(Ordinal)  IMAGE_SNAP_BY_ORDINAL32(Ordinal)
    typedef IMAGE_TLS_DIRECTORY32           IMAGE_TLS_DIRECTORY;
    typedef PIMAGE_TLS_DIRECTORY32          PIMAGE_TLS_DIRECTORY;
    #endif

 */

void DumpImportsSection32(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_IMPORT_DESCRIPTOR importDesc;
    PIMAGE_SECTION_HEADER pSection;
    PIMAGE_THUNK_DATA32 thunk, thunkIAT=0;
    PIMAGE_IMPORT_BY_NAME pOrdinalName;
    DWORD importsStartRVA;
	PSTR pszTimeDate;
    DWORD count = 0;
    DWORD totentries = 0;
    DWORD hdrcnt = 0;

    // Look up where the imports section is (normally in the .idata section)
    // but not necessarily so.  Therefore, grab the RVA from the data dir.
    importsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_IMPORT);
    if ( !importsStartRVA )
        return;

    // Get the IMAGE_SECTION_HEADER that contains the imports.  This is
    // usually the .idata section, but doesn't have to be.
    pSection = GetEnclosingSectionHeader( importsStartRVA, pNTHeader );
    if ( !pSection )
        return;

    importDesc = (PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA(importsStartRVA,pNTHeader,base);
    if (!importDesc) {
        SPRTF("TODO: (PIMAGE_IMPORT_DESCRIPTOR)GetPtrFromRVA failed RVA %08X\n", importsStartRVA);
        return;
    }
            
    SPRTF("Imports Table: thunk32\n");
    
    while ( 1 )
    {
        // See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
        if ( (importDesc->TimeDateStamp==0 ) && (importDesc->Name==0) )
            break;
        
        char *pimp = (char *)GetPtrFromRVA(importDesc->Name, pNTHeader, base);
        if (!pimp) {
            SPRTF("TODO: (char *)GetPtrFromRVA failed RVA %08X\n", importDesc->Name);
            break;
        }
        add_2_dll_list(pimp);

        count++;

        SPRTF("  %s\n", pimp );


        SPRTF("  OrigFirstThunk:  %08X (Unbound IAT)\n",
      			importDesc->Characteristics);

        if (importDesc->TimeDateStamp) {
            pszTimeDate = get_ctime_stg((time_t *)&importDesc->TimeDateStamp);
            SPRTF("  TimeDateStamp:   %08X", importDesc->TimeDateStamp);
            SPRTF(pszTimeDate ? " -> %s\n" : "\n", pszTimeDate);
        }

        SPRTF("  ForwarderChain:  %08X\n", importDesc->ForwarderChain);
        SPRTF("  First thunk RVA: %08X\n", importDesc->FirstThunk);
    
        DWORD dwthunk = importDesc->Characteristics;
        DWORD dwthunkIAT = importDesc->FirstThunk;

        if ( dwthunk == 0 )   // No Characteristics field?
        {
            // Yes! Gotta have a non-zero FirstThunk field then.
            dwthunk = dwthunkIAT;
            
            if ( dwthunk == 0 )   // No FirstThunk field?  Ooops!!!
                return;
        }
        
        // Adjust the pointer to point where the tables are in the
        // mem mapped file.
        thunk = (PIMAGE_THUNK_DATA32)GetPtrFromRVA(dwthunk, pNTHeader, base);
        if (!thunk) {
            SPRTF("TODO: (PIMAGE_THUNK_DATA32)GetPtrFromRVA failed RVA %08X\n", dwthunk);
            return;
        }

        thunkIAT = (PIMAGE_THUNK_DATA32)GetPtrFromRVA(dwthunkIAT, pNTHeader, base);
        if (!thunkIAT) {
            SPRTF("TODO: (PIMAGE_THUNK_DATA32)GetPtrFromRVA failed RVA %08X\n", dwthunkIAT);
            return;
        }
    
        SPRTF("  Ordn  Name\n");
        hdrcnt = 0;
        while ( 1 ) // Loop forever (or until we break out)
        {
            if ( thunk->u1.AddressOfData == 0 )
                break;

            hdrcnt++;
            totentries++;
            if ( thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG32 )
            {
                SPRTF( "  %4u", IMAGE_ORDINAL32(thunk->u1.Ordinal) );
            }
            else
            {
                DWORD dwOff = (DWORD)thunk->u1.AddressOfData;
                pOrdinalName = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(dwOff, pNTHeader, base);
                if (!pOrdinalName) {
                    SPRTF("TODO: (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA failed RVA %08X\n", dwOff);
                    break;
                }
                SPRTF("  %4u  %s", pOrdinalName->Hint, pOrdinalName->Name);
            }
            
			// If the user explicitly asked to see the IAT entries, or
			// if it looks like the image has been bound, append the address
            if ( fShowIATentries || importDesc->TimeDateStamp )
                SPRTF( " (Bound to: %08X)", thunkIAT->u1.Function );

            SPRTF( "\n" );

            thunk++;            // Advance to next thunk
            thunkIAT++;         // advance to next thunk
        }
        SPRTF("Done %u entries.\n", hdrcnt);
        importDesc++;   // advance to next IMAGE_IMPORT_DESCRIPTOR
        SPRTF("\n");
    }
    SPRTF("Done %u imports... total %u entries...\n", count, totentries);
}

void DumpImportsSection64(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
//    SPRTF("TODO: DumpImportsSection64 to be ported!\n");
    PIMAGE_IMPORT_DESCRIPTOR importDesc;
    PIMAGE_SECTION_HEADER pSection;
    PIMAGE_THUNK_DATA64 thunk, thunkIAT = 0;
    PIMAGE_IMPORT_BY_NAME pOrdinalName;
    DWORD importsStartRVA;
    PSTR pszTimeDate;
    DWORD count = 0;
    DWORD totentries = 0;
    DWORD hdrcnt = 0;

    // Look up where the imports section is (normally in the .idata section)
    // but not necessarily so.  Therefore, grab the RVA from the data dir.
    importsStartRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_IMPORT);
    if (!importsStartRVA)
        return;

    // Get the IMAGE_SECTION_HEADER that contains the imports.  This is
    // usually the .idata section, but doesn't have to be.
    pSection = GetEnclosingSectionHeader(importsStartRVA, pNTHeader);
    if (!pSection)
        return;

    importDesc = (PIMAGE_IMPORT_DESCRIPTOR) GetPtrFromRVA(importsStartRVA, pNTHeader, base);
    if (!importDesc) {
        SPRTF("TODO: (PIMAGE_IMPORT_DESCRIPTOR) GetPtrFromRVA failed with RVA %08X\n", importsStartRVA);
        return;
    }

    SPRTF("Imports Table: thunk64\n");

    while (1)
    {
        // See if we've reached an empty IMAGE_IMPORT_DESCRIPTOR
        if ((importDesc->TimeDateStamp == 0) && (importDesc->Name == 0))
            break;

        char *pimp = (char *)GetPtrFromRVA(importDesc->Name, pNTHeader, base);
        if (!pimp) {
            SPRTF("TODO: (char *)GetPtrFromRVA from RVA %08X\n", importDesc->Name);
            break;
        }

        add_2_dll_list(pimp);

        count++;
        SPRTF("  %s\n", pimp);


        SPRTF("  OrigFirstThunk:  %08X (Unbound IAT)\n",
            importDesc->Characteristics);

        if (importDesc->TimeDateStamp) {
            pszTimeDate = get_ctime_stg((time_t *)&importDesc->TimeDateStamp);
            SPRTF("  TimeDateStamp:   %08X", importDesc->TimeDateStamp);
            SPRTF(pszTimeDate ? " -> %s\n" : "\n", pszTimeDate);
        }

        SPRTF("  ForwarderChain:  %08X\n", importDesc->ForwarderChain);
        SPRTF("  First thunk RVA: %08X\n", importDesc->FirstThunk);

        DWORD dwthunk = importDesc->Characteristics;
        DWORD dwthunkIAT = importDesc->FirstThunk;

        if (dwthunk == 0)   // No Characteristics field?
        {
            // Yes! Gotta have a non-zero FirstThunk field then.
            dwthunk = dwthunkIAT;

            if (dwthunk == 0)   // No FirstThunk field?  Ooops!!!
                return;
        }

        // Adjust the pointer to point where the tables are in the
        // mem mapped file.
        thunk = (PIMAGE_THUNK_DATA64)GetPtrFromRVA(dwthunk, pNTHeader, base);
        if (!thunk) {
            SPRTF("TODO: (PIMAGE_THUNK_DATA64)GetPtrFromRVA from RVA %08X\n", dwthunk);
            return;
        }

        thunkIAT = (PIMAGE_THUNK_DATA64)GetPtrFromRVA(dwthunkIAT, pNTHeader, base);
        if (!thunkIAT) {
            SPRTF("TODO: (PIMAGE_THUNK_DATA64)GetPtrFromRVA failed with RVA %08X\n", dwthunkIAT);
            return;
        }

        SPRTF("  Ordn  Name\n");
        hdrcnt = 0;
        while (1) // Loop forever (or until we break out)
        {
            if (thunk->u1.AddressOfData == 0)
                break;

            hdrcnt++;
            totentries++;
            if (thunk->u1.Ordinal & IMAGE_ORDINAL_FLAG64)
            {
                SPRTF("  %4u", IMAGE_ORDINAL64(thunk->u1.Ordinal));
            }
            else
            {
                DWORD dwOff = (DWORD)thunk->u1.AddressOfData;
                pOrdinalName = (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA(dwOff, pNTHeader, base);
                if (!pOrdinalName) {
                    SPRTF("TODO: (PIMAGE_IMPORT_BY_NAME)GetPtrFromRVA failed RVA %08X\n", dwOff);
                    break;
                }

                SPRTF("  %4u  %s", pOrdinalName->Hint, pOrdinalName->Name);
            }

            // If the user explicitly asked to see the IAT entries, or
            // if it looks like the image has been bound, append the address
            if (fShowIATentries || importDesc->TimeDateStamp)
                SPRTF(" (Bound to: %08X)", thunkIAT->u1.Function);

            SPRTF("\n");

            thunk++;            // Advance to next thunk
            thunkIAT++;         // advance to next thunk
        }
        SPRTF("Done %u entries\n", hdrcnt);
        importDesc++;   // advance to next IMAGE_IMPORT_DESCRIPTOR
        SPRTF("\n");
    }
    SPRTF("Done %u imports... total %u entries...\n", count, totentries);
}

void DumpImportsSection(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
    if (Is32)
        DumpImportsSection32(base, pNTHeader);
    else
        DumpImportsSection64(base, pNTHeader);
}




PSTR GetSafeFileName(PSTR fn1, PSTR filename)
{
    PSTR cp = GetNxtBuf();
    int i, c, len, max = 256;
    int got0 = 0;
    *cp = 0;
    len = 0;
    if (IsAddressInRange((BYTE *)fn1, max)) {
        for (i = 0; i < max; i++) {
            c = fn1[i] & 0xff;
            if (c == 0) {
                got0 = 1;
                break;
            }
            else if ((c >= 0x20) && (c < 128)) {
                cp[len++] = c;
                cp[len] = 0;
            }
            else
                break;
        }
    }
    if (got0 && len)
        return cp;
    *cp = 0;
    len = 0;
    if (IsAddressInRange((BYTE *)fn1, max)) {
        for (i = 0; i < max; i++) {
            c = fn1[i] & 0xff;
            if (c == 0) {
                got0 = 1;
                break;
            }
            else if ((c >= 0x20) && (c < 128)) {
                cp[len++] = c;
                cp[len] = 0;
            }
            else
                break;
        }
    }
    if (got0 && len)
        return cp;
    cp = "N/A";
    return cp;
}


//
// Dump the exports table (usually the .edata section) of a PE file
//
void DumpExportsSection(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
    PIMAGE_EXPORT_DIRECTORY exportDir;
    PIMAGE_SECTION_HEADER header;
    INT delta; 
    PSTR filename;
    DWORD i;
    PDWORD functions;
    PWORD ordinals;
    // PSTR *name;
    DWORD exportsStartRVA, exportsEndRVA;
    
    exportsStartRVA = GetImgDirEntryRVA(pNTHeader,IMAGE_DIRECTORY_ENTRY_EXPORT);
    exportsEndRVA = exportsStartRVA +
	   				GetImgDirEntrySize(pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT);

    // Get the IMAGE_SECTION_HEADER that contains the exports.  This is
    // usually the .edata section, but doesn't have to be.
    header = GetEnclosingSectionHeader( exportsStartRVA, pNTHeader );
    if ( !header )
        return;

    delta = (INT)(header->VirtualAddress - header->PointerToRawData);
    
    // alternate coding, from : https://www.experts-exchange.com/questions/21066562/Listing-the-Export-Address-Table-EAT-IMAGE-EXPORT-DIRECTORY.html
    // but seems to FAIL!
    // IMAGE_DOS_HEADER *dosHeader = (IMAGE_DOS_HEADER *)base;
    // IMAGE_NT_HEADERS *ntHeaders = (IMAGE_NT_HEADERS *)(((BYTE *)dosHeader) + dosHeader->e_lfanew);
    // MAYBE changes 32 vs 64
    // IMAGE_OPTIONAL_HEADER *optionalHeader = &ntHeaders->OptionalHeader; 
    // IMAGE_DATA_DIRECTORY *dataDirectory = &optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    // IMAGE_DATA_DIRECTORY *dataDirectory = 0;
    // if (Is32)
    //    dataDirectory = (IMAGE_DATA_DIRECTORY *)(base + opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    // else
    //    dataDirectory = (IMAGE_DATA_DIRECTORY *)(base + opt64->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
    // IMAGE_EXPORT_DIRECTORY *Exp = (IMAGE_EXPORT_DIRECTORY *)(dosHeader + dataDirectory->VirtualAddress);

    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms680305(v=vs.85).aspx
    // This seems to WORK
    exportDir = MakePtr(PIMAGE_EXPORT_DIRECTORY, base,
                         exportsStartRVA - delta);

    DWORD Rva = exportDir->Name;
    PSTR fn1 = (PSTR)(base + (Rva - delta));
    // PSTR fn2 = (PSTR)ImageRvaToVa(pNTHeader, base, exportDir->Name, 0);
    // PSTR fn3 = (PSTR)GetPtrFromRVA(Rva, pNTHeader, base);
    filename = (PSTR)(base + Rva);  // MAYBE???
    filename = GetSafeFileName(fn1, filename);

    SPRTF("exports table:\n\n");
    SPRTF("  Name:            %s\n", filename);
    SPRTF("  Characteristics: %08X\n", exportDir->Characteristics);
    SPRTF("  TimeDateStamp:   %08X -> %s\n",
    			exportDir->TimeDateStamp,
    			get_ctime_stg((time_t *)&exportDir->TimeDateStamp) );
    SPRTF("  Version:         %u.%02u\n", exportDir->MajorVersion,
            exportDir->MinorVersion);
    SPRTF("  Ordinal base:    %08X\n", exportDir->Base);
    SPRTF("  # of functions:  %08X\n", exportDir->NumberOfFunctions);
    SPRTF("  # of Names:      %08X\n", exportDir->NumberOfNames);
    
    // functions = (PDWORD)((DWORD)exportDir->AddressOfFunctions - delta + base);
    // ordinals = (PWORD)((DWORD)exportDir->AddressOfNameOrdinals - delta + base);
    // name = (PSTR *)((DWORD)exportDir->AddressOfNames - delta + base);
    functions = (PDWORD)(base + (exportDir->AddressOfFunctions - delta));
    ordinals = (PWORD)(base + (exportDir->AddressOfNameOrdinals - delta));
    DWORD *dwpname = (DWORD *)(base + (exportDir->AddressOfNames - delta));

    SPRTF("\n  Entry Pt  Ordn  Name\n");
    for ( i=0; i < exportDir->NumberOfFunctions; i++ )
    {
        DWORD entryPointRVA = functions[i];
        DWORD j;

        if ( entryPointRVA == 0 )   // Skip over gaps in exported function
            continue;               // ordinals (the entrypoint is 0 for
                                    // these functions).

        SPRTF("  %08X  %4u", entryPointRVA, i + exportDir->Base );

        // See if this function has an associated name exported for it.
        for ( j=0; j < exportDir->NumberOfNames; j++ )
            if (ordinals[j] == i) {
                INT off = dwpname[j];
                if (off > delta) {
                    BYTE *pn = base + (off - delta);
                    SPRTF("  %s", pn);
                }
            }
        // Is it a forwarder?  If so, the entry point RVA is inside the
        // .edata section, and is an RVA to the DllName.EntryPointName
        if ( (entryPointRVA >= exportsStartRVA)
             && (entryPointRVA <= exportsEndRVA) )
        {
            SPRTF(" (forwarder -> %s)", entryPointRVA - delta + base );
        }
        
        SPRTF("\n");
    }
}

void DumpRuntimeFunctions( BYTE *base, PIMAGE_NT_HEADERS pNTHeader )
{
	DWORD rtFnRVA;

	rtFnRVA = GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXCEPTION );
	if ( !rtFnRVA )
		return;

	DWORD cEntries =
		GetImgDirEntrySize( pNTHeader, IMAGE_DIRECTORY_ENTRY_EXCEPTION )
		/ sizeof( IMAGE_RUNTIME_FUNCTION_ENTRY );
	if ( 0 == cEntries )
		return;

	PIMAGE_RUNTIME_FUNCTION_ENTRY pRTFn = (PIMAGE_RUNTIME_FUNCTION_ENTRY)GetPtrFromRVA( rtFnRVA, pNTHeader, base );
    if (!pRTFn) {
        SPRTF("TODO: (PIMAGE_RUNTIME_FUNCTION_ENTRY)GetPtrFromRVA failed RVA %08X\n", rtFnRVA);
        return;
    }

    if (!DoesSectionExist("EXCEPTION")) {
        SPRTF("Runtime Function Table (Exception handling) does NOT exist\n", cEntries);
        return;
    }

	SPRTF( "Runtime Function Table (Exception handling) - count %lu\n", cEntries );
#ifdef	ADD_EXCEPTION_HANDLER
    SPRTF( "  Begin     End       Handler   HndlData  PrologEnd\n" );
	SPRTF( "  --------  --------  --------  --------  --------\n" );
#else
    SPRTF("  Begin     End     \n");
    SPRTF("  --------  --------\n");
#endif
	for ( unsigned i = 0; i < cEntries; i++, pRTFn++ )
	{
#ifdef	ADD_EXCEPTION_HANDLER
        SPRTF("  %08X  %08X  %08X  %08X  %08X",
            pRTFn->BeginAddress, pRTFn->EndAddress, pRTFn->ExceptionHandler,
            pRTFn->HandlerData, pRTFn->PrologEndAddress);
#else
        SPRTF("  %08X  %08X",
            pRTFn->BeginAddress, pRTFn->EndAddress);
#endif

		if ( g_pCOFFSymbolTable )
		{
#if 0 // 0000000000000000000000000000000000000000000000000000000000
			PCOFFSymbol pSymbol
				= g_pCOFFSymbolTable->GetNearestSymbolFromRVA(
										pRTFn->BeginAddress
										- pNTHeader->OptionalHeader.ImageBase,
										TRUE );
			if ( pSymbol )
				SPRTF( "  %s", pSymbol->GetName() );

			delete pSymbol;
#endif // 0000000000000000000000000000000000000000000000000000000000
		}

		SPRTF( "\n" );
	}

    if (g_pCOFFSymbolTable)
        SPRTF("TODO: Get symbol name 32/64 bits... using g_pCOFFSymbolTable - %p\n", g_pCOFFSymbolTable);
}

// The names of the available base relocations
/*
    UPDATE 20171020
    //
    // Based relocation types.
    //
    #define IMAGE_REL_BASED_ABSOLUTE              0
    #define IMAGE_REL_BASED_HIGH                  1
    #define IMAGE_REL_BASED_LOW                   2
    #define IMAGE_REL_BASED_HIGHLOW               3
    #define IMAGE_REL_BASED_HIGHADJ               4
    #define IMAGE_REL_BASED_MACHINE_SPECIFIC_5    5
    #define IMAGE_REL_BASED_RESERVED              6
    #define IMAGE_REL_BASED_MACHINE_SPECIFIC_7    7
    #define IMAGE_REL_BASED_MACHINE_SPECIFIC_8    8
    #define IMAGE_REL_BASED_MACHINE_SPECIFIC_9    9
    #define IMAGE_REL_BASED_DIR64                 10

    // Platform-specific based relocation types.
    #define IMAGE_REL_BASED_IA64_IMM64            9
    #define IMAGE_REL_BASED_MIPS_JMPADDR          5
    #define IMAGE_REL_BASED_MIPS_JMPADDR16        9
    #define IMAGE_REL_BASED_ARM_MOV32             5
    #define IMAGE_REL_BASED_THUMB_MOV32           7

*/
char *SzRelocTypes[] = {
"ABSOLUTE","HIGH","LOW","HIGHLOW","HIGHADJ","MIPS/ARM",
"SECTION","REL32","MIPS16","DIR64", "TYPE10" };

#define RELOC_TYPE_COUNT (sizeof(SzRelocTypes) / sizeof(char *))
//
// Dump the base relocation table of a PE file
//
void DumpBaseRelocationsSection(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
	DWORD dwBaseRelocRVA;
    PIMAGE_BASE_RELOCATION baseReloc;

	dwBaseRelocRVA =
		GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_BASERELOC );
    if ( !dwBaseRelocRVA )
        return;

    baseReloc = (PIMAGE_BASE_RELOCATION)GetPtrFromRVA( dwBaseRelocRVA, pNTHeader, base );
    if (!baseReloc) {
        SPRTF("TODO: (PIMAGE_BASE_RELOCATION)GetPtrFromRVA failed RVA %08X\n", dwBaseRelocRVA);
        return;
    }

    SPRTF("base relocations:\n\n");

    while ( baseReloc->SizeOfBlock != 0 )
    {
        unsigned i,cEntries;
        PWORD pEntry;
        char *szRelocType;
        WORD relocType;

		// Sanity check to make sure the data looks OK.
		if ( 0 == baseReloc->VirtualAddress )
			break;
		if ( baseReloc->SizeOfBlock < sizeof(*baseReloc) )
			break;
		
        cEntries = (baseReloc->SizeOfBlock-sizeof(*baseReloc))/sizeof(WORD);
        pEntry = MakePtr( PWORD, baseReloc, sizeof(*baseReloc) );
        
        SPRTF("Virtual Address: %08X  size: %08X\n",
                baseReloc->VirtualAddress, baseReloc->SizeOfBlock);
            
        for ( i=0; i < cEntries; i++ )
        {
            // Extract the top 4 bits of the relocation entry.  Turn those 4
            // bits into an appropriate descriptive string (szRelocType)
            relocType = (*pEntry & 0xF000) >> 12;
            szRelocType = (relocType < RELOC_TYPE_COUNT) ? SzRelocTypes[relocType] : "unknown";
            
            SPRTF("  %08X %s",
                    (*pEntry & 0x0FFF) + baseReloc->VirtualAddress,
                    szRelocType);

			if ( IMAGE_REL_BASED_HIGHADJ == relocType )
			{
				pEntry++;
				cEntries--;
				SPRTF( " (%X)", *pEntry );
			}

			SPRTF( "\n" );
            pEntry++;   // Advance to next relocation entry
        }
        
        baseReloc = MakePtr( PIMAGE_BASE_RELOCATION, baseReloc,
                             baseReloc->SizeOfBlock);
    }
}

//
// Dump out the new IMAGE_BOUND_IMPORT_DESCRIPTOR that NT 3.51 added
//
void DumpBoundImportDescriptors( BYTE *base, PIMAGE_NT_HEADERS pNTHeader )
{
    DWORD bidRVA;   // Bound import descriptors RVA
    PIMAGE_BOUND_IMPORT_DESCRIPTOR pibid;

    bidRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT);
    if ( !bidRVA )
        return;
    
    pibid = MakePtr( PIMAGE_BOUND_IMPORT_DESCRIPTOR, base, bidRVA );
    if (!IsAddressInRange((BYTE *)pibid, (int)sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR))) {
        SPRTF("TODO: DumpBoundImportDescriptors PIMAGE_BOUND_IMPORT_DESCRIPTOR out of range - %p\n", pibid);
        return;
    }
    SPRTF( "Bound import descriptors:\n\n" );
    SPRTF( "  Module        TimeDate\n" );
    SPRTF( "  ------------  --------\n" );
    
    while ( pibid->TimeDateStamp )
    {
        unsigned i;
        PIMAGE_BOUND_FORWARDER_REF pibfr;
        WORD off = pibid->OffsetModuleName;
        BYTE *pMod = base + bidRVA + off;
        if (!IsAddressInRange(pMod, 32)) {
            SPRTF("TODO: DumpBoundImportDescriptors pMod out of range - %p\n", pMod);
            return;
        }
        SPRTF( "  %-12s  %08X -> %s\n",
        		pMod,   // base + bidRVA + pibid->OffsetModuleName,
                pibid->TimeDateStamp,
                get_ctime_stg((time_t *)&pibid->TimeDateStamp) );
                            
        pibfr = MakePtr(PIMAGE_BOUND_FORWARDER_REF, pibid,
                            sizeof(IMAGE_BOUND_IMPORT_DESCRIPTOR));

        for ( i=0; i < pibid->NumberOfModuleForwarderRefs; i++ )
        {
            SPRTF("    forwarder:  %-12s  %08X -> %s\n", 
                            base + bidRVA + pibfr->OffsetModuleName,
                            pibfr->TimeDateStamp,
                            get_ctime_stg((time_t *)&pibfr->TimeDateStamp) );
            pibfr++;    // advance to next forwarder ref
                
            // Keep the outer loop pointer up to date too!
            pibid = MakePtr( PIMAGE_BOUND_IMPORT_DESCRIPTOR, pibid,
                             sizeof( IMAGE_BOUND_FORWARDER_REF ) );
        }

        pibid++;    // Advance to next pibid;
    }
}

//
// top level routine called from PEDUMP.C to dump the components of a PE file
//
void DumpExeFile( PIMAGE_DOS_HEADER dosHeader )
{
    PIMAGE_NT_HEADERS pNTHeader;
    BYTE *base = (BYTE *)dosHeader;
    
    pNTHeader = MakePtr( PIMAGE_NT_HEADERS, dosHeader,
                                dosHeader->e_lfanew );

    // First, verify that the e_lfanew field gave us a reasonable
    // pointer, then verify the PE signature.
    //__try
    //{
        if ( pNTHeader->Signature != IMAGE_NT_SIGNATURE )
        {
            SPRTF("Not a Portable Executable (PE) EXE\n");
            return;
        }
    //}
    //__except( TRUE )    // Should only get here if pNTHeader (above) is bogus
    //{
    //    SPRTF( "invalid .EXE\n");
    //    return;
    //}
    PIMAGE_NT_HEADERS32 header32 = (PIMAGE_NT_HEADERS32)pNTHeader;
    PIMAGE_NT_HEADERS64 header64 = (PIMAGE_NT_HEADERS64)pNTHeader;
    PIMAGE_OPTIONAL_HEADER64 opt64 = 0;
    PIMAGE_OPTIONAL_HEADER32 opt32 = 0;
    
    DumpHeader((PIMAGE_FILE_HEADER)&pNTHeader->FileHeader);
    SPRTF("\n");

    if (header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) { // PE32
                                                                           // offset = header32->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
                                                                           // exports = (PIMAGE_EXPORT_DIRECTORY)(base + offset);
        Is32 = 1;
        opt32 = &header32->OptionalHeader;
    }
    else if (header32->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) { // PE32+
                                                                           // offset = header64->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
                                                                           // exports = (PIMAGE_EXPORT_DIRECTORY)(base + offset);
        opt64 = &header64->OptionalHeader;
        Is32 = 0;
    }
    else {
        SPRTF("Is NOT a PE32 nor PE32+ magic value! Got %x\n", header32->OptionalHeader.Magic);
        return;
    }
    //DumpOptionalHeader((PIMAGE_OPTIONAL_HEADER)&pNTHeader->OptionalHeader);
    if (Is32) {
        DumpOptionalHeader32(opt32);
    }
    else {
        DumpOptionalHeader64(opt64);
    }
    SPRTF("\n");

    DumpSectionTable( IMAGE_FIRST_SECTION(pNTHeader), pNTHeader->FileHeader.NumberOfSections, TRUE, "exedump");
    SPRTF("\n");

    DumpExeDebugDirectory(base, pNTHeader);
    if ( pNTHeader->FileHeader.PointerToSymbolTable == 0 )
        g_pCOFFHeader = 0; // Doesn't really exist!
    SPRTF("\n");

    DumpResourceSection(base, pNTHeader);
    SPRTF("\n");

    DumpImportsSection(base, pNTHeader);
    SPRTF("\n");
    
    if ( GetImgDirEntryRVA( pNTHeader, IMAGE_DIRECTORY_ENTRY_BOUND_IMPORT) )
    {
        DumpBoundImportDescriptors( base, pNTHeader );
        SPRTF( "\n" );
    }
    
    if (DoesSectionExist("EXPORT") && GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_EXPORT)) {
        DumpExportsSection(base, pNTHeader);
    }
    else {
        SPRTF("No EXPORT section to dump\n");
    }

    SPRTF("\n");
    //=========================================================================
	//
	// If we have COFF symbols, create a symbol table now
	//
	//=========================================================================

	if ( g_pCOFFHeader )	// Did we see a COFF symbols header while looking
	{						// through the debug directory?
		g_pCOFFSymbolTable = new COFFSymbolTable(
				(PVOID)(base+ pNTHeader->FileHeader.PointerToSymbolTable),
				pNTHeader->FileHeader.NumberOfSymbols );
	}

	if ( fShowPDATA )
	{
		DumpRuntimeFunctions( base, pNTHeader );
		SPRTF( "\n" );
	}

    if ( fShowRelocations )
    {
        DumpBaseRelocationsSection(base, pNTHeader);
        SPRTF("\n");
    } 

	if ( fShowSymbolTable && g_pMiscDebugInfo )
	{
		DumpMiscDebugInfo( g_pMiscDebugInfo );
		SPRTF( "\n" );
	}

	if ( fShowSymbolTable && g_pCVHeader )
	{
		DumpCVDebugInfo( g_pCVHeader );
		SPRTF( "\n" );
	}

    if ( fShowSymbolTable && g_pCOFFHeader )
    {
        DumpCOFFHeader( g_pCOFFHeader );
        SPRTF("\n");
    }
    
    if ( fShowLineNumbers && g_pCOFFHeader )
    {
        DumpLineNumbers( MakePtr(PIMAGE_LINENUMBER, g_pCOFFHeader,
                            g_pCOFFHeader->LvaToFirstLinenumber),
                            g_pCOFFHeader->NumberOfLinenumbers);
        SPRTF("\n");
    }

    if ( fShowSymbolTable )
    {
        if ( pNTHeader->FileHeader.NumberOfSymbols 
            && pNTHeader->FileHeader.PointerToSymbolTable
			&& g_pCOFFSymbolTable )
        {
            DumpSymbolTable( g_pCOFFSymbolTable );
            SPRTF("\n");
        }
    }
    
    if ( fShowRawSectionData )
    {
        //DumpRawSectionData( (PIMAGE_SECTION_HEADER)(pNTHeader+1),
        //                    dosHeader,
        //                    pNTHeader->FileHeader.NumberOfSections);
        DumpRawSectionData(IMAGE_FIRST_SECTION(pNTHeader),
            dosHeader,
            pNTHeader->FileHeader.NumberOfSections);
    }

	if ( g_pCOFFSymbolTable )
		delete g_pCOFFSymbolTable;
    g_pCOFFSymbolTable = 0;
}
