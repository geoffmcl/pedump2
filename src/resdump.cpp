//==================================
// PEDUMP - Matt Pietrek 1997
// FILE: RESDUMP.C
//==================================

#include <windows.h>
#include <stdio.h>
#include <time.h>
#pragma hdrstop
#include "common.h"
#include "extrnvar.h"
#include "resdump.h"

// Function prototype (necessary because two functions recurse)
void DumpResourceDirectory( PIMAGE_RESOURCE_DIRECTORY resDir, BYTE *resourceBase, DWORD level, DWORD resourceType );

// The predefined resource types
char *SzResourceTypes[] = {
"???_0",
"CURSOR",
"BITMAP",
"ICON",
"MENU",
"DIALOG",
"STRING",
"FONTDIR",
"FONT",
"ACCELERATORS",
"RCDATA",
"MESSAGETABLE",
"GROUP_CURSOR",
"???_13",
"GROUP_ICON",
"???_15",
"VERSION",
"DLGINCLUDE",
"???_18",
"PLUGPLAY",
"VXD",
"ANICURSOR",
"ANIICON"
};

PIMAGE_RESOURCE_DIRECTORY_ENTRY pStrResEntries = 0;
PIMAGE_RESOURCE_DIRECTORY_ENTRY pDlgResEntries = 0;
DWORD cStrResEntries = 0;
DWORD cDlgResEntries = 0;

DWORD GetOffsetToDataFromResEntry( 	BYTE *base,
									BYTE *resourceBase,
									PIMAGE_RESOURCE_DIRECTORY_ENTRY pResEntry )
{
	// The IMAGE_RESOURCE_DIRECTORY_ENTRY is gonna point to a single
	// IMAGE_RESOURCE_DIRECTORY, which in turn will point to the
	// IMAGE_RESOURCE_DIRECTORY_ENTRY, which in turn will point
	// to the IMAGE_RESOURCE_DATA_ENTRY that we're really after.  In
	// other words, traverse down a level.

	PIMAGE_RESOURCE_DIRECTORY pStupidResDir;
	pStupidResDir = (PIMAGE_RESOURCE_DIRECTORY)
                    (resourceBase + pResEntry->OffsetToDirectory);

    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry =
	    	(PIMAGE_RESOURCE_DIRECTORY_ENTRY)(pStupidResDir + 1);// PTR MATH

	PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry =
			(PIMAGE_RESOURCE_DATA_ENTRY)(resourceBase +
										 pResDirEntry->OffsetToData);

	return pResDataEntry->OffsetToData;
}

void DumpStringTable( 	BYTE *base,
						PIMAGE_NT_HEADERS pNTHeader,
						BYTE *resourceBase,
						PIMAGE_RESOURCE_DIRECTORY_ENTRY pStrResEntry,
						DWORD cStrResEntries )
{
	for ( unsigned i = 0; i < cStrResEntries; i++, pStrResEntry++ )
	{
		DWORD offsetToData
			= GetOffsetToDataFromResEntry( base, resourceBase, pStrResEntry );
			
 		PWORD pStrEntry = (PWORD)GetPtrFromRVA(	offsetToData, pNTHeader, base );
        if (!pStrEntry) {
            SPRTF("TODO: (PWORD)GetPtrFromRVA failed RVA %08X\n", offsetToData);
            break;
        }
		
		unsigned id = (pStrResEntry->Name - 1) << 4;

		for ( unsigned j = 0; j < 16; j++ )
		{
			WORD len = *pStrEntry++;
			if ( len )
			{
				SPRTF( "%-5u: ", id + j );

				for ( unsigned k = 0; k < min(len, (WORD)64); k++ )
				{
					char * s;
					char szBuff[20];
					char c = (char)pStrEntry[k];
					switch( c )
					{
						case '\t': s = "\\t"; break;
						case '\r': s = "\\r"; break;
						case '\n': s = "\\n"; break;
						default:
							wsprintf( szBuff, "%c", isprint(c) ? c : '.' );
							s=szBuff;
							break;
					}

					SPRTF( "%s", s );
				}

				SPRTF( "\n" );
			}

			pStrEntry += len;
		}
	}
}

