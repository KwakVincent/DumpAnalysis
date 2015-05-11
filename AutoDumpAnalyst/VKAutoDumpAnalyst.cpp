#include "stdafx.h"
#include "VKAutoDumpAnalyst.h"

VKAutoDumpAnalyst::VKAutoDumpAnalyst()
{
	m_DumpFileCount			= 0;
	m_InvalidDumpFileCount	= 0;
	m_ValidDumpFileCount	= 0;
}

CString VKAutoDumpAnalyst::GetErrorStringFromCode(eVKErrorCode eCode)
{
	CString sError;
	switch(eCode)
	{
	case E_FILEOPEN_INVALID:
		sError.Format(_T("잘못된 파일입니다. - File Open"));
		break;
	case E_FILEOPEN_ZEROSIZE:
		sError.Format(_T("0 kb 파일입니다."));
		break;
	case E_FILEOPEN_MAPPING:
		sError.Format(_T("잘못된 파일입니다. - File Mapping\n Error Code : %d"), GetLastError());
		break;
	case E_FILEOPEN_MAPVIEW:
		sError.Format(_T("잘못된 파일입니다. - Map of View"));
		break;
	case E_FILEOPEN_CHECKHEADER:
		sError.Format(_T("헤더가 깨져있습니다."));
		break;
	case E_FILEOPEN_DUMPSTREAM:
		sError.Format(_T("내용이 깨져있습니다."));
		break;
	case E_DUMPSTREAM_BADMEM:
		sError.Format(_T("메모리 주소가 깨져있습니다."));
		break;
	case E_DUMPSTREAM_EMPTYTHREAD:
		sError.Format(_T("쓰레드 정보가 없습니다."));
		break;
	case E_DUMPSTREAM_EMPTYSTACK:
		sError.Format(_T("쓰레드에 스택이 비어있습니다."));
		break;
	default:
		sError.Format(_T("정의되지 않은 에러입니다."));
		break;
	}

	return sError;
}

void VKAutoDumpAnalyst::SetModulePath()
{
	CString sModulePath;
	GetModuleFileName(NULL, sModulePath.GetBuffer(MAX_PATH), MAX_PATH);
	sModulePath.ReleaseBuffer();
	PathRemoveFileSpec(sModulePath.GetBuffer(MAX_PATH));
	sModulePath.ReleaseBuffer();

	SetCurrentDirectory(sModulePath);
}

bool VKAutoDumpAnalyst::IsValidMem(MINIDUMP_MEMORY_LIST *pMem)
{
	try
	{
		MINIDUMP_MEMORY_DESCRIPTOR *check1 = pMem->MemoryRanges;
		ULONG32 check2 = pMem->NumberOfMemoryRanges;
	}
	catch(...)
	{
		return false;
	}

	return true;
}


