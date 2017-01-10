// ICO.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "ICO.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int filter(unsigned int code, struct _EXCEPTION_POINTERS *ep,CHAR *fileName) 
{

	if (code == EXCEPTION_ACCESS_VIOLATION) 
	{
		MessageBoxA( hWndMain, "Eroare LoadLibrary: verifica daca exista fisierul", fileName, MB_OK );
		CloseHandle(hFile);
		FreeLibrary(currentUI);

		return EXCEPTION_EXECUTE_HANDLER;
	}

   else 
   {

      puts("didn't catch AV, unexpected.");

      return EXCEPTION_CONTINUE_SEARCH;

   }

}

LPICONIMAGE* ExtractIcoFromFile(LPSTR filename,LPICONDIR pIconDir)
{
	BOOL res=true;
	HANDLE	hFile1 = NULL, hFile2=NULL, hFile3=NULL;
	//LPICONDIR pIconDir;
	DWORD	dwBytesRead;
	LPICONIMAGE pIconImage;
	LPICONIMAGE *arrayIconImage;
	DWORD cbInit=0,cbOffsetDir=0,cbOffset=0,cbInitOffset=0;
	BYTE *temp;
	int i;

	
	if( (hFile1 = CreateFileA( filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL )) == INVALID_HANDLE_VALUE )
    {
        MessageBoxA( hWndMain, "Error Opening File for Reading", filename, MB_OK );
        return NULL;
    }
	
	
	ReadFile( hFile1, &(pIconDir->idReserved), sizeof( WORD ), &dwBytesRead, NULL );
	ReadFile( hFile1, &(pIconDir->idType), sizeof( WORD ), &dwBytesRead, NULL );
	ReadFile( hFile1, &(pIconDir->idCount), sizeof( WORD ), &dwBytesRead, NULL );

#ifdef WRICOFILE
	hFile2 = CreateFileA("replicaICO.ico", GENERIC_READ | GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES) NULL,
						CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,(HANDLE) NULL); 
	if (hFile2 == INVALID_HANDLE_VALUE)
	{
        MessageBoxA( hWndMain, "Error Opening File for Reading", "replicaICO.ico", MB_OK );
        return NULL;
    }
	WriteFile(hFile2, &(pIconDir->idReserved),dwBytesRead, &cbWritten, NULL);
	WriteFile(hFile2, &(pIconDir->idType),dwBytesRead, &cbWritten, NULL);
	WriteFile(hFile2, &(pIconDir->idCount),dwBytesRead, &cbWritten, NULL);
#endif
	
	
	pIconDir->idEntries = new ICONDIRENTRY[pIconDir->idCount];
	
	// Read the ICONDIRENTRY elements
	temp=new BYTE[sizeof(ICONDIRENTRY)];
	for(i=0;i<pIconDir->idCount;i++)
	{
		ReadFile( hFile1, &pIconDir->idEntries[i], sizeof(ICONDIRENTRY), &dwBytesRead, NULL );
#ifdef WRICOFILE
		WriteFile(hFile2, &pIconDir->idEntries[i],dwBytesRead, &cbWritten, NULL);
#endif
	}
	arrayIconImage=new LPICONIMAGE[pIconDir->idCount];
	// Loop through and read in each image
	for(i=0;i<pIconDir->idCount;i++)
	{
		pIconImage = (LPICONIMAGE)malloc( pIconDir->idEntries[i].dwBytesInRes );
		SetFilePointer( hFile1, pIconDir->idEntries[i].dwImageOffset,NULL, FILE_BEGIN );
		ReadFile( hFile1, pIconImage, pIconDir->idEntries[i].dwBytesInRes,&dwBytesRead, NULL );
		arrayIconImage[i]=(LPICONIMAGE)malloc( pIconDir->idEntries[i].dwBytesInRes );
		memcpy( arrayIconImage[i],pIconImage, pIconDir->idEntries[i].dwBytesInRes );
		free(pIconImage);
		
#ifdef WRICOFILE
		SetFilePointer( hFile2, pIconDir->idEntries[i].dwImageOffset,NULL, FILE_BEGIN );
		WriteFile(hFile2, pIconImage,dwBytesRead, &cbWritten, NULL);
#endif	
	}
	CloseHandle(hFile1);
#ifdef WRICOFILE
	CloseHandle(hFile2);
#endif 
	return arrayIconImage;
}

