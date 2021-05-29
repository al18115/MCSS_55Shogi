#pragma once
#pragma once
#include <string>
#include "MemMapFile.h"
#include <iostream>
#include <fstream>
#include <vector>

static constexpr size_t KB = 1 * 1024;
static constexpr size_t MB = KB * 1024;
static constexpr size_t GB = MB * 1024;

template <class T>
class MMapVector {
public:
	MMapVector();
	MMapVector(std::string filename);
	~MMapVector();
	void init(std::string filename);
	void fin();
	void read(size_t index, T& _inst);
	void write(size_t index, T _inst);
	size_t getCount() { return count; }
	bool isOpen() { return open; }
	void allLoad(void* ptr);
private:
	bool open = false;
	std::vector<CMemMapFile*> cmaps;
	std::vector<char*>ptrs;
	std::string filename;
	static const size_t maxSize = GB;
	DWORD size;
	//char* ptr;
	size_t count = 0;
	size_t sizeofT = sizeof(T);
	size_t maxCount = (maxSize) / sizeofT;

	void readCount();
	void writeCount();

	void addCMMF();

	char* getPoint(size_t index);
};


//https://www.wabiapp.com/WabiSampleSource/windows/string_to_wstring.html
static std::wstring StringToWString(std::string oString) {
	// SJIS → wstring
	int iBufferSize = MultiByteToWideChar(CP_ACP, 0, oString.c_str()
		, -1, (wchar_t*)NULL, 0);

	// バッファの取得
	wchar_t* cpUCS2 = new wchar_t[iBufferSize];

	// SJIS → wstring
	MultiByteToWideChar(CP_ACP, 0, oString.c_str(), -1, cpUCS2
		, iBufferSize);

	// stringの生成
	std::wstring oRet(cpUCS2, cpUCS2 + iBufferSize - 1);

	// バッファの破棄
	delete[] cpUCS2;

	// 変換結果を返す
	return(oRet);
}

template<class T>
MMapVector<T>::MMapVector() {
}

template<class T>
inline MMapVector<T>::MMapVector(std::string filename) {
	init(filename);
}

template<class T>
inline MMapVector<T>::~MMapVector(){
	fin();
}

template<class T>
void MMapVector<T>::init(std::string filename) {
	this->filename = filename;
	readCount();
	open = true;
}

template<class T>
inline void MMapVector<T>::fin(){
	writeCount();
}

template<class T>
inline void MMapVector<T>::read(size_t index, T& _inst) {
	T inst;
	memcpy(&_inst, getPoint(index), sizeof(T));
}

template<class T>
inline void MMapVector<T>::write(size_t index, T _inst) {
	T inst;
	auto t = getPoint(index);
	memcpy(t, &_inst, sizeof(T));

	if (count < index + 1) {
		count = index + 1;
	}
}

template<class T>
inline void MMapVector<T>::allLoad(void* _ptr){
	char* ptr = (char*)_ptr;
	size_t loadCount = count;
	for (int i = 0;;++i) {
		size_t loadCurrent = loadCount;
		if (loadCurrent > maxCount) {
			loadCurrent = maxCount;
		}
		memcpy(&(ptr[i * maxCount * sizeofT]), getPoint(i * maxCount), loadCurrent * sizeofT);
		if (loadCount < maxCount) {
			break;
		}
		else {
			loadCount -= maxCount;
		}
	}
}

template<class T>
inline void MMapVector<T>::readCount(){
	std::ifstream ifs(filename + "_count.txt");
	if (ifs.is_open()) {
		ifs >> count;
	}
	else {
		count = 0;
	}
	ifs.close();
}

template<class T>
inline void MMapVector<T>::writeCount(){
	std::ofstream ofs(filename + "_count.txt");
	if (ofs.is_open()) {
		ofs << count << std::endl;
	}
	ofs.close();
}

template<class T>
inline void MMapVector<T>::addCMMF()
{
	auto cmsize = cmaps.size();
	auto tfilename = filename + "_" + std::to_string(cmsize) + ".bin";
	auto fn = StringToWString(tfilename);
	CMemMapFile* cm = new CMemMapFile;
	if (!cm->Open(fn.c_str(), 0, maxSize)) {
		printf("%ls\n", fn.c_str());
		std::cout << "Error! Last Error Code:" << GetLastError() << std::endl;
		exit(1);
	}
	//cm.GetPtr((void**)&ptr, (filename + "_0.bin").c_str(), &size);
	char* ptr;
	cm->GetPtr((void**)&ptr);
	ptrs.push_back(ptr);
	cmaps.push_back(cm);
}

template<class T>
inline char* MMapVector<T>::getPoint(size_t index) {
	char* ptr;
	int fileNum = index / maxCount;
	int fileIndex = index % maxCount;
	while (cmaps.size() <= fileNum) {
		addCMMF();
	}
	return &(ptrs[fileNum][fileIndex * sizeofT]);
}