bool VKAutoDumpAnalyst::OpenDumpFile(const TCHAR *sFilePath, CString &sOutPath)
{
	++m_DumpFileCount;

	bool bResult = false;
	const HANDLE hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if(hFile == INVALID_HANDLE_VALUE) 
	{
		m_MapError.insert(ErrMap::value_type(E_FILEOPEN_INVALID, sFilePath));
		goto OpenDumpFile_FILEEND;
	}

	const DWORD dwFileSize = GetFileSize(hFile, NULL);
	if(dwFileSize == 0)
	{
		m_MapError.insert(ErrMap::value_type(E_FILEOPEN_ZEROSIZE, sFilePath));
		goto OpenDumpFile_FILEEND;
	}

	const HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, NULL, NULL, NULL);

	if(hMapping == NULL) 
	{
		m_MapError.insert(ErrMap::value_type(E_FILEOPEN_MAPPING, sFilePath));
		goto OpenDumpFile_MAPPINGEND;
	}

	const BYTE *pMappingMem = static_cast<BYTE *>(MapViewOfFile(hMapping, FILE_MAP_READ, NULL, NULL, NULL));

	if(pMappingMem == NULL) 
	{
		m_MapError.insert(ErrMap::value_type(E_FILEOPEN_MAPVIEW, sFilePath));
		goto OpenDumpFile_VIEWEND;
	}

	// check header
	if(memcmp(pMappingMem, "MDMP", 4))
	{

		m_MapError.insert(ErrMap::value_type(E_FILEOPEN_CHECKHEADER, sFilePath));
		goto OpenDumpFile_VIEWEND;
	}
	
	MINIDUMP_DIRECTORY	*pMdDir = NULL;
	VOID	*pStreamPointer		= NULL;
	DWORD	dwStreamSize		= 0;

	MINIDUMP_STREAM_TYPE	eStreamType[] = { ThreadListStream, ModuleListStream, MemoryListStream, ExceptionStream, SystemInfoStream };

	for(int i = 0; i < _countof(eStreamType); ++i)
	{
		const BOOL bDumpResult = MiniDumpReadDumpStream((PVOID)pMappingMem, eStreamType[i], &pMdDir, &pStreamPointer, &dwStreamSize) ? true : false;

		if(bDumpResult == FALSE)
		{
			m_MapError.insert(ErrMap::value_type(E_FILEOPEN_DUMPSTREAM, sFilePath));
			goto OpenDumpFile_VIEWEND;
		}
		if(eStreamType[i] == ThreadListStream)
		{
			MINIDUMP_THREAD_LIST *pThreadList = (MINIDUMP_THREAD_LIST *)pStreamPointer;

			if(pThreadList->NumberOfThreads == 0)
			{
				m_MapError.insert(ErrMap::value_type(E_DUMPSTREAM_EMPTYTHREAD, sFilePath));
				goto OpenDumpFile_VIEWEND;
			}

			for(ULONG32 j = 0; j < pThreadList->NumberOfThreads; ++j)
			{
				MINIDUMP_THREAD &curThread = pThreadList->Threads[j];

				MINIDUMP_MEMORY_DESCRIPTOR &statck = curThread.Stack;
				
				//저장된 스택이 없는 경우.
				if(statck.Memory.DataSize == 0)
				{
					m_MapError.insert(ErrMap::value_type(E_DUMPSTREAM_EMPTYSTACK, sFilePath));
					goto OpenDumpFile_VIEWEND;
				}
			}
		}

		if(eStreamType[i] == ModuleListStream)
		{
			MINIDUMP_MODULE_LIST *pModuleList = (MINIDUMP_MODULE_LIST *)pStreamPointer;
			int a = 5;
		}

		if(eStreamType[i] == ExceptionStream)
		{
			MINIDUMP_EXCEPTION_STREAM *pExceptionList = (MINIDUMP_EXCEPTION_STREAM *)pStreamPointer;
			//0x%08x
			MINIDUMP_EXCEPTION *pException = &pExceptionList->ExceptionRecord;

			m_ExceptionInfo.push_back(*pException);
		}

		if(eStreamType[i] == MemoryListStream)
		{
			MINIDUMP_MEMORY_LIST *pMemList = (MINIDUMP_MEMORY_LIST *)pStreamPointer;

			if(!IsValidMem(pMemList))
			{
				m_MapError.insert(ErrMap::value_type(E_DUMPSTREAM_BADMEM, sFilePath));
				goto OpenDumpFile_VIEWEND;
			}

			for(size_t j = 0; j < pMemList->NumberOfMemoryRanges; ++j)
			{
				const BYTE *pVirtualAddress = pMappingMem + pMemList->MemoryRanges[j].Memory.Rva;	//Virtual Address
				const DWORD dwMemSize = pMemList->MemoryRanges[j].Memory.DataSize;
				const ULONG64 beginAddress	= pMemList->MemoryRanges[j].StartOfMemoryRange;
				const ULONG64 endAddress	= beginAddress + dwMemSize;
							
				
				//실제 주소값이 넘어갔는지 검사한다.
				if(dwMemSize != endAddress - beginAddress || beginAddress > endAddress)
				{
					m_MapError.insert(ErrMap::value_type(E_DUMPSTREAM_BADMEM, sFilePath));
					goto OpenDumpFile_VIEWEND;
				}
			}
		}
	}

	bResult = true;

OpenDumpFile_VIEWEND:
	UnmapViewOfFile(pMappingMem);
OpenDumpFile_MAPPINGEND:
	CloseHandle(hMapping);
OpenDumpFile_FILEEND:
	CloseHandle(hFile);

	if(!bResult)
		++m_InvalidDumpFileCount;
	else
	{
		++m_ValidDumpFileCount;
		CString sDesc;
		DWORD dwError = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, 
									m_ExceptionInfo.back().ExceptionCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), sDesc.GetBuffer(MAX_PATH), MAX_PATH, NULL);
		sDesc.ReleaseBuffer();
//		if(sDesc.IsEmpty())
	//		sDesc = _T("Unknow");
		//대분류 - 예외 코드 번호
		sOutPath.Format(_T("ExceptionCode_0x%08x"), m_ExceptionInfo.back().ExceptionCode);
		//EXCEPTION_ACCESS_VIOLATION
		

		//폴더 생성..
		CString sNewPath;
		sNewPath.Format(_T("%s\\%s"), VALIDDUMPDIR_NAME, sOutPath);
		if(!PathIsDirectory(sNewPath))
		{
			::CreateDirectory(sNewPath, NULL);
		}
	}

	return bResult;
}

void VKAutoDumpAnalyst::OutputAnalysisReport()
{
	FILE *pFile = NULL;
	if(_tfopen_s(&pFile, _T("DumpAnalysisReport.txt"), _T("wt,ccs=UNICODE")))
	{
		return ;
	}

	_ftprintf_s(pFile, _T("Dump File	: %d\n"), m_DumpFileCount);
	_ftprintf_s(pFile, _T("Valid Dump File	: %d\n"), m_ValidDumpFileCount);
	_ftprintf_s(pFile, _T("Invalid Dump File	: %d\n"), m_InvalidDumpFileCount);
	_ftprintf_s(pFile, _T("================= Invalid Dump List ======================\n"));

	ErrMap::iterator iter = m_MapError.begin();

	for(size_t i = 0; iter != m_MapError.end(); ++iter, ++i)
	{
		CString sErrorContent = GetErrorStringFromCode((*iter).first);
		_ftprintf_s(pFile, _T("FileName : %s\n"), (*iter).second);
		_ftprintf_s(pFile, _T("Error : %s\n"), sErrorContent);
	}

	fclose(pFile);
}