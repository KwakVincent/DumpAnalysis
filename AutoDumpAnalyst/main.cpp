#include "stdafx.h"
#include "VKAutoDumpAnalyst.h"

CString sValidDumpDir;
int findFileCount		= 0;

vector<CString>		g_FileList;
vector<CString>		g_VisitDirectory;

bool MoveTargetFile(const TCHAR *sFrom, const TCHAR *sTo)
{
	if(CopyFile(sFrom, sTo, FALSE))
	{
		g_FileList.push_back(sFrom);
		DeleteFile(sFrom);
	}

	return true;
}

int CheckValiedDump(const TCHAR *SearchPath, VKAutoDumpAnalyst &analyst)
{
	system("cls");

	_tprintf_s(_T("File File		: %d\n"), findFileCount);
	_tprintf_s(_T("Find Dump File		: %d\n"), analyst.GetDumpFileCount());
	_tprintf_s(_T("Valid Dump File		: %d\n"), analyst.GetValidDumpFileCount());
	_tprintf_s(_T("Invalid Dump File	: %d\n"), analyst.GetInvalidDumpFileCount());

	CString sAllFile	= ALL_EXTENTION;
	CString sDumpFile	= FIND_EXTENTION;

	HANDLE	hFile;
	WIN32_FIND_DATA	findData;

	CString sFindPath;
	sFindPath.Format(_T("%s\\%s"), SearchPath, sAllFile);
	hFile = FindFirstFile(sFindPath, &findData);

	if(hFile == INVALID_HANDLE_VALUE)
		return 0;

	do
	{
 		if(findData.cFileName[0] == _T('.'))
			continue;

		//Folder 
		if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if(_tcscmp(findData.cFileName, VALIDDUMPDIR_NAME) == 0)
				continue;

			CString sNewDirectory;
			sNewDirectory.Format(_T("%s\\%s"), SearchPath, findData.cFileName);

			auto iter = find(g_VisitDirectory.begin(), g_VisitDirectory.end(), sNewDirectory); 
			
			if(iter == g_VisitDirectory.end())
			{
				g_VisitDirectory.push_back(sNewDirectory);
				CheckValiedDump(sNewDirectory, analyst);
			}
			else
			{
				::MessageBox(NULL, L"test", L"test", MB_OK);
			}
		}
		//File
		else
		{
			CString sExtention = findData.cFileName;

			const int nExIndex = sExtention.ReverseFind(_T('.'));
			sExtention.Delete(0, nExIndex + 1);

			if(sExtention.Compare(sDumpFile))
				continue;
			
			CString sDumpPath, sAddPath;
			sDumpPath.Format(_T("%s\\%s"), SearchPath, findData.cFileName);

			if(analyst.OpenDumpFile(sDumpPath, sAddPath))
			{
				CString sNewFilePath;
				sNewFilePath.Format(_T("%s\\%s\\%s"), sValidDumpDir, sAddPath, findData.cFileName);
				MoveTargetFile(sDumpPath, sNewFilePath);
			}
		}
	}
	while(FindNextFile(hFile, &findData));

	FindClose(hFile);

	return static_cast<int>(g_FileList.size());
}

int _tmain(int argc, _TCHAR* argv[])
{
	setlocale(LC_ALL, "");
	VKAutoDumpAnalyst analyst;

#ifndef _DEBUG
	analyst.SetModulePath();
#endif
	const DWORD dwBeginTime = GetTickCount();

	CString sCurPath;
	GetCurrentDirectory(MAX_PATH, sCurPath.GetBuffer(MAX_PATH));
	sCurPath.ReleaseBuffer();

	sValidDumpDir.Format(_T("%s\\%s"), sCurPath, VALIDDUMPDIR_NAME);

	//폴더 생성..
	if(!PathIsDirectory(sValidDumpDir))
	{
		::CreateDirectory(sValidDumpDir, NULL);
	}

	int nValidDumpCount = CheckValiedDump(sCurPath, analyst);
	if(nValidDumpCount == 0)
	{
		MessageBox(NULL, _T("분석가능한 Dump File이 존재하지 않습니다."), _T("Error"), MB_OK);
	}

	ErrMap &mapErrorList = *analyst.GetErrors();
	printf("======================= Complate Sort Valid Dump ==+======================\n");
	printf("Valid Dump Count	: %d\n", nValidDumpCount);
	printf("=========================== Invalid Dump List ============================\n");
	printf("Invalid Dump Count	: %u\n", mapErrorList.size());
	printf("=========================== Write Report =================================\n");
	analyst.OutputAnalysisReport();
	printf("Analysis Time : %d ms\n", GetTickCount() - dwBeginTime);
	printf("Complate Analysis Dump. Thank!\n");

	getchar();

	return 0;
}