#define ADD_DUMP_DIALOGS

void DumpDialogs( 	BYTE *base,
					PIMAGE_NT_HEADERS pNTHeader,
					BYTE *resourceBase,
					PIMAGE_RESOURCE_DIRECTORY_ENTRY pDlgResEntry,
					DWORD cDlgResEntries )
{
#ifdef ADD_DUMP_DIALOGS
	for ( unsigned i = 0; i < cDlgResEntries; i++, pDlgResEntry++ )
	{
		DWORD offsetToData
			= GetOffsetToDataFromResEntry( base, resourceBase, pDlgResEntry );
			
        PDWORD pDlgStyle = (PDWORD)GetPtrFromRVA(offsetToData, pNTHeader, base);
        if (!pDlgStyle) {
            SPRTF("TODO: (PDWORD)GetPtrFromRVA failed RVA %08X\n", offsetToData);
            break;
        }
													
		SPRTF( "  ====================\n" );
		if ( HIWORD(*pDlgStyle) != 0xFFFF )
		{
			//	A regular DLGTEMPLATE
			DLGTEMPLATE * pDlgTemplate = ( DLGTEMPLATE * )pDlgStyle;

			SPRTF( "  style: %08X\n", pDlgTemplate->style );			
			SPRTF( "  extended style: %08X\n", pDlgTemplate->dwExtendedStyle );			

			SPRTF( "  controls: %u\n", pDlgTemplate->cdit );
			SPRTF( "  (%u,%u) - (%u,%u)\n",
						pDlgTemplate->x, pDlgTemplate->y,
						pDlgTemplate->x + pDlgTemplate->cx,
						pDlgTemplate->y + pDlgTemplate->cy );
			PWORD pMenu = (PWORD)(pDlgTemplate + 1);	// ptr math!

			//
			// First comes the menu
			//
			if ( *pMenu )
			{
				if ( 0xFFFF == *pMenu )
				{
					pMenu++;
					SPRTF( "  ordinal menu: %u\n", *pMenu );
				}
				else
				{
					SPRTF( "  menu: " );
					while ( *pMenu )
						SPRTF( "%c", LOBYTE(*pMenu++) );				

					pMenu++;
					SPRTF( "\n" );
				}
			}
			else
				pMenu++;	// Advance past the menu name

			//
			// Next comes the class
			//			
			PWORD pClass = pMenu;
						
			if ( *pClass )
			{
				if ( 0xFFFF == *pClass )
				{
					pClass++;
					SPRTF( "  ordinal class: %u\n", *pClass );
				}
				else
				{
					SPRTF( "  class: " );
					while ( *pClass )
					{
						SPRTF( "%c", LOBYTE(*pClass++) );				
					}		
					pClass++;
					SPRTF( "\n" );
				}
			}
			else
				pClass++;	// Advance past the class name
			
			//
			// Finally comes the title
			//

			PWORD pTitle = pClass;
			if ( *pTitle )
			{
				SPRTF( "  title: " );

				while ( *pTitle )
					SPRTF( "%c", LOBYTE(*pTitle++) );
					
				pTitle++;
			}
			else
				pTitle++;	// Advance past the Title name

			SPRTF( "\n" );

			PWORD pFont = pTitle;
						
			if ( pDlgTemplate->style & DS_SETFONT )
			{
				SPRTF( "  Font: %u point ",  *pFont++ );
				while ( *pFont )
					SPRTF( "%c", LOBYTE(*pFont++) );

				pFont++;
				SPRTF( "\n" );
			}
	        else
    	        pFont = pTitle; 

			// DLGITEMPLATE starts on a 4 byte boundary
			LPDLGITEMTEMPLATE pDlgItemTemplate = (LPDLGITEMTEMPLATE)pFont;
			
			for ( unsigned i=0; i < pDlgTemplate->cdit; i++ )
			{
				// Control item header....
				//pDlgItemTemplate = (DLGITEMTEMPLATE *)
				//					(((BYTE *)pDlgItemTemplate+3) & ~3);
				
				SPRTF( "    style: %08X\n", pDlgItemTemplate->style );			
				SPRTF( "    extended style: %08X\n",
						pDlgItemTemplate->dwExtendedStyle );			

				SPRTF( "    (%u,%u) - (%u,%u)\n",
							pDlgItemTemplate->x, pDlgItemTemplate->y,
							pDlgItemTemplate->x + pDlgItemTemplate->cx,
							pDlgItemTemplate->y + pDlgItemTemplate->cy );
				SPRTF( "    id: %u\n", pDlgItemTemplate->id );
				
				//
				// Next comes the control's class name or ID
				//			
				PWORD pClass = (PWORD)(pDlgItemTemplate + 1);
				if ( *pClass )
				{							
					if ( 0xFFFF == *pClass )
					{
						pClass++;
						SPRTF( "    ordinal class: %u", *pClass++ );
					}
					else
					{
						SPRTF( "    class: " );
						while ( *pClass )
							SPRTF( "%c", LOBYTE(*pClass++) );

						pClass++;
						SPRTF( "\n" );
					}
				}
				else
					pClass++;
					
				SPRTF( "\n" );			

				//
				// next comes the title
				//

				PWORD pTitle = pClass;
				
				if ( *pTitle )
				{
					SPRTF( "    title: " );
					if ( 0xFFFF == *pTitle )
					{
						pTitle++;
						SPRTF( "%u\n", *pTitle++ );
					}
					else
					{
						while ( *pTitle )
							SPRTF( "%c", LOBYTE(*pTitle++) );
						pTitle++;
						SPRTF( "\n" );
					}
				}
				else	
					pTitle++;	// Advance past the Title name

				SPRTF( "\n" );
				
				// PBYTE pCreationData = (PBYTE)(((BYTE *)pTitle + 1) & 0xFFFFFFFE);
                PBYTE pCreationData = (PBYTE)(pTitle + 1);

				if ( *pCreationData )
					pCreationData += *pCreationData;
				else
					pCreationData++;

				pDlgItemTemplate = (DLGITEMTEMPLATE *)pCreationData;	
				
				SPRTF( "\n" );
			}
			
			SPRTF( "\n" );
		}
		else
		{
			// A DLGTEMPLATEEX		
		}
		
		SPRTF( "\n" );
	}
#else
    SPRTF("TODO: Port DumpDialogs!\n");
#endif // #ifdef ADD_DUMP_DIALOGS

}

