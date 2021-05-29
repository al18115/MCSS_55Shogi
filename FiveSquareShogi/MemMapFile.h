#pragma once
#include <Windows.h>

///////////////////////////////////
// メモリマップドファイルクラス
///////////
class CMemMapFile {
	//http://marupeke296.com/DXCLSSmp_MemoryMappedFile.html
protected:
	HANDLE m_hFile;
	HANDLE m_hMap;
	void* m_pPointer;
	DWORD m_dwFileSize;

public:
	CMemMapFile();
	virtual ~CMemMapFile();

	// ファイルオープン
	virtual bool Open(LPCWSTR filename, DWORD dwMaximumSizeHigh = 0, DWORD dwMaximumSizeLow = 0, DWORD rwflag = GENERIC_READ | GENERIC_WRITE, DWORD openflag = OPEN_ALWAYS);

	// ファイルポインタ取得
	virtual bool GetPtr(void** ptr, LPCWSTR subfilename = NULL, DWORD* pfilesize = NULL);
};