BOOL ReplaceIconResource(LPSTR lpFileName, LPCTSTR lpName, UINT langId, LPICONDIR pIconDir,	LPICONIMAGE* pIconImage)
{
	BOOL res=true;
	HANDLE	hFile3=NULL;
	LPMEMICONDIR lpInitGrpIconDir=new MEMICONDIR;
	
	//LPICONIMAGE pIconImage;
	HINSTANCE hUi;
	BYTE *test,*test1,*temp,*temp1;
	DWORD cbInit=0,cbOffsetDir=0,cbOffset=0,cbInitOffset=0;
	WORD cbRes=0;
	int i;

	hUi = LoadLibraryExA(lpFileName,NULL,DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
	HRSRC hRsrc = FindResourceEx(hUi, RT_GROUP_ICON, lpName,langId);
	//nu stiu de ce returneaza 104 wtf???
	//cbRes=SizeofResource( hUi, hRsrc );
	

	HGLOBAL hGlobal = LoadResource( hUi, hRsrc );
	test1 =(BYTE*) LockResource( hGlobal );
	temp1=test1;
//	temp1=new BYTE[118];
//	CopyMemory(temp1,test1,118);
	lpInitGrpIconDir->idReserved=(WORD)*test1;
	test1=test1+sizeof(WORD);
	lpInitGrpIconDir->idType=(WORD)*test1;
	test1=test1+sizeof(WORD);
	lpInitGrpIconDir->idCount=(WORD)*test1;
	test1=test1+sizeof(WORD);

	lpInitGrpIconDir->idEntries=new MEMICONDIRENTRY[lpInitGrpIconDir->idCount];

	for(i=0;i<lpInitGrpIconDir->idCount;i++)
	{
		lpInitGrpIconDir->idEntries[i].bWidth=(BYTE)*test1;
		test1=test1+sizeof(BYTE);
		lpInitGrpIconDir->idEntries[i].bHeight=(BYTE)*test1;
		test1=test1+sizeof(BYTE);
		lpInitGrpIconDir->idEntries[i].bColorCount=(BYTE)*test1;
		test1=test1+sizeof(BYTE);
		lpInitGrpIconDir->idEntries[i].bReserved=(BYTE)*test1;
		test1=test1+sizeof(BYTE);
 		lpInitGrpIconDir->idEntries[i].wPlanes=(WORD)*test1;
		test1=test1+sizeof(WORD);
		lpInitGrpIconDir->idEntries[i].wBitCount=(WORD)*test1;
		test1=test1+sizeof(WORD);
		//nu merge cu (DWORD)*test
		lpInitGrpIconDir->idEntries[i].dwBytesInRes=pIconDir->idEntries[i].dwBytesInRes;
		test1=test1+sizeof(DWORD);
		lpInitGrpIconDir->idEntries[i].nID=(WORD)*test1;
		test1=test1+sizeof(WORD);
	}
//	memcpy( lpInitGrpIconDir->idEntries, test, cbRes-3*sizeof(WORD) );

	UnlockResource((HGLOBAL)test1);
	
	LPMEMICONDIR lpGrpIconDir=new MEMICONDIR;
	lpGrpIconDir->idReserved=pIconDir->idReserved;
	lpGrpIconDir->idType=pIconDir->idType;
	lpGrpIconDir->idCount=pIconDir->idCount;
	cbRes=3*sizeof(WORD)+lpGrpIconDir->idCount*sizeof(MEMICONDIRENTRY);
	test=new BYTE[cbRes];
	temp=test;
	CopyMemory(test,&lpGrpIconDir->idReserved,sizeof(WORD));
	test=test+sizeof(WORD);
	CopyMemory(test,&lpGrpIconDir->idType,sizeof(WORD));
	test=test+sizeof(WORD);
	CopyMemory(test,&lpGrpIconDir->idCount,sizeof(WORD));
	test=test+sizeof(WORD);

	lpGrpIconDir->idEntries=new MEMICONDIRENTRY[lpGrpIconDir->idCount];
	for(i=0;i<lpGrpIconDir->idCount;i++)
	{
		lpGrpIconDir->idEntries[i].bWidth=pIconDir->idEntries[i].bWidth;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].bWidth,sizeof(BYTE));
		test=test+sizeof(BYTE);
		lpGrpIconDir->idEntries[i].bHeight=pIconDir->idEntries[i].bHeight;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].bHeight,sizeof(BYTE));
		test=test+sizeof(BYTE);
		lpGrpIconDir->idEntries[i].bColorCount=pIconDir->idEntries[i].bColorCount;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].bColorCount,sizeof(BYTE));
		test=test+sizeof(BYTE);
		lpGrpIconDir->idEntries[i].bReserved=pIconDir->idEntries[i].bReserved;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].bReserved,sizeof(BYTE));
		test=test+sizeof(BYTE);
		lpGrpIconDir->idEntries[i].wPlanes=pIconDir->idEntries[i].wPlanes;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].wPlanes,sizeof(WORD));
		test=test+sizeof(WORD);
		lpGrpIconDir->idEntries[i].wBitCount=pIconDir->idEntries[i].wBitCount;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].wBitCount,sizeof(WORD));
		test=test+sizeof(WORD);
		lpGrpIconDir->idEntries[i].dwBytesInRes=pIconDir->idEntries[i].dwBytesInRes;
		CopyMemory(test,&lpGrpIconDir->idEntries[i].dwBytesInRes,sizeof(DWORD));
		test=test+sizeof(DWORD);
		if(i<lpInitGrpIconDir->idCount) //nu am depasit numarul initial de RT_ICON
			lpGrpIconDir->idEntries[i].nID=lpInitGrpIconDir->idEntries[i].nID;
		else
		{
			nMaxID++;
			lpGrpIconDir->idEntries[i].nID=nMaxID; //adaug noile ICO la sfarsitul RT_ICON-urilor
		}
		CopyMemory(test,&lpGrpIconDir->idEntries[i].nID,sizeof(WORD));
		test=test+sizeof(WORD);
	}

	

	//offsetul de unde incep structurile ICONIMAGE
	cbInitOffset=3*sizeof(WORD)+lpGrpIconDir->idCount*sizeof(ICONDIRENTRY);
	cbOffset=cbInitOffset; //cbOffset=118
	
	FreeLibrary(hUi);

	HANDLE hUpdate;
	
	
	
	_chmod((char*)lpFileName,_S_IWRITE);
	hUpdate = BeginUpdateResourceA(lpFileName, FALSE); //false sa nu stearga resursele neupdated
	if(hUpdate==NULL)
	{

		MessageBoxA( hWndMain, "eroare BeginUpdateResource", lpFileName, MB_OK );
		res=false;
	}
	//aici e cu lang NEUTRAL
	//res=UpdateResource(hUpdate,RT_GROUP_ICON,MAKEINTRESOURCE(6000),langId,lpGrpIconDir,cbRes);
 	res=UpdateResource(hUpdate,RT_GROUP_ICON,lpName,langId,temp,cbRes);
	if(res==false)
		MessageBoxA( hWndMain, "eroare UpdateResource RT_GROUP_ICON", lpFileName, MB_OK );
	
	for(i=0;i<lpGrpIconDir->idCount;i++)
	{
		res=UpdateResource(hUpdate,RT_ICON,MAKEINTRESOURCE(lpGrpIconDir->idEntries[i].nID),langId,pIconImage[i],lpGrpIconDir->idEntries[i].dwBytesInRes);
		if(res==false)
			MessageBoxA( hWndMain, "eroare UpdateResource RT_ICON", lpFileName, MB_OK );
	}

	for(i=lpGrpIconDir->idCount;i<lpInitGrpIconDir->idCount;i++)
	{
		res=UpdateResource(hUpdate,RT_ICON,MAKEINTRESOURCE(lpInitGrpIconDir->idEntries[i].nID),langId,"",0);
		if(res==false)
			MessageBoxA( hWndMain, "eroare stergere resurse vechi", lpFileName, MB_OK );
			
	}

	if(!EndUpdateResource(hUpdate,FALSE)) //false ->resource updates will take effect.
		MessageBoxA( hWndMain, "eroare EndUpdateResource", lpFileName, MB_OK );
		
	
	delete[] lpGrpIconDir->idEntries;
	delete lpGrpIconDir;
	delete[] temp;

	return res;
}

