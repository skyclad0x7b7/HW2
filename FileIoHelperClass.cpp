/**----------------------------------------------------------------------------
* FileIoHelperClass.cpp
*-----------------------------------------------------------------------------
*
*-----------------------------------------------------------------------------
* All rights reserved by somma (fixbrain@gmail.com, unsorted@msn.com)
*-----------------------------------------------------------------------------
* 13:10:2011   17:04 created
**---------------------------------------------------------------------------*/
#include "stdafx.h"
#include "FileIoHelperClass.h"
#include "util.h"

void write_to_console(_In_ DWORD log_level, _In_ wchar_t* function, _In_ wchar_t* format, ...)
{
	static HANDLE	_stdout_handle = INVALID_HANDLE_VALUE;
	static WORD		_old_color = 0x0000;

	//> initialization for console text color manipulation.
	if (INVALID_HANDLE_VALUE == _stdout_handle)
	{
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		_stdout_handle = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(_stdout_handle, &csbi);
		_old_color = csbi.wAttributes;
	}

	std::wstringstream strm_prefix;
	switch (log_level)

	{
	case LL_DEBG:
	{
		strm_prefix << L"[DEBG] " << function << L", ";
		break;
	}
	case LL_INFO:
	{
		strm_prefix << L"[INFO] " << function << L", ";
		SetConsoleTextAttribute(_stdout_handle, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
		break;
	}
	case LL_ERRR:
	{
		strm_prefix << L"[ERR ] " << function << L", ";
		SetConsoleTextAttribute(_stdout_handle, FOREGROUND_RED | FOREGROUND_INTENSITY);
		break;
	}
	case LL_NONE:
	default:
	{
		//strm_prefix << function << L", ";
		break;
	}
	}

	DWORD len;
	va_list args;
	WCHAR msg[4096];

	va_start(args, format);
	if (TRUE != SUCCEEDED(StringCchVPrintf(msg, sizeof(msg), format, args))){ return; }
	va_end(args);

	WriteConsole(_stdout_handle, strm_prefix.str().c_str(), (DWORD)strm_prefix.str().size(), &len, NULL);
	WriteConsole(_stdout_handle, msg, (DWORD)wcslen(msg), &len, NULL);
	WriteConsole(_stdout_handle, L"\n", (DWORD)wcslen(L"\n"), &len, NULL);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), _old_color);

#ifdef _USE_AHNTRACE_
	switch (log_level)
	{
	case LL_DEBG: DEBUGLOG(msg); break;
	case LL_INFO: INFOLOG(msg); break;
	case LL_ERRR: ERRORLOG(msg); break;
	default:
		break;
	}