// Get an ASCII string representing a resource type
void GetResourceTypeName(DWORD type, PSTR buffer, UINT cBytes)
{
    // if (MAKEINTRESOURCE(type) <= RT_ANIICON)
    if (type <= (WORD)RT_ANIICON )
        strncpy(buffer, SzResourceTypes[type], cBytes);
    else
        sprintf(buffer, "%X", type);
}

//
// If a resource entry has a string name (rather than an ID), go find
// the string and convert it from unicode to ascii.
//
void GetResourceNameFromId
(
    DWORD id, BYTE *resourceBase, PSTR buffer, UINT cBytes
)
{
    PIMAGE_RESOURCE_DIR_STRING_U prdsu;

    // If it's a regular ID, just format it.
    if ( !(id & IMAGE_RESOURCE_NAME_IS_STRING) )
    {
        sprintf(buffer, "%X", id);
        return;
    }
    
    id &= 0x7FFFFFFF;
    prdsu = (PIMAGE_RESOURCE_DIR_STRING_U)(resourceBase + id);

    if (IsAddressInRange((BYTE *)prdsu, (int)sizeof(IMAGE_RESOURCE_DIR_STRING_U))) {
        // prdsu->Length is the number of unicode characters
        WideCharToMultiByte(CP_ACP, 0, prdsu->NameString, prdsu->Length,
            buffer, cBytes, 0, 0);
        buffer[min(cBytes - 1, prdsu->Length)] = 0;  // Null terminate it!!!
    }
    else {
        sprintf(buffer, "PIMAGE_RESOURCE_DIR_STRING_U out of range - %p", prdsu);
        SPRTF("TODO: GetResourceNameFromId - PIMAGE_RESOURCE_DIR_STRING_U out of range - %p\n", prdsu);
    }
}