BOOL EnumLangsFunc(HANDLE hModule,LPCTSTR lpType, LPCTSTR lpName,WORD wLang,LONG lParam)
{
	currentLangId=(UINT)wLang;
	return true;
}

BOOL EnumNamesFunc(HANDLE hModule, LPCTSTR lpType, LPTSTR lpName, LONG lParam)  
{ 
	
	
	if(IS_INTRESOURCE(lpName))
	{

		if(lpType==RT_ICON)
		{
			if((USHORT)lpName>nMaxID)
				nMaxID=(USHORT)lpName;
		}

		if(lpType==RT_GROUP_ICON)
		{
			EnumResourceLanguages((HMODULE)hModule,lpType,lpName,(ENUMRESLANGPROC)EnumLangsFunc,0);
			pBundles[cBundles].nBundles=(USHORT)lpName;
			pBundles[cBundles].nLangId=(USHORT)currentLangId;
			cBundles++;
		}
	} 
	

	return true;
}

//////callback function pentru enumerate types//////
//(module handle, address of res type, extra pram)//
BOOL EnumTypesFunc(HANDLE hModule, LPTSTR lpType, LONG lParam)
{
	LPSTR szBuffer;
	szBuffer=(LPSTR)new CHAR[300];    // print buffer for EnumResourceTypes
	

    ///// Find the names of all resources of type String. //////
	if(lpType==RT_ICON||lpType==RT_GROUP_ICON) //e string
	{
		if(EnumResourceNames((HINSTANCE)hModule,lpType,(ENUMRESNAMEPROC)EnumNamesFunc,0)==false)
		{
			MessageBoxA( hWndMain, "eroare EnumResourceNames","RT_ICON||RT_GROUP_ICON", MB_OK );
			return false;
		}
	}
	delete[] szBuffer;	 
	return true;
}

