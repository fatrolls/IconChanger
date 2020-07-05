#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>
#include "resource.h"

// g (Global optimization), s (Favor small code), y (No frame pointers).
#pragma optimize("gsy", on)
#pragma comment(linker, "/FILEALIGN:0x200")
#pragma comment(linker, "/MERGE:.rdata=.data")
#pragma comment(linker, "/MERGE:.text=.data")
#pragma comment(linker, "/MERGE:.reloc=.data")
#pragma comment(linker, "/SECTION:.text, EWR /IGNORE:4078")
#pragma comment(linker, "/OPT:NOWIN98")		// Make section alignment really small.
#define WIN32_LEAN_AND_MEAN

#define BUFFERSIZE		32768

typedef struct _ICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  DWORD dwImageOffset;
} ICONDIRENTRY, 
 * LPICONDIRENTRY;

typedef struct _ICONDIR {
  WORD idReserved;
  WORD idType;
  WORD idCount;
  ICONDIRENTRY idEntries[1];
} ICONDIR, 
 * LPICONDIR;

#pragma pack(push)
#pragma pack(2)
typedef struct _GRPICONDIRENTRY {
  BYTE bWidth;
  BYTE bHeight;
  BYTE bColorCount;
  BYTE bReserved;
  WORD wPlanes;
  WORD wBitCount;
  DWORD dwBytesInRes;
  WORD nID;
} GRPICONDIRENTRY, 
 * LPGRPICONDIRENTRY;
#pragma pack(pop)

#pragma pack(push)
#pragma pack(2)
typedef struct _GRPICONDIR {
  WORD idReserved;
  WORD idType;
  WORD idCount;
  GRPICONDIRENTRY idEntries[1];
} GRPICONDIR, 
 * LPGRPICONDIR;
#pragma pack(pop)

BOOL			CALLBACK Main(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);

HINSTANCE		hInst;
HWND			hDlg;
char			szFileName[MAX_PATH]= "";
char			iconfilename[MAX_PATH]= "";
char			*szBuffer;

int WINAPI WinMain(HINSTANCE hinstCurrent,HINSTANCE hinstPrevious,LPSTR lpszCmdLine,int nCmdShow){
	hInst= hinstCurrent;
	DialogBox(hinstCurrent,MAKEINTRESOURCE(IDD_MAIN),NULL,(DLGPROC)Main);
	return 0;
}