//
// Dump the information about one resource directory entry.  If the
// entry is for a subdirectory, call the directory dumping routine
// instead of printing information in this routine.
//
void DumpResourceEntry( PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry,
    BYTE * resourceBase,
    DWORD level
)
{
    UINT i;
    char nameBuffer[264];
    PIMAGE_RESOURCE_DATA_ENTRY pResDataEntry;

    if (IsAddressInRange((BYTE *)resDirEntry, (int)sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {

        if (resDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
        {
            DumpResourceDirectory((PIMAGE_RESOURCE_DIRECTORY)
                ((resDirEntry->OffsetToData & 0x7FFFFFFF) + resourceBase),
                resourceBase, level, resDirEntry->Name);
            return;
        }

        // Spit out the spacing for the level indentation
        for (i = 0; i < level; i++)
            SPRTF("    ");

        if (resDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING)
        {
            GetResourceNameFromId(resDirEntry->Name, resourceBase, nameBuffer,
                sizeof(nameBuffer));
            SPRTF("Name: %s  DataEntryOffs: %08X\n",
                nameBuffer, resDirEntry->OffsetToData);
        }
        else
        {
            SPRTF("ID: %08X  DataEntryOffs: %08X\n",
                resDirEntry->Name, resDirEntry->OffsetToData);
        }

        // the resDirEntry->OffsetToData is a pointer to an
        // IMAGE_RESOURCE_DATA_ENTRY.  Go dump out that information.  First,
        // spit out the proper indentation
        for (i = 0; i < level; i++)
            SPRTF("    ");

        pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)
            (resourceBase + resDirEntry->OffsetToData);
        if (IsAddressInRange((BYTE *)pResDataEntry, (int)sizeof(IMAGE_RESOURCE_DATA_ENTRY))) {

            SPRTF("DataRVA: %05X  DataSize: %05X  CodePage: %X\n",
                pResDataEntry->OffsetToData, pResDataEntry->Size,
                pResDataEntry->CodePage);
        }
        else {
            SPRTF("TODO: DumpResourceEntry - PIMAGE_RESOURCE_DATA_ENTRY out of range - %p\n", pResDataEntry);
        }
    }
    else {
        SPRTF("TODO: DumpResourceEntry - PIMAGE_RESOURCE_DIRECTORY_ENTRY out of range - %p\n", resDirEntry);
    }
}

//
// Dump the information about one resource directory.
//
void DumpResourceDirectory( PIMAGE_RESOURCE_DIRECTORY resDir,
    BYTE *resourceBase,
    DWORD level,
    DWORD resourceType
)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry;
    char szType[64];
    UINT i;

    // Level 1 resources are the resource types
    if ( level == 1 )
    {
		SPRTF( "    ---------------------------------------------------"
	            "-----------\n" );

		if ( resourceType & IMAGE_RESOURCE_NAME_IS_STRING )
		{
			GetResourceNameFromId( resourceType, resourceBase,
									szType, sizeof(szType) );
		}
		else
		{
	        GetResourceTypeName( resourceType, szType, sizeof(szType) );
		}
	}
    else    // All other levels, just print out the regular id or name
    {
        GetResourceNameFromId( resourceType, resourceBase, szType,
                               sizeof(szType) );
    }
	    
    // Spit out the spacing for the level indentation
    for ( i=0; i < level; i++ )
        SPRTF("    ");

    if (IsAddressInRange((BYTE *)resDir, (int)sizeof(IMAGE_RESOURCE_DIRECTORY))) {
        SPRTF(
            "ResDir (%s) Entries:%02X (Named:%02X, ID:%02X) TimeDate:%08X",
            szType, resDir->NumberOfNamedEntries + resDir->NumberOfIdEntries,
            resDir->NumberOfNamedEntries, resDir->NumberOfIdEntries,
            resDir->TimeDateStamp);

        if (resDir->MajorVersion || resDir->MinorVersion)
            SPRTF(" Vers:%u.%02u", resDir->MajorVersion, resDir->MinorVersion);
        if (resDir->Characteristics)
            SPRTF(" Char:%08X", resDir->Characteristics);
        SPRTF("\n");
        //
        // The "directory entries" immediately follow the directory in memory
        //
        resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir + 1);

        // If it's a stringtable, save off info for future use
        // if (level == 1 && (MAKEINTRESOURCE(resourceType) == RT_STRING))
        if (level == 1 && (resourceType == (WORD)RT_STRING))
        {
            pStrResEntries = resDirEntry;
            cStrResEntries = resDir->NumberOfIdEntries;
        }

        // If it's a stringtable, save off info for future use
        // if (level == 1 && (MAKEINTRESOURCE(resourceType) == RT_DIALOG))
        if (level == 1 && (resourceType == (WORD)RT_DIALOG))
        {
            pDlgResEntries = resDirEntry;
            cDlgResEntries = resDir->NumberOfIdEntries;
        }

        for (i = 0; i < resDir->NumberOfNamedEntries; i++, resDirEntry++)
            DumpResourceEntry(resDirEntry, resourceBase, level + 1);

        for (i = 0; i < resDir->NumberOfIdEntries; i++, resDirEntry++)
            DumpResourceEntry(resDirEntry, resourceBase, level + 1);
    }
    else {
        SPRTF("TODO: DumpResourceDirectory - IMAGE_RESOURCE_DIRECTORY out of range - %p\n", resDir);
    }
}