#endif//_USE_AHNTRACE_
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
FileIoHelper::FileIoHelper()
	: mReadOnly(TRUE),
	mFileHandle(INVALID_HANDLE_VALUE),
	mFileMap(NULL),
	mFileView(NULL)
{
	mFileSize.QuadPart = 0;
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
FileIoHelper::~FileIoHelper()
{
	this->FIOClose();
}

/**----------------------------------------------------------------------------
\brief  ���� IO �� ���� ������ �����Ѵ�.

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
DTSTATUS
FileIoHelper::FIOpenForRead(
IN std::wstring FilePath
)
{
	if (TRUE == Initialized()) { FIOClose(); }

	mReadOnly = TRUE;
	if (TRUE != is_file_existsW(FilePath.c_str()))
	{
		log_err
			L"no file exists. file=%ws", FilePath.c_str()
			log_end
			return DTS_NO_FILE_EXIST;
	}

#pragma warning(disable: 4127)
	DTSTATUS status = DTS_WINAPI_FAILED;
	do
	{
		mFileHandle = CreateFileW(
			FilePath.c_str(),
			GENERIC_READ,
			NULL,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == mFileHandle)
		{
			log_err
				L"CreateFile(%ws) failed, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				log_end
				break;
		}

		// check file size 
		// 
		if (TRUE != GetFileSizeEx(mFileHandle, &mFileSize))
		{
			log_err
				L"%ws, can not get file size, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				log_end
				break;
		}

		mFileMap = CreateFileMapping(
			mFileHandle,
			NULL,
			PAGE_READONLY,
			0,
			0,
			NULL
			);
		if (NULL == mFileMap)
		{
			log_err
				L"CreateFileMapping(%ws) failed, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				log_end
				break;
		}

		status = DTS_SUCCESS;
	} while (FALSE);
#pragma warning(default: 4127)

	if (TRUE != DT_SUCCEEDED(status))
	{
		if (INVALID_HANDLE_VALUE != mFileHandle)
		{
			CloseHandle(mFileHandle);
			mFileHandle = INVALID_HANDLE_VALUE;
		}
		if (NULL != mFileMap) CloseHandle(mFileMap);
	}

	return status;
}

/**----------------------------------------------------------------------------
\brief		FileSize ����Ʈ ¥�� ������ �����Ѵ�.

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
DTSTATUS
FileIoHelper::FIOCreateFile(
IN std::wstring FilePath,
IN LARGE_INTEGER FileSize
)
{
	if (TRUE == Initialized()) { FIOClose(); }
	if (FileSize.QuadPart == 0) return DTS_INVALID_PARAMETER;

	mReadOnly = FALSE;

#pragma warning(disable: 4127)
	DTSTATUS status = DTS_WINAPI_FAILED;
	do
	{
		mFileSize = FileSize;

		mFileHandle = CreateFileW(
			FilePath.c_str(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,		// write ���� �ٸ� ���μ������� �бⰡ ����
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL
			);
		if (INVALID_HANDLE_VALUE == mFileHandle)
		{
			log_err
				L"CreateFile(%ws failed, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				log_end
				break;
		}

		// increase file size
		// 
		if (TRUE != SetFilePointerEx(mFileHandle, mFileSize, NULL, FILE_BEGIN))
		{
			log_err
				L"SetFilePointerEx( move to %I64d ) failed, gle=0x%08x",
				FileSize.QuadPart, GetLastError()
				log_end
				break;
		}

		if (TRUE != SetEndOfFile(mFileHandle))
		{
			log_err L"SetEndOfFile( ) failed, gle=0x%08x", GetLastError() log_end
				break;
		}

		mFileMap = CreateFileMapping(
			mFileHandle,
			NULL,
			PAGE_READWRITE,
			0,
			0,
			NULL
			);
		if (NULL == mFileMap)
		{
			log_err
				L"CreateFileMapping(%ws) failed, gle=0x%08x",
				FilePath.c_str(),
				GetLastError()
				log_end
				break;
		}

		status = DTS_SUCCESS;
	} while (FALSE);
#pragma warning(default: 4127)

	if (TRUE != DT_SUCCEEDED(status))
	{
		if (INVALID_HANDLE_VALUE != mFileHandle)
		{
			CloseHandle(mFileHandle);
			mFileHandle = INVALID_HANDLE_VALUE;
		}
		if (NULL != mFileMap) CloseHandle(mFileMap);
	}

	return status;
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
void
FileIoHelper::FIOClose(
)
{
	if (TRUE != Initialized()) return;

	FIOUnreference();
	CloseHandle(mFileMap); mFileMap = NULL;
	CloseHandle(mFileHandle); mFileHandle = INVALID_HANDLE_VALUE;
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
DTSTATUS
FileIoHelper::FIOReference(
IN BOOL ReadOnly,
IN LARGE_INTEGER Offset,
IN DWORD Size,
IN OUT PUCHAR& ReferencedPointer
)
{
	if (TRUE != Initialized()) return DTS_INVALID_OBJECT_STATUS;
	if (TRUE == IsReadOnly())
	{
		if (TRUE != ReadOnly)
		{
			log_err L"file handle is read-only!" log_end
				return DTS_INVALID_PARAMETER;
		}
	}

	_ASSERTE(NULL == mFileView);
	FIOUnreference();

	if (Offset.QuadPart + Size > mFileSize.QuadPart)
	{
		log_err
			L"invalid offset. file size=%I64d, req offset=%I64d",
			mFileSize.QuadPart, Offset.QuadPart
			log_end
			return DTS_INVALID_PARAMETER;
	}


	static DWORD AllocationGranularity = 0;
	if (0 == AllocationGranularity)
	{
		SYSTEM_INFO si = { 0 };
		GetSystemInfo(&si);
		AllocationGranularity = si.dwAllocationGranularity;
	}

	DWORD AdjustMask = AllocationGranularity - 1;
	LARGE_INTEGER AdjustOffset = { 0 };
	AdjustOffset.HighPart = Offset.HighPart;


	AdjustOffset.LowPart = (Offset.LowPart & ~AdjustMask);

	DWORD BytesToMap = (Offset.LowPart & AdjustMask) + Size;

	mFileView = (PUCHAR)MapViewOfFile(
		mFileMap,
		(TRUE == ReadOnly) ? FILE_MAP_READ : FILE_MAP_READ | FILE_MAP_WRITE,
		AdjustOffset.HighPart,
		AdjustOffset.LowPart,
		BytesToMap
		);
	if (NULL == mFileView)
	{
		log_err
			L"MapViewOfFile(high=0x%08x, log=0x%08x, bytes to map=%u) failed, gle=0x%08x",
			AdjustOffset.HighPart, AdjustOffset.LowPart, BytesToMap, GetLastError()
			log_end
			return DTS_WINAPI_FAILED;
	}

	ReferencedPointer = &mFileView[Offset.LowPart & AdjustMask];
	return DTS_SUCCESS;
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
void
FileIoHelper::FIOUnreference(
)
{
	if (NULL != mFileView)
	{
		UnmapViewOfFile(mFileView);
		mFileView = NULL;
	}
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
DTSTATUS
FileIoHelper::FIOReadFromFile(
IN LARGE_INTEGER Offset,
IN DWORD Size,
IN OUT PUCHAR Buffer
)
{
	_ASSERTE(NULL != Buffer);
	if (NULL == Buffer) return DTS_INVALID_PARAMETER;

	UCHAR* p = NULL;
	DTSTATUS status = FIOReference(TRUE, Offset, Size, p);
	if (TRUE != DT_SUCCEEDED(status))
	{
		log_err
			L"FIOReference() failed. status=0x%08x",
			status
			log_end
			return status;
	}

	__try
	{
		RtlCopyMemory(Buffer, p, Size);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		log_err
			L"exception. code=0x%08x", GetExceptionCode()
			log_end
			status = DTS_EXCEPTION_RAISED;
	}

	FIOUnreference();
	return status;
}

/**----------------------------------------------------------------------------
\brief

\param
\return
\code

\endcode
-----------------------------------------------------------------------------*/
DTSTATUS
FileIoHelper::FIOWriteToFile(
IN LARGE_INTEGER Offset,
IN DWORD Size,
IN PUCHAR Buffer
)
{
	_ASSERTE(NULL != Buffer);
	_ASSERTE(0 != Size);
	_ASSERTE(NULL != Buffer);
	if (NULL == Buffer || 0 == Size || NULL == Buffer) return DTS_INVALID_PARAMETER;

	UCHAR* p = NULL;
	DTSTATUS status = FIOReference(FALSE, Offset, Size, p);
	if (TRUE != DT_SUCCEEDED(status))
	{
		log_err
			L"FIOReference() failed. status=0x%08x",
			status
			log_end
			return status;
	}

	__try
	{
		RtlCopyMemory(p, Buffer, Size);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		log_err
			L"exception. code=0x%08x", GetExceptionCode()
			log_end
			status = DTS_EXCEPTION_RAISED;
	}

	FIOUnreference();
	return status;
}