#pragma once
#include <Windows.h>

///////////////////////////////////
// メモリマップドファイルクラス
///////////
class CMemMapFile{
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
	virtual bool Open(char* filename, DWORD rwflag = GENERIC_READ | GENERIC_WRITE, DWORD openflag = OPEN_EXISTING);

	// ファイルポインタ取得
	virtual bool GetPtr(void** ptr, char* subfilename = NULL, DWORD* pfilesize = NULL);
};