//
// Top level routine called to dump out the entire resource hierarchy
//
void DumpResourceSection(BYTE *base, PIMAGE_NT_HEADERS pNTHeader)
{
	DWORD resourcesRVA;
    PIMAGE_RESOURCE_DIRECTORY resDir;

	resourcesRVA = GetImgDirEntryRVA(pNTHeader, IMAGE_DIRECTORY_ENTRY_RESOURCE);
	if ( !resourcesRVA )
		return;

    resDir = (PIMAGE_RESOURCE_DIRECTORY)GetPtrFromRVA( resourcesRVA, pNTHeader, base );
    if (!resDir) {
        SPRTF("TODO: (PIMAGE_RESOURCE_DIRECTORY)GetPtrFromRVA failed RVA %08X\n", resourcesRVA);
        return;
    }
		
    if (!fShowResources)
        return;

    if (!IsAddressInRange((BYTE *)resDir, (int)sizeof(IMAGE_RESOURCE_DIRECTORY))) {
        SPRTF("TODO: DumpResourceSection Resources (RVA: %X), PIMAGE_RESOURCE_DIRECTORY out of range - %p\n", resourcesRVA, resDir );
        return;
    }

    // TODO: Need to walk Resource Directories, without output, to see if /ANY/ are valid!!!
    // Presently, for say msvcp100.dll, can ouput 100's of invlaid entries - How to verify before output
    // Immediately following the IMAGE_RESOURCE_DIRECTORY structure is a series of IMAGE_RESOURCE_DIRECTORY_ENTRY's, the number of which are 
    // defined by the WORD NumberOfNamedEntries; and WORD NumberOfIdEntries;...
    WORD nNamed = resDir->NumberOfNamedEntries;
    WORD nIds = resDir->NumberOfIdEntries;
    PIMAGE_RESOURCE_DIRECTORY_ENTRY resDirEntry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(resDir + 1);

    SPRTF("Resources (RVA: %X), named %u, ids %u\n", resourcesRVA, nNamed, nIds );
    UINT i;
    BOOL isValid = TRUE;
    DWORD disd, name, niss, niso, offd, offD, id;
    DWORD isDirectory = 0;
    DWORD isString = 0;
    PIMAGE_RESOURCE_DATA_ENTRY  pResDataEntry;
    for (i = 0; i < nNamed; i++, resDirEntry++) {
        if (!IsAddressInRange((BYTE *)resDirEntry, (int)sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
            isValid = FALSE;
            break;
        }
        disd = resDirEntry->DataIsDirectory;
        id = resDirEntry->Id;
        name = resDirEntry->Name;
        niss = resDirEntry->NameIsString;
        niso = resDirEntry->NameOffset;
        offd = resDirEntry->OffsetToData;
        offD = resDirEntry->OffsetToDirectory;
        // if (resDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
        if (offd & IMAGE_RESOURCE_DATA_IS_DIRECTORY) {
            isDirectory++;
        }
        else {
            // if (resDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING)
            if (name & IMAGE_RESOURCE_NAME_IS_STRING) {
                isString++;
            }
            else {
                pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)((BYTE *)resDir + offd);
                // (resourceBase + resDirEntry->OffsetToData);
                if (!IsAddressInRange((BYTE *)pResDataEntry, (int)sizeof(IMAGE_RESOURCE_DATA_ENTRY))) {
                    isValid = FALSE;
                    break;
                }
            }
        }
    }
    if (isValid) {
        if (nNamed)
            resDirEntry++;
        for (i = 0; i < nIds; i++, resDirEntry++) {
            if (!IsAddressInRange((BYTE *)resDirEntry, (int)sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY))) {
                isValid = FALSE;
                break;
            }
            disd = resDirEntry->DataIsDirectory;
            id = resDirEntry->Id;
            name = resDirEntry->Name;
            niss = resDirEntry->NameIsString;
            niso = resDirEntry->NameOffset;
            offd = resDirEntry->OffsetToData;
            offD = resDirEntry->OffsetToDirectory;
            // if (resDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY)
            if (offd & IMAGE_RESOURCE_DATA_IS_DIRECTORY) {
                isDirectory++;
            }
            else {
                // if (resDirEntry->Name & IMAGE_RESOURCE_NAME_IS_STRING)
                if (name & IMAGE_RESOURCE_NAME_IS_STRING) {
                    isString++;
                }
                else {
                    pResDataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)((BYTE *)resDir + offd);
                    // (resourceBase + resDirEntry->OffsetToData);
                    if (!IsAddressInRange((BYTE *)pResDataEntry, (int)sizeof(IMAGE_RESOURCE_DATA_ENTRY))) {
                        isValid = FALSE;
                        break;
                    }
                }
            }
        }
    }
    if (isValid)
        DumpResourceDirectory(resDir, (BYTE *)resDir, 0, 0);
    else {
        SPRTF("TODO: Walking PIMAGE_RESOURCE_DIRECTORY FAILED\n");
    }

	SPRTF( "\n" );

	if ( cStrResEntries )
	{
		SPRTF( "String Table\n" );

		DumpStringTable( 	base, pNTHeader, (BYTE *)resDir,
							pStrResEntries, cStrResEntries );
		SPRTF( "\n" );
	}

	if ( cDlgResEntries )
	{
		SPRTF( "Dialogs\n" );

		DumpDialogs( 	base, pNTHeader, (BYTE *)resDir,
						pDlgResEntries, cDlgResEntries );
		SPRTF( "\n" );
	}
}
