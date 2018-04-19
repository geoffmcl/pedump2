//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: LIBDUMP.C
//==================================

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "common.h"
#include "objdump.h"
#include "libdump.h"
#include "extrnvar.h"

PSTR PszLongnames = 0;

DWORD ConvertBigEndian(DWORD bigEndian);

void DisplayArchiveMemberHeader( PIMAGE_ARCHIVE_MEMBER_HEADER pArchHeader, DWORD fileOffset )
{
    SPRTF("Archive Member Header (offset %08X):\n", fileOffset);

    SPRTF("  Name:     %.16s", pArchHeader->Name);
    if ( pArchHeader->Name[0] == '/' && isdigit(pArchHeader->Name[1]) )
        SPRTF( "  (%s)\n", PszLongnames + atoi((char *)pArchHeader->Name+1) );
    SPRTF("\n");

	char szDateAsLong[64];
	sprintf( szDateAsLong, "%.12s", pArchHeader->Date );
	time_t dateAsLong = atol(szDateAsLong);
	
    SPRTF("  Date:     %.12s %s\n", pArchHeader->Date, get_ctime_stg(&dateAsLong) );
    SPRTF("  UserID:   %.6s\n", pArchHeader->UserID);
    SPRTF("  GroupID:  %.6s\n", pArchHeader->GroupID);
    SPRTF("  Mode:     %.8s\n", pArchHeader->Mode);
    SPRTF("  Size:     %.10s\n", pArchHeader->Size);
}

void DumpFirstLinkerMember(PVOID p)
{
    DWORD cSymbols = *(PDWORD)p;
    PDWORD pMemberOffsets = MakePtr( PDWORD, p, 4 );
    PSTR pSymbolName;
    unsigned i;

    cSymbols = ConvertBigEndian(cSymbols);
    pSymbolName = MakePtr( PSTR, pMemberOffsets, 4 * cSymbols );
    
    SPRTF("First Linker Member:\n");
    SPRTF( "  Symbols:         %08X\n", cSymbols );
    SPRTF( "  MbrOffs   Name\n  --------  ----\n" );
        
    for ( i = 0; i < cSymbols; i++ )
    {
        DWORD offset;
        
        offset = ConvertBigEndian( *pMemberOffsets );
        
        SPRTF("  %08X  %s\n", offset, pSymbolName);
        
        pMemberOffsets++;
        pSymbolName += strlen(pSymbolName) + 1;
    }
}

void DumpSecondLinkerMember(PVOID p)
{
    DWORD cArchiveMembers = *(PDWORD)p;
    PDWORD pMemberOffsets = MakePtr( PDWORD, p, 4 );
    DWORD cSymbols;
    PSTR pSymbolName;
    PWORD pIndices;
    unsigned i;

    cArchiveMembers = cArchiveMembers;

    // The number of symbols is in the DWORD right past the end of the
    // member offset array.
    cSymbols = pMemberOffsets[cArchiveMembers];

    pIndices = MakePtr( PWORD, p, 4 + cArchiveMembers * sizeof(DWORD) + 4 );

    pSymbolName = MakePtr( PSTR, pIndices, cSymbols * sizeof(WORD) );
    
    SPRTF("Second Linker Member:\n");
    
    SPRTF( "  Archive Members: %08X\n", cArchiveMembers );
    SPRTF( "  Symbols:         %08X\n", cSymbols );
    SPRTF( "  MbrOffs   Name\n  --------  ----\n" );

    for ( i = 0; i < cSymbols; i++ )
    {
        SPRTF("  %08X  %s\n", pMemberOffsets[pIndices[i] - 1], pSymbolName);
        pSymbolName += strlen(pSymbolName) + 1;
    }
}

void DumpLongnamesMember(PVOID p, DWORD len)
{
    PSTR pszName = (PSTR)p;
    DWORD offset = 0;

    PszLongnames = (PSTR)p;     // Save off pointer for use when dumping
                                // out OBJ member names

    SPRTF("Longnames:\n");
    
    // The longnames member is a series of null-terminated string.  Print
    // out the offset of each string (in decimal), followed by the string.
    while ( offset < len )
    {
        unsigned cbString = lstrlen( pszName )+1;

        SPRTF("  %05u: %s\n", offset, pszName);
        offset += cbString;
        pszName += cbString;
    }
}