BOOL CALLBACK Main(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam){
	HDC				hdc;
	PAINTSTRUCT		ps;
	HMODULE			hLibModule;
	HANDLE			hFile, hLoadFile= NULL, hIcon= NULL;
	LPICONDIR		lpID= NULL;
	LPGRPICONDIR	lpGID= NULL;
	OPENFILENAME	ofn, oin;
	DWORD			dwFileSize, dwBytesRead;
	LPBYTE			lpBuffer;
	char			szBackup[MAX_PATH];
	int				i;
	HANDLE			hUpdate;

	switch(uMsg){
		case WM_INITDIALOG:
			SendMessageA(hDlg,WM_SETICON,ICON_SMALL, (LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
			SendMessageA(hDlg,WM_SETICON, ICON_BIG,(LPARAM) LoadIcon(hInst,MAKEINTRESOURCE(IDI_ICON)));
		break;
		case WM_COMMAND:
			if( LOWORD(wParam)==IDC_LOADFILE ){
				if( HIWORD(wParam)==BN_CLICKED ){
					ZeroMemory(&ofn,sizeof(ofn));
					szFileName[0]= '\0';
					ofn.lStructSize = sizeof(OPENFILENAME);
					ofn.hwndOwner=hDlg;
					ofn.lpstrFilter= "All files (*.*)\0*.*\0";
					ofn.lpstrFile= szFileName;
					ofn.nMaxFile= MAX_PATH;
					ofn.Flags= OFN_EXPLORER|OFN_PATHMUSTEXIST;
					GetOpenFileName(&ofn);
					
					if( hLoadFile!=NULL ){
						CloseHandle(hLoadFile);
					}
					hLoadFile= CreateFile(szFileName, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
					if( hLoadFile==INVALID_HANDLE_VALUE ){
						MessageBox(hDlg,"No File or could not open the selected file.","Error accessing file.",MB_OK);
					}else{
						SetDlgItemText(hDlg, IDC_FILENAME, szFileName);
					}
					CloseHandle(hLoadFile);
				}
			}
			if( LOWORD(wParam)==IDC_LOADICON ){
				if( HIWORD(wParam)==BN_CLICKED ){
					ZeroMemory(&oin,sizeof(oin));
					iconfilename[0]= '\0';
					oin.lStructSize= sizeof(OPENFILENAME);
					oin.hwndOwner= hDlg;
					oin.lpstrFilter= "Icon files (*.ico)\0*.ico\0";
					oin.lpstrFile= iconfilename;
					oin.nMaxFile= MAX_PATH;
					oin.Flags= OFN_EXPLORER|OFN_PATHMUSTEXIST;
					GetOpenFileName(&oin);
					
					if( hIcon!=NULL ){
						CloseHandle(hIcon);
					}
					hIcon= CreateFile(iconfilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
					dwFileSize= GetFileSize(hIcon, NULL);
					szBuffer= (char*) malloc(4096);
					memset(szBuffer, 0, sizeof(szBuffer));
					ReadFile(hIcon, szBuffer, dwFileSize, &dwBytesRead, NULL);
					if( hIcon==INVALID_HANDLE_VALUE ){
						MessageBox(hDlg, "No Icon or could not open the selected icon.", "Error accessing icon.",MB_OK);
					}else{
						EnableWindow(GetDlgItem(hDlg,IDC_OK), TRUE);
						CloseHandle(hIcon);
						hIcon= ExtractIcon(hInst, iconfilename, 0);
						SendDlgItemMessage(hDlg, IDC_ICONIMG, STM_SETICON, hIcon, 0);
					}
					
				}
			}
			if( LOWORD(wParam)==IDC_OK ){
				if( HIWORD(wParam)==BN_CLICKED ){
					// Create a backup of the .exe file.
					GetDlgItemText(hDlg, IDC_FILENAME, szBackup, MAX_PATH);
					lstrcat(szBackup, ".bak");
					GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
					CopyFile(szFileName, szBackup, FALSE);

					hFile= CreateFile(iconfilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
					if( hFile==INVALID_HANDLE_VALUE ){
						MessageBox(hDlg, "Could not change icon!", "Error", MB_ICONERROR);
						return TRUE;
					}
					lpID= (LPICONDIR)malloc(sizeof(ICONDIR));
					ReadFile(hFile, &lpID->idReserved, sizeof(WORD), &dwBytesRead, NULL);
					ReadFile(hFile, &lpID->idType, sizeof(WORD), &dwBytesRead, NULL);
					ReadFile(hFile, &lpID->idCount, sizeof(WORD), &dwBytesRead, NULL);
					lpID= (LPICONDIR)realloc(lpID, (sizeof(WORD) * 3) + (sizeof(ICONDIRENTRY) * lpID->idCount));
					ReadFile(hFile, &lpID->idEntries[0], sizeof(ICONDIRENTRY) * lpID->idCount, &dwBytesRead, NULL);
					lpGID= (LPGRPICONDIR)malloc(sizeof(GRPICONDIR));
					lpGID->idReserved= lpID->idReserved;
					lpGID->idType= lpID->idType;
					lpGID->idCount= lpID->idCount;
					lpGID= (LPGRPICONDIR)realloc(lpGID, (sizeof(WORD) * 3) + (sizeof(GRPICONDIRENTRY) * lpGID->idCount));

					for( i=0; i<lpGID->idCount; i++){
						lpGID->idEntries[i].bWidth= lpID->idEntries[i].bWidth;
						lpGID->idEntries[i].bHeight= lpID->idEntries[i].bHeight;
						lpGID->idEntries[i].bColorCount= lpID->idEntries[i].bColorCount;
						lpGID->idEntries[i].bReserved= lpID->idEntries[i].bReserved;
						lpGID->idEntries[i].wPlanes= lpID->idEntries[i].wPlanes;
						lpGID->idEntries[i].wBitCount= lpID->idEntries[i].wBitCount;
						lpGID->idEntries[i].dwBytesInRes= lpID->idEntries[i].dwBytesInRes;
						lpGID->idEntries[i].nID= i+1;
					}
					GetDlgItemText(hDlg, IDC_FILENAME, szFileName, MAX_PATH);
					hUpdate= BeginUpdateResource(szFileName, FALSE);
					if( hUpdate==NULL ){
						MessageBox(hDlg, "Could not change icon!", "Error", MB_ICONERROR);
						return TRUE;
					}
					for( i=0; i<lpID->idCount; i++){
						lpBuffer= (LPBYTE)malloc(lpID->idEntries[i].dwBytesInRes);
						SetFilePointer(hFile, lpID->idEntries[i].dwImageOffset, NULL, FILE_BEGIN);
						ReadFile(hFile, lpBuffer, lpID->idEntries[i].dwBytesInRes, &dwBytesRead, NULL);
						if( UpdateResource(hUpdate, RT_ICON, MAKEINTRESOURCE(lpGID->idEntries[i].nID), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), &lpBuffer[0], lpID->idEntries[i].dwBytesInRes)==FALSE ){
							MessageBox(hDlg, "Could not change icon!", "Error", MB_ICONERROR);
							free(lpBuffer);
							return TRUE;
						}
						free(lpBuffer);
					}
					CloseHandle(hFile);
					if( UpdateResource(hUpdate, RT_GROUP_ICON, MAKEINTRESOURCE(1), MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), &lpGID[0], (sizeof(WORD) * 3) + (sizeof(GRPICONDIRENTRY) * lpGID->idCount))==FALSE ){
						MessageBox(hDlg, "Could not change icon!", "Error", MB_ICONERROR);
						return TRUE;
					}
					if( EndUpdateResource(hUpdate, FALSE)==FALSE ){
						MessageBox(hDlg, "Could not change icon!", "Error", MB_ICONERROR);
						return TRUE;
					}
					MessageBox(hDlg, "Icon successfully changed!", "Information", MB_ICONINFORMATION);
					break;
				}
			}
			if( LOWORD(wParam)==IDC_EXIT ){
				if( HIWORD(wParam)==BN_CLICKED ){
					EndDialog(hDlg,wParam);
					DestroyWindow(hDlg);
				}
			}
		break;
		case WM_PAINT:
			hdc= BeginPaint(hDlg, &ps);
			InvalidateRect(hDlg, NULL, TRUE);
			EndPaint (hDlg, &ps);
		return 0;
		case WM_CLOSE:			
			EndDialog(hDlg,wParam);
			DestroyWindow(hDlg);
		break;
		case WM_DESTROY:
			PostQuitMessage(0);
		break;
	}
	return 0;
}
