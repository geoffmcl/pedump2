//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: EXEDUMP.C
//==================================

#include <windows.h>
#include <stdio.h>
#include <time.h>
#pragma hdrstop
#include "common.h"
#include "exedump.h"
#include "coffsymboltable.h"
#include "extrnvar.h"

void DumpROMOptionalHeader( PIMAGE_ROM_OPTIONAL_HEADER pROMOptHdr )
{
    UINT width = 30;

    SPRTF("Optional Header\n");
    
    SPRTF("  %-*s%04X\n", width, "Magic", pROMOptHdr->Magic);
    SPRTF("  %-*s%u.%02u\n", width, "linker version",
        pROMOptHdr->MajorLinkerVersion,
        pROMOptHdr->MinorLinkerVersion);
    SPRTF("  %-*s%X\n", width, "size of code", pROMOptHdr->SizeOfCode);
    SPRTF("  %-*s%X\n", width, "size of initialized data",
        pROMOptHdr->SizeOfInitializedData);
    SPRTF("  %-*s%X\n", width, "size of uninitialized data",
        pROMOptHdr->SizeOfUninitializedData);
    SPRTF("  %-*s%X\n", width, "entrypoint RVA",
        pROMOptHdr->AddressOfEntryPoint);
    SPRTF("  %-*s%X\n", width, "base of code", pROMOptHdr->BaseOfCode);
    SPRTF("  %-*s%X\n", width, "base of Bss", pROMOptHdr->BaseOfBss);
    SPRTF("  %-*s%X\n", width, "GprMask", pROMOptHdr->GprMask);

	SPRTF("  %-*s%X\n", width, "CprMask[0]", pROMOptHdr->CprMask[0] );
	SPRTF("  %-*s%X\n", width, "CprMask[1]", pROMOptHdr->CprMask[1] );
	SPRTF("  %-*s%X\n", width, "CprMask[2]", pROMOptHdr->CprMask[2] );
	SPRTF("  %-*s%X\n", width, "CprMask[3]", pROMOptHdr->CprMask[3] );

    SPRTF("  %-*s%X\n", width, "GpValue", pROMOptHdr->GpValue);
}

// VARIATION on the IMAGE_FIRST_SECTION macro from WINNT.H

#define IMAGE_FIRST_ROM_SECTION( ntheader ) ((PIMAGE_SECTION_HEADER)        \
    ((DWORD)ntheader +                                                  \
     FIELD_OFFSET( IMAGE_ROM_HEADERS, OptionalHeader ) +                 \
     ((PIMAGE_ROM_HEADERS)(ntheader))->FileHeader.SizeOfOptionalHeader   \
    ))

void DumpROMImage( PIMAGE_ROM_HEADERS pROMHeader )
{
    DumpHeader(&pROMHeader->FileHeader);
    SPRTF("\n");

    DumpROMOptionalHeader(&pROMHeader->OptionalHeader);
    SPRTF("\n");

    DumpSectionTable( IMAGE_FIRST_ROM_SECTION(pROMHeader), pROMHeader->FileHeader.NumberOfSections, TRUE, "romimage");
    SPRTF("\n");

	// Dump COFF symbols out here.  Get offsets from the header
}

