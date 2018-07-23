#include <string>
#include <vector>
#include <iostream>
#include <map>
#include "../Common/iarchive.h"
#include "../Common/MyCom.h"
using namespace std;

class CompressExtract
{
public:
	CompressExtract();
	~CompressExtract();
	
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames, const wstring &load7zDllName);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"多个文件之间以'/'分开,若有和压缩
	bool ExtractFile(const wstring &archiveFileName, const wstring &outputPathName, const wstring &load7zDllName);//archiveFile="xxx.7z",解压文件到指定的目录，outputPathName以'\\'结尾
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring, int> &archivefilelist, const wstring &load7zDllName);//展示压缩文件里面的文件,map中first为文件名，second为大小
private:
	bool Load7zDLL(const wstring &load7zDllName);
	bool createObjectInit();
	void FileStringSepar(const wstring &fileNames);
	bool findOpenInit(const wstring &archiveFileName);
	bool filePathExist(const wstring &wsfileName, vector<wstring>& filesList);
	bool DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList);
	bool GetAllFiles();
	wstring FindCompressFilePath(const wstring  &filecompresspath, const wstring &filecompressFullpath);
	
private:

	HMODULE DllHandleName;
	Func_CreateObject _createObjectFunc;
	CMyComPtr<IInArchive> _archive;
	vector<wstring>_filename;
	vector<wstring>_allfileList;
	vector<wstring>_compressfilePath;
};