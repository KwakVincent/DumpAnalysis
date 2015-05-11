#pragma once

#define ALL_EXTENTION		_T("*.*")
#define FIND_EXTENTION		_T("dmp")
#define VALIDDUMPDIR_NAME	_T("ValidDumps")

enum	eVKErrorCode
{
	E_FILEOPEN_INVALID	= 1,
	E_FILEOPEN_ZEROSIZE,
	E_FILEOPEN_MAPPING,
	E_FILEOPEN_MAPVIEW,
	E_FILEOPEN_CHECKHEADER,
	E_FILEOPEN_DUMPSTREAM,

	E_DUMPSTREAM_BADMEM,
	E_DUMPSTREAM_EMPTYTHREAD,
	E_DUMPSTREAM_EMPTYSTACK,
};

typedef multimap<eVKErrorCode, CString>		ErrMap;

class VKAutoDumpAnalyst
{
public:
	VKAutoDumpAnalyst();

	void							CreateDetailDir(...);
	const TCHAR						&GetModulePath();
	void							SetModulePath();
	bool							OpenDumpFile(const TCHAR *pFilePath, CString &sOutPath);
	ErrMap							*GetErrors()			{ return &m_MapError;	}
	bool							IsValidMem(MINIDUMP_MEMORY_LIST *pMem);

	CString							GetErrorStringFromCode(eVKErrorCode);

	void							OutputAnalysisReport();

	int								GetDumpFileCount()			const		{ return m_DumpFileCount;	}
	int								GetInvalidDumpFileCount()	const		{ return m_InvalidDumpFileCount;	}
	int								GetValidDumpFileCount()		const		{ return m_ValidDumpFileCount;	}

protected:
	int								m_DumpFileCount;
	int								m_InvalidDumpFileCount;
	int								m_ValidDumpFileCount;
	ErrMap							m_MapError;
	vector<MINIDUMP_EXCEPTION>		m_ExceptionInfo;
};