BOOL CProcFile(LPSTR lpSrcFileName)
{
		HINSTANCE hui;
		

		//hui=LoadLibraryA(fileName);

		hui = LoadLibraryExA(lpSrcFileName,NULL,DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE);
		if (hui == NULL) 
		{
			MessageBoxA( hWndMain, "Eroare LoadLibrary: verifica daca exista fisierul", lpSrcFileName, MB_OK );
			//DWORD dwLastError = GetLastError(); 
			//wprintf(L"failed with error %d: \n", dwLastError);
			return false;
		}
		else
		{
			//cout<<"\nOPENED library"<<lpSrcFileName;
			currentUI=hui;
		}
		
		if(EnumResourceTypes(hui,(ENUMRESTYPEPROC)EnumTypesFunc,0)==false)
		{
			_tprintf(_T("eroare enum res types\n"));
			return 0;
		}
		//////////////////////////////////////

		FreeLibrary(hui);
		
	

	return true;
}

/*
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;

	// initialize MFC and print and error on failure
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: change error code to suit your needs
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		nRetCode = 1;
	}
	else
	{
		// TODO: code your application's behavior here.
		CHAR *szDLLname, *szICOname, *temp;
		int bwritten,bundle,i,lang;
		HRESULT hResult;
		szDLLname=new CHAR[300];
		szICOname=new CHAR[300];
		temp=new CHAR[300];
		bwritten=WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)argv[2], 300, (LPSTR)szDLLname, 300, NULL, NULL);
		bwritten=WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)argv[3], 300, (LPSTR)szICOname, 300, NULL, NULL);
		bwritten=WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)argv[1], 300, (LPSTR)temp, 300, NULL, NULL);
		
		bundle=atoi(temp);

		LPICONDIR pIconDir;
		LPICONIMAGE* pIconImage;
		pIconDir = new ICONDIR;
		pIconImage=ExtractIcoFromFile(szICOname,pIconDir);
		__try
		{
			CProcFile(szDLLname);
			for(i=0;i<cBundles;i++)
			{
				if(pBundles[i].nBundles==bundle)
				{
					lang=pBundles[i].nLangId;
					break;
				}
			}
			cBundles=0;
			nMaxID=0;
						
					
			ReplaceIconResource(szDLLname,MAKEINTRESOURCE(bundle),lang, pIconDir, pIconImage);
			for(i=0;i<pIconDir->idCount;i++)
			{
				free(pIconImage[i]);
			}
			delete[] pIconDir->idEntries;
			delete pIconDir;
			free(pIconImage); //aici trebuie eliberat fiecare pointer????
			i=0;
			_leave;
		}
		__except(filter(GetExceptionCode(), GetExceptionInformation(),szDLLname))
		{			
		} 
	}

	return nRetCode;
}
*/