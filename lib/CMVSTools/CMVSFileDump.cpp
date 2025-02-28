#include "CmvsFileDump.h"
#include "CmvsFilePixelArray.h"
#include "../../lib/Rxx/Hook.h"

#include <Windows.h>

namespace CMVSTools
{
	namespace CMVSFileDump
	{
		bool g_isDebug_FileDump = FALSE;
		static LPCSTR g_lpDumpScriptFolder = ".\\DumpScript\\";
		static LPCSTR g_lpDumpPictureFolder = ".\\DumpPicture\\";

		struct PICTUREDECINFO
		{
			PDWORD pfile;
			DWORD size;
			DWORD width;
			DWORD hight;
			DWORD bit;
			DWORD type;
		};

		typedef PDWORD(__thiscall* pReadScript)(
			PVOID pTHIS,
			LPCSTR lpFileName,
			PDWORD pFileSize);
		pReadScript rawReadScript = (pReadScript)0x0046CE10;
		//pReadScript rawReadScript = (pReadScript)0x0046CB80;

		typedef BOOL(__thiscall* pReadPicture)(
			PVOID pTHIS,
			PICTUREDECINFO* lpPICTUREDECINFO,
			LPCSTR lpString,
			LPCSTR lpFileName,
			LPCSTR a5);
		pReadPicture rawReadPicture = (pReadPicture)0x0043FDF0;
		//pReadPicture rawReadPicture = (pReadPicture)0x0043FEA0;


		BOOL WritePictureFile(PICTUREDECINFO* lpPICTUREDECINFO, LPCSTR lpFileName)
		{
			PBYTE allocMem = 0;
			DWORD sizeFile = 0;
			CHAR filepath[MAX_PATH] = { 0 };
			tagBITMAPFILEHEADER bFile = { 0 };
			tagBITMAPINFOHEADER bInfo = { 0 };

			lstrcatA(filepath, g_lpDumpPictureFolder);
			lstrcatA(filepath, lpFileName);
			lstrcatA(filepath, ".bmp");

			sizeFile = lpPICTUREDECINFO->size + sizeof(tagBITMAPFILEHEADER) + sizeof(tagBITMAPINFOHEADER);

			allocMem = (PBYTE)VirtualAlloc(NULL, sizeFile, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
			if (allocMem != NULL)
			{
				bFile.bfType = 0x4D42;
				bFile.bfSize = sizeFile;
				bFile.bfOffBits = sizeof(tagBITMAPFILEHEADER) + sizeof(tagBITMAPINFOHEADER);
				bInfo.biSize = sizeof(tagBITMAPINFOHEADER);
				bInfo.biWidth = lpPICTUREDECINFO->width;
				bInfo.biHeight = lpPICTUREDECINFO->hight;
				bInfo.biPlanes = 0x1;
				bInfo.biBitCount = (WORD)lpPICTUREDECINFO->bit;

				memcpy(&allocMem[1], &bFile, sizeof(tagBITMAPFILEHEADER));
				memcpy(&allocMem[1 + sizeof(tagBITMAPFILEHEADER)], &bInfo, sizeof(tagBITMAPINFOHEADER));
				memcpy(&allocMem[1 + sizeof(tagBITMAPFILEHEADER) + sizeof(tagBITMAPINFOHEADER)], lpPICTUREDECINFO->pfile, lpPICTUREDECINFO->size);
				DumpBMPFile(&allocMem[1], filepath, lpPICTUREDECINFO->size);

				VirtualFree(allocMem, NULL, MEM_RELEASE);
				return TRUE;
			}

			return FALSE;
		}

		BOOL WriteScriptFile(LPCSTR lpFileName, PDWORD pFile, PDWORD dwSize)
		{
			CHAR filepath[MAX_PATH] = { 0 };
			lstrcatA(filepath, g_lpDumpScriptFolder);
			lstrcatA(filepath, lpFileName);

			HANDLE hFile = CreateFileA(filepath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, pFile, *dwSize, NULL, NULL);
				FlushFileBuffers(hFile);
				CloseHandle(hFile);
				return TRUE;
			}
			return FALSE;
		}

		PDWORD __fastcall newReadScript(PVOID pTHIS, DWORD dwReserved, LPCSTR lpFileName, PDWORD pFileSize)
		{
			PDWORD pFile = rawReadScript(pTHIS, lpFileName, pFileSize);

			if (pFile != NULL)
			{
				BOOL isDump = WriteScriptFile(lpFileName, pFile, pFileSize);
				if (g_isDebug_FileDump)
				{
					if (isDump)
					{
						printf("DumpFile:%s - Successful!!", lpFileName);
					}
					else
					{
						printf("DumpFile:%s - Failed!!!", lpFileName);
					}
				}
			}

			return pFile;
		}

		BOOL __fastcall newReadPicture(PVOID pTHIS, DWORD dwReserved, PICTUREDECINFO* lpPICTUREDECINFO, LPCSTR lpString, LPCSTR lpFileName, LPCSTR lpUnknow)
		{
			BOOL isRead = rawReadPicture(pTHIS, lpPICTUREDECINFO, lpString, lpFileName, lpUnknow);

			if (isRead == TRUE)
			{
				BOOL isDump = WritePictureFile(lpPICTUREDECINFO, lpFileName);
				if (g_isDebug_FileDump)
				{
					if (isDump)
					{
						printf("DumpFile:%s - Successful!!", lpFileName);
					}
					else
					{
						printf("DumpFile:%s - Failed!!!", lpFileName);
					}
				}
			}

			return isRead;
		}

		void DumpFile()
		{
			CreateDirectoryA(g_lpDumpScriptFolder, NULL);
			CreateDirectoryA(g_lpDumpPictureFolder, NULL);

			Rut::HookX::DetourAttachFunc(&rawReadPicture, newReadPicture);
			Rut::HookX::DetourAttachFunc(&rawReadScript, newReadScript);
		}
	}
}