void DumpLibFile( LPVOID lpFileBase )
{
    PIMAGE_ARCHIVE_MEMBER_HEADER pArchHeader;
    BOOL fSawFirstLinkerMember = FALSE;
    BOOL fSawSecondLinkerMember = FALSE;
    BOOL fBreak = FALSE;

    if ( strncmp((char *)lpFileBase,IMAGE_ARCHIVE_START,
                            		IMAGE_ARCHIVE_START_SIZE ) )
    {
        SPRTF("Not a valid .LIB file - signature not found\n");
        return;
    }
    
    pArchHeader = MakePtr(PIMAGE_ARCHIVE_MEMBER_HEADER, lpFileBase,
                            IMAGE_ARCHIVE_START_SIZE);

    while ( pArchHeader )
    {
        DWORD thisMemberSize;
        DWORD fileOffset = (DWORD)((PBYTE)pArchHeader - (PBYTE)lpFileBase);
        if (!fShowMachineType) {
            DisplayArchiveMemberHeader(pArchHeader, fileOffset);
            SPRTF("\n");
        }

        if ( !strncmp( 	(char *)pArchHeader->Name,
        				IMAGE_ARCHIVE_LINKER_MEMBER, 16) )
        {
            if ( !fSawFirstLinkerMember )
            {
                if (!fShowMachineType) {
                    DumpFirstLinkerMember((PVOID)(pArchHeader + 1));
                    SPRTF("\n");
                }
                fSawFirstLinkerMember = TRUE;
            }
            else if ( !fSawSecondLinkerMember )
            {
                if (!fShowMachineType) {
                    DumpSecondLinkerMember((PVOID)(pArchHeader + 1));
                    SPRTF("\n");
                }
                fSawSecondLinkerMember = TRUE;
            }
        }
        else if( !strncmp(	(char *)pArchHeader->Name,
        					IMAGE_ARCHIVE_LONGNAMES_MEMBER, 16) )
        {
            if (!fShowMachineType) {
                DumpLongnamesMember((PVOID)(pArchHeader + 1),
                    atoi((char *)pArchHeader->Size));
                SPRTF("\n");
            }
        }
        else    // It's an OBJ file
        {
            // from : https://msdn.microsoft.com/en-us/library/windows/desktop/ms680313(v=vs.85).aspx
            // NumberOfSections - Note that the Windows loader limits the number of sections to 96.
            // 20171021 avoid dumping INVALID headers, especially section count out-of-range

            PIMAGE_FILE_HEADER pifh = (PIMAGE_FILE_HEADER)(pArchHeader + 1);
            WORD nSects = pifh->NumberOfSections;
            WORD nSzOpt = pifh->SizeOfOptionalHeader;
            PIMAGE_SECTION_HEADER pSections = MakePtr(PIMAGE_SECTION_HEADER, (pifh + 1), nSzOpt);
            PIMAGE_SECTION_HEADER last = pSections;
            if (nSects) {
                last = pSections + (nSects - 1);
            }
            if (IsAddressInRange((BYTE *)last, (int)sizeof(IMAGE_SECTION_HEADER))) {
                DumpObjFile(pifh);
                if (fShowMachineType)
                    return;
            }
            else {
                SPRTF("TODO: section count %u! last PIMAGE_SECTION_HEADER out of range - %p\n", nSects, last);
                SPRTF("\n");
            }
        }

        // Calculate how big this member is (it's originally stored as 
        // as ASCII string.
        thisMemberSize = atoi((char *)pArchHeader->Size)
                        + IMAGE_SIZEOF_ARCHIVE_MEMBER_HDR;

        thisMemberSize = (thisMemberSize+1) & ~1;   // Round up

        // Get a pointer to the next archive member
        pArchHeader = MakePtr(PIMAGE_ARCHIVE_MEMBER_HEADER, pArchHeader,
                                thisMemberSize);

        // Bail out if we don't see the EndHeader signature in the next record
        __try
        {
            if (strncmp( (char *)pArchHeader->EndHeader, IMAGE_ARCHIVE_END, 2))
                break;
        }
        __except( TRUE )    // Should only get here if pArchHeader is bogus
        {
            fBreak = TRUE;  // Ideally, we could just put a "break;" here,
        }                   // but BC++ doesn't like it.
        
        if ( fBreak )   // work around BC++ problem.
            break;
    }
}

// Routine to convert from big endian to little endian
DWORD ConvertBigEndian(DWORD bigEndian)
{
	DWORD temp = 0;

	// SPRTF( "bigEndian: %08X\n", bigEndian );

	temp |= bigEndian >> 24;
	temp |= ((bigEndian & 0x00FF0000) >> 8);
	temp |= ((bigEndian & 0x0000FF00) << 8);
	temp |= ((bigEndian & 0x000000FF) << 24);

	return temp;
}

