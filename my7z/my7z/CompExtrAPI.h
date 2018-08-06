#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <windows.h>
using namespace std;



class Progress
{

};

class CompressExtract
{
public:
	CompressExtract();
	~CompressExtract();
	void cGetFullandCompleteSize();
	void eGetFullandCompleteSize();
	bool CompressFile(const wstring &archiveFileName, const wstring &fileNames, const wstring &load7zDllName);//archiveFile="xxx.7z",FileName="test1.txt test2.txt"多个文件之间以'/'分开,若有和压缩
	bool ExtractFile(const wstring &archiveFileName, const wstring &outputPathName, const wstring &load7zDllName);//archiveFile="xxx.7z",解压文件到指定的目录，outputPathName
	bool ShowArchivefileList(const wstring &archiveFileName, map<wstring, int> &archivefilelist, const wstring &load7zDllName);//展示压缩文件里面的文件,map中first为文件名，second为大小
public:
	unsigned long long _cFullSize;
	unsigned long long _cCompleteSize;
	unsigned long long _eFullSize;
	unsigned long long _eCompleteSize;
private:
	bool Load7zDLL(const wstring &load7zDllName);
	void FileStringSepar(const wstring &fileNames);
	bool filePathExist(const wstring &wsfileName, vector<wstring>& filesList);
	bool DirectoryPathExit(const wstring &wsDirName, vector<wstring>&filesList);
	bool GetAllFiles();
	wstring FindCompressFilePath(const wstring  &filecompresspath, const wstring &filecompressFullpath);
private:

	HMODULE DllHandleName;
	vector<wstring>_filename;
	vector<wstring>_allfileList;
	vector<wstring>_compressfilePath;
};